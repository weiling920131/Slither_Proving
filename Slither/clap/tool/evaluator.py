import argparse
import asyncio
import concurrent.futures
import multiprocessing
import os
import re
import time
from collections import Counter
from pathlib import Path

import clap.game
import clap.mcts
import numpy as np
import pandas as pd
import torch.cuda
import yaml
from clap.pb import clap_pb2
from torch.utils.tensorboard import SummaryWriter


class Evaluator:
    def __init__(self, args, config):
        self.eval_config = config['evaluator']

        if 'opening' not in self.eval_config or self.eval_config['opening'] is None:
            self.eval_config['opening'] = ['']
        print('opening:', self.eval_config['opening'])

        mcts_config = self.eval_config.get('mcts', {})
        if mcts_config is None:
            mcts_config = {}
        if 'dirichlet' not in mcts_config or mcts_config['dirichlet'] is None:
            mcts_config['dirichlet'] = {}
        config['mcts']['dirichlet'].update(mcts_config.get('dirichlet', {}))
        mcts_config['dirichlet'] = config['mcts']['dirichlet']
        config['mcts'].update(mcts_config)
        print('mcts parameter:', config['mcts'])

        self.args = args
        self.args.batch_size = args.batch_size or self.eval_config['num_games'] * len(self.eval_config['opening'])

        self.storage_dir = Path(args.storage_dir)
        self.model_dir = self.storage_dir / 'model'
        self.log_dir = self.storage_dir / self.args.log_dir
        self.summary_writer = SummaryWriter(log_dir=self.log_dir, purge_step=self.args.best_iteration)

        self.game = clap.game.load(config['game'])
        self.engine = clap.mcts.Engine(self.args.gpus, self.game.num_players)

        self.engine.load_game(config['game'])

        self.engine.batch_size = self.args.batch_size
        self.engine.max_simulations = config['mcts']['max_simulations']
        self.engine.c_puct = config['mcts']['c_puct']
        self.engine.dirichlet_alpha = config['mcts']['dirichlet']['alpha']
        self.engine.dirichlet_epsilon = config['mcts']['dirichlet']['epsilon']
        self.engine.temperature_drop = config['mcts']['temperature_drop']
        self.engine.num_sampled_transformations = config['mcts']['num_sampled_transformations']
        self.engine.batch_per_job = 1 if self.engine.num_sampled_transformations == 0 \
            else self.engine.num_sampled_transformations

        self.engine.play_until_terminal = True
        self.engine.auto_reset_job = False
        self.engine.play_until_turn_player = True
        self.engine.mcts_until_turn_player = True
        self.engine.verbose = False
        self.engine.top_n_childs = 0
        self.engine.states_value_using_childs = True
        self.engine.report_serialize_string = True

    async def fight(self, model_paths: list):
        for index in range(len(model_paths)):
            print(index, str(model_paths[index]))
            self.engine.load_model(str(model_paths[index]), index)

        print("start fight")
        for serialize_string in self.eval_config['opening']:
            self.engine.add_job(self.eval_config['num_games'], serialize_string)
            # state = self.game.deserialize_state(serialize_string)
            # print(state)

        player_returns_counter = []
        for _ in range(self.game.num_players):
            player_returns_counter.append(Counter())

        executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        event_loop = asyncio.get_event_loop()
        finish = 0
        returns_results = []
        while finish < self.eval_config['num_games'] * len(self.eval_config['opening']):
            raw = await event_loop.run_in_executor(executor, self.engine.get_trajectory)
            if raw:
                trajectory = clap_pb2.Trajectory.FromString(raw)
                # print(trajectory.appendix)

                for index in range(len(trajectory.statistics.rewards)):
                    player_returns_counter[index].update([trajectory.statistics.rewards[index]])

                returns_results.append(trajectory.statistics.rewards)
                finish += 1
                print("finish:", finish, "\t detail rewards:", player_returns_counter, end="\r")
                # print(finish, returns_results, end="\r")

        print("\nwin rate (win=1.0, draw=0.5, lose=0.0)", (np.sum(returns_results, axis=0) + finish) / (2 * finish))
        return returns_results

    async def start(self):
        self.engine.start(self.args.cpu_workers, self.args.gpu_workers, 0)

        regex = r"(\S*)/(\d*).pt"
        model_paths = sorted(Path(self.model_dir).glob("*.pt"))

        best_model_path = self.model_dir / f'{self.args.best_iteration:05d}.pt'
        best_elo = self.args.best_elo_rating
        string_match = re.match(regex, str(best_model_path))
        best_iteration = int(string_match.group(2))

        self.summary_writer.add_scalar('best/best iteration', best_iteration, best_iteration)
        self.summary_writer.add_scalar('best/elo rating(Corresponding)', best_elo, best_iteration)

        try:
            for file_path in model_paths:
                if best_model_path == file_path:
                    continue
                string_match = re.match(regex, str(file_path))
                iteration = int(string_match.group(2))
                if iteration % self.eval_config['frequency'] == 0 and iteration >= self.args.best_iteration:
                    try:
                        print('-' * 30)
                        print([best_model_path, file_path])
                        results = await self.fight([best_model_path, file_path])
                        results = np.array(results)
                        results2 = await self.fight([file_path, best_model_path])
                        results2 = np.array(results2)

                        score = np.concatenate((results, np.fliplr(results2)))
                        print("total win rate:", np.mean((score + 1.0) / 2.0, axis=0))
                        if np.abs(score.mean(axis=0)[1]) == 1:
                            score = np.concatenate((score, np.array([[0.0, 0.0]])))

                        score = np.mean((score + 1.0) / 2.0, axis=0)
                        win_rate = score[1]

                        elo_diff = 400 * np.log10(win_rate / (1 - win_rate))
                        self.summary_writer.add_scalar('best/elo rating(Corresponding)', best_elo + elo_diff, iteration)

                        if win_rate >= self.eval_config['replace_rate']:
                            best_model_path = file_path
                            best_elo += elo_diff
                            best_iteration = iteration

                        self.summary_writer.add_scalar('best/best iteration', best_iteration, iteration)
                        print("best model:", best_model_path, "\telo rating:", best_elo)

                    except asyncio.CancelledError:
                        self.engine.stop()
                        break

        except asyncio.CancelledError:
            self.engine.stop()

    def stop(self):
        self.engine.stop()


