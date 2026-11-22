// Deep production regression tests for cap:pkg lockb.cp
// Covers: 86-byte header encode/decode, 7 ASCII 8-byte trailing extension tags,
// v2->v3 auto migration vs outdated/future version rejection, 0xDEADBEEF array sentinel & bounds guard,
// full frame roundtrip + trailing section inspection, truncation & corrupt terminator protection.

import { test, expect, expectEq } from "cap:test";
import {
    LOCKB_FIXED_HEADER_LEN,
    LOCKB_FORMAT_V2,
    LOCKB_FORMAT_CURRENT,
    LOCKB_DEAD_SENTINEL,
    LOCKB_TAG_WORKSPACES,
    LOCKB_TAG_TRUSTED_DEPS,
    LOCKB_TAG_EMPTY_TRUSTED,
    LOCKB_TAG_OVERRIDES,
    LOCKB_TAG_PATCHED_DEPS,
    LOCKB_TAG_CATALOGS,
    LOCKB_TAG_CONFIG_VERSION,
    LOCKB_EXT_WORKSPACES,
    LOCKB_EXT_TRUSTED_DEPS,
    LOCKB_EXT_EMPTY_TRUSTED,
    LOCKB_EXT_OVERRIDES,
    LOCKB_EXT_PATCHED_DEPS,
    LOCKB_EXT_CATALOGS,
    LOCKB_EXT_CONFIG_VERSION,
    LockbTagKind,
    lockbTagFromAscii8,
    isLockbMagic,
    alignLockbOffset,
    LockbHeader,
    LockbArrayRef,
    LockbFrameCodec,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("lockb: 8-byte ASCII extension tags match exact little-endian Bun constants", fn () {
    expectEq(lockbTagFromAscii8(['w', 'O', 'r', 'K', 's', 'P', 'a', 'C']), LOCKB_TAG_WORKSPACES);
    expectEq(lockbTagFromAscii8(['t', 'R', 'u', 'S', 't', 'E', 'D', 'd']), LOCKB_TAG_TRUSTED_DEPS);
    expectEq(lockbTagFromAscii8(['e', 'M', 'p', 'T', 'r', 'U', 's', 'T']), LOCKB_TAG_EMPTY_TRUSTED);
    expectEq(lockbTagFromAscii8(['o', 'V', 'e', 'R', 'r', 'i', 'D', 's']), LOCKB_TAG_OVERRIDES);
    expectEq(lockbTagFromAscii8(['p', 'A', 't', 'C', 'h', 'E', 'd', 'D']), LOCKB_TAG_PATCHED_DEPS);
    expectEq(lockbTagFromAscii8(['c', 'A', 't', 'A', 'l', 'O', 'g', 'S']), LOCKB_TAG_CATALOGS);
    expectEq(lockbTagFromAscii8(['c', 'N', 'f', 'G', 'v', 'R', 's', 'N']), LOCKB_TAG_CONFIG_VERSION);

    expectEq(LockbTagKind.fromU64(LOCKB_TAG_CATALOGS), LockbTagKind.Catalogs);
    expectEq(LockbTagKind.Catalogs.toU64(), LOCKB_TAG_CATALOGS);
    expect(strEq(LockbTagKind.ConfigVersion.label(), "configVersion"));
    expectEq(LockbTagKind.fromU64(0x1234), LockbTagKind.Unknown);

    expectEq(alignLockbOffset(0, 8), @as(usize, 0));
    expectEq(alignLockbOffset(1, 8), @as(usize, 8));
    expectEq(alignLockbOffset(16, 8), @as(usize, 16));
    expectEq(alignLockbOffset(17, 16), @as(usize, 32));
});

test("lockb: LockbHeader encode/decode roundtrip and meta_hash hex formatting", fn () {
    let hdr = LockbHeader{
        format_version: LOCKB_FORMAT_CURRENT,
        total_buffer_size: 256,
    };
    hdr.meta_hash[0] = 0xde;
    hdr.meta_hash[1] = 0xad;
    hdr.meta_hash[31] = 0x0f;

    let buf: [128]u8 = [0; 128];
    const written = hdr.encode(buf[0..]) catch unreachable;
    expectEq(written, LOCKB_FIXED_HEADER_LEN);
    expect(isLockbMagic(buf[0..written]));

    const decoded = LockbHeader.decode(buf[0..written]) catch unreachable;
    expectEq(decoded.format_version, LOCKB_FORMAT_CURRENT);
    expectEq(decoded.total_buffer_size, @as(u64, 256));
    expect(!decoded.migrated_from_v2);
    expectEq(decoded.meta_hash[0], @as(u8, 0xde));
    expectEq(decoded.meta_hash[1], @as(u8, 0xad));
    expectEq(decoded.meta_hash[31], @as(u8, 0x0f));

    let hex: [64]u8 = [0; 64];
    const hn = decoded.formatMetaHashHex(hex[0..]) catch unreachable;
    expectEq(hn, @as(usize, 64));
    expect(strEq(hex[0..4], "dead"));
    expect(strEq(hex[62..64], "0f"));
});

test("lockb: v2 auto-migration succeeds while v1 (outdated) and v4 (future) fail", fn () {
    let buf: [96]u8 = [0; 96];
    const hdr_v2 = LockbHeader{ format_version: LOCKB_FORMAT_V2, total_buffer_size: 96 };
    * = hdr_v2.encode(buf[0..]) catch unreachable;

    const dec_v2 = LockbHeader.decode(buf[0..]) catch unreachable;
    expectEq(dec_v2.format_version, LOCKB_FORMAT_CURRENT);
    expect(dec_v2.migrated_from_v2);

    // Outdated v1 must fail
    const hdr_v1 = LockbHeader{ format_version: 1, total_buffer_size: 96 };
    * = hdr_v1.encode(buf[0..]) catch unreachable;
    let failed_v1 = false;
    * = LockbHeader.decode(buf[0..]) catch {
        failed_v1 = true;
    };
    expect(failed_v1);

    // Future v4 must fail
    const hdr_v4 = LockbHeader{ format_version: 4, total_buffer_size: 96 };
    * = hdr_v4.encode(buf[0..]) catch unreachable;
    let failed_v4 = false;
    * = LockbHeader.decode(buf[0..]) catch {
        failed_v4 = true;
    };
    expect(failed_v4);
});

test("lockb: LockbArrayRef handles 0xDEADBEEF empty sentinel and validates ranges", fn () {
    let buf: [64]u8 = [0; 64];

    const end_sentinel = LockbArrayRef.writeEmptySentinel(buf[0..], 0) catch unreachable;
    expectEq(end_sentinel, @as(usize, 8));
    const sref = LockbArrayRef.readFrom(buf[0..], 0) catch unreachable;
    expect(sref.is_empty_sentinel);
    expectEq(sref.start_pos, LOCKB_DEAD_SENTINEL);
    expectEq(sref.encodedSize(), @as(usize, 8));

    const end_arr = LockbArrayRef.writeTo(buf[0..], 16, 32, 24) catch unreachable;
    expectEq(end_arr, @as(usize, 32));
    const aref = LockbArrayRef.readFrom(buf[0..], 16) catch unreachable;
    expect(!aref.is_empty_sentinel);
    expectEq(aref.start_pos, @as(u64, 32));
    expectEq(aref.byte_len, @as(u64, 24));
    expectEq(aref.encodedSize(), @as(usize, 16));

    // Out-of-bounds array range must be rejected
    * = LockbArrayRef.writeTo(buf[0..], 16, 48, 32) catch unreachable;
    let failed_oob = false;
    * = LockbArrayRef.readFrom(buf[0..], 16) catch {
        failed_oob = true;
    };
    expect(failed_oob);
});

test("lockb: LockbFrameCodec encodes and inspects full trailing extensions & config_version", fn () {
    let hdr = LockbHeader{};
    hdr.meta_hash[0] = 0x42;
    const core = "CORE_PKGS_AND_BUFFERS";
    const mask: u16 = LOCKB_EXT_WORKSPACES | LOCKB_EXT_EMPTY_TRUSTED | LOCKB_EXT_OVERRIDES |
        LOCKB_EXT_PATCHED_DEPS | LOCKB_EXT_CATALOGS;

    let buf: [512]u8 = [0; 512];
    const total = LockbFrameCodec.encodeFrame(hdr, core, mask, 2, buf[0..]) catch unreachable;
    expect(total > LOCKB_FIXED_HEADER_LEN + core.len + 8);

    const res = LockbFrameCodec.inspectFrame(buf[0..total], core.len) catch unreachable;
    expectEq(res.effective_size, total);
    expectEq(res.trailing.section_count, @as(usize, 6));
    expect(res.trailing.hasExtension(LOCKB_EXT_WORKSPACES));
    expect(res.trailing.hasExtension(LOCKB_EXT_EMPTY_TRUSTED));
    expect(!res.trailing.hasExtension(LOCKB_EXT_TRUSTED_DEPS));
    expect(res.trailing.hasExtension(LOCKB_EXT_OVERRIDES));
    expect(res.trailing.hasExtension(LOCKB_EXT_PATCHED_DEPS));
    expect(res.trailing.hasExtension(LOCKB_EXT_CATALOGS));
    expect(res.trailing.hasExtension(LOCKB_EXT_CONFIG_VERSION));
    expect(res.trailing.has_empty_trusted_deps);
    expectEq(res.trailing.config_version.?, @as(u64, 2));
});

test("lockb: rejects invalid magic, truncated header, and corrupt zero terminator", fn () {
    const bad_magic = "not-a-bun-lockfile-at-all-padded-to-more-than-eighty-six-bytes-long-012345678901234567890123456789";
    expect(!isLockbMagic(bad_magic));
    let failed_magic = false;
    * = LockbHeader.decode(bad_magic) catch {
        failed_magic = true;
    };
    expect(failed_magic);

    let failed_trunc = false;
    * = LockbHeader.decode(bad_magic[0..20]) catch {
        failed_trunc = true;
    };
    expect(failed_trunc);

    // Corrupt terminator byte after core payload must fail inspectFrame
    let buf: [256]u8 = [0; 256];
    const hdr = LockbHeader{};
    const core = "DATA";
    const total = LockbFrameCodec.encodeFrame(hdr, core, LOCKB_EXT_TRUSTED_DEPS, null, buf[0..]) catch unreachable;
    buf[LOCKB_FIXED_HEADER_LEN + core.len] = 0x01;
    let failed_term = false;
    * = LockbFrameCodec.inspectFrame(buf[0..total], core.len) catch {
        failed_term = true;
    };
    expect(failed_term);
});
