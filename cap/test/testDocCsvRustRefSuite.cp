// Origin: Pure-CP Stdlib Test Suite (lib/doc/csvTable.cp) | Created: 2026-11-23
// Reference-hardened against BurntSushi/rust-csv (MIT/Unlicense): csv-core/src/reader.rs & writer.rs
// 35+ ported boundary & roundtrip regression cases covering:
//   - Single/multi-row LF, CRLF, standalone CR, trailing commas, single space & single comma
//   - Empty input, empty lines (LF/CRLF/CR/mixed), interspersed & trailing empty lines
//   - Custom record terminators & ASCII unit/record separators (\x1F / \x1E)
//   - UTF-8 BOM at file start vs inside fields
//   - Quoting edge cases: empty quotes, inner/outer spaces, custom quote char, quote==delimiter
//   - Escaping matrix: double_quote=true/false, escape_char='\\'/custom, quoting_enabled=false
//   - Comment lines vs leading spaces, comment==terminator, mid-line '#', quoted '#'
//   - Strict RFC 4180 vs flexible_quotes, strict_column_count, trim_fields vs trim_whitespace
//   - Reader reset() & line() tracking
//   - CsvWriteOptions / writeCsvRowWith (Necessary/Always/Never/NonNumeric, escape_char, comment_char, LF/CRLF/CR/custom)
//   - Full write -> readRecordUnescaped roundtrip parity & standalone CR dialect sniffing

import { test, expect_eq, expect_true } from "cap:test";
import {
    CsvLineEnding,
    CsvQuoteStyle,
    CsvTableOptions,
    CsvWriteOptions,
    CsvRowCursor,
    CsvTableReader,
    unescapeCsvFieldWith,
    isCsvFieldNonNumeric,
    writeCsvRowWith,
    sniffCsvDialect,
} from "cap:doc";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn expectEof(cur: CsvRowCursor?) {
    if (c := cur) {
        * = c;
        expect_true(false);
    }
}

fn expectNoneUsize(v: usize?) {
    if (x := v) {
        * = x;
        expect_true(false);
    }
}

fn expectSingleRow(data: string, opts: CsvTableOptions, expected: []const string) {
    let r = CsvTableReader.init(data, opts);
    let buf: [256]u8 = [0; 256];
    let fields: [16]string = [""; 16];
    const cur = r.readRecordUnescaped(buf[0..], fields[0..]) orelse unreachable;
    expect_true(!cur.malformed);
    expect_true(cur.fields_count == expected.len);
    expect_true(cur.fields_stored == expected.len);
    let i: usize = 0;
    while (i < expected.len) : (i += 1) {
        expect_true(strEq(fields[i], expected[i]));
    }
    expectEof(r.readRecordUnescaped(buf[0..], fields[0..]));
}

fn expectTwoRows(data: string, opts: CsvTableOptions, exp0: []const string, exp1: []const string) {
    let r = CsvTableReader.init(data, opts);
    let buf: [256]u8 = [0; 256];
    let fields: [16]string = [""; 16];
    const c0 = r.readRecordUnescaped(buf[0..], fields[0..]) orelse unreachable;
    expect_true(!c0.malformed);
    expect_true(c0.fields_count == exp0.len);
    let i: usize = 0;
    while (i < exp0.len) : (i += 1) {
        expect_true(strEq(fields[i], exp0[i]));
    }
    const c1 = r.readRecordUnescaped(buf[0..], fields[0..]) orelse unreachable;
    expect_true(!c1.malformed);
    expect_true(c1.fields_count == exp1.len);
    i = 0;
    while (i < exp1.len) : (i += 1) {
        expect_true(strEq(fields[i], exp1[i]));
    }
    expectEof(r.readRecordUnescaped(buf[0..], fields[0..]));
}

fn expectZeroRows(data: string, opts: CsvTableOptions) {
    let r = CsvTableReader.init(data, opts);
    let buf: [64]u8 = [0; 64];
    let fields: [8]string = [""; 8];
    expectEof(r.readRecordUnescaped(buf[0..], fields[0..]));
}

// ============================================================================
// 1. rust-csv Reader Matrix: Single/Multi-Row, LF / CRLF / Standalone CR & Trailing Comma
// ============================================================================

