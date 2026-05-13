from math import sqrt
import torch
import torch.nn as nn
import numpy as np
import json


boardH = 9
boardW = 9
input_c=3


BaGuaStr='' \
         '. . . . x . . . . '\
         '. x . . . . . x . '\
         '. . . . . . . . . '\
         '. . . . . . . . . '\
         'x . . . x . . . x '\
         '. . . . . . . . . '\
         '. . . . . . . . . '\
         '. x . . . . . x . '\
         '. . . . x . . . . '
BaGuaBoard=np.zeros((boardH*boardW,))

for i in range(boardH*boardW):
    c=BaGuaStr[2*i]
    if(c=='x'):
        BaGuaBoard[i]=1.0

class CNNLayer(nn.Module):
    def __init__(self, in_c, out_c):
        super().__init__()
        self.conv=nn.Conv2d(in_c,
                      out_c,
                      3,
                      stride=1,
                      padding=1,
                      dilation=1,
                      groups=1,
                      bias=False,
                      padding_mode='zeros')
        self.bn= nn.BatchNorm2d(out_c)

    def forward(self, x):
        y = self.conv(x)
        y = self.bn(y)
        y = torch.relu(y)
        return y


class ResnetLayer(nn.Module):
    def __init__(self, inout_c, mid_c):
        super().__init__()
        self.conv_net = nn.Sequential(
            CNNLayer(inout_c, mid_c),
            CNNLayer(mid_c, inout_c)
        )

    def forward(self, x):
        x = self.conv_net(x) + x
        return x

class Outputhead_v1(nn.Module):

    def __init__(self,out_c,head_mid_c):
        super().__init__()
        self.cnn=CNNLayer(out_c, head_mid_c)
        self.valueHeadLinear = nn.Linear(head_mid_c, 3)
        self.policyHeadLinear = nn.Conv2d(head_mid_c, 1, 1)

    def forward(self, h):
        x=self.cnn(h)

        # value head
        value = x.mean((2, 3))
        value = self.valueHeadLinear(value)

        # policy head
        policy = self.policyHeadLinear(x)
        policy = policy.squeeze(1)

        return value, policy



class Model_ResNet(nn.Module):

    def __init__(self,b,f):
        super().__init__()
        self.model_name = "res"
        self.model_size=(b,f)

        self.inputhead=CNNLayer(input_c, f)
        self.trunk=nn.ModuleList()
        for i in range(b):
            self.trunk.append(ResnetLayer(f,f))
        self.outputhead=Outputhead_v1(f,f)

    def forward(self, x):
        if(x.shape[1]==2):#global feature is none
            x=torch.cat((x,torch.zeros((1,input_c-2,boardH,boardW))),dim=1)
        h=self.inputhead(x)

        for block in self.trunk:
            h=block(h)

        return self.outputhead(h)


ModelDic = {
    "res": Model_ResNet
}

class Board:
    def __init__(self):
        self.board = np.zeros(shape=(2, boardH, boardW))
    def play(self,x,y,color):
        self.board[color,y,x]=1
    def isLegal(self,x,y):
        if(self.board[0,y,x] or self.board[1,y,x]):
            return False

        result=True
        self.board[0,y,x]=1
        if(self.isDead(x,y,0)):
            result=False
        if(self.isDead(x+1,y,1)):
            result=False
        if(self.isDead(x-1,y,1)):
            result=False
        if(self.isDead(x,y+1,1)):
            result=False
        if(self.isDead(x,y-1,1)):
            result=False

        self.board[0,y,x]=0
        return result

    def isDead(self,x,y,color):
        if( x<0 or x>=boardW or y<0 or y>=boardH or self.board[color,y,x]==0 ):
            return False
        self.visited=np.zeros((boardH,boardW))
        return (not self.hasLibertyIter(x,y,color))

    def hasLibertyIter(self,x,y,color):
        if(x<0 or x>=boardW or y<0 or y>=boardH):
            return False
        if(self.board[0,y,x]==0 and self.board[1,y,x]==0):
            return True
        if(self.board[color,y,x]==0):
            return False
        if(self.visited[y,x]):
            return False
        self.visited[y, x]=1
        if(self.hasLibertyIter(x+1,y,color)):
            return True
        if(self.hasLibertyIter(x-1,y,color)):
            return True
        if(self.hasLibertyIter(x,y+1,color)):
            return True
        if(self.hasLibertyIter(x,y-1,color)):
            return True
        return False




if __name__ == '__main__':
    file_path="data/40x128.pth"
    #这份代码并没有什么特别之处，就是调用了一个非常基础的resnet
    #猜猜我的model怎么来的
    modeldata = torch.load(file_path,map_location=torch.device('cpu'))

    model_type=modeldata['model_name']
    model_param=modeldata['model_size']
    model = ModelDic[model_type](*model_param)
    model.load_state_dict(modeldata['state_dict'])
    model.eval()

    board = Board()
    #
    # boardstr='' \
    #          'x x x x x o x x x '\
    #          'x x x o . . o o . '\
    #          'o o x x x o . o x '\
    #          'x o o o o x o x o '\
    #          'x x x x o . o x . '\
    #          'o x o . o o o x o '\
    #          '. o x o x . x o x '\
    #          'x o x x o x o o . '\
    #          'o . o x . o o o x '
    #
    #
    # for y in range(9):
    #     for x in range(9):
    #         c=boardstr[2*(y*9+x)]
    #         if(c=='x'):
    #             board.play(x,y,1)
    #         if(c=='o'):
    #             board.play(x,y,0)


    # 解析读入的JSON
    full_input = json.loads(input())
    if "data" in full_input:
        my_data = full_input["data"]  # 该对局中，上回合该Bot运行时存储的信息
    else:
        my_data = None
    # 分析自己收到的输入和自己过往的输出，并恢复状态
    all_requests = full_input["requests"]
    all_responses = full_input["responses"]
    for i in range(len(all_responses)):
        myInput = all_requests[i]  # i回合我的输入
        myOutput = all_responses[i]  # i回合我的输出
        if int(myInput['x']) == -1:
            pass
        else:
            board.play(int(myInput['x']),int(myInput['y']),1)
        board.play(int(myOutput['x']),int(myOutput['y']),0)

    # 看看自己最新一回合输入
    curr_input = all_requests[-1]
    board.play(int(curr_input['x']),int(curr_input['y']),1)

    nnboard = torch.FloatTensor(board.board)
    nnboard.unsqueeze_(0)
    v,p = model(nnboard)


    policytemp=0.3

    policy=p.detach().numpy().reshape((-1))
    policy=policy-np.max(policy)

    for i in range(boardW*boardH):
        if not board.isLegal(i%boardW,i//boardW):
            policy[i]=-10000
        elif(BaGuaBoard[i]):
            policy[i]=policy[i]+300

    policy=policy-np.max(policy)
    for i in range(boardW*boardH):
        if(policy[i]<-1):
            policy[i]=-10000
    policy=policy/policytemp
    probs=np.exp(policy)
    probs=probs/sum(probs)
    for i in range(boardW*boardH):
        if(probs[i]<1e-3):
            probs[i]=0

    action = int(np.random.choice([i for i in range(boardW*boardW)],p=probs))
    my_action = {"x": action % boardW, "y": action // boardW}
    print(json.dumps({
        "response": my_action
    }))