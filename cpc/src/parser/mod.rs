mod container;
mod decl;
mod expr;
mod pattern;
mod primary;
mod stmt;
mod types;

use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;
use std::path::Path;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum ParseError {
    #[error("Diagnostic: {0:?}")]
    Diagnostic(Diagnostic),
    #[error("Unexpected token: {0:?} at {1:?}")]
    UnexpectedToken(Token, Span),
    #[error("Expected token: {0:?}, found {1:?} at {2:?}")]
    ExpectedToken(Token, Token, Span),
    #[error("End of input reached unexpectedly")]
    EndOfInput,
    #[error("Parse error: {0} at {1:?}")]
    Generic(String, Span),
    #[error("Multiple parse errors")]
    Multiple(Vec<Diagnostic>),
}

impl ParseError {
    pub fn to_diagnostic(&self) -> Diagnostic {
        match self {
            ParseError::Multiple(diags) => {
                diags.first().cloned().unwrap_or_else(|| Diagnostic::new("E0100", "syntax error"))
            }
            ParseError::Diagnostic(d) => d.clone(),
            ParseError::UnexpectedToken(tok, span) => match tok {
                Token::Interface => Diagnostic::new("E0102", "CP does not support 'interface' keyword")
                    .with_span(*span)
                    .with_cause("'interface' has been abolished in CP to decouple data layout from behavior.")
                    .with_help("in CP, use 'trait' for behavior contracts and 'class' / 'struct' for data containers.")
                    .with_suggestion("replace 'interface' with 'trait': trait <Name> { ... }"),
                Token::Try => Diagnostic::new("E0103", "traditional try/catch exception block is not supported in CP")
                    .with_span(*span)
                    .with_cause("CP has completely abolished try/catch in favor of zero-cost error unions and '?' operator.")
                    .with_help("use '?' to propagate errors upward, or 'expr catch err => { ... }' / '?? fallback' to handle them.")
                    .with_suggestion("use 'let val = expr ?;' or 'expr catch err => { ... }'"),
                Token::Mut => Diagnostic::new("E0104", "'mut' keyword is not used in CP")
                    .with_span(*span)
                    .with_cause("in CP, variables declared with 'let' are already mutable by default, and 'const' is used for immutable bindings.")
                    .with_help("CP has abolished 'mut'. Use 'let' directly for mutable variables and mutable local parameters ('let param: Type'), and use '*' for mutable borrows ('*param: Type').")
                    .with_suggestion("remove 'mut': use 'let ...' instead"),
                Token::Any => Diagnostic::new("E0105", "'any' type is not supported in CP")
                    .with_span(*span)
                    .with_cause("untyped 'any' is prohibited to ensure strict type safety and zero-cost runtime performance.")
                    .with_help("use a concrete strong type (e.g. i32, string), a generic type parameter '<T>', or '*anyopaque' for C pointers.")
                    .with_suggestion("replace 'any' with a concrete type or generic '<T>'"),
                _ => Diagnostic::new("E0100", format!("unexpected token '{:?}'", tok))
                    .with_span(*span)
                    .with_cause(format!("parser encountered unexpected token '{:?}'", tok))
                    .with_help("check the syntax around this token."),
            },
            ParseError::ExpectedToken(expected, found, span) => {
                if *expected == Token::LParen {
                    Diagnostic::new("E0101", "control flow statement must be enclosed in parentheses")
                        .with_span(*span)
                        .with_cause("in CP, control flow statements ('if', 'while', 'for') must be enclosed in parentheses to eliminate ambiguity with struct literal syntax and maintain syntax consistency.")
                        .with_help("headers in 'if', 'while' and 'for' must be enclosed in parentheses: if (condition) { ... }, while (condition) { ... }, for (item of collection) { ... }")
                        .with_suggestion("wrap the control flow expression in parentheses")
                } else {
                    Diagnostic::new("E0111", format!("expected token '{:?}', found '{:?}'", expected, found))
                        .with_span(*span)
                        .with_cause(format!("syntax requires '{:?}' at this position, but found '{:?}'", expected, found))
                        .with_help(format!("ensure that '{:?}' is provided.", expected))
                        .with_suggestion(format!("insert '{:?}' before '{:?}'", expected, found))
                }
            }
            ParseError::EndOfInput => Diagnostic::new("E0199", "end of input reached unexpectedly")
                .with_cause("source code terminated prematurely, likely missing a closing bracket or delimiter.")
                .with_help("check for unclosed braces '{', parentheses '(', or quotes."),
            ParseError::Generic(msg, span) => {
                if msg.contains("= cause:") || msg.contains("= suggestion:") || msg.contains("= help:") {
                    let first_line = msg.lines().next().unwrap_or(msg).to_string();
                    let mut diag = Diagnostic::new("E0101", first_line).with_span(*span);
                    for line in msg.lines() {
                        let trimmed = line.trim();
                        if let Some(rest) = trimmed.strip_prefix("= cause:") {
                            diag.cause = Some(rest.trim().to_string());
                        } else if let Some(rest) = trimmed.strip_prefix("= help:") {
                            diag.help = Some(rest.trim().to_string());
                        } else if let Some(rest) = trimmed.strip_prefix("= suggestion:") {
                            diag.suggestion = Some(rest.trim().to_string());
                        }
                    }
                    diag
                } else if msg.contains("Expected 'of' in for loop") {
                    Diagnostic::new("E0106", "invalid for loop syntax")
                        .with_span(*span)
                        .with_cause("CP does not use C-style three-clause 'for (;;)' loops.")
                        .with_help("CP uses 'for elem of collection' or 'while (cond)' for indexed loops.")
                        .with_suggestion("use 'for elem of list' or 'while (i < len)'")
                } else {
                    Diagnostic::new("E0190", format!("syntax error: {}", msg))
                        .with_span(*span)
                        .with_cause(msg.clone())
                        .with_help("check syntax matching CP language grammar rules.")
                }
            }
        }
    }

