// Origin: Pure-CP Stdlib Test Suite (lib/doc/csvTable.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap RFC 4180 CSV/TSV Dialect Sniffer, Schema Inference, Filter/Sort/Agg & JSONL:
//   1. Dialect sniffer (`,` / `\t` / `;` / `|`, CRLF/LF, UTF-8 BOM, header heuristic) + multiline quoted & `#` comment lines
//   2. Monotonic column schema inference (`empty/bool/int/float/iso_date/string`) & Welford online mean/variance accuracy
//   3. Multi-predicate AND filter + column projection (`projectAndFilterCsv`)
//   4. Zero-heap group-by aggregation (`aggregateCsvByGroup`: count/sum/min/max/avg)
//   5. Stable iterative O(N log N) merge sort (`sortCsvRowsByColumn`: numeric desc & string asc with tie stability)
//   6. Bidirectional RFC 4180 CSV <-> RFC 8259 JSONL (NDJSON) roundtrip (typed scalars, null, escaped quotes/newlines)
//   7. Malformed unclosed quote / strict column count / invalid JSONL guards & cap.doc namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CsvTableError,
    CsvTableOptions,
    CsvTableReader,
    CsvLineEnding,
    sniffCsvDialect,
    CsvColumnKind,
    CsvColumnProfile,
    inferCsvSchema,
    CsvFilterOp,
    CsvFilterPred,
    projectAndFilterCsv,
    CsvAggKind,
    CsvGroupAggSlot,
    aggregateCsvByGroup,
    CsvRowSpan,
    sortCsvRowsByColumn,
    csvToJsonl,
    jsonlToCsv,
} from "cap:doc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

test("CSV dialect sniffer (comma, tab, semicolon, CRLF, UTF-8 BOM) + multiline quoted & comments") {
    // 1. Semicolon-delimited CRLF with UTF-8 BOM and header
    const semi_csv = "\xEF\xBB\xBFid;name;price\r\n1;Widget;19.99\r\n2;Gadget;29.50\r\n";
    const d1 = sniffCsvDialect(semi_csv, 10);
    expect_eq(d1.delimiter, ';');
    expect_eq(d1.line_ending, CsvLineEnding.crlf);
    expect_true(d1.has_bom);
    expect_true(d1.has_header);
    expect_eq(d1.estimated_columns, 3);

    // 2. Tab-delimited TSV with LF
    const tsv = "k\tv\tflag\nalpha\t10\ttrue\nbeta\t20\tfalse\n";
    const d2 = sniffCsvDialect(tsv, 10);
    expect_eq(d2.delimiter, '\t');
    expect_eq(d2.line_ending, CsvLineEnding.lf);
    expect_true(d2.has_header);

    // 3. Comment lines (`#`) + multiline quoted field + escaped ""
    const raw = "# comment line 1\nid,note\n1,\"line1\n\"\"quoted\"\"\"\n# mid comment\n2,plain\n";
    let r = CsvTableReader.init(raw, CsvTableOptions{
        delimiter: ',',
        has_header: true,
        comment_char: '#',
        strict_column_count: true,
    });
    let cols: [4]string = [""; 4];
    const h = r.parseHeader(cols[0..]) orelse unreachable;
    expect_eq(h.columnCount(), 2);
    const c1 = r.nextRow(cols[0..]) orelse unreachable;
    expect_true(!c1.malformed);
    expect_true(bytesEq(cols[0], "1"));
    expect_true(bytesEq(cols[1], "line1\n\"\"quoted\"\""));
    const c2 = r.nextRow(cols[0..]) orelse unreachable;
    expect_true(!c2.malformed);
    expect_true(bytesEq(cols[0], "2"));
    expect_true(bytesEq(cols[1], "plain"));
}

