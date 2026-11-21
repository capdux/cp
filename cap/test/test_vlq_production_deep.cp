// Source: cp/cap/test/test_vlq_production_deep.cp (Production deep tests for cap:vlq)

import { test, expect_eq } from "cap:test";
import {
    VLQ,
    VLQResult,
    ZERO,
    MAX_VLQ_BYTES,
    encode,
    encodeSlowPath,
    encodeInto,
    encodeSliceInto,
    decode,
    decodeChecked,
    decodeSegment,
} from "cap:vlq";

test("0..255 fast lookup table matches slow path and roundtrips exactly") {
    expect_eq(MAX_VLQ_BYTES, @as(usize, 7));
    expect_eq(ZERO.slice(), "A");

    // 标准 SourceMap VLQ 规范向量验证
    expect_eq(encode(0).slice(), "A");
    expect_eq(encode(1).slice(), "C");
    expect_eq(encode(-1).slice(), "D");
    expect_eq(encode(15).slice(), "e");
    expect_eq(encode(-15).slice(), "f");
    expect_eq(encode(16).slice(), "gB");
    expect_eq(encode(-16).slice(), "hB");

    let v: i32 = 0;
    while (v <= 255) {
        const fast = encode(v);
        const slow = encodeSlowPath(v);
        expect_eq(fast.slice(), slow.slice());

        const decoded = decodeChecked(fast.slice(), 0) catch unreachable;
        expect_eq(decoded.value, v);
        expect_eq(decoded.next, fast.len as usize);

        if (opt := decode(fast.slice(), 0)) {
            expect_eq(opt.value, v);
            expect_eq(opt.next, fast.len as usize);
        } else {
            expect_eq(true, false);
        }
        v += 1;
    }
}

test("Large positive, negative and i32 extreme boundary roundtrips") {
    const samples: [12]i32 = [
        -1,
        -16,
        -256,
        -1024,
        -1000000,
        -2147483647,
        -2147483648,
        256,
        1024,
        1000000,
        2147483646,
        2147483647,
    ];

    let i: usize = 0;
    while (i < samples.len) {
        const val = samples[i];
        const enc = encode(val);
        expect_eq(enc.len >= 1 && (enc.len as usize) <= MAX_VLQ_BYTES, true);

        const decoded = decodeChecked(enc.slice(), 0) catch unreachable;
        expect_eq(decoded.value, val);
        expect_eq(decoded.next, enc.len as usize);
        i += 1;
    }
}

test("Multi-field SourceMap mapping segment continuous decode") {
    // "AAAA" -> [0, 0, 0, 0]
    let out4: [4]i32 = [0; 4];
    const c1 = decodeSegment("AAAA", out4[0..4]) catch unreachable;
    expect_eq(c1, @as(usize, 4));
    expect_eq(out4[0], @as(i32, 0));
    expect_eq(out4[1], @as(i32, 0));
    expect_eq(out4[2], @as(i32, 0));
    expect_eq(out4[3], @as(i32, 0));

    // "SAAS" -> [9, 0, 0, 9]
    const c2 = decodeSegment("SAAS", out4[0..4]) catch unreachable;
    expect_eq(c2, @as(usize, 4));
    expect_eq(out4[0], @as(i32, 9));
    expect_eq(out4[1], @as(i32, 0));
    expect_eq(out4[2], @as(i32, 0));
    expect_eq(out4[3], @as(i32, 9));

    // 带名称索引的 5 字段段：encodeSliceInto -> decodeSegment 往返
    const fields5: [5]i32 = [12, 0, -3, 18, 5];
    let buf: [32]u8 = [0; 32];
    const written = encodeSliceInto(fields5[0..5], buf[0..32]) catch unreachable;

    let dec5: [5]i32 = [0; 5];
    const c5 = decodeSegment(buf[0..written], dec5[0..5]) catch unreachable;
    expect_eq(c5, @as(usize, 5));
    let j: usize = 0;
    while (j < 5) {
        expect_eq(dec5[j], fields5[j]);
        j += 1;
    }
}