    pub fn to_diagnostics(&self) -> Vec<Diagnostic> {
        match self {
            ParseError::Multiple(diags) => diags.clone(),
            other => vec![other.to_diagnostic()],
        }
    }

    pub fn render_all(&self, source: Option<&str>, path: Option<&Path>) -> String {
        let diags = self.to_diagnostics();
        let mut out = String::new();
        for (i, d) in diags.iter().enumerate() {
            if i > 0 {
                out.push('\n');
            }
            out.push_str(&d.render(source, path));
        }
        out
    }

    pub fn to_json_report(&self, source: Option<&str>, path: Option<&Path>) -> String {
        let diags = self.to_diagnostics();
        let json_diags: Vec<_> = diags
            .iter()
            .map(|d| d.to_diagnostic_json(source, path))
            .collect();
        let errors = json_diags.len();
        let root_errors = json_diags.iter().filter(|d| d.cascade_of.is_none()).count();
        let report = crate::diagnostic::DiagnosticReport {
            status: "error".to_string(),
            errors,
            root_errors,
            diagnostics: json_diags,
        };
        serde_json::to_string(&report).unwrap_or_else(|_| "{}".to_string())
    }
}

pub type ParseResult<T> = Result<T, ParseError>;

pub struct Parser<'a> {
    lexer: Lexer<'a>,
    input: &'a str,
    peeked: Option<(Token, Span)>,
    peeked2: Option<(Token, Span)>,
    prev_token_end: usize,
    in_match_guard: bool,
    in_pipeline_rhs: usize,
    in_switch_arm: usize,
    in_paren_pattern: usize,
    pub errors: Vec<Diagnostic>,
}

impl<'a> Parser<'a> {
    pub fn new(input: &'a str) -> Self {
        Self {
            lexer: Lexer::new(input),
            input,
            peeked: None,
            peeked2: None,
            prev_token_end: 0,
            in_match_guard: false,
            in_pipeline_rhs: 0,
            in_switch_arm: 0,
            in_paren_pattern: 0,
            errors: Vec::new(),
        }
    }

    pub(super) fn bump(&mut self) -> ParseResult<(Token, Span)> {
        let (token, span) = if let Some(p) = self.peeked.take() {
            self.peeked = self.peeked2.take();
            p
        } else {
            let (token, range) = self.lexer.next().ok_or(ParseError::EndOfInput)?;
            (
                token,
                Span {
                    start: range.start,
                    end: range.end,
                },
            )
        };
        self.prev_token_end = span.end;
        Ok((token, span))
    }

