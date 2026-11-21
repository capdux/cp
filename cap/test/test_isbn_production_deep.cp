// Source: cp/cap/test/test_isbn_production_deep.cp (Production deep tests for cap:isbn)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ISBN10_LEN,
    ISBN13_LEN,
    ISSN_LEN,
    ISSN_FORMATTED_LEN,
    EAN13_LEN,
    EAN8_LEN,
    UPCA_LEN,
    isbn10CheckDigit,
    isValidIsbn10,
    isbn13CheckDigit,
    isValidIsbn13,
    isValidIsbn,
    isbn10To13,
    isbn13To10,
    issnCheckDigit,
    isValidIssn,
    formatIssn,
    eanCheckDigit,
    isValidEan13,
    isValidEan8,
    isValidUpcA,
} from "cap:isbn";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. Real-world ISBN-10 (including X/x check digit) & ISBN-13 (978/979) validation", () => {
    expect_eq(ISBN10_LEN, 10 as usize);
    expect_eq(ISBN13_LEN, 13 as usize);
    expect_eq(ISSN_LEN, 8 as usize);
    expect_eq(ISSN_FORMATTED_LEN, 9 as usize);
    expect_eq(EAN13_LEN, 13 as usize);
    expect_eq(EAN8_LEN, 8 as usize);
    expect_eq(UPCA_LEN, 12 as usize);

    // ISBN-10 check digits & validation
    expect_eq(isbn10CheckDigit("0-306-40615") ?? unreachable, 0x32 as u8); // '2'
    expect_eq(isbn10CheckDigit("0-8044-2957") ?? unreachable, 0x58 as u8); // 'X'
    expect_eq(isbn10CheckDigit("0 19 852663") ?? unreachable, 0x36 as u8); // '6'

    expect_eq(isValidIsbn10("0-306-40615-2"), true);
    expect_eq(isValidIsbn10("0306406152"), true);
    expect_eq(isValidIsbn10("0-8044-2957-X"), true);
    expect_eq(isValidIsbn10("0-8044-2957-x"), true);
    expect_eq(isValidIsbn10("0-19-852663-6"), true);

    // ISBN-13 check digits & validation (both 978 and 979 prefixes)
    expect_eq(isbn13CheckDigit("978-0-306-40615") ?? unreachable, 0x37 as u8); // '7'
    expect_eq(isbn13CheckDigit("978-0-8044-2957") ?? unreachable, 0x33 as u8); // '3'
    expect_eq(isbn13CheckDigit("979-10-90636-07") ?? unreachable, 0x31 as u8); // '1'

    expect_eq(isValidIsbn13("978-0-306-40615-7"), true);
    expect_eq(isValidIsbn13("9780804429573"), true);
    expect_eq(isValidIsbn13("979-10-90636-07-1"), true);

    // Unified isValidIsbn
    expect_eq(isValidIsbn("0-8044-2957-X"), true);
    expect_eq(isValidIsbn("979-10-90636-07-1"), true);
    expect_eq(isValidIsbn("4006381333931"), false); // valid EAN-13, NOT ISBN-13
});

test("2. Lossless ISBN-10 <-> ISBN-13 roundtrip, 979-to-10 rejection & buffer bounds", () => {
    let buf13: [13]u8 = std.mem.zeroes([13]u8);
    let buf10: [10]u8 = std.mem.zeroes([10]u8);

    // 0-306-40615-2 <-> 9780306406157
    const n13_a = isbn10To13("0-306-40615-2", buf13[0..]) ?? unreachable;
    expect_eq(n13_a, 13 as usize);
    expect_eq(bytesEq(buf13[0..n13_a], "9780306406157"), true);
    const n10_a = isbn13To10(buf13[0..n13_a], buf10[0..]) ?? unreachable;
    expect_eq(n10_a, 10 as usize);
    expect_eq(bytesEq(buf10[0..n10_a], "0306406152"), true);

    // 0-8044-2957-x <-> 9780804429573 (check digit 'X' roundtrip)
    const n13_b = isbn10To13("0-8044-2957-x", buf13[0..]) ?? unreachable;
    expect_eq(n13_b, 13 as usize);
    expect_eq(bytesEq(buf13[0..n13_b], "9780804429573"), true);
    const n10_b = isbn13To10("978-0-8044-2957-3", buf10[0..]) ?? unreachable;
    expect_eq(n10_b, 10 as usize);
    expect_eq(bytesEq(buf10[0..n10_b], "080442957X"), true);

    // 979-prefixed ISBN-13 has no ISBN-10 equivalent -> MUST return null
    expect_eq(isbn13To10("979-10-90636-07-1", buf10[0..]) == null, true);

    // Output buffer too small -> MUST return null
    expect_eq(isbn10To13("0-306-40615-2", buf13[0..12]) == null, true);
    expect_eq(isbn13To10("978-0-306-40615-7", buf10[0..9]) == null, true);

    // Invalid source -> MUST return null
    expect_eq(isbn10To13("0-306-40615-9", buf13[0..]) == null, true);
    expect_eq(isbn13To10("978-0-306-40615-0", buf10[0..]) == null, true);
});

