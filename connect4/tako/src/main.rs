use std::io::{self, BufRead, Write};

use tako::game::{Game, State};

fn main() -> io::Result<()> {
    let mut buffer = String::new();
    let mut stdin = io::stdin().lock();
    let mut stdout = io::stdout().lock();

    stdin.read_line(&mut buffer)?;

    let config: Vec<u8> = buffer
        .trim()
        .split(' ')
        .map(|s| s.parse().unwrap())
        .collect();
    let game = Game::new(config[0], config[1], (config[2], config[3]));
    let mut top = vec![0; game.col as usize];
    let mut board = vec![0; game.row as usize * game.col as usize];
    let mut state = State::new(&game, &mut top, &mut board);

    let mut first = true;
    loop {
        buffer.clear();
        stdin.read_line(&mut buffer)?;
        let mut parts = buffer.trim().split(' ');
        let last_x = parts.next().unwrap();
        let last_y = parts.next().unwrap();
        let mut message = parts.map(|s| s.parse().unwrap());
        if first {
            for dst in state.top.iter_mut() {
                *dst = message.next().unwrap();
            }
            for dst in state.board.iter_mut() {
                *dst = message.next().unwrap();
            }
            first = false;
            state.init();
        } else {
            state.perform(last_x.parse().unwrap(), last_y.parse().unwrap());
            for dst in state.top.iter() {
                assert_eq!(*dst, message.next().unwrap());
            }
            for dst in state.board.iter() {
                assert_eq!(*dst, message.next().unwrap());
            }
        }
        let (x, y) = state.get_move();
        state.perform(x, y);

        let msg = format!("{x} {y}");
        let len = msg.len() as u32;
        stdout.write_all(&len.to_be_bytes())?;
        stdout.write_all(msg.as_bytes())?;
        stdout.flush()?;
    }
}
