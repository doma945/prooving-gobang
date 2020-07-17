#include "board.h"
#include "assert.h"

NodeType operator!(const NodeType& type){
    return (type==OR?AND:OR);
}

int get_player(const NodeType& type){
    return (type==OR?1:-1);
}

bool operator<(const Board& b1, const Board& b2) {
    return (b1.node_type < b2.node_type) || ((b1.node_type == b2.node_type) && b1.white<b2.white) || ( (b1.node_type == b2.node_type) && b1.white == b2.white && b1.black<b2.black);
}

//bool operator<(const Board& b1, const Board& b2) {
//    return (b1.white<b2.white) || (b1.white == b2.white && b1.black<b2.black);
//}

bool Board::heuristic_stop(const std::vector<Line_info>& all_lines) const{
    double sum = 0;
    for(auto line: all_lines){
        bool is_free = !(line.line_board & black);
        if(!is_free) continue;
        else{
            int emptynum = line.size - __builtin_popcountll(line.line_board & white);
            sum += std::pow(2.0,-emptynum);
            if(sum>=1.0) return false;
        }
    }

    // The sum is under 1, the game is over
    return true;
}

int Board::one_way(const std::vector<Line_info>& all_lines) const{
    std::vector<bool> two_line(ACTION_SIZE, 0);
    
    for(auto line: all_lines){
        bool is_free = !(line.line_board & black);
        if(!is_free) continue;
        else{
            int emptynum = line.size - __builtin_popcountll(line.line_board & white);
            for(int field: line.points){
                if(white & (1ULL << field)) continue;

                if(emptynum == 1){
                    return field;
                }
                else if(emptynum == 2){
                    if(two_line[field]) return field;
                    else two_line[field] = 1;
                }
            }
        }
    }

    // No obvious action
    return -1;
}

void Board::flip(){
    board_int w=0,b=0;
    board_int col = 0x0f;

    for(int i=0;i<COL;i++){
        board_int old_w = (white & (col<<(4*i)));
        board_int old_b = (black & (col<<(4*i)));
        int move = (COL-2*(i+1)+1)*4;
        if(move>=0){
            w |= (old_w<<move);
            b |= (old_b<<move);
        }
        else{
            move = -move;
            w |= (old_w>>move);
            b |= (old_b>>move);
        }
    }
    white = w;
    black = b;
    //white = static_cast<board_int>(flip_bit(white))>>FLIP_SIZE;
    //black = static_cast<board_int>(flip_bit(black))>>FLIP_SIZE;
}


// === Policy ===
std::array<float, ACTION_SIZE> Board::heuristic_mtx(const std::vector<Line_info>& lines) const{
    // Returns a heuristic value for every possible action
    std::array<float, ACTION_SIZE> mtx= {0};

    for(auto line: lines){
        bool is_free = !(line.line_board & black);
        if(!is_free) continue;
        else{
            int emptynum = line.size - __builtin_popcountll(line.line_board & white);
            for(int field: line.points){
                mtx[field] += std::pow(2.0,-emptynum);
            }
        }
    }
    return mtx;
}

double Board::heuristic_val(const std::vector<Line_info>& lines) const{
    double sum = 0.0;
    for(auto line: lines){
        bool is_free = !(line.line_board & black);
        if(!is_free) continue;
        else{
            int emptynum = line.size - __builtin_popcountll(line.line_board & white);
            sum += std::pow(2.0,-emptynum);
        }
    }
    return sum;
}

// ==============================================
//                 REMOVE DEAD 2 LINE
// ==============================================
void Board::remove_dead_fields_line(const Line_info& line, const std::vector<unsigned int>& field_linesum){
    for(auto field: line.points){
        if(field_linesum[field] == 1){
            set_black(field);
        }
    }
}

void Board::remove_dead_fields(const std::array<std::vector<Line_info>, ACTION_SIZE>& linesinfo_per_field,
                        const int action){
    // === For all lines, which cross the action ===
    for(auto line: linesinfo_per_field[action]){
        //  === Skip for not empty line (empty: except action) ===
        board_int new_black = (black ^ (1ULL) << action);
        if(line.line_board & new_black){ // Line not empty
            continue;
        }

        // === For every field on the line ===
        for(auto field: line.points){
            // === If field not empty, continue
            //if((black & ((1ULL) << field)) || (white & ((1ULL) << field))) continue;
            if(black & ((1ULL) << field)) continue;
            
            unsigned int free_lines = 0;
            for(auto side_line: linesinfo_per_field[field]){
                bool is_free = !(side_line.line_board & black);
                if(is_free) free_lines++;
            }
            if(free_lines == 0){
                set_black(field);
            }
        }
    }
}