test("rust-csv reader: one_row variants (plain, LF, CRLF, standalone CR, trailing comma)") {
    const opts = CsvTableOptions.relaxed();
    const e1: [1]string = ["a"];
    const e3: [3]string = ["a", "b", "c"];
    const et: [3]string = ["a", "b", ""];

    // one_row_one_field / many_fields / trailing_comma (no terminator)
    expectSingleRow("a", opts, e1[0..]);
    expectSingleRow("a,b,c", opts, e3[0..]);
    expectSingleRow("a,b,", opts, et[0..]);

    // LF
    expectSingleRow("a\n", opts, e1[0..]);
    expectSingleRow("a,b,c\n", opts, e3[0..]);
    expectSingleRow("a,b,\n", opts, et[0..]);

    // CRLF
    expectSingleRow("a\r\n", opts, e1[0..]);
    expectSingleRow("a,b,c\r\n", opts, e3[0..]);
    expectSingleRow("a,b,\r\n", opts, et[0..]);

    // Standalone CR
    expectSingleRow("a\r", opts, e1[0..]);
    expectSingleRow("a,b,c\r", opts, e3[0..]);
    expectSingleRow("a,b,\r", opts, et[0..]);

    // Single space & single comma
    const esp: [1]string = [" "];
    const ecm: [2]string = ["", ""];
    expectSingleRow(" ", opts, esp[0..]);
    expectSingleRow(",", opts, ecm[0..]);
}

test("rust-csv reader: many_rows variants (LF, CRLF, standalone CR, trailing comma)") {
    const opts = CsvTableOptions.relaxed();
    const a1: [1]string = ["a"];
    const b1: [1]string = ["b"];
    const abc: [3]string = ["a", "b", "c"];
    const xyz: [3]string = ["x", "y", "z"];
    const abt: [3]string = ["a", "b", ""];
    const xyt: [3]string = ["x", "y", ""];

    expectTwoRows("a\nb", opts, a1[0..], b1[0..]);
    expectTwoRows("a,b,c\nx,y,z", opts, abc[0..], xyz[0..]);
    expectTwoRows("a,b,\nx,y,", opts, abt[0..], xyt[0..]);

    expectTwoRows("a\nb\n", opts, a1[0..], b1[0..]);
    expectTwoRows("a,b,c\nx,y,z\n", opts, abc[0..], xyz[0..]);
    expectTwoRows("a,b,\nx,y,\n", opts, abt[0..], xyt[0..]);

    expectTwoRows("a\r\nb\r\n", opts, a1[0..], b1[0..]);
    expectTwoRows("a,b,c\r\nx,y,z\r\n", opts, abc[0..], xyz[0..]);
    expectTwoRows("a,b,\r\nx,y,\r\n", opts, abt[0..], xyt[0..]);

    expectTwoRows("a\rb\r", opts, a1[0..], b1[0..]);
    expectTwoRows("a,b,c\rx,y,z\r", opts, abc[0..], xyz[0..]);
    expectTwoRows("a,b,\rx,y,\r", opts, abt[0..], xyt[0..]);

    // extra_record_crlf_1 & 2
    const foo: [1]string = ["foo"];
    const one: [1]string = ["1"];
    expectTwoRows("foo\n1\n", opts, foo[0..], one[0..]);
    expectTwoRows("foo\r\n1\r\n", opts, foo[0..], one[0..]);
}

// ============================================================================
// 2. Empty Input & Empty Lines Matrix (LF / CRLF / CR / Mixed)
// ============================================================================

