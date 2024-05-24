#include "clap/game/slither/slither.h"
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stack>
#include <iostream>
#include <set>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>

namespace clap::game::slither {

SlitherState::SlitherState(GamePtr game_ptr)
    : State(std::move(game_ptr)),
      turn_(0),
      skip_(0),
      winner_(-1) {
  std::fill(std::begin(board_), std::end(board_), EMPTY);
  history_.clear();
  W = std::vector<std::vector<std::vector<int>>>(12);
  W_ht = std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>>(12, std::vector<std::vector<std::vector<std::vector<int>>>>(5, std::vector<std::vector<std::vector<int>>>(5)));
    // noBlock = std::vector<std::vector<int>>;
}
int SlitherGame::getBoardSize() const {
	return kBoardSize;
}
bool SlitherGame::save_manual(const std::vector<Action> actions, const std::string savepath) const {
	std::fstream sgf_file(savepath, std::fstream::out | std::fstream::app);
	if (sgf_file.is_open()) {
		auto pos_int_to_char = [](const int pos) {
			int rpos = pos / kBoardSize;
			int cpos = pos % kBoardSize;
			std::string res;
			res.push_back(cpos + 'A');
			res.push_back(kBoardSize - rpos - 1 + 'A');
			return res;
		};
		sgf_file << "(";
		sgf_file << ";GM[511]";
		for (int i = 0; i < actions.size(); ) {
			std::vector<Action> one_move;
			for (int j = 0; j < 3; ++i, ++j) {
				one_move.push_back(actions[i]);
			}
			if (one_move[0] < kNumOfGrids) {
				sgf_file << ((((i / 3) % 2) != 0) ? ";B[" : ";W[");
				sgf_file << pos_int_to_char(one_move[0]);
				sgf_file << pos_int_to_char(one_move[1]);
				sgf_file << "]";
			}
			sgf_file << ((((i / 3) % 2) != 0) ? ";B[" : ";W[");
			sgf_file << pos_int_to_char(one_move[2]);
			sgf_file << "]";
		}
		sgf_file << ")";
		sgf_file << "\n";
		
		sgf_file.close();
		return true;
	}
	return false;
}

std::unique_ptr<State> SlitherState::clone() const {
  return std::make_unique<SlitherState>(*this);
}

void SlitherState::apply_action(const Action &action) {
  
//   std::cout << "current turn:" << turn_ << "\n"; 
//   std::cout << "action:" << action << '\n';
  if (turn_ % 3 == 0) {  // choose
		if (action == empty_index) skip_ = 1;
		++turn_;
		history_.push_back(action);
		//std::cout << "success\n";
	} 
	else if (turn_ % 3 == 1) { // move
		Player player = current_player();
		const int src = history_[history_.size() - 1];
		if(src != empty_index && action != empty_index){
			std::swap(board_[src], board_[action]);
			board_[src] = EMPTY;
		} else if (action == empty_index) {
			skip_ = 1;
			history_[history_.size() - 1] = empty_index;
		}
		++turn_;
		history_.push_back(action);
	} 
	else {  
		// place new piece
		Player player = current_player();
		board_[action] = player;
		++turn_;
		history_.push_back(action);
		// check if valid
		if (skip_ == 1) { // only play a new piece
			skip_ = 0;
			if (!is_valid(action)) {
				if(player == BLACK)
					winner_ = WHITE;
				else 
					winner_ = BLACK;
			}
			// check if win
			if(winner_ == -1 && have_win(action))
				winner_ = player;
		}
		else {
			const int src_empty = history_[history_.size() - 3]; // piece -> empty
			const int src_new_piece = history_[history_.size() - 2]; // empty -> piece
			if (!is_valid(action) || !is_valid(src_new_piece) || (action != src_empty && !is_empty_valid(src_empty) ) ) {
				if(player == BLACK)
					winner_ = WHITE;
				else 
					winner_ = BLACK;
			}
			// check if win
			if(winner_ == -1){
				if(have_win(action) || have_win(src_new_piece))
					winner_ = player;
			}
		}
	}
}

void SlitherState::manual_action(const Action &action, Player p) {
 	if (turn_ % 3 == 0) {  // choose
		if (p != current_player()) turn_ += 3;
		if (action == empty_index) skip_ = 1;
		++turn_;
		history_.push_back(action);
		//std::cout << "success\n";
	} 
	else if (turn_ % 3 == 1) { // move
		Player player = current_player();
		const int src = history_[history_.size() - 1];
		if(src != empty_index && action != empty_index){
			std::swap(board_[src], board_[action]);
			board_[src] = EMPTY;
		} else if (action == empty_index) {
			skip_ = 1;
			history_[history_.size() - 1] = empty_index;
		}
		++turn_;
		history_.push_back(action);
	} 
	else {  
		// place new piece
		Player player = current_player();
		board_[action] = player;
		++turn_;
		history_.push_back(action);
		// check if valid
		if (skip_ == 1) { // only play a new piece
			skip_ = 0;
			if (!is_valid(action)) {
				if(player == BLACK)
					winner_ = WHITE;
				else 
					winner_ = BLACK;
			}
			// check if win
			if(winner_ == -1 && have_win(action))
				winner_ = player;
		}
		else {
			const int src_empty = history_[history_.size() - 3]; // piece -> empty
			const int src_new_piece = history_[history_.size() - 2]; // empty -> piece
			if (!is_valid(action) || !is_valid(src_new_piece) || (action != src_empty && !is_empty_valid(src_empty) ) ) {
				if(player == BLACK)
					winner_ = WHITE;
				else 
					winner_ = BLACK;
			}
			// check if win
			if(winner_ == -1){
				if(have_win(action) || have_win(src_new_piece))
					winner_ = player;
			}
		}
	}
}

// 11/7 modified

std::vector<std::vector<int>> SlitherState::get_critical(std::vector<std::vector<int>> pathes) {
	std::vector<std::vector<int>> pathes_1, pathes_2, all_critical, all_pathes;
	for(auto& path: pathes){
		if(path.size() == 1) pathes_1.push_back(path);
		else if(path.size() == 2) pathes_2.push_back(path);
	}

	// CLI_agent.py Line: 259 - 287
	if(pathes_2.size()){
		// CLI_agent.py Line: 260 - 261
		std::stack<std::pair<std::set<int> ,int>> s;
		std::set<int> emp;
		s.push(std::make_pair(emp, 0));
		while(!s.empty()){
			std::set<int> combination = s.top().first;
			int ind = s.top().second;
			s.pop();
			if(ind == pathes_2.size()) {
				all_critical.push_back(std::vector<int>(combination.begin(), combination.end()));
				continue;
			}
			for(auto &point: pathes_2[ind]){
				std::set<int> tmp = combination;
				tmp.insert(point);
				s.push(std::make_pair(tmp, ind+1));
			}
		}

		// CLI_agent.py Line: 263 - 268
		int max_length = 0;
		for(int i = 0;i<all_critical.size();i++){
			for(auto& point: pathes_1){
				if(std::find(all_critical[i].begin(), all_critical[i].end(), point[0]) == all_critical[i].end()){
					all_critical[i].push_back(point[0]);
				}
			}
			if(max_length < all_critical[i].size()) max_length = all_critical[i].size();
		}

		// CLI_agent.py Line: 269 - 271
		std::vector<std::set<std::set<int>>> pathes_n;
		for(int i = 1;i<max_length+1;i++){
			std::set<std::set<int>> tmp;
			for(auto& j:all_critical){
				if(j.size() == i){
					std::set<int> tmp_j(j.begin(), j.end());
					tmp.insert(tmp_j);
				}
			}
			pathes_n.push_back(tmp);
		}

		// CLI_agent.py Line: 273 - 278
		for(int i =0;i<max_length-1;i++){
			for(int j = i+1;j<max_length;j++){
				for(auto& setb: pathes_n[i]){
					std::set<std::set<int>> tmp_pathes_j(pathes_n[j].begin(), pathes_n[j].end());
					for(auto& seta: tmp_pathes_j){
						if(std::includes(seta.begin(), seta.end(), setb.begin(), setb.end())) {	// b is a subset of a
							pathes_n[j].erase(seta);
						}
					}
				}
			}
		}

		// CLI_agent.py Line: 280 - 283
		for(auto& path_n: pathes_n) {
			if(path_n.size() > 0) {
				for(auto& p: path_n) {
					std::vector<int> tmp_p(p.begin(), p.end());
					all_pathes.push_back(tmp_p);
				}
			}
		}

	}
	// CLI_agent.py Line: 285 - 287
	else {
		std::vector<int> tmp_p;
		for(auto& p: pathes_1) {
			tmp_p.push_back(p[0]);
		}
		all_pathes.push_back(tmp_p);
	}

	return all_pathes;
}

std::vector<int> SlitherState::getboard() {
	std::vector<int> board(25); //1 2 0 
	for(int i=0;i<25;i++) {		//0 1 2
		board[i] = board_[i];
	}
	return board;
}


std::vector<std::vector<Action>> SlitherState::test_action(std::vector<Action> path, std::vector<std::vector<Action>> &pathes, Player p) {
	int cur_turn = path.size();
	// std::cout << cur_turn << '\n';
	if(cur_turn == 3) {
		if(winner_ != -1) {
			// path.push_back(winner_);
			pathes.push_back(path);
			return pathes;
		}
		return pathes;
	}
	if (p != current_player()) turn_ += 3;
	// int num_of_win = 0;
	std::vector<Action> actions = legal_actions();
	for(auto action: actions) {
		auto copy_path = path;
		copy_path.push_back(action);
		SlitherState cur_state = SlitherState(*this);
		cur_state.apply_action(action);
		cur_state.test_action(copy_path, pathes, p);
	}
	return pathes;
}

bool SlitherState::test_action_bool(std::vector<Action> path, std::vector<std::vector<Action>> &pathes, Player p) {
	int cur_turn = path.size();
	// std::cout << cur_turn << '\n';
	if(cur_turn == 3) {
		// for (auto& p: path) {
		// 	std::cout << p << ' ';
		// } std::cout << winner_ << '\n';
		if(winner_ != -1) {
			// path.push_back(winner_);
			// pathes.push_back(path);
			return true;
		}
		return false;
	}
	if (p != current_player()) turn_ += 3;
	// int num_of_win = 0;
	std::vector<Action> actions = legal_actions();
	for(auto action: actions) {
		auto copy_path = path;
		copy_path.push_back(action);
		SlitherState cur_state = SlitherState(*this);
		cur_state.apply_action(action);
		if (cur_state.test_action_bool(copy_path, pathes, p)){
			return true;
		}
	}
	return false;
}

bool SlitherState::test_board(std::vector<Action> board) {
	SlitherState cur_state = SlitherState(*this);
	for(int i=0; i<kNumOfGrids; i++) {
		cur_state.board_[i] = board[i];
	}

	std::vector<Action> path;
	std::vector<std::vector<Action>> pathes;
	// for (auto a: cur_state.legal_actions())
	// 	std::cout << a << ' ';
	return cur_state.test_action_bool(path, pathes, BLACK);
}

// 11/7 modified
//whp
//check redundant
bool SlitherState::check_redundent(std::vector<int> M, int num){
    for(int i=5;i<num;i++){
        for(int j=0;j<W[i].size();j++){
            int f = 0;
            for(int k=0;k<i;k++){
                if(M[W[i][j][k]]==1) f++;
            }
            if (f==i) return false;
        }
    }
    return true;
}

//check win
bool SlitherState::check_win(std::vector<int> M, int color){
    std::vector<bool> m(25, false);
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

// std::pair<int, int> * SlitherState::check_win(std::vector<int> M){
//     std::pair<int, int> *p = new std::pair<int, int>;
// 	int head, tail;
// 	for(int i=0;i<20;i++){
//         if(M[i]==i/5+1&&M[i+5]) {
// 			if(i<5) head = i;
//             M[i+5] = (i+5)/5+1;
//             if(i<15){
//                 for(int j=1;j+i%5<=4;j++){
//                     if(M[i+5+j]) M[i+5+j] = (i+5)/5+1;
//                     else break;
//                 }
//                 for(int j=1;i%5-j>=0;j++){
//                     if(M[i+5-j]) M[i+5-j] = (i+5)/5+1;
//                     else break;
//                 }
//             }
//         }       
//     }
//     for(int i=20;i<25;i++){
//         if(M[i]==5) {
//             tail = i%5;
//             *p=std::make_pair(head, tail);
//             return p;
//         }
//     }
//     return NULL;
// }


// check diag
bool SlitherState::check_diag(std::vector<int> M, int color){
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

bool SlitherState::check_blocked(std::vector<int> M, std::vector<std::vector<int>>CPs){
	int blocked = CPs.size();
    // for every crtical points set
    for(int i=0;i<CPs.size();i++){
        // check if white has block every critical points
        for(int j=0;j<CPs[i].size();j++){
            if(M[CPs[i][j]]!=1) {
				blocked--;
				break;
			}
        }
    }
    return blocked;
}

bool SlitherState::check_move(std::vector<int> M, int pos, std::vector<int> w){
    std::vector<int> dir = {-5, -1, 1, 5, -6, -4,  4, 6};
    //移動到place
    for(int i=0;i<8;i++){
		if(pos<5){
			if(dir[i]==-5||dir[i]==-4||dir[i]==-6) continue;
		} else if(pos>=20){
			if(dir[i]==5||dir[i]==4||dir[i]==6) continue;
		}
		if(pos%5==0){
			if(dir[i]==-1||dir[i]==-6||dir[i]==4) continue;
		}else if(pos%5==4){
			if(dir[i]==1||dir[i]==-4||dir[i]==6) continue;
		}
		bool b = false;
		for(int j=0;j<w.size();j++){
			if(pos+dir[i]==w[j]){
				b = true;
				break;
			}
		}
        if(M[pos+dir[i]]==1&&!b){
            M[pos+dir[i]] = 2;
            M[pos] = 1;
            if(check_diag(M, 1)){
                // printf("%d -> %d\n", pos+dir[i], pos);
                return true;
            }
            M[pos+dir[i]] = 1;
            M[pos] = 2;
        }
    }
    
    return false;   
}

uint64_t convert_to_uint64_t(const std::vector<int>& M) {
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

void SlitherState::store_TT(std::unordered_map<uint64_t, int> &TT, std::vector<int> M, int label){
	uint64_t board = convert_to_uint64_t(M);
    TT[board] = {label};
	return;
}

bool SlitherState::lookup_TT(std::unordered_map<uint64_t, int> &TT, std::vector<int> M){
    uint64_t board = convert_to_uint64_t(M);
    auto it = TT.find(board);
    if (it != TT.end()) {
        return true;
    } else {
        return false;
    }
}

bool SlitherState::check_can_block(std::unordered_map<uint64_t, int> &TT){
    std::vector<int> M = getboard();
	std::vector<std::vector<int>> pos = get_critical(match_WP());
	if(pos.size() == 0) return true;
	std::vector<int> dir = {-5, -1, 1, 5};
    for(int j=0;j<pos.size();j++){
		// std::cout << "cp set" << j << ": ";
		int cp_num = pos[j].size();
		std::vector<int> blocked;
		for(int i=0;i<pos[j].size();i++){
			if(M[pos[j][i]]==1){
				cp_num--;
				blocked.push_back(pos[j][i]);
			}
		}
		if(cp_num==0){
			return true;
		}
        if(cp_num>=3){
            continue;
        }
		if(cp_num==1){ //只有pos1git
			M[pos[j][0]] = 1;
			if(check_diag(M, 1)){ //直接下pos1
				// std::cout << "直接下pos1\n"; 
				return true;
			}

			for(int d=0;d<4;d++){ //移動加下pos1
				if(pos[j][0]<5){
					if(dir[d]==-5) continue;
				}else if(pos[j][0]>=20){
					if(dir[d]==5) continue;
				}
				if(pos[j][0]%5==0){
					if(dir[d]==-1) continue;
				}else if(pos[j][0]%5==4){
					if(dir[d]==1) continue;
				}
				if(M[pos[j][0]+dir[d]]==2){
					blocked.push_back(pos[j][0]);
					if(check_move(M, M[pos[j][0]+dir[d]], blocked)){
						// std::cout << "移動加下pos1\n"; 
						return true;
					}
					blocked.pop_back();
				}
			}
			M[pos[j][0]] = 2;
			if(check_move(M, M[pos[j][0]], blocked)){ //移動到pos1
				// std::cout << "移動到pos1\n"; 
				return true;	
			}else{
				for(int d=0;d<4;d++){ //移動到pos1加下
					if(pos[j][0]<5){
						if(dir[d]==-5) continue;
					}else if(pos[j][0]>=20){
						if(dir[d]==5) continue;
					}
					if(pos[j][0]%5==0){
						if(dir[d]==-1) continue;
					}else if(pos[j][0]%5==4){
						if(dir[d]==1) continue;
					}
					M[pos[j][0]+dir[d]] = 1;
					if(check_diag(M, 1)){
						// std::cout << "移動到pos1加下\n"; 
						return true;
					}
					M[pos[j][0]+dir[d]] = 2;
				}
			}
		}else if(cp_num==2){
			M[pos[j][0]] = 1; //移動到pos2下pos1
			blocked.push_back(pos[j][0]);
			if(check_move(M, M[pos[j][1]], blocked)){ 
				// std::cout << "移動到pos2下pos1\n"; 
				return true;
			}
			M[pos[j][0]] = 2;
			blocked.pop_back();
			M[pos[j][1]] = 1; //移動到pos1下pos2
			blocked.push_back(pos[j][1]);
			if(check_move(M, M[pos[j][0]], blocked)){ 
				// std::cout << "移動到pos1下pos2\n"; 
				return true;
			}
			M[pos[j][1]] = 2;
			blocked.pop_back();
		}
    }
	//can't block, store in TT
	store_TT(TT, M, 0);
    return false;
}


std::vector<std::vector<int>> SlitherState::match_WP(){
	std::vector<int> M = getboard();
	int num = 0;
	for(int i=0;i<25;i++){
		if(M[i]==0) num++;
	}
	std::string folder = "./winning_path/";
	std::vector<std::vector<int>> pathes;
	for(int i=5;i<=num+1;i++){
		std::string filename = folder+std::to_string(i)+".txt";
		std::ifstream file;
		file.open(filename);

		std::string line;
        while (std::getline(file, line)) { 
			bool miss_one = false;
			bool miss_two = false;
			bool is_wp = true;
			std::stringstream ss(line);
			std::string point;
			std::vector<int> miss_points;
			std::vector<int> wp;
			while (std::getline(ss, point, ' ')) {
				if(M[std::stoi(point)] != 0){
					// std::cout << point << "\n";
					if(miss_two) 
					{
						is_wp = false;
					}
					else if(miss_one) {
						miss_two = true;
						miss_points.push_back(std::stoi(point));
					}
					else {
						miss_one = true;
						miss_points.push_back(std::stoi(point));
					}
				}/* else if (M[std::stoi(point)]==1) {
					is_wp = false;
				} */
				else{
					wp.push_back(std::stoi(point));
				}
			}
			// for(int i=0;i<pathes.size();i++){
			// 	for(int j=0;j<pathes[i].size();j++){
			// 		std::cout << pathes[i][j] << " ";
			// 	}
			// 	std::cout << "\n";
			// }
			if(is_wp){
				if(miss_two){
					bool can_move = false;
					for(int i=0;i<2;i++){
						if(miss_points[i]/5>0&&miss_points[i]%5!=0&&M[miss_points[i]-6]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]-6)==wp.end()){
							std::swap(M[miss_points[i]-6], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]-6], M[miss_points[i]]);
						}
						if(miss_points[i]/5>0&&M[miss_points[i]-5]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]-5)==wp.end()){
							std::swap(M[miss_points[i]-5], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]-5], M[miss_points[i]]);
						}
						if(miss_points[i]/5>0&&miss_points[i]%5!=4&&M[miss_points[i]-4]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]-4)==wp.end()){
							std::swap(M[miss_points[i]-4], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]-4], M[miss_points[i]]);
						}
						if(miss_points[i]%5!=0&&M[miss_points[i]-1]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]-1)==wp.end()){
							std::swap(M[miss_points[i]-1], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]-1], M[miss_points[i]]);
						}
						if(miss_points[i]%5!=4&&M[miss_points[i]+1]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]+1)==wp.end()){
							std::swap(M[miss_points[i]+1], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]+1], M[miss_points[i]]);
						}
						if(miss_points[i]/5<4&&miss_points[i]%5!=0&&M[miss_points[i]+4]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]+4)==wp.end()){
							std::swap(M[miss_points[i]+4], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]+4], M[miss_points[i]]);
						}
						if(miss_points[i]/5<4&&M[miss_points[i]+5]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]+5)==wp.end()){
							std::swap(M[miss_points[i]+5], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]+5], M[miss_points[i]]);
						}
						if(miss_points[i]/5<4&&miss_points[i]%5!=4&&M[miss_points[i]+6]==0
							&&std::find(wp.begin(), wp.end(), miss_points[i]+6)==wp.end()){
							std::swap(M[miss_points[i]+6], M[miss_points[i]]);
							M[miss_points[1-i]] = 0;
							if(check_diag(M, 0)){
								can_move=true;
							}
							M[miss_points[1-i]] = 2;
							std::swap(M[miss_points[i]+6], M[miss_points[i]]);
						}
					}
					if(can_move){
						pathes.push_back(miss_points);
					}
				} else {
					pathes.push_back(miss_points);
				}
			}
        }
        file.close();
	}
	// for(auto path:pathes) {
	// 	for(auto mp:path) {
	// 		std::cout << mp << ' ';
	// 	} std::cout << '\n';
	// }
	// std::cout << "done\n";
	return pathes;
}

