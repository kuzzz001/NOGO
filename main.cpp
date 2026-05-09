// 不围棋（NoGo）样例程序
// 随机策略
// 游戏信息：http://www.botzone.org/games#NoGo


//简单交互


#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <ctime>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <map>
using namespace std;

int board[9][9];
bool dfs_air_visit[9][9];
const int cx[] = { -1,0,1,0 };
const int cy[] = { 0,-1,0,1 };

bool inBorder(int x, int y) { return x >= 0 && y >= 0 && x<9 && y<9; }

//true: has air
bool dfs_air(int fx, int fy)
{
	dfs_air_visit[fx][fy] = true;
	bool flag = false;
	for (int dir = 0; dir < 4; dir++)
	{
		int dx = fx + cx[dir], dy = fy + cy[dir];
		if (inBorder(dx, dy))
		{
			if (board[dx][dy] == 0)
				flag = true;
			if (board[dx][dy] == board[fx][fy] && !dfs_air_visit[dx][dy])
				if (dfs_air(dx, dy))
					flag = true;
		}
	}
	return flag;
}

//true: available
bool judgeAvailable(int fx, int fy, int col)
{
	if (board[fx][fy]) return false;
	board[fx][fy] = col;
	memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
	if (!dfs_air(fx, fy))
	{
		board[fx][fy] = 0;
		return false;
	}
	for (int dir = 0; dir < 4; dir++)
	{
		int dx = fx + cx[dir], dy = fy + cy[dir];
		if (inBorder(dx, dy))
		{
			if (board[dx][dy] && !dfs_air_visit[dx][dy])
				if (!dfs_air(dx, dy))
				{
					board[fx][fy] = 0;
					return false;
				}
		}
	}
	board[fx][fy] = 0;
	return true;
}

const int positionWeight[9][9] = {
    {0, 0, 1, 2, 3, 2, 1, 0, 0},
    {0, 1, 3, 4, 5, 4, 3, 1, 0},
    {1, 3, 5, 6, 7, 6, 5, 3, 1},
    {2, 4, 6, 8, 9, 8, 6, 4, 2},
    {3, 5, 7, 9, 10, 9, 7, 5, 3},
    {2, 4, 6, 8, 9, 8, 6, 4, 2},
    {1, 3, 5, 6, 7, 6, 5, 3, 1},
    {0, 1, 3, 4, 5, 4, 3, 1, 0},
    {0, 0, 1, 2, 3, 2, 1, 0, 0}
};

int countLiberties(int fx, int fy, int b[9][9]) {
    bool visited[9][9] = {false};
    bool libertiesVisited[9][9] = {false};
    int liberties = 0;
    vector<pair<int, int>> stack;
    stack.push_back({fx, fy});
    visited[fx][fy] = true;
    int color = b[fx][fy];
    
    while (!stack.empty()) {
        int x = stack.back().first;
        int y = stack.back().second;
        stack.pop_back();
        
        for (int dir = 0; dir < 4; dir++) {
            int dx = x + cx[dir], dy = y + cy[dir];
            if (inBorder(dx, dy)) {
                if (b[dx][dy] == 0 && !libertiesVisited[dx][dy]) {
                    liberties++;
                    libertiesVisited[dx][dy] = true;
                } else if (b[dx][dy] == color && !visited[dx][dy]) {
                    visited[dx][dy] = true;
                    stack.push_back({dx, dy});
                }
            }
        }
    }
    return liberties;
}

int evaluateBoard(int b[9][9], int player) {
    int score = 0;
    bool visited[9][9] = {false};
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] != 0 && !visited[i][j]) {
                int liberties = countLiberties(i, j, b);
                int color = b[i][j];
                int groupScore = liberties * 10;
                
                bool tempVisited[9][9] = {false};
                vector<pair<int, int>> stack;
                stack.push_back({i, j});
                tempVisited[i][j] = true;
                
                while (!stack.empty()) {
                    int x = stack.back().first;
                    int y = stack.back().second;
                    stack.pop_back();
                    visited[x][y] = true;
                    groupScore += positionWeight[x][y];
                    
                    for (int dir = 0; dir < 4; dir++) {
                        int dx = x + cx[dir], dy = y + cy[dir];
                        if (inBorder(dx, dy) && b[dx][dy] == color && !tempVisited[dx][dy]) {
                            tempVisited[dx][dy] = true;
                            stack.push_back({dx, dy});
                        }
                    }
                }
                score += groupScore * (color == player ? 1 : -1);
            }
        }
    }
    return score;
}

