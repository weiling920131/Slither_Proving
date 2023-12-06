#include <iostream>
#include <cstdio>
#include <vector>
#include <fstream>
#include <queue>
using namespace std;
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
bool check(vector<int> M){
    for(int i=0;i<20;i++){
        if(M[i]!=0){
            if(i%5!=0){
                if(M[i+4]!=0&&M[i+5]==0&&M[i-1]==0) return false;
            }
            if(i%5!=4){
                if(M[i+6]!=0&&M[i+5]==0&&M[i+1]==0) return false;
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
        if(check(M)&&p&&check3(M, num)){
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

int main(){
    char *s = (char *)malloc(sizeof(char) * 50);
    for(int i=12;i<=12;i++){
        ofstream file;
        cout << i << "\n";
        sprintf(s, "winning_path_%d.txt", i);
        file.open(s);
        cout << i <<"====\n";
        DFS(MM, i, 0, i);
        // file << "total: " << W[i].size() << "\n";
        // total+=W[i].size();
        // for(int h=0;h<5;h++){

        // }
        // for(int j=0;j<W[i].size();j++){
        //     int k=0;
        //     for(int index=0;index<25;index++){
        //         if(index==W[i][j][k]) {
        //             file << "1 ";
        //             k++;
        //         }
        //         else file << "0 ";
        //         if(index%5==4) file << '\n';
        //     }
        //     if(j!=W[i].size()-1) file << "----------\n";
        // }
        // file.close();
        for(int h=0;h<5;h++){
            for(int t=0;t<5;t++){
                file << "head: " << h << " ";
                file << "tail: " << t << "\n";
                // cout << "head: " << h << " ";
                // cout << "tail: " << t << "\n";
                for(int j=0;j<WW[i][h][t].size();j++){
                    int k=0;
                    for(int index=0;index<25;index++){
                        if(index==WW[i][h][t][j][k]) {
                            file << "1 ";
                            cout << "1 ";
                            k++;
                        }
                        else {
                            file << "0 ";
                            cout << "0 ";
                        }
                        if(index%5==4) {
                            file << '\n';
                            cout << '\n';
                        }
                    }
                    if(j!=WW[i][h][t].size()-1) {
                        file << "----------\n";
                        cout << "----------\n";
                    }
                }
            }
        }
        file.close();

    }

    

    // vector<int> V(25, 0);
    // V[0] = 1;
    // V[5] = 1;
    // V[10] = 1;
    // V[15] = 1;
    // V[16] = 1;
    // zone(V, 6);
    // for(int i=0;i<CP.size();i++){
    //     V[CP[i]] = 3;
    // }
    // print(V);
    // cout << "all: " << total << "\n";

}