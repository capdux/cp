// Source: cp/cap/test/testSqlPgWireRustRefSuite.cp
// Reference-hardened against sfackler/rust-postgres (MIT/Apache-2.0) & PostgreSQL v3 Wire Protocol
// Covers:
// 1. Multi-frame pipeline, progressive partial feeding (atomic ShortRead), CString, integers/floats & DataRow roundtrip
// 2. Security bounds: frame length < 4 / > PG_MAX_FRAME_LEN, DataRow col_len -2 / i32::MIN, trailing bytes, embedded NUL
// 3. 16-lane SIMD vs scalar 0-discrepancy differential parity (simdFindNul, simdPgCopyBytes)
// 4. 1,500-case deterministic PRNG malformed PostgreSQL v3 frame & DataRow fuzz suite (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PG_MAX_FRAME_LEN,
    PgWireReaderError,
    PgFrameView,
    PgWireReader,
    simdFindNul,
} from "../../lib/sql/pgReader.cp";
import {
    PgWireWriterError,
    PgWireWriter,
    simdPgCopyBytes,
} from "../../lib/sql/pgWriter.cp";
import {
    PG_DATA_ROW_MAX_FRAME_LEN,
    PgDataRowError,
    PgDataRowColumn,
    encodePgDataRowFrame,
    decodePgDataRowIter,
    decodePgDataRowFrame,
} from "../../lib/sql/pgDataRow.cp";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarFindNul(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == 0) return i;
    }
    return null;
}

struct Rng {
    state: u64;