    pub(super) fn peek(&mut self) -> Option<&Token> {
        if self.peeked.is_none() {
            let (token, range) = self.lexer.next()?;
            self.peeked = Some((
                token,
                Span {
                    start: range.start,
                    end: range.end,
                },
            ));
        }
        self.peeked.as_ref().map(|(t, _)| t)
    }

    pub(super) fn peek_next(&mut self) -> Option<&Token> {
        let _ = self.peek();
        if self.peeked2.is_none() {
            if let Some((token, range)) = self.lexer.next() {
                self.peeked2 = Some((
                    token,
                    Span {
                        start: range.start,
                        end: range.end,
                    },
                ));
            }
        }
        self.peeked2.as_ref().map(|(t, _)| t)
    }

    pub(super) fn peek_span(&mut self) -> Span {
        if self.peeked.is_none() {
            if let Some((token, range)) = self.lexer.next() {
                self.peeked = Some((
                    token,
                    Span {
                        start: range.start,
                        end: range.end,
                    },
                ));
            } else {
                return Span { start: 0, end: 0 };
            }
        }
        self.peeked
            .as_ref()
            .map(|(_, s)| *s)
            .unwrap_or(Span { start: 0, end: 0 })
    }

    pub(super) fn expect(&mut self, expected: Token) -> ParseResult<Span> {
        let (token, span) = self.bump()?;
        if token == expected {
            Ok(span)
        } else {
            Err(ParseError::ExpectedToken(expected, token, span))
        }
    }

    pub fn push_error(&mut self, diag: Diagnostic) {
        if let Some(last) = self.errors.last() {
            if last.span == diag.span && last.code == diag.code {
                return;
            }
        }
        if self.errors.len() < 200 {
            self.errors.push(diag);
        }
    }

    pub fn recover_stmt(&mut self) {
        let mut brace_depth: usize = 0;
        let mut iterations = 0;
        while let Some(tok) = self.peek() {
            iterations += 1;
            if iterations > 1000 {
                let _ = self.bump();
                return;
            }
            match tok {
                Token::LBrace => {
                    brace_depth += 1;
                    let _ = self.bump();
                }
                Token::RBrace => {
                    if brace_depth > 0 {
                        brace_depth -= 1;
                        let _ = self.bump();
                        if brace_depth == 0 {
                            return;
                        }
                    } else {
                        // Reached enclosing block's right brace
                        return;
                    }
                }
                Token::Semicolon => {
                    if brace_depth == 0 {
                        let _ = self.bump();
                        return;
                    } else {
                        let _ = self.bump();
                    }
                }
                Token::Let
                | Token::Const
                | Token::If
                | Token::While
                | Token::For
                | Token::Return
                | Token::Switch
                | Token::Break
                | Token::Continue
                | Token::Defer
                | Token::ErrDefer
                | Token::Unsafe
                    if brace_depth == 0 =>
                {
                    return;
                }
                _ => {
                    let _ = self.bump();
                }
            }
        }
    }

    pub fn recover_module_item(&mut self) {
        let mut brace_depth: usize = 0;
        let mut iterations = 0;
        while let Some(tok) = self.peek() {
            iterations += 1;
            if iterations > 1000 {
                let _ = self.bump();
                return;
            }
            match tok {
                Token::LBrace => {
                    brace_depth += 1;
                    let _ = self.bump();
                }
                Token::RBrace => {
                    if brace_depth > 0 {
                        brace_depth -= 1;
                        let _ = self.bump();
                        if brace_depth == 0 {
                            return;
                        }
                    } else {
                        let _ = self.bump();
                        return;
                    }
                }
                Token::Semicolon if brace_depth == 0 => {
                    let _ = self.bump();
                    return;
                }
                Token::Export
                | Token::Private
                | Token::Import
                | Token::Class
                | Token::Struct
                | Token::Union
                | Token::Enum
                | Token::Fn
                | Token::Async
                | Token::TypeKw
                | Token::Const
                | Token::Let
                | Token::Var
                | Token::Trait
                | Token::Impl
                | Token::Interface
                | Token::At
                    if brace_depth == 0 =>
                {
                    return;
                }
                _ => {
                    let _ = self.bump();
                }
            }
        }
    }

