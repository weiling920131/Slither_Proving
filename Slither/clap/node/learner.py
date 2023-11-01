#! /usr/bin/env python3
import argparse
import asyncio
import random
import tempfile
import yaml
import zmq
import zmq.asyncio
from pathlib import Path
import zstandard as zstd

import numpy as np
from collections import Counter
import subprocess
import os
import psutil

import torch
import torch.cuda
import torch.jit
import torch.nn
import torch.optim
from torch.utils.data import Dataset, DataLoader
from torch.utils.tensorboard import SummaryWriter

import clap.game
import clap.utils.socket
from clap.pb import clap_pb2
from clap.nn import AlphaZero
from clap.utils import LRUCache


class ReplayBuffer(Dataset):
    def __init__(self, game, config):
        self.game = game

        self.capacity = config['replay_buffer_size']
        self.states_to_train = config.get('states_to_train', None)
        self.trajectories_to_train = config.get('trajectories_to_train', None)
        assert self.states_to_train != self.trajectories_to_train, 'the two options are disjoint.'
        self.sample_ratio = config.get('sample_ratio', None)
        self.sample_states = config.get('sample_states', None)
        assert self.sample_ratio != self.sample_states, 'the two options are disjoint.'
        self.sample_retention = config.get('sample_retention', False)

        self.use_observation = config.get('use_observation', False)
        self.use_transformation = config.get('use_transformation', False)

        self.data = []
        self.reset_stats()

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        # if use transformation, sample one transformation and apply to state and policy
        if self.use_transformation:
            transformation_type = random.randint(0, self.game.num_transformations - 1)
            observation = self.game.transform_observation(self.data[index][0].flatten().numpy(), transformation_type)
            observation_tensor = torch.tensor(observation).view(self.game.observation_tensor_shape)
            policy = self.game.transform_policy(self.data[index][1].numpy(), transformation_type)
            target_policy = torch.tensor(policy)
            return (observation_tensor, target_policy, self.data[index][2])
        return self.data[index]

    def add_trajectory(self, trajectory):
        if not self.use_observation:
            state = self.game.new_initial_state()
        for pb_state in trajectory.states:
            if not self.use_observation:
                observation_tensor = torch.tensor(state.observation_tensor()).view(self.game.observation_tensor_shape)
                state.apply_action(pb_state.transition.action)
            else:
                observation_tensor = torch.tensor(pb_state.observation_tensor).view(self.game.observation_tensor_shape)
            policy = torch.tensor(pb_state.evaluation.policy)
            returns = torch.tensor(trajectory.statistics.rewards)

            self.data.append((observation_tensor, policy, returns))

            self.num_new_states += 1

            if len(self.data) > self.capacity:
                self.data.pop(0)
        self.num_new_trajectories += 1

        # train the model when there are N newly generated states
        if self.states_to_train is not None:
            if self.num_new_states >= self.states_to_train:
                self.ready = True
        # train the model when there are N newly generated trajectories
        else:  # if self.trajectories_to_train is not None:
            if self.num_new_trajectories >= self.trajectories_to_train:
                self.ready = True

    def get_states_to_train(self):
        if self.ready:
            self.ready = False
            if self.states_to_train is not None:
                num_states = self.states_to_train
                self.num_new_states -= self.states_to_train
                self.num_new_trajectories = 0
            else:  # if self.trajectories_to_train is not None
                num_states = self.num_new_states
                self.num_new_states = 0
                self.num_new_trajectories -= self.trajectories_to_train
            if self.sample_ratio is not None:
                num_states *= self.sample_ratio
            else:  # if self.sample_states is not None
                num_states = self.sample_states
            if self.sample_retention:
                num_states *= len(self.data) / self.capacity
            return int(num_states)
        return 0

    def reset_stats(self):
        self.num_new_states = 0
        self.num_new_trajectories = 0
        self.ready = False