test("3. ISO 3297 ISSN check digit, validation (with X/x) & canonical XXXX-XXXX formatting", () => {
    expect_eq(issnCheckDigit("0378-595") ?? unreachable, 0x35 as u8); // '5'
    expect_eq(issnCheckDigit("2434-561") ?? unreachable, 0x58 as u8); // 'X'
    expect_eq(issnCheckDigit("0028083") ?? unreachable, 0x36 as u8);  // '6' (Nature)

    expect_eq(isValidIssn("0378-5955"), true);
    expect_eq(isValidIssn("03785955"), true);
    expect_eq(isValidIssn("2434-561X"), true);
    expect_eq(isValidIssn("2434 561x"), true);
    expect_eq(isValidIssn("0028-0836"), true);

    let out: [9]u8 = std.mem.zeroes([9]u8);
    const n1 = formatIssn("03785955", out[0..]) ?? unreachable;
    expect_eq(n1, 9 as usize);
    expect_eq(bytesEq(out[0..n1], "0378-5955"), true);

    const n2 = formatIssn("2434 561x", out[0..]) ?? unreachable;
    expect_eq(n2, 9 as usize);
    expect_eq(bytesEq(out[0..n2], "2434-561X"), true);

    // Buffer too small & invalid ISSN rejection
    expect_eq(formatIssn("03785955", out[0..8]) == null, true);
    expect_eq(formatIssn("0378-5954", out[0..]) == null, true);
});

test("4. GS1 retail barcodes (EAN-13, EAN-8, UPC-A) & generic eanCheckDigit verification", () => {
    // EAN-13
    expect_eq(eanCheckDigit("400638133393") ?? unreachable, 0x31 as u8); // '1'
    expect_eq(eanCheckDigit("590123412345") ?? unreachable, 0x37 as u8); // '7'
    expect_eq(isValidEan13("4006381333931"), true);
    expect_eq(isValidEan13("590-1234-12345-7"), true);
    expect_eq(isValidEan13("4006381333932"), false);

    // EAN-8
    expect_eq(eanCheckDigit("9638507") ?? unreachable, 0x34 as u8); // '4'
    expect_eq(eanCheckDigit("5512345") ?? unreachable, 0x37 as u8); // '7'
    expect_eq(isValidEan8("96385074"), true);
    expect_eq(isValidEan8("5512-3457"), true);
    expect_eq(isValidEan8("96385075"), false);

    // UPC-A
    expect_eq(eanCheckDigit("03600029145") ?? unreachable, 0x32 as u8); // '2'
    expect_eq(eanCheckDigit("04210000526") ?? unreachable, 0x34 as u8); // '4'
    expect_eq(isValidUpcA("036000291452"), true);
    expect_eq(isValidUpcA("0-42100-00526-4"), true);
    expect_eq(isValidUpcA("036000291459"), false);
});

