#! /usr/bin/env python3
import torch
import torch.jit
import argparse

import clap.game
from clap.nn import AlphaZero


def main():
    game = clap.game.load('tic_tac_toe')
    model = AlphaZero(game, 32, 5)
    model.eval()

    sample_input = torch.rand(1, *game.observation_tensor_shape)
    traced_model = torch.jit.trace(model, sample_input)
    traced_model.save('0.jit.pt')


if __name__ == '__main__':
    main()
