from dataset import embedding


def create_model(config):
    model_type = config.pop("type")
    if model_type == "cnn":
        from model.cnn import CNN

        return CNN(
            embedding,
            output_dim=2,
            **config,
        )
    elif model_type == "rnn":
        from model.rnn import RNN

        return RNN(
            embedding,
            output_dim=2,
            **config,
        )
    elif model_type == "mlp":
        from model.mlp import MLP

        return MLP(
            embedding,
            output_dim=2,
            **config,
        )
    else:
        raise ValueError(f"Unknown model type: {config['type']}")