int SlitherState::DFS_noBlock(std::vector<int> &M, int cnt, int max, int num, std::vector<std::vector<int>>CPs, int &board_num){
    if(cnt<=0){
        if(check_diag(M, 1)){
			board_num++;
			if(!check_blocked(M, CPs) && !check_win(M, 1)){
				// print(M);
				noBlock.push_back(M);
			}
		}
    }else{
        for(int i=max;i<=25-cnt;i++){
            if(M[i]==0) continue;
            cnt=cnt-1;
            M[i] = 1;
            DFS_noBlock(M, cnt, i+1, num, CPs, board_num);
            M[i] = 2;
            cnt=cnt+1;
        }
    }
	return board_num;
}

std::vector<std::vector<int>> SlitherState::get_noBlock(){
	// std::cout << "get not blocked\n";
	// for(int i=0;i<noBlock.size();i++){
	// 	for(int j=0;j<25;j++){
	// 		if(noBlock[i][j]==2) std::cout << ". ";
	// 		else if(noBlock[i][j]==0) std::cout << "x ";
	// 		else if(noBlock[i][j]==1) std::cout << "o ";
	// 		if(j%5==4) std::cout << "\n";
	// 	}
	// 	std::cout << "-------------\n";
	// }
	// std::cout << "total: " << noBlock.size() <<"\n";
	std::vector<std::vector<int>> copy = noBlock;
	noBlock.clear();
	return copy;
}