    pub fn parse_module_with_recovery(&mut self) -> (Module, Vec<Diagnostic>) {
        let mut body = Vec::new();
        let start = self.peek_span().start;
        while self.peek().is_some() {
            let start_pos = self.peek_span().start;
            match self.parse_module_item() {
                Ok(items) => body.extend(items),
                Err(err) => {
                    self.push_error(err.to_diagnostic());
                    self.recover_module_item();
                    if let Some(_) = self.peek() {
                        if self.peek_span().start == start_pos {
                            let _ = self.bump();
                        }
                    }
                }
            }
        }
        let end = self.peek_span().end;
        (
            Module {
                span: Span { start, end },
                body,
            },
            self.errors.clone(),
        )
    }

    pub fn parse_module(&mut self) -> ParseResult<Module> {
        let (module, errors) = self.parse_module_with_recovery();
        if !errors.is_empty() {
            return Err(ParseError::Multiple(errors));
        }
        Ok(module)
    }

    pub(super) fn clone_lookahead(&self) -> Parser<'a> {
        Parser {
            lexer: self.lexer.clone(),
            input: self.input,
            peeked: self.peeked.clone(),
            peeked2: self.peeked2.clone(),
            prev_token_end: self.prev_token_end,
            in_match_guard: self.in_match_guard,
            in_pipeline_rhs: self.in_pipeline_rhs,
            in_switch_arm: self.in_switch_arm,
            in_paren_pattern: self.in_paren_pattern,
            errors: Vec::new(),
        }
    }

    pub(super) fn is_decl_keyword(t: &Token) -> bool {
        matches!(
            t,
            Token::Export
                | Token::Const
                | Token::Let
                | Token::Var
                | Token::ThreadLocal
                | Token::Fn
                | Token::Async
                | Token::Static
                | Token::Unsafe
                | Token::Class
                | Token::Struct
                | Token::Union
                | Token::Ref
                | Token::Meta
                | Token::Dec
                | Token::Macro
                | Token::Trait
                | Token::Impl
                | Token::Enum
                | Token::Interface
                | Token::ErrorKw
                | Token::TypeKw
        ) || matches!(t, Token::Ident(id) if id == "extern" || id == "inline" || id == "noinline")
    }

}

pub(super) fn left_span(expr: &Expr) -> Span {
    match expr {
        Expr::Ident(i) => i.span,
        Expr::Lit(_) => Span { start: 0, end: 0 },
        Expr::Bin(b) => b.span,
        Expr::Call(c) | Expr::OptCall(c) | Expr::New(c) => c.span,
        Expr::Member(m) | Expr::OptMember(m) => m.span,
        Expr::Slice(s) => s.span,
        Expr::Match(m) => m.span,
        Expr::Spawn(s) => s.span,
        Expr::Receive(_) => Span { start: 0, end: 0 },
        Expr::Zig(z) => z.span,
        Expr::Question(e) | Expr::ErrWrap(e) | Expr::BoxNew(e) | Expr::Unwrap(e) => left_span(e),
        Expr::QuestionFallback { expr, .. } | Expr::Catch { expr, .. } => left_span(expr),
        Expr::NullCoalesce { left, .. } => left_span(left),
        Expr::Downcast { obj, .. } => left_span(obj),
        Expr::Array(_) => Span { start: 0, end: 0 },
        Expr::ArrayRepeat(val, _) => left_span(val),
        Expr::Object(_) | Expr::Dict(_) => Span { start: 0, end: 0 },
        Expr::Index { obj, .. } => left_span(obj),
        Expr::Arrow(a) => a.span,
        Expr::EnumInit(e) => e.span,
        Expr::Template(t) => t.span,
        Expr::Await(e) => left_span(e),
        Expr::Move(e) => left_span(e),
        Expr::Borrow(e, _) => left_span(e),
        Expr::Builtin(b) => b.span,
        Expr::ReflectType { span, .. }
        | Expr::ReflectOf { span, .. }
        | Expr::TypeVal { span, .. } => *span,
        Expr::PipePlaceholder(span) => *span,
        Expr::Range { span, .. } => *span,
        Expr::Cast { span, .. } => *span,
        Expr::Deref(d) => left_span(d),
        Expr::BitNot(d) => left_span(d),
        Expr::Unsafe(b) | Expr::Comptime(b) | Expr::Arena(b) => b.span,
        Expr::ComptimeExpr(e) => left_span(e),
        Expr::If { span, .. } => *span,
        Expr::Continue(span) | Expr::LabeledContinue(span, _) => *span,
        Expr::Break(span, _) | Expr::LabeledBreak(span, _, _) => *span,
        Expr::LabeledBlock { block, .. } => block.span,
        Expr::Return(span, _) => *span,
        Expr::InlineAsm { span, .. } | Expr::ComptimeMap { span, .. } => *span,
    }
}