void Board::remove_2lines_all(const std::vector<Line_info>& all_line){
    std::vector<unsigned int> free_num(ACTION_SIZE, 0);

    for(auto line: all_line){
        bool is_free = !(line.line_board & black);
        if(is_free){
            for(auto field: line.points){
                free_num[field] +=1;
            }
        }
    }

    bool rerun = false;
    for(auto line: all_line){
        bool is_free = !(line.line_board & black);
        int emptynum = line.size - __builtin_popcountll(line.line_board & white);
        if(is_free && (emptynum == 2)){
            for(auto field: line.points){
                if((free_num[field]==1) && !(white & (1ULL << field))){
                    int other_empty = find_empty(line, field);
                    move(other_empty, 1);
                    move(field, -1);
                    remove_dead_fields_line(line, free_num);
                    rerun = true;
                    //remove_2lines_all(all_line);
                    //return;
                }
            }
        }
    }
    
    //if(rerun) remove_2lines_all(all_line);
}


void Board::remove_2lines(const std::array<std::vector<Line_info>, ACTION_SIZE>& linesinfo_per_field,
                   const int action){
    // === For all lines, which cross the action ===
    for(auto line: linesinfo_per_field[action]){
        // === If the line was dead before "action", continue ===
        board_int new_black = (black ^ (1ULL) << action);
        if(line.line_board & new_black){ // Line not empty
            continue;
        }

        for(auto field: line.points){
            if((white & (1ULL << field))) continue;

            unsigned int free_lines = 0;
            int emptynum = 0;
            Line_info act_line;
            for(auto side_line: linesinfo_per_field[field]){
                bool is_free = !(side_line.line_board & black);
                if(is_free){
                    emptynum = side_line.size - __builtin_popcountll(side_line.line_board & white);
                    free_lines++;
                    act_line = side_line;
                }
            }

            if(free_lines == 1 && (emptynum == 2)){
                // Delete field
                // Find other and move there one step, and call remove_2_lines
                int other_empty = find_empty(act_line, field);
                move(other_empty, 1);
                move(field, -1);
                remove_dead_fields(linesinfo_per_field, field);
                remove_2lines(linesinfo_per_field, other_empty);
            }
        }
    }
}

// ==============================================
//       REMOVE LINES WITH 2 1-DEGREE NODES
// ==============================================
void Board::remove_lines_with_two_ondegree(const std::vector<Line_info>& all_line){
    std::vector<unsigned int> free_num(ACTION_SIZE, 0);

    for(auto line: all_line){
        bool is_free = !(line.line_board & black);
        if(is_free){
            for(auto field: line.points){
                free_num[field] +=1;
            }
        }
    }

    bool rerun = false;
    for(auto line: all_line){
        bool is_free = !(line.line_board & black);
        int emptynum = line.size - __builtin_popcountll(line.line_board & white);
        if(is_free){
            int deg_1 = -1;
            for(auto field: line.points){
                if((free_num[field]==1) && !(white & (1ULL << field))){
                    if(deg_1 > -1){
                        move(field, 1);
                        move(deg_1, -1);
                        remove_dead_fields_line(line, free_num);
                        rerun = true;
                    }
                    else{
                        deg_1 = field;
                    }
                }

            }
        }
    }
    
    //if(rerun) remove_lines_with_two_ondegree(all_line);
}



// ==============================================
//               SPLIT TO COMPONENTS
// ==============================================
bool valid_action(int action){
    return action >=0 && action < ACTION_SIZE;
}

void get_component(const std::vector<std::array<bool,11>>& adjacent_nodes,
                    std::vector<int>& node_component,
                    int start, int act_component){
    for(int dir=0;dir<11;dir++){
        int neigh = start+dir-5; // dir = node - neigh + 5
        if(adjacent_nodes[start][dir] && node_component[neigh] ==-1 ){
            node_component[neigh] = act_component;
            get_component(adjacent_nodes, node_component, neigh, act_component);
        }
    }
}

