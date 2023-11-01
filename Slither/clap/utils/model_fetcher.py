import asyncio
from uuid import uuid4

import clap.utils.socket
from clap.pb import clap_pb2
from .lru_cache import LRUCache


class ModelFetcher:
    def __init__(self, upstream, cache_size):
        self.cache = LRUCache(capacity=cache_size)
        self.latest_version = None
        self.upstream = clap.utils.socket.dealer(
            identity=f'model-fetcher-{uuid4().hex}',
            remote_address=upstream
        )
        asyncio.create_task(self.recv_loop())

    def get(self, model_info):
        key = model_info.version
        if key not in self.cache:
            self.cache[key] = asyncio.Future()
            asyncio.create_task(self.send_model_request(model_info))
        return self.cache[key]

    async def get_latest_version(self, name):
        if self.latest_version is None:
            self.latest_version = asyncio.Future()
            await self.send_model_request(
                clap_pb2.ModelInfo(
                    name=name,
                    version=-1,
                ))
            return await self.latest_version
        return self.latest_version

    async def recv_loop(self):
        while True:
            packet = await self.recv_packet()
            packet_type = packet.WhichOneof('payload')

            if packet_type == 'model_response':
                model = packet.model_response
                info = model.info
                name, version = info.name, info.version

                # update latest version
                if asyncio.isfuture(self.latest_version):  # get_latest_version
                    self.latest_version.set_result(version)
                    self.latest_version = version
                elif version > self.latest_version:
                    self.latest_version = version

                # update model
                if model:
                    key = version
                    if key not in self.cache:
                        self.cache[key] = asyncio.Future()
                    self.cache[key].set_result(model)

    async def send_model_request(self, model_info):
        packet = clap_pb2.Packet(model_request=model_info)
        await self.send_packet(packet)

    async def recv_packet(self):
        raw = await self.upstream.recv()
        packet = clap_pb2.Packet.FromString(raw)
        return packet

    async def send_packet(self, packet):
        raw = packet.SerializeToString()
        await self.upstream.send(raw)
