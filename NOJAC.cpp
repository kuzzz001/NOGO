#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#pragma GCC target "avx2,fma"
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <x86intrin.h>
#include <cstring>
#include <unordered_map>
#include "jsoncpp/json.h"

using namespace std;
using namespace std::chrono;

#define ONE 0
#define TWO 1

typedef __uint128_t uint128_t;

void printx(uint128_t board) {
    for (int i=0; i < 81; i++) {
        uint128_t p = uint128_t(1) << i;
        if (board&p) cout << "1";
        else cout << ".";
        if ((i+1)%9 == 0) {
            cout << endl;
        }
    }
}

const uint128_t BOARD = (uint128_t(131071ULL) << 64) | 18446744073709551615ULL;
const uint128_t TOP_ROW = 511ULL;
const uint128_t BOTTOM_ROW = uint128_t(511ULL) << 72;
const uint128_t LEFT_COLUMN = 9241421688590303745ULL | (uint128_t(256ULL) << 64);
const uint128_t RIGHT_COLUMN = LEFT_COLUMN << 8;

uint128_t shift(uint128_t board, int dir) {
    static const uint128_t MASKS[] = {
        (~RIGHT_COLUMN) & BOARD,
        (~BOTTOM_ROW) & BOARD,
        (~TOP_ROW) & BOARD,
        (~LEFT_COLUMN) & BOARD,
    };

    static const uint128_t LSHIFTS[] = {
        0,
        0,
        9,
        1,
    };

    static const uint128_t RSHIFTS[] = {
        1,
        9,
        0,
        0,
    };

    if (dir < 4 / 2) {
        return (board >> RSHIFTS[dir]) & MASKS[dir];
    } else {
        return (board << LSHIFTS[dir]) & MASKS[dir];
    }
}

int rot[81] = {
    72, 63, 54, 45, 36, 27, 18, 9, 0,
    73, 64, 55, 46, 37, 28, 19, 10, 1,
    74, 65, 56, 47, 38, 29, 20, 11, 2,
    75, 66, 57, 48, 39, 30, 21, 12, 3,
    76, 67, 58, 49, 40, 31, 22, 13, 4,
    77, 68, 59, 50, 41, 32, 23, 14, 5,
    78, 69, 60, 51, 42, 33, 24, 15, 6,
    79, 70, 61, 52, 43, 34, 25, 16, 7,
    80, 71, 62, 53, 44, 35, 26, 17, 8,
};

int sym[81] = {
    8, 7, 6, 5, 4, 3, 2, 1, 0,
    17, 16, 15, 14, 13, 12, 11, 10, 9,
    26, 25, 24, 23, 22, 21, 20, 19, 18,
    35, 34, 33, 32, 31, 30, 29, 28, 27,
    44, 43, 42, 41, 40, 39, 38, 37, 36,
    53, 52, 51, 50, 49, 48, 47, 46, 45,
    62, 61, 60, 59, 58, 57, 56, 55, 54,
    71, 70, 69, 68, 67, 66, 65, 64, 63,
    80, 79, 78, 77, 76, 75, 74, 73, 72,
};

uint128_t PEXT2[64] = {};
int BASE32[512*512] = {};

inline uint64_t pext(const uint64_t & x, const uint64_t & mask){
    uint64_t ret;
    asm("pext %1,%2,%0":"=r"(ret):"r"(mask), "r"(x));
    return ret;
}

uint128_t _pext_u128(uint128_t val, uint128_t mask) {
    uint128_t res = pext((uint64_t)val,(uint64_t)mask);
    res |= pext(val>>64,mask>>64) << __builtin_popcountll((uint64_t)mask);
    return res;
}

void generatePext() {
    int n = 0;
    for (int i=0; i < 81; i++) {
        if (i%9 > 7) continue;
        if (i/9 > 7) continue;
        uint128_t p = uint128_t(1) << i;
        p |= uint128_t(1) << (i+1);
        p |= uint128_t(1) << (i+9);
        p |= uint128_t(1) << (i+10);
        PEXT2[n++] = p;
    }
    for (int one=0; one < 16; one++) {
        for (int two=0; two < 16; two++) {
            if (one&two) continue;
            for (int three = 0; three < 16; three++) {
                for (int four=0; four < 16; four++) {
                    int tuple = 0;
                    int power = 1;
                    for (uint64_t j=0; j < 4; j++) {
                        if (one&(1ULL<<j)) {
                            tuple += power * 1;
                        } else if (two&(1ULL<<(j))) {
                            tuple += power * 2;
                        } else {
                            bool a = three&(1ULL<<j);
                            bool b = four&(1ULL<<j);
                            if (a&&b) tuple += power * 3;
                            else if (a) tuple += power * 4;
                            else if (b) tuple += power * 5;
                        }
                        power *= 6;
                    }
                    BASE32[one*4096+two*256+three*16+four] = tuple;
                }
            }
        }
    }
}

const uint128_t LEFT_COL = 9241421688590303745ULL | (uint128_t(256ULL) << 64);
const uint128_t RIGHT_COL = LEFT_COL << 8;