test("Monotonic column schema inference & Welford online mean/variance statistics") {
    const csv = "id,score,active,joined,label,empty_col\n"
        ++ "1,10,true,2026-01-01,alpha,\n"
        ++ "2,20.5,false,2026-02-15,beta,\n"
        ++ "3,30,yes,2026-03-30,gamma,\n"
        ++ "4,,no,2026-04-10,delta,\n";

    let profiles: [8]CsvColumnProfile = [CsvColumnProfile{}; 8];
    let scratch: [8]string = [""; 8];
    const ncols = inferCsvSchema(csv, CsvTableOptions.csv(), profiles[0..], scratch[0..]) catch unreachable;
    expect_eq(ncols, 6);

    // col 0: int_col [1,2,3,4] -> mean=2.5, pop_var=1.25
    expect_eq(profiles[0].kind, CsvColumnKind.int_col);
    expect_eq(profiles[0].non_null_count, 4);
    expect_eq(profiles[0].min_val, 1.0);
    expect_eq(profiles[0].max_val, 4.0);
    expect_true(absF64(profiles[0].mean_val - 2.5) < 0.0001);
    expect_true(absF64(profiles[0].variance() - 1.25) < 0.0001);

    // col 1: int promoted to float_col [10, 20.5, 30] + 1 null -> sum=60.5
    expect_eq(profiles[1].kind, CsvColumnKind.float_col);
    expect_eq(profiles[1].non_null_count, 3);
    expect_eq(profiles[1].null_count, 1);
    expect_true(absF64(profiles[1].sum_val - 60.5) < 0.0001);

    // col 2: bool_col, col 3: iso_date_col, col 4: string_col, col 5: empty
    expect_eq(profiles[2].kind, CsvColumnKind.bool_col);
    expect_eq(profiles[3].kind, CsvColumnKind.iso_date_col);
    expect_eq(profiles[4].kind, CsvColumnKind.string_col);
    expect_eq(profiles[4].max_str_len, 5);
    expect_eq(profiles[5].kind, CsvColumnKind.empty);
    expect_eq(profiles[5].null_count, 4);
}

test("Multi-predicate AND filter & column projection (`projectAndFilterCsv`)") {
    const csv = "id,dept,name,salary\n"
        ++ "1,eng,Alice,150\n"
        ++ "2,sales,Bob,120\n"
        ++ "3,eng,Alicia,180\n"
        ++ "4,eng,Charlie,95\n";

    // Filter: dept == "eng" AND salary >= 120 AND name prefix "Ali" -> rows 1 & 3, projected to [name, salary]
    const preds: [3]CsvFilterPred = [
        CsvFilterPred{ col_index: 1, op: CsvFilterOp.eq, str_val: "eng" },
        CsvFilterPred{ col_index: 3, op: CsvFilterOp.gte, num_val: 120.0, numeric_compare: true },
        CsvFilterPred{ col_index: 2, op: CsvFilterOp.prefix, str_val: "Ali" },
    ];
    const proj_cols: [2]usize = [2, 3];
    let sf: [8]string = [""; 8];
    let sp: [4]string = [""; 4];
    let out: [256]u8 = [0; 256];

    const n = projectAndFilterCsv(csv, CsvTableOptions.csv(), proj_cols[0..], preds[0..], sf[0..], sp[0..], out[0..]) catch unreachable;
    expect_true(bytesEq(out[0..n], "name,salary\nAlice,150\nAlicia,180\n"));
}

test("Zero-heap group-by aggregation (`aggregateCsvByGroup`: count/sum/min/max/avg)") {
    const csv = "region,amount\n"
        ++ "us,100\n"
        ++ "eu,50\n"
        ++ "us,200\n"
        ++ "ap,75\n"
        ++ "eu,150\n"
        ++ "us,300\n";

    let groups: [8]CsvGroupAggSlot = [CsvGroupAggSlot{}; 8];
    let sf: [4]string = [""; 4];
    const ng = aggregateCsvByGroup(csv, CsvTableOptions.csv(), 0, 1, groups[0..], sf[0..]) catch unreachable;
    expect_eq(ng, 3);

    // Group 0: "us" -> count=3, sum=600, min=100, max=300, avg=200
    expect_true(bytesEq(groups[0].key, "us"));
    expect_eq(groups[0].count, 3);
    expect_eq(groups[0].metric(CsvAggKind.sum), 600.0);
    expect_eq(groups[0].metric(CsvAggKind.min), 100.0);
    expect_eq(groups[0].metric(CsvAggKind.max), 300.0);
    expect_eq(groups[0].metric(CsvAggKind.avg), 200.0);

    // Group 1: "eu" -> count=2, sum=200, avg=100
    expect_true(bytesEq(groups[1].key, "eu"));
    expect_eq(groups[1].count, 2);
    expect_eq(groups[1].avg(), 100.0);

    // Group 2: "ap" -> count=1, sum=75
    expect_true(bytesEq(groups[2].key, "ap"));
    expect_eq(groups[2].sum, 75.0);
}

