#include <fstream>

#include "PNS.h"
#include "limits.h"
#include "assert.h"
#include "common.h"



PNSNode::PNSNode(const Board& b, unsigned int d, int heur_val):children(), board(b), depth(d){
    type = b.node_type;
    unsigned int sum = 0;
    for(int i=0;i<ACTION_SIZE;i++){
        if(b.is_valid(i)) ++sum;
    }

    // === Init proof and disploof number ===
    if(sum == 0){ // Game is over, black wins
        pn = UINT_MAX;
        dn = 0;
    }
    else{
        pn = (type==OR ? 1:sum);
        //dn = (type==AND ? heur_val:sum);
        dn = (type==AND ? 1:sum);
    }

    parent_num = 1;
}

inline unsigned int get_child_value(PNSNode* child_node, const ProofType type){
    if(child_node == nullptr){
        return 1; // Improvement: Initialization policy
    }
    else if(type == PN){
        return child_node->pn;
    }
    else{
        return child_node->dn;
    }
}

unsigned int PNS::get_min_children(PNSNode* node, const ProofType type, bool index = false) const{
    unsigned int min = UINT_MAX;
    unsigned int min_ind = -1;

    for(int i=0;i<ACTION_SIZE;i++){
        if(!node->board.is_valid(i)) continue;

        unsigned int child = get_child_value(node->children[i], type);

        if(child < min ){
            min = child;
            min_ind = i;
        }
    }
    
    if(index) return min_ind;
    else return min;
}

unsigned int PNS::get_min_delta_index(PNSNode* node, int& second_ind) const{
    unsigned int first_min = UINT_MAX;
    unsigned int second_min = UINT_MAX;

    unsigned int first_ind = -1;
    second_ind = -1;

    for(int i=0;i<ACTION_SIZE;i++){
        if(!node->board.is_valid(i)) continue;
        // === take care of initialization ===
        unsigned int child = node->children[i] == nullptr ? 1: node->children[i]->delta();

        if(child < second_min ){
            second_min = child;
            second_ind = i;
            if(child < first_min){
                second_min = first_min;
                second_ind = first_ind;
                
                first_min = child;
                first_ind = i;
            }
        }
    }
    return first_min;
}


unsigned int PNS::get_sum_children(PNSNode* node, const ProofType type) const{
    unsigned int sum = 0;

    for(int i=0;i<ACTION_SIZE;i++){
        if(!node->board.is_valid(i)) continue;
        
        unsigned int child = get_child_value(node->children[i], type);

        if(child == UINT_MAX) return UINT_MAX;
        sum += child;
    }
    return sum;
}

inline void PNS::simplify_board(Board& next_state, const unsigned int action){
    if(next_state.node_type == OR){ // Last move: black

        next_state.remove_lines_with_two_ondegree(heuristic.all_linesinfo);
        next_state.remove_2lines_all(heuristic.all_linesinfo);
        next_state.remove_dead_fields(heuristic.linesinfo_per_field, action);

        //Board b1(next_state);
        //b1.remove_dead_fields(heuristic.linesinfo_per_field, action);
    }
}

void PNS::extend(PNSNode* node, const unsigned int action){
    Board next_state(node->board, action, get_player(node->type));

    simplify_board(next_state, action);
    
    Board reversed(next_state);
    reversed.flip();

    // === Find next_state in discovered states ===
    if(states.find(next_state) != states.end()){
        node->children[action] = states[next_state];
        node->children[action] -> parent_num += 1;
    }
    // === Find next_state in reversed discovered states ===
    else if (states.find(reversed) != states.end()){
        node->children[action] = states[reversed];
        node->children[action] -> parent_num += 1;
    }
    else{
        int heur_val = (int) floor(pow(2.0, 8*(next_state.heuristic_val(heuristic.all_linesinfo)-1)));
        node->children[action] = new PNSNode(next_state, node->depth+1, heur_val);

        if((next_state.node_type == AND) && next_state.white_win(get_lines(action))){
            node->children[action]->pn = 0;
            node->children[action]->dn = UINT_MAX;
            return;
        }
        else if((next_state.node_type == OR) && next_state.heuristic_stop(get_all_lines())){
            node->children[action]->pn = UINT_MAX;
            node->children[action]->dn = 0;
            return;
        }
        states[next_state] = node->children[action];

        // === life-and-death ===
        //std::cout<<node->depth<<std::endl;
        int act = next_state.one_way(get_all_lines());
        PNSNode* act_node = node->children[action];
        if(act > -1){
            extend(act_node, act);
            for(int i=0;i<ACTION_SIZE;i++){
                if(i == act) continue;
                // === This will leak memory ===
                Board b(act_node->board, i, get_player(act_node->type));
                act_node->children[i] = new PNSNode(b, act_node->depth+1, 1);
                if(states.find(b) != states.end()){
                    states[b] = act_node->children[i];
                }
                //extend(act_node, i);
                act_node->children[i]->pn = (act_node->type == AND ? 0 : UINT_MAX);
                act_node->children[i]->dn = (act_node->type == AND ? UINT_MAX : 0);
            }
        }
    }
}

