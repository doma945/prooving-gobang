#ifndef BOARD_H_
#define BOARD_H_

#include <stdio.h>
#include <cstdint>
#include <math.h>
#include <cmath>
#include <assert.h>

#include "common.h"

enum NodeType : uint8_t {OR, AND};
int get_player(const NodeType& type);
NodeType operator!(const NodeType& type);

bool operator<(const Board& b1, const Board& b2);

struct Board{
    board_int white;
    board_int black;
    NodeType node_type;

    Board(){
        init();
    }

    Board(const Board& b){
        white = b.white;
        black = b.black;
        node_type = b.node_type;
    }

    Board(const Board& b, int action, int player){
        white = b.white;
        black = b.black;
        node_type = b.node_type;
        move(action, player);
    }

    Board& operator=(const Board&& b){
        white = b.white;
        black = b.black;
        node_type = b.node_type;
        return *this;
    }

    inline void init(){
        white = 0;
        black = 0;
        node_type = OR;
    }

    // === ACTION FUNCTIONS ===
    inline void move(const int action, const int player){
        assert(player == get_player(node_type));
        
        if(player == 1) white |= ((1ULL)<<action);
        else if (player == -1) black |= ((1ULL)<<action);
        else{
            std::cout<<"Bad player\n";
        }
        node_type = (!node_type);
    }

    inline void move(std::vector<int> actions, int& player){
        for(auto& act: actions){
            move(act, player);
            player = -player;
        }
    }

    inline int random_action(){
        board_int valids = get_valids();
        int number_of_ones =__builtin_popcountll(valids);
        return selectBit(valids, 1 + (rand() % number_of_ones))-1;
    }

    inline int take_random_action(int player){
        int act = random_action();
        move(act, player);
        return act;
    }

    inline bool is_valid(const int action) const{
        return !((white | black) & ((1ULL)<<action));
    }

    inline board_int get_valids(){
        return ~(white | black) & FULL_BOARD;
    }

    // === GAME OVER FUNCTIONS===
    bool white_win(const std::vector<board_int> & lines)const {
        for(auto line: lines){
            bool blocked = (line & black);
            if(!blocked && (__builtin_popcountll(line & white)==__builtin_popcountll(line))){
                return true;
            }
        }
        return false;
    }

    inline bool black_win() const {
        // === No free field ===
        return __builtin_popcountll(white | black) == MAX_ROUND;
    }

    int get_winner(const std::vector<board_int> & lines) const {
        if(white_win(lines)) return 1;
        else if (black_win())
        {
            return -1;
        }
        else return 0;
        
    }

    // === TODO with saved constant array ===
    bool no_free_lines(const std::vector<std::pair<board_int, unsigned int>>& all_lines) const{
        for(auto line: all_lines){
            bool is_free = !(line.first & black);
            if(is_free) return false;
        }
        return true;
    }

    bool heuristic_stop(const std::vector<std::pair<board_int, unsigned int>>& all_lines) const;
    void flip();

    // === Policy ===
    std::array<float, ACTION_SIZE> heuristic_mtx(const std::vector<Line_info>& lines) const;
    
    // ==============================================
    //               SPLIT TO COMPONENTS
    // ==============================================
    void get_fields_and_lines(const std::vector<Line_info>& all_lines,
                           std::vector<int>& emptynum_in_line,
                           std::vector<int>& first_field_in_line,
                           std::vector<bool>& free_node,
                           std::vector<std::array<bool,11>>& adjacent_nodes);

    std::vector<int> get_all_components(const std::vector<std::array<bool,11>>& adjacent_nodes,
                                        const std::vector<bool>& free_node,
                                        int& num_component);
    void remove_small_components(const std::vector<Line_info>& all_lines);
};


#endif