void SlitherState::generate_all(std::vector<std::vector<int>> &MM, std::vector<int> &M, int cnt, int max){
    if(cnt<=0){
        if(check_diag(M, 0)){
            // for(int i=0;i<25;i++){
            //     std::cout << M[i] << " ";
            //     if(i%5==4) std::cout << "\n";
            // }
            // std::cout << "============\n";
            MM.push_back(M);
            return;
        }
	}else{
        for(int i=max;i<=25-cnt;i++){
            cnt=cnt-1;
            M[i] = 0;
            generate_all(MM, M, cnt, i+1);
            M[i] = 2;
            cnt=cnt+1;
        }
        return;
    }
}	

void SlitherState::DFS_WP(std::vector<int> &M, int cnt, int max, int num){
    // cout << "cnt: " << cnt << "\n";
    if(cnt<=0){
        if(check_diag(M, 1)&&check_win(M, 1)&&check_redundent(M, num)){
            std::vector<int> w;
            for(int i=0;i<25;i++){
                if(M[i]) w.push_back(i);
            }
            // W[num].push_back(w);
            // W_ht[num][p->first][p->second].push_back(w);
            return;
        }
    }else{
        for(int i=max;i<=25-cnt;i++){
            cnt=cnt-1;
            M[i] = 1;
            DFS_WP(M, cnt, i+1, num);
            M[i] = 0;
            cnt=cnt+1;
        }
        return;
    }
}

