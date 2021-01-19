#include "common.h"
#include "board.h"
#include "heuristic.h"



template<class T>
void print_mtx(mtx<T>& data, int prec=5){
    std::cout.precision(prec);
    std::cout << std::fixed;
    
    for(int j=0;j<ROW;j++){
        for(int i=0;i<COL;i++){
            std::cout<<data[i][j]<<" ";
        }
        std::cout<<"\n";
    }
}

void print_v(std::vector<int>& v){
    std::cout<<std::endl;
    for(int i=0;i<ROW;i++){
        for(int j=0;j<COL;j++){
            std::cout<<std::setw(3)<<v[j*ROW+i]<<" ";
        }
        std::cout<<std::endl;
    }
    std::cout<<std::endl;
}

unsigned long long set_full_board(){
    unsigned long long board = 0;
    for(int i=0;i<ROW*COL;i++) board|=((1ULL)<<i);
    return board;
}

void display(mtx<int>& board, bool end, std::pair<int, int> score, std::vector<int> show, bool nocolor){
    std::vector<bool> is_show(ACTION_SIZE, 0);
    for(auto f: show){
        is_show[f]=1;
    }

    const std::string WARNING = "\033[94m";
    const std::string FAIL = "\033[91m";
    const std::string ENDC = "\033[0m";


    int back_step = ROW+5; 
    printf("=== Gobang Game ===               \n");
    printf("  ");
    for(int i=0;i<COL;i++){
        std::string num = std::to_string(i);
        num = num.substr(num.size()-1,num.size());
        printf("%s ",num.c_str());
    }
    printf("\n");
    
    printf(" +%s+\n",std::string(2*COL,'=').c_str());
    for(int x =0;x<ROW;x++){
        printf("%d|",x);
        for(int y =0;y<COL;y++){
            int piece = board[y][x];
            std::string background = (is_show[y*ROW+x]?"\033[43m":"");
            if(piece>0){
                if(nocolor) printf("o ");
                else printf("%s%s%s\033[0m", WARNING.c_str(),background.c_str(),  "o ");
            }
            else if(piece<0){
                if(nocolor) printf("x ");
                else printf("%s%s%s\033[0m", FAIL.c_str(), background.c_str(), "x ");
            }
            else{
                if(nocolor) printf("  ");
                else printf("%s  \033[0m", background.c_str());
            }
        }
        printf("|\n");
    }
    printf(" +%s+\n",std::string(2*COL,'=').c_str());
    if (!end) printf("\033[%dA",back_step);
    printf("Score %d %d                           \n\n", score.first, score.second);
    printf("\033[1A");
}

void display(const board_int board, bool end, std::vector<int> show, bool nocolor){
    mtx<int> big_board;
    for(int x=0;x<ROW;x++){
        for(int y=0;y<COL;y++){
            int white = (board & ((1ULL)<<(y*ROW+x)))>0;
            big_board[y][x] = white;
        }
    }
    display(big_board, end, {0,0}, show, nocolor);
}

void display(const Board board, bool end, std::vector<int> show, bool nocolor){
    mtx<int> big_board;
    for(int x=0;x<ROW;x++){
        for(int y=0;y<COL;y++){
            int white = (board.white & ((1ULL)<<(y*ROW+x)))>0;
            int black = (board.black & ((1ULL)<<(y*ROW+x)))>0;
            big_board[y][x] = white-black;
        }
    }
    display(big_board, end, {board.score_left, board.score_right}, show, nocolor);
}


unsigned int selectBit(const board_int v, unsigned int r) {
    // Source: https://graphics.stanford.edu/~seander/bithacks.html
    // v - Input:  value to find position with rank r.
    // r - Input: bit's desired rank [1-64].
    unsigned int s;      // Output: Resulting position of bit with rank r [1-64]
    uint64_t a, b, c, d; // Intermediate temporaries for bit count.
    unsigned int t;      // Bit count temporary.

    // Do a normal parallel bit count for a 64-bit integer,
    // but store all intermediate steps.
    a =  v - ((v >> 1) & ~0UL/3);
    b = (a & ~0UL/5) + ((a >> 2) & ~0UL/5);
    c = (b + (b >> 4)) & ~0UL/0x11;
    d = (c + (c >> 8)) & ~0UL/0x101;
    t = (d >> 32) + (d >> 48);
    // Now do branchless select!
    s  = 64;
    s -= ((t - r) & 256) >> 3; r -= (t & ((t - r) >> 8));
    t  = (d >> (s - 16)) & 0xff;
    s -= ((t - r) & 256) >> 4; r -= (t & ((t - r) >> 8));
    t  = (c >> (s - 8)) & 0xf;
    s -= ((t - r) & 256) >> 5; r -= (t & ((t - r) >> 8));
    t  = (b >> (s - 4)) & 0x7;
    s -= ((t - r) & 256) >> 6; r -= (t & ((t - r) >> 8));
    t  = (a >> (s - 2)) & 0x3;
    s -= ((t - r) & 256) >> 7; r -= (t & ((t - r) >> 8));
    t  = (v >> (s - 1)) & 0x1;
    s -= ((t - r) & 256) >> 8;
    return s;
}


static const unsigned char BitReverseTable256[256] = 
{
#   define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#   define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#   define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
    R6(0), R6(2), R6(1), R6(3)
};

unsigned int flip_bit(board_int v){
    unsigned int c; // c will get v reversed

    // Option 1:
    c = (BitReverseTable256[v & 0xff] << 24) | 
        (BitReverseTable256[(v >> 8) & 0xff] << 16) | 
        (BitReverseTable256[(v >> 16) & 0xff] << 8) |
        (BitReverseTable256[(v >> 24) & 0xff]);

    // Option 2:
    unsigned char * p = (unsigned char *) &v;
    unsigned char * q = (unsigned char *) &c;
    q[3] = BitReverseTable256[p[0]]; 
    q[2] = BitReverseTable256[p[1]]; 
    q[1] = BitReverseTable256[p[2]]; 
    q[0] = BitReverseTable256[p[3]];
    
    return c;
}