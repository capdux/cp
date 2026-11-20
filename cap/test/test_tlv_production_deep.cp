// Source: cp/cap/test/test_tlv_production_deep.cp
// Deep Production Test Suite for cap:tlv
// Covers:
//   1. TlvWriter / TlvReader multi-type fields, nested containers, findTag, TypeMismatch, Truncated, MaxDepthExceeded
//   2. Castagnoli crc32c standard test vectors ("123456789" == 0xE3069283)
//   3. LengthPrefixedFramer byte-by-byte half-packet feeding & multi-frame sticky-packet splitting
//   4. Frame tampering InvalidChecksum, oversized FrameTooLarge DoS guard & std.testing.allocator zero leaks
//   5. Dynamic namespace mounting via `import cap from "cap"; cap.tlv`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TLV_MAX_DEPTH,
    TlvError,
    FrameHeaderKind,
    TlvNestToken,
    TlvField,
    TlvWriter,
    TlvReader,
    LengthPrefixedFramer,
    crc32c,
} from "cap:tlv";
import cap from "cap";

fn testTlvRoundTripAndGuards()! {
    const alloc = std.testing.allocator;

    let w = TlvWriter.init(alloc);
    defer w.deinit();

    w.writeU64(0x0001, 0x0123456789ABCDEF)?;
    w.writeI64(0x0002, -9876543210123)?;
    w.writeStr(0x0003, "cp-tlv-protocol")?;
    w.writeBool(0x0004, true)?;
    w.writeBool(0x0005, false)?;
    w.writeBytes(0x0006, "\x00\xFF\x80\x7F")?;

    const outer_tok = w.beginNested(0x0100)?;
    w.writeStr(0x0101, "outer-name")?;
    const inner_tok = w.beginNested(0x0200)?;
    w.writeU64(0x0201, 42)?;
    w.writeBool(0x0202, true)?;
    w.endNested(inner_tok)?;
    w.endNested(outer_tok)?;

    const encoded = w.toOwnedSlice()?;
    defer alloc.free(encoded);

    let r = TlvReader.init(encoded);
    const f1 = (r.next()?).?;
    expect_eq(f1.tag, 0x0001 as u16);
    expect_eq(f1.asU64()?, 0x0123456789ABCDEF as u64);

    const f2 = (r.next()?).?;
    expect_eq(f2.tag, 0x0002 as u16);
    expect_eq(f2.asI64()?, -9876543210123 as i64);

    const f3 = (r.next()?).?;
    expect_eq(std.mem.eql(u8, f3.asStr()?, "cp-tlv-protocol"), true);

    const f4 = (r.next()?).?;
    expect_eq(f4.asBool()?, true);

    const f5 = (r.next()?).?;
    expect_eq(f5.asBool()?, false);

    const f6 = (r.next()?).?;
    expect_eq(std.mem.eql(u8, f6.asBytes(), "\x00\xFF\x80\x7F"), true);

    const f_nested = (r.next()?).?;
    expect_eq(f_nested.tag, 0x0100 as u16);
    expect_eq(r.next()? == null, true);

    // Nested reader + findTag
    let outer_r = f_nested.reader()?;
    const outer_name = outer_r.findTag(0x0101)?;
    expect_eq(std.mem.eql(u8, outer_name.asStr()?, "outer-name"), true);

    const inner_field = outer_r.findTag(0x0200)?;
    let inner_r = inner_field.reader()?;
    expect_eq(inner_r.findTag(0x0201)?.asU64()?, 42 as u64);
    expect_eq(inner_r.findTag(0x0202)?.asBool()?, true);

    // TagNotFound
    switch (inner_r.findTag(0x9999)) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.TagNotFound, true),
    }

    // TypeMismatch: asU64 on 1-byte bool, asBool on 8-byte u64, asStr on invalid UTF-8
    switch (f4.asU64()) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.TypeMismatch, true),
    }
    switch (f1.asBool()) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.TypeMismatch, true),
    }
    switch (f6.asStr()) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.TypeMismatch, true),
    }

    // Truncated header & truncated payload
    let trunc_hdr = TlvReader.init(encoded[0..4]);
    switch (trunc_hdr.next()) {
        null => expect_eq(true, false),
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.Truncated, true),
    }
    let trunc_val = TlvReader.init(encoded[0..10]);
    switch (trunc_val.next()) {
        null => expect_eq(true, false),
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.Truncated, true),
    }

    // MaxDepthExceeded (32 allowed, 33rd rejected)
    let dw = TlvWriter.init(alloc);
    defer dw.deinit();
    let tokens: [32]TlvNestToken = undefined;
    let d: usize = 0;
    while (d < TLV_MAX_DEPTH) : (d += 1) {
        tokens[d] = dw.beginNested(0x0010)?;
    }
    switch (dw.beginNested(0x0010)) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.MaxDepthExceeded, true),
    }
    while (d > 0) {
        d -= 1;
        dw.endNested(tokens[d])?;
    }
}

fn testCrc32cVectors() {
    expect_eq(crc32c(""), 0x00000000 as u32);
    expect_eq(crc32c("123456789"), 0xE3069283 as u32);
    const a = crc32c("cp-lang-tlv");
    const b = crc32c("cp-lang-tlv");
    const c = crc32c("cp-lang-tlw");
    expect_eq(a, b);
    expect_eq(a != c, true);
}