void PNS::init_PN_search(PNSNode* node){
    add_state(node->board, node);
}

void PNS::PN_search(PNSNode* node){
    assert(node != nullptr);
    if(node->pn == 0 || node->dn == 0) return;

    if(node->type == OR){ // === OR  node ===
        unsigned int min_ind = get_min_children(node, PN, true);

        if(min_ind == (-1)) 0; // Disproof found
        else if(node->children[min_ind] == nullptr) extend(node, min_ind);
        else PN_search(node->children[min_ind]);
        // === Update PN and DN in node ===
        node->pn = get_min_children(node, PN);
        node->dn = get_sum_children(node, DN);

    }
    else{                 // === AND node ===
        unsigned int min_ind = get_min_children(node, DN, true);

        if(min_ind == (-1)) 0; // Proof found
        else if(node->children[min_ind] == nullptr) extend(node, min_ind);
        else PN_search(node->children[min_ind]);
        // === Update PN and DN in node ===
        node->pn = get_sum_children(node, PN);
        node->dn = get_min_children(node, DN);
    }

    // If PN or DN is 0, delete all unused descendents
    if(node->pn == 0 || node->dn == 0){
        delete_node(node);
    }
}

void PNS::delete_all(PNSNode* node){
    if( node->parent_num>1 ){
        node->parent_num -= 1;
    }
    else{
        for(int i=0;i<ACTION_SIZE;i++){
            if((!node->board.is_valid(i)) || (node->children[i]==nullptr)) continue;
            else{
                delete_all(node->children[i]);
                node->children[i]=nullptr;
            }
        }
        states.erase(node->board);
        delete node;
    }
}
void PNS::delete_node(PNSNode* node){
    // === In this case, we need max 1 branch ===
    if( ((node->type == OR) && (node->pn == 0)) ||
        ((node->type == AND) && (node->dn==0)) ){
        
        ProofType proof_type = (node->pn == 0 ? PN:DN);
        unsigned int min_ind = get_min_children(node, proof_type, true);

        assert(min_ind !=-1);
        // === Delete all children, except min_ind
        for(int i=0;i<ACTION_SIZE;i++){
            if((!node->board.is_valid(i)) || (node->children[i]==nullptr) ||
                (min_ind == i)) continue;
            else{
                delete_all(node->children[i]);
                node->children[i]=nullptr;
            }
        }
    }
    // ELSE: keep all children, because we need all of them
}

// === Helper Functions ===
void PNS::log_solution(std::string filename){
    std::ofstream log_file;
    log_file.open(filename);
    if (!log_file.is_open()) {
        std::cout<<"Problem with file (Check file/folder existency)\n";
    }

    for(auto & it: states){
        Board board = it.first;
        unsigned int pn = it.second->pn;
        unsigned int dn = it.second->dn;
        if(pn == 0 || dn == 0)
            log_file<<board.white<<" "<<board.black<<" "<<pn<<" "<<dn<<std::endl;
    }

    log_file.close();
}

void PNS::log_solution_min(PNSNode* node, std::ofstream& file){
    if(node == nullptr) return;
    else{
        file<<node->board.white<<" "<<node->board.black<<" "<<node->pn<<" "<<node->dn<<std::endl;
        
        if( ((node->type == OR) && (node->pn == 0)) ||
            ((node->type == AND) && (node->dn==0))){
            ProofType proof_type = (node->pn == 0 ? PN:DN);
            unsigned int min_ind = get_min_children(node, proof_type, true);
            //assert(node->children[min_ind] != nullptr);
            log_solution_min(node->children[min_ind], file);
        }
        else{
            for(int i=0;i<ACTION_SIZE;i++){
                if(!node->board.is_valid(i)) continue;
                //assert(node->children[i] != nullptr);

                log_solution_min(node->children[i], file);
            }
        }
    }
}

void PNS::add_state(const Board& b, PNSNode* node){
    if(states.find(b)==states.end()){
        states[b]=node;
    }
}

void PNS::free_states(){
    for(auto node_it: states){
        delete node_it.second;
    }
}
