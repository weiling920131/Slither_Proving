#include <iostream>
#include <vector>
using namespace std;
vector<vector<int>> MM;
vector<int> M (25, 0);
int total = 0;

bool check(vector<int> &M){
    for(int i=0;i<20;i++){
        if(M[i]==1){
            if(i%5!=0){
                if(M[i+4]==1&&M[i+5]!=1&&M[i-1]!=1) return false;
            }
            if(i%5!=4){
                if(M[i+6]==1&&M[i+5]!=1&&M[i+1]!=1) return false;
            }
        }
    }
    return true;
}


void DFS(vector<int> &M, int cnt, int max){
    // cout << "cnt: " << cnt << "\n";
    if(cnt<=0){
        if(check(M)){
            total++;
            for(int i=0;i<25;i++){
                cout << M[i] << " ";
                if(i%5==4) cout << "\n";
            }
            cout << "============\n";
            MM.push_back(M);
            return;
        }
        
    }else{
        for(int i=max;i<=25-cnt;i++){
            cnt=cnt-1;
            M[i] = 1;
            DFS(M, cnt, i+1);
            M[i] = 0;
            cnt=cnt+1;
        }
        return;
    }
}

int main(){
    DFS(M, 2, 0);
    cout << "total: " << total << "\n";
    cout << "total: " << MM.size() << "\n";
}