#include <iostream>
#include <cstdio>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <queue>
using namespace std;
vector<vector<int>> P;
vector<int> MM (25, 0);
int total = 0;
vector<vector<vector<int>>> W (12);
vector<vector<vector<vector<vector<int>>>>> WW (12, vector<vector<vector<vector<int>>>>(5, vector<vector<vector<int>>>(5)));
vector<int> CP;

void print(vector<int> M){
    for(int i=0;i<25;i++){
        cout << M[i] << " ";
        if(i%5==4) cout << "\n";
    }
    cout << "--------\n";
    return;
}

//check redundant
bool check3(vector<int> M, int num){
    for(int i=5;i<num;i++){
        for(int j=0;j<W[i].size();j++){
            int f = 0;
            for(int k=0;k<i;k++){
                if(M[W[i][j][k]]) f++;
            }
            if (f==i) return false;
        }
    }
    return true;
}
//check win
pair<int, int> *check2(vector<int> M){
    pair<int, int> *p = new pair<int, int>;
    int head, tail;
    for(int i=0;i<20;i++){
        if(M[i]==i/5+1&&M[i+5]) {
            if(i<5) head = i;
            M[i+5] = (i+5)/5+1;
            if(i<15){
                for(int j=1;j+i%5<=4;j++){
                    if(M[i+5+j]) M[i+5+j] = (i+5)/5+1;
                    else break;
                }
                for(int j=1;i%5-j>=0;j++){
                    if(M[i+5-j]) M[i+5-j] = (i+5)/5+1;
                    else break;
                }
            }
        }       
    }
    for(int i=20;i<25;i++){
        if(M[i]==5) {
            tail = i%5;
            *p=make_pair(head, tail);
            return p;
        }
    }
    return NULL;
}

bool newcheck2(vector<int> M, int color){
    vector<bool> m(25, false);
    for(int i=0;i<20;i++){
        if(i<5&&M[i]==color) m[i]=true;
        if(m[i]==true) {
            if(M[i+5]==color&&m[i+5]==false){
                m[i+5]=true;
                if(i%5!=0&&M[i+4]==color&&m[i+4]==false) m[i+4]==true;
                if(i%5!=4&&M[i+6]==color&&m[i+6]==false) m[i+6]==true;
            }
        }
    }
    for(int i=20;i<25;i++){
        if(m[i]) return true;
    }
    return false;
}

// check diag
bool check(vector<int> M, int color){
    for(int i=0;i<20;i++){
        if(M[i]==color){
            if(i%5!=0){
                if(M[i+4]==color&&M[i+5]!=color&&M[i-1]!=color) {
                    // return {i+5, i-1};
                    return false;
                }
            }
            if(i%5!=4){
                if(M[i+6]==color&&M[i+5]!=color&&M[i+1]!=color) {
                    return false;
                    // return {i+5, i+1};
                    
                }
            }
        }
    }
    return true;
}



bool check_move(vector<int> &M, int pos, int D){
    vector<int> dir = {-5, -1, 1, 5, -6, -4,  4, 6};
    //移動到place
    for(int i=0;i<8;i++){
        if(M[pos+dir[i]]==1&&pos+dir[i]!=D){
            M[pos+dir[i]] = 2;
            M[pos] = 1;

            if(check(M, 1)){
                printf("%d -> %d\n", pos+dir[i], pos);
                print(M);
                return true;
            }
            M[pos+dir[i]] = 1;
            M[pos] = 2;
        }
    }
    //移動在下place
    if(D!=-1) return false;
    M[pos] = 1;
    for(int i=0;i<4;i++){
        if(M[pos+dir[i]]==2){
            if(check_move(M, pos+dir[i], pos)) return true;
        }
    }
    
    return false;
   
}

bool check4(vector<int> M, vector<vector<int>> pos){
    vector<int> dir = {-5, -1, 1, 5, -6, -4,  4, 6};
    for(int j=0;j<pos.size();j++){
        if(pos[j].size()>=3){
            continue;
        }
        bool b = false;
        bool moved = false;
        if(!check_move(M, pos[j][0], -1)){ //無法移動到pos
            M[pos[j][0]] = 1; //移動加下
            for(int i=0;i<4;i++){ //相鄰四格哪一格是空
                if(M[pos[j][0]+dir[i]]==2){
                    if(check_move(M, pos[j][0]+dir[i], pos[j][0])) {
                        if(pos[j].size()==1) return true; //只有1顆
                        else { //兩顆
                            b = true;
                            moved = true;
                            break;
                        }
                    }
                }
            }
        }else{
            if(pos[j].size()==1) return true; // 可以移動到pos
            else{
                b = true;
                moved = true;
            }
        }
        if(!b){ //直接用下的
            M[pos[j][0]] = 1;
            if(check(M, 1)) {
                printf("place %d\n", pos[j][0]);
                print(M);
                if(pos[j].size()==1) return true; //只有1顆
                else b = true;
            }else{ //這個critical point沒法檔
                M[pos[j][0]] = 2;
                break;
            }
        }
      
        if(moved){ //移動過直接用下的
            M[pos[j][1]] = 1;
            if(check(M, 1)) {
                printf("place %d\n", pos[j][0]);
                print(M);
                return true; 
            }else{ //這個critical point沒法檔
                M[pos[j][1]] = 2;
                break;
            }
        }
        if(!check_move(M, pos[j][1], -1)){ //無法移動到pos
            M[pos[j][1]] = 1; //移動加下
            for(int i=0;i<4;i++){ //相鄰四格哪一格是空
                if(M[pos[j][0]+dir[i]]==2){
                    if(check_move(M, pos[j][1]+dir[i], pos[j][1])) {
                        return true;
                    }
                }
            }
        }else{
            return true; // 可以移動到pos
        }
    }
    return false;
}

