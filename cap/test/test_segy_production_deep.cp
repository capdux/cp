// Source: cp/cap/test/test_segy_production_deep.cp (Production deep tests for cap:segy)

import { test, expect_eq } from "cap:test";
import {
    SEGY_TEXT_HEADER_SIZE,
    SEGY_BINARY_HEADER_SIZE,
    SEGY_TRACE_HEADER_SIZE,
    SEGY_MIN_HEADER_SIZE,
    SegySampleFormat,
    segySampleBytes,
    SegyBinaryHeader,
    SegyTraceHeader,
    SegyTrace,
    SegyTraceIter,
    ibmFloatToF64,
    parseSegyHeader,
    readSampleF64,
    applyCoordScalar,
} from "cap:binfmt";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) a - b else b - a;
    return diff <= eps;
}

fn writeU16Be(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

fn writeI16Be(buf: *[]u8, off: usize, v: i16) {
    const u: u16 = if (v < 0) ((v as i32) + 65536) as u16 else v as u16;
    writeU16Be(buf, off, u);
}

fn writeU32Be(buf: *[]u8, off: usize, v: u32) {
    buf.*[off] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 3] = (v & 0xFF) as u8;
}

fn writeI32Be(buf: *[]u8, off: usize, v: i32) {
    const u: u32 = if (v < 0) ((v as i64) + 4294967296) as u32 else v as u32;
    writeU32Be(buf, off, u);
}

fn writeF32Be(buf: *[]u8, off: usize, v: f32) {
    const bits: u32 = @bitCast(v);
    writeU32Be(buf, off, bits);
}

fn fillBinaryHeader(slice: *[]u8, fmt_code: u16, samples_per_trace: u16, num_ext: i16) {
    writeI32Be(slice, 3200, 1001);       // job_id
    writeI32Be(slice, 3204, 42);         // line_number
    writeI32Be(slice, 3208, 7);          // reel_number
    writeI16Be(slice, 3212, 2);          // traces_per_ensemble
    writeU16Be(slice, 3216, 2000);       // sample_interval_us (2 ms)
    writeU16Be(slice, 3220, samples_per_trace);
    writeU16Be(slice, 3224, fmt_code);
    writeI16Be(slice, 3254, 1);          // measurement_system = meters
    writeU16Be(slice, 3500, 0x0100);     // Rev 1
    writeI16Be(slice, 3502, 1);          // fixed_length_flag
    writeI16Be(slice, 3504, num_ext);    // num_ext_text_headers
}

fn fillTraceHeader(
    slice: *[]u8,
    base: usize,
    seq: i32,
    cdp: i32,
    scalar: i16,
    sx: i32,
    sy: i32,
    gx: i32,
    gy: i32,
    nsamp: u16,
) {
    writeI32Be(slice, base + 0, seq);
    writeI32Be(slice, base + 4, seq);
    writeI32Be(slice, base + 8, 500 + seq);
    writeI32Be(slice, base + 20, cdp);
    writeI16Be(slice, base + 28, 1);     // seismic data
    writeI16Be(slice, base + 70, scalar);
    writeI32Be(slice, base + 72, sx);
    writeI32Be(slice, base + 76, sy);
    writeI32Be(slice, base + 80, gx);
    writeI32Be(slice, base + 84, gy);
    writeU16Be(slice, base + 114, nsamp);
    writeU16Be(slice, base + 116, 2000);
}

test("1. IBM 32-bit hex float to IEEE-754 f64 standard vectors and edge cases", () => {
    expect_eq(SEGY_TEXT_HEADER_SIZE, 3200);
    expect_eq(SEGY_BINARY_HEADER_SIZE, 400);
    expect_eq(SEGY_TRACE_HEADER_SIZE, 240);
    expect_eq(SEGY_MIN_HEADER_SIZE, 3600);

    expect_eq(approxEq(ibmFloatToF64(0x00000000), 0.0, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0x80000000), 0.0, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0x41100000), 1.0, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0xC1100000), -1.0, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0xC276A000), -118.625, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0x40800000), 0.5, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0x42100000), 16.0, 1e-12), true);
    expect_eq(approxEq(ibmFloatToF64(0x3F800000), 0.03125, 1e-12), true);

    expect_eq(segySampleBytes(.ibm_f32) ?? 0, 4);
    expect_eq(segySampleBytes(.int32) ?? 0, 4);
    expect_eq(segySampleBytes(.int16) ?? 0, 2);
    expect_eq(segySampleBytes(.ieee_f32) ?? 0, 4);
    expect_eq(segySampleBytes(.ieee_f64) ?? 0, 8);
    expect_eq(segySampleBytes(.int8) ?? 0, 1);
    expect_eq(segySampleBytes(.unknown) == null, true);
});

