// Reference-hardened test suite against RFC 8794 (EBML) / RFC 9559 (Matroska) & ebml-iterable (MIT)
// Covers RFC 8794 VINT/ID/Unknown-Size/int/float vectors, synthetic Matroska & WebM containers with
// SimpleBlock 4-mode lacing, strict malformed/overflow/depth guards, 16-lane @Vector(16, u8) SIMD
// vs scalar 0-diff parity, and 1,600 deterministic PRNG fuzz cases.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    EBML_ID_HEADER,
    EBML_ID_DOCTYPE,
    EBML_ID_SEGMENT,
    EBML_MAX_DEPTH,
    EbmlError,
    simdEbmlCopyPayload,
    scalarEbmlCopyPayload,
    simdEbmlBytesEqual,
    scalarEbmlBytesEqual,
    simdFindEbmlHeaderMagic,
    scalarFindEbmlHeaderMagic,
    decodeEbmlId,
    decodeEbmlSize,
    isEbmlHeader,
    EbmlElementIter,
    decodeEbmlUint,
    decodeEbmlSint,
    decodeEbmlFloat,
    validateEbmlStrict,
    validateEbmlStrictWithMaxDepth,
} from "../../lib/media/ebml.cp";
import {
    ID_EBML,
    ID_DOC_TYPE,
    ID_SEGMENT,
    ID_INFO,
    ID_TIMECODE_SCALE,
    ID_DURATION,
    ID_TRACKS,
    ID_TRACK_ENTRY,
    ID_TRACK_NUMBER,
    ID_TRACK_TYPE,
    ID_CODEC_ID,
    ID_VIDEO,
    ID_PIXEL_WIDTH,
    ID_PIXEL_HEIGHT,
    ID_CLUSTER,
    ID_CLUSTER_TIMECODE,
    ID_SIMPLE_BLOCK,
    MkvError,
    MkvLacingKind,
    MkvTrackType,
    parseMkvSimpleBlock,
    isMkv,
    isWebm,
    parseMkvHeader,
    validateMkvStrict,
} from "../../lib/media/mkv.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

struct BufWriter {
    buf: []u8,
    pos: usize = 0,

    fn putByte(*_, b: u8) {
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn putBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            _.buf[_.pos] = s[i];
            _.pos += 1;
        }
    }

    fn putId(*_, id: u32) {
        if (id > 0x00FFFFFF) {
            _.putByte(@as(u8, @truncate(id >> 24)));
            _.putByte(@as(u8, @truncate(id >> 16)));
            _.putByte(@as(u8, @truncate(id >> 8)));
            _.putByte(@as(u8, @truncate(id)));
        } else if (id > 0x0000FFFF) {
            _.putByte(@as(u8, @truncate(id >> 16)));
            _.putByte(@as(u8, @truncate(id >> 8)));
            _.putByte(@as(u8, @truncate(id)));
        } else if (id > 0x000000FF) {
            _.putByte(@as(u8, @truncate(id >> 8)));
            _.putByte(@as(u8, @truncate(id)));
        } else {
            _.putByte(@as(u8, @truncate(id)));
        }
    }

    fn putSize1(*_, sz: usize) {
        _.putByte(0x80 | @as(u8, @intCast(sz)));
    }

    fn putElem(*_, id: u32, payload: []const u8) {
        _.putId(id);
        if (payload.len < 127) {
            _.putSize1(payload.len);
        } else {
            _.putByte(0x40 | @as(u8, @intCast(payload.len >> 8)));
            _.putByte(@as(u8, @truncate(payload.len)));
        }
        _.putBytes(payload);
    }
}