vector<pair<int, int>> getValidMoves(int b[9][9], int color) {
    vector<pair<int, int>> moves;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] != 0) continue;
            bool valid = true;
            
            b[i][j] = color;
            bool dfsVisit[9][9] = {false};
            bool hasAir = false;
            vector<pair<int, int>> stack;
            stack.push_back({i, j});
            dfsVisit[i][j] = true;
            
            while (!stack.empty() && !hasAir) {
                int x = stack.back().first;
                int y = stack.back().second;
                stack.pop_back();
                for (int dir = 0; dir < 4; dir++) {
                    int dx = x + cx[dir], dy = y + cy[dir];
                    if (inBorder(dx, dy)) {
                        if (b[dx][dy] == 0) {
                            hasAir = true;
                            break;
                        } else if (b[dx][dy] == color && !dfsVisit[dx][dy]) {
                            dfsVisit[dx][dy] = true;
                            stack.push_back({dx, dy});
                        }
                    }
                }
            }
            
            if (!hasAir) {
                b[i][j] = 0;
                continue;
            }
            
            for (int dir = 0; dir < 4; dir++) {
                int dx = i + cx[dir], dy = j + cy[dir];
                if (inBorder(dx, dy) && b[dx][dy] != 0 && b[dx][dy] != color) {
                    bool oppVisit[9][9] = {false};
                    bool oppHasAir = false;
                    vector<pair<int, int>> oppStack;
                    oppStack.push_back({dx, dy});
                    oppVisit[dx][dy] = true;
                    
                    while (!oppStack.empty() && !oppHasAir) {
                        int x = oppStack.back().first;
                        int y = oppStack.back().second;
                        oppStack.pop_back();
                        for (int d = 0; d < 4; d++) {
                            int ndx = x + cx[d], ndy = y + cy[d];
                            if (inBorder(ndx, ndy)) {
                                if (b[ndx][ndy] == 0) {
                                    oppHasAir = true;
                                    break;
                                } else if (b[ndx][ndy] == b[dx][dy] && !oppVisit[ndx][ndy]) {
                                    oppVisit[ndx][ndy] = true;
                                    oppStack.push_back({ndx, ndy});
                                }
                            }
                        }
                    }
                    
                    if (!oppHasAir) {
                        valid = false;
                        break;
                    }
                }
            }
            
            b[i][j] = 0;
            if (valid) {
                moves.push_back({i, j});
            }
        }
    }
    return moves;
}

const int MAX_DEPTH = 3;

int minimax(int b[9][9], int depth, int alpha, int beta, int player, int originalPlayer) {
    if (depth == 0) {
        return evaluateBoard(b, originalPlayer);
    }
    
    vector<pair<int, int>> moves = getValidMoves(b, player);
    if (moves.empty()) {
        return player == originalPlayer ? -10000 : 10000;
    }
    
    if (player == originalPlayer) {
        int maxEval = -10000;
        for (size_t i = 0; i < moves.size(); i++) {
            int x = moves[i].first;
            int y = moves[i].second;
            b[x][y] = player;
            int eval = minimax(b, depth - 1, alpha, beta, -player, originalPlayer);
            b[x][y] = 0;
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        int minEval = 10000;
        for (size_t i = 0; i < moves.size(); i++) {
            int x = moves[i].first;
            int y = moves[i].second;
            b[x][y] = player;
            int eval = minimax(b, depth - 1, alpha, beta, -player, originalPlayer);
            b[x][y] = 0;
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

struct BoardHash {
    size_t operator()(const pair<int[9][9], int>& key) const {
        size_t hash = 0;
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                hash ^= ((size_t)key.first[i][j] << ((i * 9 + j) * 2));
            }
        }
        hash ^= (size_t)key.second << 162;
        return hash;
    }
};

struct BoardEqual {
    bool operator()(const pair<int[9][9], int>& lhs, const pair<int[9][9], int>& rhs) const {
        if (lhs.second != rhs.second) return false;
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                if (lhs.first[i][j] != rhs.first[i][j]) return false;
            }
        }
        return true;
    }
};

struct TranspositionEntry {
    int score;
    int depth;
    int flag;
};

unordered_map<pair<int[9][9], int>, TranspositionEntry, BoardHash, BoardEqual> transpositionTable;

int probeTranspositionTable(int b[9][9], int player, int depth, int alpha, int beta) {
    pair<int[9][9], int> key;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            key.first[i][j] = b[i][j];
        }
    }
    key.second = player;
    
    auto it = transpositionTable.find(key);
    if (it != transpositionTable.end() && it->second.depth >= depth) {
        if (it->second.flag == 0) {
            return it->second.score;
        } else if (it->second.flag == 1 && it->second.score >= beta) {
            return it->second.score;
        } else if (it->second.flag == -1 && it->second.score <= alpha) {
            return it->second.score;
        }
    }
    return -10001;
}

