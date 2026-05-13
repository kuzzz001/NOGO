#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <vector>
using namespace std;

// ????(???????)
int x, y;
int depth;
int chessboard[9][9] = { 0 };
int black = 1, white = -1;
int player = 1, player0;
int dx[4] = { 1,0,-1,0 }, dy[4] = { 0,1,0,-1 };
int start, timeout = (int)(0.95 * (double)CLOCKS_PER_SEC);

// MCTS ????(?????:???,???????)
#define NODE_MAX 1500000
struct MCTSNode {
    int x, y;
    bool Expanded;
    int NowChildren;
    int Children[81];
    double Value;
    double Times;
} Node[NODE_MAX] = {};
int now = 0;
int now_expanded = 0;

// ?????(???????)
int len[2] = { 0 }, avail[2][81] = { 0 };
int len_store[2] = { 0 }, avail_store[2][81] = { 0 };

inline int trans(int color) { return (color + 1) / 2; }
inline int retrans(int number) { return number * 2 - 1; }

// ====================== ????(?????) ======================
bool air_judge(int x, int y, int player) {
    if (x < 0 || x>8 || y < 0 || y>8) return 0;
    if (chessboard[x][y] == -player) return 0;
    if (chessboard[x][y] == 0) return 1;

    chessboard[x][y] = -player;
    bool res = air_judge(x + 1, y, player)
            || air_judge(x - 1, y, player)
            || air_judge(x, y + 1, player)
            || air_judge(x, y - 1, player);
    chessboard[x][y] = player;
    return res;
}

bool put_judge(int x, int y, int player) {
    if (x < 0 || y < 0 || x>8 || y>8 || chessboard[x][y] != 0) return 0;
    chessboard[x][y] = player;

    bool legal = true;
    if (!air_judge(x, y, player)) legal = false;
    else if ((x + 1 <= 8 && chessboard[x + 1][y] == -player && !air_judge(x + 1, y, -player))
          || (x - 1 >= 0 && chessboard[x - 1][y] == -player && !air_judge(x - 1, y, -player))
          || (y + 1 <= 8 && chessboard[x][y + 1] == -player && !air_judge(x, y + 1, -player))
          || (y - 1 >= 0 && chessboard[x][y - 1] == -player && !air_judge(x, y - 1, -player)))
        legal = false;

    chessboard[x][y] = 0;
    return legal;
}

bool end_judge(int player) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (put_judge(i, j, player)) return 0;
    return 1;
}

// ====================== ????(?????,????) ======================
bool eye_judge(int x, int y, int player) {
    if (chessboard[x][y]) return 0;
    return ((x + 1 > 8 || chessboard[x + 1][y] == player)
         && (x - 1 < 0 || chessboard[x - 1][y] == player)
         && (y + 1 > 8 || chessboard[x][y + 1] == player)
         && (y - 1 < 0 || chessboard[x][y - 1] == player));
}

double judge_value(int player) {
    double p = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (chessboard[i][j] != 0) continue;
            if (!put_judge(i, j, player)) {
                p -= 1;
                if (eye_judge(i, j, player)) p -= 0.2;
            } else if (eye_judge(i, j, player)) p += 0.2;
            if (!put_judge(i, j, -player)) p += 1;
        }
    return 1.0 / (1.0 + exp(-p));
}

// ====================== ????(???Minimax) ======================
int enumerate(int k) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, player)) {
                chessboard[i][j] = player;
                player = -player;
                bool win = enumerate(k + 1);
                if (!win) {
                    if (k == 1) { x = i; y = j; }
                    chessboard[i][j] = 0;
                    player = -player;
                    return 1;
                }
                chessboard[i][j] = 0;
                player = -player;
            }
        }
    return 0;
}

// ====================== MCTS ??(???????) ======================
int selection() {
    int selec = 0;
    int number = trans(player);

    if (!Node[now].Expanded) {
        int tmplen = len[number];
        int tmpavail[81];
        memcpy(tmpavail, avail[number], sizeof(tmpavail));

        int* sign;
        for (int i = 1; i <= Node[now].NowChildren; i++) {
            int c = Node[Node[now].Children[i]].x * 9 + Node[Node[now].Children[i]].y;
            sign = find(tmpavail, tmpavail + tmplen, c);
            if (sign != tmpavail + tmplen) *sign = tmpavail[--tmplen];
        }

        while (true) {
            if (tmplen == 0) {
                if (Node[now].NowChildren == 0) return -1;
                Node[now].Expanded = 1;
                selec = -1;
                break;
            }
            int r = rand() % tmplen;
            int xx = tmpavail[r] / 9, yy = tmpavail[r] % 9;
            if (put_judge(xx, yy, player)) {
                selec = tmpavail[r];
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number]) *sign = avail[number][--len[number]];
                break;
            } else {
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number]) *sign = avail[number][--len[number]];
                tmpavail[r] = tmpavail[--tmplen];
            }
        }

        if (selec != -1) {
            Node[now].NowChildren++;
            Node[now].Children[Node[now].NowChildren] = ++now_expanded;
            Node[now_expanded] = { selec / 9, selec % 9 };
            if (tmplen == 1) Node[now].Expanded = 1;
            now = now_expanded;
            return 1;
        }
    }

    double max_ucb = -1;
    int best = 0;
    for (int i = 1; i <= Node[now].NowChildren; i++) {
        int c = Node[now].Children[i];
        double ucb = Node[c].Value / Node[c].Times + sqrt(2) * sqrt(log(Node[now].Times) / Node[c].Times);
        if (ucb > max_ucb) {
            max_ucb = ucb;
            best = c;
        }
    }
    now = best;
    return 0;
}

