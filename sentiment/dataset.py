import torch

from gensim.models import KeyedVectors
from pathlib import Path

from torch import nn
from torch.utils.data import Dataset, DataLoader
from torch.nn.utils.rnn import pad_sequence

device = "cuda" if torch.cuda.is_available() else "cpu"

data_dir = Path("Dataset")

model = Path("model.bin")

if model.exists():
    word_ids, embedding = torch.load(model)
else:
    word2vec = KeyedVectors.load_word2vec_format(
        data_dir / "wiki_word2vec_50.bin", binary=True
    )

    word_ids = {"": 0}
    for txt in data_dir.glob("*.txt"):
        for line in txt.read_text().splitlines():
            for word in line.split()[1:]:
                if word not in word_ids:
                    word_ids[word] = len(word_ids)

    embedding = nn.Embedding(len(word_ids), word2vec.vector_size)
    with torch.no_grad():
        for word, id in word_ids.items():
            if not word:
                continue
            try:
                embedding.weight[id] = torch.tensor(word2vec[word])
            except KeyError:
                pass

    torch.save((word_ids, embedding), model)

embedding.weight.requires_grad = False


class AppDataset(Dataset):
    def __init__(self, file: str):
        self.texts = []
        self.sentiments = []
        for line in Path(file).read_text().splitlines():
            sentiment, text = line.split("\t", 1)
            self.texts.append(
                torch.tensor(
                    [word_ids[word] for word in text.split()], device=device
                )
            )
            self.sentiments.append(int(sentiment))

    def __len__(self):
        return len(self.sentiments)

    def __getitem__(self, index):
        return {
            "text": self.texts[index],
            "label": self.sentiments[index],
        }


def collate_fn(batch):
    return {
        "text": pad_sequence(
            [item["text"] for item in batch], batch_first=True
        ),
        "label": torch.tensor(
            [item["label"] for item in batch], device=device
        ),
    }


def get_dataloader(name: str, batch_size: int = 64, **kwargs):
    return DataLoader(
        AppDataset(data_dir / name),
        batch_size=batch_size,
        collate_fn=collate_fn,
        **kwargs,
    )
