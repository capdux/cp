import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MAX_CSS_DIAGNOSTICS,
    CssDiagnosticSeverity,
    CssErrorLocation,
    CssParserErrorKind,
    CssPrinterErrorKind,
    CssDiagnosticDomain,
    CssDiagnostic,
    CssDiagnosticBag,
} from "cap:css";

fn main() {
    test("css_diagnostics: parser error formatting with dynamic details", () => {
        let buf: [256]u8 = [0; 256];

        const d1 = CssDiagnostic.parseError(
            CssParserErrorKind.at_rule_invalid,
            CssErrorLocation.init("app.css", 12, 3),
            "unknown-rule",
            "",
        );
        expect_eq(d1.isError(), true);
        expect_eq(d1.isWarning(), false);
        expect_eq(d1.domain == CssDiagnosticDomain.parser, true);
        expect_eq(std.mem.eql(u8, d1.formatMessage(buf[0..]).?, "Unknown at-rule @unknown-rule"), true);
        expect_eq(std.mem.eql(u8, d1.formatLine(buf[0..]).?, "app.css:12:3: error: Unknown at-rule @unknown-rule"), true);

        const d2 = CssDiagnostic.parseError(
            CssParserErrorKind.unexpected_value,
            null,
            "<length>",
            "auto",
        );
        expect_eq(std.mem.eql(u8, d2.formatLine(buf[0..]).?, "error: Expected <length>, received auto"), true);

        const d3 = CssDiagnostic.parseError(
            CssParserErrorKind.unexpected_token,
            CssErrorLocation.init("", 5, 9),
            "';'",
            "",
        );
        expect_eq(std.mem.eql(u8, d3.formatLine(buf[0..]).?, "5:9: error: Unexpected token: ';'"), true);
    });

    test("css_diagnostics: deprecated @nest produces warning severity", () => {
        let buf: [256]u8 = [0; 256];
        const w = CssDiagnostic.parseError(
            CssParserErrorKind.deprecated_nest_rule,
            CssErrorLocation.init("legacy.css", 2, 1),
            "",
            "",
        );
        expect_eq(w.severity == CssDiagnosticSeverity.warning, true);
        expect_eq(w.isWarning(), true);
        expect_eq(w.isError(), false);
        expect_eq(
            std.mem.eql(
                u8,
                w.formatLine(buf[0..]).?,
                "legacy.css:2:1: warning: The @nest rule is deprecated, use standard CSS nesting instead",
            ),
            true,
        );
    });

    test("css_diagnostics: printer & CSS Modules error formatting", () => {
        let buf: [256]u8 = [0; 256];

        const p1 = CssDiagnostic.printerError(
            CssPrinterErrorKind.ambiguous_url_in_custom_property,
            CssErrorLocation.init("vars.css", 8, 15),
            "./bg.png",
        );
        expect_eq(p1.domain == CssDiagnosticDomain.printer, true);
        expect_eq(
            std.mem.eql(
                u8,
                p1.formatLine(buf[0..]).?,
                "vars.css:8:15: error: Ambiguous relative URL './bg.png' in custom property declaration",
            ),
            true,
        );

        const p2 = CssDiagnostic.printerError(CssPrinterErrorKind.invalid_composes_nesting, null, "");
        expect_eq(
            std.mem.eql(
                u8,
                p2.formatMessage(buf[0..]).?,
                "The 'composes' property cannot be used within nested rules",
            ),
            true,
        );

        const p3 = CssDiagnostic.printerError(CssPrinterErrorKind.invalid_css_modules_pattern_in_grid, null, "");
        expect_eq(
            std.mem.eql(
                u8,
                p3.formatMessage(buf[0..]).?,
                "CSS modules pattern must end with '[local]' when used in CSS grid",
            ),
            true,
        );
    });

    test("css_diagnostics: CssDiagnosticBag counts, formatAll & clear", () => {
        let bag = CssDiagnosticBag.init();
        expect_eq(bag.hasErrors(), false);
        expect_eq(bag.count(), 0 as usize);

        expect_eq(bag.push(CssDiagnostic.parseError(
            CssParserErrorKind.deprecated_nest_rule,
            CssErrorLocation.init("a.css", 1, 1),
            "",
            "",
        )), true);
        expect_eq(bag.hasErrors(), false);
        expect_eq(bag.warningCount(), 1 as usize);

        expect_eq(bag.push(CssDiagnostic.parseError(
            CssParserErrorKind.maximum_nesting_depth,
            CssErrorLocation.init("a.css", 40, 2),
            "",
            "",
        )), true);
        expect_eq(bag.hasErrors(), true);
        expect_eq(bag.errorCount(), 1 as usize);
        expect_eq(bag.count(), 2 as usize);

        let out: [512]u8 = [0; 512];
        const all = bag.formatAll(out[0..]).?;
        expect_eq(std.mem.indexOf(u8, all, "a.css:1:1: warning:") != null, true);
        expect_eq(std.mem.indexOf(u8, all, "\na.css:40:2: error: Maximum CSS nesting depth exceeded") != null, true);

        const first: CssDiagnostic = bag.get(0).?;
        expect_eq(first.isWarning(), true);
        expect_eq(bag.get(99) == null, true);

        bag.clear();
        expect_eq(bag.count(), 0 as usize);
        expect_eq(bag.hasErrors(), false);
    });

    test("css_diagnostics: buffer overflow safety & CssErrorLocation equality", () => {
        const l1 = CssErrorLocation.init("x.css", 3, 7);
        const l2 = CssErrorLocation.init("x.css", 3, 7);
        const l3 = CssErrorLocation.init("y.css", 3, 7);
        expect_eq(l1.eql(l2), true);
        expect_eq(l1.eql(l3), false);

        const d = CssDiagnostic.parseError(CssParserErrorKind.unexpected_import_rule, l1, "", "");
        let tiny: [10]u8 = [0; 10];
        expect_eq(d.formatMessage(tiny[0..]) == null, true);
        expect_eq(d.formatLine(tiny[0..]) == null, true);
    });

    test("css_diagnostics: CssDiagnosticBag capacity limit & dropped counter", () => {
        let bag = CssDiagnosticBag.init();
        let i: usize = 0;
        while (i < MAX_CSS_DIAGNOSTICS) : (i += 1) {
            expect_eq(bag.push(CssDiagnostic.parseError(CssParserErrorKind.invalid_value, null, "", "")), true);
        }
        expect_eq(bag.count(), MAX_CSS_DIAGNOSTICS);
        expect_eq(bag.droppedCount(), 0 as usize);

        expect_eq(bag.push(CssDiagnostic.parseError(CssParserErrorKind.end_of_input, null, "", "")), false);
        expect_eq(bag.push(CssDiagnostic.parseError(CssParserErrorKind.end_of_input, null, "", "")), false);
        expect_eq(bag.droppedCount(), 2 as usize);
    });
}