class Learner:
    def __init__(self, args, config):
        self.compressor = zstd.ZstdCompressor(level=config['misc']['compression_level'], threads=-1)
        self.decompressor = zstd.ZstdDecompressor()

        self.game = clap.game.load(config['game'])

        self.storage_dir = Path(args.storage_dir)
        self.model_dir = self.storage_dir / 'model'
        self.trajectory_dir = self.storage_dir / 'trajectory'
        self.checkpoint_dir = self.storage_dir / 'checkpoint'
        self.log_dir = self.storage_dir / 'log'
        self.trajectory_manual_dir = self.storage_dir / 'trajectory_manual'
    
        self.model_dir = self.model_dir / f"slither{self.game.getBoardSize}"
        
        self.model_dir.mkdir(parents=True, exist_ok=True)
        self.trajectory_dir.mkdir(parents=True, exist_ok=True)
        self.checkpoint_dir.mkdir(parents=True, exist_ok=True)
        self.trajectory_manual_dir.mkdir(parents=True, exist_ok=True)
        self.latest_manual_name = 'latest.sgf'
        Path(self.trajectory_manual_dir / self.latest_manual_name).unlink(missing_ok=True)

        # self.model_response_cache[iteration]
        self.model_response_cache = LRUCache(capacity=4)
        self.iteration = 0
        self.trajectories = 0

        self.pb_trajectories = clap_pb2.Trajectories()


        self.checkpoint_freq = config['train']['checkpoint_freq']

        self.peers = set()

        self.replay_buffer = ReplayBuffer(
            self.game,
            config['train']['replay_buffer']
        )

        self.batch_size = config['train']['batch_size']

        self.model_config = config['model']

        self.socket = clap.utils.socket.router(port=args.port)

        self.device = torch.device(f'cuda:{args.gpus[0]}')

        latest_checkpoint = torch.load(self.checkpoint_dir / args.checkpoint) if args.restore else None

        self.model = AlphaZero(self.game, self.model_config)
        if latest_checkpoint:
            self.model.load_state_dict(latest_checkpoint['model'])
        self.model = torch.nn.DataParallel(self.model, device_ids=args.gpus)
        self.model.to(self.device)

        if config['train']['optimizer']['name'] == 'SGD':
            self.optimizer = torch.optim.SGD(
                self.model.parameters(),
                lr=config['train']['optimizer']['learning_rate'],
                momentum=config['train']['optimizer']['momentum'],
                weight_decay=config['train']['optimizer']['weight_decay'],
                nesterov=config['train']['optimizer']['nesterov']
            )

        if config['train']['lr_scheduler']['name'] == 'MultiStepLR':
            self.scheduler = torch.optim.lr_scheduler.MultiStepLR(
                self.optimizer,
                milestones=config['train']['lr_scheduler']['milestones'],
                gamma=config['train']['lr_scheduler']['gamma']
            )
            self.lr_scheduler_need_loss = False
        elif config['train']['lr_scheduler']['name'] == 'ReduceLROnPlateau':
            self.scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
                self.optimizer,
                factor=config['train']['lr_scheduler']['factor'],
                patience=config['train']['lr_scheduler']['patience'],
                min_lr=config['train']['lr_scheduler']['min_lr'],
                verbose=config['train']['lr_scheduler']['verbose']
            )
            self.lr_scheduler_need_loss = True

        if latest_checkpoint:
            self.iteration = latest_checkpoint['iteration']
            self.trajectories = latest_checkpoint['trajectories']
            if not config['train']['reset_learning_rate']:
                print("[Learning Rate] Restore scheduler")
                self.optimizer.load_state_dict(latest_checkpoint['optimizer'])
                self.scheduler.load_state_dict(latest_checkpoint['scheduler'])
            self.restore_replay_buffer()
        else:
            self.save_model()
            self.save_checkpoint()
        self.summary_writer = SummaryWriter(log_dir=self.log_dir, purge_step=self.iteration)

        if args.old_trajectory_dir:
            print("old trajectory :", args.old_trajectory_dir)
            self.old_trajectory_train(args.old_trajectory_dir)

    async def loop(self):
        while True:
            identity, raw = await self.socket.recv_multipart()
            packet = clap_pb2.Packet.FromString(raw)
            packet_type = packet.WhichOneof('payload')
            if packet_type == 'model_subscribe':
                self.peers.add(identity)
            elif packet_type == 'goodbye':
                self.peers.remove(identity)
            elif packet_type == 'model_request':
                await self.send_model(identity, packet.model_request)
            elif packet_type == 'trajectory':
                self.add_trajectory(packet.trajectory)
                if self.game.name == 'slither':
                    actions = [state.transition.action for state in packet.trajectory.states]
                    self.game.save_manual(actions, str(self.trajectory_manual_dir / self.latest_manual_name))
                
                if self.replay_buffer.ready:
                    if self.game.name == 'slither':
                        os.system(f'mv {self.trajectory_manual_dir / self.latest_manual_name} {self.trajectory_manual_dir / f"{self.iteration:05d}"}.sgf')
                    validate_loss = self.train()

                    process = psutil.Process()
                    process_memory = process.memory_info()
                    for name in process_memory._fields:
                        value = getattr(process_memory, name)
                        self.summary_writer.add_scalar("Memory/{}".format(name.capitalize()), value, self.iteration)

                    print("Iteration {} : Replay buffer {}, loss {}".format(self.iteration, len(self.replay_buffer), validate_loss))

                    self.iteration += 1
                    self.scheduler.step(validate_loss)
                    self.save_model()
                    if self.iteration % self.checkpoint_freq == 0:
                        self.save_checkpoint()
                    await self.notify_update()

    def restore_replay_buffer(self):
        start_iteration = 0
        total_states = 0
        for iteration in range(self.iteration - 1, -1, -1):
            trajectory_path = self.trajectory_dir / f'{iteration:05d}.pb.zstd'
            compressed_trajectory = trajectory_path.read_bytes()
            decompressed_trajectory = self.decompressor.decompress(compressed_trajectory)
            pb_trajectories = clap_pb2.Trajectories()
            pb_trajectories.ParseFromString(decompressed_trajectory)
            # print(trajectory_path, len(pb_trajectories.trajectories))
            for trajectory in pb_trajectories.trajectories:
                total_states += len(trajectory.states)
            if total_states >= self.replay_buffer.capacity:
                start_iteration = iteration
                # print(start_iteration, total_states)
                break

        print("self.iteration :", self.iteration)
        print("start iteration :", start_iteration)
        for iteration in range(start_iteration, self.iteration):
            # print("Iteration :", iteration)
            trajectory_path = self.trajectory_dir / f'{iteration:05d}.pb.zstd'
            compressed_trajectory = trajectory_path.read_bytes()
            decompressed_trajectory = self.decompressor.decompress(compressed_trajectory)
            pb_trajectories = clap_pb2.Trajectories()
            pb_trajectories.ParseFromString(decompressed_trajectory)
            # print(pb_trajectories)
            for trajectory in pb_trajectories.trajectories:
                self.replay_buffer.add_trajectory(trajectory)
            print("Replay Buffer {}: ".format(iteration), len(self.replay_buffer))
        self.replay_buffer.reset_stats()
        print("Finish restore replay")

    def old_trajectory_train(self, old_trajectory_dir):
        for file_path in sorted(Path(old_trajectory_dir).glob("*.pb.zstd")):
            print(file_path)
            compressed_trajectory = file_path.read_bytes()
            decompressed_trajectory = self.decompressor.decompress(compressed_trajectory)
            pb_trajectories = clap_pb2.Trajectories()
            pb_trajectories.ParseFromString(decompressed_trajectory)
            # print(pb_trajectories)
            for trajectory in pb_trajectories.trajectories:
                self.add_trajectory(trajectory)
                if self.replay_buffer.ready:
                    validate_loss = self.train()
                    print("Iteration {} : Replay buffer {}, loss {}".format(self.iteration, len(self.replay_buffer), validate_loss))
                    self.iteration += 1
                    self.scheduler.step(validate_loss)
                    self.save_model()
                    if self.iteration % self.checkpoint_freq == 0:
                        self.save_checkpoint()

        self.save_checkpoint('FinishOldTrajectoryTrain_{:05d}.pt'.format(self.iteration))
        print("Finish old trajectory train")

    def add_trajectory(self, trajectory):
        self.trajectories += 1
        self.pb_trajectories.trajectories.add().CopyFrom(trajectory)
        self.replay_buffer.add_trajectory(trajectory)

    def save_checkpoint(self, name='latest.pt'):
        checkpoint = {
            'iteration': self.iteration,
            'trajectories': self.trajectories,
            'model': self.model.module.state_dict(),
            'optimizer': self.optimizer.state_dict(),
            'scheduler': self.scheduler.state_dict()
        }
        torch.save(checkpoint, self.checkpoint_dir / f'{self.iteration:05d}.pt')
        torch.save(checkpoint, self.checkpoint_dir / name)

    def save_model(self):
        model = AlphaZero(self.game, self.model_config)
        model.load_state_dict(self.model.module.state_dict())
        model.eval()
        model_path = self.model_dir / f'{self.iteration:05d}.pt'
        model_path = self.model_dir / f'{self.iteration:05d}.pt'

        torch.save({'model': model, 'game': self.game.observation_tensor_shape}, model_path)

        env = os.environ.copy()
        print("subprocess :", end=" ")
        process = subprocess.Popen(
            args=["python3", "clap/node/model_saver.py", "-path", model_path],
            env=env,
            universal_newlines=True
        )
        process.wait()
        print(process.pid, model_path)

        #sample_input = torch.rand(1, *self.game.observation_tensor_shape)
        #traced_model = torch.jit.trace(model, sample_input)
        # traced_model.save(str(model_path))

    async def notify_update(self):
        model_info = clap_pb2.ModelInfo(version=self.iteration)
        packet = clap_pb2.Packet(model_info=model_info)
        raw = packet.SerializeToString()
        for peer in self.peers:
            await self.send_raw(peer, raw)

    def log_stats(self):
        total_game = len(self.pb_trajectories.trajectories)
        player_returns_counter = []
        for _ in range(self.game.num_players):
            player_returns_counter.append(Counter())
        game_steps = []
        for trajectory in self.pb_trajectories.trajectories:
            game_steps.append(len(trajectory.states))
            for index in range(len(trajectory.statistics.rewards)):
                player_returns_counter[index].update([str(trajectory.statistics.rewards[index])])

        self.summary_writer.add_scalar('stats/total', self.trajectories, self.iteration)
        self.summary_writer.add_scalar('stats/#games', total_game, self.iteration)
        stats_steps = {"mean": np.mean(game_steps), "min": np.min(game_steps), "max": np.max(game_steps), "std": np.std(game_steps)}
        self.summary_writer.add_scalars('stats/steps', stats_steps, self.iteration)
        for i in range(len(player_returns_counter)):
            returns_frequency = {key: value/total_game for key, value in player_returns_counter[i].items()}
            self.summary_writer.add_scalars('stats/PLAYER:{}_rate'.format(i), returns_frequency, self.iteration)
            self.summary_writer.add_scalars('stats/PLAYER:{}_games'.format(i), player_returns_counter[i], self.iteration)

    def train(self):
        trajectory_path = self.trajectory_dir / f'{self.iteration:05d}.pb.zstd'
        trajectory = self.pb_trajectories.SerializeToString()
        compressed_trajectory = self.compressor.compress(trajectory)
        trajectory_path.write_bytes(compressed_trajectory)
        self.log_stats()
        self.pb_trajectories.Clear()

        states_to_train = self.replay_buffer.get_states_to_train()
        trained_states = 0

        data_loader = DataLoader(
            dataset=self.replay_buffer,
            batch_size=self.batch_size,
            shuffle=True
        )

        self.model.train()
        for observation_tensor, target_policy, target_value in data_loader:
            trained_states += len(observation_tensor)
            if (drop := trained_states - states_to_train) > 0:
                trained_states -= drop
                observation_tensor = observation_tensor[drop:]
                target_policy = target_policy[drop:]
                target_value = target_value[drop:]
            # print("len(observation_tensor)", len(observation_tensor))
            target_policy = target_policy.to(self.device)
            target_value = target_value.to(self.device)

            self.optimizer.zero_grad()
            policy, value = self.model.forward(observation_tensor)
            policy_loss = (-target_policy * (1e-8 + policy).log()).sum(dim=1).mean()
            value_loss = torch.nn.MSELoss()(target_value, value)
            loss = policy_loss + value_loss
            loss.backward()
            self.optimizer.step()

            # the end of current training epoch
            if drop >= 0:
                lr = next(iter(self.optimizer.param_groups))['lr']
                self.summary_writer.add_scalar('params/lr', lr, self.iteration)
                self.summary_writer.add_scalar('loss/', loss.item(), self.iteration)
                self.summary_writer.add_scalar('loss/policy', policy_loss.item(), self.iteration)
                self.summary_writer.add_scalar('loss/value', value_loss.item(), self.iteration)
                validate_loss = loss.item()
                break

        if self.lr_scheduler_need_loss:
            return validate_loss

    async def send_raw(self, identity, raw):
        await self.socket.send_multipart([identity, raw])

    async def send_packet(self, identity, packet):
        await self.send_raw(identity, packet.SerializeToString())

    async def send_model(self, identity, model_request):
        if model_request.version == -1:
            model_request.version = self.iteration
        iteration = model_request.version
        if iteration not in self.model_response_cache:
            model_path = self.model_dir / f'{iteration:05d}.pt'
            model_response = clap_pb2.Model()
            model_response.info.CopyFrom(model_request)
            model_response.blobs.append(model_path.read_bytes())
            packet = clap_pb2.Packet(model_response=model_response)
            self.model_response_cache[iteration] = packet.SerializeToString()
        await self.send_raw(identity, self.model_response_cache[iteration])


async def main():
    GPU_COUNT = torch.cuda.device_count()
    DEFAULT_GPUS = ','.join(str(i) for i in range(GPU_COUNT))

    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', required=True)
    parser.add_argument('-f', '--config', required=True)
    parser.add_argument('-s', '--storage-dir', default=str(Path('storage').resolve()))
    parser.add_argument('-r', '--restore', action='store_true')
    parser.add_argument('-pt', '--checkpoint', default='latest.pt', type=str)
    parser.add_argument('-o', '--old-trajectory-dir', type=str)
    parser.add_argument('--gpus', default=DEFAULT_GPUS)

    args = parser.parse_args()
    config = yaml.safe_load(Path(args.config).read_text())

    args.gpus = [int(gpu) for gpu in args.gpus.split(',')]

    learner = Learner(args, config)
    await learner.loop()


def run_main():
    '''Run main program in asyncio'''
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        zmq.asyncio.Context.instance().destroy()
        print('\rterminated by ctrl-c')


if __name__ == '__main__':
    import faulthandler
    faulthandler.enable()
    run_main()
