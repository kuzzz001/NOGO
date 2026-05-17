#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

// ================= 全局数据 =================
int ansX, ansY;
int limitDepth;

int board[9][9] = {0};

const int BLACK = 1;
const int WHITE = -1;

int currentPlayer;
int rootPlayer;

const int dirX[4] = {1, -1, 0, 0};
const int dirY[4] = {0, 0, 1, -1};

int mark[9][9];
int markId = 0;

// ================= 计时 =================
chrono::steady_clock::time_point beginTime;
const int TIME_LIMIT = 950;

inline int usedTime() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - beginTime
    ).count();
}

// ================= 随机数 =================
unsigned int seedNum = 19260817;

inline void initRand(unsigned int s) {
    seedNum = s;
}

inline int nextRand() {
    seedNum ^= seedNum << 13;
    seedNum ^= seedNum >> 17;
    seedNum ^= seedNum << 5;
    return seedNum & 0x7fffffff;
}

inline int randint(int mod) {
    return nextRand() % mod;
}

inline int colorId(int c) {
    return (c == BLACK ? 1 : 0);
}

// ================= 判断气 =================
bool hasLiberty(int sx, int sy, int color) {

    int stackX[100];
    int stackY[100];
    int top = 0;

    stackX[top] = sx;
    stackY[top] = sy;
    top++;

    mark[sx][sy] = markId;

    while (top) {

        top--;

        int x = stackX[top];
        int y = stackY[top];

        for (int d = 0; d < 4; d++) {

            int nx = x + dirX[d];
            int ny = y + dirY[d];

            if (nx < 0 || nx >= 9 || ny < 0 || ny >= 9)
                continue;

            if (mark[nx][ny] == markId)
                continue;

            if (board[nx][ny] == 0)
                return true;

            if (board[nx][ny] != color)
                continue;

            mark[nx][ny] = markId;

            stackX[top] = nx;
            stackY[top] = ny;
            top++;
        }
    }

    return false;
}

// ================= 判断是否合法 =================
bool legalMove(int x, int y, int color) {

    if (x < 0 || x >= 9 || y < 0 || y >= 9)
        return false;

    if (board[x][y] != 0)
        return false;

    board[x][y] = color;

    bool ok = true;

    markId++;

    if (!hasLiberty(x, y, color)) {
        ok = false;
    }

    if (ok) {

        for (int d = 0; d < 4; d++) {

            int nx = x + dirX[d];
            int ny = y + dirY[d];

            if (nx < 0 || nx >= 9 || ny < 0 || ny >= 9)
                continue;

            if (board[nx][ny] == -color) {

                markId++;

                if (!hasLiberty(nx, ny, -color)) {
                    ok = false;
                    break;
                }
            }
        }
    }

    board[x][y] = 0;

    return ok;
}

// ================= 判断终局 =================
bool noMove(int color) {

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {

            if (legalMove(i, j, color))
                return false;
        }
    }

    return true;
}

// ================= 合法点计数 =================
int legalCount(int color) {

    int cnt = 0;

    for (int i = 0; i < 9; i++) {

        for (int j = 0; j < 9; j++) {

            if (legalMove(i, j, color))
                cnt++;
        }
    }

    return cnt;
}

// ================= 局面估值 =================
// NoGo 本质：逼对方无路可走
// 估值 = 我方合法点 - 对方合法点
double evaluate(int color) {

    int myLegal = legalCount(color);
    int oppLegal = legalCount(-color);

    double score = (myLegal - oppLegal) * 2.5;

    return 1.0 / (1.0 + exp(-score / 5.0));
}

// ================= 小残局搜索 =================
bool dfsSolve(int step) {

    if (usedTime() > TIME_LIMIT * 0.8)
        return false;

    bool exist = false;

    for (int i = 0; i < 9; i++) {

        for (int j = 0; j < 9; j++) {

            if (!legalMove(i, j, currentPlayer))
                continue;

            exist = true;

            board[i][j] = currentPlayer;
            currentPlayer = -currentPlayer;

            bool res = dfsSolve(step + 1);

            if (!res) {

                if (step == 1) {
                    ansX = i;
                    ansY = j;
                }

                board[i][j] = 0;
                currentPlayer = -currentPlayer;

                return true;
            }

            board[i][j] = 0;
            currentPlayer = -currentPlayer;
        }
    }

    if (!exist)
        return false;

    return false;
}

// ================= MCTS 节点 =================
#define MAX_NODE 1200000

struct TreeNode {

    int px, py;

    bool fullExpand;

    int childCnt;

    int child[82];

    double winValue;

    double visitCnt;

    void reset(int x, int y) {

        px = x;
        py = y;

        fullExpand = false;

        childCnt = 0;

        winValue = 0;

        visitCnt = 0;
    }

} tree[MAX_NODE];

int currentNode = 0;
int totalNode = 0;