inline uint128_t get_neighbors(uint128_t b) {
    return ((b << 1) & ~LEFT_COL) | ((b >> 1) & ~RIGHT_COL) | (b << 9) | (b >> 9);
}

struct Board {
    uint128_t oneBoard = 0;
    uint128_t twoBoard = 0;

    explicit Board() {

    }

    void makeMove(int player, int move) {
        if (player == ONE) {
            oneBoard |= uint128_t(1) << move;
        } else {
            twoBoard |= uint128_t(1) << move;
        }
    }

    void undoMove(int player, int move) {
        if (player == ONE) {
            oneBoard ^= uint128_t(1) << move;
        } else {
            twoBoard ^= uint128_t(1) << move;
        }
    }

    // bool isOver() {
    //     uint128_t empty = ~(oneBoard|twoBoard)&BOARD;
    //     uint128_t o = (shift(empty,0)|shift(empty,1)|shift(empty,2)|shift(empty,3)) & oneBoard;
    //     while (true) {
    //         uint128_t x = o;
    //         o |= (shift(o,0)|shift(o,1)|shift(o,2)|shift(o,3)) & oneBoard;
    //         if (x == o) break;
    //     }
    //     if (o != oneBoard) return true;
    //     printx(o);
    //     uint128_t t = (shift(empty,0)|shift(empty,1)|shift(empty,2)|shift(empty,3)) & twoBoard;
    //     while (true) {
    //         uint128_t x = t;
    //         t |= (shift(t,0)|shift(t,1)|shift(t,2)|shift(t,3)) & twoBoard;
    //         if (x == t) break;
    //     }
    //     if (t != twoBoard) return true;
    //     return false;
    // }
    
    // Checks if all stones in 'stones' have a path to 'empty' squares
    bool allGroupsHaveLiberties(uint128_t stones, uint128_t empty) {
        if (!stones) return true;

        // Start from stones adjacent to empty spaces (seeds)
        uint128_t reachable = get_neighbors(empty) & stones;

        // Flood fill: Expand seeds through the stone mask
        uint128_t last;
        do {
            last = reachable;
            reachable |= get_neighbors(reachable) & stones;
        } while (reachable != last);

        return reachable == stones;
    }

    bool isMoveOk(int player, int move) {
        uint128_t p = uint128_t(1) << move;
        uint128_t myNewBoard = (player == ONE ? oneBoard : twoBoard) | p;
        uint128_t oppBoard = (player == TWO ? oneBoard : twoBoard);
        uint128_t empty = ~(myNewBoard | oppBoard) & BOARD;

        // 1. Check if the opponent's groups are still legal
        if (!allGroupsHaveLiberties(oppBoard, empty)) return false;

        // 2. Check if your own groups are still legal
        // Optimization: If the placed stone is next to an empty square,
        // it's guaranteed a liberty.
        if (!(get_neighbors(p) & empty)) {
            if (!allGroupsHaveLiberties(myNewBoard, empty)) return false;
        }

        return true;
    }

    bool isMoveOk_(int player, int move) {
        uint128_t p = uint128_t(1) << move;
        if (player == ONE) {
            uint128_t oneBoard = this->oneBoard | p;
            uint128_t empty = ~(oneBoard|twoBoard)&BOARD;
            uint128_t o = (shift(empty,0)|shift(empty,1)|shift(empty,2)|shift(empty,3)) & oneBoard;
            while (true) {
                uint128_t x = o;
                o |= (shift(o,0)|shift(o,1)|shift(o,2)|shift(o,3)) & oneBoard;
                if (x == o) break;
            }
            if (o != oneBoard) return false;
            uint128_t t = (shift(empty,0)|shift(empty,1)|shift(empty,2)|shift(empty,3)) & twoBoard;
            while (true) {
                uint128_t x = t;
                t |= (shift(t,0)|shift(t,1)|shift(t,2)|shift(t,3)) & twoBoard;
                if (x == t) break;
            }
            if (t != twoBoard) return false;
        } else {
            uint128_t twoBoard = this->twoBoard | p;
            uint128_t empty = ~(oneBoard|twoBoard)&BOARD;
            uint128_t o = (shift(empty,0)|shift(empty,1)|shift(empty,2)|shift(empty,3)) & oneBoard;
            while (true) {
                uint128_t x = o;
                o |= (shift(o,0)|shift(o,1)|shift(o,2)|shift(o,3)) & oneBoard;
                if (x == o) break;
            }
            if (o != oneBoard) return false;
            uint128_t t = (shift(empty,0)|shift(empty,1)|shift(empty,2)|shift(empty,3)) & twoBoard;
            while (true) {
                uint128_t x = t;
                t |= (shift(t,0)|shift(t,1)|shift(t,2)|shift(t,3)) & twoBoard;
                if (x == t) break;
            }
            if (t != twoBoard) return false;
        }
        return true;
    }

