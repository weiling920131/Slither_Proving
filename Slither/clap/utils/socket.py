import zmq
import zmq.asyncio


def dealer(identity, remote_address):
    context = zmq.asyncio.Context.instance()
    socket = context.socket(zmq.DEALER)
    socket.setsockopt(zmq.LINGER, 0)
    socket.setsockopt_string(zmq.IDENTITY, identity)
    socket.connect(f'tcp://{remote_address}')
    return socket


def router(port):
    context = zmq.asyncio.Context.instance()
    socket = context.socket(zmq.ROUTER)
    socket.setsockopt(zmq.LINGER, 0)
    socket.bind(f'tcp://*:{port}')
    return socket
