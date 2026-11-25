use crate::Span;
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, HashSet};
use std::path::Path;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct DiagnosticEdit {
    pub span_start: usize,
    pub span_end: usize,
    pub replacement: String,
    pub offset: usize,
    pub delete_len: usize,
    pub insert: String,
}

impl DiagnosticEdit {
    pub fn new(span_start: usize, span_end: usize, replacement: impl Into<String>) -> Self {
        let rep = replacement.into();
        let delete_len = span_end.saturating_sub(span_start);
        Self {
            span_start,
            span_end,
            replacement: rep.clone(),
            offset: span_start,
            delete_len,
            insert: rep,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct DiagnosticJson {
    pub code: String,
    pub severity: String, // "error" | "warning"
    pub message: String,
    pub file: String,
    pub line: usize,
    pub col: usize,
    pub span_start: usize,
    pub span_end: usize,
    pub cause: Option<String>,
    pub help: Option<String>,
    pub suggestion: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cascade_of: Option<usize>,
    pub edits: Vec<DiagnosticEdit>,
}

impl DiagnosticJson {
    pub fn to_json(&self) -> String {
        serde_json::to_string(self).unwrap_or_else(|_| "{}".to_string())
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct DiagnosticReport {
    pub status: String,
    pub errors: usize,
    pub root_errors: usize,
    pub diagnostics: Vec<DiagnosticJson>,
}

impl DiagnosticReport {
    pub fn ok() -> Self {
        Self {
            status: "ok".to_string(),
            errors: 0,
            root_errors: 0,
            diagnostics: Vec::new(),
        }
    }

    pub fn ok_json() -> String {
        serde_json::to_string(&Self::ok()).unwrap_or_else(|_| {
            "{\"status\":\"ok\",\"errors\":0,\"root_errors\":0,\"diagnostics\":[]}".to_string()
        })
    }

    pub fn error(diag: DiagnosticJson) -> Self {
        Self {
            status: "error".to_string(),
            errors: 1,
            root_errors: if diag.cascade_of.is_none() { 1 } else { 0 },
            diagnostics: vec![diag],
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct BriefGroup {
    pub count: usize,
    pub message: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub help: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub suggestion: Option<String>,
    pub lines: Vec<usize>,
    pub fixable_count: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct BriefDiagnosticReport {
    pub status: String,
    pub errors: usize,
    pub root_errors: usize,
    pub cascades_hidden: usize,
    pub fixable: usize,
    pub groups: BTreeMap<String, BriefGroup>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Diagnostic {
    pub code: String,
    pub message: String,
    pub span: Option<Span>,
    pub cause: Option<String>,
    pub help: Option<String>,
    pub suggestion: Option<String>,
    pub cascade_of: Option<usize>,
    pub explicit_edits: Vec<DiagnosticEdit>,
}

impl Diagnostic {
    pub fn new(code: impl Into<String>, message: impl Into<String>) -> Self {
        Self {
            code: code.into(),
            message: message.into(),
            span: None,
            cause: None,
            help: None,
            suggestion: None,
            cascade_of: None,
            explicit_edits: Vec::new(),
        }
    }

    pub fn with_span(mut self, span: Span) -> Self {
        self.span = Some(span);
        self
    }

    pub fn with_cause(mut self, cause: impl Into<String>) -> Self {
        self.cause = Some(cause.into());
        self
    }

    pub fn with_help(mut self, help: impl Into<String>) -> Self {
        self.help = Some(help.into());
        self
    }

    pub fn with_suggestion(mut self, suggestion: impl Into<String>) -> Self {
        self.suggestion = Some(suggestion.into());
        self
    }

    pub fn with_edit(
        mut self,
        span_start: usize,
        span_end: usize,
        replacement: impl Into<String>,
    ) -> Self {
        self.explicit_edits
            .push(DiagnosticEdit::new(span_start, span_end, replacement));
        self
    }

    pub fn render(&self, source_code: Option<&str>, file_path: Option<&Path>) -> String {
        self.render_dedup(source_code, file_path, false)
    }

    pub fn render_dedup(
        &self,
        source_code: Option<&str>,
        file_path: Option<&Path>,
        seen_code_before: bool,
    ) -> String {
        let mut out = String::new();
        out.push_str(&format!("error[{}]: {}\n", self.code, self.message));

        let file_display = file_path
            .map(|p| p.display().to_string())
            .unwrap_or_else(|| "<input>".to_string());

        if let (Some(code), Some(span)) = (source_code, self.span) {
            let (line_idx, col_idx, line_str) = find_line_and_col(code, span.start);
            let line_num = line_idx + 1;
            let col_num = col_idx + 1;

            out.push_str(&format!(
                "  --> {}:{}:{}\n",
                file_display, line_num, col_num
            ));
            out.push_str("   |\n");

            let line_prefix = format!("{:>4} | ", line_num);
            out.push_str(&line_prefix);
            out.push_str(line_str);
            if !line_str.ends_with('\n') {
                out.push('\n');
            }

            // Calculate underline length
            let underline_len = if span.end > span.start {
                (span.end - span.start)
                    .min(line_str.len().saturating_sub(col_idx))
                    .max(1)
            } else {
                1
            };

            let indent = " ".repeat(col_idx);
            let carets = "^".repeat(underline_len);
            out.push_str(&format!("   | {}{}\n", indent, carets));
            out.push_str("   |\n");
        } else if let Some(span) = self.span {
            out.push_str(&format!(
                "  --> {}: (span: {}..{})\n",
                file_display, span.start, span.end
            ));
        }

        if !seen_code_before {
            if let Some(cause) = &self.cause {
                out.push_str(&format!("   = cause: {}\n", cause));
            }
            if let Some(help) = &self.help {
                out.push_str(&format!("   = help: {}\n", help));
            }
        }
        if let Some(suggestion) = &self.suggestion {
            out.push_str(&format!("   = suggestion: {}\n", suggestion));
        }

        out
    }

    pub fn to_diagnostic_json(
        &self,
        source_code: Option<&str>,
        file_path: Option<&Path>,
    ) -> DiagnosticJson {
        let file = file_path
            .map(|p| p.display().to_string())
            .unwrap_or_else(|| "<input>".to_string());

        let (line, col, span_start, span_end) = if let Some(span) = self.span {
            let (l, c) = if let Some(code) = source_code {
                let (line_idx, col_idx, _) = find_line_and_col(code, span.start);
                (line_idx + 1, col_idx + 1)
            } else {
                (1, 1)
            };
            (l, c, span.start, span.end)
        } else {
            (1, 1, 0, 0)
        };

        let edits = self.generate_edits(source_code);

        DiagnosticJson {
            code: self.code.clone(),
            severity: "error".to_string(),
            message: self.message.clone(),
            file,
            line,
            col,
            span_start,
            span_end,
            cause: self.cause.clone(),
            help: self.help.clone(),
            suggestion: self.suggestion.clone(),
            cascade_of: self.cascade_of,
            edits,
        }
    }

    pub fn to_json(&self, source_code: Option<&str>, file_path: Option<&Path>) -> String {
        let diag_json = self.to_diagnostic_json(source_code, file_path);
        let report = DiagnosticReport::error(diag_json);
        serde_json::to_string(&report).unwrap_or_else(|_| "{}".to_string())
    }

    pub fn generate_edits(&self, source_code: Option<&str>) -> Vec<DiagnosticEdit> {
        if !self.explicit_edits.is_empty() {
            return self.explicit_edits.clone();
        }

        let mut edits = Vec::new();

        // 1. Missing parentheses in control flow statements (E0101)
        if self.code == "E0101"
            || self
                .suggestion
                .as_ref()
                .map_or(false, |s| s.contains("parentheses"))
            || self.message.contains("parentheses")
        {
            if let Some(span) = self.span {
                if let Some(code) = source_code {
                    let (start, end) = find_condition_range(code, span.start);
                    edits.push(DiagnosticEdit::new(start, start, "("));
                    edits.push(DiagnosticEdit::new(end, end, ")"));
                } else {
                    edits.push(DiagnosticEdit::new(span.start, span.start, "("));
                    edits.push(DiagnosticEdit::new(span.end, span.end, ")"));
                }
                return edits;
            }
        }

        // 2. Abolished 'interface' keyword (E0102) -> replace with 'trait'
        if self.code == "E0102"
            || self
                .suggestion
                .as_ref()
                .map_or(false, |s| s.contains("replace 'interface' with 'trait'"))
        {
            if let Some(span) = self.span {
                edits.push(DiagnosticEdit::new(span.start, span.end, "trait"));
                return edits;
            }
        }

        // 3. Abolished 'var' keyword (E0103) -> replace with 'let'
        if self.code == "E0103" {
            if let Some(span) = self.span {
                edits.push(DiagnosticEdit::new(span.start, span.end, "let"));
                return edits;
            }
        }

        // 4. Abolished 'mut' in var decl (E0104) -> remove 'mut'
        if self.code == "E0104"
            || self
                .suggestion
                .as_ref()
                .map_or(false, |s| s.contains("remove 'mut'"))
        {
            if let Some(span) = self.span {
                let del_end = if let Some(code) = source_code {
                    if code.as_bytes().get(span.end) == Some(&b' ') {
                        span.end + 1
                    } else {
                        span.end
                    }
                } else {
                    span.end
                };
                edits.push(DiagnosticEdit::new(span.start, del_end, ""));
                return edits;
            }
        }

        // 5. Prefix '?T' optional type (E0115) -> postfix 'T?'
        if self.code == "E0115" {
            if let (Some(span), Some(code)) = (self.span, source_code) {
                if span.end > span.start && span.end <= code.len() {
                    let snippet = &code[span.start..span.end];
                    if let Some(inner) = snippet.strip_prefix('?') {
                        edits.push(DiagnosticEdit::new(
                            span.start,
                            span.end,
                            format!("{}?", inner.trim()),
                        ));
                        return edits;
                    }
                }
            }
        }

        // 6. Missing ':' before function return type (E0116) -> insert ': '
        if self.code == "E0116" {
            if let Some(span) = self.span {
                let start = if let Some(code) = source_code {
                    if span.start > 0 && code.as_bytes().get(span.start - 1) == Some(&b' ') {
                        span.start - 1
                    } else {
                        span.start
                    }
                } else {
                    span.start
                };
                edits.push(DiagnosticEdit::new(start, span.start, ": "));
                return edits;
            }
        }

        // 7. Abolished 'any' type (E0119) -> replace with 'auto'
        if self.code == "E0119" {
            if let Some(span) = self.span {
                edits.push(DiagnosticEdit::new(span.start, span.end, "auto"));
                return edits;
            }
        }

        // 8. Abolished '_ = expr' discard (E0120) -> replace '_' with '*'
        if self.code == "E0120" {
            if let Some(span) = self.span {
                edits.push(DiagnosticEdit::new(span.start, span.end, "*"));
                return edits;
            }
        }

        // 9. Explicit 'self'/'this' receiver (E0121) -> replace with '*_' (or '_' if already preceded by '*')
        if self.code == "E0121" {
            if let Some(span) = self.span {
                let already_starred = source_code.map_or(false, |code| {
                    let bytes = code.as_bytes();
                    let mut p = span.start.min(bytes.len());
                    while p > 0 && (bytes[p - 1] == b' ' || bytes[p - 1] == b'\t') {
                        p -= 1;
                    }
                    p > 0 && bytes[p - 1] == b'*'
                });
                let repl = if already_starred { "_" } else { "*_" };
                edits.push(DiagnosticEdit::new(span.start, span.end, repl));
                return edits;
            }
        }

        // 10. Redundant ': void' return type (E0122) -> remove ': void'
        if self.code == "E0122" {
            if let Some(span) = self.span {
                if let Some(code) = source_code {
                    let bytes = code.as_bytes();
                    let mut start = span.start.min(bytes.len());
                    let mut p = start;
                    while p > 0 && (bytes[p - 1] == b' ' || bytes[p - 1] == b'\t') {
                        p -= 1;
                    }
                    if p > 0 && bytes[p - 1] == b':' {
                        p -= 1;
                        while p > 0 && (bytes[p - 1] == b' ' || bytes[p - 1] == b'\t') {
                            p -= 1;
                        }
                        start = p;
                    }
                    edits.push(DiagnosticEdit::new(start, span.end, ""));
                } else {
                    edits.push(DiagnosticEdit::new(span.start, span.end, ""));
                }
                return edits;
            }
        }

        // 11. Redundant read-only '_' receiver (E0123) -> remove '_' (and trailing ', ')
        if self.code == "E0123" {
            if let Some(span) = self.span {
                if let Some(code) = source_code {
                    let bytes = code.as_bytes();
                    let mut end = span.end.min(bytes.len());
                    let mut p = end;
                    while p < bytes.len() && (bytes[p] == b' ' || bytes[p] == b'\t') {
                        p += 1;
                    }
                    if p < bytes.len() && bytes[p] == b',' {
                        p += 1;
                        while p < bytes.len() && (bytes[p] == b' ' || bytes[p] == b'\t') {
                            p += 1;
                        }
                        end = p;
                    }
                    edits.push(DiagnosticEdit::new(span.start, end, ""));
                } else {
                    edits.push(DiagnosticEdit::new(span.start, span.end, ""));
                }
                return edits;
            }
        }

        // 12. Parameter read-only borrow (E0201) -> insert '*' after ':' for 'param: *T'
        if self.code == "E0201"
            && self
                .suggestion
                .as_ref()
                .map_or(false, |s| s.contains("Add '*'"))
        {
            if let (Some(span), Some(code)) = (self.span, source_code) {
                let bytes = code.as_bytes();
                let mut p = span.end.min(bytes.len());
                while p < bytes.len()
                    && bytes[p] != b':'
                    && bytes[p] != b','
                    && bytes[p] != b')'
                    && bytes[p] != b'\n'
                {
                    p += 1;
                }
                if p < bytes.len() && bytes[p] == b':' {
                    p += 1;
                    while p < bytes.len() && (bytes[p] == b' ' || bytes[p] == b'\t') {
                        p += 1;
                    }
                    if p < bytes.len() && bytes[p] != b'*' {
                        edits.push(DiagnosticEdit::new(p, p, "*"));
                        return edits;
                    }
                }
            }
        }

        // 13. Const reassignment (E0204) -> change 'const' to 'let' (or locate nearest declaration before span)
        if self.code == "E0204" {
            if let (Some(code), Some(target)) = (source_code, self.message.split('\'').nth(1)) {
                let pattern = format!("const {}", target);
                let search_limit = self.span.map(|s| s.start.min(code.len())).unwrap_or(code.len());
                if let Some(pos) = code[..search_limit].rfind(&pattern) {
                    edits.push(DiagnosticEdit::new(pos, pos + 5, "let"));
                    return edits;
                }
                if let Some(pos) = code.find(&pattern) {
                    edits.push(DiagnosticEdit::new(pos, pos + 5, "let"));
                    return edits;
                }
            }
        }

        // 13b. Reserved 'dec' keyword used as identifier (E0131) -> rename to 'dec_buf'
        if self.code == "E0131" {
            if let (Some(span), Some(code)) = (self.span, source_code) {
                let end = span.end.min(code.len());
                let start = span.start.min(end);
                if &code[start..end] == "dec" {
                    edits.push(DiagnosticEdit::new(start, end, "dec_buf"));
                    return edits;
                }
            }
        }

        // 13c. Optional-element slice `[]T?` mistaken for optional slice `([]T)?` (E0132)
        if self.code == "E0132" {
            if let (Some(span), Some(code)) = (self.span, source_code) {
                let end = span.end.min(code.len());
                let start = span.start.min(end);
                let slice_txt = &code[start..end];
                if slice_txt.starts_with("[]") && slice_txt.ends_with('?') {
                    let inner = &slice_txt[..slice_txt.len() - 1];
                    edits.push(DiagnosticEdit::new(start, end, format!("({})?", inner)));
                    return edits;
                }
            }
        }

        // 14. Undefined symbol with standard library import suggestion (E0206)
        if self.code == "E0206" {
            if let Some(sug) = &self.suggestion {
                if sug.starts_with("import { ") {
                    edits.push(DiagnosticEdit::new(0, 0, format!("{}\n", sug)));
                    return edits;
                }
            }
        }

        // 15. Read-only method mutation (E0130) -> auto insert '*_' into method signature
        if self.code == "E0130"
            || (self.code == "E0201"
                && (self.message.contains("read-only method")
                    || self
                        .suggestion
                        .as_ref()
                        .map_or(false, |s| s.contains("add '*_'"))))
        {
            if let (Some(span), Some(code)) = (self.span, source_code) {
                let bytes = code.as_bytes();
                let search_start = span.start.min(bytes.len());
                if let Some(fn_pos) = code[..search_start].rfind("fn ") {
                    if let Some(open_paren_offset) = code[fn_pos..search_start].find('(') {
                        let open_paren = fn_pos + open_paren_offset;
                        let mut p = open_paren + 1;
                        while p < bytes.len()
                            && (bytes[p] == b' '
                                || bytes[p] == b'\t'
                                || bytes[p] == b'\n'
                                || bytes[p] == b'\r')
                        {
                            p += 1;
                        }
                        if p < bytes.len() && bytes[p] == b')' {
                            edits.push(DiagnosticEdit::new(open_paren + 1, open_paren + 1, "*_"));
                            return edits;
                        } else if p < bytes.len()
                            && bytes[p] == b'_'
                            && (p == 0 || bytes[p - 1] != b'*')
                        {
                            edits.push(DiagnosticEdit::new(p, p, "*"));
                            return edits;
                        } else {
                            edits.push(DiagnosticEdit::new(open_paren + 1, open_paren + 1, "*_, "));
                            return edits;
                        }
                    }
                }
            }
        }

        // 16. Reserved identifier (E0134) -> rename 'err' to 'e', or wrap with @""
        if self.code == "E0134" {
            if let Some(span) = self.span {
                if let Some(sug) = &self.suggestion {
                    if sug.contains("replace 'err' with 'e'") {
                        edits.push(DiagnosticEdit::new(span.start, span.end, "e"));
                        return edits;
                    } else if let Some(escaped) = sug.strip_prefix("replace with ") {
                        edits.push(DiagnosticEdit::new(span.start, span.end, escaped.trim()));
                        return edits;
                    }
                }
            }
        }

        // 17. Prefix '*' on named parameter or misplaced 'move'/'own' after ':' (E0125)
        if self.code == "E0125" {
            if let (Some(span), Some(code)) = (self.span, source_code) {
                let bytes = code.as_bytes();
                if span.start < bytes.len() && bytes[span.start] == b'*' {
                    edits.push(DiagnosticEdit::new(span.start, span.start + 1, ""));
                    let mut p = span.end;
                    while p < bytes.len()
                        && bytes[p] != b':'
                        && bytes[p] != b','
                        && bytes[p] != b')'
                    {
                        p += 1;
                    }
                    if p < bytes.len() && bytes[p] == b':' {
                        p += 1;
                        while p < bytes.len() && (bytes[p] == b' ' || bytes[p] == b'\t') {
                            p += 1;
                        }
                        if p < bytes.len() {
                            edits.push(DiagnosticEdit::new(p, p, "*"));
                        }
                    }
                    return edits;
                }
                // Handle 'param: move T' / 'param: own T' -> 'move param: T' / 'own param: T'
                if span.end <= bytes.len() {
                    let kw = &code[span.start..span.end];
                    if kw == "move" || kw == "own" {
                        let mut p = span.start;
                        while p > 0 && (bytes[p - 1] == b' ' || bytes[p - 1] == b'\t') {
                            p -= 1;
                        }
                        if p > 0 && bytes[p - 1] == b':' {
                            p -= 1;
                            while p > 0 && (bytes[p - 1] == b' ' || bytes[p - 1] == b'\t') {
                                p -= 1;
                            }
                            let name_end = p;
                            while p > 0
                                && (bytes[p - 1].is_ascii_alphanumeric() || bytes[p - 1] == b'_')
                            {
                                p -= 1;
                            }
                            if p < name_end {
                                let del_end = if bytes.get(span.end) == Some(&b' ') {
                                    span.end + 1
                                } else {
                                    span.end
                                };
                                edits.push(DiagnosticEdit::new(p, p, format!("{} ", kw)));
                                edits.push(DiagnosticEdit::new(span.start, del_end, ""));
                                return edits;
                            }
                        }
                    }
                }
            }
        }

        edits
    }
}

/// Annotate cascade errors (`cascade_of: Some(root_index)`) caused by earlier parser recovery.
pub fn mark_cascades(diags: &mut [Diagnostic], source_code: Option<&str>) {
    let mut last_root_syntax: Option<(usize, usize)> = None; // (index, line)
    for i in 0..diags.len() {
        let line = match (source_code, diags[i].span) {
            (Some(code), Some(span)) => find_line_and_col(code, span.start).0 + 1,
            _ => 1,
        };
        let code_str = diags[i].code.as_str();
        let is_generic_recovery = code_str == "E0100" || code_str == "E0111";
        let is_specific_syntax = code_str.starts_with("E01") && !is_generic_recovery;

        if is_specific_syntax {
            last_root_syntax = Some((i, line));
        } else if is_generic_recovery {
            if let Some((root_idx, root_line)) = last_root_syntax {
                if line >= root_line && line.saturating_sub(root_line) <= 60 {
                    diags[i].cascade_of = Some(root_idx);
                } else {
                    last_root_syntax = Some((i, line));
                }
            } else {
                last_root_syntax = Some((i, line));
            }
        } else if code_str == "E0206" {
            if let Some((root_idx, root_line)) = last_root_syntax {
                if line >= root_line && line.saturating_sub(root_line) <= 45 {
                    diags[i].cascade_of = Some(root_idx);
                }
            }
        }
    }
}

/// Build a deduplicated brief report grouping root errors by code and folding cascades.
pub fn build_brief_report(
    diags: &[Diagnostic],
    source_code: Option<&str>,
    file_path: Option<&Path>,
    max_groups: Option<usize>,
) -> BriefDiagnosticReport {
    use std::collections::BTreeMap;

    let mut groups: BTreeMap<String, BriefGroup> = BTreeMap::new();
    let mut total_errors = 0usize;
    let mut root_errors = 0usize;
    let mut cascades_hidden = 0usize;
    let mut fixable = 0usize;

    for diag in diags {
        total_errors += 1;
        let json_d = diag.to_diagnostic_json(source_code, file_path);
        if !json_d.edits.is_empty() {
            fixable += 1;
        }
        if diag.cascade_of.is_some() {
            cascades_hidden += 1;
            continue;
        }
        root_errors += 1;
        let entry = groups
            .entry(diag.code.clone())
            .or_insert_with(|| BriefGroup {
                count: 0,
                message: diag.message.clone(),
                help: diag.help.clone(),
                suggestion: diag.suggestion.clone(),
                lines: Vec::new(),
                fixable_count: 0,
            });
        entry.count += 1;
        if !entry.lines.contains(&json_d.line) {
            entry.lines.push(json_d.line);
        }
        if !json_d.edits.is_empty() {
            entry.fixable_count += 1;
        }
    }

    if let Some(limit) = max_groups {
        let keys_to_keep: Vec<String> = groups.keys().take(limit).cloned().collect();
        groups.retain(|k, _| keys_to_keep.contains(k));
    }

    BriefDiagnosticReport {
        status: if total_errors == 0 {
            "ok".to_string()
        } else {
            "error".to_string()
        },
        errors: total_errors,
        root_errors,
        cascades_hidden,
        fixable,
        groups,
    }
}

/// Render a human/AI compact text summary from a `BriefDiagnosticReport`.
pub fn render_brief_text(report: &BriefDiagnosticReport, file_path: Option<&Path>) -> String {
    let file_display = file_path
        .map(|p| p.display().to_string())
        .unwrap_or_else(|| "<input>".to_string());
    if report.errors == 0 {
        return format!("ok: {} (0 errors)\n", file_display);
    }
    let mut out = format!(
        "{}: {} errors ({} root, {} cascades folded, {} auto-fixable with --fix)\n",
        file_display, report.errors, report.root_errors, report.cascades_hidden, report.fixable
    );
    for (code, grp) in &report.groups {
        let lines_str = grp
            .lines
            .iter()
            .map(|l| l.to_string())
            .collect::<Vec<_>>()
            .join(", ");
        out.push_str(&format!(
            "  [{}] ({}x, lines: {}): {}\n",
            code, grp.count, lines_str, grp.message
        ));
        if let Some(help) = &grp.help {
            out.push_str(&format!("    = help: {}\n", help));
        }
        if let Some(sug) = &grp.suggestion {
            out.push_str(&format!("    = suggestion: {}\n", sug));
        }
    }
    out
}

/// Apply non-overlapping mechanical edits from diagnostics to `source_code`.
/// Returns `(fixed_source, applied_edit_count)`.
pub fn apply_fixes(source_code: &str, diags: &[Diagnostic]) -> (String, usize) {
    let mut all_edits: Vec<DiagnosticEdit> = Vec::new();
    for diag in diags {
        if diag.cascade_of.is_some() {
            continue;
        }
        let edits = diag.generate_edits(Some(source_code));
        all_edits.extend(edits);
    }

    // Sort by span_start ascending, then span_end ascending
    all_edits.sort_by(|a, b| {
        a.span_start
            .cmp(&b.span_start)
            .then(a.span_end.cmp(&b.span_end))
    });

    // Filter out overlapping or out-of-bounds edits
    let mut valid_edits: Vec<DiagnosticEdit> = Vec::new();
    let mut last_end = 0usize;
    for edit in all_edits {
        if edit.span_start > edit.span_end || edit.span_end > source_code.len() {
            continue;
        }
        if !valid_edits.is_empty() && edit.span_start < last_end {
            continue;
        }
        last_end = edit.span_end;
        valid_edits.push(edit);
    }

    let applied = valid_edits.len();
    let mut result = source_code.to_string();
    for edit in valid_edits.into_iter().rev() {
        result.replace_range(edit.span_start..edit.span_end, &edit.replacement);
    }
    (result, applied)
}

pub fn find_condition_range(code: &str, span_start: usize) -> (usize, usize) {
    let start = span_start.min(code.len());
    let rest = &code[start..];

    let mut paren_depth = 0usize;
    let mut bracket_depth = 0usize;
    let mut in_str = None;
    let mut escape = false;
    let mut end_offset = None;

    for (rel_idx, ch) in rest.char_indices() {
        if escape {
            escape = false;
            continue;
        }
        if ch == '\\' {
            escape = true;
            continue;
        }
        if let Some(quote) = in_str {
            if ch == quote {
                in_str = None;
            }
            continue;
        }
        if ch == '"' || ch == '\'' || ch == '`' {
            in_str = Some(ch);
            continue;
        }

        match ch {
            '(' => paren_depth += 1,
            ')' => {
                if paren_depth > 0 {
                    paren_depth -= 1;
                }
            }
            '[' => bracket_depth += 1,
            ']' => {
                if bracket_depth > 0 {
                    bracket_depth -= 1;
                }
            }
            '{' => {
                if paren_depth == 0 && bracket_depth == 0 {
                    end_offset = Some(start + rel_idx);
                    break;
                }
            }
            '\n' | ';' => {
                if paren_depth == 0 && bracket_depth == 0 {
                    end_offset = Some(start + rel_idx);
                    break;
                }
            }
            _ => {}
        }
    }

    let raw_end = end_offset.unwrap_or(code.len());
    let cond_str = &code[start..raw_end];
    let trimmed_len = cond_str.trim_end().len();
    let cond_end = start + trimmed_len;

    (start, cond_end.max(start))
}

pub fn find_line_and_col(code: &str, offset: usize) -> (usize, usize, &str) {
    let offset = offset.min(code.len());
    let before = &code[..offset];
    let line_idx = before.chars().filter(|&c| c == '\n').count();

    let line_start = before.rfind('\n').map(|pos| pos + 1).unwrap_or(0);
    let col_idx = offset - line_start;

    let line_end = code[line_start..]
        .find('\n')
        .map(|pos| line_start + pos)
        .unwrap_or(code.len());

    let line_str = &code[line_start..line_end];
    (line_idx, col_idx, line_str)
}

pub fn levenshtein_distance(a: &str, b: &str) -> usize {
    let len_b = b.chars().count();
    let mut prev = (0..=len_b).collect::<Vec<_>>();
    let mut curr = vec![0; len_b + 1];

    for (i, ca) in a.chars().enumerate() {
        curr[0] = i + 1;
        for (j, cb) in b.chars().enumerate() {
            let cost = if ca == cb { 0 } else { 1 };
            curr[j + 1] =
                std::cmp::min(std::cmp::min(curr[j] + 1, prev[j + 1] + 1), prev[j] + cost);
        }
        prev.copy_from_slice(&curr);
    }
    prev[len_b]
}

pub fn find_closest_match<'a>(
    target: &str,
    candidates: &[&'a str],
    max_dist: usize,
) -> Option<&'a str> {
    let mut best: Option<(&'a str, usize)> = None;
    for &cand in candidates {
        let dist = levenshtein_distance(target, cand);
        if dist <= max_dist {
            if let Some((_, best_dist)) = best {
                if dist < best_dist {
                    best = Some((cand, dist));
                }
            } else {
                best = Some((cand, dist));
            }
        }
    }
    best.map(|(c, _)| c)
}

pub const STANDARD_BUILTIN_IMPORTS: &[(&str, &str)] = &[
    ("print", "cap:io"),
    ("println", "cap:io"),
    ("read_line", "cap:io"),
    ("fs", "cap:fs"),
    ("File", "cap:fs"),
    ("json", "cap:json"),
    ("Json", "cap:json"),
    ("actor", "cap:actor"),
    ("spawn", "cap:actor"),
    ("join", "cap:actor"),
    ("SharedTensor", "cap:mem"),
    ("Socket", "cap:net"),
    ("TcpServer", "cap:net"),
    ("HttpServer", "cap:http"),
    ("HttpRequest", "cap:http"),
    ("HttpResponse", "cap:http"),
];

pub fn from_compiler_error_msg(raw_msg: &str, source_code: Option<&str>) -> Diagnostic {
    let msg = raw_msg.trim_start_matches("CP Compile Error:").trim();

    let mut diag = if msg.contains("Cannot perform mutable operation")
        && msg.contains("read-only parameter")
    {
        let param_name = msg
            .split("read-only parameter '")
            .nth(1)
            .and_then(|s| s.split('\'').next())
            .unwrap_or("param");
        Diagnostic::new(
            "E0201",
            format!("cannot perform mutable operation on read-only parameter '{}'", param_name),
        )
        .with_cause(format!(
            "parameter '{}' is a read-only borrow (immutable by default in CP).",
            param_name
        ))
        .with_help("in CP, function parameters are read-only by default. Use '*_' for mutable method receivers or 'v: *T' for mutable parameters.")
        .with_suggestion(format!(
            "parameter '{}' is a read-only borrow. Add '*' to declare it as mutable: '{}: *<Type>'",
            param_name, param_name
        ))
    } else if msg.contains("read-only method") {
        Diagnostic::new(
            "E0130",
            "cannot perform mutable operation in a read-only method",
        )
        .with_cause("method receiver is read-only by default in CP.")
        .with_help(
            "methods that modify instance fields must declare a mutable receiver parameter '*_'.",
        )
        .with_suggestion("add '*_' as the first parameter of the method: fn method(*_) { ... }")
    } else if msg.contains("Cannot mutate read-only parameter")
        || msg.contains("Cannot mutate read-only variable or captured outer variable")
    {
        let var_name = msg.split('\'').nth(1).unwrap_or("param");
        Diagnostic::new(
            "E0133",
            format!("cannot reassign to read-only parameter '{}'", var_name),
        )
        .with_cause(format!(
            "function parameter '{}' is a read-only borrow (immutable by default in CP).",
            var_name
        ))
        .with_help("in CP, function parameters are immutable by default.")
        .with_suggestion(format!(
            "- 需修改调用者数据，请将形参改为可变借用: '{}: *<Type>'\n- 函数内局部游标，请在首行显式声明: 'let {}_local = {};'",
            var_name, var_name, var_name
        ))
    } else if msg.contains("read-only borrow") {
        let parts: Vec<&str> = msg.split('\'').collect();
        let var_name = if parts.len() >= 4 {
            parts[3]
        } else if parts.len() >= 2 {
            parts[1]
        } else {
            "variable"
        };
        Diagnostic::new(
            "E0201",
            format!(
                "cannot perform mutable operation through read-only borrow '{}'",
                var_name
            ),
        )
        .with_cause(format!(
            "'{}' is borrowed as read-only (immutable).",
            var_name
        ))
        .with_help("in CP, mutable operations require exclusive mutable borrows ('v: *T' or '*_').")
        .with_suggestion(format!(
            "declare '{}' with '*' for mutable borrowing: '{}: *<Type>'",
            var_name, var_name
        ))
    } else if msg.contains("Borrow conflict") {
        let target = msg
            .split("on '")
            .nth(1)
            .and_then(|s| s.split('\'').next())
            .unwrap_or("variable");
        Diagnostic::new("E0202", msg)
            .with_cause(format!("variable '{}' is already borrowed in the current scope.", target))
            .with_help("CP enforces single-writer OR multiple-readers aliasing XOR mutability at compile time.")
            .with_suggestion(format!("ensure existing borrows of '{}' go out of scope before taking a mutable borrow, or clone the value.", target))
    } else if msg.contains("borrowed in a 'for' loop") {
        Diagnostic::new("E0210", msg)
            .with_cause("collection is actively borrowed by an active for-loop iterator.")
            .with_help("mutating a container while iterating over it causes iterator invalidation undefined behavior.")
            .with_suggestion("iterate over a cloned collection or use an index-based 'while' loop")
    } else if msg.contains("Use of moved value") {
        let var_name = msg.split('\'').nth(1).unwrap_or("variable");
        Diagnostic::new("E0203", format!("use of moved value '{}'", var_name))
            .with_cause(format!("value of '{}' was moved and ownership was transferred.", var_name))
            .with_help("once a value is transferred with 'move', the original identifier cannot be accessed.")
            .with_suggestion(format!("avoid moving '{}' if you need it later, or pass by borrow '&{}'", var_name, var_name))
    } else if msg.contains("Cannot move") && msg.contains("borrowed by") {
        Diagnostic::new("E0203", msg)
            .with_cause("attempted to move a value that is currently borrowed.")
            .with_help("values cannot be moved or transferred while active borrows exist.")
            .with_suggestion("ensure all borrows end before moving the value.")
    } else if msg.contains("Cannot return 'null' from function") {
        let fn_name = msg
            .split("from function '")
            .nth(1)
            .and_then(|s| s.split('\'').next())
            .unwrap_or("function");
        if msg.contains("Array(Optional(") {
            Diagnostic::new("E0200", format!("compile error: {}", msg))
                .with_cause(format!(
                    "function '{}' has a slice-of-optionals return type (e.g. '[]const T?' parses as '[]const (?T)'), not an optional slice.",
                    fn_name
                ))
                .with_help("in CP, postfix '?' binds tighter than prefix '[]' / '[]const'. Wrap the slice type in parentheses or use prefix '?' for an optional slice.")
                .with_suggestion("change the return type from '[]const T?' to '([]const T)?' or '?[]const T'")
        } else {
            Diagnostic::new("E0200", format!("compile error: {}", msg))
                .with_cause(format!(
                    "function '{}' does not have an optional return type ('?T' or 'T?').",
                    fn_name
                ))
                .with_help("only functions returning an optional type can return 'null'.")
                .with_suggestion(format!(
                    "change the return type of '{}' to optional (e.g. 'T?' or '?T')",
                    fn_name
                ))
        }
    } else {
        Diagnostic::new("E0200", format!("compile error: {}", msg))
            .with_help("check CP language ownership, borrowing and semantic rules.")
    };

    // If source code is present, try locating the symbol in source code for pinpoint span
    if let Some(code) = source_code {
        if msg.contains("Cannot return 'null' from function") {
            if let Some(fn_name) = msg
                .split("from function '")
                .nth(1)
                .and_then(|s| s.split('\'').next())
            {
                let fn_needle = format!("fn {}(", fn_name);
                if let Some(fn_pos) = code.find(&fn_needle) {
                    if let Some(rel_null) = code[fn_pos..].find("return null") {
                        let start = fn_pos + rel_null;
                        diag.span = Some(crate::Span {
                            start,
                            end: start + "return null".len(),
                        });
                    } else {
                        diag.span = Some(crate::Span {
                            start: fn_pos,
                            end: fn_pos + fn_needle.len(),
                        });
                    }
                }
            }
        }
        if diag.span.is_none() {
            if let Some(target) = msg.split('\'').nth(1).and_then(|s| s.split('\'').next()) {
                if let Some(pos) = code.find(target) {
                    diag.span = Some(crate::Span {
                        start: pos,
                        end: pos + target.len(),
                    });
                }
            }
        }
    }

    diag
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_autofix_e0131_dec_reserved_keyword() {
        let src = "let dec: [16]u8 = undefined;";
        let start = src.find("dec").unwrap();
        let diag = Diagnostic::new("E0131", "'dec' is a reserved decimal type keyword in CP")
            .with_span(Span { start, end: start + 3 });
        let (fixed, count) = apply_fixes(src, &[diag]);
        assert_eq!(count, 1);
        assert_eq!(fixed, "let dec_buf: [16]u8 = undefined;");
    }

    #[test]
    fn test_autofix_e0132_optional_slice_parens() {
        let src = "fn alloc(n: usize): []u8? { return null; }";
        let start = src.find("[]u8?").unwrap();
        let diag = Diagnostic::new("E0132", "optional-element slice mistaken for optional slice")
            .with_span(Span { start, end: start + 5 });
        let (fixed, count) = apply_fixes(src, &[diag]);
        assert_eq!(count, 1);
        assert_eq!(fixed, "fn alloc(n: usize): ([]u8)? { return null; }");
    }

    #[test]
    fn test_autofix_e0204_nearest_const_to_let() {
        let src = "const x = 1;\nfn f() {\n    const x = 2;\n    x = 3;\n}\n";
        let assign_pos = src.find("x = 3").unwrap();
        let diag = Diagnostic::new("E0204", "cannot assign to const variable 'x'")
            .with_span(Span { start: assign_pos, end: assign_pos + 1 });
        let (fixed, count) = apply_fixes(src, &[diag]);
        assert_eq!(count, 1);
        assert!(fixed.contains("const x = 1;\nfn f() {\n    let x = 2;"));
    }
}
