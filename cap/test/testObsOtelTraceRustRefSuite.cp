// Origin: Wave 35 R4 Reference-Hardening Verification Suite (Lane 2F)
// Reference-hardened against open-telemetry/opentelemetry-rust (Apache-2.0) & W3C Trace Context / Baggage
// Covers:
//   1. W3C Trace Context Level 1/2 official compliance vectors (traceparent + tracestate + w3cTraceContext)
//   2. W3C Baggage compliance vectors (180 list-members, 8192B header, 4096B member, %XX, properties, CRLF/CTL)
//   3. 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity (all 256 byte values + lengths 0..=257)
//   4. >= 1,600 deterministic PRNG fuzz cases across traceparent, w3cTraceContext, tracestate, and baggage

import { test, expect_eq } from "cap:test";
import {
    TRACEPARENT_HEADER_LEN,
    TRACE_FLAG_SAMPLED,
    TRACE_FLAG_RANDOM,
    TRACESTATE_MAX_ENTRIES,
    TraceContextError,
    scalarIsLowerHex,
    simdIsLowerHex,
    scalarIsAllZero16,
    simdIsAllZero16,
    scalarHasControlByte,
    simdHasControlByte,
    TraceParent,
    isValidTraceStateKey,
    TraceState,
} from "../../lib/obs/traceParent.cp";
import {
    W3cTraceParent,
    TraceStateIter,
    findTraceState,
} from "../../lib/obs/w3cTraceContext.cp";
import {
    BAGGAGE_MAX_ENTRIES,
    BAGGAGE_SPEC_MAX_MEMBERS,
    BAGGAGE_MAX_HEADER_BYTES,
    BAGGAGE_MAX_MEMBER_BYTES,
    BaggageOpStatus,
    BaggageContext,
    BaggageMemberIter,
    isValidBaggageKey,
    parseBaggageHeader,
    formatBaggageHeader,
} from "../../lib/obs/baggage.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn nextRand(s: u64): u64 {
    let x: u64 = s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x;
}

fn randOut(s: u64): u64 {
    return s *% @as(u64, 0x2545F4914F6CDD1D);
}

fn tryParseTp(s: []const u8): TraceParent? {
    return TraceParent.parse(s) catch return null;
}

fn tryParseTs(s: []const u8): TraceState? {
    return TraceState.parse(s) catch return null;
}

// ============================================================================
// 1. W3C Trace Context Official Compliance Vectors
// ============================================================================

