// Regression test for:
// 1. Calling .name() / .value() methods on structs (including via array indexing & cross-module structs)
//    without triggering enum @tagName/@intFromEnum property lowering
// 2. Nested fixed-size array repeat literals like [[0; 128]; 16]

import { test, expect_eq } from "cap:test";
import std from "std";

enum Color {
    Red,
    Green,
    Blue,
}

struct WorldEntry {
    name_buf: [32]u8 = [0; 32],
    name_len: usize = 0,
    val: i32 = 0,

    fn name(*_): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    fn value(): i32 {
        return _.val;
    }
}

struct SessionBox {
    worlds: [4]WorldEntry = [WorldEntry{}; 4],
    binding_names: [4][16]u8 = [[0; 16]; 4],
}

fn main() {
    test("struct .name()/.value() method call & enum .name property & nested [[0;16];4]", () => {
        let box = SessionBox{};
        const src = "isolated_1";
        @memcpy(box.worlds[1].name_buf[0..src.len], src);
        box.worlds[1].name_len = src.len;
        box.worlds[1].val = 42;

        let idx: usize = 1;
        const got_name: []const u8 = box.worlds[idx].name();
        expect_eq(std.mem.eql(u8, got_name, "isolated_1"), true);
        expect_eq(box.worlds[idx].value(), 42);

        // Enum .name property access still works
        const c = Color.Green;
        expect_eq(std.mem.eql(u8, c.name, "Green"), true);
        // Enum .name() method call also works
        expect_eq(std.mem.eql(u8, c.name(), "Green"), true);

        // Nested array repeat initialized cleanly to zero
        expect_eq(box.binding_names.len, 4);
        expect_eq(box.binding_names[0].len, 16);
        expect_eq(box.binding_names[3][15], 0);
    });
}
