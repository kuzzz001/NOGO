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
#include <sstream>
using namespace std;

int board[9][9];
bool dfs_air_visit[9][9];
const int cx[] = { -1,0,1,0 };
const int cy[] = { 0,-1,0,1 };

bool inBorder(int x, int y) { return x >= 0 && y >= 0 && x<9 && y<9; }

//true: has air
bool dfs_air(int fx, int fy, int b[9][9])
{
	dfs_air_visit[fx][fy] = true;
	bool flag = false;
	for (int dir = 0; dir < 4; dir++)
	{
		int dx = fx + cx[dir], dy = fy + cy[dir];
		if (inBorder(dx, dy))
		{
			if (b[dx][dy] == 0)
				flag = true;
			if (b[dx][dy] == b[fx][fy] && !dfs_air_visit[dx][dy])
				if (dfs_air(dx, dy, b))
					flag = true;
		}
	}
	return flag;
}

//true: available
bool judgeAvailable(int fx, int fy, int col, int b[9][9])
{
	if (b[fx][fy]) return false;
	
	int tempBoard[9][9];
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			tempBoard[i][j] = b[i][j];
		}
	}
	
	tempBoard[fx][fy] = col;
	memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
	if (!dfs_air(fx, fy, tempBoard))
	{
		return false;
	}
	for (int dir = 0; dir < 4; dir++)
	{
		int dx = fx + cx[dir], dy = fy + cy[dir];
		if (inBorder(dx, dy))
		{
			if (tempBoard[dx][dy] && tempBoard[dx][dy] != col)
			{
				memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
				if (!dfs_air(dx, dy, tempBoard))
				{
					return false;
				}
			}
		}
	}
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
            
            int tempBoard[9][9];
            for (int x = 0; x < 9; x++) {
                for (int y = 0; y < 9; y++) {
                    tempBoard[x][y] = b[x][y];
                }
            }
            
            if (judgeAvailable(i, j, color, tempBoard)) {
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
        hash ^= (size_t)key.second;
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
                if (judgeAvailable(i, j, player, tempBoard)) {
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
            if (b[i][j] == 0 && judgeAvailable(i, j, player, b)) {
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
    int tempBoard[9][9];
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            tempBoard[i][j] = b[i][j];
        }
    }
    
    vector<pair<int, int>> moves = getValidMoves(tempBoard, player);
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

pair<int, int> mctsSearch(int b[9][9], int player, int timeLimit) {
    MCTSNode* root = new MCTSNode(-1, -1, nullptr);
    expand(root, b, player);
    
    if (root->children.empty()) {
        delete root;
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                if (b[i][j] == 0) return {i, j};
            }
        }
        return {4, 4};
    }
    
    clock_t startTime = clock();
    int simulations = 0;
    
    while (true) {
        clock_t currentTime = clock();
        if ((int)(currentTime - startTime) >= timeLimit) break;
        
        MCTSNode* node = select(root);
        
        if (!node->expanded) {
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
                current = current->parent;
            }
            reverse(path.begin(), path.end());
            for (size_t i = 0; i < path.size(); i++) {
                int level = (int)i + 1;
                tempBoard[path[i]->x][path[i]->y] = (level % 2 == 1) ? player : -player;
            }
            
            int nextPlayer = ((int)path.size() % 2 == 0) ? player : -player;
            expand(node, tempBoard, nextPlayer);
        }
        
        MCTSNode* simNode = node;
        if (!node->children.empty()) {
            simNode = node->children[0];
        }
        
        int tempBoard[9][9];
        for (int j = 0; j < 9; j++) {
            for (int k = 0; k < 9; k++) {
                tempBoard[j][k] = b[j][k];
            }
        }
        
        vector<MCTSNode*> path;
        MCTSNode* current = simNode;
        while (current != root) {
            path.push_back(current);
            current = current->parent;
        }
        reverse(path.begin(), path.end());
        for (size_t i = 0; i < path.size(); i++) {
            int level = (int)i + 1;
            tempBoard[path[i]->x][path[i]->y] = (level % 2 == 1) ? player : -player;
        }
        
        int currentPlayer = ((int)path.size() % 2 == 0) ? -player : player;
        int result = simulate(tempBoard, currentPlayer, player);
        backpropagate(simNode, result);
        simulations++;
    }
    
    MCTSNode* bestChild = nullptr;
    int bestVisits = -1;
    
    for (size_t i = 0; i < root->children.size(); i++) {
        if (root->children[i]->visits > bestVisits) {
            bestVisits = root->children[i]->visits;
            bestChild = root->children[i];
        }
    }
    
    if (bestChild != nullptr) {
        updateHistoryTable(bestChild->x, bestChild->y, 3);
    }
    
    pair<int, int> result = bestChild != nullptr ? make_pair(bestChild->x, bestChild->y) : make_pair(4, 4);
    delete root;
    return result;
}