test("2. Full 3600B binary header + multi-trace SegyTraceIter (IEEE f32 and IBM f32)", () => {
    // 3600B 卷头 + 2 道 * (240B 道头 + 3 * 4B 样本) = 3600 + 504 = 4104B
    let buf_ieee: [4104]u8 = [0; 4104];
    let s_ieee: []u8 = buf_ieee[0..4104];
    fillBinaryHeader(*s_ieee, 5, 3, 0);

    // Trace 1 @ 3600
    fillTraceHeader(*s_ieee, 3600, 1, 101, -100, 51234500, 43210000, 51235500, 43211000, 3);
    writeF32Be(*s_ieee, 3600 + 240 + 0, 1.25);
    writeF32Be(*s_ieee, 3600 + 240 + 4, -2.5);
    writeF32Be(*s_ieee, 3600 + 240 + 8, 3.75);

    // Trace 2 @ 3852
    fillTraceHeader(*s_ieee, 3852, 2, 102, 10, 5000, 6000, 5100, 6100, 3);
    writeF32Be(*s_ieee, 3852 + 240 + 0, 0.0);
    writeF32Be(*s_ieee, 3852 + 240 + 4, 42.5);
    writeF32Be(*s_ieee, 3852 + 240 + 8, -0.125);

    const hdr = parseSegyHeader(buf_ieee[0..4104]) ?? unreachable;
    expect_eq(hdr.job_id, 1001);
    expect_eq(hdr.line_number, 42);
    expect_eq(hdr.reel_number, 7);
    expect_eq(hdr.traces_per_ensemble, 2);
    expect_eq(hdr.sample_interval_us, 2000);
    expect_eq(hdr.samples_per_trace, 3);
    expect_eq(hdr.sample_format == .ieee_f32, true);
    expect_eq(hdr.measurement_system, 1);
    expect_eq(hdr.format_rev, 0x0100);
    expect_eq(hdr.fixed_length_flag, 1);
    expect_eq(hdr.num_ext_text_headers, 0);
    expect_eq(hdr.traces_offset, 3600);

    let it = SegyTraceIter.init(buf_ieee[0..4104], hdr);
    const t1 = it.next() ?? unreachable;
    expect_eq(t1.header.trace_seq_line, 1);
    expect_eq(t1.header.cdp, 101);
    expect_eq(t1.header.coord_scalar, -100);
    expect_eq(approxEq(readSampleF64(t1, hdr.sample_format, 0) ?? 0.0, 1.25, 1e-6), true);
    expect_eq(approxEq(readSampleF64(t1, hdr.sample_format, 1) ?? 0.0, -2.5, 1e-6), true);
    expect_eq(approxEq(readSampleF64(t1, hdr.sample_format, 2) ?? 0.0, 3.75, 1e-6), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.header.trace_seq_line, 2);
    expect_eq(t2.header.cdp, 102);
    expect_eq(approxEq(readSampleF64(t2, hdr.sample_format, 1) ?? 0.0, 42.5, 1e-6), true);
    expect_eq(approxEq(readSampleF64(t2, hdr.sample_format, 2) ?? 0.0, -0.125, 1e-6), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);

    // IBM f32 多道解析测试（同一布局改写 fmt=1 与 IBM 编码位）
    let buf_ibm: [4104]u8 = [0; 4104];
    let s_ibm: []u8 = buf_ibm[0..4104];
    fillBinaryHeader(*s_ibm, 1, 3, 0);
    fillTraceHeader(*s_ibm, 3600, 1, 201, 0, 100, 200, 300, 400, 3);
    writeU32Be(*s_ibm, 3600 + 240 + 0, 0x41100000); // 1.0
    writeU32Be(*s_ibm, 3600 + 240 + 4, 0xC276A000); // -118.625
    writeU32Be(*s_ibm, 3600 + 240 + 8, 0x40800000); // 0.5
    fillTraceHeader(*s_ibm, 3852, 2, 202, 0, 100, 200, 300, 400, 3);
    writeU32Be(*s_ibm, 3852 + 240 + 0, 0x00000000); // 0.0
    writeU32Be(*s_ibm, 3852 + 240 + 4, 0x42100000); // 16.0
    writeU32Be(*s_ibm, 3852 + 240 + 8, 0xC1100000); // -1.0

    const ibm_hdr = parseSegyHeader(buf_ibm[0..4104]) ?? unreachable;
    expect_eq(ibm_hdr.sample_format == .ibm_f32, true);
    let ibm_it = SegyTraceIter.init(buf_ibm[0..4104], ibm_hdr);
    const it1 = ibm_it.next() ?? unreachable;
    expect_eq(approxEq(readSampleF64(it1, .ibm_f32, 0) ?? 0.0, 1.0, 1e-12), true);
    expect_eq(approxEq(readSampleF64(it1, .ibm_f32, 1) ?? 0.0, -118.625, 1e-12), true);
    expect_eq(approxEq(readSampleF64(it1, .ibm_f32, 2) ?? 0.0, 0.5, 1e-12), true);

    const it2 = ibm_it.next() ?? unreachable;
    expect_eq(approxEq(readSampleF64(it2, .ibm_f32, 0) ?? 0.0, 0.0, 1e-12), true);
    expect_eq(approxEq(readSampleF64(it2, .ibm_f32, 1) ?? 0.0, 16.0, 1e-12), true);
    expect_eq(approxEq(readSampleF64(it2, .ibm_f32, 2) ?? 0.0, -1.0, 1e-12), true);
    expect_eq(ibm_it.next() == null, true);
    expect_eq(ibm_it.had_error, false);
});

