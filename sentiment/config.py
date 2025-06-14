import toml

from pydantic import BaseModel

from model import create_model


class TrainConfig(BaseModel):
    init: str = "xavier"

    epoch: int = 30
    lr: float = 1e-3

    patience: int = 5
    loss_delta: float = 0.01


def load_config(config_path: str) -> tuple[dict, TrainConfig]:
    config = toml.load(config_path)
    model = create_model(config["model"])
    return model, TrainConfig.model_validate(config.get("train", {}))