// ====================== MCTS ???(?????) ======================
void MCTS() {
    memset(Node, 0, sizeof(Node));
    now_expanded = 0;
    Node[0] = { -1, -1 };
    int search_path[100];

    len_store[1] = len_store[0] = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (put_judge(i, j, black)) avail_store[1][len_store[1]++] = i * 9 + j;
            if (put_judge(i, j, white)) avail_store[0][len_store[0]++] = i * 9 + j;
        }

    while (clock() - start < timeout) {
        int i = 0;
        now = 0;
        search_path[0] = 0;
        memcpy(avail, avail_store, sizeof(avail));
        memcpy(len, len_store, sizeof(len));

        while (!end_judge(player) && i <= depth) {
            int unexpand = selection();
            if (unexpand == -1) break;
            search_path[++i] = now;
            chessboard[Node[now].x][Node[now].y] = player;
            player = -player;
        }

        double val = judge_value(-player);
        for (int j = i; j > 0; j--) {
            Node[search_path[j]].Times++;
            Node[search_path[j]].Value += val;
            val *= -1;
            chessboard[Node[search_path[j]].x][Node[search_path[j]].y] = 0;
        }
        Node[0].Times++;
        player = player0;
    }

    double max_val = -1;
    int best = 0;
    for (int i = 1; i <= Node[0].NowChildren; i++) {
        int c = Node[0].Children[i];
        double v = Node[c].Value / Node[c].Times;
        if (v > max_val) {
            max_val = v;
            best = c;
        }
    }
    x = Node[best].x;
    y = Node[best].y;
}

// ====================== ???(?????) ======================
void choose() {
    player0 = player;
    int cnt = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            cnt += put_judge(i, j, player);

    if (cnt > 49) depth = 6;
    else if (cnt > 36) depth = 8;
    else if (cnt > 20) depth = 10;
    else depth = 99;

    if (cnt <= 10 && enumerate(1)) return;
    MCTS();
}

// ====================== ??? ======================
int main() {
    srand((unsigned)time(0));
    start = clock();
    memset(chessboard, 0, sizeof(chessboard));

    string line;
    getline(cin, line);

    vector<pair<int, int>> requests;
    vector<pair<int, int>> responses;

    auto parse_json = [&](string s) {
        size_t reqPos = s.find("\"requests\"");
        if (reqPos != string::npos) {
            size_t l = s.find('[', reqPos);
            size_t r = s.find(']', l);
            if (l != string::npos && r != string::npos) {
                string arr = s.substr(l + 1, r - l - 1);
                size_t pos = 0;
                while (true) {
                    size_t xPos = arr.find("\"x\"", pos);
                    if (xPos == string::npos) break;
                    size_t colon1 = arr.find(':', xPos);
                    size_t comma = arr.find(',', colon1);
                    int vx = stoi(arr.substr(colon1 + 1, comma - colon1 - 1));
                    size_t yPos = arr.find("\"y\"", comma);
                    size_t colon2 = arr.find(':', yPos);
                    size_t endObj = arr.find('}', colon2);
                    int vy = stoi(arr.substr(colon2 + 1, endObj - colon2 - 1));
                    requests.push_back({vx, vy});
                    pos = endObj + 1;
                }
            }
        }
        size_t resPos = s.find("\"responses\"");
        if (resPos != string::npos) {
            size_t l = s.find('[', resPos);
            size_t r = s.find(']', l);
            if (l != string::npos && r != string::npos) {
                string arr = s.substr(l + 1, r - l - 1);
                size_t pos = 0;
                while (true) {
                    size_t xPos = arr.find("\"x\"", pos);
                    if (xPos == string::npos) break;
                    size_t colon1 = arr.find(':', xPos);
                    size_t comma = arr.find(',', colon1);
                    int vx = stoi(arr.substr(colon1 + 1, comma - colon1 - 1));
                    size_t yPos = arr.find("\"y\"", comma);
                    size_t colon2 = arr.find(':', yPos);
                    size_t endObj = arr.find('}', colon2);
                    int vy = stoi(arr.substr(colon2 + 1, endObj - colon2 - 1));
                    responses.push_back({vx, vy});
                    pos = endObj + 1;
                }
            }
        }
    };

    parse_json(line);

    int turn = responses.size();
    player = 1;

    for (int i = 0; i < turn; i++) {
        int ax = requests[i].first;
        int ay = requests[i].second;
        if (ax != -1) { chessboard[ax][ay] = player; player = -player; }

        int rx = responses[i].first;
        int ry = responses[i].second;
        if (rx != -1) { chessboard[rx][ry] = player; player = -player; }
    }

    int fx = requests[turn].first;
    int fy = requests[turn].second;
    if (fx != -1) { chessboard[fx][fy] = player; player = -player; }

    choose();

    cout << "{\"response\":{\"x\":" << x << ",\"y\":" << y << "}}" << endl;

    return 0;
}