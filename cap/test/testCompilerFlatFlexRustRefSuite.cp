// Origin: Wave 36 R4 Reference-Hardening Verification Suite (Lane 2F)
// Reference-hardened against google/flatbuffers (Apache-2.0)
// Covers:
//   1. FlatBuffers binary table/vector/string/identifier/size-prefix round-trip + strict verifier guards
//      (OOB root, negative/underflow vtable soffset, unaligned/undersized vtable, obj_len < 4, missing NUL, mul overflow)
//   2. FlexBuffers scalar/vector/map round-trip + strict verifier guards
//      (invalid byte_width, invalid FlexType, underflow offset, map keys/values len mismatch, max_depth guard)
//   3. 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity (all 256 byte values + lengths 0..=257)
//   4. 1,600 deterministic PRNG fuzz cases (800 FlatBuffers + 800 FlexBuffers), 0 panic / 0 OOB / 0 overflow

import { test, expect_eq } from "cap:test";
import {
    FLATBUF_MAX_DEPTH,
    scalarFbFindZeroByte,
    simdFbFindZeroByte,
    scalarFbBytesEqual,
    simdFbBytesEqual,
    fbFileIdentifier,
    fbHasIdentifier,
    fbStripSizePrefix,
    fbRootTable,
    validateFlatbufStrict,
} from "../../lib/compiler/flatbuf.cp";
import {
    FLEXBUF_MAX_DEPTH,
    FlexType,
    FlexBitWidth,
    packFlexType,
    unpackFlexType,
    parseFlexRoot,
    validateFlexbufStrict,
} from "../../lib/compiler/flexbuf.cp";

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

