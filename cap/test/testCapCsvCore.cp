// cap/test/testCapCsvCore.cp
// Comprehensive 6-test suite for @cap/csv-core (RFC 4180 streaming CSV, Rust csv-core v0.2 parity)

import { test, expect_eq } from "cap:test";
import {
    CSV_READ_INPUT_EMPTY,
    CSV_READ_OUTPUT_FULL,
    CSV_READ_FIELD,
    CSV_READ_RECORD_END,
    CSV_READ_END,
    CsvReadResult,
    CsvReader,
    CsvWriter,
    countCsvRecords,
    countCsvFieldsInLine
} from "../../packages/@cap/csv-core/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/csv-core — Standard multi-row CSV streaming fields, records & counters", () => {
        let data = "name,age,city\nalice,30,sydney\nbob,25,tokyo\n";
        expect_eq(countCsvRecords(data), 3 as usize);
        expect_eq(countCsvFieldsInLine(data), 3 as usize);

        let rdr = CsvReader.new();
        let out: [64]u8 = [0; 64];
        let pos: usize = 0;

        let r1 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + r1.bytes_read;
        expect_eq(r1.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..r1.bytes_written], "name"), true);

        let r2 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + r2.bytes_read;
        expect_eq(r2.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..r2.bytes_written], "age"), true);

        let r3 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + r3.bytes_read;
        expect_eq(r3.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..r3.bytes_written], "city"), true);

        let r4 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + r4.bytes_read;
        expect_eq(r4.status, CSV_READ_RECORD_END);
        expect_eq(rdr.line(), 2 as u64);
    });

    test("2. @cap/csv-core — RFC 4180 quoted commas, embedded newlines & doubled-quote escaping", () => {
        let data = "\"hello, world\",\"line1\nline2\",\"say \"\"hi\"\"\"\n";
        expect_eq(countCsvRecords(data), 1 as usize);
        expect_eq(countCsvFieldsInLine(data), 3 as usize);

        let rdr = CsvReader.new();
        let out: [64]u8 = [0; 64];
        let pos: usize = 0;

        let f1 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + f1.bytes_read;
        expect_eq(f1.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..f1.bytes_written], "hello, world"), true);

        let f2 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + f2.bytes_read;
        expect_eq(f2.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..f2.bytes_written], "line1\nline2"), true);

        let f3 = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + f3.bytes_read;
        expect_eq(f3.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..f3.bytes_written], "say \"hi\""), true);

        let rec = rdr.readField(data[pos..data.len], out[0..64]);
        pos = pos + rec.bytes_read;
        expect_eq(rec.status, CSV_READ_RECORD_END);
    });

    test("3. @cap/csv-core — CRLF line endings & custom TSV/semicolon delimiters", () => {
        let crlf = "x;y\r\n1;2\r\n";
        let rdr = CsvReader.withDelimiter(0x3B); // ';'
        let out: [32]u8 = [0; 32];
        let pos: usize = 0;

        let a = rdr.readField(crlf[pos..crlf.len], out[0..32]);
        pos = pos + a.bytes_read;
        expect_eq(eqBytes(out[0..a.bytes_written], "x"), true);

        let b = rdr.readField(crlf[pos..crlf.len], out[0..32]);
        pos = pos + b.bytes_read;
        expect_eq(eqBytes(out[0..b.bytes_written], "y"), true);

        let e1 = rdr.readField(crlf[pos..crlf.len], out[0..32]);
        pos = pos + e1.bytes_read;
        expect_eq(e1.status, CSV_READ_RECORD_END);

        let c = rdr.readField(crlf[pos..crlf.len], out[0..32]);
        pos = pos + c.bytes_read;
        expect_eq(eqBytes(out[0..c.bytes_written], "1"), true);

        // TSV with tab delimiter
        let tsv = "col1\tcol2\n";
        let tRdr = CsvReader.withDelimiter(0x09);
        let t1 = tRdr.readField(tsv, out[0..32]);
        expect_eq(t1.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..t1.bytes_written], "col1"), true);
    });

    test("4. @cap/csv-core — Chunked streaming resumption & tiny output buffer CSV_READ_OUTPUT_FULL", () => {
        let rdr = CsvReader.new();
        let tiny: [3]u8 = [0; 3];
        let acc: [32]u8 = [0; 32];
        let accLen: usize = 0;

        // Chunk 1 ends mid-field, chunk 2 finishes the field
        let chunk1 = "abcd";
        let chunk2 = "efg,next\n";
        let p1: usize = 0;
        while (p1 < chunk1.len) {
            let r = rdr.readField(chunk1[p1..chunk1.len], tiny[0..3]);
            p1 = p1 + r.bytes_read;
            let k: usize = 0;
            while (k < r.bytes_written) {
                acc[accLen] = tiny[k];
                accLen = accLen + 1;
                k = k + 1;
            }
            if (p1 < chunk1.len) {
                expect_eq(r.status, CSV_READ_OUTPUT_FULL);
            } else {
                expect_eq(r.status, CSV_READ_INPUT_EMPTY);
            }
        }

        let p2: usize = 0;
        let doneField = false;
        while (!doneField) {
            let r2 = rdr.readField(chunk2[p2..chunk2.len], tiny[0..3]);
            p2 = p2 + r2.bytes_read;
            let k2: usize = 0;
            while (k2 < r2.bytes_written) {
                acc[accLen] = tiny[k2];
                accLen = accLen + 1;
                k2 = k2 + 1;
            }
            if (r2.status == CSV_READ_FIELD) {
                doneField = true;
            }
        }
        expect_eq(eqBytes(acc[0..accLen], "abcdefg"), true);
    });

    test("5. @cap/csv-core — CsvWriter auto-quoting, writeRecord & roundtrip parse", () => {
        let w = CsvWriter.new();
        let buf: [128]u8 = [0; 128];
        let fields: [3]string = ["plain", "a,b", "say \"yo\""];
        let n = w.writeRecord(fields[0..3], buf[0..128]);
        expect_eq(eqBytes(buf[0..n], "plain,\"a,b\",\"say \"\"yo\"\"\"\n"), true);

        // Roundtrip back through CsvReader
        let rdr = CsvReader.new();
        let out: [64]u8 = [0; 64];
        let pos: usize = 0;
        let r1 = rdr.readField(buf[pos..n], out[0..64]);
        pos = pos + r1.bytes_read;
        expect_eq(eqBytes(out[0..r1.bytes_written], "plain"), true);

        let r2 = rdr.readField(buf[pos..n], out[0..64]);
        pos = pos + r2.bytes_read;
        expect_eq(eqBytes(out[0..r2.bytes_written], "a,b"), true);

        let r3 = rdr.readField(buf[pos..n], out[0..64]);
        pos = pos + r3.bytes_read;
        expect_eq(eqBytes(out[0..r3.bytes_written], "say \"yo\""), true);
    });

    test("6. @cap/csv-core — Empty input, empty fields a,,b & missing trailing newline", () => {
        expect_eq(countCsvRecords(""), 0 as usize);
        expect_eq(countCsvFieldsInLine(""), 0 as usize);

        let noNl = "a,,b";
        expect_eq(countCsvRecords(noNl), 1 as usize);
        expect_eq(countCsvFieldsInLine(noNl), 3 as usize);

        let rdr = CsvReader.new();
        let out: [16]u8 = [0; 16];
        let pos: usize = 0;

        let f1 = rdr.readField(noNl[pos..noNl.len], out[0..16]);
        pos = pos + f1.bytes_read;
        expect_eq(f1.status, CSV_READ_FIELD);
        expect_eq(eqBytes(out[0..f1.bytes_written], "a"), true);

        let f2 = rdr.readField(noNl[pos..noNl.len], out[0..16]);
        pos = pos + f2.bytes_read;
        expect_eq(f2.status, CSV_READ_FIELD);
        expect_eq(f2.bytes_written, 0 as usize);

        let f3a = rdr.readField(noNl[pos..noNl.len], out[0..16]);
        pos = pos + f3a.bytes_read;
        expect_eq(f3a.status, CSV_READ_INPUT_EMPTY);
        expect_eq(eqBytes(out[0..f3a.bytes_written], "b"), true);

        // Signal EOF with empty slice
        let f3b = rdr.readField("", out[0..16]);
        expect_eq(f3b.status, CSV_READ_FIELD);
        let recEnd = rdr.readField("", out[0..16]);
        expect_eq(recEnd.status, CSV_READ_RECORD_END);
        let eofEnd = rdr.readField("", out[0..16]);
        expect_eq(eofEnd.status, CSV_READ_END);
    });
}
