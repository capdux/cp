import { test, expect_eq } from "cap:test";

pub enum E(u16) {
    a = 0,
    v_shl = 1 << 3,
    pub const b: u16 = 1 << 1;
    pub const c: u16 = 16 >> 2;
}

fn main() {
    test("enum const shift and variant shift", () => {
        expect_eq(E.b, 2);
        expect_eq(E.c, 4);
        expect_eq(E.v_shl as u16, 8);
    });
}
