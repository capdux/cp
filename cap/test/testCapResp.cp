// cap/test/testCapResp.cp
// Comprehensive unit tests for @cap/resp (Redis RESP2 wire protocol)

import { test, expect_eq } from "cap:test";
import {
    RESP_INCOMPLETE,
    RESP_ERROR_SYNTAX,
    RESP_SIMPLE_STRING,
    RESP_ERR,
    RESP_INTEGER,
    RESP_BULK_STRING,
    RESP_NULL,
    RESP_ARRAY,
    RespFrame,
    parseFrame,
    encodeCommand,
    encodeSimpleString,
    encodeBulkString,
    encodeInteger,
    encodeNull,
} from "../../packages/@cap/resp/src/index.cp";

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
    test("1. @cap/resp — Simple String, Error & positive/negative Integer parsing", () => {
        const s1 = "+OK\r\n";
        const f1: RespFrame = parseFrame(s1);
        expect_eq(f1.kind, RESP_SIMPLE_STRING);
        expect_eq(f1.consumed, 5 as usize);
        expect_eq(sliceEq(s1[f1.data_start..(f1.data_start + f1.data_len)], "OK"), true);

        const s2 = "-ERR bad\r\n";
        const f2: RespFrame = parseFrame(s2);
        expect_eq(f2.kind, RESP_ERR);
        expect_eq(f2.consumed, 10 as usize);
        expect_eq(sliceEq(s2[f2.data_start..(f2.data_start + f2.data_len)], "ERR bad"), true);

        const f3: RespFrame = parseFrame(":1000\r\n");
        expect_eq(f3.kind, RESP_INTEGER);
        expect_eq(f3.int_val, 1000 as i64);

        const f4: RespFrame = parseFrame(":-42\r\n");
        expect_eq(f4.kind, RESP_INTEGER);
        expect_eq(f4.int_val, -42 as i64);
    });

    test("2. @cap/resp — binary-safe Bulk String with embedded CRLF & Null $-1\\r\\n", () => {
        const raw = "$7\r\nhi\r\nbye\r\n";
        const f: RespFrame = parseFrame(raw);
        expect_eq(f.kind, RESP_BULK_STRING);
        expect_eq(f.consumed, raw.len);
        expect_eq(sliceEq(raw[f.data_start..(f.data_start + f.data_len)], "hi\r\nbye"), true);

        const fnul: RespFrame = parseFrame("$-1\r\n");
        expect_eq(fnul.kind, RESP_NULL);
        expect_eq(fnul.consumed, 5 as usize);

        let dst: [32]u8 = undefined;
        const nn: i64 = encodeNull(dst[0..32]);
        expect_eq(sliceEq(dst[0..(nn as usize)], "$-1\r\n"), true);
    });

    test("3. @cap/resp — encodeCommand builds exact RESP2 SET command & parses back", () => {
        const args: [3][]const u8 = ["SET", "k", "hello"];
        let buf: [128]u8 = undefined;
        const n: i64 = encodeCommand(args[0..3], buf[0..128]);
        expect_eq(n > 0, true);
        const pkt: []const u8 = buf[0..(n as usize)];
        expect_eq(sliceEq(pkt, "*3\r\n$3\r\nSET\r\n$1\r\nk\r\n$5\r\nhello\r\n"), true);

        const arr: RespFrame = parseFrame(pkt);
        expect_eq(arr.kind, RESP_ARRAY);
        expect_eq(arr.array_len, 3 as i64);

        const a0: RespFrame = parseFrame(pkt[arr.consumed..pkt.len]);
        expect_eq(a0.kind, RESP_BULK_STRING);
        const off1: usize = arr.consumed;
        expect_eq(sliceEq(pkt[(off1 + a0.data_start)..(off1 + a0.data_start + a0.data_len)], "SET"), true);
    });

    test("4. @cap/resp — partial / truncated frames return RESP_INCOMPLETE", () => {
        expect_eq(parseFrame("").kind, RESP_INCOMPLETE);
        expect_eq(parseFrame("+OK\r").kind, RESP_INCOMPLETE);
        expect_eq(parseFrame(":123").kind, RESP_INCOMPLETE);
        expect_eq(parseFrame("$5\r\nhel").kind, RESP_INCOMPLETE);
        expect_eq(parseFrame("$5\r\nhello\r").kind, RESP_INCOMPLETE);
        expect_eq(parseFrame("$5\r\nhello\r").consumed, 0 as usize);
    });

    test("5. @cap/resp — invalid prefixes & negative lengths return RESP_ERROR_SYNTAX", () => {
        expect_eq(parseFrame("?weird\r\n").kind, RESP_ERROR_SYNTAX);
        expect_eq(parseFrame(":abc\r\n").kind, RESP_ERROR_SYNTAX);
        expect_eq(parseFrame("$-2\r\n").kind, RESP_ERROR_SYNTAX);
        expect_eq(parseFrame("*-5\r\n").kind, RESP_ERROR_SYNTAX);
        // Bulk string missing trailing CRLF
        expect_eq(parseFrame("$3\r\nabcXX").kind, RESP_ERROR_SYNTAX);
    });

    test("6. @cap/resp — encoder helpers & tiny buffer overflow returns -1", () => {
        let buf: [64]u8 = undefined;
        const ns: i64 = encodeSimpleString("PONG", buf[0..64]);
        expect_eq(sliceEq(buf[0..(ns as usize)], "+PONG\r\n"), true);
        expect_eq(encodeSimpleString("bad\r\n", buf[0..64]), -1 as i64);

        const ni: i64 = encodeInteger(-99, buf[0..64]);
        expect_eq(sliceEq(buf[0..(ni as usize)], ":-99\r\n"), true);

        const nb: i64 = encodeBulkString("xy", buf[0..64]);
        expect_eq(sliceEq(buf[0..(nb as usize)], "$2\r\nxy\r\n"), true);

        let tiny: [4]u8 = undefined;
        expect_eq(encodeNull(tiny[0..4]), -1 as i64);
        const args: [1][]const u8 = ["PING"];
        expect_eq(encodeCommand(args[0..1], tiny[0..4]), -1 as i64);
    });
}
