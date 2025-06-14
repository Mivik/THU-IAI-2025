import argparse
import shutil
import torch
import torch.nn.functional as F

from pathlib import Path
from tqdm import tqdm

from torch import nn

from config import load_config
from dataset import get_dataloader, device
from eval import eval_model


parser = argparse.ArgumentParser()
parser.add_argument(
    "config",
    type=str,
    help="Path to the model config",
)
parser.add_argument(
    "-o",
    "--output",
    type=str,
    default="checkpoints",
    help="Path to the output directory",
)
args = parser.parse_args()

dl_train = get_dataloader("train.txt", shuffle=True)
dl_valid = get_dataloader("validation.txt")

model, train = load_config(args.config)
model = model.to(device)

init_fn = {
    "none": lambda x: None,
    "uniform": nn.init.uniform_,
    "normal": nn.init.normal_,
    "xavier": nn.init.xavier_uniform_,
    "kaiming": lambda x: nn.init.kaiming_uniform_(x, nonlinearity="relu"),
}[train.init]

for p in model.parameters():
    if not p.requires_grad:
        continue
    if p.dim() > 1:
        init_fn(p)
    else:
        nn.init.zeros_(p)

optimizer = torch.optim.AdamW(model.parameters(), lr=train.lr)

model_name = Path(args.config).stem
ckpt_dir = Path(args.output) / model_name
if ckpt_dir.exists():
    shutil.rmtree(ckpt_dir)

ckpt_dir.mkdir(parents=True)

strike = 0
min_loss = float("inf")
for epoch in range(train.epoch):
    model.train()
    for batch in tqdm(dl_train):
        optimizer.zero_grad()
        output = model(batch["text"])
        loss = F.cross_entropy(output, batch["label"])
        loss.backward()
        optimizer.step()

    result = eval_model(model, dl_valid)
    summary = ", ".join(f"{k}={v:.4f}" for k, v in result.items())
    print(f"Epoch {epoch}: {summary}")

    torch.save(model.state_dict(), ckpt_dir / f"{epoch:02}.pt")

    if result["loss"] < min_loss:
        strike = 0
        min_loss = result["loss"]
    elif result["loss"] > min_loss + train.loss_delta:
        strike += 1
        if strike >= train.patience:
            print(f"Early stopping at epoch {epoch}")
            break