// void SlitherState::generate_WP(){
// 	std::vector<int> M (25, 0);
//     for(int i=5;i<=10;i++){
// 		std::string filename = "winning_path/"+std::to_string(i)+".txt";
//         DFS_WP(M, i, 0, i);
// 		std::ofstream file;
// 		file.open(filename);
// 		for(int j=0;j<W[i].size();j++){
// 			for(int k=0;k<W[i][j].size();k++){
// 				file << W[i][j][k] << " ";
// 			}
// 			file << "\n";
// 		}
// 		file.close();
//     }
	
// }

std::vector<std::vector<int>> SlitherState::generate(int cnt){
	std::vector<std::vector<int>> MM;
	std::vector<int> M (25, 2);
	generate_all(MM, M, cnt, 0);
    std::cout << "total: " << MM.size() << "\n";
	return MM;
}

int SlitherState::test_generate(std::vector<Action> path, int chess_num, int color){
	std::cout<<"generating "<<chess_num << "\n";
	std::string filename = "checkmate/checkmate_"+std::to_string(chess_num)+".txt";
	std::ofstream file;
	file.open(filename);
	std:: vector<char> change = {'x', 'o', '.'};
	std::vector<std::vector<int>> M = generate(chess_num);
	int pruning_num = 0;
	for(auto& m : M){
		SlitherState cur_state = SlitherState(*this);
		for(int i=0;i<5;i++){
			for(int j=0;j<5;j++){
				cur_state.board_[5*i+j] = m[i*5+j];
				// std::cout << m[i*5+j]<<' ';
			}
			// std::cout<<'\n';
		}
		// std::cout<<'\n';
		std::vector<std::vector<Action>> pathes = {};
		if(cur_state.test_action(path, pathes, color).size() >= 1){
			if(cur_state.check_win(cur_state.getboard(), 0)) continue;
			pruning_num++;
			std::vector<int> curBlack;
			// std::cout<<"\n==================\n";
			for(int i=0;i<5;i++){
				// std::cout << 5-i << " ";
				for(int j=0;j<5;j++){
					if(cur_state.board_[i * 5 + j]==0) {
						// curBlack.push_back(i*5+j);
						file << i * 5 + j << " ";  // checkmate
						// file << "X ";
					} 
					// else {
					// 	file << ". ";
					// }
					// std::cout << change[cur_state.board_[i * 5 + j]] <<' ';
				}
				// file << "\n";
				// std::cout<<'\n';
			}
			// std::cout << "  A B C D E\n";
			file << "\n";
		}
	}
	std::cout<<"pruning_num: "<<pruning_num<<'\n';
}