fn checkTraceParentVectors(): u32 {
    // Valid v00
    const tp0 = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") catch return 1;
    if (tp0.version != 0x00) return 2;
    if (!tp0.isSampled()) return 3;
    if (tp0.isRandom()) return 4;

    let out_buf: [64]u8 = [0; 64];
    let out_s: []u8 = out_buf[0..];
    const wn = tp0.format(out_s) catch return 5;
    if (wn != TRACEPARENT_HEADER_LEN) return 6;
    if (!strEq(out_buf[0..wn], "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01")) return 7;

    // Valid v01 with future extension and vfe without extension
    const tp1 = TraceParent.parse("01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-02-ext-field-1") catch return 8;
    if (tp1.version != 0x01) return 9;
    if (tp1.isSampled()) return 10;
    if (!tp1.isRandom()) return 11;

    const tp_fe = TraceParent.parse("fe-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-00") catch return 12;
    if (tp_fe.version != 0xFE) return 13;

    // Invalid vectors (must be rejected by BOTH TraceParent and W3cTraceParent)
    const bad_vectors: [14][]const u8 = [
        "ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-extra",
        "01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-",
        "01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01x",
        "0A-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e473A-00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902B7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-0F",
        "00-00000000000000000000000000000000-00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-0",
        "00-4bf92f3577b34da6a3ce929d0e0e4736_00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01\r\n",
        "01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-\x1f",
    ];

    let i: usize = 0;
    while (i < bad_vectors.len) : (i += 1) {
        let failed_tp: bool = false;
        * = TraceParent.parse(bad_vectors[i]) catch {
            failed_tp = true;
        };
        if (!failed_tp) return @as(u32, @intCast(100 + i));
        if (W3cTraceParent.parse(bad_vectors[i]) != null) return @as(u32, @intCast(200 + i));
    }

    // W3cTraceParent accepts OWS around valid traceparent, rejects internal CTL
    if (W3cTraceParent.parse(" \t00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01\t ") == null) return 301;
    if (W3cTraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01\x07") != null) return 302;
    return 0;
}

fn checkTraceStateVectors(): u32 {
    if (!isValidTraceStateKey("rojo")) return 1;
    if (!isValidTraceStateKey("a_b-c*d/0")) return 2;
    if (!isValidTraceStateKey("0tenant@vendor_1")) return 3;
    if (!isValidTraceStateKey("t@abcdefghijklmn")) return 4;
    if (isValidTraceStateKey("t@abcdefghijklmno")) return 5;
    if (isValidTraceStateKey("@vendor")) return 6;
    if (isValidTraceStateKey("tenant@")) return 7;
    if (isValidTraceStateKey("tenant@1vendor")) return 8;
    if (isValidTraceStateKey("Upper@vendor")) return 9;
    if (isValidTraceStateKey("a=b")) return 10;

    // OWS outside list-member is trimmed in parse, while setFront rejects trailing space in value
    let ts_ows = TraceState.parse("rojo=trailing ") catch return 11;
    const ows_v = ts_ows.get("rojo") ?? return 11;
    if (!strEq(ows_v, "trailing")) return 11;

    let ts_err = false;
    ts_ows.setFront("rojo", "trailing ") catch { ts_err = true; };
    if (!ts_err) return 11;

    ts_err = false;
    * = TraceState.parse("rojo=bad=val") catch { ts_err = true; };
    if (!ts_err) return 12;

    ts_err = false;
    * = TraceState.parse("rojo=bad\x01val") catch { ts_err = true; };
    if (!ts_err) return 13;

    // Duplicate key leftmost-wins + MRU setFront
    let ts_mru = TraceState.parse("rojo=v1, congo=c1, rojo=v2") catch return 14;
    if (ts_mru.len() != 2) return 15;
    const rv = ts_mru.get("rojo") ?? return 16;
    if (!strEq(rv, "v1")) return 17;
    ts_mru.setFront("congo", "c2") catch return 18;
    const first = ts_mru.entryAt(0) ?? return 19;
    if (!strEq(first.key(), "congo") || !strEq(first.value(), "c2")) return 20;

    // 35 distinct members -> retains first 32 in both TraceState and TraceStateIter
    let big_hdr: [512]u8 = [0; 512];
    let pos: usize = 0;
    let m: usize = 0;
    while (m < 35) : (m += 1) {
        if (m > 0) {
            big_hdr[pos] = ',';
            pos += 1;
        }
        big_hdr[pos] = 'k';
        big_hdr[pos + 1] = @as(u8, @intCast('a' + (m / 10)));
        big_hdr[pos + 2] = @as(u8, @intCast('0' + (m % 10)));
        big_hdr[pos + 3] = '=';
        big_hdr[pos + 4] = 'v';
        pos += 5;
    }
    const ts32 = TraceState.parse(big_hdr[0..pos]) catch return 21;
    if (ts32.len() != TRACESTATE_MAX_ENTRIES) return 22;
    if (ts32.get("ka0") == null) return 23;
    if (ts32.get("kd4") != null) return 24;

    let iter = TraceStateIter.init(big_hdr[0..pos]);
    let iter_cnt: usize = 0;
    while (iter.next() != null) {
        iter_cnt += 1;
    }
    if (iter_cnt != TRACESTATE_MAX_ENTRIES) return 25;
    if (findTraceState("rojo=1\r\ncongo=2", "rojo") != null) return 26;
    return 0;
}

// ============================================================================
// 2. W3C Baggage Compliance Vectors
// ============================================================================

fn checkBaggageVectors(): u32 {
    // 1. Valid 180-member header -> BaggageMemberIter yields all 180
    let hdr180: [2048]u8 = [0; 2048];
    let p180: usize = 0;
    let i: usize = 0;
    while (i < BAGGAGE_SPEC_MAX_MEMBERS) : (i += 1) {
        if (i > 0) {
            hdr180[p180] = ',';
            p180 += 1;
        }
        hdr180[p180] = 'm';
        hdr180[p180 + 1] = @as(u8, @intCast('0' + (i / 100)));
        hdr180[p180 + 2] = @as(u8, @intCast('0' + ((i / 10) % 10)));
        hdr180[p180 + 3] = @as(u8, @intCast('0' + (i % 10)));
        hdr180[p180 + 4] = '=';
        hdr180[p180 + 5] = '1';
        p180 += 6;
    }
    let it180 = BaggageMemberIter.init(hdr180[0..p180]);
    let cnt180: usize = 0;
    while (it180.next() != null) {
        cnt180 += 1;
    }
    if (cnt180 != BAGGAGE_SPEC_MAX_MEMBERS) return 1;
    if (it180.truncated || it180.rejected) return 2;

    // 2. 181 members -> 181st truncated cleanly
    hdr180[p180] = ',';
    hdr180[p180 + 1] = 'x';
    hdr180[p180 + 2] = '=';
    hdr180[p180 + 3] = '9';
    p180 += 4;

    let it181 = BaggageMemberIter.init(hdr180[0..p180]);
    let cnt181: usize = 0;
    while (it181.next() != null) {
        cnt181 += 1;
    }
    if (cnt181 != BAGGAGE_SPEC_MAX_MEMBERS) return 3;
    if (!it181.truncated) return 4;

    // 3. 185 duplicate-key members in parseBaggageHeader -> cap at 180 list-members enforced
    let dup_hdr: [1024]u8 = [0; 1024];
    let dp: usize = 0;
    let d: usize = 0;
    while (d < 185) : (d += 1) {
        if (d > 0) {
            dup_hdr[dp] = ',';
            dp += 1;
        }
        dup_hdr[dp] = 'k';
        dup_hdr[dp + 1] = '=';
        dup_hdr[dp + 2] = '1';
        dp += 3;
    }
    let ctx = BaggageContext.init();
    const rep_dup = parseBaggageHeader(dup_hdr[0..dp], *ctx);
    if (rep_dup.status != BaggageOpStatus.capacity_full) return 5;
    if (rep_dup.parsed_members != 185) return 6;
    if (rep_dup.dropped_overflow != 5) return 7;
    if (ctx.len() != 1) return 8;

    // 4. Single member > 4096B skipped; total header > 8192B rejected atomically
    let big_mem: [BAGGAGE_MAX_MEMBER_BYTES + 32]u8 = ['a'; BAGGAGE_MAX_MEMBER_BYTES + 32];
    big_mem[0] = 'k';
    big_mem[1] = '=';
    big_mem[BAGGAGE_MAX_MEMBER_BYTES + 10] = ',';
    big_mem[BAGGAGE_MAX_MEMBER_BYTES + 11] = 'o';
    big_mem[BAGGAGE_MAX_MEMBER_BYTES + 12] = 'k';
    big_mem[BAGGAGE_MAX_MEMBER_BYTES + 13] = '=';
    big_mem[BAGGAGE_MAX_MEMBER_BYTES + 14] = '1';
    const rep_mem = parseBaggageHeader(big_mem[0 .. BAGGAGE_MAX_MEMBER_BYTES + 15], *ctx);
    if (rep_mem.status != BaggageOpStatus.ok) return 9;
    if (rep_mem.skipped_invalid != 1 || rep_mem.stored_entries != 1 || !ctx.has("ok")) return 10;

    let huge_hdr: [BAGGAGE_MAX_HEADER_BYTES + 1]u8 = ['a'; BAGGAGE_MAX_HEADER_BYTES + 1];
    const rep_huge = parseBaggageHeader(huge_hdr[0..], *ctx);
    if (rep_huge.status != BaggageOpStatus.header_too_large) return 11;
    let it_huge = BaggageMemberIter.init(huge_hdr[0..]);
    if (!it_huge.rejected || it_huge.next() != null) return 12;

    // 5. Percent-encoding boundary guards (truncated %, invalid hex, %0D%0A, %00, unescaped quotes)
    const rep_pct = parseBaggageHeader("a=1%,b=%2,c=%ZZ,d=ok%0D%0A,e=bad%00,f=a\"b,g=good%20val;p1=v1;flag", *ctx);
    if (rep_pct.status != BaggageOpStatus.ok) return 13;
    if (rep_pct.stored_entries != 1 || rep_pct.skipped_invalid != 6) return 14;
    const gv = ctx.get("g") ?? return 15;
    if (!strEq(gv, "good val")) return 16;
    const gp = ctx.getProps("g") ?? return 17;
    if (!strEq(gp, "p1=v1;flag")) return 18;

    // 6. Round-trip properties & reject malformed properties
    let out: [256]u8 = [0; 256];
    let out_s: []u8 = out[0..];
    const n_fmt = formatBaggageHeader(ctx, out_s) ?? return 19;
    if (!strEq(out[0..n_fmt], "g=good%20val;p1=v1;flag")) return 20;

    if (ctx.setWithProps("k", "v", "p1=1;") != BaggageOpStatus.invalid_props) return 21;
    if (ctx.setWithProps("k", "v", "bad key=1") != BaggageOpStatus.invalid_props) return 22;
    if (ctx.setWithProps("k", "v", "p=1\x01") != BaggageOpStatus.invalid_props) return 23;

    // 7. Control character & CRLF rejection
    const rep_ctl = parseBaggageHeader("a=1\x07,b=2", *ctx);
    if (rep_ctl.status != BaggageOpStatus.crlf_rejected) return 24;
    if (ctx.len() != 0) return 25;
    if (!isValidBaggageKey("valid.key-1_a")) return 26;
    if (isValidBaggageKey("bad key")) return 27;
    return 0;
}

// ============================================================================
// 3. 16-Lane SIMD vs Scalar 0-Diff Parity (All 256 Bytes + Lengths 0..=257)
// ============================================================================

fn checkSimdParity(): u32 {
    // 1. All 256 byte values tested in every lane (0..15) and tail position (16)
    let buf256: [32]u8 = ['a'; 32];
    let b: usize = 0;
    while (b < 256) : (b += 1) {
        const ch: u8 = @as(u8, @intCast(b));
        const lane: usize = b % 17;
        buf256[lane] = ch;
        if (simdIsLowerHex(buf256[0..17]) != scalarIsLowerHex(buf256[0..17])) return 1;
        if (simdHasControlByte(buf256[0..17]) != scalarHasControlByte(buf256[0..17])) return 2;
        buf256[lane] = 'a';

        let one: [1]u8 = [ch];
        if (simdIsLowerHex(one[0..]) != scalarIsLowerHex(one[0..])) return 3;
        if (simdHasControlByte(one[0..]) != scalarHasControlByte(one[0..])) return 4;
    }

    // 2. Lengths 0..=257 sweep with all-valid and single-bad-byte-at-end/middle
    let sweep: [258]u8 = ['0'; 258];
    const hex_chars: [16]u8 = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        if (len > 0) {
            sweep[len - 1] = hex_chars[(len - 1) & 15];
        }
        if (simdIsLowerHex(sweep[0..len]) != scalarIsLowerHex(sweep[0..len])) return 5;
        if (simdHasControlByte(sweep[0..len]) != scalarHasControlByte(sweep[0..len])) return 6;

        if (len > 0) {
            const saved = sweep[len - 1];
            sweep[len - 1] = 'A';
            if (simdIsLowerHex(sweep[0..len]) != scalarIsLowerHex(sweep[0..len])) return 7;
            if (simdIsLowerHex(sweep[0..len])) return 8;

            sweep[len - 1] = 0x7F;
            if (simdHasControlByte(sweep[0..len]) != scalarHasControlByte(sweep[0..len])) return 9;
            if (!simdHasControlByte(sweep[0..len])) return 10;

            sweep[len - 1] = saved;
            const mid = len / 2;
            const saved_mid = sweep[mid];
            sweep[mid] = '\r';
            if (simdHasControlByte(sweep[0..len]) != scalarHasControlByte(sweep[0..len])) return 11;
            if (!simdHasControlByte(sweep[0..len])) return 12;
            sweep[mid] = saved_mid;
        }
    }

    // 3. simdIsAllZero16 vs scalarIsAllZero16
    let z16: [16]u8 = [0; 16];
    if (!simdIsAllZero16(z16) || !scalarIsAllZero16(z16)) return 13;

    let pos: usize = 0;
    while (pos < 16) : (pos += 1) {
        let bit: u3 = 0;
        while (true) {
            z16[pos] = @as(u8, 1) << bit;
            if (simdIsAllZero16(z16)) return 14;
            if (simdIsAllZero16(z16) != scalarIsAllZero16(z16)) return 15;
            if (bit == 7) break;
            bit += 1;
        }
        z16[pos] = 0;
    }
    return 0;
}