fn testFramerHalfAndStickyPackets()! {
    const alloc = std.testing.allocator;

    const kinds: [3]FrameHeaderKind = [
        FrameHeaderKind.U16Be,
        FrameHeaderKind.U32Be,
        FrameHeaderKind.U32Le,
    ];

    let ki: usize = 0;
    while (ki < 3) : (ki += 1) {
        let framer = LengthPrefixedFramer.init(alloc, kinds[ki], 4096, true);
        defer framer.deinit();

        const p1 = "frame-one-hello";
        const p2 = "frame-two-\x00\xFF-binary";
        const p3 = "";

        const f1 = framer.encodeFrame(alloc, p1)?;
        defer alloc.free(f1);
        const f2 = framer.encodeFrame(alloc, p2)?;
        defer alloc.free(f2);
        const f3 = framer.encodeFrame(alloc, p3)?;
        defer alloc.free(f3);

        // Combine all 3 frames into one sticky buffer
        const total_len = f1.len + f2.len + f3.len;
        const combined = alloc.alloc(u8, total_len)?;
        defer alloc.free(combined);
        std.mem.copyForwards(u8, combined[0..f1.len], f1);
        std.mem.copyForwards(u8, combined[f1.len .. f1.len + f2.len], f2);
        std.mem.copyForwards(u8, combined[f1.len + f2.len .. total_len], f3);

        // Feed half-packet byte-by-byte for the first frame
        let idx: usize = 0;
        while (idx < f1.len - 1) : (idx += 1) {
            framer.feed(combined[idx .. idx + 1])?;
            expect_eq(framer.nextFrame(alloc)? == null, true);
        }

        // Feed remaining bytes (last byte of f1 + all of f2 + all of f3) at once (sticky!)
        framer.feed(combined[f1.len - 1 .. total_len])?;

        const out1 = (framer.nextFrame(alloc)?).?;
        defer alloc.free(out1);
        expect_eq(std.mem.eql(u8, out1, p1), true);

        const out2 = (framer.nextFrame(alloc)?).?;
        defer alloc.free(out2);
        expect_eq(std.mem.eql(u8, out2, p2), true);

        const out3 = (framer.nextFrame(alloc)?).?;
        defer alloc.free(out3);
        expect_eq(std.mem.eql(u8, out3, p3), true);

        expect_eq(framer.nextFrame(alloc)? == null, true);
        expect_eq(framer.bufferedBytes(), 0 as usize);
    }
}

fn testFramerTamperAndDosGuards()! {
    const alloc = std.testing.allocator;

    let framer = LengthPrefixedFramer.init(alloc, FrameHeaderKind.U32Be, 64, true);
    defer framer.deinit();

    // 1. Tampered payload -> InvalidChecksum
    let valid = framer.encodeFrame(alloc, "secure-payload")?;
    defer alloc.free(valid);
    valid[6] = valid[6] ^ 0xFF;
    framer.feed(valid)?;
    switch (framer.nextFrame(alloc)) {
        null => expect_eq(true, false),
        v => { alloc.free(v); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.InvalidChecksum, true),
    }

    // 2. Oversized encodeFrame -> FrameTooLarge
    const big_payload: [65]u8 = [0xAA; 65];
    switch (framer.encodeFrame(alloc, &big_payload)) {
        v => { alloc.free(v); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.FrameTooLarge, true),
    }

    // 3. Oversized incoming header -> FrameTooLarge immediately without needing full body
    const evil_hdr: [4]u8 = [0x00, 0x00, 0x01, 0x00]; // 256 > 64
    framer.feed(&evil_hdr)?;
    switch (framer.nextFrame(alloc)) {
        null => expect_eq(true, false),
        v => { alloc.free(v); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.FrameTooLarge, true),
    }
}

fn testCapDynamicNamespaceMount()! {
    const alloc = std.testing.allocator;

    expect_eq(cap.tlv.crc32c("123456789"), 0xE3069283 as u32);
    expect_eq(cap.tlv.TLV_MAX_DEPTH, 32 as usize);

    let w = cap.tlv.TlvWriter.init(alloc);
    defer w.deinit();
    w.writeStr(0x000A, "dynamic-cap-tlv")?;
    const raw = w.toOwnedSlice()?;
    defer alloc.free(raw);

    let r = cap.tlv.TlvReader.init(raw);
    const f = r.findTag(0x000A)?;
    expect_eq(std.mem.eql(u8, f.asStr()?, "dynamic-cap-tlv"), true);
}

test("tlv: writer/reader multi-type round-trip, nested containers, findTag, TypeMismatch, Truncated, MaxDepthExceeded", () => {
    testTlvRoundTripAndGuards() catch unreachable;
});

test("tlv: Castagnoli crc32c standard check vectors", () => {
    testCrc32cVectors();
});

test("tlv: LengthPrefixedFramer byte-by-byte half-packet & multi-frame sticky-packet reassembly", () => {
    testFramerHalfAndStickyPackets() catch unreachable;
});

test("tlv: frame tampering InvalidChecksum, FrameTooLarge DoS guard & zero memory leak", () => {
    testFramerTamperAndDosGuards() catch unreachable;
});

test("tlv: dynamic cap.tlv namespace mounting", () => {
    testCapDynamicNamespaceMount() catch unreachable;
});
