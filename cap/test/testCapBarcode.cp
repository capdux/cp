// cap/test/testCapBarcode.cp
// Comprehensive unit tests for @cap/barcode

import { test, expect_eq } from "cap:test";
import {
    ean13Checksum,
    isValidEan13,
    upcAChecksum,
    isValidUpcA,
    encodeEan13,
    encodeUpcA,
    encodeCode128B,
    encodeCode39,
} from "../../packages/jsbarcode/src/index.cp";

fn containsSub(hay: string, needle: string) -> bool {
    if (needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= hay.len) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/barcode — EAN-13 checksum & 95-module encoding", () => {
        expect_eq(ean13Checksum("400638133393"), 1 as i8);
        expect_eq(isValidEan13("4006381333931"), true);
        expect_eq(isValidEan13("4006381333932"), false);

        const bc1 = encodeEan13("4006381333931");
        const bc2 = encodeEan13("400638133393"); // auto-computed checksum
        expect_eq(bc1.ok, true);
        expect_eq(bc2.ok, true);
        expect_eq(bc1.len, 95 as usize);
        expect_eq(bc2.len, 95 as usize);
        // Start guard 101 and end guard 101
        expect_eq(bc1.bitAt(0 as usize), 1 as u8);
        expect_eq(bc1.bitAt(1 as usize), 0 as u8);
        expect_eq(bc1.bitAt(2 as usize), 1 as u8);
        expect_eq(bc1.bitAt(94 as usize), 1 as u8);

        expect_eq(encodeEan13("4006381333932").ok, false);
    });

    test("2. @cap/barcode — UPC-A checksum & encoding", () => {
        expect_eq(upcAChecksum("03600029145"), 2 as i8);
        expect_eq(isValidUpcA("036000291452"), true);
        expect_eq(isValidUpcA("036000291459"), false);

        const bc = encodeUpcA("036000291452");
        expect_eq(bc.ok, true);
        expect_eq(bc.len, 95 as usize);
    });

    test("3. @cap/barcode — Code 128B full ASCII encoding & module count", () => {
        // Code 128B: Start(11) + N*11 + Checksum(11) + Stop(13) = 35 + 11*N
        const bc = encodeCode128B("CAP-2026");
        expect_eq(bc.ok, true);
        expect_eq(bc.len, (35 + 11 * 8) as usize);
        expect_eq(encodeCode128B("").ok, false);
    });

    test("4. @cap/barcode — Code 39 encoding & lowercase normalization", () => {
        const bc1 = encodeCode39("CAP-123");
        const bc2 = encodeCode39("cap-123");
        expect_eq(bc1.ok, true);
        expect_eq(bc2.ok, true);
        expect_eq(bc1.len, bc2.len);
        expect_eq(encodeCode39("BAD*CHAR").ok, false);
    });

    test("5. @cap/barcode — SVG & ASCII rendering", () => {
        const bc = encodeEan13("4006381333931");
        let ascii_buf: [128]u8 = undefined;
        const ascii = bc.renderAscii(ascii_buf[0..128]);
        expect_eq(ascii.len, 95 as usize);
        expect_eq(ascii[0], '#');
        expect_eq(ascii[1], ' ');
        expect_eq(ascii[2], '#');

        let svg_buf: [8192]u8 = undefined;
        const svg = bc.renderSvg(2 as usize, 80 as usize, 10 as usize, svg_buf[0..8192]);
        expect_eq(containsSub(svg, "<svg "), true);
        expect_eq(containsSub(svg, "width=\"230\""), true); // (95 + 20) * 2
        expect_eq(containsSub(svg, "</svg>"), true);
    });
}