void storeTranspositionTable(int b[9][9], int player, int depth, int score, int alpha, int beta) {
    pair<int[9][9], int> key;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            key.first[i][j] = b[i][j];
        }
    }
    key.second = player;
    
    TranspositionEntry entry;
    entry.score = score;
    entry.depth = depth;
    if (score <= alpha) {
        entry.flag = -1;
    } else if (score >= beta) {
        entry.flag = 1;
    } else {
        entry.flag = 0;
    }
    
    transpositionTable[key] = entry;
}

int historyTable[9][9] = {0};

void updateHistoryTable(int x, int y, int depth) {
    historyTable[x][y] += depth * depth;
}

void sortMovesByHistory(vector<pair<int, int>>& moves) {
    sort(moves.begin(), moves.end(), [](const pair<int, int>& a, const pair<int, int>& b) {
        return historyTable[a.first][a.second] > historyTable[b.first][b.second];
    });
}

int getBoardHash(int b[9][9]) {
    int hash = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            hash = hash * 3 + b[i][j] + 1;
        }
    }
    return hash;
}

map<int, pair<int, int>> openingBook;

void initOpeningBook() {
    openingBook[0] = {4, 4};
    openingBook[1] = {3, 4};
    openingBook[2] = {4, 3};
    openingBook[3] = {5, 4};
    openingBook[4] = {4, 5};
}

pair<int, int> lookupOpeningBook(int b[9][9], int moveCount) {
    if (moveCount < 5) {
        int hash = getBoardHash(b);
        auto it = openingBook.find(hash);
        if (it != openingBook.end()) {
            return it->second;
        }
    }
    return {-1, -1};
}

int calculateSimulations(double timeLeft, int moveCount) {
    if (timeLeft > 0.5) {
        return 500;
    } else if (timeLeft > 0.3) {
        return 300;
    } else if (timeLeft > 0.1) {
        return 150;
    } else {
        return 80;
    }
}

int evaluateConnectivity(int b[9][9], int player) {
    bool visited[9][9] = {false};
    int connectivity = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] == player && !visited[i][j]) {
                int groupSize = 0;
                vector<pair<int, int>> stack;
                stack.push_back({i, j});
                visited[i][j] = true;
                
                while (!stack.empty()) {
                    int x = stack.back().first;
                    int y = stack.back().second;
                    stack.pop_back();
                    groupSize++;
                    
                    for (int dir = 0; dir < 4; dir++) {
                        int dx = x + cx[dir], dy = y + cy[dir];
                        if (inBorder(dx, dy) && b[dx][dy] == player && !visited[dx][dy]) {
                            visited[dx][dy] = true;
                            stack.push_back({dx, dy});
                        }
                    }
                }
                
                connectivity += groupSize * groupSize;
            }
        }
    }
    return connectivity;
}

int evaluateThreats(int b[9][9], int player) {
    int threats = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] == 0) {
                int tempBoard[9][9];
                for (int x = 0; x < 9; x++) {
                    for (int y = 0; y < 9; y++) {
                        tempBoard[x][y] = b[x][y];
                    }
                }
                
                tempBoard[i][j] = player;
                if (judgeAvailable(i, j, player)) {
                    int liberties = countLiberties(i, j, tempBoard);
                    if (liberties == 1) {
                        threats += 5;
                    } else if (liberties == 2) {
                        threats += 2;
                    }
                }
            }
        }
    }
    return threats;
}

int evaluateFlexibility(int b[9][9], int player) {
    int flexibility = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] == 0 && judgeAvailable(i, j, player)) {
                flexibility += positionWeight[i][j];
            }
        }
    }
    return flexibility;
}

int evaluateBoardAdvanced(int b[9][9], int player) {
    int score = 0;
    
    score += evaluateBoard(b, player);
    score += evaluateConnectivity(b, player) * 5;
    score += evaluateThreats(b, player) * 3;
    score += evaluateFlexibility(b, player) * 2;
    
    return score;
}

struct MCTSNode {
    int x, y;
    int wins;
    int visits;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    bool expanded;
    
    MCTSNode(int px, int py, MCTSNode* p) : x(px), y(py), wins(0), visits(0), parent(p), expanded(false) {}
    
    ~MCTSNode() {
        for (size_t i = 0; i < children.size(); i++) {
            delete children[i];
        }
    }
};

double ucb1(MCTSNode* node, double explorationParam = 1.414) {
    if (node->visits == 0) return 1000000.0;
    return (double)node->wins / node->visits + explorationParam * sqrt(log(node->parent->visits) / node->visits);
}

