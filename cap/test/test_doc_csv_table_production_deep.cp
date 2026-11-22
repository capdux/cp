// Origin: Pure-CP Stdlib Test Suite (lib/doc/csv_table.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc RFC 4180 CSV/TSV Table Decoder & Writer:
//   1. Header column mapping (columnIndex) & strongly-typed i64/f64/bool row extraction
//   2. RFC 4180 quoted fields: embedded commas, embedded newlines & "" unescaping
//   3. Custom TSV (\t) & semicolon (;) delimiters, trim_whitespace & UTF-8 BOM stripping
//   4. writeCsvRow RFC 4180 roundtrip encoding & decoding parity
//   5. Scalar parser edge cases (signed integers, overflow, scientific notation, bool variants)
//   6. Unclosed quote malformed detection, column truncation & small-buffer guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CsvTableOptions,
    CsvRowCursor,
    CsvTableHeader,
    CsvTableReader,
    unescapeCsvField,
    parseCsvI64,
    parseCsvF64,
    parseCsvBool,
    writeCsvRow,
} from "cap:doc";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = if (a >= b) (a - b) else (b - a);
    return d <= eps;
}

// ============================================================================
// Test 1: Header Column Mapping & Typed Row Extraction
// ============================================================================

fn testHeaderMappingAndTypedExtraction()! {
    const csv: string =
        "id,name,score,active\n" ++
        "101,Alice,98.5,true\n" ++
        "-42,Bob,0.25,no\n";

    let reader = cap.doc.CsvTableReader.init(csv, CsvTableOptions.csv());
    let hdr_buf: [8]string = [""; 8];
    const hdr: CsvTableHeader = reader.parseHeader(hdr_buf[0..]) ?? return error.HeaderFailed;

    expect_eq(hdr.columnCount(), 4);
    const c_id = hdr.columnIndex("id") ?? return error.MissingCol;
    const c_name = hdr.columnIndex("name") ?? return error.MissingCol;
    const c_score = hdr.columnIndex("score") ?? return error.MissingCol;
    const c_active = hdr.columnIndex("active") ?? return error.MissingCol;
    expect_eq(c_id, 0);
    expect_eq(c_name, 1);
    expect_eq(c_score, 2);
    expect_eq(c_active, 3);
    expect_eq(hdr.columnIndex("nonexistent") == null, true);

    let cols: [8]string = [""; 8];
    const r1: CsvRowCursor = reader.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(r1.malformed, false);
    expect_eq(r1.fields_count, 4);
    expect_eq(parseCsvI64(cols[c_id]) ?? 0, 101);
    expect_eq(strEq(cols[c_name], "Alice"), true);
    expect_eq(approxEq(parseCsvF64(cols[c_score]) ?? 0.0, 98.5, 0.000001), true);
    expect_eq(parseCsvBool(cols[c_active]) ?? false, true);

    const r2 = reader.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(r2.malformed, false);
    expect_eq(parseCsvI64(cols[c_id]) ?? 0, -42);
    expect_eq(strEq(cols[c_name], "Bob"), true);
    expect_eq(approxEq(parseCsvF64(cols[c_score]) ?? 0.0, 0.25, 0.000001), true);
    expect_eq(parseCsvBool(cols[c_active]) ?? true, false);

    // EOF
    expect_eq(reader.nextRow(cols[0..]) == null, true);
}

// ============================================================================
// Test 2: RFC 4180 Quoted Embedded Commas, Newlines & "" Unescaping
// ============================================================================

fn testRfc4180QuotedFieldsAndUnescape()! {
    const csv: string =
        "\"Doe, \"\"Jane\"\"\",\"Line1\nLine2\",42\r\n" ++
        "simple,\"quoted\r\ncrlf\",99\r\n";

    let reader = CsvTableReader.init(csv, CsvTableOptions.csv());
    let cols: [4]string = [""; 4];
    const r1 = reader.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(r1.malformed, false);
    expect_eq(r1.fields_count, 3);
    expect_eq(strEq(cols[1], "Line1\nLine2"), true);
    expect_eq(parseCsvI64(cols[2]) ?? 0, 42);

    let unesc: [64]u8 = [0; 64];
    const n1 = unescapeCsvField(cols[0], unesc[0..]) ?? return error.UnescapeFailed;
    expect_eq(strEq(unesc[0..n1] as string, "Doe, \"Jane\""), true);

    const r2 = reader.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(r2.malformed, false);
    expect_eq(strEq(cols[0], "simple"), true);
    expect_eq(strEq(cols[1], "quoted\r\ncrlf"), true);
    expect_eq(parseCsvI64(cols[2]) ?? 0, 99);
}

// ============================================================================
// Test 3: TSV & Semicolon Delimiters, Whitespace Trimming & UTF-8 BOM
// ============================================================================

fn testTsvAndCustomDelimiters()! {
    const tsv: string = "\xEF\xBB\xBFkey\tval\tflag\nalpha\t123\tYES\n";
    let reader = CsvTableReader.init(tsv, CsvTableOptions.tsv());
    let hdr_buf: [4]string = [""; 4];
    const hdr = reader.parseHeader(hdr_buf[0..]) ?? return error.HeaderFailed;
    expect_eq(hdr.columnIndex("key") ?? 99, 0);

    let cols: [4]string = [""; 4];
    const r1 = reader.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(r1.malformed, false);
    expect_eq(strEq(cols[0], "alpha"), true);
    expect_eq(parseCsvI64(cols[1]) ?? 0, 123);
    expect_eq(parseCsvBool(cols[2]) ?? false, true);

    // Semicolon delimiter with trim_whitespace = true
    const semi: string = " a ; 456 ; false ";
    let r_semi = CsvTableReader.init(semi, CsvTableOptions{
        delimiter: ';',
        has_header: false,
        trim_whitespace: true,
    });
    const rs = r_semi.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(rs.fields_count, 3);
    expect_eq(strEq(cols[0], "a"), true);
    expect_eq(strEq(cols[1], "456"), true);
    expect_eq(strEq(cols[2], "false"), true);
}

