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
#include <sstream>
using namespace std;

int board[9][9];
const int cx[] = { -1,0,1,0 };
const int cy[] = { 0,-1,0,1 };

bool inBorder(int x, int y) { return x >= 0 && y >= 0 && x<9 && y<9; }

int countLiberties(int fx, int fy, int b[9][9], int* outGroupSize = nullptr) {
    int color = b[fx][fy];
    if (color == 0) {
        if (outGroupSize) *outGroupSize = 0;
        return 0;
    }
    
    bool visited[9][9] = {false};
    bool libVisited[9][9] = {false};
    int liberties = 0;
    int groupSize = 0;
    pair<int, int> stack[81];
    int top = 0;
    stack[top++] = {fx, fy};
    visited[fx][fy] = true;
    
    while (top > 0) {
        top--;
        int x = stack[top].first;
        int y = stack[top].second;
        groupSize++;
        
        for (int dir = 0; dir < 4; dir++) {
            int dx = x + cx[dir], dy = y + cy[dir];
            if (!inBorder(dx, dy)) continue;
            if (b[dx][dy] == 0 && !libVisited[dx][dy]) {
                liberties++;
                libVisited[dx][dy] = true;
            } else if (b[dx][dy] == color && !visited[dx][dy]) {
                visited[dx][dy] = true;
                stack[top++] = {dx, dy};
            }
        }
    }
    if (outGroupSize) *outGroupSize = groupSize;
    return liberties;
}

//true: available
bool judgeAvailable(int fx, int fy, int col, int b[9][9])
{
	if (b[fx][fy]) return false;
	
	int tempBoard[9][9];
	memcpy(tempBoard, b, sizeof(tempBoard));
	
	tempBoard[fx][fy] = col;
	
	if (countLiberties(fx, fy, tempBoard) == 0)
		return false;
	
	for (int dir = 0; dir < 4; dir++)
	{
		int dx = fx + cx[dir], dy = fy + cy[dir];
		if (inBorder(dx, dy) && tempBoard[dx][dy] && tempBoard[dx][dy] != col)
		{
			if (countLiberties(dx, dy, tempBoard) == 0)
				return false;
		}
	}
	return true;
}

bool isFinalLegal(int x, int y, int col, int b[9][9])
{
	if (x < 0 || x >= 9 || y < 0 || y >= 9) return false;
	if (b[x][y] != 0) return false;
	return judgeAvailable(x, y, col, b);
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

vector<pair<int, int>> getValidMoves(int b[9][9], int color) {
    vector<pair<int, int>> moves;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (b[i][j] != 0) continue;
            if (judgeAvailable(i, j, color, b)) {
                moves.push_back({i, j});
            }
        }
    }
    return moves;
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

int evalMovePriority(int x, int y, int player, int b[9][9]) {
    int temp[9][9];
    memcpy(temp, b, sizeof(temp));
    temp[x][y] = player;
    
    int score = 0;
    
    int gs;
    int lib = countLiberties(x, y, temp, &gs);
    score += lib * 4;
    score -= gs * gs * 3;
    
    for (int d = 0; d < 4; d++) {
        int nx = x + cx[d], ny = y + cy[d];
        if (!inBorder(nx, ny)) continue;
        if (temp[nx][ny] == -player) {
            int oppLib = countLiberties(nx, ny, temp);
            if (oppLib == 1) score += 80;
            else if (oppLib == 2) score += 30;
            else if (oppLib == 3) score += 10;
        } else if (temp[nx][ny] == 0) {
            int blockedSides = 0;
            for (int d2 = 0; d2 < 4; d2++) {
                int ax = nx + cx[d2], ay = ny + cy[d2];
                if (!inBorder(ax, ay) || temp[ax][ay] != 0) blockedSides++;
            }
            if (blockedSides >= 4) score += 18;
            else if (blockedSides == 3) score += 6;
        }
    }
    
    score += positionWeight[x][y];
    score += historyTable[x][y] / 8;
    
    return score;
}

void sortMovesByPriority(vector<pair<int, int>>& moves, int player, int b[9][9]) {
    sort(moves.begin(), moves.end(), [player, b](const pair<int, int>& a, const pair<int, int>& bb) {
        return evalMovePriority(a.first, a.second, player, b) >
               evalMovePriority(bb.first, bb.second, player, b);
    });
}