test("Stable iterative merge sort (`sortCsvRowsByColumn`: numeric desc & string asc)") {
    const csv = "name,score\n"
        ++ "Charlie,80\n"
        ++ "Alice,95\n"
        ++ "Bob,80\n"
        ++ "Dana,100\n";

    let rows: [8]CsvRowSpan = [CsvRowSpan{}; 8];
    let tmp: [8]CsvRowSpan = [CsvRowSpan{}; 8];
    let sf: [4]string = [""; 4];
    let out: [256]u8 = [0; 256];

    // Numeric descending on score (col 1): Dana(100), Alice(95), Charlie(80), Bob(80) — stable tie order!
    const n1 = sortCsvRowsByColumn(csv, CsvTableOptions.csv(), 1, true, true, rows[0..], tmp[0..], sf[0..], out[0..]) catch unreachable;
    expect_true(bytesEq(out[0..n1], "name,score\nDana,100\nAlice,95\nCharlie,80\nBob,80\n"));

    // Lexicographic ascending on name (col 0): Alice, Bob, Charlie, Dana
    const n2 = sortCsvRowsByColumn(csv, CsvTableOptions.csv(), 0, false, false, rows[0..], tmp[0..], sf[0..], out[0..]) catch unreachable;
    expect_true(bytesEq(out[0..n2], "name,score\nAlice,95\nBob,80\nCharlie,80\nDana,100\n"));
}

test("Bidirectional RFC 4180 CSV <-> RFC 8259 JSONL (NDJSON) roundtrip") {
    const csv = "id,name,active,note\n"
        ++ "1,\"Alice, PhD\",true,\"say \"\"hi\"\"\"\n"
        ++ "2,Bob,false,\n";

    let profiles: [4]CsvColumnProfile = [CsvColumnProfile{}; 4];
    let sh: [4]string = [""; 4];
    let sf: [4]string = [""; 4];
    const nc = inferCsvSchema(csv, CsvTableOptions.csv(), profiles[0..], sf[0..]) catch unreachable;
    expect_eq(nc, 4);

    let jsonl_buf: [512]u8 = [0; 512];
    const jlen = csvToJsonl(csv, CsvTableOptions.csv(), profiles[0..nc], sh[0..], sf[0..], jsonl_buf[0..]) catch unreachable;
    const expected_jsonl = "{\"id\":1,\"name\":\"Alice, PhD\",\"active\":true,\"note\":\"say \\\"hi\\\"\"}\n"
        ++ "{\"id\":2,\"name\":\"Bob\",\"active\":false,\"note\":null}\n";
    expect_true(bytesEq(jsonl_buf[0..jlen], expected_jsonl));

    // Roundtrip JSONL -> CSV
    const cols: [4]string = ["id", "name", "active", "note"];
    let unesc: [256]u8 = [0; 256];
    let csv_back: [512]u8 = [0; 512];
    const clen = jsonlToCsv(jsonl_buf[0..jlen] as string, cols[0..], ',', sf[0..], unesc[0..], csv_back[0..]) catch unreachable;
    expect_true(bytesEq(csv_back[0..clen], csv));
}

test("CSV malformed unclosed quote, strict column mismatch, invalid JSONL & cap.doc parity") {
    // 1. Unclosed quote -> MalformedQuote in inferCsvSchema
    let profs: [4]CsvColumnProfile = [CsvColumnProfile{}; 4];
    let sf: [4]string = [""; 4];
    let got_bad_q: bool = false;
    * = inferCsvSchema("a,b\n1,\"unclosed\n", CsvTableOptions.csv(), profs[0..], sf[0..]) catch |e| {
        if (e == CsvTableError.MalformedQuote) got_bad_q = true;
    };
    expect_true(got_bad_q);

    // 2. Strict column count mismatch
    let sr = CsvTableReader.init("a,b,c\n1,2\n", CsvTableOptions{ strict_column_count: true });
    * = sr.parseHeader(sf[0..]);
    const bad_row = sr.nextRow(sf[0..]) orelse unreachable;
    expect_true(bad_row.malformed);

    // 3. Invalid JSONL (missing '{')
    const cols: [1]string = ["a"];
    let unesc: [32]u8 = [0; 32];
    let out: [64]u8 = [0; 64];
    let got_bad_json: bool = false;
    * = jsonlToCsv("not-json\n", cols[0..], ',', sf[0..], unesc[0..], out[0..]) catch |e| {
        if (e == CsvTableError.InvalidJsonl) got_bad_json = true;
    };
    expect_true(got_bad_json);

    // 4. cap.doc namespace parity
    const d = cap.doc.sniffCsvDialect("a|b\n1|2\n", 4);
    expect_eq(d.delimiter, '|');
    expect_true(cap.doc.isIsoDateField("2026-11-23"));
}
