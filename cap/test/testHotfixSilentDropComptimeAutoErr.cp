// Regression test for task-bun-20261123-silent-drop-comptime-auto-dotted-err
// Ensures top-level functions named `print`, `spawn`, `join` (including ones with
// comptime + auto params and dotted error-union return types) are NOT silently dropped.

import { test, expect_eq } from "cap:test";

pub const Encoding = enum {
    utf8,
    utf16,
};

pub struct DummyWriter {
    pub written: i32 = 0,

    pub fn writeByte(*_, b: u8): std.Io.Writer.Error!void {
        * = b;
        _.written += 1;
    }
}

pub fn print(comptime encoding: Encoding, writer: auto): std.Io.Writer.Error!void {
    if (comptime encoding == .utf8) {
        writer.writeByte(65)?;
    } else {
        writer.writeByte(66)?;
        writer.writeByte(67)?;
    }
}

pub fn spawn(comptime tag: Encoding, seed: auto): i32 {
    if (comptime tag == .utf8) {
        return seed + 1;
    }
    return seed + 2;
}

pub fn join(comptime tag: Encoding, a: auto, b: auto): i32 {
    if (comptime tag == .utf8) {
        return a + b;
    }
    return a * b;
}

fn runPrintTest(): std.Io.Writer.Error!i32 {
    let w = DummyWriter{};
    print(.utf8, *w)?;
    print(.utf16, *w)?;
    return w.written;
}

fn main() {
    test("top-level print with comptime + auto + dotted error union is emitted and callable", () => {
        let count = runPrintTest() catch 0;
        expect_eq(count, 3);
    });

    test("top-level user spawn and join are not silently dropped", () => {
        expect_eq(spawn(.utf8, 10), 11);
        expect_eq(spawn(.utf16, 10), 12);
        expect_eq(join(.utf8, 3, 4), 7);
        expect_eq(join(.utf16, 3, 4), 12);
    });
}