    void print() {
        for (int i=0; i < 81; i++) {
            uint128_t p = uint128_t(1) << i;
            if (oneBoard&p) cout << "1";
            else if (twoBoard&p) cout << "2";
            else cout << ".";
            if ((i+1)%9 == 0) {
                cout << endl;
            }
        }
    }
};

struct Game {
    Board board;
    int currentPlayer = ONE;
    int rounds = 0;
    vector<int> history;

    explicit Game() {
        history.reserve(81);
    }
    
    uint64_t getHash() {
        __uint128_t a = __uint128_t(3852213006194032149ULL) * (board.oneBoard<<40);
        __uint128_t b = __uint128_t(8124055821ULL) * board.twoBoard;
        uint64_t c = (a >> 64) ^ uint64_t(a);
        uint64_t d = (b >> 64) ^ uint64_t(b);
        return uint64_t(c^d);
    }

    void setGameNoHistory(Game & game) {
        this->board = game.board;
        this->currentPlayer = game.currentPlayer;
        this->rounds = game.rounds;
    }

    vector<int> getAvailableMoves() {
        vector<int> availableMoves; availableMoves.reserve(81);
        uint128_t empty = ~(board.oneBoard|board.twoBoard)&BOARD;
        for (int i=0; i < 81; i++) {
            uint128_t p = uint128_t(1) << i;
            if (p&empty) {
                if (board.isMoveOk(currentPlayer, i)) {
                    availableMoves.push_back(i);
                }
            }
        }
        return availableMoves;
    }

    int getWinner() {
        return currentPlayer^1;
    }

    bool isOver() {
        return !anyMoveAvailable();
    }

    bool anyMoveAvailable() {
        uint128_t empty = ~(board.oneBoard|board.twoBoard)&BOARD;
        for (int i=0; i < 81; i++) {
            uint128_t p = uint128_t(1) << i;
            if (p&empty) {
                if (board.isMoveOk(currentPlayer, i)) {
                    return true;
                }
            }
        }
        return false;
    }

    void makeMove(int move) {
        history.push_back(move);
        board.makeMove(currentPlayer, move);
        currentPlayer ^= 1;
        rounds++;
    }

    void makeMoveNoHistory(int move) {
        board.makeMove(currentPlayer, move);
        currentPlayer ^= 1;
        rounds++;
    }

    void undoMove() {
        int move = history.back();
        history.pop_back();
        currentPlayer ^= 1;
        rounds--;
        board.undoMove(currentPlayer, move);
    }

    vector<int> getTuplesBase() {
        vector<int> tuples; tuples.reserve(81);

        uint128_t oneBoard = currentPlayer == TWO ? board.oneBoard : board.twoBoard;
        uint128_t twoBoard = currentPlayer == TWO ? board.twoBoard : board.oneBoard;

        for (int i=0; i < 81; i++) {
            uint128_t p = uint128_t(1) << i;
            if (p&oneBoard) tuples.push_back(2*i);
            else if (p&twoBoard) tuples.push_back(2*i+1);
        }

        return tuples;
    }

    vector<int> getTuples() {
        vector<int> tuples; tuples.reserve(81);

        uint128_t oneBoard = currentPlayer == ONE ? board.oneBoard : board.twoBoard;
        uint128_t twoBoard = currentPlayer == ONE ? board.twoBoard : board.oneBoard;

        for (int i=0; i < 81; i++) {
            uint128_t p = uint128_t(1) << i;
            if (p&oneBoard) tuples.push_back(2*i);
            else if (p&twoBoard) tuples.push_back(2*i+1);
        }

        return tuples;
    }
    
    uint128_t getMoveForPlayer_(int player) {
        uint128_t moves = 0;
        uint128_t empty = ~(board.oneBoard|board.twoBoard)&BOARD;
        for (int i=0; i < 81; i++) {
            uint128_t p = uint128_t(1) << i;
            if (p&empty) {
                if (board.isMoveOk(player, i)) {
                    moves |= p;
                }
            }
        }
        return moves;
    }
    
    uint128_t getMoveForPlayer(int player) {
        uint128_t moves = 0;
        uint128_t empty = ~(board.oneBoard|board.twoBoard)&BOARD;

        uint64_t low = (uint64_t)empty;
        uint64_t high = (uint64_t)(empty >> 64);

        auto scan = [&](uint64_t bits, int offset) {
            while (bits) {
                int i = __builtin_ctzll(bits);
                if (board.isMoveOk(player, i + offset)) {
                    moves |= uint128_t(1) << (i + offset);
                }
                bits &= (bits - 1);
            }
        };

        scan(low, 0);
        scan(high, 64);
        return moves;
    }