test("3. applyCoordScalar positive, negative, zero, and int16/int32/int8 sample decoding", () => {
    expect_eq(approxEq(applyCoordScalar(51234500, -100), 512345.0, 1e-9), true);
    expect_eq(approxEq(applyCoordScalar(-12345, -10), -1234.5, 1e-9), true);
    expect_eq(approxEq(applyCoordScalar(5000, 10), 50000.0, 1e-9), true);
    expect_eq(approxEq(applyCoordScalar(4242, 0), 4242.0, 1e-9), true);
    // 边界：scalar = -32768 绝不溢出
    expect_eq(approxEq(applyCoordScalar(32768, -32768), 1.0, 1e-9), true);

    // 验证 int16 / int32 / int8 统一 readSampleF64 解码
    let raw_i16: [4]u8 = [0xFF, 0xFE, 0x00, 0x7F]; // -2, 127 (BE)
    const dummy_th = SegyTraceHeader {
        trace_seq_line: 1,
        trace_seq_file: 1,
        field_record: 1,
        cdp: 1,
        trace_id_code: 1,
        coord_scalar: 0,
        source_x: 0,
        source_y: 0,
        group_x: 0,
        group_y: 0,
        trace_samples: 2,
        trace_interval_us: 1000,
    };
    const tr16 = SegyTrace { header: dummy_th, raw_samples: raw_i16[0..4], is_little_endian: false };
    expect_eq(approxEq(readSampleF64(tr16, .int16, 0) ?? 0.0, -2.0, 1e-9), true);
    expect_eq(approxEq(readSampleF64(tr16, .int16, 1) ?? 0.0, 127.0, 1e-9), true);

    let raw_i8: [2]u8 = [0x80, 0x7F]; // -128, 127
    const tr8 = SegyTrace { header: dummy_th, raw_samples: raw_i8[0..2], is_little_endian: false };
    expect_eq(approxEq(readSampleF64(tr8, .int8, 0) ?? 0.0, -128.0, 1e-9), true);
    expect_eq(approxEq(readSampleF64(tr8, .int8, 1) ?? 0.0, 127.0, 1e-9), true);
});