test("rust-csv reader: empty input, empty lines & interspersed/trailing empty lines") {
    const opts = CsvTableOptions.relaxed();
    expectZeroRows("", opts);
    expectZeroRows("\n\n\n\n", opts);
    expectZeroRows("\r\n\r\n\r\n\r\n", opts);
    expectZeroRows("\r\r\r\r", opts);
    expectZeroRows("\r\n\n\r\n\n", opts);

    const abc: [3]string = ["a", "b", "c"];
    const xyz: [3]string = ["x", "y", "z"];
    expectTwoRows("\n\n\na,b,c\nx,y,z\n\n\n", opts, abc[0..], xyz[0..]);
    expectTwoRows("\r\r\ra,b,c\rx,y,z\r\r\r", opts, abc[0..], xyz[0..]);
    expectTwoRows("\r\n\r\n\r\na,b,c\r\nx,y,z\r\n\r\n\r\n", opts, abc[0..], xyz[0..]);

    // empty_lines_interspersed (LF, CRLF, CR, mixed)
    const inputs: [4]string = [
        "\n\na,b\n\n\nx,y\n\n\nm,n\n",
        "\r\n\r\na,b\r\n\r\n\r\nx,y\r\n\r\n\r\nm,n\r\n",
        "\r\ra,b\r\r\rx,y\r\r\rm,n\r",
        "\n\r\na,b\r\n\n\r\nx,y\r\n\n\r\nm,n\r\n",
    ];
    let k: usize = 0;
    while (k < 4) : (k += 1) {
        let r = CsvTableReader.init(inputs[k], opts);
        let buf: [64]u8 = [0; 64];
        let f: [4]string = [""; 4];
        const c0 = r.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
        expect_eq(c0.fields_count, 2);
        expect_true(strEq(f[0], "a") && strEq(f[1], "b"));
        const c1 = r.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
        expect_eq(c1.fields_count, 2);
        expect_true(strEq(f[0], "x") && strEq(f[1], "y"));
        const c2 = r.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
        expect_eq(c2.fields_count, 2);
        expect_true(strEq(f[0], "m") && strEq(f[1], "n"));
        expectEof(r.readRecordUnescaped(buf[0..], f[0..]));
    }

    // Opt-in skip_empty_lines = false preserves blank lines as single empty field
    let keep_empty = CsvTableOptions.relaxed();
    keep_empty.skip_empty_lines = false;
    const emp: [1]string = [""];
    const a1: [1]string = ["a"];
    expectTwoRows("\na\n", keep_empty, emp[0..], a1[0..]);
}

// ============================================================================
// 3. Custom Record Terminator, ASCII Delimited & UTF-8 BOM Boundaries
// ============================================================================

test("rust-csv reader: custom terminator (term_weird), ASCII US/RS & UTF-8 BOM") {
    const ab: [2]string = ["a", "b"];
    const cd: [2]string = ["c", "d"];

    // term_weird: "zza,bzc,dzz" with terminator 'z'
    let opt_z = CsvTableOptions.relaxed();
    opt_z.record_terminator = 'z';
    expectTwoRows("zza,bzc,dzz", opt_z, ab[0..], cd[0..]);

    // ascii_delimited: "a\x1fb\x1ec\x1fd"
    expectTwoRows("a\x1Fb\x1Ec\x1Fd", CsvTableOptions.ascii(), ab[0..], cd[0..]);

    // Custom terminator does NOT split on \n inside unquoted field
    const nl_ab: [2]string = ["a\n1", "b"];
    expectTwoRows("a\n1\x1Fb\x1Ec\x1Fd", CsvTableOptions.ascii(), nl_ab[0..], cd[0..]);

    // BOM at start stripped; BOM inside field or at second field start preserved
    const opts = CsvTableOptions.relaxed();
    const e_a: [1]string = ["a"];
    const e_abom: [1]string = ["a\xEF\xBB\xBF"];
    const e_a_bomb: [2]string = ["a", "\xEF\xBB\xBFb"];
    expectSingleRow("\xEF\xBB\xBFa", opts, e_a[0..]);
    expectSingleRow("a\xEF\xBB\xBF", opts, e_abom[0..]);
    expectSingleRow("a,\xEF\xBB\xBFb", opts, e_a_bomb[0..]);
}

// ============================================================================
// 4. Quoting, Escaping, Double-Quote Toggle & Quoting Disabled
// ============================================================================

