import { test, expect_eq } from "cap:test";
import {
    H3_UNUSED_DIGIT,
    H3Cell,
    parseH3Hex,
    avgHexAreaKm2
} from "../../packages/h3o/src/index.cp";

fn h3TestBufEq(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn h3TestDummy(): H3Cell {
    return H3Cell { bits: 0 };
}

fn main() {
    test("1. @cap/h3o — parseH3Hex & toHex on real Uber H3 index (San Francisco res 5)", () => {
        let maybe = parseH3Hex("85283473fffffff");
        expect_eq(maybe == null, false);
        let cell: H3Cell = maybe ?? h3TestDummy();
        expect_eq(cell.resolution(), 5);
        expect_eq(cell.baseCell(), 20);
        expect_eq(cell.digitAt(1), 0);
        expect_eq(cell.digitAt(2), 6);
        expect_eq(cell.digitAt(3), 4);
        expect_eq(cell.digitAt(4), 3);
        expect_eq(cell.digitAt(5), 4);
        expect_eq(cell.digitAt(6), H3_UNUSED_DIGIT);
        expect_eq(cell.isPentagon(), false);

        let hex_buf: [32]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        ];
        let n: usize = cell.toHex(hex_buf[0..32]);
        expect_eq(h3TestBufEq(hex_buf[0..32], n, "85283473fffffff"), true);

        let prefixed = parseH3Hex("0x85283473FFFFFFF");
        expect_eq(prefixed == null, false);
        expect_eq((prefixed ?? h3TestDummy()).bits, cell.bits);
    });

    test("2. @cap/h3o — H3Cell.fromParts constructs exact bit layout", () => {
        let digits: [15]u8 = [0, 6, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let maybe = H3Cell.fromParts(5, 20, digits);
        expect_eq(maybe == null, false);
        let cell: H3Cell = maybe ?? h3TestDummy();

        let ref_cell: H3Cell = parseH3Hex("85283473fffffff") ?? h3TestDummy();
        expect_eq(cell.bits, ref_cell.bits);
    });

    test("3. @cap/h3o — parent & childAt hierarchy traversal round-trip", () => {
        let cell: H3Cell = parseH3Hex("85283473fffffff") ?? h3TestDummy();
        let p3 = cell.parent(3);
        expect_eq(p3 == null, false);
        let c3: H3Cell = p3 ?? h3TestDummy();
        expect_eq(c3.resolution(), 3);
        expect_eq(c3.baseCell(), 20);
        expect_eq(c3.digitAt(3), 4);
        expect_eq(c3.digitAt(4), H3_UNUSED_DIGIT);
        expect_eq(c3.digitAt(5), H3_UNUSED_DIGIT);

        // Drilling back down from res 3 -> res 4 (digit 3) -> res 5 (digit 4)
        let c4: H3Cell = c3.childAt(3) ?? h3TestDummy();
        let c5: H3Cell = c4.childAt(4) ?? h3TestDummy();
        expect_eq(c5.bits, cell.bits);

        // Finer parent rejected
        expect_eq(c3.parent(4) == null, true);
    });

    test("4. @cap/h3o — pentagon detection & K-axis (digit 1) deletion", () => {
        let zeros: [15]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let p0: H3Cell = H3Cell.fromParts(0, 4, zeros) ?? h3TestDummy();
        expect_eq(p0.isPentagon(), true);

        let p3: H3Cell = H3Cell.fromParts(3, 117, zeros) ?? h3TestDummy();
        expect_eq(p3.isPentagon(), true);

        // Pentagon child at digit 0 stays pentagon; digit 1 is invalid; digit 2 becomes hexagon
        let pc0: H3Cell = p0.childAt(0) ?? h3TestDummy();
        expect_eq(pc0.isPentagon(), true);
        expect_eq(p0.childAt(1) == null, true);
        let pc2: H3Cell = p0.childAt(2) ?? h3TestDummy();
        expect_eq(pc2.isPentagon(), false);
    });

    test("5. @cap/h3o — strict validation rejects invalid mode, reserved bits, base cell & digits", () => {
        let zeros: [15]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        expect_eq(H3Cell.fromParts(16, 0, zeros) == null, true);
        expect_eq(H3Cell.fromParts(0, 122, zeros) == null, true);
        let bad_digits: [15]u8 = [7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        expect_eq(H3Cell.fromParts(1, 10, bad_digits) == null, true);

        // Invalid raw u64: wrong mode (2), reserved bit set, non-7 trailing digit
        expect_eq(H3Cell.fromU64(0) == null, true);
        expect_eq(parseH3Hex("15283473fffffff") == null, true);
        expect_eq(parseH3Hex("85283473ffffff0") == null, true);
        expect_eq(parseH3Hex("not-hex") == null, true);
        expect_eq(parseH3Hex("") == null, true);
    });

    test("6. @cap/h3o — avgHexAreaKm2 monotonic table across resolutions 0..15", () => {
        expect_eq(avgHexAreaKm2(0) > 4000000.0, true);
        expect_eq(avgHexAreaKm2(15) > 0.0, true);
        expect_eq(avgHexAreaKm2(16), 0.0);

        let r: u8 = 0;
        while (r < 15) {
            let ratio: f64 = avgHexAreaKm2(r) / avgHexAreaKm2(r + 1);
            expect_eq(ratio > 6.5 && ratio < 7.5, true);
            r = r + 1;
        }
    });
}
