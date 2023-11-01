#! /usr/bin/env python3
import argparse
import asyncio
import zmq
import zmq.asyncio

import clap.utils.socket
from clap.pb import clap_pb2
from clap.utils import ModelFetcher


class ModelProvider:
    def __init__(self, args):
        self.socket = clap.utils.socket.router(args.port)

        self.model_fetcher = ModelFetcher(
            upstream=args.upstream,
            cache_size=4
        )

    async def loop(self):
        await asyncio.gather(self.recv_loop())

    async def send_packet(self, identity, packet):
        raw = packet.SerializeToString()
        self.socket.send_multipart([identity, raw])

    async def recv_packet(self):
        identity, raw = await self.socket.recv_multipart()
        packet = clap_pb2.Packet.FromString(raw)
        return identity, packet

    async def recv_loop(self):
        while True:
            identity, packet = await self.recv_packet()
            packet_type = packet.WhichOneof('payload')
            if packet_type == 'model_request':
                model_request = packet.model_request
                asyncio.create_task(self.send_model(identity, model_request))

    async def send_model(self, identity, model_request):
        model = await self.model_fetcher.get(model_request)
        packet = clap_pb2.Packet(model_response=model)
        await self.send_packet(identity, packet)


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-u', '--upstream', required=True)
    parser.add_argument('-p', '--port', type=int, required=True)

    args = parser.parse_args()

    model_provider = ModelProvider(args)
    try:
        await model_provider.loop()
    except asyncio.CancelledError:
        pass


def run_main():
    '''Run main program in asyncio'''
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        zmq.asyncio.Context.instance().destroy()
        print('\rterminated by ctrl-c')


if __name__ == '__main__':
    run_main()