test("rust-csv reader: quotes, escapes, double_quote=false, quoting=false & quote==delimiter") {
    const opts = CsvTableOptions.relaxed();

    // quote_empty, quote_lf, quote_space, quote_inner_space, quote_outer_space
    const e_empty: [1]string = [""];
    const e_sp: [1]string = [" "];
    const e_isp: [1]string = [" a "];
    const e_osp: [1]string = ["  \"a\"  "];
    expectSingleRow("\"\"", opts, e_empty[0..]);
    expectSingleRow("\"\"\n", opts, e_empty[0..]);
    expectSingleRow("\" \"", opts, e_sp[0..]);
    expectSingleRow("\" a \"", opts, e_isp[0..]);
    expectSingleRow("  \"a\"  ", opts, e_osp[0..]);

    // quote_change: "zaz" with quote='z' -> ["a"]
    let opt_qz = CsvTableOptions.relaxed();
    opt_qz.quote_char = 'z';
    const e_a: [1]string = ["a"];
    expectSingleRow("zaz", opt_qz, e_a[0..]);

    // quote_delimiter: ",a,,b" with quote=',' -> ["a,b"]
    let opt_qd = CsvTableOptions.relaxed();
    opt_qd.quote_char = ',';
    const e_acb: [1]string = ["a,b"];
    expectSingleRow(",a,,b", opt_qd, e_acb[0..]);

    // quote_no_escapes: "\"a\\\"b\"" -> ["a\\b\""]
    const e_noesc: [1]string = ["a\\b\""];
    expectSingleRow("\"a\\\"b\"", opts, e_noesc[0..]);

    // quote_escapes_no_double: "\"a\"\"b\"" with double_quote_enabled=false -> ["a\"b\""]
    let opt_nodq = CsvTableOptions.relaxed();
    opt_nodq.double_quote_enabled = false;
    const e_nodq: [1]string = ["a\"b\""];
    expectSingleRow("\"a\"\"b\"", opt_nodq, e_nodq[0..]);

    // quote_escapes: "\"a\\\"b\"" with escape='\\' -> ["a\"b"]
    let opt_esc = CsvTableOptions.relaxed();
    opt_esc.escape_char = '\\';
    const e_esc: [1]string = ["a\"b"];
    expectSingleRow("\"a\\\"b\"", opt_esc, e_esc[0..]);

    // quote_escapes_change: "\"az\"b\"" with escape='z' -> ["a\"b"]
    let opt_escz = CsvTableOptions.relaxed();
    opt_escz.escape_char = 'z';
    expectSingleRow("\"az\"b\"", opt_escz, e_esc[0..]);

    // quote_escapes_with_comma: "\"\\\"A,B\\\"\"" with escape='\\', double_quote=false -> ["\"A,B\""]
    let opt_ec = CsvTableOptions.relaxed();
    opt_ec.escape_char = '\\';
    opt_ec.double_quote_enabled = false;
    const e_qab: [1]string = ["\"A,B\""];
    expectSingleRow("\"\\\"A,B\\\"\"", opt_ec, e_qab[0..]);

    // quoting_disabled: "\"abc,foo\"" -> ["\"abc", "foo\""]
    let opt_noq = CsvTableOptions.relaxed();
    opt_noq.quoting_enabled = false;
    const e_noq: [2]string = ["\"abc", "foo\""];
    expectSingleRow("\"abc,foo\"", opt_noq, e_noq[0..]);

    // delimiter_tabs & delimiter_weird ('z')
    let opt_tab = CsvTableOptions.relaxed();
    opt_tab.delimiter = '\t';
    let opt_dz = CsvTableOptions.relaxed();
    opt_dz.delimiter = 'z';
    const ab: [2]string = ["a", "b"];
    expectSingleRow("a\tb", opt_tab, ab[0..]);
    expectSingleRow("azb", opt_dz, ab[0..]);

    // unescapeCsvFieldWith custom escape_char & double_quote_enabled=false
    let ub: [64]u8 = [0; 64];
    const n1 = unescapeCsvFieldWith("\"\\\"A,B\\\"\"", '"', '\\', false, ub[0..]) orelse unreachable;
    expect_true(strEq(ub[0..n1] as string, "\"A,B\""));
}

// ============================================================================
// 5. Comment Lines & Quote Interaction Matrix
// ============================================================================

