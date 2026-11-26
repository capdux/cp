// cap/test/testCapNdjson.cp
// Comprehensive unit tests for @cap/ndjson (NDJSON / JSON Lines streaming codec)

import { test, expect_eq } from "cap:test";
import {
    MAX_LINE_BYTES,
    NdjsonLine,
    NdjsonSplitter,
    nextLine,
    countLines,
    writeLine,
} from "../../packages/@cap/ndjson/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ndjson — mixed LF/CRLF and blank line skipping", () => {
        const doc = "\n  \t \r\n{\"a\":1}\r\n\n{\"b\":2}\n   \n{\"c\":3}";
        let pos: usize = 0;

        const l1: NdjsonLine = nextLine(doc, pos, &pos);
        expect_eq(l1.found, true);
        expect_eq(sliceEq(doc[l1.start..(l1.start + l1.len)], "{\"a\":1}"), true);

        const l2: NdjsonLine = nextLine(doc, pos, &pos);
        expect_eq(l2.found, true);
        expect_eq(sliceEq(doc[l2.start..(l2.start + l2.len)], "{\"b\":2}"), true);

        const l3: NdjsonLine = nextLine(doc, pos, &pos);
        expect_eq(l3.found, true);
        expect_eq(sliceEq(doc[l3.start..(l3.start + l3.len)], "{\"c\":3}"), true);

        const l4: NdjsonLine = nextLine(doc, pos, &pos);
        expect_eq(l4.found, false);
    });

    test("2. @cap/ndjson — countLines accurate non-empty record count", () => {
        expect_eq(countLines(""), 0 as usize);
        expect_eq(countLines("   \n\r\n\t\n"), 0 as usize);
        expect_eq(countLines("{\"k\":1}\n\n{\"k\":2}\r\n{\"k\":3}"), 3 as usize);
    });

    test("3. @cap/ndjson — NdjsonSplitter incremental chunked byte feeding", () => {
        let sp: NdjsonSplitter = NdjsonSplitter.init();
        const stream = "{\"id\":1}\r\n\n{\"id\":2}\n";
        let records: usize = 0;

        let i: usize = 0;
        while (i < stream.len) {
            const r: i64 = sp.pushByte(stream[i]);
            expect_eq(r >= 0, true);
            if (r > 0) {
                records += 1;
                if (records == 1) {
                    expect_eq(sliceEq(sp.currentLine(), "{\"id\":1}"), true);
                } else if (records == 2) {
                    expect_eq(sliceEq(sp.currentLine(), "{\"id\":2}"), true);
                }
            }
            i += 1;
        }
        expect_eq(records, 2 as usize);
    });

    test("4. @cap/ndjson — NdjsonSplitter flush emits trailing record without newline", () => {
        let sp: NdjsonSplitter = NdjsonSplitter.init();
        const tail = "{\"eof\":true}";
        let i: usize = 0;
        while (i < tail.len) {
            expect_eq(sp.pushByte(tail[i]), 0 as i64);
            i += 1;
        }
        const flushed: i64 = sp.flush();
        expect_eq(flushed, tail.len as i64);
        expect_eq(sliceEq(sp.currentLine(), tail), true);
        // Second flush returns 0
        expect_eq(sp.flush(), 0 as i64);
    });

    test("5. @cap/ndjson — NdjsonSplitter rejects line exceeding 1024 bytes with -1", () => {
        let sp: NdjsonSplitter = NdjsonSplitter.init();
        let i: usize = 0;
        while (i < MAX_LINE_BYTES) {
            expect_eq(sp.pushByte('x' as u8), 0 as i64);
            i += 1;
        }
        // 1025th byte triggers overflow (-1)
        expect_eq(sp.pushByte('y' as u8), -1 as i64);
        // Newline resets overflow state cleanly for subsequent valid line
        expect_eq(sp.pushByte('\n' as u8), 0 as i64);

        const ok_line = "{\"ok\":1}\n";
        let got: i64 = 0;
        i = 0;
        while (i < ok_line.len) {
            got = sp.pushByte(ok_line[i]);
            i += 1;
        }
        expect_eq(got, 8 as i64);
        expect_eq(sliceEq(sp.currentLine(), "{\"ok\":1}"), true);
    });

    test("6. @cap/ndjson — writeLine appends LF & rejects raw embedded newlines or tiny buffer", () => {
        let dst: [64]u8 = undefined;
        const n: i64 = writeLine("{\"a\":1}", dst[0..64]);
        expect_eq(n, 8 as i64);
        expect_eq(sliceEq(dst[0..8], "{\"a\":1}\n"), true);

        // Reject embedded LF/CR and empty record
        expect_eq(writeLine("{\"a\":1}\n{\"b\":2}", dst[0..64]), -1 as i64);
        expect_eq(writeLine("{\"a\":1}\r", dst[0..64]), -1 as i64);
        expect_eq(writeLine("", dst[0..64]), -1 as i64);

        // Reject tiny buffer
        let tiny: [4]u8 = undefined;
        expect_eq(writeLine("{\"a\":1}", tiny[0..4]), -1 as i64);
    });
}
