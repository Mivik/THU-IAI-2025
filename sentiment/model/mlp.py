import torch.nn.functional as F

from torch import nn


class MLP(nn.Module):
    def __init__(
        self,
        embedding: nn.Embedding,
        output_dim: int,
        hidden_dims: list[int],
        dropout: float,
    ):
        super().__init__()
        self.embedding = embedding
        self.first_fc = nn.Linear(embedding.embedding_dim, hidden_dims[0])
        self.hidden_fcs = nn.ModuleList(
            nn.Linear(hidden_dims[i], hidden_dims[i + 1])
            for i in range(len(hidden_dims) - 1)
        )
        self.last_fc = nn.Linear(hidden_dims[-1], output_dim)
        self.dropout = nn.Dropout(dropout)

    def forward(self, text):
        x = self.embedding(text)
        x = self.first_fc(x)
        for fc in self.hidden_fcs:
            x = F.relu(fc(x))
            x = self.dropout(x)
        x = x.permute(0, 2, 1)
        x = F.max_pool1d(x, x.size(2)).squeeze(2)
        return self.last_fc(x)
