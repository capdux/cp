// Source: Absorbed from /data/dev/bun/src/css/error.cp | Synced: 2026-11-22T01:43:00Z
// Pure-CP zero-allocation LightningCSS / Bun CSS structured diagnostics:
// parser & printer error codes, source locations, human-readable formatter, and bounded diagnostic bag.

pub const MAX_CSS_DIAGNOSTICS: usize = 32;

/// Severity level of a CSS diagnostic.
pub enum CssDiagnosticSeverity: u8 {
    err = 0,
    warning = 1,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .err => "error",
            .warning => "warning",
        };
    }
}

/// Source file location for a CSS diagnostic (1-based line and column).
pub struct CssErrorLocation {
    filename: []const u8 = "",
    line: u32 = 1,
    column: u32 = 1,

    pub static fn init(filename: []const u8, line: u32, column: u32): CssErrorLocation {
        return CssErrorLocation{
            filename: filename,
            line: line,
            column: column,
        };
    }

    pub fn eql(other: CssErrorLocation): bool {
        return _.line == other.line && _.column == other.column && strEq(_.filename, other.filename);
    }
}

/// CSS Parser error codes (mirrors Bun `src/css/error.cp` `ParserError`).
pub enum CssParserErrorKind: u8 {
    at_rule_body_invalid = 0,
    at_rule_prelude_invalid = 1,
    at_rule_invalid = 2,
    end_of_input = 3,
    invalid_declaration = 4,
    invalid_media_query = 5,
    invalid_nesting = 6,
    deprecated_nest_rule = 7,
    invalid_page_selector = 8,
    invalid_value = 9,
    qualified_rule_invalid = 10,
    selector_error = 11,
    unexpected_import_rule = 12,
    unexpected_namespace_rule = 13,
    unexpected_token = 14,
    maximum_nesting_depth = 15,
    unexpected_value = 16,

    /// Static default message matching Bun / LightningCSS wording.
    pub fn defaultMessage(): []const u8 {
        return switch (_) {
            .at_rule_body_invalid => "Invalid at-rule body",
            .at_rule_prelude_invalid => "Invalid at-rule prelude",
            .at_rule_invalid => "Unknown at-rule",
            .end_of_input => "Unexpected end of input",
            .invalid_declaration => "Invalid declaration",
            .invalid_media_query => "Invalid media query",
            .invalid_nesting => "Invalid CSS nesting",
            .deprecated_nest_rule => "The @nest rule is deprecated, use standard CSS nesting instead",
            .invalid_page_selector => "Invalid @page selector",
            .invalid_value => "Invalid value",
            .qualified_rule_invalid => "Invalid qualified rule",
            .selector_error => "Invalid selector",
            .unexpected_import_rule => "@import rules must come before any other rules except @charset and @layer",
            .unexpected_namespace_rule => "@namespace rules must come before any other rules except @charset, @import, and @layer",
            .unexpected_token => "Unexpected token",
            .maximum_nesting_depth => "Maximum CSS nesting depth exceeded",
            .unexpected_value => "Unexpected value",
        };
    }
}

/// CSS Printer & CSS Modules error codes (mirrors Bun `src/css/error.cp` `PrinterErrorKind`).
pub enum CssPrinterErrorKind: u8 {
    ambiguous_url_in_custom_property = 0,
    fmt_error = 1,
    invalid_composes_nesting = 2,
    invalid_composes_selector = 3,
    invalid_css_modules_pattern_in_grid = 4,
    no_import_records = 5,

    /// Static default message matching Bun / LightningCSS wording.
    pub fn defaultMessage(): []const u8 {
        return switch (_) {
            .ambiguous_url_in_custom_property => "Ambiguous relative URL in custom property declaration",
            .fmt_error => "Formatting error occurred",
            .invalid_composes_nesting => "The 'composes' property cannot be used within nested rules",
            .invalid_composes_selector => "The 'composes' property can only be used with a simple class selector",
            .invalid_css_modules_pattern_in_grid => "CSS modules pattern must end with '[local]' when used in CSS grid",
            .no_import_records => "No import records found",
        };
    }
}

/// Discriminator for parser vs printer diagnostics.
pub enum CssDiagnosticDomain: u8 {
    parser = 0,
    printer = 1,
}

/// Structured CSS diagnostic entry with optional dynamic context fields (`detail_a`, `detail_b`).
pub struct CssDiagnostic {
    domain: CssDiagnosticDomain = .parser,
    severity: CssDiagnosticSeverity = .err,
    parser_kind: CssParserErrorKind = .invalid_value,
    printer_kind: CssPrinterErrorKind = .fmt_error,
    loc: CssErrorLocation? = null,
    /// Primary dynamic detail (e.g. at-rule name, token text, expected value, or ambiguous URL).
    detail_a: []const u8 = "",
    /// Secondary dynamic detail (e.g. received value for `unexpected_value`).
    detail_b: []const u8 = "",

    pub static fn parseError(
        kind: CssParserErrorKind,
        loc: CssErrorLocation?,
        detail_a: []const u8,
        detail_b: []const u8,
    ): CssDiagnostic {
        const sev = if (kind == CssParserErrorKind.deprecated_nest_rule)
            CssDiagnosticSeverity.warning
        else
            CssDiagnosticSeverity.err;
        return CssDiagnostic{
            domain: .parser,
            severity: sev,
            parser_kind: kind,
            loc: loc,
            detail_a: detail_a,
            detail_b: detail_b,
        };
    }

