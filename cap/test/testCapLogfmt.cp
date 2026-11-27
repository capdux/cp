import { test, expect_eq } from "cap:test";
import { LogfmtRecord, LogfmtWriter } from "../../packages/logfmt/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/logfmt — Basic level=info path=/api status=200 parsing & last-wins", () => {
        let rec = LogfmtRecord.parse("level=info path=/api/v1 status=200 level=warn");
        expect_eq(rec.len(), 4 as usize);
        expect_eq(rec.has("path"), true);
        expect_eq(rec.has("missing"), false);
        expect_eq(testEq(rec.get("path"), "/api/v1"), true);
        expect_eq(testEq(rec.get("status"), "200"), true);
        // Last occurrence wins
        expect_eq(testEq(rec.get("level"), "warn"), true);
    });

    test("2. @cap/logfmt — Quoted values with spaces, '=', escaped quotes & newlines", () => {
        const line = "msg=\"hello \\\"world\\\" a=b\\nline2\" svc=auth";
        let rec = LogfmtRecord.parse(line);
        expect_eq(rec.len(), 2 as usize);
        expect_eq(testEq(rec.get("msg"), "hello \"world\" a=b\nline2"), true);
        expect_eq(testEq(rec.get("svc"), "auth"), true);
    });

    test("3. @cap/logfmt — Bare flag key 'verbose' -> 'true' & empty value 'err='", () => {
        let rec = LogfmtRecord.parse("verbose err= dry_run count=0");
        expect_eq(rec.len(), 4 as usize);
        expect_eq(testEq(rec.get("verbose"), "true"), true);
        expect_eq(rec.has("err"), true);
        expect_eq(testEq(rec.get("err"), ""), true);
        expect_eq(testEq(rec.get("dry_run"), "true"), true);
        expect_eq(testEq(rec.get("count"), "0"), true);
    });

    test("4. @cap/logfmt — LogfmtWriter strings, negative ints, bools & auto-quoting", () => {
        let buf: [256]u8 = undefined;
        let w = LogfmtWriter.init(buf[0..256]);
        expect_eq(w.writeStr("level", "info"), true);
        expect_eq(w.writeStr("msg", "request \"ok\"\ndone"), true);
        expect_eq(w.writeInt("delta", -42), true);
        expect_eq(w.writeBool("cached", true), true);
        expect_eq(w.writeStr("empty", ""), true);

        const expected = "level=info msg=\"request \\\"ok\\\"\\ndone\" delta=-42 cached=true empty=\"\"";
        expect_eq(w.len(), expected.len);
        expect_eq(testEq(w.written(), expected), true);
    });

    test("5. @cap/logfmt — LogfmtWriter -> LogfmtRecord.parse full roundtrip", () => {
        let buf: [256]u8 = undefined;
        let w = LogfmtWriter.init(buf[0..256]);
        * = w.writeStr("service", "gateway");
        * = w.writeStr("query", "a=1 b=2");
        * = w.writeInt("latency_ms", 128);
        * = w.writeBool("ok", false);

        let rec = LogfmtRecord.parse(w.written());
        expect_eq(rec.len(), 4 as usize);
        expect_eq(testEq(rec.get("service"), "gateway"), true);
        expect_eq(testEq(rec.get("query"), "a=1 b=2"), true);
        expect_eq(testEq(rec.get("latency_ms"), "128"), true);
        expect_eq(testEq(rec.get("ok"), "false"), true);
    });

    test("6. @cap/logfmt — Empty line & writer buffer overflow protection", () => {
        let empty = LogfmtRecord.parse("   \t  ");
        expect_eq(empty.len(), 0 as usize);
        expect_eq(empty.has("a"), false);
        expect_eq(testEq(empty.get("a"), ""), true);

        let tiny: [12]u8 = undefined;
        let w = LogfmtWriter.init(tiny[0..12]);
        expect_eq(w.writeStr("a", "12345"), true); // "a=12345" (7 bytes)
        expect_eq(w.len(), 7 as usize);
        // Next field won't fit -> returns false and preserves previous 7 bytes atomically
        expect_eq(w.writeStr("overflow", "toolong"), false);
        expect_eq(w.len(), 7 as usize);
        expect_eq(testEq(w.written(), "a=12345"), true);
    });
}
