// Regression test for task-bun-20261122-builtin-regexp-collision:
// Top-level user-defined `pub struct RegExp` (including self-referential fields/consts/methods)
// must not collide with the built-in `pub const RegExp = struct { ... }` prelude.

import { expect_eq } from "cap:test";

pub struct RegExp {
    value: string = "",
    flags_offset: u16 = 0,

    pub const empty: RegExp = RegExp {
        value: "",
        flags_offset: 0,
    };

    pub fn get_offset(): u16 {
        return _.flags_offset;
    }
}

fn make_regexp(val: string, off: u16): RegExp {
    return RegExp {
        value: val,
        flags_offset: off,
    };
}

fn main() {
    let e: RegExp = RegExp.empty;
    expect_eq(e.flags_offset, 0);
    expect_eq(e.get_offset(), 0);

    let r = make_regexp("/abc/gi", 4);
    expect_eq(r.flags_offset, 4);
    expect_eq(r.get_offset(), 4);
}
