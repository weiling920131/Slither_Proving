#! /usr/bin/env python3
import argparse
import asyncio
import concurrent.futures
import multiprocessing
import platform
import tempfile
import torch.cuda
import yaml
import zmq
import zmq.asyncio
from pathlib import Path
from uuid import uuid4

import clap.game
import clap.mcts
import clap.utils
import clap.utils.socket
from clap.pb import clap_pb2
from clap.utils import ModelFetcher


class Actor:
    def __init__(self, args, config):
        self.identity = f'actor-{args.suffix}'

        self.node = clap_pb2.Node(
            identity=f'actor-{args.suffix}',
            hostname=platform.node(),
            hardware=clap.utils.get_hardware_info()
        )

        self.learner = clap.utils.socket.dealer(
            identity=self.node.identity,
            remote_address=args.learner
        )

        self.model_fetcher = ModelFetcher(
            upstream=args.model_provider,
            cache_size=8
        )

        self.model_info = clap_pb2.ModelInfo(name='default', version=-1)

        self.engine = clap.mcts.Engine(args.gpus)
        self.engine.load_game(config['game'])
        self.engine.batch_size = args.batch_size
        self.engine.max_simulations = config['mcts']['max_simulations']
        self.engine.c_puct = config['mcts']['c_puct']
        self.engine.dirichlet_alpha = config['mcts']['dirichlet']['alpha']
        self.engine.dirichlet_epsilon = config['mcts']['dirichlet']['epsilon']
        self.engine.temperature_drop = config['mcts']['temperature_drop']
        self.engine.num_sampled_transformations = config['mcts']['num_sampled_transformations']
        self.engine.batch_per_job = 1 if self.engine.num_sampled_transformations == 0 \
                                    else self.engine.num_sampled_transformations
        self.engine.save_observation = config['mcts']['save_observation']

    async def prepare(self, args):
        model_subscribe = clap_pb2.Heartbeat()
        await self.send_packet(clap_pb2.Packet(model_subscribe=model_subscribe))

        self.model_info.version = await self.model_fetcher.get_latest_version(self.model_info.name)
        latest_model = await self.model_fetcher.get(self.model_info)
        self.load_model(latest_model)

        self.engine.start(args.cpu_workers, args.gpu_workers)

    async def loop(self):
        await asyncio.gather(
            self.recv_loop(),
            self.send_loop()
        )

    def stop(self):
        self.engine.stop()

    async def recv_loop(self):
        while True:
            packet = await self.recv_packet()
            packet_type = packet.WhichOneof('payload')
            if packet_type == 'model_info':
                model = await self.model_fetcher.get(packet.model_info)
                print("[Actor.Engine] Load model : ", end='')
                self.load_model(model)

    async def send_loop(self):
        executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        event_loop = asyncio.get_event_loop()
        while True:
            raw = await event_loop.run_in_executor(executor, self.engine.get_trajectory)
            if raw:
                trajectory = clap_pb2.Trajectory.FromString(raw)
                await self.send_packet(clap_pb2.Packet(trajectory=trajectory))

    def load_model(self, model):
        with tempfile.NamedTemporaryFile() as tmp_file:
            tmp_file.write(model.blobs[0])
            self.engine.load_model(tmp_file.name)
            print("Iteration", model.info.version)

    async def recv_packet(self):
        raw = await self.learner.recv()
        packet = clap_pb2.Packet.FromString(raw)
        return packet

    async def send_packet(self, packet):
        raw = packet.SerializeToString()
        await self.learner.send(raw)


async def main():
    GPU_COUNT = torch.cuda.device_count()
    DEFAULT_GPUS = ','.join(str(i) for i in range(GPU_COUNT))

    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--batch-size', type=int, required=True)
    parser.add_argument('-c', '--cpu-workers', type=int, default=multiprocessing.cpu_count())
    parser.add_argument('-g', '--gpu-workers', type=int)
    parser.add_argument('--gpus', default=DEFAULT_GPUS)
    parser.add_argument('-f', '--config', required=True)
    parser.add_argument('-l', '--learner', required=True)
    parser.add_argument('-m', '--model-provider', required=False)
    parser.add_argument('-s', '--suffix', default=uuid4().hex)

    args = parser.parse_args()
    config = yaml.safe_load(Path(args.config).read_text())

    args.gpus = [int(gpu) for gpu in args.gpus.split(',')]
    args.gpu_workers = args.gpu_workers or len(args.gpus) * 2
    args.model_provider = args.model_provider or args.learner

    actor = Actor(args, config)
    try:
        await actor.prepare(args)
        await actor.loop()
    except asyncio.CancelledError:
        actor.stop()


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