test("rust-csv reader: comment_1..5 & quoted comment char protection") {
    let opt_c = CsvTableOptions.relaxed();
    opt_c.comment_char = '#';

    const foo: [1]string = ["foo"];
    const bar: [1]string = ["bar"];
    // comment_1: "foo\n# hi\nbar\n" -> [["foo"], ["bar"]]
    expectTwoRows("foo\n# hi\nbar\n", opt_c, foo[0..], bar[0..]);

    // comment_2: leading space disables comment -> [["foo"], [" # hi"], ["bar"]]
    let r2 = CsvTableReader.init("foo\n # hi\nbar\n", opt_c);
    let buf: [64]u8 = [0; 64];
    let f: [4]string = [""; 4];
    * = r2.readRecordUnescaped(buf[0..], f[0..]);
    expect_true(strEq(f[0], "foo"));
    * = r2.readRecordUnescaped(buf[0..], f[0..]);
    expect_true(strEq(f[0], " # hi"));
    * = r2.readRecordUnescaped(buf[0..], f[0..]);
    expect_true(strEq(f[0], "bar"));
    expectEof(r2.readRecordUnescaped(buf[0..], f[0..]));

    // comment_3: comment_char == '\n' is ignored because terminator has priority
    let opt_cnl = CsvTableOptions.relaxed();
    opt_cnl.comment_char = '\n';
    let r3 = CsvTableReader.init("foo\n# hi\nbar\n", opt_cnl);
    * = r3.readRecordUnescaped(buf[0..], f[0..]);
    expect_true(strEq(f[0], "foo"));
    * = r3.readRecordUnescaped(buf[0..], f[0..]);
    expect_true(strEq(f[0], "# hi"));
    * = r3.readRecordUnescaped(buf[0..], f[0..]);
    expect_true(strEq(f[0], "bar"));

    // comment_4 & comment_5: mid-line '#' is ordinary data
    const c4: [3]string = ["foo", "b#ar", "baz"];
    const c5: [3]string = ["foo", "#bar", "baz"];
    expectSingleRow("foo,b#ar,baz", opt_c, c4[0..]);
    expectSingleRow("foo,#bar,baz", opt_c, c5[0..]);

    // Quoted field starting with '#' and containing "\n# inner" is NOT a comment
    const qhash: [2]string = ["# lead", "line1\n# inner"];
    expectSingleRow("\"# lead\",\"line1\n# inner\"\n# trailing comment\n", opt_c, qhash[0..]);
}

// ============================================================================
// 6. Strict RFC 4180 vs Relaxed Mode, Variable Columns, Trim Fields, Reset & Line Numbers
// ============================================================================

test("rust-csv reader: strict vs flexible quotes, trim_fields, reset() & line() tracking") {
    let buf: [128]u8 = [0; 128];
    let f: [8]string = [""; 8];

    // Strict RFC 4180 rejects unclosed quote & trailing garbage after closing quote
    let r_strict1 = CsvTableReader.init("\"unclosed", CsvTableOptions.csv());
    const s1 = r_strict1.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    expect_true(s1.malformed);

    let r_strict2 = CsvTableReader.init("\"abc\"def,g\n", CsvTableOptions.csv());
    const s2 = r_strict2.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    expect_true(s2.malformed);

    // Flexible mode parses both cleanly
    const e_unc: [1]string = ["unclosed"];
    const e_trl: [2]string = ["abcdef", "g"];
    expectSingleRow("\"unclosed", CsvTableOptions.relaxed(), e_unc[0..]);
    expectSingleRow("\"abc\"def,g\n", CsvTableOptions.relaxed(), e_trl[0..]);

    // Variable length columns allowed by default, flagged when strict_column_count=true
    let r_var = CsvTableReader.init("a,b\n1,2,3\n", CsvTableOptions.relaxed());
    const v0 = r_var.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    const v1 = r_var.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    expect_eq(v0.fields_count, 2);
    expect_eq(v1.fields_count, 3);
    expect_true(!v1.malformed);

    let opt_sc = CsvTableOptions.relaxed();
    opt_sc.strict_column_count = true;
    let r_sc = CsvTableReader.init("a,b\n1,2,3\n", opt_sc);
    const sc0 = r_sc.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    const sc1 = r_sc.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    expect_true(!sc0.malformed);
    expect_true(sc1.malformed);

    // trim_fields trims both unquoted and quoted inner whitespace; trim_whitespace trims only unquoted
    let opt_tf = CsvTableOptions.relaxed();
    opt_tf.trim_fields = true;
    const e_tf: [2]string = ["a", "b"];
    expectSingleRow("  a  ,\"  b  \"", opt_tf, e_tf[0..]);

    let opt_tw = CsvTableOptions.relaxed();
    opt_tw.trim_whitespace = true;
    const e_tw: [2]string = ["a", "  b  "];
    expectSingleRow("  a  ,\"  b  \"", opt_tw, e_tw[0..]);

    // reset_works: "\"\"bar\"" -> ["bar\""]
    const e_barq: [1]string = ["bar\""];
    expectSingleRow("\"\"bar\"", CsvTableOptions.relaxed(), e_barq[0..]);

    // line_numbers: starts at 1, 4 leading '\n' -> line 5, after "foo,bar\n" -> line 6, reset -> line 1
    let r_ln = CsvTableReader.init("\n\n\n\nfoo,bar\n", CsvTableOptions.relaxed());
    expect_eq(r_ln.line(), 1);
    const c_ln = r_ln.readRecordUnescaped(buf[0..], f[0..]) orelse unreachable;
    expect_eq(c_ln.fields_count, 2);
    expect_eq(r_ln.line(), 6);
    r_ln.reset();
    expect_eq(r_ln.line(), 1);
    r_ln.setLine(42);
    expect_eq(r_ln.line(), 42);
}