std::string SlitherState::printBoard(std::vector<Action> board, std::vector<std::vector<Action>> CPs) {
	if(board.size() == 0) {
		board = getboard();
	}
	std::stringstream ss;
	const std::vector<std::string> chess{"x", "o", "·", "@"};

	if(CPs.size() > 0) {
		for(const auto CP: CPs) {
			std::vector<Action> newBoard = board;
			for(const auto c: CP) {
				newBoard[c] = 3;
			}
			for (int i = 0; i < kBoardSize; ++i) {
				ss << std::setw(2) << std::setfill(' ') << kBoardSize - i;
				for (int j = 0; j < kBoardSize; ++j) {
					ss << ' ' << chess[newBoard[i * kBoardSize + j]];
				}
				ss << std::endl;
			}
			ss << "  ";
			for (int i = 0; i < kBoardSize; ++i) ss << ' ' << static_cast<char>('A' + i);
			ss << "\n\n";
		}
	}
	else {
		for (int i = 0; i < kBoardSize; ++i) {
			ss << std::setw(2) << std::setfill(' ') << kBoardSize - i;
			for (int j = 0; j < kBoardSize; ++j) {
				ss << ' ' << chess[board[i * kBoardSize + j]];
			}
			ss << std::endl;
		}
		ss << "  ";
		for (int i = 0; i < kBoardSize; ++i) ss << ' ' << static_cast<char>('A' + i);
	}
	
	return ss.str();
}
//whp


std::vector<Action> SlitherState::legal_actions() const {
	std::vector<Action> actions;
	if(turn_ % 3 == 0) {// empty
		// actions.push_back(empty_index);
	}
	else if(turn_ % 3 == 1 && skip_ == 1){
		actions.push_back(empty_index);
		return actions;
	}
	for (uint32_t i = 0; i <= kNumOfGrids; i++) {
		if (is_legal_action(i)) {
			actions.push_back(i);
		}
	}
	if (actions.empty()) {
		actions.push_back(empty_index);
	}

	return actions;
}

bool SlitherState::is_legal_action(const Action &action) const {
	if (action < 0 || action > kNumOfGrids) return false;
	const Player player = current_player();
	if (turn_ % 3 == 0) {  // choose
		if ((board_[action] == player && have_move(action)) || action == kNumOfGrids) {
			// return true;
			return is_selecting_valid(action, player);
		}
	}
	else if (turn_ % 3 == 1) {  // move
		const Action &src = history_[history_.size() - 1];
		if (board_[action] == EMPTY) {
			if ((std::abs(action / kBoardSize - src / kBoardSize) <= 1) &&
				(std::abs(action % kBoardSize - src % kBoardSize) <= 1)){
				// return true;
				return is_moving_valid(src, action, player);
			}
		}
		return false;
	} else {  // new chess
		if(board_[action] == EMPTY) {
			// return true;
			Action src = history_[history_.size() - 2];
			Action dst = history_[history_.size() - 1];
			if (dst == empty_index) {
				return is_placing_valid(action, player);
			}

			std::vector<int> constrained_points = get_restrictions(src, dst, player);
			// if the moving action gets no restriction, or if placing action is a possible valid option
			if (constrained_points.empty() || std::find(constrained_points.begin(), constrained_points.end(), action) != constrained_points.end()) {
				return is_placing_valid(action, player);
			}
		}
	}
	return false;
}

bool SlitherState::have_move(const Action &action) const {
  for (int const &diff : MoveDirection) {
    int dest = action + diff;
    if ((dest >= 0 && dest < kNumOfGrids && board_[dest] == EMPTY) &&
        (std::abs(dest / kBoardSize - action / kBoardSize) <= 1) &&
        (std::abs(dest % kBoardSize - action % kBoardSize) <= 1)) {
      return true;
    }
  }
  return false;
}