    vector<int> getTuples2x2() {
        vector<int> tuples(64);

        if (currentPlayer == ONE) {
            uint128_t a = getMoveForPlayer(ONE);
            uint128_t b = getMoveForPlayer(TWO);
            for (int i=0; i < 64; i++) {
                uint64_t one = _pext_u128(board.oneBoard,PEXT2[i]);
                uint64_t two = _pext_u128(board.twoBoard,PEXT2[i]);
                uint64_t three = _pext_u128(a,PEXT2[i]);
                uint64_t four = _pext_u128(b,PEXT2[i]);
                int tuple = BASE32[one*4096+two*256+three*16+four];
                tuples[i] = 1296 * i + tuple;
            }
        } else {
            uint128_t a = getMoveForPlayer(TWO);
            uint128_t b = getMoveForPlayer(ONE);
            uint128_t oneBoard = (this->board.twoBoard);
            uint128_t twoBoard = (this->board.oneBoard);
            for (int i=0; i < 64; i++) {
                uint64_t one = _pext_u128(oneBoard,PEXT2[i]);
                uint64_t two = _pext_u128(twoBoard,PEXT2[i]);
                uint64_t three = _pext_u128(a,PEXT2[i]);
                uint64_t four = _pext_u128(b,PEXT2[i]);
                int tuple = BASE32[one*4096+two*256+three*16+four];
                tuples[i] = 1296 * i + tuple;
            }
        }

        return tuples;
    }

    vector<int> getTuples2x2Base() {
        vector<int> tuples(64);

        if (currentPlayer == TWO) {
            uint128_t a = getMoveForPlayer(ONE);
            uint128_t b = getMoveForPlayer(TWO);
            for (int i=0; i < 64; i++) {
                uint64_t one = _pext_u128(board.oneBoard,PEXT2[i]);
                uint64_t two = _pext_u128(board.twoBoard,PEXT2[i]);
                uint64_t three = _pext_u128(a,PEXT2[i]);
                uint64_t four = _pext_u128(b,PEXT2[i]);
                int tuple = BASE32[one*4096+two*256+three*16+four];
                tuples[i] = 1296 * i + tuple;
            }
        } else {
            uint128_t a = getMoveForPlayer(TWO);
            uint128_t b = getMoveForPlayer(ONE);
            uint128_t oneBoard = (this->board.twoBoard);
            uint128_t twoBoard = (this->board.oneBoard);
            for (int i=0; i < 64; i++) {
                uint64_t one = _pext_u128(oneBoard,PEXT2[i]);
                uint64_t two = _pext_u128(twoBoard,PEXT2[i]);
                uint64_t three = _pext_u128(a,PEXT2[i]);
                uint64_t four = _pext_u128(b,PEXT2[i]);
                int tuple = BASE32[one*4096+two*256+three*16+four];
                tuples[i] = 1296 * i + tuple;
            }
        }

        return tuples;
    }
};

static const double NORM_DOUBLE = 1.0 / (1ULL << 53);
static const float NORM_FLOAT = 1.0f / (1ULL << 24);

class Random {
public:
    Random() {
        seed = 685404112622437557ULL * (uint64_t)this + 54321ULL;
        seed ^= 9876542111ULL * rand() + 9632587411000005ULL;
        if (seed == 0ULL) {
            seed = 3210123505555ULL;
        }
    }

    Random(uint64_t seed) {
        if (seed == 0ULL) {
            seed = 3210123505555ULL;
        }
        this->seed = seed;
    }

    uint64_t nextLong() {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return seed * 0x2545F4914F6CDD1DULL;
    }

    uint32_t nextInt(int n) {
        uint32_t x = nextLong() >> 32;
        uint64_t m = uint64_t(x) * uint64_t(n);
        return m >> 32;
    }

    double nextDouble() {
        return (nextLong() >> 11) * NORM_DOUBLE;
    }

    double nextDouble(double mi, double ma) {
        return mi + nextDouble() * (ma - mi);
    }

    float nextFloat() {
        return (float)((nextLong() >> 40) * NORM_FLOAT);
    }

    float nextFloat(float mi, float ma) {
        return mi + nextFloat() * (ma - mi);
    }

private:
    uint64_t seed;
};

#define INF 10000

class MoveMctsTTR {
public:
    int index; // 4
    int parent; // 4

    float heuristic; // 4
    int games; // 4
    float score; // 4

    int childStart; // 4
    int8_t player; // 1
    int8_t move; // 1
    int8_t childrenSize; // 1
    bool terminal; // 1

    explicit MoveMctsTTR(int parent, int player, const int & move) : parent(parent), player(player), move(move) {
        terminal = false;
        games = 0;
        heuristic = 0;
        score = 0;
        childStart = -1;
        childrenSize = -1;
        index = -1;
    }

    void updateScore(vector<MoveMctsTTR> & movesPool, float score = 0) {
        this->games += 1;
        this->score += score;
        if (childrenSize == -1) {
            return;
        }
        float h = -INF;
        bool toTerminate = true;

        for (int i=0; i < childrenSize; i++) {
            int index = (childStart+i) % (movesPool.size());
            auto & c = movesPool[index];
            h = max(h,c.heuristic);
            if (c.heuristic > INF/2) {
                toTerminate = true;
            }
            toTerminate &= c.terminal;
        }

        this->heuristic = this->player == movesPool[childStart].player ? h : -h;
        this->terminal = toTerminate || h > INF/2;
    }