std::vector<double> get_component_sum(const std::vector<Line_info>& all_lines,
                                      const std::vector<int>& emptynum_in_line,
                                      const std::vector<int>& first_field_in_line,
                                      const std::vector<int>& node_component,
                                      const int num_component){
    std::vector<double> component_sum(num_component);
    for(int i=0;i<all_lines.size();i++){
        int act_comp = node_component[first_field_in_line[i]];
        int empty_num = emptynum_in_line[i];
        if(empty_num > -1){
            component_sum[act_comp] += std::pow(2.0, -empty_num);
        }
    }
    return component_sum;
}

std::vector<int> Board::get_all_components(const std::vector<std::array<bool,11>>& adjacent_nodes, const std::vector<bool>& free_node, int& num_component){
    std::vector<int> node_component(ACTION_SIZE, -1);
    // === Get components ===
    for(int ind = 0; ind<ACTION_SIZE; ind++){
        if(!free_node[ind] || node_component[ind] != -1 || !is_valid(ind)) continue;
        else{
            node_component[ind] = num_component;
            get_component(adjacent_nodes, node_component, ind, num_component);
            num_component++;
        }
    }
    return node_component;
}

void Board::get_fields_and_lines(const std::vector<Line_info>& all_lines,
                           std::vector<int>& emptynum_in_line,
                           std::vector<int>& first_field_in_line,
                           std::vector<bool>& free_node,
                           std::vector<std::array<bool,11>>& adjacent_nodes){
    // ======== ADJACENCY TABLE ==========
    // node1-node2 : if node1 > node2 [+5]
    // -5 -1  3       0  4  8
    // -4  #  4  ==>  1  #  9
    // -3  1  5       2  6  10black |= ((1ULL) << i);

    int iter = 0;    
    // === Iterate on lines ===
    for(auto line: all_lines){
        bool is_free = !(line.line_board & black);

        if(!is_free){
            emptynum_in_line[iter] = -1;
        }
        else{
            // === This line is free, update it's fileds
            int emptynum = line.size - __builtin_popcountll(line.line_board & white);
            emptynum_in_line[iter] = emptynum;
            first_field_in_line[iter] = line.points[0];

            free_node[line.points[0]] = true;
            for(int i=1;i<line.points.size();i++){ // We doesn't matter with lines with length 1
                int act = line.points[i];
                int prev = line.points[i-1];
                free_node[act] = true;
                
                int direction = prev-act + 5; // Only for 4xn table
                adjacent_nodes[act][direction] = true;
                direction = act-prev + 5;     // Only for 4xn table
                adjacent_nodes[prev][direction] = true;
            }
        }
        iter += 1;
    }
}

void Board::remove_small_components(const std::vector<Line_info>& all_lines){
    std::vector<int> emptynum_in_line(all_lines.size());         // -1 if not empty
    std::vector<int> first_field_in_line(all_lines.size(), -1);  // -1 if ther is none
    std::vector<bool> free_node(ACTION_SIZE, 0);
    std::vector<std::array<bool,11>> adjacent_nodes(ACTION_SIZE);// 3 and 7 unused

    // === INIT line and field features ===
    get_fields_and_lines(all_lines, emptynum_in_line, first_field_in_line, free_node, adjacent_nodes);

    // === Split to components ===
    int num_component = 0;
    std::vector<int> node_component = get_all_components(adjacent_nodes, free_node, num_component);

    // === SUM lines on components ===
    std::vector<double> component_sum = get_component_sum(all_lines, emptynum_in_line, first_field_in_line, node_component, num_component);

    // === Delete small components ===
    for(unsigned int i=0;i<ACTION_SIZE;i++){
        int act_component = node_component[i];
        // === If small component, make all values black ===
        if(act_component >= 0 && component_sum[act_component] >= 1.0){
            // filed is in a big component
        }
        else{
            if((white & ((1ULL) << i))>0){
                white = white ^ ((1ULL) << i);
            }
            //move(i,-1);
            black |= ((1ULL) << i);
        }
    }

    if(0){
        // === Print Comp ===
        for(int i=0;i<ROW;i++){
            for(int j=0;j<COL;j++){
                std::cout<<node_component[j*ROW+i]<<" ";
            }
            std::cout<<std::endl;
        }

        for(auto comp_size: component_sum){
            std::cout<<comp_size<<" ";
        }
        std::cout<<std::endl;
    }

}
