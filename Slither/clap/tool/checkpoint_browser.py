import argparse
from pathlib import Path

import torch
if __name__ == '__main__':
    import faulthandler
    faulthandler.enable()

    parser = argparse.ArgumentParser()
    parser.add_argument('-cp', '--checkpoint-dir', default=str(Path('storage/checkpoint').resolve()))
    args = parser.parse_args()

    print("-" * 62)
    print("{:>35} | {:>9} | {:>12}".format("file_path", 'iteration', 'trajectories'))
    print("-" * 62)
    for file_path in sorted(Path(args.checkpoint_dir).glob("*.pt")):
        checkpoint = torch.load(file_path)
        print("{:>35} | {:>9} | {:>12}".format(str(file_path), checkpoint['iteration'], checkpoint['trajectories']))
    print("-" * 62)