import { test, expect_eq } from "cap:test";
import {
    SPLIT2_BUF_CAP,
    LineSplitter,
    splitLines,
} from "../../packages/@cap/split2/src/index.cp";

fn main() {
    test("1. @cap/split2 — basic LF & CRLF chunk splitting", () => {
        let sp: LineSplitter = LineSplitter.new();
        let out_buf: [128]u8 = [0; 128];
        let starts: [8]usize = [0; 8];
        let lens: [8]usize = [0; 8];

        const chunk: string = "alpha\nbeta\r\ngamma\n";
        const n: usize = sp.pushChunk(chunk, out_buf[0..128], starts[0..8], lens[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(out_buf[starts[0]..starts[0] + lens[0]], "alpha");
        expect_eq(out_buf[starts[1]..starts[1] + lens[1]], "beta");
        expect_eq(out_buf[starts[2]..starts[2] + lens[2]], "gamma");
        expect_eq(sp.pendingLen(), 0 as usize);
        expect_eq(sp.overflowed(), false);
        expect_eq(SPLIT2_BUF_CAP, 4096 as usize);
    });

    test("2. @cap/split2 — partial lines & split CRLF across chunk boundaries", () => {
        let sp: LineSplitter = LineSplitter.new();
        let out_buf: [128]u8 = [0; 128];
        let starts: [8]usize = [0; 8];
        let lens: [8]usize = [0; 8];

        // Chunk 1 ends mid-line with '\r'
        const n1: usize = sp.pushChunk("hel", out_buf[0..128], starts[0..8], lens[0..8]);
        expect_eq(n1, 0 as usize);
        expect_eq(sp.pendingLen(), 3 as usize);

        const n2: usize = sp.pushChunk("lo\r", out_buf[0..128], starts[0..8], lens[0..8]);
        expect_eq(n2, 0 as usize);
        expect_eq(sp.pendingLen(), 6 as usize);

        // Chunk 3 starts with '\n', completing CRLF across boundary
        const n3: usize = sp.pushChunk("\nworld", out_buf[0..128], starts[0..8], lens[0..8]);
        expect_eq(n3, 1 as usize);
        expect_eq(out_buf[starts[0]..starts[0] + lens[0]], "hello");
        expect_eq(sp.pendingLen(), 5 as usize);

        // Flush remaining unterminated line
        let tail: [32]u8 = [0; 32];
        const flen: usize = sp.flush(tail[0..32]);
        expect_eq(flen, 5 as usize);
        expect_eq(tail[0..flen], "world");
        expect_eq(sp.pendingLen(), 0 as usize);
        expect_eq(sp.flush(tail[0..32]), 0 as usize);
    });

    test("3. @cap/split2 — skip_empty filters empty lines", () => {
        let sp: LineSplitter = LineSplitter.new();
        sp.skip_empty = true;

        let out_buf: [64]u8 = [0; 64];
        let starts: [8]usize = [0; 8];
        let lens: [8]usize = [0; 8];

        const n: usize = sp.pushChunk("\n\r\nfoo\n\nbar\r\n\n", out_buf[0..64], starts[0..8], lens[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(out_buf[starts[0]..starts[0] + lens[0]], "foo");
        expect_eq(out_buf[starts[1]..starts[1] + lens[1]], "bar");
    });

    test("4. @cap/split2 — withMaxLen overflow detection & reset", () => {
        let sp: LineSplitter = LineSplitter.withMaxLen(4);
        let out_buf: [64]u8 = [0; 64];
        let starts: [4]usize = [0; 4];
        let lens: [4]usize = [0; 4];

        const n: usize = sp.pushChunk("abcdef\nok\n", out_buf[0..64], starts[0..4], lens[0..4]);
        expect_eq(sp.overflowed(), true);
        expect_eq(n, 2 as usize);
        expect_eq(out_buf[starts[0]..starts[0] + lens[0]], "abcd");
        expect_eq(out_buf[starts[1]..starts[1] + lens[1]], "ok");

        sp.reset();
        expect_eq(sp.overflowed(), false);
        expect_eq(sp.pendingLen(), 0 as usize);
    });

    test("5. @cap/split2 — splitLines stateless helper (LF, CRLF, trailing newline)", () => {
        let slices: [8]string = [""; 8];

        const n1: usize = splitLines("one\ntwo\r\nthree", slices[0..8]);
        expect_eq(n1, 3 as usize);
        expect_eq(slices[0], "one");
        expect_eq(slices[1], "two");
        expect_eq(slices[2], "three");

        const n2: usize = splitLines("a\r\nb\r\n", slices[0..8]);
        expect_eq(n2, 2 as usize);
        expect_eq(slices[0], "a");
        expect_eq(slices[1], "b");

        expect_eq(splitLines("", slices[0..8]), 0 as usize);
    });

    test("6. @cap/split2 — splitLines preserves interior empty lines & respects slice cap", () => {
        let slices: [2]string = [""; 2];
        const n: usize = splitLines("first\n\nthird", slices[0..2]);
        expect_eq(n, 2 as usize);
        expect_eq(slices[0], "first");
        expect_eq(slices[1], "");

        let all: [4]string = [""; 4];
        const n_all: usize = splitLines("first\n\nthird", all[0..4]);
        expect_eq(n_all, 3 as usize);
        expect_eq(all[0], "first");
        expect_eq(all[1], "");
        expect_eq(all[2], "third");
    });
}