// ================= 合法动作缓存 =================
int moveCnt[2];
int moveList[2][81];

int backupCnt[2];
int backupMove[2][81];

// ================= 选择扩展 =================
int expandTree() {

    int side = colorId(currentPlayer);

    if (!tree[currentNode].fullExpand) {

        int tmpCnt = moveCnt[side];

        int tmpMove[81];

        memcpy(tmpMove, moveList[side], sizeof(tmpMove));

        for (int i = 1; i <= tree[currentNode].childCnt; i++) {

            int son = tree[currentNode].child[i];

            int pos = tree[son].px * 9 + tree[son].py;

            int* p = find(tmpMove, tmpMove + tmpCnt, pos);

            if (p != tmpMove + tmpCnt) {
                *p = tmpMove[--tmpCnt];
            }
        }

        while (true) {

            if (tmpCnt == 0) {

                if (tree[currentNode].childCnt == 0)
                    return -1;

                tree[currentNode].fullExpand = true;

                break;
            }

            int bestIdx = -1;
            int bestPressure = -1;
            int bestX, bestY;
            int sampleCnt = min(tmpCnt, 5);

            for (int s = 0; s < sampleCnt; s++) {

                int id = randint(tmpCnt);

                int pos = tmpMove[id];

                int x = pos / 9;
                int y = pos % 9;

                if (legalMove(x, y, currentPlayer)) {

                    int before = legalCount(-currentPlayer);
                    board[x][y] = currentPlayer;
                    int after = legalCount(-currentPlayer);
                    board[x][y] = 0;

                    int pressure = before - after;

                    if (pressure > bestPressure) {
                        bestPressure = pressure;
                        bestIdx = id;
                        bestX = x;
                        bestY = y;
                    }

                    if (pressure >= 6) break;
                }
                else {

                    tmpMove[id] = tmpMove[--tmpCnt];

                    if (tmpCnt == 0) break;
                }
            }

            if (bestIdx != -1) {

                if (totalNode + 1 >= MAX_NODE)
                    return -1;

                tree[currentNode].childCnt++;

                int nxt = ++totalNode;

                tree[currentNode].child[tree[currentNode].childCnt] = nxt;

                tree[nxt].reset(bestX, bestY);

                currentNode = nxt;

                return 1;
            }
            else {

                if (tmpCnt == 0) continue;

                int id = randint(tmpCnt);

                int pos = tmpMove[id];

                int x = pos / 9;
                int y = pos % 9;

                if (legalMove(x, y, currentPlayer)) {

                    if (totalNode + 1 >= MAX_NODE)
                        return -1;

                    tree[currentNode].childCnt++;

                    int nxt = ++totalNode;

                    tree[currentNode].child[tree[currentNode].childCnt] = nxt;

                    tree[nxt].reset(x, y);

                    currentNode = nxt;

                    return 1;
                }
                else {
                    tmpMove[id] = tmpMove[--tmpCnt];
                }
            }
        }
    }

    double bestScore = -1e18;
    int bestNode = -1;

    double lg = log(max(1.0, tree[currentNode].visitCnt));

    for (int i = 1; i <= tree[currentNode].childCnt; i++) {

        int son = tree[currentNode].child[i];

        double ucb =
            tree[son].winValue / tree[son].visitCnt +
            0.9 * sqrt(lg / tree[son].visitCnt);

        if (ucb > bestScore) {
            bestScore = ucb;
            bestNode = son;
        }
    }

    if (bestNode == -1)
        return -1;

    currentNode = bestNode;

    return 0;
}

// ================= MCTS =================
void runMCTS() {

    totalNode = 0;

    tree[0].reset(-1, -1);

    int path[120];

    backupCnt[0] = backupCnt[1] = 0;

    for (int i = 0; i < 9; i++) {

        for (int j = 0; j < 9; j++) {

            if (legalMove(i, j, BLACK))
                backupMove[1][backupCnt[1]++] = i * 9 + j;

            if (legalMove(i, j, WHITE))
                backupMove[0][backupCnt[0]++] = i * 9 + j;
        }
    }

    while (usedTime() < TIME_LIMIT) {

        memcpy(moveCnt, backupCnt, sizeof(moveCnt));
        memcpy(moveList, backupMove, sizeof(moveList));

        currentNode = 0;

        int dep = 0;

        path[0] = 0;

        while (dep < limitDepth) {

            int state = expandTree();

            if (state == -1)
                break;

            path[++dep] = currentNode;

            board[tree[currentNode].px][tree[currentNode].py] = currentPlayer;

            currentPlayer = -currentPlayer;

            int id = colorId(currentPlayer);

            moveCnt[id] = 0;

            for (int i = 0; i < 9; i++) {

                for (int j = 0; j < 9; j++) {

                    if (legalMove(i, j, currentPlayer))
                        moveList[id][moveCnt[id]++] = i * 9 + j;
                }
            }
        }

        double value = evaluate(-currentPlayer);

        for (int i = dep; i >= 1; i--) {

            int nodeId = path[i];

            tree[nodeId].visitCnt++;

            tree[nodeId].winValue += value;

            value = 1.0 - value;

            board[tree[nodeId].px][tree[nodeId].py] = 0;
        }

        tree[0].visitCnt++;

        currentPlayer = rootPlayer;
    }

    double bestRate = -1e18;
    int bestNode = -1;

    for (int i = 1; i <= tree[0].childCnt; i++) {

        int son = tree[0].child[i];

        double rate = tree[son].winValue / tree[son].visitCnt;

        if (rate > bestRate) {

            bestRate = rate;

            bestNode = son;
        }
    }

    if (bestNode != -1) {

        ansX = tree[bestNode].px;
        ansY = tree[bestNode].py;
    }
}

