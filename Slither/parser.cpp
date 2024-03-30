#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <ctime>
using namespace std;

enum class Player {
    kPlayerNone = 0,
    kPlayer1 = 1,
    kPlayer2 = 2,
    kPlayerSize = 3
};

std::string getPlayerType(Player p)
{
	switch (p) {
		case Player::kPlayerNone: return "N";
		case Player::kPlayer1: return "B";
		case Player::kPlayer2: return "W";
		default: return "?";		
	}
	
	return "?";
}

class Action {
public:	
    Action() : action_id_(-1), player_(Player::kPlayerNone) {}
    Action(int action_id, Player player) : action_id_(action_id), player_(player) {}

    inline int getActionID() const { return action_id_; }
    inline Player getPlayer() const { return player_; }

protected:
    int action_id_;
    Player player_;
};

class TwoLayersNode {
public:
	std::string action;
	std::string player;
	std::vector<TwoLayersNode*> children;
	std::string comment;
};

class SGFTreeLoader{
public:
    class SGFTreeNode {
    public:
        Action action_;
		SGFTreeNode* parent_;
        SGFTreeNode* child_;
        SGFTreeNode* next_silbing_;
        std::string comment_;

        SGFTreeNode() { reset(); }
        void reset()
        {
            action_ = Action();
			parent_ = nullptr;
            child_ = nullptr;
            next_silbing_ = nullptr;
            comment_.clear();
        }
		
		std::string getComment() const { return comment_; }
    };

public:
    SGFTreeLoader()
    {
        sgf_tree_nodes_.resize(1);
        reset();
    }

    void setTreeSize(int tree_size)
    {
        sgf_tree_nodes_.clear();
        sgf_tree_nodes_.resize(tree_size);
    }

    bool loadFromFile(const std::string& file_name)
    {
        std::ifstream fin(file_name.c_str());
        if (!fin) { return false; }

        std::string line;
        std::string sgf_content;
        while (std::getline(fin, line)) {
            if (line.back() == '\r') { line.pop_back(); }
            if (line.empty()) { continue; }
            sgf_content += line;
        }
        
        return loadFromString(sgf_content);        
    }

    bool loadFromString(const std::string& sgf_content)
    {
        reset();
        sgf_content_ = trimSpace(sgf_content);		
        SGFTreeNode* root = getNextNode();
        return (parseSGFTree(sgf_content, 0, root) != std::string::npos);
    }

    size_t parseSGFTree(const std::string& sgf_content, int start_pos, SGFTreeNode* node)
    {
        SGFTreeNode* root = node;
        SGFTreeNode* rightmost_child = nullptr;
        size_t index = start_pos + 1;
		bool bFirstSemi = true;
        while (index < sgf_content_.size() && sgf_content_[index] != ')') {
            if (sgf_content[index] == '(') {				
                SGFTreeNode* next_node = getNextNode();
                if (root->child_ == nullptr) {
                    root->child_ = next_node;
                    rightmost_child = next_node;
                } else {
                    rightmost_child->next_silbing_ = next_node;
                    rightmost_child = next_node;
                }
                index = parseSGFTree(sgf_content, index, next_node) + 1;
            } else {
				if (sgf_content[index] == ';') { 
					if (bFirstSemi) { bFirstSemi = false; }
					else { 
						root->child_ = getNextNode();
						root = root->child_; 
					}
					++index; 
				}
				std::pair<std::string, std::string> key_value;
				if ((index = calculateKeyValuePair(sgf_content_, index, key_value)) == std::string::npos) { return std::string::npos; }								
                if (key_value.first == "B" || key_value.first == "W") {
                    int board_size = tags_.count("SZ") ? std::stoi(tags_["SZ"]) : -1;
                    if (board_size == -1) { return std::string::npos; }
                    Player player = key_value.first == "B" ? Player::kPlayer1 : Player::kPlayer2;
                    root->action_ = Action(sgfStringToActionID(key_value.second, board_size), player);					
				} else if (key_value.first == "C") {
					root->comment_ = key_value.second;					
				} else {
					if (key_value.first == "SZ") { 
						board_size_ = std::stoi(key_value.second); 
						pass_location_ = board_size_ * board_size_;
					}
					tags_[key_value.first] = key_value.second;
				}				
			}
        }
        return index;
    }

