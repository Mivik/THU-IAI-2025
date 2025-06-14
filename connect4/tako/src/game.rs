use std::{iter, mem, ops::Deref, time::Instant};

use arrayvec::ArrayVec;
use ordered_float::{Float, OrderedFloat};
use rand::{rngs::SmallRng, seq::SliceRandom, SeedableRng};
use slab::Slab;

pub const WIN_SCORE: i32 = 20000;

fn parse_board(board: &str) -> Vec<u8> {
    board
        .split_whitespace()
        .map(|it| it.parse().unwrap())
        .collect()
}

pub struct Game {
    pub row: u8,
    pub col: u8,
    pub no: (u8, u8),
}
impl Game {
    pub fn new(row: u8, col: u8, no: (u8, u8)) -> Self {
        Game { row, col, no }
    }
}

pub struct State<'a> {
    pub game: &'a Game,
    pub top: &'a mut [u8],
    pub board: &'a mut [u8],

    who: u8,
    nodes: Slab<Node>,
    root_node: usize,
    rng: SmallRng,
}
impl Deref for State<'_> {
    type Target = Game;

    fn deref(&self) -> &Self::Target {
        &self.game
    }
}

#[derive(Default)]
struct Node {
    pub terminal: bool,
    pub action: (u8, u8),
    pub visits: u32,
    pub value: f32,
    pub children: ArrayVec<usize, 12>,

    pub actions: ArrayVec<u8, 12>,
    pub tried: u8,
}
impl Node {
    pub fn score(&self, parent_visits: u32, c: f32) -> f32 {
        assert!(self.visits > 0);
        1. - self.value / self.visits as f32
            + c * (2.0 * (parent_visits as f32).ln() / self.visits as f32).sqrt()
    }

    pub fn update(&mut self, score: f32) {
        self.value += (score + 1.) / 2.;
        self.visits += 1;
    }
}

impl<'a> State<'a> {
    pub fn new(game: &'a Game, top: &'a mut [u8], board: &'a mut [u8]) -> Self {
        debug_assert_eq!(top.len(), game.col as usize);
        debug_assert_eq!(board.len(), (game.row * game.col) as usize);
        State {
            game,
            top,
            board,

            who: 2,
            nodes: Slab::new(),
            root_node: 0,
            rng: SmallRng::from_seed([0; 32]),
        }
    }
    pub fn get(&self, r: u8, c: u8) -> u8 {
        self.board[(r * self.col + c) as usize]
    }
    pub fn set(&mut self, r: u8, c: u8, v: u8) {
        self.board[(r * self.col + c) as usize] = v;
    }

    pub fn init(&mut self) {
        self.root_node = self.nodes.insert(self.new_node(false, (0, 0)));
    }

    fn remove_node(&mut self, id: usize) {
        for child in mem::take(&mut self.nodes[id].children) {
            self.remove_node(child);
        }
        self.nodes.remove(id);
    }
    pub fn perform(&mut self, r: u8, c: u8) {
        let children = mem::take(&mut self.nodes[self.root_node].children);
        self.nodes.remove(self.root_node);

        let mut found = false;
        for child in children {
            if self.nodes[child].action == (r, c) {
                found = true;
                self.root_node = child;
                continue;
            }
            self.remove_node(child);
        }
        if !found {
            self.nodes.clear();
            self.init();
        }

        self.do_action(c);
    }

    fn in_bounds(&self, r: i8, c: i8) -> bool {
        (0..self.row as i8).contains(&r) && (0..self.col as i8).contains(&c)
    }

    fn check_win_direction<const DX: i8, const DY: i8>(&self, r: u8, c: u8) -> bool {
        let who = self.get(r, c);
        assert_ne!(who, 0);

        let mut count = 0;
        for o in -3..=3 {
            let r = r as i8 + o * DX;
            let c = c as i8 + o * DY;
            if !self.in_bounds(r, c) {
                continue;
            }
            let val = self.get(r as u8, c as u8);
            if val != who {
                count = 0;
            } else {
                count += 1;
                if count == 4 {
                    return true;
                }
            }
        }

        false
    }

    pub fn check_win(&self, r: u8, c: u8) -> bool {
        self.check_win_direction::<1, 0>(r, c)
            || self.check_win_direction::<0, 1>(r, c)
            || self.check_win_direction::<1, 1>(r, c)
            || self.check_win_direction::<1, -1>(r, c)
    }