    float updateScore2(vector<MoveMctsTTR> & movesPool) {
        this->games += 1;
        float h = -INF;
        bool toTerminate = true;

        for (int i=0; i < childrenSize; i++) {
            int index = (childStart+i) % (movesPool.size());
            auto & c = movesPool[index];
            h = max(h,c.heuristic);
            if (c.heuristic > INF/2) {
                toTerminate = true;
            }
            toTerminate &= c.terminal;
        }

        this->heuristic = this->player == movesPool[childStart].player ? h : -h;
        float s = this->heuristic;
        if (s < -1) s = -1;
        if (s > 1) s = 1;
        this->score += s;
        this->terminal = toTerminate || h > INF/2;
        return s;
    }
};

static inline float hsum256_ps(__m256 v) {
    __m128 vlow  = _mm256_castps256_ps128(v);
    __m128 vhigh = _mm256_extractf128_ps(v, 1);
    vlow = _mm_add_ps(vlow, vhigh);
    __m128 shuf = _mm_movehdup_ps(vlow);
    __m128 sums = _mm_add_ps(vlow, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

class Network {
public:
    const int inputs,hidden;
    int type = 0;
    vector<int16_t> hiddenWeights;
    vector<float> outputWeights;

    vector<float> cacheScores;

    explicit Network(int inputs, int hidden) : inputs(inputs), hidden(hidden) {
        hiddenWeights.resize(inputs * hidden);
        outputWeights.resize(hidden);
        cacheScores.resize(64 * hidden);
    }

    void load(string name) {
        ifstream plik; plik.open(name);
        {
            vector<char> result(sizeof(int16_t) * hiddenWeights.size());
            plik.read(&result[0], sizeof(int16_t) * hiddenWeights.size());
            memcpy(&hiddenWeights[0], &result[0], sizeof(int16_t) * hiddenWeights.size());
        }
        {
            vector<char> result(sizeof(float) * outputWeights.size());
            plik.read(&result[0], sizeof(float) * outputWeights.size());
            memcpy(&outputWeights[0], &result[0], sizeof(float) * outputWeights.size());
        }
        plik.close();
    }

    void cacheScore(const vector<int> & indexes, int id) {
        float* cache = &cacheScores[id * hidden];
        const __m256 zero = _mm256_setzero_ps();

        for (int i = 0; i < hidden; i += 8)
            _mm256_storeu_ps(cache + i, zero);

        for (int idx : indexes) {
            const int16_t* row = &hiddenWeights[idx * hidden];
            for (int i = 0; i < hidden; i += 8) {
                __m128i v16 = _mm_loadu_si128((__m128i*)(row + i));
                __m256i v32 = _mm256_cvtepi16_epi32(v16);
                __m256  vf  = _mm256_cvtepi32_ps(v32);

                __m256 vcache = _mm256_loadu_ps(cache + i);
                vcache = _mm256_add_ps(vcache, vf);
                _mm256_storeu_ps(cache + i, vcache);
            }
        }
    }

    float getScoreDiff(const vector<int> & indexesBase, const vector<int> & indexes, int id) {
        static thread_local vector<float> scores;
        if (static_cast<int>(scores.size()) < hidden)
            scores.resize(hidden);
        float* s = scores.data();

        const __m256 zero = _mm256_setzero_ps();
        for (int i = 0; i < hidden; i += 8)
            _mm256_storeu_ps(s + i, zero);

        for (int idx : indexesBase) {
            const int16_t* row = &hiddenWeights[idx * hidden];
            for (int i = 0; i < hidden; i += 8) {
                __m128i v16 = _mm_loadu_si128((__m128i*)(row + i));
                __m256i v32 = _mm256_cvtepi16_epi32(v16);
                __m256  vf  = _mm256_cvtepi32_ps(v32);

                __m256 vs = _mm256_loadu_ps(s + i);
                vs = _mm256_sub_ps(vs, vf);
                _mm256_storeu_ps(s + i, vs);
            }
        }

        for (int idx : indexes) {
            const int16_t* row = &hiddenWeights[idx * hidden];
            for (int i = 0; i < hidden; i += 8) {
                __m128i v16 = _mm_loadu_si128((__m128i*)(row + i));
                __m256i v32 = _mm256_cvtepi16_epi32(v16);
                __m256  vf  = _mm256_cvtepi32_ps(v32);

                __m256 vs = _mm256_loadu_ps(s + i);
                vs = _mm256_add_ps(vs, vf);
                _mm256_storeu_ps(s + i, vs);
            }
        }

        const float* cache = &cacheScores[id * hidden];
        const __m256 v_inv8192 = _mm256_set1_ps(1.0f / 4096.0f);
        const __m256 v_leaky   = _mm256_set1_ps(0.01f);
        const __m256 vzero     = _mm256_setzero_ps();

        for (int i = 0; i < hidden; i += 8) {
            __m256 vcache = _mm256_loadu_ps(cache + i);
            __m256 vs     = _mm256_loadu_ps(s + i);
            __m256 vt     = _mm256_add_ps(vcache, vs);
            vt = _mm256_mul_ps(vt, v_inv8192);     

            __m256 vsq   = _mm256_mul_ps(vt, vt);
            __m256 vleak = _mm256_mul_ps(vt, v_leaky);
            __m256 mask  = _mm256_cmp_ps(vt, vzero, _CMP_LT_OS);
            __m256 vres  = _mm256_blendv_ps(vsq, vleak, mask);
            _mm256_storeu_ps(s + i, vres);
        }

        __m256 vout = _mm256_setzero_ps();
        for (int i = 0; i < hidden; i += 8) {
            __m256 vw = _mm256_loadu_ps(&outputWeights[i]);
            __m256 vs = _mm256_loadu_ps(s + i);
            vout = _mm256_fmadd_ps(vw, vs, vout);
        }
        float output = hsum256_ps(vout);
        return fast_tanh(output);
    }

    float relu(float x) {
        if (x < 0) return 0.01f * x;
        return x*x;
    }

    float fast_tanh(float x) {
        if (x > 4.95f) return 1;
        if (x < -4.95f) return -1;
        float x2 = x * x;
        float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        return a / b;
    }
};


class CpuMctsTTR {
public:
    int id = 0;
    Network *agent = nullptr;
    bool train = false;
    Random ran;
    const int SIZE;

    vector<MoveMctsTTR> movesPool = vector<MoveMctsTTR>(SIZE,MoveMctsTTR(-1,-1,-1));
    int ccc = 0;

    float alpha = 0.4f;
    float FPU = 0.08f;
    float C = 0.2f;
    float Croot = 0.25f;

    int games;
    int maxLevel;
    bool provenEnd;

    int getMove(int parent, int player, const int & m) {
        auto move = &movesPool[ccc];
        move->index = ccc;
        move->parent = parent;
        move->player = player;
        move->move = m;
        move->heuristic = 0;
        move->games = 0;
        move->score = 0;
        move->terminal = false;
        move->childStart = -1;
        move->childrenSize = -1;
        ccc = ((ccc+1)%(SIZE));
        return move->index;
    }

    explicit CpuMctsTTR(int SIZE = 1048576) : SIZE(SIZE) {

    }

    void setPlayer(int player) {
        this->player = player;
    }

    int getPlayer() {
        return player;
    }

    void setGame(Game *game) {
        this->game = game;
    }

    MoveMctsTTR *lastMove=nullptr;
    int lastOpponentMove = -1;
    int found = 0;
    unordered_map<uint64_t,pair<int,int>> tt;

    MoveMctsTTR* getBestMove(long timeInMicro, bool print = false) {
        auto start = high_resolution_clock::now();
        vector<MoveMctsTTR*> moves;
        pair<int,int> childs;

        games = 0;
        maxLevel = 0;
        provenEnd = false;
        tt.clear();

        if (lastMove != nullptr && lastMove->childrenSize != -1) {
            MoveMctsTTR *opponent = nullptr;
            auto children = getMoves(lastMove->childStart,lastMove->childrenSize);
            for (auto & m : children) {
                if (m->move == lastOpponentMove) {
                    opponent = &movesPool[m->index];
                    break;
                }
            }
            if (opponent != nullptr && opponent->childrenSize != -1) {
                moves = getMoves(opponent->childStart, opponent->childrenSize);
                games = opponent->games;
                for (auto & child : moves) child->parent = -1;
                childs = make_pair(opponent->childStart,opponent->childrenSize);
            }
        }
        lastOpponentMove = -1;
        lastMove = nullptr;

        if (moves.size() == 0) {
            childs = generateMoves(-1);
            moves = getMoves(childs.first, childs.second);
            found = 0;
        } else {
            // cerr << "found " << games << endl;
        }

        long duration = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
        Game copy = *game;
        while (!provenEnd && duration < timeInMicro) {
            selectAndExpand(childs.first, childs.second, games+1,0);
            games++;
            *game = copy;
            duration = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
        }

        sort(moves.begin(),moves.end(), [](const MoveMctsTTR *a, const MoveMctsTTR *b) -> bool
             {
                 float avg1 = a->heuristic + log(a->games+3);
                 float avg2 = b->heuristic + log(b->games+3);
                 return avg1 > avg2;
             });

        if (print)
            for (auto & m : moves) {
                cout << ": " << m->score/m->games << " " << m->heuristic << " " << m->games << endl;
            }

        //                cout << "games: " << games << endl;
        //                cout << "maxLevel: " << maxLevel << endl;

        lastMove = moves[0];
        return moves[0];
    }

    MoveMctsTTR* getBestMoveIterations(int iterations, double temp = 3.0, bool print = false) {
        vector<MoveMctsTTR*> moves;

        games = 0;
        maxLevel = 0;
        provenEnd = false;
        tt.clear();

        pair<int,int> childs;
        if (moves.size() == 0) {
            childs = generateMoves(-1);
            moves = getMoves(childs.first, childs.second);
            found = 0;
        } else {
            // cerr << "found " << games << endl;
        }

        Game copy = *game;
        while (!provenEnd && games < iterations) {
            selectAndExpand(childs.first, childs.second, games+1,0);
            games++;
            *game = copy;
        }

        sort(moves.begin(),moves.end(), [](const MoveMctsTTR *a, const MoveMctsTTR *b) -> bool
             {
                 float avg1 = a->heuristic + log(a->games+3);
                 float avg2 = b->heuristic + log(b->games+3);
                 return avg1 > avg2;
             });

        if (print) {
            for (auto & m : moves) {
                cout << ": " << m->score/m->games << " " << m->heuristic << " " << m->games << endl;
            }
        }

        if (train && !moves[0]->terminal) {
            vector<double> values;
            for (auto & m : moves) {
                if (m->terminal) break;
                values.push_back(m->heuristic + log(m->games+3));
            }
            vector<double> soft = softmax(values,temp);
            int n = pick(soft);
            auto & m = moves[n];
            moves[0]->move = m->move;
            return moves[0];
        }

        lastMove = moves[0];
        return moves[0];
    }

    vector<double> softmax(const vector<double> & input, double t) {
        vector<double> output;
        double sum = 0;
        double ma = -999;
        for (auto & i : input) ma = max(ma,i);
        for (auto & i : input) sum += exp(t*(i - ma));
        for (auto & i : input) {
            output.push_back(exp(t*(i - ma))/sum);
        }
        return output;
    }

    int pick(const vector<double> & input) {
        double cutoff = 0;
        double point = ran.nextDouble();
        for (size_t i = 0; i < input.size()-1; i++){
            cutoff += input[i];
            if (point < cutoff) return i;
        }
        return input.size()-1;
    }

private:
    int player;
    Game *game;

    vector<MoveMctsTTR*> getMoves(int childStart, int childrenSize) {
        vector<MoveMctsTTR*> moves; moves.reserve(childrenSize);
        for (int i=0; i < childrenSize; i++) {
            int index = (childStart+i) % (movesPool.size());
            moves.push_back(&movesPool[index]);
        }
        return moves;
    }

    pair<int,int> generateMoves(int parent) {
        vector<int> movesNum = game->getAvailableMoves();
        int player = game->currentPlayer;
        int childrenSize = 0;
        int childStart = -1;
        uint64_t hash = game->getHash();
        auto & cachedMoves = tt[hash];
        if (true && cachedMoves.second > 0) {
            int cStart = cachedMoves.first;
            childrenSize = cachedMoves.second;
            for (int m=0; m < childrenSize; m++) {
                int i = (cStart+m) % (movesPool.size());
                auto & cachedMove = movesPool[i];
                int childIndex = getMove(parent,player,cachedMove.move);
                auto & move = movesPool[childIndex];
                move.heuristic = cachedMove.heuristic;
                move.score = cachedMove.score;
                move.games = cachedMove.games;
                move.terminal = cachedMove.terminal;
                if (childStart == -1) childStart = childIndex;
            }
            return make_pair(childStart,childrenSize);
        }
        vector<int> tsBase = game->getTuples2x2Base();
        agent->cacheScore(tsBase,id);
        for (auto & moveNum : movesNum) {
            game->makeMove(moveNum);
            if (game->isOver()) {
                int winner = game->getWinner();
                if (winner == player) {
                    int childIndex = getMove(parent,player,moveNum);
                    auto & move = movesPool[childIndex];
                    move.heuristic = INF - 10 * game->rounds;
                    move.score = 1;
                    move.games = 1;
                    move.terminal = true;
                    if (childStart == -1) childStart = childIndex;
                    childrenSize++;
                    game->undoMove();
                    cachedMoves.first = childStart;
                    cachedMoves.second = childrenSize;
                    return make_pair(childStart,childrenSize);
                } else {
                    int childIndex = getMove(parent,player,moveNum);
                    auto & move = movesPool[childIndex];
                    move.heuristic = -INF + 10 * game->rounds;
                    move.score = -1;
                    move.games = 1;
                    move.terminal = true;
                    if (childStart == -1) childStart = childIndex;
                    childrenSize++;
                }
            } else {
                int childIndex = getMove(parent,player,moveNum);
                auto & move = movesPool[childIndex];
                tsDiffBase.clear();
                tsDiff.clear();

                auto ts = game->getTuples2x2();
                for (int i=0; i < 64; i++) {
                    if (tsBase[i] != ts[i]) {
                        tsDiffBase.push_back(tsBase[i]);
                        tsDiff.push_back(ts[i]);
                    }
                }
                float h2 = agent->getScoreDiff(tsDiffBase,tsDiff,id);
                move.heuristic = -h2;

                if (childStart == -1) childStart = childIndex;
                childrenSize++;
            }
            game->undoMove();
        }
        cachedMoves.first = childStart;
        cachedMoves.second = childrenSize;
        return make_pair(childStart,childrenSize);
    }

    vector<int> tsDiffBase;
    vector<int> tsDiff;

    vector<int> indexes;
    void selectAndExpand(int childStart, int childSize, int games, int level) {
        while (true) {
            maxLevel = max(maxLevel,level);
            indexes.clear();
            float maxArg = -2 * INF;
            float a,b;
            float t = log(games);
            for (int m=0; m < childSize; m++) {
                int i = (childStart+m) % (movesPool.size());
                MoveMctsTTR *move = &movesPool[i];
                if (move->terminal) {
                    a = move->heuristic / move->games;
                    if (a == 0.0) a = -1.5;
                    b = 0.0;
                } else {
                    if (move->games == 0) {
                        a = move->heuristic;
                        if (train) {
                            a *= ran.nextFloat(0.9f, 1.1f);
                        }
                        b = level == 0 ? 1.0 : FPU;
                    } else {
                        a = alpha * move->heuristic + (1-alpha) * move->score / move->games;
                        if (train) {
                            a *= ran.nextFloat(0.8f, 1.2f);
                        } else {
                            a *= ran.nextFloat(0.9f, 1.1f);
                        }
                        b = (level == 0 ? (Croot) : C) * sqrtf(t/move->games);
                    }
                }

                if (a + b > maxArg) {
                    maxArg = a + b;
                    indexes.clear();
                    indexes.push_back(i);
                } else if (a + b == maxArg) {
                    indexes.push_back(i);
                }
            }
            MoveMctsTTR *move = &movesPool[indexes[ran.nextInt(indexes.size())]];
            if (move->terminal) {
                move->games += 1;
                move->score += move->heuristic > INF/2 ? 1 : move->heuristic < -INF/2 ? -1 : 0;
                if (level == 0) {
                    if (move->heuristic > INF/2) {
                        provenEnd = true;
                        return;
                    }
                    bool allTerminal = true;
                    for (int m=0; m < childSize; m++) {
                        int i = (childStart+m) % (movesPool.size());
                        MoveMctsTTR *move = &movesPool[i];
                        if (!move->terminal) {
                            allTerminal = false;
                            break;
                        }
                    }
                    if (allTerminal) {
                        provenEnd = true;
                        return;
                    }
                }
                MoveMctsTTR *parent = move->parent == -1 ? nullptr : &movesPool[move->parent];
                while (parent != nullptr) {
                    parent->updateScore(movesPool, parent->player == move->player ? (move->heuristic > INF/2 ? 1 : move->heuristic < -INF/2 ? -1 : 0) : (move->heuristic > INF/2 ? -1 : move->heuristic < -INF/2 ? 1 : 0));
                    parent = parent->parent == -1 ? nullptr : &movesPool[parent->parent];
                }
                return;
            }
            game->makeMoveNoHistory(move->move);
            if (move->games > 0) {
                if (move->childrenSize == -1) {
                    auto children = generateMoves(move->index);
                    move->childStart = children.first;
                    move->childrenSize = children.second;
                }
                childStart = move->childStart;
                childSize = move->childrenSize;
                level = level+1;
                games = move->games+1;
                // selectAndExpand(move->childStart, move->childrenSize, move->games+1, level+1);
            } else {
                move->games += 1;
                move->score += move->heuristic;
                MoveMctsTTR *parent = move->parent == -1 ? nullptr : &movesPool[move->parent];
                while (parent != nullptr) {
                    parent->updateScore(movesPool, parent->player == move->player ? move->heuristic : -move->heuristic);
                    parent = parent->parent == -1 ? nullptr : &movesPool[parent->parent];
                }
                return;
            }
            // game->undoMove();
        }
    }
};


int main() {
    srand(time(NULL) ^ uint64_t(&main));

    generatePext();

    Json::Reader reader;
    Json::FastWriter writer;
    Json::Value input;
    Json::Value result;

    Network* agent = new Network(1296*64,256);
    agent->load("data/256_model6i");

    Game game;
    CpuMctsTTR cpu(8000000);
    cpu.id = 0;
    cpu.agent = agent;
    cpu.setPlayer(ONE);
    cpu.setGame(&game);

    bool firstTurn = true;
    string line;
    while (cin >> line) {
        reader.parse(line, input);
        int x,y;
        if (firstTurn) {
            x = input["requests"][(Json::Value::UInt) 0]["x"].asInt();
            y = input["requests"][(Json::Value::UInt) 0]["y"].asInt();
        } else {
            x = input["x"].asInt();
            y = input["y"].asInt();
        }
        if (firstTurn && (x != -1 || y != -1)) {
            cpu.setPlayer(TWO);
        }
        if (x != -1 && y != -1) {
            game.makeMove(9 * x + y);
            cpu.lastOpponentMove = 9 * x + y;
        }

        auto moveNum = cpu.getBestMove(100*9800L);
        int move = moveNum->move;
        game.makeMove(move);

        firstTurn = false;
        result["response"]["x"] = move / 9;
        result["response"]["y"] = move % 9;
        result["debug"]["games"] = cpu.games;
        result["debug"]["ml"] = cpu.maxLevel;
        result["debug"]["ccc"] = cpu.ccc;
        result["debug"]["score"] = moveNum->heuristic;
        cout << writer.write(result) << endl;
        cout << "\n>>>BOTZONE_REQUEST_KEEP_RUNNING<<<\n";
        fflush(stdout);
    }

    return 0;
}