bool SlitherState::have_win(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	const Player player = board_[action];
	std::stack<int> pieces;
	pieces.push(int(action));
	int traversed[kNumOfGrids] = {0}; //const
	traversed[action] = 1;
	int head = 0, tail = 0; 
	while(!pieces.empty()){
		int cur = pieces.top();
		pieces.pop();
		int u = cur + MoveDirection[1];
		int l = cur + MoveDirection[3];
		int r = cur + MoveDirection[4];
		int b = cur + MoveDirection[6];
		int top_check = 1, bottom_check = 1, left_check = 1, right_check = 1;
		if(cur / kBoardSize == 0) top_check = 0;
		if(cur / kBoardSize == kBoardSize - 1) bottom_check = 0;
		if(cur % kBoardSize == 0) left_check = 0;
		if(cur % kBoardSize == kBoardSize - 1) right_check = 0;
		if(top_check && traversed[u] == 0 && board_[u] == player){
			traversed[u] = 1;
			pieces.push(u);
		}
		if(left_check && traversed[l] == 0 && board_[l] == player){
			traversed[l] = 1;
			pieces.push(l);
		}
		if(right_check && traversed[r] == 0 && board_[r] == player){
			traversed[r] = 1;
			pieces.push(r);
		}
		if(bottom_check && traversed[b] == 0 && board_[b] == player){
			traversed[b] = 1;
			pieces.push(b);
		}
	}
	if(player == BLACK){
		for(int i = 0; i < kBoardSize; i++){
			if(traversed[i] == 1)
				head = 1;
			if(traversed[kBoardSize * (kBoardSize - 1) + i] == 1)
				tail = 1;
		}
	}
	else{
		for(int i = 0; i < kBoardSize; i++){
			if(traversed[i*kBoardSize] == 1)
				head = 1;
			if(traversed[kBoardSize * (i+1) - 1] == 1)
				tail = 1;
		}
	}
	if(head && tail){
		return true;
	}
	return false;
}
std::vector<int> SlitherState::get_restrictions(const Action src, const Action action, const Player player, std::array<short, kNumOfGrids>* bptr) const {
	// return empty vector if there is no restriction
	const std::vector<int> kValid = {};
	// empty_index is used to mark the existence of restriction
	std::vector<int> constrained_points = {empty_index};
	const std::vector<int>& kInvalid = constrained_points;

	if (src < 0 || src > kNumOfGrids) return kInvalid;
	if (action < 0 || action >= kNumOfGrids) return kInvalid;


	std::array<short, kNumOfGrids> mimic_board = (!bptr) ? board_ : *bptr;
	mimic_board[src] = EMPTY;
	mimic_board[action] = player;
	
	/*
	0x1001	0x0001	0x0011
	0x1000	 pos	0x0010
	0x1100	0x0100	0x0110
	*/

	auto CheckPostion = [=](Action pos, Action check) -> bool{ 
		if (check < 0 || check >= empty_index) return false;

		int rpos = pos / kBoardSize;
		int cpos = pos % kBoardSize;
		int rchk = check / kBoardSize;
		int cchk = check % kBoardSize;
		if (abs(rpos - rchk) > 1) return false;
		if (abs(cpos - cchk) > 1) return false;
		return true;
	};

	const int Cross[4] = {0x0001, 0x0010, 0x0100, 0x1000};
	const int Corner[4] = {0x1001, 0x0011, 0x0110, 0x1100};
	
	// check if dst is valid
	int ul = action + MoveDirection[0];
	int u = action + MoveDirection[1];
	int ur = action + MoveDirection[2];
	int l = action + MoveDirection[3];
	int r = action + MoveDirection[4];
	int bl = action + MoveDirection[5];
	int b = action + MoveDirection[6];
	int br = action + MoveDirection[7];
	const int PointsCrossDst[4] = {u, r, b, l};
	const int PointsCornerDst[4] = {ul, ur, br, bl};

	int place_cross = 0x0000;
	for(int i = 0; i < 4; i++){
		int chkpt = PointsCrossDst[i];
		if (CheckPostion(action, chkpt) == false) continue;
		if (mimic_board[chkpt] == player) {
			place_cross = place_cross | Cross[i];
		}
	}
	int restrictions = 0x1111;
	for (int i = 0; i < 4; i++){
		int chkpt = PointsCornerDst[i];
		if (CheckPostion(action, chkpt) == false) continue;
		if (mimic_board[chkpt] == player) {
			if (!(place_cross & Corner[i])){
				restrictions = restrictions & Corner[i];
			}
		}
	}

	// if there is no feasible restriction
	if (restrictions == false) return kInvalid;

	if (src == empty_index) {
		// placing action but have restrictions
		if (restrictions ^ 0x1111) return kInvalid;
		else return kValid;
	}

	std::vector<int> valid_points_dst;
	std::vector<int> valid_points_src;

	if (restrictions ^ 0x1111){
		for (int i = 0; i < 4; i++){
			if(restrictions & 0x1) {
				int pt = PointsCrossDst[i];
				valid_points_dst.push_back(pt);
			}
			restrictions = restrictions >> 4;
		}
	} 

	if (!valid_points_dst.empty()){
		valid_points_dst.erase(std::remove_if(
			valid_points_dst.begin(), valid_points_dst.end(), 
			[&](const Action& p) {
				return !is_placing_valid(p, player, &mimic_board);
			}), valid_points_dst.end());
		if (valid_points_dst.empty()) return kInvalid;
	}

	// check if src is valid
	ul = src + MoveDirection[0];
	u = src + MoveDirection[1];
	ur = src + MoveDirection[2];
	l = src + MoveDirection[3];
	r = src + MoveDirection[4];
	bl = src + MoveDirection[5];
	b = src + MoveDirection[6];
	br = src + MoveDirection[7];
	const int PointsCrossSrc[4] = {u, r, b, l};
	const int PointsCornerSrc[4] = {ul, ur, br, bl};

	place_cross = 0x0000;
	for(int i = 0; i < 4; i++){
		int chkpt = PointsCrossSrc[i];
		if (CheckPostion(src, chkpt) == false) continue;
		if(mimic_board[chkpt] == player){
			place_cross = place_cross | Cross[i];
		}
	}
	for(int i = 0; i < 4; i++){
		int chkpt = PointsCornerSrc[i];
		if (CheckPostion(src, chkpt) == false) continue;
		if (mimic_board[chkpt] != player){
			if(!(~(place_cross) & Corner[i])){
				valid_points_src.push_back(chkpt);
			}
		}
	}

	if (valid_points_src.size() == 1) {
		if (!is_placing_valid(valid_points_src[0], player, &mimic_board)) {
			valid_points_src.pop_back();
		}
		if (is_placing_valid(src, player, &mimic_board)) {
			valid_points_src.push_back(src);
		}
		if (valid_points_src.empty()) return kInvalid;
	// more than a corner must be placed
	} else if (valid_points_src.size() > 1) {
		if (is_placing_valid(src, player, &mimic_board)) {
			valid_points_src.clear();
			valid_points_src.push_back(src);
		} else {
			return kInvalid;
		}
	}

	if (valid_points_src.empty() && valid_points_dst.empty()) {
		return kValid;
	} else if (valid_points_dst.empty()) {
		constrained_points.insert(constrained_points.end(), valid_points_src.begin(), valid_points_src.end());
	} else if (valid_points_src.empty()) {
		constrained_points.insert(constrained_points.end(), valid_points_dst.begin(), valid_points_dst.end());
	} else {
		std::sort(valid_points_dst.begin(), valid_points_dst.end());
		std::sort(valid_points_src.begin(), valid_points_src.end());
		std::set_intersection(
			valid_points_dst.begin(), valid_points_dst.end(),
			valid_points_src.begin(), valid_points_src.end(),
			std::inserter(constrained_points, constrained_points.begin()));
	}

	constrained_points.erase(std::remove_if(
		constrained_points.begin(), constrained_points.end(), 
		[&](const Action& p) {
			if (p < 0 || p >= empty_index) return false;
			return (mimic_board[p] != EMPTY);
		}), constrained_points.end());

	return constrained_points;
}

