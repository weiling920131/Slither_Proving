import argparse
import sys
import time
import os
import re
import atexit
from pathlib import Path
import tempfile
# modified start
import random
from itertools import product
# modified end

import clap.game
import clap.mcts
import torch
from clap.pb import clap_pb2

class CLI_agent:
    def __init__(self, args):
        self.engine = clap.mcts.VLEngine(args.gpus)

        self.engine.batch_size = args.batch_size
        self.engine.max_simulations = args.simulation_count
        self.engine.c_puct = args.c_puct
        self.engine.dirichlet_alpha = 0
        self.engine.dirichlet_epsilon = 0
        self.engine.temperature_drop = 0

        self.engine.virtual_loss = 3
        self.engine.num_sampled_transformations = args.transform
        self.engine.batch_per_job = 1 if self.engine.num_sampled_transformations == 0 else self.engine.num_sampled_transformations

        self.engine.play_until_terminal = False
        self.engine.auto_reset_job = False
        self.engine.play_until_turn_player = True
        self.engine.mcts_until_turn_player = True
        self.engine.verbose = args.verbose
        self.engine.top_n_childs = args.top
        self.engine.states_value_using_childs = True

        self.game = clap.game.load(args.game)
        self.state = self.game.new_initial_state()

        self.load_game(args.game)
        self.load_model(args.model)

        self.engine.start(args.cpu_workers, args.gpu_workers, 0)
        self.num_jobs = args.cpu_workers

        self.save_result = args.save

        # modified start
        self.automode = False if args.mode!='auto' else True
        # if not self.automode :
            # atexit.register(self.saveAtExit, 'autosave.sgf') # will execute after code terminate
        # modified end

    def saveAtExit(self, file_name='save_at_exit.sgf'):
        self.save_manual(file_name)

    def load_game(self, game):
        self.engine.load_game(game)

    def load_model(self, model_path):
        self.engine.load_model(model_path)

    def showboard(self, file=sys.stdout):
        print("=", file=file)
        print(self.state, file=file)
        print(file=file)

    def printBoard(self, board=[]):
        path = self.state.match_WP()
        CPs = self.state.get_critical(path)
        print(CPs)
        print("=")
        print(self.state.printBoard(board, CPs))
        print()

    def clear(self):
        self.state = self.game.new_initial_state()

    def genmove(self):
        # print("serialize:", self.state.serialize())
        # print("+++++\n+++++\n++++\n+++++1")
        self.engine.add_job(self.num_jobs, self.state.serialize())
        # print("+++++\n+++++\n++++\n+++++2")
        raw = self.engine.get_trajectory()
        # print("+++++\n+++++\n++++\n+++++3")
        trajectory = clap_pb2.Trajectory.FromString(raw)
        # print("+++++\n+++++\n++++\n+++++4")
        actions = []
        for state in trajectory.states:
            print("value:", state.evaluation.value)
            # print("policy:", state.evaluation.policy)
            actions.append(state.transition.action)
        # print("+++++\n+++++\n++++\n+++++5")
        return actions

    def play(self, actions):
        for action in actions:
            if action not in self.state.legal_actions():
                print('=====> illegal move:', action, ', legal_actions: ',self.state.legal_actions(),';' )
                print("Illegal Action")
            else:
                print('=====> legal move:', action, ', legal_actions: ',self.state.legal_actions(),';')
                self.state.apply_action(action)
    
    def play_manual(self, input_string: str):
        player = int(input_string[input_string.find("play_manual") + len("play_manual") + 1])

        substr = input_string[input_string.find("play_manual") + len("play_manual 0"):]
        actions = self.game.string_to_action(substr)

        for action in actions:
            self.state.manual_action(action, player)

        actions_string  = [self.game.action_to_string(action) for action in actions]
        # print("=" + ", ".join(actions_string))
        self.history.append(actions_string)        

    def playmove(self, actions):
        move_count = 0
        for action in actions:
            if action not in self.state.legal_actions():
                print("Illegal Action")
                move_count += 1
                break
            else:
                print("Legal Action")
                move_count += 1
                self.state.apply_action(action)
        return move_count

    def save(self, file_name):
        import yaml
        with open(file_name, "w") as output:
            yaml.dump(self.history, output)

    def save_manual(self, file_name="manual.sgf"):
        if not self.automode:
            print(self.history)
        posInt2Char=lambda p: chr(int(p)-1+ord('A'))
        rlt="(;GM[511]"
        color = 0
        for h in self.history:
            try:
                mov1, mov2, place = h
            except:
                print('Manual Error:', h)
                continue
            # print(f"{mov1}, {mov2}, {place}")
            if place != "X":
                if mov2 != "X":
                    rlt += ";"
                    rlt += "W" if color else "B"
                    rlt += f"[{mov1[0]}{posInt2Char(mov1[1:])}{mov2[0]}{posInt2Char(mov2[1:])}]"
                rlt += ";"
                rlt += "W" if color else "B"
                rlt += f"[{place[0]}{posInt2Char(place[1:])}]"
                color = not color
        rlt+=")"

        # modified start
        if self.automode:
            files = [f for f in os.listdir('./automode_save') if os.path.isfile(os.path.join('./automode_save', f))]
            with open('./automode_save/result_'+str(len(files)+1)+".sgf", "w") as output:
                output.write(rlt)
        # modified end
        else:
            with open(file_name, "w") as output:
                output.write(rlt)
            # print(f"Saved manual as {file_name}")
            
    def load_manual(self, manual_path: Path, game_type='slither'):
        # Slither avalible only

        if len(self.history) != 0:
            print('[Error] Loading is only possible at the beginning')
            return
            
        try:
            with open(manual_path, 'r') as f:
                manual = f.read()
        except FileNotFoundError as e:
            print(e)
            return

        if game_type != 'slither':
            return NotImplementedError

        manual = manual[manual.find('('):manual.find(')')]
        manual = manual.split(';')
        # print(manual)
        all_actions = []
        actions = []
        for move in manual:
            move = move.upper()
            if move.startswith('B[') or move.startswith('W['):
                player = move[0]
                action = move[2:-1]
                pos_transform = lambda pos: pos[0] + str((ord(pos[1]) - ord('A')) + 1)
                if action == 'swap'.upper():
                    continue
                if len(action) == 2:
                    if len(actions) == 0:
                        actions.append('X') 
                        actions.append('X') 
                    actions.append(pos_transform(action[0:2]))
                elif len(action) == 4:
                    actions.append(pos_transform(action[0:2]))
                    actions.append(pos_transform(action[2:4]))
                elif len(action) == 6:
                    actions.append(pos_transform(action[0:2]))
                    actions.append(pos_transform(action[2:4]))
                    actions.append(pos_transform(action[4:6]))
            
            if len(actions) == 3:
                all_actions.append(actions)
                actions = []

        for actions in all_actions:
            cmd = f"play {' '.join(actions)}"
            self.play_game(cmd)
        
    def play_game(self, input_string: str):
        substr = input_string[input_string.find("play") + len("play"):]
        actions = self.game.string_to_action(substr)
        self.play(actions)

        actions_string  = [self.game.action_to_string(action) for action in actions]
        print("=" + ", ".join(actions_string))
        self.history.append(actions_string)

    # 11/7 modified

    # def match_WP(self):
    #     board = self.state.getboard()
    #     black_num = board.count(0)
    #     folder = './winning_path/'
    #     pathes = []
    #     for i in range(5, black_num + 2):
    #         file = open(folder+str(i)+'.txt', 'r')
    #         lines = file.readlines()
    #         for line in lines:
    #             s = list()
    #             black_cnt = 0
    #             for point in line.strip().split():
    #                 if board[int(point)] != 0:
    #                     s.append(int(point))
    #                 else:
    #                     black_cnt += 1
    #             if black_cnt > i-3:
    #                 pathes.append(s)

    #     return pathes

    # def get_critical(self, pathes):
    #     pathes_1 = [ i for i in pathes if len(i) == 1]
    #     pathes_2 = [ i for i in pathes if len(i) == 2]
    #     all_critical = []
    #     all_pathes = []
    #     if len(pathes_2) > 0:
    #         for s in list(product(*pathes_2)):
    #             all_critical.append(list(set(s)))   # 組內

    #         for i in range(len(all_critical)):
    #             for j in pathes_1:
    #                 if j[0] not in all_critical[i]:
    #                     all_critical[i].append(j[0])
                        
    #         max_length = max(len(lst) for lst in all_critical)
    #         pathes_n = []
    #         for i in range(1, max_length+1):
    #             pathes_n.append({ tuple(j) for j in all_critical if len(j) == i})

    #         for i in range(0, max_length-1):
    #             for j in range(i+1, max_length):
    #                 for setb in pathes_n[i]:
    #                     for seta in pathes_n[j].copy():
    #                         if set(setb).issubset(set(seta)):
    #                             pathes_n[j].remove(seta)

    #         for path_n in pathes_n:
    #             if len(path_n) > 0:
    #                 for p in path_n:
    #                     all_pathes.append(list(p))

    #     else:
    #         for i in pathes_1:
    #             all_pathes.append(i)

    #     return all_pathes
            

    def test_action(self, input_string: str):
        player = int(input_string[input_string.find("test_action") + len("test_action") + 1])
        path = []
        pathes = []
        # pathes = self.state.test_action(path, pathes, player)
        # return pathes
        return self.state.test_action_bool(path, pathes, player)

    # whp
    def test_generate(self, input_string: str):
        print("begin")
        # chess_num = int(input_string[input_string.find("test_generate") + len("test_generate") + 1])
        for i in range(6, 10):
            self.state.test_generate([], i, 0)
        # self.state.generate_WP()
        print("done")

        
    def test_prune(self):
        posInt2Char=lambda p: chr(int(p)-1+ord('A'))


        for i in range(4, 7):
            output = open(f"./test_prune/result_{i}.txt", "w")
            file = open('./checkmate/'+'checkmate_'+str(i)+'.txt')
            lines = file.readlines()
            cnt = 1
            white_noBlock = 0
            white_all = 0

            print(i)

            # output.write("(;GM[511]")

            for line in lines:
                # if cnt >= 210:
                #     break
                self.clear()
                self.history = []
                for point in line.strip().split():
                    self.play_manual('play_manual 0 X X '+self.game.action_to_string(int(point)))
                
                CPs = self.state.get_critical(self.state.match_WP())

                # rlt = "("
                board = self.state.getboard()
                # for b in range(len(board)):
                #     if board[b] == 0:   # BLACK
                #         piece = self.game.action_to_string(b)
                #         rlt += ";B"
                #         rlt += f"[{piece[0]}{posInt2Char(piece[1])}]"

                # rlt += ";C["
                # for CP in CPs:
                #     rlt += "{"
                #     for c in range(len(CP)):
                #         if c != 0:
                #             rlt += " "
                #         rlt += self.game.action_to_string(CP[c])
                #     rlt += "} "
                # rlt += "]"

                # output.write(rlt)

                black_num = board.count(0)
                all_num = 0
                all_num = self.state.DFS_noBlock(board, black_num, 0, black_num, CPs, all_num)
                boards = self.state.get_noBlock()
                # cnt = 0
                for b in boards:
                    # rlt = "("
                    # for w in range(len(b)):
                    #     if b[w] == 1:  # WHITE
                    #         piece = self.game.action_to_string(w)
                    #         rlt += ";W"
                    #         rlt += f"[{piece[0]}{posInt2Char(piece[1])}]"
                    # print(cnt)
                    # cnt += 1
                    if not self.state.test_board(b):
                        # rlt += ";C[black not win]"
                        print("black not win")
                        print(self.state.printBoard(b, []))
                        output.write(self.state.printBoard(b, []))
                        break
                    # else:
                    #     rlt += ";C[black win]"
                    # rlt += ")"
                    # output.write(rlt)

                # output.write(")")
                print(f"{cnt}: {len(boards)} / {all_num} = {len(boards) / all_num}") 
                cnt += 1
                white_noBlock += len(boards)
                white_all += all_num
            
            # output.write(")")

            # print(f"Saved as test_prune/{i}.sgf")
            print(f"Avg: {white_noBlock} / {white_all} = {white_noBlock / white_all}")
            output.write(f"{i}: {white_noBlock} / {white_all} = {white_noBlock / white_all}")
            file.close()
            output.close()

    def test_white_blockable(self):
        if(self.state.check_can_block()):
            print("can block")
        else :
            print("can't block")

    def loop(self):
        cnt = 0
        t = time.localtime()
        result = time.strftime("%m_%d_%Y_%H:%M:%S", t)
        self.clear()
        self.history = []
        while not self.state.is_terminal():
            # modified start
            self.showboard()
            cnt += 1
            if not self.automode:
                string = input()
            else:
                if cnt == 2:
                    randc = random.randint(0, 4)
                    randn = random.randint(1, 5)
                    string = "play X X "+chr(randc+ord('A'))+str(randn) if chr(randc+ord('A'))+str(randn) != "C3" else "play X X "+chr(randc+ord('A'))+str(randn-1)
                else:
                    string = "gen"
            # modified end

            start = time.time()

            if "showboard" in string or "sb" in string:
                self.showboard()

            elif "showcritical" in string or "sc" in string:
                self.printBoard()

            elif "tb" in string:
                b = [0, 0, 0, 0, 1, 1, 0, 2, 2, 2, 1, 0, 1, 0, 0, 2, 2, 2, 2, 1, 1, 1, 0, 1, 1]
                print(self.state.test_board(b))
                
            elif "clear" in string or 'reset' in string:
                self.clear()
                self.history = []

            elif "test_generate" in string:
                self.test_generate(string)
                break

            elif "genmove" in string or "gen" in string:
                if len(string.split()) > 1:
                    self.engine.max_simulations = int(string.split()[-1])
                actions = self.genmove()
                self.play(actions)

                actions_string = [self.game.action_to_string(action) for action in actions]
                print("=" + ", ".join(actions_string))
                self.history.append(actions_string)

            elif "playmove" in string:
                substr = string[string.find("playmove") + len("playmove"):]
                actions = self.game.string_to_action(substr)
                move_count = self.playmove(actions)

                actions_string = [self.game.action_to_string(action) for action in actions]
                actions_string = actions_string[:move_count]
                print("=" + ", ".join(actions_string))
                self.history.append(actions_string)

            elif "play_manual" in string:
                self.play_manual(string)
                
            elif "play" in string:
                self.play_game(string)
            
            elif "save" in string :
                self.save_manual()
            
            elif "exit" in string or "quit" in string:
                break

            elif "load" in string or "ld" in string:
                path = string[(string.find("load") + len("load")) if string.find("load") != -1 else (string.find("ld") + len("ld")):].strip()
                if path != '':
                    if '.sgf' not in path:
                        with tempfile.NamedTemporaryFile('w+t', delete=False) as f:
                            f.write(path)
                            path = f.name
                            is_tfile = True
                    else:
                        is_tfile = False
                    self.load_manual(Path(path))
                    if is_tfile:
                        os.unlink(path)
            elif "test_action" in string:
                print(self.test_action(string))
                
            elif "test_prune" in string:
                self.test_prune()
            elif "test_white_block" in string or "tw" in string:
                self.test_white_blockable()

            end = time.time()
            print("Command '{}' use".format(string), (end - start), "seconds")                

        # modified start
        self.showboard()
        if self.automode:
            self.save_manual()
            return
        elif self.save_result:
            self.save("{}.yaml".format(result))
        # modified end
        self.stop()

    def stop(self):
        self.engine.stop()

    def fight(self, args):
        games_fight = 0
        first_model_wins = 0
        second_model_wins = 0
        first_model_first = 0 
        second_model_first = 0 
        first_model_second = 0
        second_model_second = 0
        fileName = ""
        while (games_fight != args.num_fight_games):
            self.clear()
            self.history = []
            model_player = games_fight % 2
            print("[Start new game]: " + str(games_fight))
            print("Initial Board:")
            self.showboard()
            while not self.state.is_terminal():
                if model_player == 0:
                    print(args.model)
                    self.engine.load_model(args.model)
                    model_player = 1
                elif model_player == 1:
                    print(args.opp_model)
                    self.engine.load_model(args.opp_model)
                    model_player = 0
                actions = self.genmove()
                self.play(actions)
                actions_string = [self.game.action_to_string(action) for action in actions]
                print("Player " + str(model_player) + ": " + ", ".join(actions_string))
                self.showboard()
                self.history.append(actions_string)

            winner = self.state.get_winner()
            print("Winner: " + str(self.state.get_winner()))
            if games_fight % 2 == winner:
                if games_fight%2==0:
                    first_model_first+=1
                else:
                    first_model_second+=1
                first_model_wins = first_model_wins + 1
            
            elif games_fight % 2 != winner:
                if games_fight%2==1:
                    second_model_first+=1
                else:
                    second_model_second+=1
                second_model_wins = second_model_wins + 1
            games_fight = games_fight + 1

            
            extractMname=lambda m: re.search('[\d]*.pt', m).group().split('.')[0].lstrip('0')
            mname1=extractMname(args.model)
            mname2=extractMname(args.opp_model)
            mname1="0" if len(mname1)==0 else mname1
            mname2="0" if len(mname2)==0 else mname2
            

            filepath=re.search('slither[\d]*', args.model)
            filepath="slither/" if filepath == None else filepath.group()
            filepath= "./manual/"+filepath
            try:
                self.save_manual(file_name=f'{filepath}/{mname1}_{mname2}_{games_fight-1}.sgf')
                fileName=f'{filepath}/{mname1}_{mname2}.txt'
            except Exception as e: 
                print(e)
                
        with open(fileName,"w") as file:
            file.write("total_games: " + str(games_fight)+"\n")
            file.write("first_model_wins: " + str(first_model_wins) + ", second_model_wins: " + str(second_model_wins)+"\n")
            file.write("first_model_wins(first): "+str(first_model_first)+", second_model_wins(first): "+str(second_model_first)+"\n")
            file.write("first_model_winrate: " + str(100*(float(first_model_wins)/games_fight)) + "%" +
                ", second_model_winrate: " + str(100*(float(second_model_wins)/games_fight)) + "%")
            print("total_games: " + str(games_fight))
            print("first_model_wins: " + str(first_model_wins) + ", second_model_wins: " + str(second_model_wins))
            print("first_model_wins(first): "+str(first_model_first)+", second_model_wins(first): "+str(second_model_first))
            print("first_model_winrate: " + str(100*(float(first_model_wins)/games_fight)) + "%" +
                ", second_model_winrate: " + str(100*(float(second_model_wins)/games_fight)) + "%")
        # print("first_model_winrate(first): ")

        self.stop()