test("5. Single-digit tamper & adjacent transposition error detection for ISBN-10, ISBN-13 & ISSN", () => {
    // 1. Every single-digit tamper in ISBN-10 ("0306406152") MUST fail
    const base10 = "0306406152";
    let mut10: [10]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let pos: usize = 0;
    while (pos < 10) {
        let j: usize = 0;
        while (j < 10) {
            mut10[j] = base10[j];
            j += 1;
        }
        mut10[pos] = if (mut10[pos] == 0x39) 0x30 else mut10[pos] + 1;
        expect_eq(isValidIsbn10(mut10[0..]), false);
        pos += 1;
    }

    // 2. Every adjacent distinct-digit transposition in ISBN-10 MUST fail (Mod-11 property!)
    pos = 0;
    while (pos < 9) {
        if (base10[pos] != base10[pos + 1]) {
            let j: usize = 0;
            while (j < 10) {
                mut10[j] = base10[j];
                j += 1;
            }
            const tmp = mut10[pos];
            mut10[pos] = mut10[pos + 1];
            mut10[pos + 1] = tmp;
            expect_eq(isValidIsbn10(mut10[0..]), false);
        }
        pos += 1;
    }

    // 3. Every single-digit tamper in ISBN-13 ("9780306406157") MUST fail
    const base13 = "9780306406157";
    let mut13: [13]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    pos = 0;
    while (pos < 13) {
        let j: usize = 0;
        while (j < 13) {
            mut13[j] = base13[j];
            j += 1;
        }
        mut13[pos] = if (mut13[pos] == 0x39) 0x30 else mut13[pos] + 1;
        expect_eq(isValidIsbn13(mut13[0..]), false);
        pos += 1;
    }

    // 4. Every single-digit tamper & adjacent transposition in ISSN ("03785955") MUST fail
    const base_issn = "03785955";
    let mut_issn: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    pos = 0;
    while (pos < 8) {
        let j: usize = 0;
        while (j < 8) {
            mut_issn[j] = base_issn[j];
            j += 1;
        }
        mut_issn[pos] = if (mut_issn[pos] == 0x39) 0x30 else mut_issn[pos] + 1;
        expect_eq(isValidIssn(mut_issn[0..]), false);
        pos += 1;
    }
    pos = 0;
    while (pos < 7) {
        if (base_issn[pos] != base_issn[pos + 1]) {
            let j: usize = 0;
            while (j < 8) {
                mut_issn[j] = base_issn[j];
                j += 1;
            }
            const tmp = mut_issn[pos];
            mut_issn[pos] = mut_issn[pos + 1];
            mut_issn[pos + 1] = tmp;
            expect_eq(isValidIssn(mut_issn[0..]), false);
        }
        pos += 1;
    }
});

test("6. Malformed inputs, illegal characters, wrong prefixes & wrong lengths rejection", () => {
    // Empty & separator-only inputs
    expect_eq(isbn10CheckDigit("") == null, true);
    expect_eq(isbn10CheckDigit("---") == null, true);
    expect_eq(isbn13CheckDigit("") == null, true);
    expect_eq(issnCheckDigit("") == null, true);
    expect_eq(eanCheckDigit("") == null, true);
    expect_eq(eanCheckDigit(" - ") == null, true);
    expect_eq(isValidIsbn(""), false);
    expect_eq(isValidIssn(""), false);
    expect_eq(isValidEan13(""), false);
    expect_eq(isValidEan8(""), false);
    expect_eq(isValidUpcA(""), false);

    // 'X' in non-check position or inside numeric-only codes
    expect_eq(isValidIsbn10("X-306-40615-2"), false);
    expect_eq(isValidIssn("X378-5955"), false);
    expect_eq(isValidIsbn13("978-0-306-40615-X"), false);
    expect_eq(eanCheckDigit("40063813339X") == null, true);

    // Wrong lengths & illegal characters
    expect_eq(isbn10CheckDigit("03064061") == null, true);
    expect_eq(isbn10CheckDigit("0306406152") == null, true);
    expect_eq(isbn13CheckDigit("97803064061") == null, true);
    expect_eq(isbn13CheckDigit("400638133393") == null, true); // non-978/979 prefix
    expect_eq(issnCheckDigit("037859") == null, true);
    expect_eq(issnCheckDigit("03785955") == null, true);
    expect_eq(isValidIsbn10("0-306-40615-20"), false);
    expect_eq(isValidIsbn13("978-0-306-40615-70"), false);
    expect_eq(isValidEan13("400638133393"), false);
    expect_eq(isValidEan8("9638507"), false);
    expect_eq(isValidUpcA("03600029145"), false);
    expect_eq(isValidUpcA("03600029145A"), false);
});