bool SlitherState::is_selecting_valid(const Action action, const Player player) const {
	auto CheckPostion = [=](Action pos, Action check) -> bool{ 
		if (check < 0 || check >= empty_index) return false;

		int rpos = pos / kBoardSize;
		int cpos = pos % kBoardSize;
		int rchk = check / kBoardSize;
		int cchk = check % kBoardSize;
		if (abs(rpos - rchk) > 1) return false;
		if (abs(cpos - cchk) > 1) return false;
		return true;
	};

	if (action == kNumOfGrids) {
		return is_placing_valid(player);
	} else {
		for (auto &dir: MoveDirection) {
			const Action pt_surrounded = action + dir;
			if (CheckPostion(action, pt_surrounded) == false) continue;
			if (board_[pt_surrounded] != EMPTY) continue;

			std::vector<int> constrained_points = get_restrictions(action, pt_surrounded, player);
			std::array<short, kNumOfGrids> mimic_board = board_;
			mimic_board[action] = EMPTY;
			mimic_board[pt_surrounded] = player;
			if (constrained_points.empty()) {
				if(is_placing_valid(player, &mimic_board)) return true;
				else continue;
			} else if (constrained_points.size() != 1) {
				for (auto pt: constrained_points) {
					if (is_placing_valid(pt, player, &mimic_board)) return true;
				}
			}
		}
	}

	return false;
}

bool SlitherState::is_placing_valid(const Action action, const Player player, std::array<short, kNumOfGrids>* bptr) const {
	return get_restrictions(empty_index, action, player, bptr).size() != 1;
}

bool SlitherState::is_placing_valid(const Player player, std::array<short, kNumOfGrids>* bptr) const {
	std::array<short, kNumOfGrids> mimic_board = (!bptr) ? board_ : *bptr;
	for (int i = 0; i < kNumOfGrids; ++i) {
		if (mimic_board[i] == EMPTY) {
			if (is_placing_valid(i, player, &mimic_board)) return true;
		}
	}
	return false;
}

bool SlitherState::is_moving_valid(const Action src, const Action action, const Player player, std::array<short, kNumOfGrids>* bptr) const {
	std::vector<int> constrained_points = get_restrictions(src, action, player, bptr);
	std::array<short, kNumOfGrids> mimic_board = board_;
	mimic_board[src] = EMPTY;
	mimic_board[action] = player;
	if (constrained_points.empty()) {
		return is_placing_valid(player, &mimic_board);
	}
	return get_restrictions(src, action, player, bptr).size() != 1;
}

bool SlitherState::is_valid(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	const Player player = board_[action];
	int top_check = 1, bottom_check = 1, left_check = 1, right_check = 1;
	if(action / kBoardSize == 0) top_check = 0;
	if(action / kBoardSize == kBoardSize - 1) bottom_check = 0;
	if(action % kBoardSize == 0) left_check = 0;
	if(action % kBoardSize == kBoardSize - 1) right_check = 0;
	int ul = action + MoveDirection[0];
	int u = action + MoveDirection[1];
	int ur = action + MoveDirection[2];
	int l = action + MoveDirection[3];
	int r = action + MoveDirection[4];
	int bl = action + MoveDirection[5];
	int b = action + MoveDirection[6];
	int br = action + MoveDirection[7];
	// upper left check
	if(top_check && left_check && board_[ul] == player && board_[u] != player && board_[l] != player)
		return false;
	// upper right check
	if(top_check && right_check && board_[ur] == player && board_[u] != player && board_[r] != player)
		return false;
	// lower left check
	if(bottom_check && left_check && board_[bl] == player && board_[b] != player && board_[l] != player)
		return false;
	// lower right check
	if(bottom_check && right_check && board_[br] == player && board_[b] != player && board_[r] != player)
		return false;
	return true;
}

bool SlitherState::is_empty_valid(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	if(board_[action] != EMPTY) return true;
	int top_check = 1, bottom_check = 1, left_check = 1, right_check = 1;
	if(action / kBoardSize == 0) top_check = 0;
	if(action / kBoardSize == kBoardSize - 1) bottom_check = 0;
	if(action % kBoardSize == 0) left_check = 0;
	if(action % kBoardSize == kBoardSize - 1) right_check = 0;
	int ul = action + MoveDirection[0];
	int u = action + MoveDirection[1];
	int ur = action + MoveDirection[2];
	int l = action + MoveDirection[3];
	int r = action + MoveDirection[4];
	int bl = action + MoveDirection[5];
	int b = action + MoveDirection[6];
	int br = action + MoveDirection[7];
	// upper left check
	if(top_check && left_check && board_[u] != EMPTY && board_[u] == board_[l] && board_[u] != board_[ul])
		return false;
	// upper right check
	if(top_check && right_check && board_[u] != EMPTY && board_[u] == board_[r] && board_[u] != board_[ur])
		return false;
	// lower left check
	if(bottom_check && left_check && board_[b] != EMPTY && board_[b] == board_[l] && board_[b] != board_[bl])
		return false;
	// lower right check
	if(bottom_check && right_check && board_[b] != EMPTY && board_[b] == board_[r] && board_[b] != board_[br])
		return false;
	return true;
}

std::string SlitherState::to_string() const {
  std::stringstream ss;
//  const std::vector<std::string> chess{"●", "o", "·"};
  const std::vector<std::string> chess{"x", "o", "·"};

  for (int i = 0; i < kBoardSize; ++i) {
    ss << std::setw(2) << std::setfill(' ') << kBoardSize - i;
    for (int j = 0; j < kBoardSize; ++j) {
        ss << ' ' << chess[board_[i * kBoardSize + j]];
    }
    ss << std::endl;
  }
  ss << "  ";
  for (int i = 0; i < kBoardSize; ++i) ss << ' ' << static_cast<char>('A' + i);
  return ss.str();
}

