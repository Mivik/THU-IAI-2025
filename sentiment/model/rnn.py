from torch import nn


class RNN(nn.Module):
    def __init__(
        self,
        embedding: nn.Embedding,
        output_dim: int,
        hidden_dim: int,
        num_layers: int,
        dropout: float,
    ):
        super().__init__()
        self.embedding = embedding
        self.rnn = nn.LSTM(
            input_size=embedding.embedding_dim,
            hidden_size=hidden_dim,
            num_layers=num_layers,
            batch_first=True,
            bidirectional=True,
        )
        self.fc = nn.Linear(hidden_dim * 2, output_dim)
        self.dropout = nn.Dropout(dropout)

    def forward(self, text):
        x = self.embedding(text)
        x, _ = self.rnn(x)
        x = self.dropout(x[:, -1, :])
        return self.fc(x)