void zone(vector<int> &M, int p){
    cout << "========\n";
    for(int i=0;i<25;i++){
        if(!M[i]) {
            M[i] = 1;
            for(int k=5;k<=p;k++){
                for(int j=0;j<W[k].size();j++){
                    int flag = true;
                    for(int ii=0;ii<W[k][j].size();ii++){
                        if(!M[W[k][j][ii]]) {
                            flag = false;
                            // break;
                        }
                    }
                    if(flag) {
                        CP.push_back(i);
                    }
                }
            }
            M[i] = 0;
        }
    }
    return;
}

void DFS_checkmate(vector<int> &W, int cnt, int num){
    for(int i=0;i<num;i++){
        for(int j=0;j<num;j++){
            // if()
        }
    }
    
}

void DFS(vector<int> &M, int cnt, int max, int num,int color){
    // cout << "cnt: " << cnt << "\n";
    if(cnt<=0){
        // if(p) cout << p->first << "-" << p->second << "\n";
        // else cout << "error\n";
        if(check(M, color)){
            if(newcheck2(M, color)) print(M);
            return;
        }
    }else{
        for(int i=max;i<=25-cnt;i++){
            cnt=cnt-1;
            M[i] = color;
            DFS(M, cnt, i+1, num, color);
            M[i] = 2;
            cnt=cnt+1;
        }
        return;
    }
}

bool check_blocked(vector<int> M, vector<vector<int>>CPs){
    // for every crtical points set
    bool blocked = true;
    for(int i=0;i<CPs.size();i++){
        // check if white has block every critical points
        for(int j=0;j<CPs[i].size();j++){
            if(M[CPs[i][j]]!=2) blocked = false;
        }
        if(blocked) {
            // print(M);
            // total++;
            return blocked;
        }
    }
    return blocked;
}

void DFSW(vector<int> &M, int cnt, int max, int num, vector<vector<int>>CPs){
    if(cnt<=0){
        if(check(M, 2)&&!check_blocked(M, CPs)){
            // print(M);
            P.push_back(M);
            total++;
            return;
        } 
    }else{
        for(int i=max;i<=25-cnt;i++){
            if(M[i]==1) continue;
            cnt=cnt-1;
            M[i] = 2;
            DFSW(M, cnt, i+1, num, CPs);
            M[i] = 0;
            cnt=cnt+1;
        }
        return;
    }
}

unordered_map<uint64_t, int> TT;

uint64_t convert_to_uint64_t(const vector<int>& M) {
    uint64_t board = 0;
    for (int i = 0; i < 25; i++) {
        if (M[i] == 0) {
            board |= (uint64_t(1) << i);
        } else if (M[i] == 1) {
            board |= (uint64_t(1) << (i + 25));
        }
    }
    return board;
}

void store_in_TT(vector<int> M, int label){
	uint64_t board = convert_to_uint64_t(M);
    TT[board] = {label};
}

bool lookup_TT(vector<int> M){
    uint64_t board = convert_to_uint64_t(M);
    auto it = TT.find(board);
    if (it != TT.end()) {
        return true;
    } else {
        return false;
    }
}

int main(){
    vector<int> M(25);
    vector<int> M2(25);
    M ={2, 2, 2, 2, 2,
        1, 2, 0, 0, 2,
        1, 2, 0, 2, 2,
        1, 2, 0, 2, 2,
        1, 2, 0, 2, 2};
    M2 ={2, 2, 2, 2, 2,
        1, 2, 0, 0, 2,
        1, 2, 0, 2, 2,
        1, 2, 0, 2, 2,
        1, 2, 0, 2, 2};
    store_in_TT(M, 0);
    bool label = lookup_TT(M2);
    if (label) {
        cout << "Found in transposition table with label"<< endl;
    } else {
        cout << "Not found in transposition table" << endl;
    }

    return 0;
}