    fn column_order(&self) -> impl Iterator<Item = u8> {
        let col = self.col;
        let mut l = (col + 1) / 2;
        let mut r = l;
        iter::from_fn(move || {
            if l == 0 && r == col {
                return None;
            }
            if (l ^ r) & 1 == 0 {
                debug_assert!(l >= 1);
                l -= 1;
                Some(l)
            } else {
                debug_assert!(r < col);
                r += 1;
                Some(r - 1)
            }
        })
    }

    fn actions(&self) -> impl Iterator<Item = u8> + '_ {
        self.column_order().filter(|&c| self.top[c as usize] > 0)
    }

    fn new_node(&self, terminal: bool, action: (u8, u8)) -> Node {
        Node {
            terminal,
            action,
            visits: 0,
            value: 0.,
            children: ArrayVec::new(),

            actions: self.actions().collect(),
            tried: 0,
        }
    }

    pub fn get_move(&mut self) -> (u8, u8) {
        self.print();

        let start = Instant::now();
        while start.elapsed().as_secs_f32() < 2.8 {
            self.augment(self.root_node);
        }

        let node = &self.nodes[self.root_node];
        let mut cnt = 0;
        for c in &node.children {
            let child = &self.nodes[*c];
            eprintln!(
                "Action: {:?}, Win Rate: {}",
                child.action,
                child.value as f32 / child.visits as f32,
            );
            cnt += child.visits;
        }
        eprintln!("Count: {cnt}");

        let child = self.best_child(self.root_node, 0.);
        self.nodes[child].action
    }

    fn do_action(&mut self, c: u8) -> u8 {
        let nt = self.top[c as usize] - 1;
        self.top[c as usize] = if nt > 0 && (nt - 1, c) == self.no {
            nt - 1
        } else {
            nt
        };
        self.set(nt, c, self.who);
        self.who = 3 - self.who;
        nt
    }
    fn undo_action(&mut self, c: u8) {
        let mut t = self.top[c as usize];
        if (t, c) == self.no {
            t += 1;
        }
        self.set(t, c, 0);
        self.top[c as usize] = t + 1;
        self.who = 3 - self.who;
    }

    fn augment(&mut self, id: usize) -> f32 {
        // self.print();

        let node = &mut self.nodes[id];
        if node.terminal {
            return -1.0;
        }
        if node.actions.is_empty() {
            return 0.0;
        }

        let score = if let Some(&c) = node.actions.get(node.tried as usize) {
            node.tried += 1;
            let r = self.do_action(c);

            let win = self.check_win(r, c);
            if win {
                for node in mem::take(&mut self.nodes[id].children) {
                    self.remove_node(node);
                }
                let tried = self.nodes[id].tried;
                self.nodes[id].actions.truncate(tried as usize);
            }
            let score = {
                let child = self.nodes.insert(self.new_node(win, (r, c)));
                self.nodes[id].children.push(child);
                let score = if win { -1.0 } else { self.default_policy() };
                self.nodes[child].update(score);
                -score
            };

            self.undo_action(c);

            score
        } else {
            let child = self.best_child(id, (0.5f32).sqrt());
            let (_, c) = self.nodes[child].action;
            self.do_action(c);
            let score = -self.augment(child);
            self.undo_action(c);
            score
        };

        self.nodes[id].update(score);
        score
    }

    fn default_policy(&mut self) -> f32 {
        let mut actions: ArrayVec<u8, 12> = ArrayVec::new();
        let mut stack = vec![];
        let mut score = 1.0;
        loop {
            actions.clear();
            actions.extend(self.actions());
            if actions.is_empty() {
                score = 0.0;
                break;
            }
            let c = *actions.choose(&mut self.rng).unwrap();
            let r = self.do_action(c);
            stack.push(c);
            if self.check_win(r, c) {
                break;
            }
            score = -score;
        }
        for c in stack.into_iter().rev() {
            self.undo_action(c);
        }
        score
    }

    fn best_child(&mut self, id: usize, c: f32) -> usize {
        let node = &self.nodes[id];
        node.children
            .iter()
            .copied()
            .max_by_key(|x| OrderedFloat(self.nodes[*x].score(node.visits, c)))
            .unwrap()
    }

    pub fn print(&self) {
        eprintln!("==========================");
        for r in 0..self.row {
            for c in 0..self.col {
                eprint!("{} ", self.get(r, c));
            }
            eprintln!();
        }
        eprintln!("==========================");
    }
}
