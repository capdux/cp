import { test, expect_eq } from "cap:test";
import {
    DiagSeverity,
    Diagnostic,
    severityLabel,
    locateOffset,
    renderDiagnostic,
} from "../../packages/@cap/codespan-reporting/src/index.cp";

fn main() {
    test("1. @cap/codespan-reporting — locateOffset resolves line, column, and line slice", () => {
        const src = "let a = 1;\nlet b = foo + 2;\nreturn b;";
        const loc = locateOffset(src, 19); // 'f' in "foo"
        expect_eq(loc.line, 2 as usize);
        expect_eq(loc.col, 9 as usize);
        expect_eq(src[loc.line_start..loc.line_end], "let b = foo + 2;");
    });

    test("2. @cap/codespan-reporting — renders full Rustc-style error with code, label, and note", () => {
        const src = "let a = 1;\nlet b = foo + 2;";
        const diag = Diagnostic {
            severity: .Error,
            code: "E0425",
            message: "cannot find value `foo`",
            file_name: "src/main.cp",
            span_start: 19,
            span_end: 22,
            label: "not found in this scope",
            note: "did you mean `a`?",
        };
        let buf: [512]u8 = [0; 512];
        const n = renderDiagnostic(src, diag, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "error[E0425]: cannot find value `foo`\n  --> src/main.cp:2:9\n  |\n2 | let b = foo + 2;\n  |         ^^^ not found in this scope\n  = note: did you mean `a`?\n",
        );
    });

    test("3. @cap/codespan-reporting — renders warning without code or note cleanly", () => {
        const src = "const unused = 42;";
        const diag = Diagnostic {
            severity: .Warning,
            code: "",
            message: "unused variable",
            file_name: "app.cp",
            span_start: 6,
            span_end: 12,
            label: "",
            note: "",
        };
        let buf: [512]u8 = [0; 512];
        const n = renderDiagnostic(src, diag, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "warning: unused variable\n  --> app.cp:1:7\n  |\n1 | const unused = 42;\n  |       ^^^^^^\n",
        );
    });

    test("4. @cap/codespan-reporting — clamps zero-length span to single caret", () => {
        const src = "fn main() {}";
        const diag = Diagnostic {
            severity: .Note,
            code: "",
            message: "here",
            file_name: "t.cp",
            span_start: 3,
            span_end: 3,
            label: "point",
            note: "",
        };
        let buf: [256]u8 = [0; 256];
        const n = renderDiagnostic(src, diag, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "note: here\n  --> t.cp:1:4\n  |\n1 | fn main() {}\n  |    ^ point\n",
        );
    });

    test("5. @cap/codespan-reporting — handles CRLF line endings cleanly", () => {
        const src = "line1\r\nline2\r\nline3";
        const loc = locateOffset(src, 7);
        expect_eq(loc.line, 2 as usize);
        expect_eq(loc.col, 1 as usize);
        expect_eq(src[loc.line_start..loc.line_end], "line2");
    });

    test("6. @cap/codespan-reporting — severityLabel maps all 5 severity levels", () => {
        expect_eq(severityLabel(.Bug), "bug");
        expect_eq(severityLabel(.Error), "error");
        expect_eq(severityLabel(.Warning), "warning");
        expect_eq(severityLabel(.Note), "note");
        expect_eq(severityLabel(.Help), "help");
    });
}
