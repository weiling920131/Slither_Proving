from torch import nn


class BasicBlock(nn.Module):
    def __init__(self, in_channels, out_channels):
        super().__init__()
        self.conv1 = nn.Conv2d(in_channels,
                               out_channels,
                               kernel_size=3,
                               stride=1,
                               padding=1)
        self.bn1 = nn.BatchNorm2d(num_features=out_channels)
        self.relu1 = nn.ReLU()
        self.conv2 = nn.Conv2d(in_channels=out_channels,
                               out_channels=out_channels,
                               kernel_size=3,
                               stride=1,
                               padding=1)
        self.bn2 = nn.BatchNorm2d(num_features=out_channels)
        self.relu2 = nn.ReLU()

    def forward(self, x):
        y = self.conv1(x)
        y = self.bn1(y)
        y = self.relu1(y)
        y = self.conv2(y)
        y = self.bn2(y)
        x = x + y
        x = self.relu2(x)
        return x


class ResNet(nn.Module):
    def __init__(self, in_channels, blocks, out_channels):
        super().__init__()
        self.conv1 = nn.Sequential(
            nn.Conv2d(in_channels,
                      out_channels,
                      kernel_size=3,
                      stride=1,
                      padding=1),
            nn.BatchNorm2d(out_channels),
            nn.ReLU(),
        )
        self.convs = nn.ModuleList([
            BasicBlock(in_channels=out_channels,
                       out_channels=out_channels) for _ in range(blocks)
        ])

    def forward(self, x):
        x = self.conv1(x)
        for conv in self.convs:
            x = conv(x)
        return x
