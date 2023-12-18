#include <iostream>
#include <cstdio>
#include <vector>
#include <fstream>
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

// check diag
bool check(vector<int> M, int color){
    for(int i=0;i<20;i++){
        if(M[i]==color){
            if(i%5!=0){
                if(M[i+4]==color&&M[i+5]!=color&&M[i-1]!=color) return false;
            }
            if(i%5!=4){
                if(M[i+6]==color&&M[i+5]!=color&&M[i+1]!=color) return false;
            }
        }
    }
    return true;
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

void DFS(vector<int> &M, int cnt, int max, int num){
    // cout << "cnt: " << cnt << "\n";
    if(cnt<=0){
        pair<int, int> *p = check2(M);
        // if(p) cout << p->first << "-" << p->second << "\n";
        // else cout << "error\n";
        if(check(M, 1)&&p&&check3(M, num)){
            print(M);
            vector<int> w;
            for(int i=0;i<25;i++){
                if(M[i]) w.push_back(i);
            }
            W[num].push_back(w);
            WW[num][p->first][p->second].push_back(w);
            // MM.push_back(M);
            return;
        }
    }else{
        for(int i=max;i<=25-cnt;i++){
            cnt=cnt-1;
            M[i] = 1;
            DFS(M, cnt, i+1, num);
            M[i] = 0;
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



int main(){
    // char *s = (char *)malloc(sizeof(char) * 50);
    // for(int i=12;i<=12;i++){
    //     ofstream file;
    //     cout << i << "\n";
    //     sprintf(s, "winning_path_%d.txt", i);
    //     file.open(s);
    //     cout << i <<"====\n";
    //     DFS(MM, i, 0, i);
    //     // file << "total: " << W[i].size() << "\n";
    //     // total+=W[i].size();
    //     // for(int h=0;h<5;h++){
    //     // }
    //     // for(int j=0;j<W[i].size();j++){
    //     //     int k=0;
    //     //     for(int index=0;index<25;index++){
    //     //         if(index==W[i][j][k]) {
    //     //             file << "1 ";
    //     //             k++;
    //     //         }
    //     //         else file << "0 ";
    //     //         if(index%5==4) file << '\n';
    //     //     }
    //     //     if(j!=W[i].size()-1) file << "----------\n";
    //     // }
    //     // file.close();
    //     for(int h=0;h<5;h++){
    //         for(int t=0;t<5;t++){
    //             file << "head: " << h << " ";
    //             file << "tail: " << t << "\n";
    //             // cout << "head: " << h << " ";
    //             // cout << "tail: " << t << "\n";
    //             for(int j=0;j<WW[i][h][t].size();j++){
    //                 int k=0;
    //                 for(int index=0;index<25;index++){
    //                     if(index==WW[i][h][t][j][k]) {
    //                         file << "1 ";
    //                         cout << "1 ";
    //                         k++;
    //                     }
    //                     else {
    //                         file << "0 ";
    //                         cout << "0 ";
    //                     }
    //                     if(index%5==4) {
    //                         file << '\n';
    //                         cout << '\n';
    //                     }
    //                 }
    //                 if(j!=WW[i][h][t].size()-1) {
    //                     file << "----------\n";
    //                     cout << "----------\n";
    //                 }
    //             }
    //         }
    //     }
    //     file.close();
    // }

    vector<int> V(25, 0);
    //criticle point sets
    vector<vector<int>> cps = {{5, 6}, {10, 6}};
    V[0] = 1;
    V[1] = 1;
    V[21] = 1;
    V[11] = 1;
    V[16] = 1;
    print(V);
    DFSW(V, 5, 0, 5, cps);
    for(int i=0;i<P.size();i++){
        print(P[i]);
    }
    cout << "all: " << total << "\n";  

}