bool SlitherState::is_terminal() const {
  //return (winner_ != -1) || (legal_actions().empty());
  return (winner_ != -1);
}

Player SlitherState::current_player() const {
  return (Player)((turn_ % 6) / 3);
}

Player SlitherState::get_winner() const {
  return winner_;
}

std::vector<float> SlitherState::observation_tensor() const {
  std::vector<float> tensor;
  tensor.reserve(4 * kNumOfGrids); //8 * 6 * 6
  Player player = current_player();
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == player));
  }
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 1 - player));
  }
  /*
  00: turn % 3 == 0
  01: turn % 3 == 1
  10: turn % 3 == 2
  11: unused
  */
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>((turn_ % 3 == 2)));
  }
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>((turn_ % 3 == 1)));
  }
  return tensor;
}

std::vector<float> SlitherState::returns() const {
  if (winner_ == BLACK) {
    return {1.0, -1.0};
  }
  if (winner_ == WHITE) {
    return {-1.0, 1.0};
  }
  return {0.0, 0.0};
}

std::string SlitherState::serialize() const {
  std::stringstream ss;
  for (const Action action : history_) {
    ss << action << " ";
  }
  return ss.str();
}

std::string SlitherState::serialize_num_to_char() const {
	std::string num_to_char = serialize();

  	std::stringstream ss1;
    ss1<<num_to_char;
    std::string tmp;
    std::string res="";

	res+="SZ[";
	res+=std::to_string(kBoardSize);
	res+="]";
	
    bool color = 1;
    int counter = 3;
    while(ss1>>tmp) {
	  res+=";";
      int num = stoi(tmp);
	  
      int row = num/kBoardSize;
      int col = num%kBoardSize;
      if(color) {
          res+="B[";
      }else {
          res+="W[";
      }
	  if(num==kBoardSize*kBoardSize) {
		res+="X]";
		counter--;
		if(counter==0) {
			counter=3;
			color = !color;
		}
		continue;
	  }
      res+=char(col+'A');

      res+=std::to_string(kBoardSize-(row));
    //   res+=char(row+'1');
      counter--;
      if(counter==0) {
		counter=3;
        color = !color;
      }
      res+="]";
    }
	return res;
}
// StatePtr SlitherGame::get_pre_state() {
//   return pre_state;
// }

// void SlitherGame::save_state(){
// 	pre_state = std::make_unique<SlitherState>(*this);
// }

std::string SlitherGame::name() const {
  return "slither";
}
int SlitherGame::num_players() const { return 2; }
int SlitherGame::num_distinct_actions() const {
  return kPolicyDim;
}
std::vector<int> SlitherGame::observation_tensor_shape() const {
  return {4, kBoardSize, kBoardSize};
}

StatePtr SlitherGame::new_initial_state() const {
  return std::make_unique<SlitherState>(shared_from_this());
}

int SlitherGame::num_transformations() const { return 4; }

std::vector<float> SlitherGame::transform_observation(
    const std::vector<float> &observation, int type) const {
  std::vector<float> data(observation);
  if (type != 0) {
    int feature_num = (int)observation.size() / kNumOfGrids;
    for (int i = 0; i < feature_num; i++) {
      for (int index = 0; index < kNumOfGrids; index++) {
        int new_index = transform_index(index, type);
        data[i * kNumOfGrids + new_index] =
            observation[i * kNumOfGrids + index];
      }
    }
  }
  return data;
}

std::vector<float> SlitherGame::transform_policy(
    const std::vector<float> &policy, int type) const {
  std::vector<float> data(policy);
  if (type != 0) {
    for (int index = 0; index < kNumOfGrids; index++) {
      int new_index = transform_index(index, type);
      data[new_index] = policy[index];
    }
  }
  return data;
}

std::vector<float> SlitherGame::restore_policy(
    const std::vector<float> &policy, int type) const {
  std::vector<float> data(policy);
  if (type != 0) {
    for (int index = 0; index < kNumOfGrids; index++) {
      int new_index = transform_index(index, type);
      data[new_index] = policy[index];
    }
  }
  return data;
}

int SlitherGame::transform_index(const int &index, const int type) const {
  if (type == 0) {
    return index;
  } else {
    int i = index / kBoardSize;
    int j = index % kBoardSize;
    if (type == 1) { // reflect horizontal
      j = (kBoardSize - 1) - j;
    }
    else if (type == 2) { // reflect vertical
      i = (kBoardSize - 1) - i;
    }
    else if (type == 3) { // reverse
      i = (kBoardSize - 1) - i;
      j = (kBoardSize - 1) - j;
    }
    return i * kBoardSize + j;
  }
}



std::string SlitherGame::action_to_string(
    const Action &action) const {
  using namespace std::string_literals;
  std::stringstream ss;
  if(action == empty_index)
	  ss << "X";
  else
	ss << "ABCDEFGHIJKLM"s.at(action % kBoardSize) << kBoardSize - (action / kBoardSize);
  return ss.str();
}

std::vector<Action> SlitherGame::string_to_action(
    const std::string &str) const {
  std::string wopunc = str;
  wopunc.erase(std::remove_if(wopunc.begin(), wopunc.end(), ispunct),
               wopunc.end());
  std::stringstream ss(wopunc);
  std::string action;
  std::vector<Action> id;
  while (ss >> action) {
	if (action.at(0) == 'X' || action.at(0) == 'x') {
	  id.push_back(empty_index);
    } else if (action.at(0) >= 'a' && action.at(0) <= 'z') {
      int tmp = (kBoardSize - std::stoi(action.substr(1))) * kBoardSize +
                (action.at(0) - 'a');
      id.push_back(tmp);
    } else {
      int tmp = (kBoardSize - std::stoi(action.substr(1))) * kBoardSize +
                (action.at(0) - 'A');
      id.push_back(tmp);
    }
  }
  return id;
}

StatePtr SlitherGame::deserialize_state(
    const std::string &str) const {
  std::stringstream ss(str);
  int action;
  StatePtr state = new_initial_state();
  while (ss >> action) {
    state->apply_action(action);
  }
  return state->clone();
}

}  // namespace clap::game::slither