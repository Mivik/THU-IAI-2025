import matplotlib.pyplot as plt
import pandas as pd

import torch

from pathlib import Path

from config import load_config
from dataset import get_dataloader, device
from eval import eval_model

dl_valid = get_dataloader("validation.txt")

img_dir = Path("img")
img_dir.mkdir(exist_ok=True)

eval_dir = Path("eval")
eval_dir.mkdir(exist_ok=True)

for config in Path("config").glob("*.toml"):
    print(config)
    model, train = load_config(config)
    model = model.to(device)
    model.eval()

    model_name = config.stem
    ckpt_dir = Path("checkpoints") / model_name
    records = []
    for epoch in range(train.epoch):
        ckpt = ckpt_dir / f"{epoch:02}.pt"
        if not ckpt.exists():
            break
        model.load_state_dict(torch.load(ckpt, weights_only=True))
        result = eval_model(model, dl_valid)
        records.append(result)

    df = pd.DataFrame.from_records(records)
    df.to_csv(eval_dir / f"{model_name}.csv", index=False)

    x = list(range(1, len(df) + 1))

    plt.figure(figsize=(10, 5))
    plt.plot(x, df["f1"], label="F1 Score")
    plt.plot(x, df["acc"], label="Accuracy")
    plt.xticks(x)

    plt.title(config.stem)
    plt.xlabel("Epoch")
    plt.legend(loc="best")
    plt.savefig(img_dir / f"{config.stem}.svg")
    plt.clf()