	std::string outputTree() const
	{
		std::ostringstream oss;
		oss << "(;GM[CLAP]SZ[5]";
		oss << outputTree_r(getRoot());
		oss << ")";
		return oss.str();
	}

	std::string outputTree_r(const SGFTreeNode* node) const
	{
		if (node == NULL) { return ""; }
				
		std::ostringstream oss;
		if (node != getRoot()) {
			oss << ";" << getPlayerType(node->action_.getPlayer()) << "[" << actionIDToSGFString(node->action_.getActionID(),board_size_) << "]";
			if (node->comment_ != "") oss << "C[" << node->comment_ << "]";
		}
				
		int num_children = 0; // 這邊是算有幾個兄弟姊妹(分枝)嗎?
		for (SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
			++num_children;
		}	
		
		for (SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
			if (num_children > 1) { oss << "("; }
			oss << outputTree_r(child);
			if (num_children > 1) { oss << ")"; }
		}					
		
		return oss.str();
	}

	// --------------------------------------------------------------------------
	void parseEditorTree(TwoLayersNode* root) // two layer
	{
		parseEditorTree_r(getRoot(), root, 0);
	}

 	void parseEditorTree_r(const SGFTreeNode* node, TwoLayersNode* new_node_parent, int cnt_level) // two layer
	{		
		if (node == NULL) { return; }
			
		if (node != getRoot()) {
			if (cnt_level % 3 == 1) {
				std::string action = actionIDToSGFString(node->action_.getActionID(),board_size_);
				if (action != "") {
					for(SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
						TwoLayersNode* new_node = new TwoLayersNode;
						new_node->action = action;
						new_node->player = getPlayerType(node->action_.getPlayer());

						new_node_parent->children.push_back(new_node);
						parseEditorTree_r(child, new_node_parent, cnt_level+1);
					}
				}
				else {
					for(SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
						parseEditorTree_r(child, new_node_parent, cnt_level+1);
					}
				}
			}
			else if (cnt_level % 3 == 2) {
				std::string action = actionIDToSGFString(node->action_.getActionID(),board_size_);
				if (action != "") {
					TwoLayersNode* new_node = new_node_parent->children.back();
					new_node->action += action;
					new_node->comment = node->getComment();

					for(SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
						parseEditorTree_r(child, new_node, cnt_level+1);
					}
				}
				else {
					for(SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
						parseEditorTree_r(child, new_node_parent, cnt_level+1);
					}
				}
			}
			else {
				std::string action = actionIDToSGFString(node->action_.getActionID(),board_size_);
				TwoLayersNode* new_node = new TwoLayersNode;
				new_node->action = action;
				new_node->player = getPlayerType(node->action_.getPlayer());
				new_node->comment = node->getComment();
				new_node_parent->children.push_back(new_node);

				for(SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
					parseEditorTree_r(child, new_node, cnt_level+1);
				}
			}
		}
		else {
			for(SGFTreeNode* child = node->child_; child != NULL; child = child->next_silbing_) {
				parseEditorTree_r(child, new_node_parent, cnt_level+1);
			}
		}
	}

	std::string outputEditorTree(TwoLayersNode* root) // two layer
	{
		std::ostringstream oss;
		oss << "(;GM[511]SZ["<<board_size_<<"]";
		oss << outputEditorTree_r(root, 0);
		oss << ")";
		return oss.str();
	}

 	std::string outputEditorTree_r(const TwoLayersNode* node, int cnt_level) // two layer
	{		
		if (node == NULL) { return ""; }
		
		std::ostringstream oss;
		if (cnt_level != 0) {			
			oss << ";" << node->player 
				<< "[" << node->action << "]";
			oss << "C[" << node->comment << "]";
		}
		
		int num_children = node->children.size();
		
		for (auto child = node->children.begin(); child != node->children.end(); child++) {
			if (num_children > 1) { oss << "("; }
			oss << outputEditorTree_r(*child, cnt_level+1);
			if (num_children > 1) { oss << ")"; }
		}
		
		return oss.str();
	}
	// --------------------------------------------------------------------------
 