MCTSNode* select(MCTSNode* node) {
    while (node->expanded && !node->children.empty()) {
        MCTSNode* bestChild = node->children[0];
        double bestUCB = ucb1(bestChild);
        
        for (size_t i = 1; i < node->children.size(); i++) {
            double currentUCB = ucb1(node->children[i]);
            if (currentUCB > bestUCB) {
                bestUCB = currentUCB;
                bestChild = node->children[i];
            }
        }
        node = bestChild;
    }
    return node;
}

void expand(MCTSNode* node, int b[9][9], int player) {
    vector<pair<int, int>> moves = getValidMoves(b, player);
    sortMovesByHistory(moves);
    
    for (size_t i = 0; i < moves.size(); i++) {
        int x = moves[i].first;
        int y = moves[i].second;
        MCTSNode* child = new MCTSNode(x, y, node);
        node->children.push_back(child);
    }
    node->expanded = true;
}

int simulate(int b[9][9], int player, int originalPlayer) {
    int currentPlayer = player;
    int tempBoard[9][9];
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            tempBoard[i][j] = b[i][j];
        }
    }
    
    while (true) {
        vector<pair<int, int>> moves = getValidMoves(tempBoard, currentPlayer);
        
        if (moves.empty()) {
            return currentPlayer == originalPlayer ? 0 : 1;
        }
        
        int randIdx = rand() % moves.size();
        int x = moves[randIdx].first;
        int y = moves[randIdx].second;
        tempBoard[x][y] = currentPlayer;
        currentPlayer = -currentPlayer;
    }
}

void backpropagate(MCTSNode* node, int result) {
    while (node != nullptr) {
        node->visits++;
        node->wins += result;
        node = node->parent;
    }
}

pair<int, int> mctsSearch(int b[9][9], int player, int simulations) {
    MCTSNode* root = new MCTSNode(-1, -1, nullptr);
    expand(root, b, player);
    
    for (int i = 0; i < simulations; i++) {
        MCTSNode* node = select(root);
        
        int tempBoard[9][9];
        for (int j = 0; j < 9; j++) {
            for (int k = 0; k < 9; k++) {
                tempBoard[j][k] = b[j][k];
            }
        }
        
        vector<MCTSNode*> path;
        MCTSNode* current = node;
        while (current != root) {
            path.push_back(current);
            tempBoard[current->x][current->y] = (path.size() % 2 == 1) ? player : -player;
            current = current->parent;
        }
        
        int currentPlayer = (path.size() % 2 == 0) ? -player : player;
        int result = simulate(tempBoard, currentPlayer, player);
        backpropagate(node, result);
    }
    
    MCTSNode* bestChild = nullptr;
    int bestVisits = -1;
    
    for (size_t i = 0; i < root->children.size(); i++) {
        if (root->children[i]->visits > bestVisits) {
            bestVisits = root->children[i]->visits;
            bestChild = root->children[i];
        }
    }
    
    updateHistoryTable(bestChild->x, bestChild->y, 3);
    
    pair<int, int> result = make_pair(bestChild->x, bestChild->y);
    delete root;
    return result;
}

int main()
{
	srand((unsigned)time(0));

	int x, y, n;
	cin >> n;
	for (int i = 0; i < n - 1; i++)
	{
		cin >> x >> y; if (x != -1) board[x][y] = 1;	//对方
		cin >> x >> y; if (x != -1) board[x][y] = -1; //我方
	}
	cin >> x >> y;  if (x != -1) board[x][y] = 1;	//对方

	//此时board[][]里存储的就是当前棋盘的所有棋子信息,x和y存的是对方最近一步下的棋


	/************************************************************************************/
	/***********在下面填充你的代码，决策结果（本方将落子的位置）存入new_x和new_y中****************/

	int myColor = x == -1 ? 1 : -1;
	int moveCount = n;
	
	initOpeningBook();
	
	int new_x = 0, new_y = 0;
	
	pair<int, int> openingMove = lookupOpeningBook(board, moveCount);
	if (openingMove.first != -1) {
	    new_x = openingMove.first;
	    new_y = openingMove.second;
	} else {
	    auto startTime = chrono::high_resolution_clock::now();
	    
	    double timeLeft = 1.0;
	    int simulations = calculateSimulations(timeLeft, moveCount);
	    
	    pair<int, int> result = mctsSearch(board, myColor, simulations);
	    
	    auto endTime = chrono::high_resolution_clock::now();
	    chrono::duration<double> elapsed = endTime - startTime;
	    
	    new_x = result.first;
	    new_y = result.second;
	}

	/***********在上方填充你的代码，决策结果（本方将落子的位置）存入new_x和new_y中****************/
	/************************************************************************************/



	cout << new_x << ' ' << new_y << endl;
	return 0;
}