// ================= 决策 =================
void solve() {

    rootPlayer = currentPlayer;

    int legalCnt = 0;

    for (int i = 0; i < 9; i++) {

        for (int j = 0; j < 9; j++) {

            if (legalMove(i, j, currentPlayer))
                legalCnt++;
        }
    }

    if (legalCnt > 49)
        limitDepth = 6;
    else if (legalCnt > 36)
        limitDepth = 8;
    else if (legalCnt > 20)
        limitDepth = 10;
    else
        limitDepth = 99;

    if (legalCnt <= 16) {

        if (dfsSolve(1))
            return;
    }

    runMCTS();

    if (!legalMove(ansX, ansY, rootPlayer)) {

        for (int i = 0; i < 9; i++) {

            for (int j = 0; j < 9; j++) {

                if (legalMove(i, j, rootPlayer)) {

                    ansX = i;
                    ansY = j;

                    return;
                }
            }
        }
    }
}

// ================= 解析 JSON =================
void readData(
    string& s,
    vector<pair<int,int>>& req,
    vector<pair<int,int>>& resp
) {

    size_t p1 = s.find("\"requests\"");

    if (p1 != string::npos) {

        size_t l = s.find('[', p1);
        size_t r = s.find(']', l);

        string t = s.substr(l + 1, r - l - 1);

        size_t pos = 0;

        while (true) {

            size_t xp = t.find("\"x\"", pos);

            if (xp == string::npos)
                break;

            size_t c1 = t.find(':', xp);
            size_t c2 = t.find(',', c1);

            int vx = stoi(t.substr(c1 + 1, c2 - c1 - 1));

            size_t yp = t.find("\"y\"", c2);
            size_t c3 = t.find(':', yp);
            size_t c4 = t.find('}', c3);

            int vy = stoi(t.substr(c3 + 1, c4 - c3 - 1));

            req.push_back({vx, vy});

            pos = c4 + 1;
        }
    }

    size_t p2 = s.find("\"responses\"");

    if (p2 != string::npos) {

        size_t l = s.find('[', p2);
        size_t r = s.find(']', l);

        string t = s.substr(l + 1, r - l - 1);

        size_t pos = 0;

        while (true) {

            size_t xp = t.find("\"x\"", pos);

            if (xp == string::npos)
                break;

            size_t c1 = t.find(':', xp);
            size_t c2 = t.find(',', c1);

            int vx = stoi(t.substr(c1 + 1, c2 - c1 - 1));

            size_t yp = t.find("\"y\"", c2);
            size_t c3 = t.find(':', yp);
            size_t c4 = t.find('}', c3);

            int vy = stoi(t.substr(c3 + 1, c4 - c3 - 1));

            resp.push_back({vx, vy});

            pos = c4 + 1;
        }
    }
}

// ================= main =================
int main() {

    initRand((unsigned)time(nullptr));

    beginTime = chrono::steady_clock::now();

    string input;

    getline(cin, input);

    vector<pair<int,int>> req;
    vector<pair<int,int>> resp;

    readData(input, req, resp);

    int turn = resp.size();

    currentPlayer = BLACK;

    for (int i = 0; i < turn; i++) {

        int x1 = req[i].first;
        int y1 = req[i].second;

        if (x1 != -1) {

            board[x1][y1] = currentPlayer;

            currentPlayer = -currentPlayer;
        }

        int x2 = resp[i].first;
        int y2 = resp[i].second;

        if (x2 != -1) {

            board[x2][y2] = currentPlayer;

            currentPlayer = -currentPlayer;
        }
    }

    int fx = req[turn].first;
    int fy = req[turn].second;

    if (fx != -1) {

        board[fx][fy] = currentPlayer;

        currentPlayer = -currentPlayer;
    }

    solve();

    cout
        << "{\"response\":{\"x\":"
        << ansX
        << ",\"y\":"
        << ansY
        << "}}"
        << endl;

    return 0;
}