test("Invalid Base64 chars, truncated continuation, out-of-bounds and overflow guards") {
    // 1. 越界起点与空输入 -> OutOfBounds
    const e_empty = switch (decodeChecked("", 0)) {
        err.OutOfBounds => true,
        else => false,
    };
    expect_eq(e_empty, true);
    expect_eq(decode("", 0) == null, true);

    const e_oob = switch (decodeChecked("AAAA", 4)) {
        err.OutOfBounds => true,
        else => false,
    };
    expect_eq(e_oob, true);

    // 2. 非法 Base64 字符（标点/空格/非 ASCII 字节）-> InvalidBase64Char
    const e_bang = switch (decodeChecked("!AAA", 0)) {
        err.InvalidBase64Char => true,
        else => false,
    };
    expect_eq(e_bang, true);

    const e_comma = switch (decodeChecked(",A", 0)) {
        err.InvalidBase64Char => true,
        else => false,
    };
    expect_eq(e_comma, true);

    const non_ascii: [2]u8 = [0x80, 'A'];
    const e_non_ascii = switch (decodeChecked(non_ascii[0..2], 0)) {
        err.InvalidBase64Char => true,
        else => false,
    };
    expect_eq(e_non_ascii, true);
    expect_eq(decode(non_ascii[0..2], 0) == null, true);

    // 3. 截断的续位流（'g' = 32，带有 continuation bit 但后续无字节）-> TruncatedInput
    const e_trunc1 = switch (decodeChecked("g", 0)) {
        err.TruncatedInput => true,
        else => false,
    };
    expect_eq(e_trunc1, true);

    const e_trunc2 = switch (decodeChecked("ggg", 0)) {
        err.TruncatedInput => true,
        else => false,
    };
    expect_eq(e_trunc2, true);

    // 4. 超过 32 位上限的超大 VLQ 流（连续 7 个续位字节）-> Overflow
    const e_ovf = switch (decodeChecked("gggggggg", 0)) {
        err.Overflow => true,
        else => false,
    };
    expect_eq(e_ovf, true);
}

test("Buffer writing encodeInto, encodeSliceInto and BufferTooSmall guard") {
    let small: [1]u8 = [0; 1];

    // 单字节值 0 ("A") 可成功写入 1 字节缓冲
    const w1 = encodeInto(0, small[0..1]) catch unreachable;
    expect_eq(w1, @as(usize, 1));
    expect_eq(small[0], @as(u8, 'A'));

    // 双字节值 16 ("gB") 写入 1 字节缓冲必须返回 BufferTooSmall
    const too_small = switch (encodeInto(16, small[0..1])) {
        err.BufferTooSmall => true,
        else => false,
    };
    expect_eq(too_small, true);

    // 多值写入缓冲不足 -> BufferTooSmall
    const vals: [3]i32 = [100, 200, 300];
    let tiny: [3]u8 = [0; 3];
    const slice_too_small = switch (encodeSliceInto(vals[0..3], tiny[0..3])) {
        err.BufferTooSmall => true,
        else => false,
    };
    expect_eq(slice_too_small, true);

    // decodeSegment 目标数组过小 -> BufferTooSmall
    let out2: [2]i32 = [0; 2];
    const dec_too_small = switch (decodeSegment("AAAA", out2[0..2])) {
        err.BufferTooSmall => true,
        else => false,
    };
    expect_eq(dec_too_small, true);
}

test("SourceMap v3 multi-segment mapping line delta accumulation") {
    // 模拟一行 SourceMap 映射：3 个逗号分隔段，每段 4 个增量字段
    const deltas: [3][4]i32 = [
        [0, 0, 0, 0],
        [4, 0, 0, 4],
        [6, 0, 1, -4],
    ];

    let line_buf: [64]u8 = [0; 64];
    let pos: usize = 0;
    let s: usize = 0;
    while (s < 3) {
        if (s > 0) {
            line_buf[pos] = ',';
            pos += 1;
        }
        const n = encodeSliceInto(deltas[s][0..4], line_buf[pos..64]) catch unreachable;
        pos += n;
        s += 1;
    }

    // 解析逗号分隔行并累加恢复绝对坐标
    let gen_col: i32 = 0;
    let src_line: i32 = 0;
    let src_col: i32 = 0;
    let seg_start: usize = 0;
    let idx: usize = 0;
    while (idx <= pos) {
        if (idx == pos || line_buf[idx] == ',') {
            let f: [4]i32 = [0; 4];
            const cnt = decodeSegment(line_buf[seg_start..idx], f[0..4]) catch unreachable;
            expect_eq(cnt, @as(usize, 4));
            gen_col += f[0];
            src_line += f[2];
            src_col += f[3];
            seg_start = idx + 1;
        }
        idx += 1;
    }

    expect_eq(gen_col, @as(i32, 10));
    expect_eq(src_line, @as(i32, 1));
    expect_eq(src_col, @as(i32, 0));
}