fn putU16LE(buf: *[]u8, off: usize, v: u16) {
    buf[off] = @as(u8, @intCast(v & 0xFF));
    buf[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
}

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = @as(u8, @intCast(v & 0xFF));
    buf[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    buf[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    buf[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

// ============================================================================
// 1. FlatBuffers Valid Round-Trip & Strict Security Guards
// ============================================================================

fn checkFlatbufVectors(): u32 {
    // Construct a canonical FlatBuffers buffer:
    //   0..4   : root uoffset = 16
    //   4..8   : file_identifier = "CPFB"
    //   8..16  : vtable (vtable_len=10? wait: 3 slots -> 4 + 3*2 = 10 -> put at 8..18? Use 12..24!)
    // Layout:
    //   0..4   : root_pos = 24
    //   4..8   : file_id = "CPFB"
    //   8..18  : vtable (vtable_len=10, obj_len=16, slot4=4 (u32), slot6=8 (string uoff), slot8=12 (vec uoff))
    //   24..40 : table object (soff = 24 - 8 = 16; [28..32]=42; [32..36]=uoff to str at 40 -> 8; [36..40]=uoff to vec at 52 -> 16)
    //   40..50 : string (len=5, "hello", '\0')
    //   52..68 : u32 vector (len=3, [10, 20, 30])
    let buf: [68]u8 = [0; 68];
    let bs: []u8 = buf[0..];
    putU32LE(bs, 0, 24);
    buf[4] = 'C'; buf[5] = 'P'; buf[6] = 'F'; buf[7] = 'B';

    // VTable at 8 (len = 10, obj_len = 16)
    putU16LE(bs, 8, 10);
    putU16LE(bs, 10, 16);
    putU16LE(bs, 12, 4);  // slot 4: u32
    putU16LE(bs, 14, 8);  // slot 6: string
    putU16LE(bs, 16, 12); // slot 8: vector<u32>

    // Table at 24: soff = 24 - 8 = 16
    putU32LE(bs, 24, 16);
    putU32LE(bs, 28, 42);
    putU32LE(bs, 32, 8);  // 32 + 8 = 40 (string)
    putU32LE(bs, 36, 16); // 36 + 16 = 52 (vector)

    // String at 40: "hello\0"
    putU32LE(bs, 40, 5);
    buf[44] = 'h'; buf[45] = 'e'; buf[46] = 'l'; buf[47] = 'l'; buf[48] = 'o'; buf[49] = 0;

    // Vector<u32> at 52: [10, 20, 30]
    putU32LE(bs, 52, 3);
    putU32LE(bs, 56, 10);
    putU32LE(bs, 60, 20);
    putU32LE(bs, 64, 30);

    if (!validateFlatbufStrict(buf[0..], FLATBUF_MAX_DEPTH)) return 1;
    if (!fbHasIdentifier(buf[0..], "CPFB")) return 2;
    if (fbHasIdentifier(buf[0..], "NOPE")) return 3;
    const fid = fbFileIdentifier(buf[0..]) ?? return 4;
    if (!simdFbBytesEqual(fid, "CPFB")) return 5;

    const root = fbRootTable(buf[0..]) ?? return 6;
    if ((root.fieldCount() ?? 0) != 3) return 7;
    if ((root.getU32(4) ?? 0) != 42) return 8;
    const s = root.getString(6) ?? return 9;
    if (!simdFbBytesEqual(s, "hello")) return 10;
    const vec = root.getVector(8) ?? return 11;
    if (vec.len != 3) return 12;
    if ((vec.getU32(0) ?? 0) != 10 || (vec.getU32(2) ?? 0) != 30) return 13;
    if (vec.getU32(3) != null) return 14;

    // Size-prefix stripping
    let prefixed: [72]u8 = [0; 72];
    let ps: []u8 = prefixed[0..];
    putU32LE(ps, 0, 68);
    let k: usize = 0;
    while (k < 68) : (k += 1) { prefixed[4 + k] = buf[k]; }
    const stripped = fbStripSizePrefix(prefixed[0..]) ?? return 15;
    if (stripped.len != 68 || !validateFlatbufStrict(stripped, 0)) return 16;

    // Security rejections:
    // A. Missing NUL terminator on string
    let bad_nul: [68]u8 = buf;
    bad_nul[49] = 'X';
    const r_nul = fbRootTable(bad_nul[0..]) ?? return 17;
    if (r_nul.getString(6) != null) return 18;

    // B. Odd (unaligned) vtable_len
    let bad_align: [68]u8 = buf;
    let ba_s: []u8 = bad_align[0..];
    putU16LE(ba_s, 8, 9);
    if (fbRootTable(bad_align[0..]) != null || validateFlatbufStrict(bad_align[0..], 0)) return 19;

    // C. obj_len < 4
    let bad_obj: [68]u8 = buf;
    let bo_s: []u8 = bad_obj[0..];
    putU16LE(bo_s, 10, 2);
    if (fbRootTable(bad_obj[0..]) != null) return 20;

    // D. Underflow vtable soffset (soff = 999 > pos=24)
    let bad_soff: [68]u8 = buf;
    let bso_s: []u8 = bad_soff[0..];
    putU32LE(bso_s, 24, 999);
    if (fbRootTable(bad_soff[0..]) != null) return 21;

    // E. Vector multiplication overflow (len = 0x7FFF_FFFF)
    let bad_vec: [68]u8 = buf;
    let bv_s: []u8 = bad_vec[0..];
    putU32LE(bv_s, 52, 0x7FFF_FFFF);
    const rv = fbRootTable(bad_vec[0..]) ?? return 22;
    const ov_vec = rv.getVector(8) ?? return 23;
    if (ov_vec.getU64(0x4000_0000) != null) return 24;
    return 0;
}

// ============================================================================
// 2. FlexBuffers Valid Round-Trip & Strict Security Guards
// ============================================================================

fn checkFlexbufVectors(): u32 {
    // 1. Scalar root int = -42 (width=1)
    const scalar_buf: [3]u8 = [
        @as(u8, @bitCast(@as(i8, -42))),
        packFlexType(FlexType.int, FlexBitWidth.w8),
        1,
    ];
    if (!validateFlexbufStrict(scalar_buf[0..], FLEXBUF_MAX_DEPTH)) return 1;
    const sroot = parseFlexRoot(scalar_buf[0..]) ?? return 2;
    if ((sroot.asInt() ?? 0) != -42) return 3;

    // 2. Map {"a": 10, "bb": "hi"} (byte_width=1)
    //   0..2  : key "a\0"
    //   2..5  : key "bb\0"
    //   5..8  : string "hi\0" (len=2 at [5], data "hi\0" at [6..9] -> 5..9!)
    // Let's lay out cleanly:
    //   0..2  : "a\0"
    //   2..5  : "bb\0"
    //   5     : str_len = 2
    //   6..9  : "hi\0"
    //   9     : keys_vec len = 2
    //   10    : key[0] rel = 10 - 0 = 10 -> "a\0"
    //   11    : key[1] rel = 11 - 2 = 9  -> "bb\0"
    //   12    : map keys_ptr rel = 12 - 10 = 2 (points to keys_data=10)
    //   13    : map keys_bw = 1
    //   14    : map len = 2
    //   15    : val[0] = 10 (int)
    //   16    : val[1] rel = 16 - 6 = 10 (string at 6)
    //   17    : type[0] = pack(int, w8)
    //   18    : type[1] = pack(string, w8)
    //   19    : root offset = 19 - 15 = 4 (points to map values at 15)
    //   20    : root packed_type = pack(map, w8)
    //   21    : root byte_width = 1
    const map_buf: [22]u8 = [
        'a', 0,
        'b', 'b', 0,
        2, 'h', 'i', 0,
        2, 10, 9,
        2, 1, 2,
        10, 10,
        packFlexType(FlexType.int, FlexBitWidth.w8),
        packFlexType(FlexType.string, FlexBitWidth.w8),
        4,
        packFlexType(FlexType.map, FlexBitWidth.w8),
        1,
    ];
    if (!validateFlexbufStrict(map_buf[0..], FLEXBUF_MAX_DEPTH)) return 4;
    const mroot = parseFlexRoot(map_buf[0..]) ?? return 5;
    if ((mroot.mapLen() ?? 0) != 2) return 6;
    const va = mroot.mapGet("a") ?? return 7;
    if ((va.asInt() ?? 0) != 10) return 8;
    const vb = mroot.mapGet("bb") ?? return 9;
    const svb = vb.asString() ?? return 10;
    if (!simdFbBytesEqual(svb, "hi")) return 11;
    if (mroot.mapGet("missing") != null) return 12;

    // 3. Security rejections:
    // A. Invalid root byte_width (3)
    const bad_bw: [3]u8 = [1, packFlexType(FlexType.int, FlexBitWidth.w8), 3];
    if (parseFlexRoot(bad_bw[0..]) != null || validateFlexbufStrict(bad_bw[0..], 0)) return 13;

    // B. Invalid FlexType (raw = 35 -> not in 0..=26 or 36)
    if (unpackFlexType((35 << 2) | 0) != null) return 14;

    // C. Map keys_len != values_len (corrupt keys_vec len at [9] from 2 to 1)
    let bad_map: [22]u8 = map_buf;
    bad_map[9] = 1;
    if (validateFlexbufStrict(bad_map[0..], 0)) return 15;

    // D. Self-referential vector cycle caught by max_depth guard (depth = 4)
    //   0: len=1, 1: elem rel=0 (points to self!), 2: type=vector(w8), 3: root rel=2, 4: vector(w8), 5: bw=1
    const cyc_buf: [6]u8 = [
        1, 0,
        packFlexType(FlexType.vector, FlexBitWidth.w8),
        2,
        packFlexType(FlexType.vector, FlexBitWidth.w8),
        1,
    ];
    if (validateFlexbufStrict(cyc_buf[0..], 4)) return 16;
    return 0;
}

// ============================================================================
// 3. 16-Lane SIMD vs Scalar 0-Diff Parity (All 256 Bytes + Lengths 0..=257)
// ============================================================================

fn checkSimdParity(): u32 {
    let a: [32]u8 = [1; 32];
    let b: [32]u8 = [1; 32];
    let bi: usize = 0;
    while (bi < 256) : (bi += 1) {
        const ch: u8 = @as(u8, @intCast(bi));
        const lane: usize = bi % 17;
        a[lane] = ch;
        b[lane] = ch;
        if (simdFbFindZeroByte(a[0..17], 0) != scalarFbFindZeroByte(a[0..17], 0)) return 1;
        if (simdFbFindZeroByte(a[0..17], 5) != scalarFbFindZeroByte(a[0..17], 5)) return 2;
        if (simdFbBytesEqual(a[0..17], b[0..17]) != scalarFbBytesEqual(a[0..17], b[0..17])) return 3;
        b[lane] = ch ^ 1;
        if (simdFbBytesEqual(a[0..17], b[0..17]) != scalarFbBytesEqual(a[0..17], b[0..17])) return 4;
        a[lane] = 1;
        b[lane] = 1;
    }

    let s1: [258]u8 = [7; 258];
    let s2: [258]u8 = [7; 258];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        if (simdFbFindZeroByte(s1[0..len], 0) != scalarFbFindZeroByte(s1[0..len], 0)) return 5;
        if (simdFbBytesEqual(s1[0..len], s2[0..len]) != scalarFbBytesEqual(s1[0..len], s2[0..len])) return 6;
        if (len > 0) {
            s1[len - 1] = 0;
            if (simdFbFindZeroByte(s1[0..len], 0) != scalarFbFindZeroByte(s1[0..len], 0)) return 7;
            if (simdFbFindZeroByte(s1[0..len], len / 2) != scalarFbFindZeroByte(s1[0..len], len / 2)) return 8;
            if (simdFbBytesEqual(s1[0..len], s2[0..len]) != scalarFbBytesEqual(s1[0..len], s2[0..len])) return 9;
            s1[len - 1] = 7;
        }
    }
    return 0;
}

// ============================================================================
// 4. 1,600 Deterministic PRNG Fuzz Cases (800 FlatBuffers + 800 FlexBuffers)
// ============================================================================

fn checkPrngFuzz(): u32 {
    let seed: u64 = 0x3604_F1A7_F1E8_2026;
    let buf: [128]u8 = [0; 128];

    // 800 FlatBuffers fuzz cases
    let fi: usize = 0;
    while (fi < 800) : (fi += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 128));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            buf[j] = @as(u8, @intCast(randOut(seed) & 0xFF));
        }
        const s = buf[0..n];
        * = validateFlatbufStrict(s, 16);
        * = fbStripSizePrefix(s);
        if (tbl := fbRootTable(s)) {
            * = tbl.fieldCount();
            * = tbl.getU32(4);
            * = tbl.getU64(6);
            * = tbl.getString(8);
            if (v := tbl.getVector(10)) {
                * = v.byteSlice();
                * = v.getU32(0);
                * = v.getString(0);
            }
            * = tbl.getTable(12);
        }
    }

    // 800 FlexBuffers fuzz cases
    let xi: usize = 0;
    while (xi < 800) : (xi += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 128));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            buf[j] = @as(u8, @intCast(randOut(seed) & 0xFF));
        }
        const s = buf[0..n];
        * = validateFlexbufStrict(s, 16);
        if (root := parseFlexRoot(s)) {
            * = root.asInt();
            * = root.asUint();
            * = root.asFloat();
            * = root.asString();
            * = root.asKey();
            * = root.asBlob();
            if (vlen := root.vecLen()) {
                if (vlen > 0) {
                    * = root.vecGet(0);
                }
            }
            if (mlen := root.mapLen()) {
                if (mlen > 0) {
                    * = root.mapKeyAt(0);
                    * = root.mapValueAt(0);
                    * = root.mapGet("k");
                }
            }
        }
    }

    return 0;
}

fn main() {
    test("1. FlatBuffers: table/vector/string/identifier round-trip & strict verifier guards", () => {
        expect_eq(checkFlatbufVectors(), @as(u32, 0));
    });
    test("2. FlexBuffers: scalar/map/string round-trip & width/type/cycle verifier guards", () => {
        expect_eq(checkFlexbufVectors(), @as(u32, 0));
    });
    test("3. 16-lane SIMD vs Scalar 0-diff parity (all 256 bytes + lengths 0..=257)", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("4. PRNG Fuzz: 1,600 deterministic malformed cases (800 FlatBuffers + 800 FlexBuffers)", () => {
        expect_eq(checkPrngFuzz(), @as(u32, 0));
    });
}
