import torch
from clap.pb import clap_pb2


def get_hardware_info():
    hardware = clap_pb2.Hardware()
    for i in range(torch.cuda.device_count()):
        gpu_properties = torch.cuda.get_device_properties(i)
        gpu = hardware.gpus.add()
        gpu.name = gpu_properties.name
        gpu.memory = gpu_properties.total_memory
    return hardware