int main()
{
	memset(board, 0, sizeof(board));
	srand((unsigned)time(0));

	string line;
	getline(cin, line);
	
	int n = 0;
	bool isJsonMode = false;
	
	if (line.find("{") != string::npos && line.find("requests") != string::npos) {
		isJsonMode = true;
		
		size_t reqPos = line.find("requests");
		if (reqPos != string::npos) {
			size_t arrPos = line.find('[', reqPos);
			if (arrPos != string::npos) {
				size_t pos = arrPos + 1;
				while (pos < line.length()) {
					size_t objPos = line.find('{', pos);
					if (objPos == string::npos) break;
					
					size_t objEnd = line.find('}', objPos);
					if (objEnd == string::npos) break;
					
					string obj = line.substr(objPos, objEnd - objPos + 1);
					
					int reqX = -1, reqY = -1;
					bool hasX = false, hasY = false;
					size_t xPos = obj.find("\"x\"");
					size_t yPos = obj.find("\"y\"");
					
					if (xPos != string::npos) {
						size_t colonPos = obj.find(':', xPos);
						if (colonPos != string::npos) {
							try {
								reqX = stoi(obj.substr(colonPos + 1));
								hasX = true;
							} catch (...) {
								reqX = -1;
								hasX = false;
							}
						}
					}
					
					if (yPos != string::npos) {
						size_t colonPos = obj.find(':', yPos);
						if (colonPos != string::npos) {
							try {
								reqY = stoi(obj.substr(colonPos + 1));
								hasY = true;
							} catch (...) {
								reqY = -1;
								hasY = false;
							}
						}
					}
					
					if (hasX && hasY && reqX >= 0 && reqX < 9 && reqY >= 0 && reqY < 9) {
						if (n % 2 == 0) board[reqX][reqY] = 1;
						else board[reqX][reqY] = -1;
						n++;
					} else if (hasX && hasY && reqX == -1 && reqY == -1) {
						n++;
					}
					
					pos = objEnd + 1;
				}
			}
		}
	} else {
		istringstream iss(line);
		iss >> n;
		for (int i = 0; i < n; i++) {
			int x, y;
			iss >> x >> y;
			if (x >= 0 && x < 9 && y >= 0 && y < 9) {
				if (i % 2 == 0) board[x][y] = 1;
				else board[x][y] = -1;
			}
		}
	}
	
	int myColor = n % 2 == 0 ? 1 : -1;
	int moveCount = n;
	
	initOpeningBook();
	
	int new_x = 0, new_y = 0;
	
	pair<int, int> openingMove = lookupOpeningBook(board, moveCount);
	if (openingMove.first != -1) {
	    new_x = openingMove.first;
	    new_y = openingMove.second;
	} else {
	    int timeLimit = (int)(0.95 * CLOCKS_PER_SEC);
	    pair<int, int> result = mctsSearch(board, myColor, timeLimit);
	    new_x = result.first;
	    new_y = result.second;
	}
	
	/***********在上方填充你的代码，决策结果（本方将落子的位置）存入new_x和new_y中****************/
	/************************************************************************************/

	if (isJsonMode) {
		cout << "{\"response\":{\"x\":" << new_x << ",\"y\":" << new_y << "}}" << endl;
	} else {
		cout << new_x << ' ' << new_y << endl;
	}
	return 0;
}
