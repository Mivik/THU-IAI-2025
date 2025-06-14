import torch
import torch.nn.functional as F

from torch import nn


class CNN(nn.Module):
    def __init__(
        self,
        embedding: nn.Embedding,
        output_dim: int,
        out_channels: int,
        kernel_sizes: list[int],
        dropout: float,
    ):
        super().__init__()
        self.embedding = embedding
        self.conv = nn.ModuleList(
            nn.Conv2d(
                in_channels=1,
                out_channels=out_channels,
                kernel_size=(size, embedding.embedding_dim),
            )
            for size in kernel_sizes
        )
        self.fc = nn.Linear(out_channels * len(kernel_sizes), output_dim)
        self.dropout = nn.Dropout(dropout)

    def forward(self, text):
        x = self.embedding(text).unsqueeze(1)
        x = [F.relu(conv(x).squeeze(3)) for conv in self.conv]
        x = [F.max_pool1d(conv, conv.size(2)).squeeze(2) for conv in x]
        x = self.dropout(torch.cat(x, dim=1))
        return self.fc(x)