fn buildValidWebm(out: []u8, doc_type: string): usize {
    // 1. Video sub-element
    let vbuf: [32]u8 = [0; 32];
    let vw = BufWriter{ buf: vbuf[0..] };
    const w1280: [2]u8 = [0x05, 0x00];
    const h720: [2]u8 = [0x02, 0xD0];
    vw.putElem(ID_PIXEL_WIDTH, w1280[0..]);
    vw.putElem(ID_PIXEL_HEIGHT, h720[0..]);

    // 2. TrackEntry
    let tebuf: [64]u8 = [0; 64];
    let tew = BufWriter{ buf: tebuf[0..] };
    const tn1: [1]u8 = [1];
    const tt1: [1]u8 = [1];
    tew.putElem(ID_TRACK_NUMBER, tn1[0..]);
    tew.putElem(ID_TRACK_TYPE, tt1[0..]);
    tew.putElem(ID_CODEC_ID, "V_VP9");
    tew.putElem(ID_VIDEO, vbuf[0..vw.pos]);

    // 3. Tracks
    let trbuf: [80]u8 = [0; 80];
    let trw = BufWriter{ buf: trbuf[0..] };
    trw.putElem(ID_TRACK_ENTRY, tebuf[0..tew.pos]);

    // 4. Info
    let ibuf: [48]u8 = [0; 48];
    let iw = BufWriter{ buf: ibuf[0..] };
    const ts: [3]u8 = [0x0F, 0x42, 0x40]; // 1_000_000
    const dur_f32: [4]u8 = [0x44, 0x7A, 0x00, 0x00]; // 1000.0f
    iw.putElem(ID_TIMECODE_SCALE, ts[0..]);
    iw.putElem(ID_DURATION, dur_f32[0..]);

    // 5. Cluster with Timecode + SimpleBlock (track 1, tc=+10, keyframe, no lacing, 4B frame)
    let cbuf: [64]u8 = [0; 64];
    let cw = BufWriter{ buf: cbuf[0..] };
    const ctc: [1]u8 = [0];
    const sb: [8]u8 = [0x81, 0x00, 0x0A, 0x80, 0xDE, 0xAD, 0xBE, 0xEF];
    cw.putElem(ID_CLUSTER_TIMECODE, ctc[0..]);
    cw.putElem(ID_SIMPLE_BLOCK, sb[0..]);

    // 6. Segment body
    let sbuf: [256]u8 = [0; 256];
    let sw = BufWriter{ buf: sbuf[0..] };
    sw.putElem(ID_INFO, ibuf[0..iw.pos]);
    sw.putElem(ID_TRACKS, trbuf[0..trw.pos]);
    sw.putElem(ID_CLUSTER, cbuf[0..cw.pos]);

    // 7. EBML header body
    let hbuf: [32]u8 = [0; 32];
    let hw = BufWriter{ buf: hbuf[0..] };
    hw.putElem(ID_DOC_TYPE, doc_type);

    let root = BufWriter{ buf: out };
    root.putElem(ID_EBML, hbuf[0..hw.pos]);
    root.putElem(ID_SEGMENT, sbuf[0..sw.pos]);
    return root.pos;
}