pair<int, int> lookupOpeningBook(int moveCount) {
    if (moveCount == 0) return {3, 3};
    if (moveCount == 1) return {5, 5};
    if (moveCount == 2) return {3, 5};
    if (moveCount == 3) return {5, 3};
    if (moveCount == 4) return {2, 2};
    if (moveCount == 5) return {6, 6};
    if (moveCount == 6) return {2, 6};
    if (moveCount == 7) return {6, 2};
    return {-1, -1};
}

struct MCTSNode {
    int x, y;
    int player;
    int wins;
    int visits;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    bool expanded;
    
    MCTSNode(int px, int py, int pPlayer, MCTSNode* p) : x(px), y(py), player(pPlayer), wins(0), visits(0), parent(p), expanded(false) {}
    
    ~MCTSNode() {
        for (size_t i = 0; i < children.size(); i++) {
            delete children[i];
        }
    }
};

double ucb1(MCTSNode* node, double explorationParam = 1.414) {
    if (node->visits == 0) return 1000000.0;
    double parentVisits = (node->parent != nullptr) ? max(1.0, (double)node->parent->visits) : 1.0;
    return (double)node->wins / node->visits + explorationParam * sqrt(log(parentVisits) / node->visits);
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

int simulate(int b[9][9], int player, int originalPlayer) {
    int tempBoard[9][9];
    memcpy(tempBoard, b, sizeof(tempBoard));
    int currentPlayer = player;
    
    int emptyCount = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (tempBoard[i][j] == 0) emptyCount++;
    int maxSteps = min(emptyCount, 45);
    int gameStage = (emptyCount > 45) ? 0 : (emptyCount > 20) ? 1 : 2;
    
    for (int step = 0; step < maxSteps; step++) {
        vector<pair<int, int>> moves = getValidMoves(tempBoard, currentPlayer);
        
        if (moves.empty())
            return currentPlayer == originalPlayer ? -1 : 1;
        
        int idx;
        if (rand() % 10 < 2) {
            idx = rand() % moves.size();
        } else {
            int bestIdx = 0, bestScore = -100000;
            int evalCount = min((int)moves.size(), 20);
            for (int i = 0; i < evalCount; i++) {
                int ci = rand() % moves.size();
                int x = moves[ci].first, y = moves[ci].second;
                tempBoard[x][y] = currentPlayer;
                
                int gs;
                int lib = countLiberties(x, y, tempBoard, &gs);
                int groupPenalty = gs * gs * 3;
                
                int oppPressure = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = x + cx[d], ny = y + cy[d];
                    if (!inBorder(nx, ny)) continue;
                    if (tempBoard[nx][ny] == -currentPlayer) {
                        int oppLib = countLiberties(nx, ny, tempBoard);
                        if (oppLib == 1) oppPressure += 80;
                        else if (oppLib == 2) oppPressure += 30;
                        else if (oppLib == 3) oppPressure += 10;
                    }
                }
                
                int blocked = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = x + cx[d], ny = y + cy[d];
                    if (!inBorder(nx, ny) || tempBoard[nx][ny] != 0) continue;
                    int nb = 0;
                    for (int d2 = 0; d2 < 4; d2++) {
                        int ax = nx + cx[d2], ay = ny + cy[d2];
                        if (!inBorder(ax, ay) || tempBoard[ax][ay] != 0) nb++;
                    }
                    if (nb >= 4) blocked += 5;
                    else if (nb == 3) blocked += 2;
                }
                
                int stagePosWeight;
                if (gameStage == 0) {
                    int distCenter = max(abs(x-4), abs(y-4));
                    stagePosWeight = 10 - distCenter;
                } else if (gameStage == 1) {
                    stagePosWeight = positionWeight[x][y];
                } else {
                    stagePosWeight = positionWeight[x][y];
                    if (x <= 1 || x >= 7 || y <= 1 || y >= 7)
                        stagePosWeight = max(0, stagePosWeight - 3);
                }
                
                tempBoard[x][y] = 0;
                
                int score = lib * 5 + stagePosWeight + blocked * 6
                          - groupPenalty + oppPressure;
                if (score > bestScore) { bestScore = score; bestIdx = ci; }
            }
            idx = bestIdx;
        }
        
        tempBoard[moves[idx].first][moves[idx].second] = currentPlayer;
        currentPlayer = -currentPlayer;
    }
    return 0;
}

void backpropagate(MCTSNode* node, int result, int originalPlayer) {
    while (node != nullptr) {
        node->visits++;
        node->wins += (node->player == originalPlayer) ? result : -result;
        node = node->parent;
    }
}

