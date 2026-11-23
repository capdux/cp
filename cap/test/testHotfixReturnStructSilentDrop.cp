// Regression test for task-bun-20261123-return-struct-silent-drop
// Verifies:
// 1. Multi-line `@as(u16, @truncate(c),)` with trailing comma parses and emits full Zig code
//    inside `return struct { ... }` generic factory methods (no silent statement drop).
// 2. Surrogate-pair-like UTF-16 encoding inside `return struct` executes and produces expected items.

import { test, expect_eq } from "cap:test";

pub fn MakeDecoder(comptime T: type): type {
    return struct {
        pub fn pushPair(out: *std.ArrayList(T), codepoint: u32)! {
            let adjusted = codepoint - 0x10000;
            let hi: u32 = 0xD800 + (adjusted >> 10);
            let lo: u32 = 0xDC00 + (adjusted & 0x3FF);
            out.appendAssumeCapacity(@as(
                T,
                @truncate(hi),
            ));
            out.appendAssumeCapacity(@as(
                T,
                @truncate(lo),
            ));
        }
    };
}

fn runDecodeTest(): !i32 {
    let mut_buf = std.ArrayList(u16).initCapacity(std.heap.page_allocator, 4)?;
    defer mut_buf.deinit(std.heap.page_allocator);

    const Decoder = MakeDecoder(u16);
    let decoder = Decoder{};
    // U+1F600 -> 0xD83D, 0xDE00
    decoder.pushPair(*mut_buf, 0x1F600)?;

    if (mut_buf.items.len != 2) {
        return -1;
    }
    if (mut_buf.items[0] != 0xD83D) {
        return -2;
    }
    if (mut_buf.items[1] != 0xDE00) {
        return -3;
    }
    return 2;
}

fn main() {
    test("return struct method with trailing comma in @as emits and executes full body", () => {
        let n = runDecodeTest() catch -99;
        expect_eq(n, 2);
    });
}