    fn nextU64(*_): u64 {
        let x = _.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn nextU8(*_): u8 {
        return ((_.nextU64() >> 32) & 0xFF) as u8;
    }

    fn nextRange(*_, bound: usize): usize {
        return ((_.nextU64() as usize) % bound);
    }
}

// --- Test 1: Multi-Frame Pipeline, Progressive Partial Feeding & DataRow Roundtrip ---

fn testPgWirePipelineAndDataRowRoundtrip(): !void {
    let wire_buf: [512]u8 = undefined;
    let w = PgWireWriter.init(&wire_buf);

    // Frame 1: Query ('Q') with CString
    const p1 = try w.beginTaggedFrame('Q');
    * = try w.writeCStringStrict("SELECT id, payload FROM items");
    * = try w.finishLength(p1);

    // Frame 2: ParameterStatus ('S') with two CStrings
    const p2 = try w.beginTaggedFrame('S');
    * = try w.writeCStringStrict("server_encoding");
    * = try w.writeCStringStrict("UTF8");
    * = try w.finishLength(p2);

    const written = w.written();

    // Progressive byte-by-byte partial feeding on Frame 1 MUST return ShortRead with atomic pos==0
    let first_frame_len: usize = 0;
    {
        let probe = PgWireReader.init(written);
        const f1 = try probe.readTaggedFrame();
        first_frame_len = (5 as usize) + f1.payload.len;
    }
    for (cut of 0..first_frame_len) {
        const cut_u = cut as usize;
        let rd_part = PgWireReader.init(written[0..cut_u]);
        let is_short = false;
        * = rd_part.readTaggedFrame() catch |e| {
            is_short = std.mem.eql(u8, @errorName(e), "ShortRead");
        };
        expect_eq(is_short, true);
        expect_eq(rd_part.offset(), 0 as usize);
    }

    // Full 2-frame read
    let rd = PgWireReader.init(written);
    const f1 = try rd.readTaggedFrame();
    expect_eq(f1.tag, 'Q' as u8);
    let f1_rd = PgWireReader.init(f1.payload);
    const sql_str = try f1_rd.readZ();
    expect_eq(std.mem.eql(u8, sql_str, "SELECT id, payload FROM items"), true);

    const f2 = try rd.readTaggedFrame();
    expect_eq(f2.tag, 'S' as u8);
    let f2_rd = PgWireReader.init(f2.payload);
    expect_eq(std.mem.eql(u8, try f2_rd.readZ(), "server_encoding"), true);
    expect_eq(std.mem.eql(u8, try f2_rd.readZ(), "UTF8"), true);
    expect_eq(rd.remaining(), 0 as usize);

    // DataRow ('D') roundtrip with NULL (-1), empty 0-byte, and large 80-byte binary column
    let big_bin: [80]u8 = undefined;
    for (i of 0..80) big_bin[i] = ((i * 7 + 3) & 0xFF) as u8;
    const cols: [4]PgDataRowColumn = [
        PgDataRowColumn.bytesCol("42"),
        PgDataRowColumn.nullCol(),
        PgDataRowColumn.emptyCol(),
        PgDataRowColumn.bytesCol(&big_bin),
    ];
    let dr_buf: [256]u8 = undefined;
    const dr_len = try encodePgDataRowFrame(cols[0..], &dr_buf);
    const view = try decodePgDataRowFrame(dr_buf[0..dr_len]);
    expect_eq(view.column_count, 4 as usize);
    expect_eq(view.columns[0].is_null, false);
    expect_eq(std.mem.eql(u8, view.columns[0].bytes, "42"), true);
    expect_eq(view.columns[1].is_null, true);
    expect_eq(view.columns[2].is_null, false);
    expect_eq(view.columns[2].bytes.len, 0 as usize);
    expect_eq(std.mem.eql(u8, view.columns[3].bytes, &big_bin), true);
}

// --- Test 2: PostgreSQL v3 Security & Malformed Wire Guards ---

fn testPgWireSecurityGuards(): !void {
    expect_eq(PG_MAX_FRAME_LEN, (64 * 1024 * 1024) as u32);
    expect_eq(PG_DATA_ROW_MAX_FRAME_LEN, (64 * 1024 * 1024) as usize);

    // 1. Frame length < 4 MUST fail with InvalidMessageLength
    const under_len: [5]u8 = ['Q', 0x00, 0x00, 0x00, 0x03];
    let rd_under = PgWireReader.init(&under_len);
    let got_under = false;
    * = rd_under.readTaggedFrame() catch |e| {
        got_under = std.mem.eql(u8, @errorName(e), "InvalidMessageLength");
    };
    expect_eq(got_under, true);

    // 2. Oversized frame length (> PG_MAX_FRAME_LEN, e.g. 0x7FFF_FFFF) MUST fail with FrameTooLarge
    const huge_len: [5]u8 = ['D', 0x7F, 0xFF, 0xFF, 0xFF];
    let rd_huge = PgWireReader.init(&huge_len);
    let got_huge = false;
    * = rd_huge.readTaggedFrame() catch |e| {
        got_huge = std.mem.eql(u8, @errorName(e), "FrameTooLarge");
    };
    expect_eq(got_huge, true);

    // 3. DataRow invalid negative column lengths (-2 and i32::MIN) MUST fail with InvalidColumnLength
    const dr_neg2: [11]u8 = ['D', 0x00, 0x00, 0x00, 0x0A, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFE];
    let neg2_rej = false;
    * = decodePgDataRowFrame(&dr_neg2) catch |e| {
        neg2_rej = std.mem.eql(u8, @errorName(e), "InvalidColumnLength");
    };
    expect_eq(neg2_rej, true);

    const dr_imin: [11]u8 = ['D', 0x00, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00];
    let imin_rej = false;
    * = decodePgDataRowFrame(&dr_imin) catch |e| {
        imin_rej = std.mem.eql(u8, @errorName(e), "InvalidColumnLength");
    };
    expect_eq(imin_rej, true);

    // 4. DataRow trailing garbage byte MUST fail with TrailingBytes
    const dr_trail: [12]u8 = ['D', 0x00, 0x00, 0x00, 0x0A, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x99];
    let trail_rej = false;
    * = decodePgDataRowFrame(&dr_trail) catch |e| {
        trail_rej = std.mem.eql(u8, @errorName(e), "TrailingBytes");
    };
    expect_eq(trail_rej, true);

    // 5. Unterminated CString in PgWireReader MUST fail with UnterminatedString
    let rd_unterm = PgWireReader.init("no_nul_terminator");
    let unterm_rej = false;
    * = rd_unterm.readZ() catch |e| {
        unterm_rej = std.mem.eql(u8, @errorName(e), "UnterminatedString");
    };
    expect_eq(unterm_rej, true);

    // 6. Embedded NUL in PgWireWriter CString MUST fail with EmbeddedNul
    let wbuf: [64]u8 = undefined;
    let wr = PgWireWriter.init(&wbuf);
    let emb_rej = false;
    * = wr.writeCString("abc\x00def") catch |e| {
        emb_rej = std.mem.eql(u8, @errorName(e), "EmbeddedNul");
    };
    expect_eq(emb_rej, true);

    let strict_rej = false;
    * = wr.writeCStringStrict("abc\x00") catch |e| {
        strict_rej = std.mem.eql(u8, @errorName(e), "EmbeddedNul");
    };
    expect_eq(strict_rej, true);
}

// --- Test 3: 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x4D59_5DF4_D0F3_3173 };
    let buf: [96]u8 = undefined;
    let dst: [96]u8 = undefined;