test("4. Extended 3200B textual header offset calculation (traces_offset = 3600 + ext * 3200)", () => {
    // 3600B 基础卷头 + 1 个 3200B 扩展文本头 + 1 道 (240B + 2*2B int16) = 7044B
    let buf: [7044]u8 = [0; 7044];
    let slice: []u8 = buf[0..7044];
    fillBinaryHeader(*slice, 3, 2, 1); // int16, 2 samples, 1 ext text header

    // 道位于 3600 + 3200 = 6800
    fillTraceHeader(*slice, 6800, 9, 309, -10, 1000, 2000, 1100, 2100, 2);
    writeI16Be(*slice, 6800 + 240 + 0, -32768);
    writeI16Be(*slice, 6800 + 240 + 2, 32767);

    const hdr = parseSegyHeader(buf[0..7044]) ?? unreachable;
    expect_eq(hdr.num_ext_text_headers, 1);
    expect_eq(hdr.traces_offset, 6800);
    expect_eq(hdr.sample_format == .int16, true);

    let it = SegyTraceIter.init(buf[0..7044], hdr);
    const tr = it.next() ?? unreachable;
    expect_eq(tr.header.trace_seq_line, 9);
    expect_eq(tr.header.cdp, 309);
    expect_eq(approxEq(readSampleF64(tr, .int16, 0) ?? 0.0, -32768.0, 1e-9), true);
    expect_eq(approxEq(readSampleF64(tr, .int16, 1) ?? 0.0, 32767.0, 1e-9), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);

    // 扩展文本头超出缓冲区总长时 parseSegyHeader 必须返回 null
    expect_eq(parseSegyHeader(buf[0..6799]) == null, true);
});

test("5. Truncated buffers, unknown sample_format, and out-of-bounds sample read protection", () => {
    // 截断卷头 (< 3600B)
    let short_buf: [128]u8 = [0; 128];
    expect_eq(parseSegyHeader(short_buf[0..128]) == null, true);

    // 非法 sample_format (code = 99 -> .unknown)
    let bad_fmt: [3852]u8 = [0; 3852];
    let s_bad: []u8 = bad_fmt[0..3852];
    fillBinaryHeader(*s_bad, 99, 3, 0);
    const bhdr = parseSegyHeader(bad_fmt[0..3852]) ?? unreachable;
    expect_eq(bhdr.sample_format == .unknown, true);

    let bit = SegyTraceIter.init(bad_fmt[0..3852], bhdr);
    expect_eq(bit.next() == null, true);
    expect_eq(bit.had_error, true);

    // 截断道数据（道头完整 240B，但声明 3 个 f32 样本需 12B，实际仅给 8B）
    let trunc_trace: [3848]u8 = [0; 3848];
    let s_tr: []u8 = trunc_trace[0..3848];
    fillBinaryHeader(*s_tr, 5, 3, 0);
    fillTraceHeader(*s_tr, 3600, 1, 1, 0, 0, 0, 0, 0, 3);
    const thdr = parseSegyHeader(trunc_trace[0..3848]) ?? unreachable;
    let tit = SegyTraceIter.init(trunc_trace[0..3848], thdr);
    expect_eq(tit.next() == null, true);
    expect_eq(tit.had_error, true);

    // 越界样本读取防护
    let valid_buf: [3852]u8 = [0; 3852];
    let s_ok: []u8 = valid_buf[0..3852];
    fillBinaryHeader(*s_ok, 5, 3, 0);
    fillTraceHeader(*s_ok, 3600, 1, 1, 0, 0, 0, 0, 0, 3);
    writeF32Be(*s_ok, 3600 + 240 + 0, 9.5);
    const ok_hdr = parseSegyHeader(valid_buf[0..3852]) ?? unreachable;
    let ok_it = SegyTraceIter.init(valid_buf[0..3852], ok_hdr);
    const tr = ok_it.next() ?? unreachable;
    expect_eq(approxEq(readSampleF64(tr, .ieee_f32, 0) ?? 0.0, 9.5, 1e-6), true);
    expect_eq(readSampleF64(tr, .ieee_f32, 3) == null, true);
    expect_eq(readSampleF64(tr, .ieee_f32, 999) == null, true);
    expect_eq(readSampleF64(tr, .unknown, 0) == null, true);
});