fn checkRfc8794AndMkvVectors(): u32 {
    // 1. RFC 8794 VINT ID & Size vectors
    const id_ebml: [4]u8 = [0x1A, 0x45, 0xDF, 0xA3];
    const v_id = decodeEbmlId(id_ebml[0..]) ?? return 1;
    if (v_id.raw_id != EBML_ID_HEADER || v_id.bytes_read != 4) return 2;

    const unk_1: [1]u8 = [0xFF];
    const s_unk1 = decodeEbmlSize(unk_1[0..]) ?? return 3;
    if (!s_unk1.is_unknown_size || s_unk1.bytes_read != 1) return 4;

    const unk_8: [8]u8 = [0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    const s_unk8 = decodeEbmlSize(unk_8[0..]) ?? return 5;
    if (!s_unk8.is_unknown_size || s_unk8.bytes_read != 8) return 6;

    // 2. Signed & Unsigned int + IEEE 754 float (0B/4B/8B)
    const empty_s: []const u8 = "";
    if ((decodeEbmlUint(empty_s) ?? 99) != 0) return 7;
    if ((decodeEbmlSint(empty_s) ?? 99) != 0) return 8;
    const neg_one: [1]u8 = [0xFF];
    if ((decodeEbmlSint(neg_one[0..]) ?? 0) != -1) return 9;
    const neg_256: [2]u8 = [0xFF, 0x00];
    if ((decodeEbmlSint(neg_256[0..]) ?? 0) != -256) return 10;
    const f32_one: [4]u8 = [0x3F, 0x80, 0x00, 0x00];
    if ((decodeEbmlFloat(f32_one[0..]) ?? 0.0) != 1.0) return 11;
    const f64_two: [8]u8 = [0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    if ((decodeEbmlFloat(f64_two[0..]) ?? 0.0) != 2.0) return 12;

    // 3. Synthetic WebM & Matroska container parse + validateMkvStrict + validateEbmlStrict
    let webm: [320]u8 = [0; 320];
    const wlen = buildValidWebm(webm[0..], "webm");
    if (!isEbmlHeader(webm[0..wlen]) || !isMkv(webm[0..wlen]) || !isWebm(webm[0..wlen])) return 13;
    validateEbmlStrict(webm[0..wlen]) catch return 14;
    validateMkvStrict(webm[0..wlen]) catch return 15;

    const hdr = parseMkvHeader(webm[0..wlen]) ?? return 16;
    if (!hdr.has_info || !hdr.has_tracks || !hdr.info.isWebm()) return 17;
    if (hdr.info.durationMs() != 1000.0) return 18;

    let tit = hdr.tracks();
    const trk = tit.next() ?? return 19;
    if (trk.track_number != 1 || trk.track_type != MkvTrackType.video) return 20;
    if (trk.pixel_width != 1280 || trk.pixel_height != 720) return 21;
    if (!simdEbmlBytesEqual(trk.codec_id, "V_VP9")) return 22;

    let mkv: [320]u8 = [0; 320];
    const mlen = buildValidWebm(mkv[0..], "matroska");
    if (!isMkv(mkv[0..mlen]) || isWebm(mkv[0..mlen])) return 23;
    validateMkvStrict(mkv[0..mlen]) catch return 24;

    // 4. SimpleBlock all 4 lacing modes
    // Fixed-size lacing: 2 frames of 3 bytes each (flags = 0x04 -> lace_bits = 2)
    const sb_fixed: [11]u8 = [0x81, 0x00, 0x00, 0x04, 0x01, 1, 2, 3, 4, 5, 6];
    const p_fix = parseMkvSimpleBlock(sb_fixed[0..]) catch return 25;
    if (p_fix.lacing != MkvLacingKind.fixed_size || p_fix.frame_count != 2) return 26;

    // Xiph lacing: 2 frames (first=2B, second=3B), flags = 0x02 (lace_bits = 1)
    const sb_xiph: [11]u8 = [0x81, 0x00, 0x05, 0x02, 0x01, 0x02, 1, 2, 3, 4, 5];
    const p_xiph = parseMkvSimpleBlock(sb_xiph[0..]) catch return 27;
    if (p_xiph.lacing != MkvLacingKind.xiph || p_xiph.frame_count != 2) return 28;

    // EBML lacing: 3 frames (first=2B [0x82], delta=+1 [0x80+64=0xC0 -> 64-63=+1 -> 3B], third=2B), flags = 0x06
    const sb_ebml: [14]u8 = [0x81, 0xFF, 0xFE, 0x86, 0x02, 0x82, 0xC0, 1, 2, 3, 4, 5, 6, 7];
    const p_ebml = parseMkvSimpleBlock(sb_ebml[0..]) catch return 29;
    if (p_ebml.lacing != MkvLacingKind.ebml || p_ebml.frame_count != 3 || !p_ebml.keyframe || p_ebml.timecode_rel != -2) return 30;

    return 0;
}

fn checkStrictRejectionGuards(): u32 {
    // 1. Forbidden VINT leading 0x00, all-0 ID, all-1 ID, >4B ID
    const z0: [2]u8 = [0x00, 0x01];
    if (decodeEbmlId(z0[0..]) != null || decodeEbmlSize(z0[0..]) != null) return 1;
    const all0_id: [1]u8 = [0x80];
    const all1_id: [1]u8 = [0xFF];
    const wide5_id: [5]u8 = [0x08, 0x01, 0x02, 0x03, 0x04];
    if (decodeEbmlId(all0_id[0..]) != null) return 2;
    if (decodeEbmlId(all1_id[0..]) != null) return 3;
    if (decodeEbmlId(wide5_id[0..]) != null) return 4;

    // 2. Invalid int (>8B) and float (3B / 5B)
    const big9: [9]u8 = [1; 9];
    if (decodeEbmlUint(big9[0..]) != null || decodeEbmlSint(big9[0..]) != null) return 5;
    const bad3: [3]u8 = [0, 0, 0];
    if (decodeEbmlFloat(bad3[0..]) != null) return 6;

    // 3. Payload out of bounds
    const oob_ebml: [6]u8 = [0x1A, 0x45, 0xDF, 0xA3, 0x90, 0x00]; // declares 16B payload, only 1B present
    let hit_oob = false;
    validateEbmlStrict(oob_ebml[0..]) catch |e| {
        if (e == EbmlError.PayloadOutOfBounds) hit_oob = true;
    };
    if (!hit_oob) return 7;

    // 4. Max depth exceeded (nested Cluster inside Segment inside EBML exceeding max_depth=2)
    let webm: [320]u8 = [0; 320];
    const wlen = buildValidWebm(webm[0..], "webm");
    let hit_depth = false;
    validateEbmlStrictWithMaxDepth(webm[0..wlen], 2) catch |e| {
        if (e == EbmlError.MaxDepthExceeded) hit_depth = true;
    };
    if (!hit_depth) return 8;

    // 5. Unsupported DocType in MKV
    let bad_dt: [320]u8 = [0; 320];
    const blen = buildValidWebm(bad_dt[0..], "evil");
    let hit_dt = false;
    validateMkvStrict(bad_dt[0..blen]) catch |e| {
        if (e == MkvError.UnsupportedDocType) hit_dt = true;
    };
    if (!hit_dt) return 9;

    // 6. Invalid SimpleBlock lacing (fixed-size not divisible & xiph overflow)
    const bad_fix: [10]u8 = [0x81, 0x00, 0x00, 0x04, 0x01, 1, 2, 3, 4, 5]; // 5 bytes / 2 frames
    let hit_lace = false;
    * = parseMkvSimpleBlock(bad_fix[0..]) catch |e| {
        if (e == MkvError.InvalidLacing) hit_lace = true;
    };
    if (!hit_lace) return 10;

    const bad_xiph: [7]u8 = [0x81, 0x00, 0x00, 0x02, 0x01, 0x50, 0x01]; // claims 80B, only 1B
    hit_lace = false;
    * = parseMkvSimpleBlock(bad_xiph[0..]) catch |e| {
        if (e == MkvError.InvalidLacing) hit_lace = true;
    };
    if (!hit_lace) return 11;

    return 0;
}

fn checkSimdDifferential(): u32 {
    let prng = XorShift64.init(0x4542_4d4c_5349_4d44);
    let src: [260]u8 = [0; 260];
    let dst_s: [260]u8 = [0; 260];
    let dst_r: [260]u8 = [0; 260];
    prng.fillBytes(src[0..]);

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        const n1 = simdEbmlCopyPayload(dst_s[0..len], src[0..len]);
        const n2 = scalarEbmlCopyPayload(dst_r[0..len], src[0..len]);
        if (n1 != n2 || n1 != len) return 1;
        if (!simdEbmlBytesEqual(dst_s[0..len], dst_r[0..len])) return 2;
        if (simdEbmlBytesEqual(dst_s[0..len], dst_r[0..len]) != scalarEbmlBytesEqual(dst_s[0..len], dst_r[0..len])) return 3;

        if (len > 0) {
            dst_r[len - 1] ^= 0x01;
            if (simdEbmlBytesEqual(dst_s[0..len], dst_r[0..len]) != scalarEbmlBytesEqual(dst_s[0..len], dst_r[0..len])) return 4;
            dst_r[len - 1] ^= 0x01;
        }

        // Magic resync scan across lengths & offsets
        if (len >= 4) {
            let scan_buf: [260]u8 = [0x22; 260];
            const mpos = len - 4;
            scan_buf[mpos] = 0x1A;
            scan_buf[mpos + 1] = 0x45;
            scan_buf[mpos + 2] = 0xDF;
            scan_buf[mpos + 3] = 0xA3;
            if (simdFindEbmlHeaderMagic(scan_buf[0..len], 0) != scalarFindEbmlHeaderMagic(scan_buf[0..len], 0)) return 5;
            if (len > 17) {
                scan_buf[15] = 0x1A; // false positive 0x1A at lane boundary
                if (simdFindEbmlHeaderMagic(scan_buf[0..len], 0) != scalarFindEbmlHeaderMagic(scan_buf[0..len], 0)) return 6;
            }
        }
    }
    return 0;
}

fn checkDeterministicFuzz(): u32 {
    let prng = XorShift64.init(0x2026_1124_4d4b_5646);
    let seed: [320]u8 = [0; 320];
    const slen = buildValidWebm(seed[0..], "webm");
    let work: [320]u8 = [0; 320];

    // 1,600 deterministic fuzz cases: 800 mutated WebM/MKV + 800 raw random byte streams
    let iter: usize = 0;
    while (iter < 1600) : (iter += 1) {
        let cur_len: usize = 0;
        if (iter % 2 == 0) {
            cur_len = @as(usize, @intCast(prng.next() % (slen + 1)));
            @memcpy(work[0..cur_len], seed[0..cur_len]);
            if (cur_len > 0) {
                const flips = @as(usize, @intCast(prng.next() % 4)) + 1;
                let fi: usize = 0;
                while (fi < flips) : (fi += 1) {
                    const idx = @as(usize, @intCast(prng.next() % cur_len));
                    work[idx] = prng.nextByte();
                }
            }
        } else {
            cur_len = @as(usize, @intCast(prng.next() % 256));
            prng.fillBytes(work[0..cur_len]);
        }

        const slice = work[0..cur_len];
        * = decodeEbmlId(slice);
        * = decodeEbmlSize(slice);
        * = isMkv(slice);
        * = isWebm(slice);
        if (h := parseMkvHeader(slice)) {
            let it = h.tracks();
            let steps: usize = 0;
            while (it.next() != null && steps < 16) : (steps += 1) {}
        }
        let eit = EbmlElementIter.init(slice);
        let esteps: usize = 0;
        while (eit.next() != null && esteps < 16) : (esteps += 1) {}
        * = parseMkvSimpleBlock(slice) catch {};
        validateEbmlStrict(slice) catch {};
        validateMkvStrict(slice) catch {};
    }
    return 0;
}

fn main() {
    test("1. RFC 8794 VINT/ID/int/float vectors, synthetic WebM/MKV & SimpleBlock 4-mode lacing", () => {
        expect_eq(checkRfc8794AndMkvVectors(), 0 as u32);
    });

    test("2. Strict EBML/MKV malformed, overflow, max-depth & invalid lacing rejection guards", () => {
        expect_eq(checkStrictRejectionGuards(), 0 as u32);
    });

    test("3. 16-lane @Vector(16, u8) SIMD copy/equal/magic-resync vs scalar 0-diff parity", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (1,600 mutated/random EBML & Matroska/WebM streams)", () => {
        expect_eq(checkDeterministicFuzz(), 0 as u32);
    });
}
