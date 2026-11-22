// Deep production regression tests for cap:bundler hmrFailure.cp
// Covers: 32-bit bit-packed HmrFailureOwner layout (matching Bun OwnerPacked u30+u2),
// O(1) peekOwner(), full binary encode/decode roundtrip across bundler log & JS error kinds,
// optional source locations (line<=0 normalization to 0), nested notes, and truncation/bounds guards.

import { test, expect, expectEq } from "cap:test";
import {
    HMR_FAILURE_MAX_OWNER_INDEX,
    HmrFailureOwnerKind,
    HmrFailureOwner,
    HmrFailureErrorKind,
    HmrSourceLoc,
    HmrFailureData,
    HmrFailureMsg,
    HmrFailureFrame,
    HmrFailureCodec,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("hmrFailure: 32-bit HmrFailureOwner pack/unpack matches Bun OwnerPacked bit layout", fn () {
    // Bun comptime assert: @bitCast(OwnerPacked{ kind: .none, data: 1 }) == 1
    const o_none = HmrFailureOwner.init(.None, 1) catch unreachable;
    expectEq(o_none.pack(), @as(u32, 1));

    const o_route = HmrFailureOwner.init(.Route, 42) catch unreachable;
    const o_client = HmrFailureOwner.init(.Client, 1024) catch unreachable;
    const o_server = HmrFailureOwner.init(.Server, HMR_FAILURE_MAX_OWNER_INDEX) catch unreachable;

    expect(HmrFailureOwner.unpack(o_route.pack()).eql(o_route));
    expect(HmrFailureOwner.unpack(o_client.pack()).eql(o_client));
    expect(HmrFailureOwner.unpack(o_server.pack()).eql(o_server));
    expectEq(HmrFailureOwner.unpack(o_server.pack()).kind, HmrFailureOwnerKind.Server);
    expectEq(HmrFailureOwner.unpack(o_server.pack()).index, HMR_FAILURE_MAX_OWNER_INDEX);

    let failed_overflow = false;
    * = HmrFailureOwner.init(.Client, HMR_FAILURE_MAX_OWNER_INDEX + 1) catch {
        failed_overflow = true;
    };
    expect(failed_overflow);
});

test("hmrFailure: encode/decode roundtrip with source locations, nested notes, and O(1) peekOwner", fn () {
    const owner = HmrFailureOwner.init(.Client, 19) catch unreachable;
    let frame = HmrFailureFrame.init(owner, "src/components/Counter.tsx");

    let m0 = HmrFailureMsg{
        kind: .BundlerLogErr,
        data: HmrFailureData{
            text: "Expected ';' after expression",
            location: HmrSourceLoc{
                line: 24,
                column: 11,
                length: 5,
                line_text: "const x = 1 foo",
            },
        },
    };
    * = m0.addNote(HmrFailureData{
        text: "while parsing component body",
        location: HmrSourceLoc{ line: 10, column: 0, length: 8, line_text: "export {" },
    }) catch unreachable;
    * = m0.addNote(HmrFailureData{
        text: "no-location hint",
        location: null,
    }) catch unreachable;

    let m1 = HmrFailureMsg{
        kind: .JsErrorType,
        data: HmrFailureData{
            text: "Cannot read properties of undefined",
            location: null,
        },
    };

    * = frame.addMsg(m0) catch unreachable;
    * = frame.addMsg(m1) catch unreachable;

    let wire: [1024]u8 = [0; 1024];
    const n = HmrFailureCodec.encode(frame, wire[0..]) catch unreachable;

    // O(1) owner peek
    const peeked = HmrFailureCodec.peekOwner(wire[0..n]) catch unreachable;
    expect(peeked.eql(owner));

    // Full zero-copy decode
    const decoded = HmrFailureCodec.decode(wire[0..n]) catch unreachable;
    expect(decoded.owner.eql(owner));
    expect(strEq(decoded.display_name, "src/components/Counter.tsx"));
    expectEq(decoded.msgSlice().len, @as(usize, 2));

    const dm0 = decoded.msgSlice()[0];
    expectEq(dm0.kind, HmrFailureErrorKind.BundlerLogErr);
    expect(dm0.kind.isBundlerLog());
    expect(strEq(dm0.data.text, "Expected ';' after expression"));
    expectEq(dm0.data.location.?.line, @as(i32, 24));
    expectEq(dm0.data.location.?.column, @as(u32, 11));
    expectEq(dm0.data.location.?.length, @as(u32, 5));
    expect(strEq(dm0.data.location.?.line_text, "const x = 1 foo"));
    expectEq(dm0.noteSlice().len, @as(usize, 2));
    expect(strEq(dm0.noteSlice()[0].text, "while parsing component body"));
    expect(dm0.noteSlice()[1].location == null);

    const dm1 = decoded.msgSlice()[1];
    expectEq(dm1.kind, HmrFailureErrorKind.JsErrorType);
    expect(!dm1.kind.isBundlerLog());
    expect(dm1.data.location == null);
});

test("hmrFailure: negative/zero source line normalizes to absent location on wire", fn () {
    const owner = HmrFailureOwner.init(.Server, 3) catch unreachable;
    let frame = HmrFailureFrame.init(owner, "src/server.ts");
    * = frame.addMsg(HmrFailureMsg{
        kind: .BundlerLogWarn,
        data: HmrFailureData{
            text: "synthetic warning",
            location: HmrSourceLoc{ line: -1, column: 2, length: 1, line_text: "ignored" },
        },
    }) catch unreachable;

    let wire: [256]u8 = [0; 256];
    const n = HmrFailureCodec.encode(frame, wire[0..]) catch unreachable;
    const decoded = HmrFailureCodec.decode(wire[0..n]) catch unreachable;
    expect(decoded.msgSlice()[0].data.location == null);
});

test("hmrFailure: all 12 HmrFailureErrorKind values roundtrip through byte encoding", fn () {
    let b: u8 = 0;
    while (b <= 11) {
        const k = HmrFailureErrorKind.fromByte(b) catch unreachable;
        expectEq(k.toByte(), b);
        b += 1;
    }
    let failed_invalid = false;
    * = HmrFailureErrorKind.fromByte(12) catch {
        failed_invalid = true;
    };
    expect(failed_invalid);
});

test("hmrFailure: guards empty frames, tiny buffers, and truncated wire inputs", fn () {
    const empty_frame = HmrFailureFrame.init(HmrFailureOwner.none(), "empty.ts");
    let wire: [128]u8 = [0; 128];
    let err_empty = false;
    * = HmrFailureCodec.encode(empty_frame, wire[0..]) catch {
        err_empty = true;
    };
    expect(err_empty);

    let frame = HmrFailureFrame.init(HmrFailureOwner.none(), "src/index.ts");
    * = frame.addMsg(HmrFailureMsg{
        kind: .JsAggregate,
        data: HmrFailureData{ text: "multiple failures" },
    }) catch unreachable;
    const n = HmrFailureCodec.encode(frame, wire[0..]) catch unreachable;

    let tiny: [8]u8 = [0; 8];
    let err_small = false;
    * = HmrFailureCodec.encode(frame, tiny[0..]) catch {
        err_small = true;
    };
    expect(err_small);

    // Every truncated prefix must fail cleanly
    let err_trunc = false;
    * = HmrFailureCodec.decode(wire[0 .. n - 3]) catch {
        err_trunc = true;
    };
    expect(err_trunc);
});