if __name__ == '__main__':
    import faulthandler
    faulthandler.enable()

    GPU_COUNT = torch.cuda.device_count()
    DEFAULT_GPUS = ','.join(str(i) for i in range(GPU_COUNT))

    parser = argparse.ArgumentParser()
    # modified start
    parser.add_argument('-times', '--times', default=5, type=int)
    # modified end
    parser.add_argument('--gpus', default=DEFAULT_GPUS)
    parser.add_argument('-game', '--game', required=True)
    parser.add_argument('-model', '--model', required=True)
    parser.add_argument('-opp_model', '--opp_model', required=False)    
    parser.add_argument('-count', '--simulation-count', default=800, type=int)
    parser.add_argument('-cpuct', '--c-puct', default=1.5)
    parser.add_argument('-mode', '--mode', required=False, default="gtp")
    parser.add_argument('-nf', '--num-fight-games', default=1, type=int)
    parser.add_argument('-save', '--save', action='store_true')
    parser.add_argument('-verbose', '--verbose', action='store_true')
    parser.add_argument('-top', '--top', default=10, type=int)

    parser.add_argument('-c', '--cpu-workers', default=os.cpu_count(), type=int)
    parser.add_argument('-g', '--gpu-workers', default=GPU_COUNT, type=int)
    parser.add_argument('-b', '--batch-size', default=os.cpu_count()//2, type=int)
    parser.add_argument('-transform', '--transform', default=0, type=int)

    args = parser.parse_args()
    args.gpus = [int(gpu) for gpu in args.gpus.split(',')]

    if args.verbose:
        print("cpu workers =", args.cpu_workers, file=sys.stderr)
        print("gpu workers =", args.gpu_workers, file=sys.stderr)
        print("batch size =", args.batch_size, file=sys.stderr)

    time.sleep(1)
    agent = CLI_agent(args)
    if args.mode == "gtp":
        try:
            agent.loop()
        except:
            agent.stop()
    elif args.mode == "fight":
        agent.fight(args)
    # modified start
    elif args.mode == "auto":
        for round in range(args.times):
            print(f"Round {round+1}:",end=" ")
            try:
                agent.loop()
                print("completed")
                exit(0)
            except:
                agent.stop()
    # modified end
    