pair<int, int> mctsSearch(int b[9][9], int player) {
    MCTSNode* root = new MCTSNode(-1, -1, player, nullptr);
    vector<pair<int, int>> rootMoves = getValidMoves(b, player);
    sortMovesByPriority(rootMoves, player, b);
    
    for (size_t i = 0; i < rootMoves.size(); i++) {
        if (!isFinalLegal(rootMoves[i].first, rootMoves[i].second, player, b)) continue;
        MCTSNode* child = new MCTSNode(rootMoves[i].first, rootMoves[i].second, player, root);
        root->children.push_back(child);
    }
    root->expanded = true;
    
    if (root->children.empty()) {
        delete root;
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                if (isFinalLegal(i, j, player, b)) return {i, j};
            }
        }
        return {-1, -1};
    }
    
    clock_t startTime = clock();
    
    while (true) {
        if ((double)(clock() - startTime) / CLOCKS_PER_SEC >= 0.95) break;
        
        MCTSNode* node = select(root);
        
        int simBoard[9][9];
        memcpy(simBoard, b, sizeof(simBoard));
        
        int simPlayer = player;
        if (node != root) {
            vector<MCTSNode*> path;
            MCTSNode* cur = node;
            while (cur != root) {
                path.push_back(cur);
                cur = cur->parent;
            }
            bool pathLegal = true;
            for (int i = (int)path.size() - 1; i >= 0; i--) {
                if (!isFinalLegal(path[i]->x, path[i]->y, path[i]->player, simBoard)) {
                    pathLegal = false;
                    break;
                }
                simBoard[path[i]->x][path[i]->y] = path[i]->player;
            }
            if (!pathLegal) continue;
            simPlayer = -path[0]->player;
        }
        
        if (!node->expanded) {
            vector<pair<int, int>> moves = getValidMoves(simBoard, simPlayer);
            if (moves.empty()) {
                int result = (simPlayer == player) ? -1 : 1;
                backpropagate(node, result, player);
                continue;
            }
            sortMovesByPriority(moves, simPlayer, simBoard);
            for (size_t i = 0; i < moves.size(); i++) {
                if (!isFinalLegal(moves[i].first, moves[i].second, simPlayer, simBoard)) continue;
                MCTSNode* child = new MCTSNode(moves[i].first, moves[i].second, simPlayer, node);
                node->children.push_back(child);
            }
            node->expanded = true;
        }
        
        {
            MCTSNode* simNode = node;
            if (!node->children.empty()) {
                int idx = rand() % node->children.size();
                simNode = node->children[idx];
                if (isFinalLegal(simNode->x, simNode->y, simNode->player, simBoard)) {
                    simBoard[simNode->x][simNode->y] = simNode->player;
                    simPlayer = -simNode->player;
                } else {
                    simNode = node;
                }
            }
            
            int result = simulate(simBoard, simPlayer, player);
            backpropagate(simNode, result, player);
        }
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
        updateHistoryTable(bestChild->x, bestChild->y, bestChild->visits / 10 + 1);
    }
    
    pair<int, int> result = {-1, -1};
    if (bestChild != nullptr && isFinalLegal(bestChild->x, bestChild->y, player, b)) {
        result = {bestChild->x, bestChild->y};
    }
    
    if (result.first == -1) {
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                if (isFinalLegal(i, j, player, b)) {
                    result = {i, j};
                    i = 9; break;
                }
            }
        }
    }
    if (result.first == -1) result = {-1, -1};
    
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
	int myColor = 1;
	
	if (line.find("\"requests\"") != string::npos)
	{
		isJsonMode = true;
		vector<pair<int,int>> requests;
		vector<pair<int,int>> responses;
		
		// 解析 requests
		size_t reqPos = line.find("\"requests\"");
		if(reqPos != string::npos)
		{
			size_t l = line.find('[', reqPos);
			size_t r = line.find(']', l);
			string arr = line.substr(l + 1, r - l - 1);
			size_t pos = 0;
			while(true)
			{
				size_t xPos = arr.find("\"x\"", pos);
				if(xPos == string::npos) break;
				size_t colon1 = arr.find(':', xPos);
				size_t comma = arr.find(',', colon1);
				int x = stoi(arr.substr(colon1 + 1, comma - colon1 - 1));
				size_t yPos = arr.find("\"y\"", comma);
				size_t colon2 = arr.find(':', yPos);
				size_t endObj = arr.find('}', colon2);
				int y = stoi(arr.substr(colon2 + 1, endObj - colon2 - 1));
				requests.push_back({x, y});
				pos = endObj + 1;
			}
		}
		
		// 解析 responses
		size_t resPos = line.find("\"responses\"");
		if(resPos != string::npos)
		{
			size_t l = line.find('[', resPos);
			size_t r = line.find(']', l);
			string arr = line.substr(l + 1, r - l - 1);
			size_t pos = 0;
			while(true)
			{
				size_t xPos = arr.find("\"x\"", pos);
				if(xPos == string::npos) break;
				size_t colon1 = arr.find(':', xPos);
				size_t comma = arr.find(',', colon1);
				int x = stoi(arr.substr(colon1 + 1, comma - colon1 - 1));
				size_t yPos = arr.find("\"y\"", comma);
				size_t colon2 = arr.find(':', yPos);
				size_t endObj = arr.find('}', colon2);
				int y = stoi(arr.substr(colon2 + 1, endObj - colon2 - 1));
				responses.push_back({x, y});
				pos = endObj + 1;
			}
		}
		
		// 判断先后手
		bool firstPlayer = false;
		if(!requests.empty() &&
		   requests[0].first == -1 &&
		   requests[0].second == -1)
		{
			firstPlayer = true;
		}
		
		// 恢复棋盘
		if(firstPlayer)
		{
			myColor = 1;
			int oppColor = -1;
			for(size_t i = 0; i < responses.size(); i++)
			{
				int x = responses[i].first;
				int y = responses[i].second;
				if(x != -1 && y != -1)
				{
					board[x][y] = myColor;
					n++;
				}
				if(i + 1 < requests.size())
				{
					x = requests[i + 1].first;
					y = requests[i + 1].second;
					if(x != -1 && y != -1)
					{
						board[x][y] = oppColor;
						n++;
					}
				}
			}
		}
		else
		{
			myColor = -1;
			int oppColor = 1;
			for(size_t i = 0; i < requests.size(); i++)
			{
				int x = requests[i].first;
				int y = requests[i].second;
				if(x != -1 && y != -1)
				{
					board[x][y] = oppColor;
					n++;
				}
				if(i < responses.size())
				{
					x = responses[i].first;
					y = responses[i].second;
					if(x != -1 && y != -1)
					{
						board[x][y] = myColor;
						n++;
					}
				}
			}
		}
	}
	else
	{
		istringstream iss(line);
		iss >> n;
		for (int i = 0; i < n; i++)
		{
			int x, y;
			iss >> x >> y;
			if (x >= 0 && x < 9 && y >= 0 && y < 9)
			{
				if (i % 2 == 0)
					board[x][y] = 1;
				else
					board[x][y] = -1;
			}
		}
		if(!isJsonMode)
		{
			myColor = (n % 2 == 0 ? 1 : -1);
		}
	}
	
	int moveCount = n;
	
	int new_x = 0, new_y = 0;
	
	pair<int, int> openingMove = lookupOpeningBook(moveCount);

	if (openingMove.first != -1 &&
        isFinalLegal(openingMove.first,
                     openingMove.second,
                     myColor,
                     board)) {

	    new_x = openingMove.first;
	    new_y = openingMove.second;

	} else {
	    pair<int, int> result = mctsSearch(board, myColor);

	    new_x = result.first;
	    new_y = result.second;
	}

	if (!isFinalLegal(new_x, new_y, myColor, board)) {
		new_x = -1;
		vector<pair<int, int>> moves = getValidMoves(board, myColor);
		for (size_t i = 0; i < moves.size(); i++) {
			if (isFinalLegal(moves[i].first, moves[i].second, myColor, board)) {
				new_x = moves[i].first;
				new_y = moves[i].second;
				break;
			}
		}
		if (new_x == -1) {
			bool found = false;
			for (int i = 0; i < 9 && !found; i++) {
				for (int j = 0; j < 9 && !found; j++) {
					if (isFinalLegal(i, j, myColor, board)) {
						new_x = i;
						new_y = j;
						found = true;
					}
				}
			}
		}
		if (new_x == -1) {
			new_x = 4;
			new_y = 4;
			if (!isFinalLegal(new_x, new_y, myColor, board)) {
				bool found = false;
				for (int i = 0; i < 9 && !found; i++)
					for (int j = 0; j < 9; j++)
						if (board[i][j] == 0 && isFinalLegal(i, j, myColor, board)) {
							new_x = i; new_y = j;
							found = true;
							break;
						}
			}
		}
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