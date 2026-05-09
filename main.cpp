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
	int opponentColor = -myColor;
	
	int bestScore = -10000;
	int new_x = 0, new_y = 0;
	
	for (int i = 0; i < 9; i++) {
	    for (int j = 0; j < 9; j++) {
	        if (judgeAvailable(i, j, myColor)) {
            board[i][j] = myColor;
            int eval = minimax(board, MAX_DEPTH, -10000, 10000, opponentColor, myColor);
            board[i][j] = 0;
	            
	            if (eval > bestScore) {
	                bestScore = eval;
	                new_x = i;
	                new_y = j;
	            }
	        }
	    }
	}

	/***********在上方填充你的代码，决策结果（本方将落子的位置）存入new_x和new_y中****************/
	/************************************************************************************/



	cout << new_x << ' ' << new_y << endl;
	return 0;
}