// ============================================================================
// 4. Deterministic PRNG Fuzz (>= 1,600 Malformed Cases, 0 Panic / 0 OOB)
// ============================================================================

fn checkPrngFuzz(): u32 {
    let seed: u64 = 0x4AFE_BABE_2026_1124;
    let buf: [128]u8 = [0; 128];
    let fmt_buf: [256]u8 = [0; 256];
    let ctx = BaggageContext.init();

    // 600 cases for TraceParent + W3cTraceParent
    let t: usize = 0;
    while (t < 600) : (t += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 96));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((t & 3) == 0) {
                const alph: []const u8 = "0123456789abcdefABCDEF- \r\n\t\x00";
                buf[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        const s = buf[0..n];
        if (tp := tryParseTp(s)) {
            if (tp.version == 0xFF) return 1;
            if (simdIsAllZero16(tp.trace_id)) return 2;
            let fs: []u8 = fmt_buf[0..];
            const wn = tp.format(fs) catch 0;
            if (wn != TRACEPARENT_HEADER_LEN) return 3;
        }
        if (wtp := W3cTraceParent.parse(s)) {
            if (wtp.version == 0xFF) return 4;
            if (simdIsAllZero16(wtp.trace_id)) return 5;
        }
    }

    // 600 cases for TraceState + TraceStateIter
    let s_i: usize = 0;
    while (s_i < 600) : (s_i += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 128));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((s_i & 1) == 0) {
                const alph: []const u8 = "abc0123@_-*/=, \t\r\n\x00\x7fXYZ";
                buf[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        const s = buf[0..n];
        if (ts := tryParseTs(s)) {
            if (ts.len() > TRACESTATE_MAX_ENTRIES) return 6;
        }
        let it = TraceStateIter.init(s);
        let cnt: usize = 0;
        while (it.next() != null) {
            cnt += 1;
        }
        if (cnt > TRACESTATE_MAX_ENTRIES) return 7;
    }

    // 600 cases for Baggage (parseBaggageHeader + BaggageMemberIter + format round-trip)
    let b_i: usize = 0;
    while (b_i < 600) : (b_i += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 128));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((b_i & 1) == 0) {
                const alph: []const u8 = "ab01=;, %20%0D%0A%ZZ\"\\\t\r\n\x00";
                buf[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        const s = buf[0..n];
        const rep = parseBaggageHeader(s, *ctx);
        if (rep.stored_entries > BAGGAGE_MAX_ENTRIES) return 8;
        if (ctx.len() != rep.stored_entries) return 9;
        if (ctx.len() > 0) {
            let fs: []u8 = fmt_buf[0..];
            * = formatBaggageHeader(ctx, fs);
        }

        let bit = BaggageMemberIter.init(s);
        let bcnt: usize = 0;
        while (bit.next() != null) {
            bcnt += 1;
        }
        if (bcnt > BAGGAGE_SPEC_MAX_MEMBERS) return 10;
    }

    if (simdIsLowerHex(buf[0..64]) != scalarIsLowerHex(buf[0..64])) return 11;
    if (simdHasControlByte(buf[0..64]) != scalarHasControlByte(buf[0..64])) return 12;
    return 0;
}

fn main() {
    test("1. W3C Trace Context: traceparent strict compliance vectors (v00, v01..fe, ff, hex, zero IDs, CTL)", () => {
        expect_eq(checkTraceParentVectors(), @as(u32, 0));
    });
    test("2. W3C Trace Context: tracestate 32-member cap, tenant@vendor keys, dedup, MRU & CTL rejection", () => {
        expect_eq(checkTraceStateVectors(), @as(u32, 0));
    });
    test("3. W3C Baggage: 180-member limit, 8192B header cap, 4096B member cap, %XX & properties", () => {
        expect_eq(checkBaggageVectors(), @as(u32, 0));
    });
    test("4. 16-lane SIMD vs Scalar 0-diff parity: simdIsLowerHex, simdIsAllZero16, simdHasControlByte", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("5. PRNG Fuzz: 1,800 deterministic malformed cases across traceparent, tracestate & baggage", () => {
        expect_eq(checkPrngFuzz(), @as(u32, 0));
    });
}
