import { test, expect_eq } from "cap:test";
import {
    JsonDiagKind,
    JSON_VAL_OBJECT,
    JSON_VAL_ARRAY,
    JSON_VAL_STRING,
    JSON_VAL_NUMBER,
    JSON_VAL_BOOL,
    JSON_VAL_NULL,
    validateJsonWithDiag,
    formatJsonError,
} from "../../packages/@cap/parse-json/src/index.cp";

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

fn main() {
    test("1. @cap/parse-json — Valid RFC 8259 JSON values & value_kind classification", () => {
        const r_obj = validateJsonWithDiag("{\"a\": [1, -2.5e+3, true, false, null, \"hi\\u0021\"]}");
        expect_eq(r_obj.ok, true);
        expect_eq(r_obj.kind, JsonDiagKind.Ok);
        expect_eq(r_obj.value_kind, JSON_VAL_OBJECT);

        const r_arr = validateJsonWithDiag(" [ 0, 10, \"ok\" ] ");
        expect_eq(r_arr.ok, true);
        expect_eq(r_arr.value_kind, JSON_VAL_ARRAY);

        const r_str = validateJsonWithDiag("\"hello\"");
        expect_eq(r_str.ok, true);
        expect_eq(r_str.value_kind, JSON_VAL_STRING);

        const r_num = validateJsonWithDiag("-0.25");
        expect_eq(r_num.ok, true);
        expect_eq(r_num.value_kind, JSON_VAL_NUMBER);

        const r_bool = validateJsonWithDiag("true");
        expect_eq(r_bool.ok, true);
        expect_eq(r_bool.value_kind, JSON_VAL_BOOL);

        const r_null = validateJsonWithDiag("null");
        expect_eq(r_null.ok, true);
        expect_eq(r_null.value_kind, JSON_VAL_NULL);
    });

    test("2. @cap/parse-json — Trailing comma in objects & arrays with exact line/column", () => {
        const obj_bad = "{\n  \"a\": 1,\n}";
        const d1 = validateJsonWithDiag(obj_bad);
        expect_eq(d1.ok, false);
        expect_eq(d1.kind, JsonDiagKind.TrailingComma);
        expect_eq(d1.line, 2);
        expect_eq(d1.column, 9);

        const arr_bad = "[1, 2,]";
        const d2 = validateJsonWithDiag(arr_bad);
        expect_eq(d2.ok, false);
        expect_eq(d2.kind, JsonDiagKind.TrailingComma);
        expect_eq(d2.line, 1);
        expect_eq(d2.column, 6);
    });

    test("3. @cap/parse-json — SingleQuotedString & UnquotedKey diagnostics", () => {
        const d_sq_val = validateJsonWithDiag("{\"msg\": 'hello'}");
        expect_eq(d_sq_val.ok, false);
        expect_eq(d_sq_val.kind, JsonDiagKind.SingleQuotedString);
        expect_eq(d_sq_val.line, 1);
        expect_eq(d_sq_val.column, 9);

        const d_sq_key = validateJsonWithDiag("{'key': 1}");
        expect_eq(d_sq_key.ok, false);
        expect_eq(d_sq_key.kind, JsonDiagKind.SingleQuotedString);
        expect_eq(d_sq_key.column, 2);

        const d_uq = validateJsonWithDiag("{\n  foo: 123\n}");
        expect_eq(d_uq.ok, false);
        expect_eq(d_uq.kind, JsonDiagKind.UnquotedKey);
        expect_eq(d_uq.line, 2);
        expect_eq(d_uq.column, 3);
    });

    test("4. @cap/parse-json — LeadingZero, UnescapedControlChar, EmptyInput & UnexpectedEnd", () => {
        const d_lz = validateJsonWithDiag("{\"n\": 0123}");
        expect_eq(d_lz.ok, false);
        expect_eq(d_lz.kind, JsonDiagKind.LeadingZero);
        expect_eq(d_lz.column, 7);

        const d_ctrl = validateJsonWithDiag("{\"s\": \"line1\nline2\"}");
        expect_eq(d_ctrl.ok, false);
        expect_eq(d_ctrl.kind, JsonDiagKind.UnescapedControlChar);
        expect_eq(d_ctrl.line, 1);
        expect_eq(d_ctrl.column, 13);

        const d_empty = validateJsonWithDiag("   \n  ");
        expect_eq(d_empty.ok, false);
        expect_eq(d_empty.kind, JsonDiagKind.EmptyInput);

        const d_eof = validateJsonWithDiag("{\"a\": [1, 2");
        expect_eq(d_eof.ok, false);
        expect_eq(d_eof.kind, JsonDiagKind.UnexpectedEnd);
    });

    test("5. @cap/parse-json — Trailing garbage & UnexpectedToken detection", () => {
        const d_extra = validateJsonWithDiag("{\"a\": 1} extra");
        expect_eq(d_extra.ok, false);
        expect_eq(d_extra.kind, JsonDiagKind.UnexpectedToken);
        expect_eq(d_extra.line, 1);
        expect_eq(d_extra.column, 10);

        const d_bad_esc = validateJsonWithDiag("\"bad\\xescape\"");
        expect_eq(d_bad_esc.ok, false);
        expect_eq(d_bad_esc.kind, JsonDiagKind.UnexpectedToken);
    });

    test("6. @cap/parse-json — formatJsonError codeFrame output with filename and caret", () => {
        const src = "{\n  \"port\": 8080,\n}";
        const diag = validateJsonWithDiag(src);
        expect_eq(diag.ok, false);

        let buf: [512]u8 = undefined;
        const n = formatJsonError(src, diag, "config.json", buf[0..512]);
        expect_eq(n > 0, true);

        const rendered = buf[0..n];
        expect_eq(containsSubstr(rendered, "JSONError: Trailing comma in JSON"), true);
        expect_eq(containsSubstr(rendered, "at line 2, column 15 in config.json"), true);
        expect_eq(containsSubstr(rendered, ">  2 |   \"port\": 8080,"), true);
        expect_eq(containsSubstr(rendered, "^"), true);

        // Valid JSON produces 0 bytes
        const ok_diag = validateJsonWithDiag("{\"ok\": true}");
        const n_ok = formatJsonError("{\"ok\": true}", ok_diag, "ok.json", buf[0..512]);
        expect_eq(n_ok, 0);
    });
}