// ============================================================================
// 7. rust-csv Writer Matrix (`CsvWriteOptions` & `writeCsvRowWith`)
// ============================================================================

test("rust-csv writer: QuoteStyle (Necessary/Always/Never/NonNumeric), empty field, escape & comment") {
    let out: [256]u8 = [0; 256];

    // writer_one_field
    const f_abc: [1]string = ["abc"];
    const n1 = writeCsvRowWith(f_abc[0..], CsvWriteOptions.csv(), out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n1] as string, "abc"));

    // writer_one_empty_field_finish & terminator
    const f_emp: [1]string = [""];
    const n2 = writeCsvRowWith(f_emp[0..], CsvWriteOptions.csv(), out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n2] as string, "\"\""));

    let opt_lf = CsvWriteOptions{ include_terminator: true, line_terminator: CsvLineEnding.lf };
    const n3 = writeCsvRowWith(f_emp[0..], opt_lf, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n3] as string, "\"\"\n"));

    const n4 = writeCsvRowWith(f_emp[0..], CsvWriteOptions.crlf(), out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n4] as string, "\"\"\r\n"));

    // Two empty fields -> single comma (no quotes needed)
    const f_emp2: [2]string = ["", ""];
    const n5 = writeCsvRowWith(f_emp2[0..], CsvWriteOptions.csv(), out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n5] as string, ","));

    // writer_one_field_quote: a"bc -> "a""bc"
    const f_q: [1]string = ["a\"bc"];
    const n6 = writeCsvRowWith(f_q[0..], CsvWriteOptions.csv(), out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n6] as string, "\"a\"\"bc\""));

    // writer_two_fields_quote: ["a,bc", "\nz"] -> "\"a,bc\",\"\nz\""
    const f_2q: [2]string = ["a,bc", "\nz"];
    const n7 = writeCsvRowWith(f_2q[0..], CsvWriteOptions.csv(), out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n7] as string, "\"a,bc\",\"\nz\""));

    // writer_two_fields_non_numeric: ["abc", "5.2", "98"] -> "\"abc\",5.2,98"
    const f_nn: [3]string = ["abc", "5.2", "98"];
    const opt_nn = CsvWriteOptions{ quote_non_numeric: true };
    const n8 = writeCsvRowWith(f_nn[0..], opt_nn, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n8] as string, "\"abc\",5.2,98"));
    expect_true(!isCsvFieldNonNumeric("5.2"));
    expect_true(!isCsvFieldNonNumeric("-42"));
    expect_true(!isCsvFieldNonNumeric("1.5e-3"));
    expect_true(isCsvFieldNonNumeric(" 5.2 "));
    expect_true(isCsvFieldNonNumeric(""));
    expect_true(isCsvFieldNonNumeric("abc"));

    // always_quote & never_quote
    const opt_all = CsvWriteOptions{ always_quote: true };
    const n9 = writeCsvRowWith(f_nn[0..], opt_all, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n9] as string, "\"abc\",\"5.2\",\"98\""));

    const opt_nev = CsvWriteOptions{ never_quote: true };
    const n10 = writeCsvRowWith(f_2q[0..], opt_nev, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n10] as string, "a,bc,\nz"));
    const n10b = writeCsvRowWith(f_emp[0..], opt_nev, out[0..]) orelse unreachable;
    expect_eq(n10b, 0);

    // double_quote=false + escape_char='\\'
    const opt_esc = CsvWriteOptions{ double_quote: false, escape_char: '\\' };
    const f_escd: [2]string = ["a\"b", "c\\d"];
    const n11 = writeCsvRowWith(f_escd[0..], opt_esc, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n11] as string, "\"a\\\"b\",\"c\\\\d\""));

    // comment_char_is_automatically_quoted
    const opt_cmt = CsvWriteOptions{ comment_char: '#' };
    const f_cmt: [1]string = ["# abc"];
    const n12 = writeCsvRowWith(f_cmt[0..], opt_cmt, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n12] as string, "\"# abc\""));

    // Standalone CR & custom terminator 0x1E
    const opt_cr = CsvWriteOptions{ line_terminator: CsvLineEnding.cr, include_terminator: true };
    const n13 = writeCsvRowWith(f_abc[0..], opt_cr, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n13] as string, "abc\r"));

    const opt_rs = CsvWriteOptions{ delimiter: 0x1F, custom_terminator: 0x1E, include_terminator: true, quote_style: CsvQuoteStyle.necessary };
    const f_ab: [2]string = ["a", "b"];
    const n14 = writeCsvRowWith(f_ab[0..], opt_rs, out[0..]) orelse unreachable;
    expect_true(strEq(out[0..n14] as string, "a\x1Fb\x1E"));

    // Small buffer returns null safely
    let tiny: [3]u8 = [0; 3];
    expectNoneUsize(writeCsvRowWith(f_2q[0..], CsvWriteOptions.csv(), tiny[0..]));
}