    pub static fn printerError(
        kind: CssPrinterErrorKind,
        loc: CssErrorLocation?,
        detail: []const u8,
    ): CssDiagnostic {
        return CssDiagnostic{
            domain: .printer,
            severity: .err,
            printer_kind: kind,
            loc: loc,
            detail_a: detail,
        };
    }

    pub fn isError(): bool {
        return _.severity == CssDiagnosticSeverity.err;
    }

    pub fn isWarning(): bool {
        return _.severity == CssDiagnosticSeverity.warning;
    }

    /// Format the human-readable diagnostic message into `out` (returns `null` if buffer too small).
    pub fn formatMessage(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (_.domain == CssDiagnosticDomain.parser) {
            switch (_.parser_kind) {
                .at_rule_invalid => {
                    if (_.detail_a.len > 0) {
                        if (!appendBytes(out, &pos, "Unknown at-rule @")) return null;
                        if (!appendBytes(out, &pos, _.detail_a)) return null;
                        return out[0..pos];
                    }
                },
                .unexpected_token => {
                    if (_.detail_a.len > 0) {
                        if (!appendBytes(out, &pos, "Unexpected token: ")) return null;
                        if (!appendBytes(out, &pos, _.detail_a)) return null;
                        return out[0..pos];
                    }
                },
                .selector_error => {
                    if (_.detail_a.len > 0) {
                        if (!appendBytes(out, &pos, "Invalid selector. ")) return null;
                        if (!appendBytes(out, &pos, _.detail_a)) return null;
                        return out[0..pos];
                    }
                },
                .unexpected_value => {
                    if (_.detail_a.len > 0 || _.detail_b.len > 0) {
                        if (!appendBytes(out, &pos, "Expected ")) return null;
                        if (!appendBytes(out, &pos, _.detail_a)) return null;
                        if (!appendBytes(out, &pos, ", received ")) return null;
                        if (!appendBytes(out, &pos, _.detail_b)) return null;
                        return out[0..pos];
                    }
                },
                * => {},
            }
            if (!appendBytes(out, &pos, _.parser_kind.defaultMessage())) return null;
            return out[0..pos];
        } else {
            if (_.printer_kind == CssPrinterErrorKind.ambiguous_url_in_custom_property && _.detail_a.len > 0) {
                if (!appendBytes(out, &pos, "Ambiguous relative URL '")) return null;
                if (!appendBytes(out, &pos, _.detail_a)) return null;
                if (!appendBytes(out, &pos, "' in custom property declaration")) return null;
                return out[0..pos];
            }
            if (!appendBytes(out, &pos, _.printer_kind.defaultMessage())) return null;
            return out[0..pos];
        }
    }

    /// Format full diagnostic line: `[filename:line:col: ]<severity>: <message>`.
    pub fn formatLine(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (loc := _.loc) {
            if (loc.filename.len > 0) {
                if (!appendBytes(out, &pos, loc.filename)) return null;
                if (!appendBytes(out, &pos, ":")) return null;
            }
            if (!appendU32(out, &pos, loc.line)) return null;
            if (!appendBytes(out, &pos, ":")) return null;
            if (!appendU32(out, &pos, loc.column)) return null;
            if (!appendBytes(out, &pos, ": ")) return null;
        }
        if (!appendBytes(out, &pos, _.severity.asStr())) return null;
        if (!appendBytes(out, &pos, ": ")) return null;
        const msg = _.formatMessage(out[pos..]) ?? return null;
        pos += msg.len;
        return out[0..pos];
    }
}

/// Fixed-capacity zero-allocation collector for CSS diagnostics (up to 32 entries).
pub struct CssDiagnosticBag {
    items: [MAX_CSS_DIAGNOSTICS]CssDiagnostic = [CssDiagnostic{}; MAX_CSS_DIAGNOSTICS],
    len: usize = 0,
    dropped: usize = 0,

    pub static fn init(): CssDiagnosticBag {
        return CssDiagnosticBag{};
    }

    pub fn push(*_, diag: CssDiagnostic): bool {
        if (_.len >= MAX_CSS_DIAGNOSTICS) {
            _.dropped += 1;
            return false;
        }
        _.items[_.len] = diag;
        _.len += 1;
        return true;
    }

    pub fn clear(*_) {
        _.len = 0;
        _.dropped = 0;
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn droppedCount(): usize {
        return _.dropped;
    }

    pub fn errorCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.items[i].isError()) n += 1;
        }
        return n;
    }

    pub fn warningCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.items[i].isWarning()) n += 1;
        }
        return n;
    }

    pub fn hasErrors(): bool {
        return _.errorCount() > 0;
    }

    pub fn get(index: usize): CssDiagnostic? {
        if (index >= _.len) return null;
        return _.items[index];
    }

    /// Format all collected diagnostics separated by `\n` into `out`.
    pub fn formatAll(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (i > 0) {
                if (!appendBytes(out, &pos, "\n")) return null;
            }
            const line = _.items[i].formatLine(out[pos..]) ?? return null;
            pos += line.len;
        }
        return out[0..pos];
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn appendU32(out: []u8, pos: *usize, v: u32): bool {
    let tmp: [10]u8 = [0; 10];
    let n: usize = 0;
    let val = v;
    if (val == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (val > 0) {
            tmp[n] = @as(u8, '0' + @as(u8, @intCast(val % 10)));
            val /= 10;
            n += 1;
        }
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}
