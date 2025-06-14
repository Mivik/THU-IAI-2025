import argparse
import torch

from pathlib import Path

from sklearn.metrics import f1_score, accuracy_score, log_loss

from config import load_config
from dataset import get_dataloader, device


def eval_model(model, dl_valid):
    model.eval()
    with torch.no_grad():
        preds = []
        answer = []
        for batch in dl_valid:
            output = model(batch["text"])
            preds.extend(output.argmax(dim=1).tolist())
            answer.extend(batch["label"].tolist())
        f1 = f1_score(answer, preds, average="macro")
        acc = accuracy_score(answer, preds)
        loss = log_loss(answer, preds)
        return {"f1": f1, "acc": acc, "loss": loss}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "config",
        type=str,
        help="Path to the model config",
    )
    parser.add_argument(
        "-i",
        "--input",
        type=str,
        default="checkpoints",
        help="Path to the checkpoints directory",
    )
    args = parser.parse_args()

    dl_valid = get_dataloader("validation.txt")

    model, train = load_config(args.config)
    model = model.to(device)

    model_name = Path(args.config).stem
    ckpt_dir = Path(args.input) / model_name

    print("f1,acc")
    for epoch in range(train.epoch):
        model.load_state_dict(
            torch.load(ckpt_dir / f"{epoch:02}.pt", weights_only=True)
        )
        result = eval_model(model, dl_valid)
        print(f"{result['f1']},{result['acc']}")
