import argparse
import os

import torch
import torch.jit

if __name__ == '__main__':
    import faulthandler
    faulthandler.enable()

    parser = argparse.ArgumentParser()
    parser.add_argument('-path', '--path', required=True, type=str)

    args = parser.parse_args()
    model_path = args.path

    model_info = torch.load(args.path)
    model = model_info['model']
    game = model_info['game']
    model.eval()
    os.remove(args.path)

    sample_input = torch.rand(1, *game)
    traced_model = torch.jit.trace(model, sample_input)
    traced_model.save(model_path)
