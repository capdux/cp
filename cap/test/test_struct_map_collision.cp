// Regression test for task-bun-20261122-struct-map-collision:
// Top-level `pub struct Map` / `pub struct Set` must not be silently dropped by codegen.

import { expect_eq } from "cap:test";

pub struct Map {
    map: i32,

    pub fn get_val(): i32 {
        return _.map;
    }
}

pub struct Set {
    count: i32,

    pub fn size(): i32 {
        return _.count;
    }
}

fn main() {
    let m = Map { map: 42 };
    expect_eq(m.map, 42);
    expect_eq(m.get_val(), 42);

    let s = Set { count: 7 };
    expect_eq(s.count, 7);
    expect_eq(s.size(), 7);
}
