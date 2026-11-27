// cap/test/testCapMinipass.cp
// Comprehensive unit tests for @cap/minipass

import { test, expect_eq } from "cap:test";
import {
    MPASS_CAP,
    MinipassStream,
} from "../../packages/minipass/src/index.cp";

fn main() {
    test("1. @cap/minipass — write, read, bufferedBytes & freeSpace", () => {
        let s = MinipassStream.new();
        expect_eq(s.bufferedBytes(), 0 as usize);
        expect_eq(s.freeSpace(), MPASS_CAP);

        expect_eq(s.write("hello "), true);
        expect_eq(s.write("world"), true);
        expect_eq(s.bufferedBytes(), 11 as usize);
        expect_eq(s.freeSpace(), MPASS_CAP - 11);

        let out: [16]u8 = undefined;
        const n = s.read(out[0..5]);
        expect_eq(n, 5 as usize);
        expect_eq(std.mem.eql(u8, out[0..5], "hello"), true);
        expect_eq(s.bufferedBytes(), 6 as usize);

        const n2 = s.read(out[0..16]);
        expect_eq(n2, 6 as usize);
        expect_eq(std.mem.eql(u8, out[0..6], " world"), true);
        expect_eq(s.bufferedBytes(), 0 as usize);
    });

    test("2. @cap/minipass — peek & readExact atomicity", () => {
        let s = MinipassStream.new();
        expect_eq(s.write("abcdef"), true);

        let pbuf: [4]u8 = undefined;
        const pn = s.peek(pbuf[0..4]);
        expect_eq(pn, 4 as usize);
        expect_eq(std.mem.eql(u8, pbuf[0..4], "abcd"), true);
        expect_eq(s.bufferedBytes(), 6 as usize);

        let ebuf: [8]u8 = undefined;
        // Requesting more than buffered returns 0 without consuming
        expect_eq(s.readExact(7, ebuf[0..8]), 0 as usize);
        expect_eq(s.bufferedBytes(), 6 as usize);

        expect_eq(s.readExact(4, ebuf[0..8]), 4 as usize);
        expect_eq(std.mem.eql(u8, ebuf[0..4], "abcd"), true);
        expect_eq(s.bufferedBytes(), 2 as usize);
    });

    test("3. @cap/minipass — ring buffer wrap-around integrity", () => {
        let s = MinipassStream.new();
        let chunk: [1024]u8 = undefined;
        let i: usize = 0;
        while (i < 1024) : (i += 1) {
            chunk[i] = u8('A' + (i % 26));
        }

        // Write 3072 bytes, read 2048 bytes (head = 2048), then write 2048 bytes (wraps past 4096)
        expect_eq(s.write(chunk[0..1024]), true);
        expect_eq(s.write(chunk[0..1024]), true);
        expect_eq(s.write(chunk[0..1024]), true);
        expect_eq(s.bufferedBytes(), 3072 as usize);

        let drain: [2048]u8 = undefined;
        expect_eq(s.read(drain[0..2048]), 2048 as usize);
        expect_eq(s.bufferedBytes(), 1024 as usize);

        expect_eq(s.write(chunk[0..1024]), true);
        expect_eq(s.write(chunk[0..1024]), true);
        expect_eq(s.bufferedBytes(), 3072 as usize);

        // Verify wrapped data matches
        let verify: [1024]u8 = undefined;
        expect_eq(s.read(verify[0..1024]), 1024 as usize);
        expect_eq(std.mem.eql(u8, verify[0..1024], chunk[0..1024]), true);
        expect_eq(s.read(verify[0..1024]), 1024 as usize);
        expect_eq(std.mem.eql(u8, verify[0..1024], chunk[0..1024]), true);
        expect_eq(s.read(verify[0..1024]), 1024 as usize);
        expect_eq(std.mem.eql(u8, verify[0..1024], chunk[0..1024]), true);
        expect_eq(s.bufferedBytes(), 0 as usize);
    });

    test("4. @cap/minipass — backpressure when buffer is full", () => {
        let s = MinipassStream.new();
        let block: [2048]u8 = undefined;
        let i: usize = 0;
        while (i < 2048) : (i += 1) {
            block[i] = u8('x');
        }
        expect_eq(s.write(block[0..2048]), true);
        expect_eq(s.write(block[0..2048]), true);
        expect_eq(s.freeSpace(), 0 as usize);

        // Buffer is full: further write returns false (backpressure)
        expect_eq(s.write("1"), false);
        expect_eq(s.bufferedBytes(), MPASS_CAP);
    });

    test("5. @cap/minipass — pause, resume, end & isDrained lifecycle", () => {
        let s = MinipassStream.new();
        expect_eq(s.isPaused(), true);
        s.@"resume"();
        expect_eq(s.isPaused(), false);
        s.pause();
        expect_eq(s.isPaused(), true);

        expect_eq(s.write("data"), true);
        s.end();
        expect_eq(s.isEnded(), true);
        expect_eq(s.isDrained(), false);
        expect_eq(s.write("more"), false);

        let buf: [8]u8 = undefined;
        expect_eq(s.read(buf[0..8]), 4 as usize);
        expect_eq(s.isDrained(), true);
    });

    test("6. @cap/minipass — pipeTo transfers data and propagates EOF", () => {
        let src = MinipassStream.new();
        let dst = MinipassStream.new();

        expect_eq(src.write("stream-payload"), true);
        src.end();

        const moved = src.pipeTo(*dst);
        expect_eq(moved, 14 as usize);
        expect_eq(src.isDrained(), true);
        expect_eq(dst.isEnded(), true);
        expect_eq(dst.bufferedBytes(), 14 as usize);

        let out: [32]u8 = undefined;
        const n = dst.read(out[0..32]);
        expect_eq(n, 14 as usize);
        expect_eq(std.mem.eql(u8, out[0..14], "stream-payload"), true);
        expect_eq(dst.isDrained(), true);
    });
}