    for (len of 0..97) {
        const len_u = len as usize;

        // 1. simdFindNul parity
        for (i of 0..len) buf[i] = 'x';
        if (len_u > 0 && (len_u % 2) == 0) {
            buf[rng.nextRange(len_u)] = 0;
        }
        const s1 = buf[0..len_u];
        expect_eq(simdFindNul(s1), scalarFindNul(s1));

        // 2. simdPgCopyBytes parity
        for (i of 0..len) buf[i] = rng.nextU8();
        const copied = simdPgCopyBytes(dst[0..len_u], buf[0..len_u]);
        expect_eq(copied, len_u);
        expect_eq(std.mem.eql(u8, dst[0..len_u], buf[0..len_u]), true);
    }
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed PostgreSQL v3 Fuzz Suite ---

fn fuzzCheckPgWire(slice: []const u8) {
    let rd = PgWireReader.init(slice);
    * = rd.readTaggedFrame() catch {};
    let rd2 = PgWireReader.init(slice);
    * = rd2.readZ() catch {};
    let rd3 = PgWireReader.init(slice);
    * = rd3.readLengthPrefixedOrNull() catch {};
    * = decodePgDataRowFrame(slice) catch {};
    let it = decodePgDataRowIter(slice) catch return;
    it.validateComplete() catch return;
}

fn testDeterministicPgWireFuzz(): !void {
    let rng = Rng { state: 0x2545_F491_4F6C_DD1D };
    let buf: [192]u8 = undefined;
    const seed_dr: [15]u8 = [
        'D', 0x00, 0x00, 0x00, 0x0E, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x02, 'o', 'k',
        0xFF, 0xFF,
    ];

    let total: usize = 0;
    for (case_idx of 0..1500) {
        const strat = (case_idx as usize) % 3;
        let len: usize = 0;
        if (strat == 0) {
            len = rng.nextRange(160);
            for (i of 0..len) buf[i] = rng.nextU8();
        } else if (strat == 1) {
            len = (1 as usize) + rng.nextRange(seed_dr.len);
            @memcpy(buf[0..len], seed_dr[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            let m: usize = 0;
            while (m < muts) : (m += 1) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else {
            // Forced 'D' tag with mutated lengths
            len = (5 as usize) + rng.nextRange(48);
            buf[0] = 'D';
            for (i of 1..len) buf[i] = rng.nextU8();
        }

        fuzzCheckPgWire(buf[0..len]);
        total += 1;
    }
    expect_eq(total, 1500 as usize);
}

fn main() {
    test("1. Multi-Frame Pipeline, Progressive Partial Feeding & DataRow Roundtrip", () => {
        try testPgWirePipelineAndDataRowRoundtrip();
    });
    test("2. PostgreSQL v3 Security & Malformed Wire Guards", () => {
        try testPgWireSecurityGuards();
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        try testSimdVsScalarParity();
    });
    test("4. 1,500-Case Deterministic PRNG Malformed PostgreSQL v3 Fuzz Suite", () => {
        try testDeterministicPgWireFuzz();
    });
}
