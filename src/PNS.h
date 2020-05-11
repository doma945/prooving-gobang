#include "common.h"
#include "board.h"
#include <map>

enum NodeType : uint8_t {OR, AND};

struct PNSNode{

    PNSNode(const Board& b):children(), board(b){}

    PNSNode* children[ACTION_SIZE];
    Board board;

    unsigned int pn = 1;
    unsigned int dn = 1;
    NodeType type;
};

class PNS{
public:
    void search(PNSNode* node);
    bool valid(){return true;}

private :
    std::map<Board, PNSNode*> states;
};