#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
using namespace std;
int x, y;
int depth;
int chessboard[9][9] = {0};
int black = 1;
int white = -1;
int player = 1;
int player0;
int dx[4] = {1,0,-1,0};
int dy[4] = {0,1,0,-1};
int visited[9][9] = {0};
int vis_tag = 0;
chrono::steady_clock::time_point start_time;
const int timeout_ms = 950;
inline int elapsed_ms() {
    return (int)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time).count();
}
inline int trans(int color) {
    return (color + 1) / 2;
}
#define NODE_MAX 1500000
struct MCTSNode {
    int x, y;
    bool Expanded;
    int NowChildren;
    int Children[81];
    double Value;
    double Times;
    void init(int xx,int yy) {
        x = xx;
        y = yy;
        Expanded = false;
        NowChildren = 0;
        Value = 0;
        Times = 0;
    }
} Node[NODE_MAX];
int now = 0;
int now_expanded = 0;
int len[2] = {0};
int avail[2][81] = {0};
int len_store[2] = {0};
int avail_store[2][81] = {0};
bool air_judge(int x,int y,int color) {
    if (x < 0 || x > 8 || y < 0 || y > 8)
        return false;
    if (chessboard[x][y] == -color)
        return false;
    if (chessboard[x][y] == 0)
        return true;
    if (visited[x][y] == vis_tag)
        return false;
    visited[x][y] = vis_tag;
    return air_judge(x+1,y,color) || air_judge(x-1,y,color) || air_judge(x,y+1,color) || air_judge(x,y-1,color);
}
bool put_judge(int x,int y,int color) {
    if (x < 0 || x > 8 || y < 0 || y > 8)
        return false;
    if (chessboard[x][y] != 0)
        return false;
    chessboard[x][y] = color;
    bool legal = true;
    vis_tag++;
    if (!air_judge(x,y,color)) {
        legal = false;
    }
    else {
        for (int d=0; d<4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx<0 || nx>8 || ny<0 || ny>8)
                continue;
            if (chessboard[nx][ny] == -color) {
                vis_tag++;
                if (!air_judge(nx,ny,-color)) {
                    legal = false;
                    break;
                }
            }
        }
    }
    chessboard[x][y] = 0;
    return legal;
}
bool end_judge(int color) {
    for (int i=0;i<9;i++) {
        for (int j=0;j<9;j++) {
            if (put_judge(i,j,color))
                return false;
        }
    }
    return true;
}
bool eye_judge(int x,int y,int color) {
    if (chessboard[x][y])
        return false;
    return ((x+1 > 8 || chessboard[x+1][y] == color) && (x-1 < 0 || chessboard[x-1][y] == color) && (y+1 > 8 || chessboard[x][y+1] == color) && (y-1 < 0 || chessboard[x][y-1] == color));
}
double judge_value(int color) {
    double p = 0;
    for (int i=0;i<9;i++) {
        for (int j=0;j<9;j++) {
            if (chessboard[i][j] != 0)
                continue;
            if (!put_judge(i,j,color)) {
                p -= 1;
                if (eye_judge(i,j,color))
                    p -= 0.2;
            }
            else if (eye_judge(i,j,color)) {
                p += 0.2;
            }
            if (!put_judge(i,j,-color))
                p += 1;
        }
    }
    return 1.0 / (1.0 + exp(-p));
}
int enumerate(int k) {
    if (elapsed_ms() > timeout_ms * 8 / 10)
        return 0;
    for (int i=0;i<9;i++) {
        for (int j=0;j<9;j++) {
            if (put_judge(i,j,player)) {
                chessboard[i][j] = player;
                player = -player;
                bool win = enumerate(k+1);
                if (!win) {
                    if (k == 1) {
                        x = i;
                        y = j;
                    }
                    chessboard[i][j] = 0;
                    player = -player;
                    return 1;
                }
                chessboard[i][j] = 0;
                player = -player;
            }
        }
    }
    return 0;
}
int selection() {
    int selec = 0;
    int number = trans(player);
    if (!Node[now].Expanded) {
        int tmplen = len[number];
        int tmpavail[81];
        memcpy(tmpavail, avail[number], sizeof(tmpavail));
        int* sign;
        for (int i=1;i<=Node[now].NowChildren;i++) {
            int c = Node[Node[now].Children[i]].x * 9 + Node[Node[now].Children[i]].y;
            sign = find(tmpavail, tmpavail + tmplen, c);
            if (sign != tmpavail + tmplen)
                *sign = tmpavail[--tmplen];
        }
        while (true) {
            if (tmplen == 0) {
                if (Node[now].NowChildren == 0)
                    return -1;
                Node[now].Expanded = true;
                break;
            }
            int r = rand() % tmplen;
            int xx = tmpavail[r] / 9;
            int yy = tmpavail[r] % 9;
            if (put_judge(xx,yy,player)) {
                selec = tmpavail[r];
                sign = find(avail[number], avail[number] + len[number], selec);
                if (sign != avail[number] + len[number])
                    *sign = avail[number][--len[number]];
                break;
            }
            else {
                sign = find(avail[number], avail[number] + len[number], tmpavail[r]);
                if (sign != avail[number] + len[number])
                    *sign = avail[number][--len[number]];
                tmpavail[r] = tmpavail[--tmplen];
            }
        }
        if (selec != 0) {
            if (now_expanded + 1 >= NODE_MAX)
                return -1;
            Node[now].NowChildren++;
            Node[now].Children[Node[now].NowChildren] = ++now_expanded;
            Node[now_expanded].init(selec/9, selec%9);
            now = now_expanded;
            return 1;
        }
    }
    double best_ucb = -1e18;
    int best = -1;
    double logt = log(max(1.0, Node[now].Times));
    for (int i=1;i<=Node[now].NowChildren;i++) {
        int c = Node[now].Children[i];
        double ucb = Node[c].Value / Node[c].Times + 1.414 * sqrt(logt / Node[c].Times);
        if (ucb > best_ucb) {
            best_ucb = ucb;
            best = c;
        }
    }
    if (best == -1)
        return -1;
    now = best;
    return 0;
}
void MCTS() {
    now_expanded = 0;
    Node[0].init(-1,-1);
    int search_path[100];
    len_store[0] = len_store[1] = 0;
    for (int i=0;i<9;i++) {
        for (int j=0;j<9;j++) {
            if (put_judge(i,j,black))
                avail_store[1][len_store[1]++] = i*9+j;
            if (put_judge(i,j,white))
                avail_store[0][len_store[0]++] = i*9+j;
        }
    }
    while (elapsed_ms() < timeout_ms) {
        int dep = 0;
        now = 0;
        search_path[0] = 0;
        memcpy(avail, avail_store, sizeof(avail));
        memcpy(len, len_store, sizeof(len));
        while (!end_judge(player) && dep <= depth) {
            int res = selection();
            if (res == -1)
                break;
            search_path[++dep] = now;
            chessboard[Node[now].x][Node[now].y] = player;
            player = -player;
        }
        double val = judge_value(-player);
        for (int i=dep;i>0;i--) {
            Node[search_path[i]].Times++;
            Node[search_path[i]].Value += val;
            val *= -1;
            chessboard[Node[search_path[i]].x][Node[search_path[i]].y] = 0;
        }
        Node[0].Times++;
        player = player0;
    }
    double bestv = -1;
    int best = 0;
    for (int i=1;i<=Node[0].NowChildren;i++) {
        int c = Node[0].Children[i];
        double v = Node[c].Value / Node[c].Times;
        if (v > bestv) {
            bestv = v;
            best = c;
        }
    }
    x = Node[best].x;
    y = Node[best].y;
}
void choose() {
    player0 = player;
    int cnt = 0;
    for (int i=0;i<9;i++) {
        for (int j=0;j<9;j++) {
            cnt += put_judge(i,j,player);
        }
    }
    if (cnt > 49)
        depth = 6;
    else if (cnt > 36)
        depth = 8;
    else if (cnt > 20)
        depth = 10;
    else
        depth = 99;
    if (cnt <= 10) {
        if (enumerate(1))
            return;
    }
    MCTS();
    if (!put_judge(x,y,player0)) {
        for (int i=0;i<9;i++) {
            for (int j=0;j<9;j++) {
                if (put_judge(i,j,player0)) {
                    x = i;
                    y = j;
                    return;
                }
            }
        }
    }
}
void parse_input(string &line, vector<pair<int,int>>& requests, vector<pair<int,int>>& responses) {
    size_t reqPos = line.find("\"requests\"");
    if (reqPos != string::npos) {
        size_t l = line.find('[', reqPos);
        size_t r = line.find(']', l);
        string arr = line.substr(l+1, r-l-1);
        size_t pos = 0;
        while (true) {
            size_t xp = arr.find("\"x\"", pos);
            if (xp == string::npos)
                break;
            size_t c1 = arr.find(':', xp);
            size_t cm = arr.find(',', c1);
            int vx = stoi(arr.substr(c1+1, cm-c1-1));
            size_t yp = arr.find("\"y\"", cm);
            size_t c2 = arr.find(':', yp);
            size_t ed = arr.find('}', c2);
            int vy = stoi(arr.substr(c2+1, ed-c2-1));
            requests.push_back({vx, vy});
            pos = ed + 1;
        }
    }
    size_t resPos = line.find("\"responses\"");
    if (resPos != string::npos) {
        size_t l = line.find('[', resPos);
        size_t r = line.find(']', l);
        string arr = line.substr(l+1, r-l-1);
        size_t pos = 0;
        while (true) {
            size_t xp = arr.find("\"x\"", pos);
            if (xp == string::npos)
                break;
            size_t c1 = arr.find(':', xp);
            size_t cm = arr.find(',', c1);
            int vx = stoi(arr.substr(c1+1, cm-c1-1));
            size_t yp = arr.find("\"y\"", cm);
            size_t c2 = arr.find(':', yp);
            size_t ed = arr.find('}', c2);
            int vy = stoi(arr.substr(c2+1, ed-c2-1));
            responses.push_back({vx, vy});
            pos = ed + 1;
        }
    }
}
int main() {
    srand((unsigned)time(0));
    start_time = chrono::steady_clock::now();
    string line;
    getline(cin, line);
    vector<pair<int,int>> requests;
    vector<pair<int,int>> responses;
    parse_input(line, requests, responses);
    int turn = responses.size();
    player = 1;
    for (int i=0;i<turn;i++) {
        int ax = requests[i].first;
        int ay = requests[i].second;
        if (ax != -1) {
            chessboard[ax][ay] = player;
            player = -player;
        }
        int rx = responses[i].first;
        int ry = responses[i].second;
        if (rx != -1) {
            chessboard[rx][ry] = player;
            player = -player;
        }
    }
    int fx = requests[turn].first;
    int fy = requests[turn].second;
    if (fx != -1) {
        chessboard[fx][fy] = player;
        player = -player;
    }
    choose();
    cout << "{\"response\":{\"x\":" << x << ",\"y\":" << y << "}}" << endl;
    return 0;
}