import yaml
import re
import sys
import os

def changeCHeader(tPath, oPath, bSize):
    with open(tPath,'r') as f1, open(oPath,'w') as f2:
        for line in f1:
            if line.startswith("constexpr"):
                s=line.split()
                if s[2]=='kBoardSize':
                    line = re.sub(r'[0-9]+',bSize,line)
                    # print(line)
            f2.write(line)
    print(f'Output: "{oPath}" with board size = {bSize}')

def changeLearnerYaml(tPath, oPath, bSize):
    with open(tPath,'r') as f1, open(oPath,'w') as f2:
        # y = yaml.safe_load(f1)
        # y['model']['kBoardSize'] = int(bSize)
        # yaml.dump(y, f2, default_flow_style=False, sort_keys=False)
        for line in f1:
            if line.startswith("kBoardSize:"):
                line = re.sub(r'[0-9]+',bSize,line)
                # print(line)
            f2.write(line)
    print(f'Output: "{oPath}" with board size = {bSize}')

if __name__=="__main__":
    if len(sys.argv)<=1 or not sys.argv[1].isdigit():
        print('Usage: python changeBoardSize.py <BoardSize>')
        exit(0)
    
    bSize=sys.argv[1]
    CHPath='./clap/game/slither/'
    CHoutputName='./slither.h'
    CHtemplateName='slitherCHeader.txt'

    LYPath='./config/'
    LYoutputName='slither.yaml'
    LYtemplateName='slitherYaml.txt'

    changeCHeader(tPath = CHPath+CHtemplateName,oPath = CHPath+CHoutputName ,bSize=bSize)
    changeLearnerYaml(tPath=LYPath+LYtemplateName, oPath=LYPath+LYoutputName, bSize=bSize)

    os.system("pip install -e .")