	inline const SGFTreeNode* getRoot() const { return &sgf_tree_nodes_[0]; }	
    inline SGFTreeNode* getNextNode() { return &sgf_tree_nodes_[current_node_id_++]; }
    inline void resetCurrentNode() { current_node_ = &sgf_tree_nodes_[0]; }
    //inline std::string getComment() { return (current_node_ ? current_node_->comment_ : "null"); }	
	inline int getTreeSize() { return current_node_id_; }

	size_t calculateKeyValuePair(const std::string& s, size_t start_pos, std::pair<std::string, std::string>& key_value)
	{
		key_value = {"", ""};
		bool is_key = true;
		for (; start_pos < s.size(); ++start_pos) {
			if (is_key) {
				if (isalpha(s[start_pos])) {
					key_value.first += s[start_pos];
				} else if (s[start_pos] == '[') {
					if (key_value.first == "") { return std::string::npos; }
					is_key = false;
				}
			} else {
				if (s[start_pos] == ']') {
					if (s[start_pos - 1] == '\\') {
						key_value.second += s[start_pos];
					} else {
						return ++start_pos;
					}
				} else if (s[start_pos] != '\\') {
					key_value.second += s[start_pos];
				}
			}
		}
		return std::string::npos;
	}

	std::string trimSpace(const std::string& s) const
	{
		bool skip = false;
		std::string new_s;
		for (const auto& c : s) {
			skip = (c == '[' ? true : (c == ']' ? false : skip));
			if (skip || c != ' ') { new_s += c; }
		}
		return new_s;
	}

	int sgfStringToActionID(const std::string& sgf_string, int board_size)
	{
		if (sgf_string.size() < 2) { return board_size * board_size; }
		int x = std::toupper(sgf_string[0]) - 'A';
		std::string number = sgf_string;
		number.erase(number.begin());
		std::stringstream ss;
		ss<<number;
		int stringToInt;
		ss>>stringToInt;
		int y = stringToInt-1;
		return y * board_size + x;
	}
	
	std::string actionIDToSGFString(int action_id, int board_size) const
	{
		if (action_id == board_size * board_size) { return ""; }
		int x = action_id % board_size;
		int y = action_id / board_size;
		std::ostringstream oss;
		oss << (char)(x + 'a') << (char)(y + 'a');
		return oss.str();
	}	

private:
    void reset()
    {
		node_ = 0;
		file_name_ = "";
		sgf_content_ = "";
		tags_.clear();
		actions_.clear();
        current_node_id_ = 0;
        resetCurrentNode();		
    }

	int node_;
	std::string file_name_;
	std::string sgf_content_;
    int current_node_id_;
    SGFTreeNode* current_node_;
    std::vector<SGFTreeNode> sgf_tree_nodes_;
	std::unordered_map<std::string, std::string> tags_;
	std::vector<std::pair<std::vector<std::string>, std::string>> actions_;	
	int board_size_;
	int pass_location_;
};

int main(int argc, char* argv[])
{
	SGFTreeLoader loader;
	loader.setTreeSize(1000000);	
	loader.loadFromFile(argv[1]);
	std::cout << "Number of Tree Node: " << loader.getTreeSize() << std::endl;
	
// 	fstream f_output;
// 	f_output.open("tree.sgf", ios::out);
// 	f_output << loader.outputTree();
//    f_output.close();

	TwoLayersNode* root = new TwoLayersNode;
	loader.parseEditorTree(root);
 
 	fstream f_output2;

	// const game::GamePtr& game = engine->game;
	// std::ofstream sgf_file_;
	std::time_t now = std::time(0);
	std::tm* ltm = localtime(&now);
	std::stringstream dt;
	std::string folder="mcts_sgf/";
	system(("mkdir -p "+folder).c_str());
	dt << 1900 + ltm->tm_year << 'Y' << 1 + ltm->tm_mon << 'M' << ltm->tm_mday
		<< 'D' << ltm->tm_hour << ':' << ltm->tm_min << ':' << ltm->tm_sec;

	f_output2.open(std::string(argv[1]), ios::out);
	f_output2 << loader.outputEditorTree(root);
	f_output2.close();
	// f_output2.open("editor.sgf", ios::out);
	// f_output2 << loader.outputEditorTree();
	// f_output2.close();
 
	return 0;
}