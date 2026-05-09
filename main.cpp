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
	
	auto countLiberties = [&](int fx, int fy) -> int {
	    bool visited[9][9] = {false};
	    int liberties = 0;
	    vector<pair<int, int>> stack;
	    stack.push_back({fx, fy});
	    visited[fx][fy] = true;
	    
	    while (!stack.empty()) {
	        auto [x, y] = stack.back();
	        stack.pop_back();
	        
	        for (int dir = 0; dir < 4; dir++) {
	            int dx = x + cx[dir], dy = y + cy[dir];
	            if (inBorder(dx, dy)) {
	                if (board[dx][dy] == 0) {
	                    liberties++;
	                } else if (board[dx][dy] == board[fx][fy] && !visited[dx][dy]) {
	                    visited[dx][dy] = true;
	                    stack.push_back({dx, dy});
	                }
	            }
	        }
	    }
	    return liberties;
	};
	
	int bestScore = -10000;
	int new_x = 0, new_y = 0;
	
	for (int i = 0; i < 9; i++) {
	    for (int j = 0; j < 9; j++) {
	        if (judgeAvailable(i, j, myColor)) {
	            board[i][j] = myColor;
	            int myLiberties = countLiberties(i, j);
	            board[i][j] = 0;
	            
	            int score = myLiberties * 10 + positionWeight[i][j];
	            
	            for (int dir = 0; dir < 4; dir++) {
	                int dx = i + cx[dir], dy = j + cy[dir];
	                if (inBorder(dx, dy) && board[dx][dy] == opponentColor) {
	                    board[i][j] = myColor;
	                    int oppLiberties = countLiberties(dx, dy);
	                    board[i][j] = 0;
	                    score += oppLiberties;
	                }
	            }
	            
	            if (score > bestScore) {
	                bestScore = score;
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
