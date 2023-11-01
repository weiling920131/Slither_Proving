from torch import nn

from .resnet import ResNet
from .se_resnet import SEResNet


class AlphaZero(nn.Module):
    def __init__(self,
                 game,
                 config):
        super().__init__()
        self.observation_tensor_shape = game.observation_tensor_shape

        # channels, height, width
        in_channels, height, width = self.observation_tensor_shape

        channels = config["channels"]
        blocks = config["blocks"]
        backbone = config.get("backbone", "ResNet")

        if backbone == "SE-ResNet":
            fc_hidden_dimension = config.get("fc_hidden_dimension", 16)
            self.backbone = SEResNet(in_channels, blocks, channels, fc_hidden_dimension)
        else:
            self.backbone = ResNet(in_channels, blocks, channels)

        # policy head
        self.policy_head_front = nn.Sequential(
            nn.Conv2d(in_channels=channels,
                      out_channels=2,
                      kernel_size=1),
            nn.BatchNorm2d(num_features=2),
            nn.ReLU(),
        )

        self.policy_head_end = nn.Sequential(
            nn.Linear(in_features=2 * height * width,
                      out_features=game.num_distinct_actions),
            nn.Softmax(dim=1)
        )

        # value head
        self.value_head_front = nn.Sequential(
            nn.Conv2d(in_channels=channels,
                      out_channels=1,
                      kernel_size=1),
            nn.BatchNorm2d(num_features=1),
            nn.ReLU(),
        )

        self.value_head_end = nn.Sequential(
            nn.Linear(in_features=height * width,
                      out_features=channels),
            nn.ReLU(),
            nn.Linear(in_features=channels,
                      out_features=game.num_players),
            nn.Tanh()
        )

    def forward(self, x):
        _, height, width = self.observation_tensor_shape
        x = self.backbone(x)
        # policy head
        p = self.policy_head_front(x)
        p = p.view(-1, 2 * height * width)
        p = self.policy_head_end(p)
        # value head
        v = self.value_head_front(x)
        v = v.view(-1, height * width)
        v = self.value_head_end(v)
        return p, v