async def main():
    GPU_COUNT = torch.cuda.device_count()
    DEFAULT_GPUS = ','.join(str(i) for i in range(GPU_COUNT))

    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--batch-size', type=int)
    parser.add_argument('-c', '--cpu-workers', type=int, default=multiprocessing.cpu_count())
    parser.add_argument('-g', '--gpu-workers', type=int)
    parser.add_argument('--gpus', default=DEFAULT_GPUS)
    parser.add_argument('-f', '--config', required=True)
    parser.add_argument('-s', '--storage-dir', default=str(Path('storage').resolve()))
    parser.add_argument('-log', '--log-dir', default='evaluator_log')
    parser.add_argument('-i', '--best-iteration', type=int, default=0)
    parser.add_argument('-e', '--best-elo-rating', type=float, default=0)

    args = parser.parse_args()
    config = yaml.safe_load(Path(args.config).read_text())

    args.gpus = [int(gpu) for gpu in args.gpus.split(',')]
    args.gpu_workers = args.gpu_workers or len(args.gpus) * 2

    evaluator = Evaluator(args, config)
    try:
        await evaluator.start()
    except asyncio.CancelledError:
        evaluator.stop()
    finally:
        evaluator.stop()


def run_main():
    '''Run main program in asyncio'''
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print('\rterminated by ctrl-c')


if __name__ == '__main__':
    import faulthandler
    faulthandler.enable()
    run_main()