pub(super) fn right_span(expr: &Expr) -> Span {
    match expr {
        Expr::Ident(i) => i.span,
        Expr::Lit(_) => Span { start: 0, end: 0 },
        Expr::Bin(b) => right_span(&b.right),
        Expr::Call(c) | Expr::OptCall(c) | Expr::New(c) => Span {
            start: 0,
            end: c.span.end,
        },
        Expr::Member(m) | Expr::OptMember(m) => m.span,
        Expr::Slice(s) => s.span,
        Expr::Match(m) => m.span,
        Expr::Spawn(s) => s.span,
        Expr::Receive(_) => Span { start: 0, end: 0 },
        Expr::Zig(z) => z.span,
        Expr::Question(e) | Expr::ErrWrap(e) | Expr::BoxNew(e) | Expr::Unwrap(e) => right_span(e),
        Expr::QuestionFallback { fallback, .. } => right_span(fallback),
        Expr::NullCoalesce { right, .. } => right_span(right),
        Expr::Catch { expr, .. } => right_span(expr),
        Expr::Downcast { target_ty, .. } => target_ty.span,
        Expr::Array(_) => Span { start: 0, end: 0 },
        Expr::ArrayRepeat(_, count) => right_span(count),
        Expr::Object(_) | Expr::Dict(_) => Span { start: 0, end: 0 },
        Expr::Index { index, .. } => right_span(index),
        Expr::Arrow(a) => a.span,
        Expr::EnumInit(e) => e.span,
        Expr::Template(t) => t.span,
        Expr::Await(e) => right_span(e),
        Expr::Move(e) => right_span(e),
        Expr::Borrow(e, _) => right_span(e),
        Expr::Builtin(b) => b.span,
        Expr::ReflectType { span, .. }
        | Expr::ReflectOf { span, .. }
        | Expr::TypeVal { span, .. } => *span,
        Expr::PipePlaceholder(span) => *span,
        Expr::Range { span, .. } => *span,
        Expr::Cast { span, .. } => *span,
        Expr::Deref(d) => right_span(d),
        Expr::BitNot(d) => right_span(d),
        Expr::Unsafe(b) | Expr::Comptime(b) | Expr::Arena(b) => b.span,
        Expr::ComptimeExpr(e) => right_span(e),
        Expr::If { span, .. } => *span,
        Expr::Continue(span) | Expr::LabeledContinue(span, _) => *span,
        Expr::Break(span, val) | Expr::LabeledBreak(span, _, val) => {
            val.as_ref().map(|v| right_span(v)).unwrap_or(*span)
        }
        Expr::LabeledBlock { block, .. } => block.span,
        Expr::Return(span, val) => val.as_ref().map(|v| right_span(v)).unwrap_or(*span),
        Expr::InlineAsm { span, .. } | Expr::ComptimeMap { span, .. } => *span,
    }
}

pub fn expr_to_member_str(expr: &Expr) -> Option<String> {
    match expr {
        Expr::Ident(id) => Some(id.sym.clone()),
        Expr::Member(m) => {
            let base = expr_to_member_str(&m.obj)?;
            Some(format!("{}.{}", base, m.prop.sym))
        }
        _ => None,
    }
}
