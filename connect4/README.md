
# connect4

四子棋 AI Rust 实现

由于线上平台限制，在提交前需要使用 `cargo vendor` 并手动覆写 dependencies

具体而言，初次编译或添加新的依赖时，你需要将 `[patch.crates-io]` 下面的部分临时注释，并使用 `cargo vendor` 对依赖创建副本，然后取消注释。