// ============================================================================
// Test 4: RFC 4180 writeCsvRow Roundtrip
// ============================================================================

fn testWriteCsvRowRoundtrip()! {
    const fields: [4]string = [
        "plain",
        "has,comma",
        "say \"hi\"",
        "multi\nline",
    ];
    let buf: [128]u8 = [0; 128];
    const written = writeCsvRow(fields[0..], ',', buf[0..]) ?? return error.WriteFailed;
    const encoded = buf[0..written] as string;
    expect_eq(strEq(encoded, "plain,\"has,comma\",\"say \"\"hi\"\"\",\"multi\nline\""), true);

    // Parse back and verify exact field identity
    let reader = CsvTableReader.init(encoded, CsvTableOptions.csv());
    let out_cols: [4]string = [""; 4];
    const cur = reader.nextRow(out_cols[0..]) ?? return error.RowMissing;
    expect_eq(cur.malformed, false);
    expect_eq(cur.fields_count, 4);
    expect_eq(strEq(out_cols[0], "plain"), true);
    expect_eq(strEq(out_cols[1], "has,comma"), true);
    expect_eq(strEq(out_cols[3], "multi\nline"), true);

    let tmp: [32]u8 = [0; 32];
    const n2 = unescapeCsvField(out_cols[2], tmp[0..]) ?? return error.UnescapeFailed;
    expect_eq(strEq(tmp[0..n2] as string, "say \"hi\""), true);
}

// ============================================================================
// Test 5: Strongly-Typed Scalar Parser Edge Cases
// ============================================================================

fn testTypedScalarEdgeCases()! {
    expect_eq(parseCsvI64("  +77 ") ?? 0, 77);
    expect_eq(parseCsvI64("\"-9223372036854775808\"") ?? 0, -9223372036854775807 - 1);
    expect_eq(parseCsvI64("9223372036854775808") == null, true); // i64 overflow
    expect_eq(parseCsvI64("12abc") == null, true);
    expect_eq(parseCsvI64("") == null, true);

    expect_eq(approxEq(parseCsvF64(" -1.25e-2 ") ?? 0.0, -0.0125, 0.0000001), true);
    expect_eq(approxEq(parseCsvF64("3.5E2") ?? 0.0, 350.0, 0.0000001), true);
    expect_eq(parseCsvF64("abc") == null, true);
    expect_eq(parseCsvF64("1.2e") == null, true);

    expect_eq(parseCsvBool("1") ?? false, true);
    expect_eq(parseCsvBool("0") ?? true, false);
    expect_eq(parseCsvBool("YeS") ?? false, true);
    expect_eq(parseCsvBool("NO") ?? true, false);
    expect_eq(parseCsvBool("maybe") == null, true);
}

// ============================================================================
// Test 6: Malformed Unclosed Quote, Column Truncation & Small Buffer Guards
// ============================================================================

fn testMalformedAndBufferGuards()! {
    // 6a. Unclosed quote sets malformed = true
    let r_bad = CsvTableReader.init("a,\"unclosed quote", CsvTableOptions.csv());
    let cols: [4]string = [""; 4];
    const bad_cur = r_bad.nextRow(cols[0..]) ?? return error.RowMissing;
    expect_eq(bad_cur.malformed, true);

    // 6b. Column truncation when out_fields is smaller than row width
    let r_wide = CsvTableReader.init("c1,c2,c3,c4\n", CsvTableOptions.csv());
    let tiny_cols: [2]string = [""; 2];
    const wide_cur = r_wide.nextRow(tiny_cols[0..]) ?? return error.RowMissing;
    expect_eq(wide_cur.malformed, false);
    expect_eq(wide_cur.fields_count, 4);
    expect_eq(wide_cur.fields_stored, 2);
    expect_eq(wide_cur.truncated, true);
    expect_eq(strEq(tiny_cols[0], "c1"), true);
    expect_eq(strEq(tiny_cols[1], "c2"), true);

    // 6c. Empty input -> parseHeader & nextRow return null
    let r_empty = CsvTableReader.init("", CsvTableOptions.csv());
    expect_eq(r_empty.parseHeader(cols[0..]) == null, true);
    expect_eq(r_empty.nextRow(cols[0..]) == null, true);

    // 6d. Small buffer returns null on writeCsvRow & unescapeCsvField
    let tiny_buf: [4]u8 = [0; 4];
    const row: [2]string = ["hello", "world"];
    expect_eq(writeCsvRow(row[0..], ',', tiny_buf[0..]) == null, true);
    expect_eq(unescapeCsvField("abcdef", tiny_buf[0..]) == null, true);
}

test("doc.csv_table: header column mapping & typed i64/f64/bool extraction", testHeaderMappingAndTypedExtraction);
test("doc.csv_table: RFC 4180 quoted commas, newlines & double-quote unescaping", testRfc4180QuotedFieldsAndUnescape);
test("doc.csv_table: TSV, semicolon delimiter, trim_whitespace & UTF-8 BOM", testTsvAndCustomDelimiters);
test("doc.csv_table: RFC 4180 writeCsvRow roundtrip parity", testWriteCsvRowRoundtrip);
test("doc.csv_table: strongly-typed scalar parser edge cases & overflow defense", testTypedScalarEdgeCases);
test("doc.csv_table: unclosed quote malformed, column truncation & small buffer guards", testMalformedAndBufferGuards);
