use std::slice;

use crate::game::{Game, State};

#[repr(C)]
struct Point {
    x: i32,
    y: i32,
}

#[no_mangle]
extern "C" fn getPoint(
    row: i32,
    col: i32,
    top_ptr: *const i32,
    board_ptr: *const i32,
    _last_x: i32,
    _last_y: i32,
    no_x: i32,
    no_y: i32,
) -> *mut Point {
    let game = Game::new(row as _, col as _, (no_x as _, no_y as _));
    let mut top = Vec::with_capacity(col as usize);
    let mut board = Vec::with_capacity((row * col) as usize);
    unsafe {
        top.extend(
            slice::from_raw_parts(top_ptr, col as usize)
                .iter()
                .map(|&x| x as u8),
        );
        board.extend(
            slice::from_raw_parts(board_ptr, (row * col) as usize)
                .iter()
                .map(|&x| x as u8),
        );
    }

    use std::io::Write;
    let mut file = std::fs::OpenOptions::new().append(true).create(true).truncate(false).write(true).open("last_input.txt").unwrap();
    writeln!(file, "{row} {col} {no_x} {no_y}").unwrap();
    write!(file, "{_last_x} {_last_y} ").unwrap();
    for &x in &top {
        write!(file, "{x} ").unwrap();
    }
    for &x in &board {
        write!(file, "{x} ").unwrap();
    }
    writeln!(file).unwrap();
    drop(file);

    let mut state = State::new(&game, &mut top, &mut board);
    state.init();
    let (x, y) = state.get_move();

    Box::leak(Box::new(Point {
        x: x as _,
        y: y as _,
    })) as *mut Point
}

#[no_mangle]
extern "C" fn clearPoint(point: *mut Point) {
    let _ = unsafe { Box::from_raw(point) };
}