// ============================================================================
// 8. Full Write -> Read Roundtrip Parity & Standalone CR Dialect Sniffing
// ============================================================================

test("rust-csv roundtrip: embedded CRLF/CR/comma/quotes/backslash/comment & CR dialect sniff") {
    const row0: [5]string = [
        "#lead_hash",
        "has,comma",
        "quote\"and\"\"two",
        "embed\r\ncrlf\rand\nlf",
        "path\\to\\file",
    ];
    let encoded: [256]u8 = [0; 256];

    // Roundtrip 1: double_quote=true + comment_char='#' + CRLF terminator
    const wopt1 = CsvWriteOptions{
        double_quote: true,
        comment_char: '#',
        line_terminator: CsvLineEnding.crlf,
        include_terminator: true,
    };
    const wlen1 = writeCsvRowWith(row0[0..], wopt1, encoded[0..]) orelse unreachable;
    let ropt1 = CsvTableOptions.csv();
    ropt1.comment_char = '#';
    expectSingleRow(encoded[0..wlen1] as string, ropt1, row0[0..]);

    // Roundtrip 2: double_quote=false + escape_char='\\' + comment_char='#' + CR terminator
    const wopt2 = CsvWriteOptions{
        double_quote: false,
        escape_char: '\\',
        comment_char: '#',
        line_terminator: CsvLineEnding.cr,
        include_terminator: true,
    };
    const wlen2 = writeCsvRowWith(row0[0..], wopt2, encoded[0..]) orelse unreachable;
    let ropt2 = CsvTableOptions.csv();
    ropt2.double_quote_enabled = false;
    ropt2.escape_char = '\\';
    ropt2.comment_char = '#';
    expectSingleRow(encoded[0..wlen2] as string, ropt2, row0[0..]);

    // Standalone CR dialect sniffing
    const cr_doc = "id,name,val\r1,alpha,10\r2,beta,20\r";
    const d = sniffCsvDialect(cr_doc, 8);
    expect_eq(d.delimiter, ',');
    expect_eq(d.line_ending, CsvLineEnding.cr);
    expect_true(d.has_header);
    expect_eq(d.estimated_columns, 3);
}
