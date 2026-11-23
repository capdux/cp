use crate::diagnostic::Diagnostic;
use crate::lexer::Token;
use crate::Span;
use logos::Logos;

/// Strict CP purification gate (`cpc check --strict`).
/// Rejects legacy Zig/TS syntax backflow across the entire source file
/// (including inside `return struct/union/enum` factory bodies):
/// - `orelse` -> `??`
/// - `and` / `or` -> `&&` / `||`
/// - prefix `try expr` -> postfix `expr?`
/// - prefix `?T` -> postfix `T?`
/// - redundant `: void` -> omit return type
/// - `self` / `this` -> `_`
/// - `|capture|` -> `if (let v = ...)` / `for (v in ...)` / `catch (err)`
/// - `zig { ... }` -> pure CP syntax
/// - `var` -> `let` / `const`
pub fn collect_strict_diagnostics(code: &str, existing: &[Diagnostic]) -> Vec<Diagnostic> {
    let mut tokens: Vec<(Token, Span)> = Vec::new();
    let mut lexer = Token::lexer(code);
    while let Some(res) = lexer.next() {
        if let Ok(tok) = res {
            let r = lexer.span();
            tokens.push((
                tok,
                Span {
                    start: r.start,
                    end: r.end,
                },
            ));
        }
    }

    let has_overlapping_diag = |span: Span| -> bool {
        existing.iter().any(|d| {
            d.span
                .map_or(false, |s| s.start < span.end && span.start < s.end)
        })
    };

    let mut diags = Vec::new();
    let len = tokens.len();
    let mut i = 0;

    while i < len {
        let (tok, span) = &tokens[i];
        let prev = if i > 0 { Some(&tokens[i - 1].0) } else { None };
        let next = if i + 1 < len {
            Some(&tokens[i + 1].0)
        } else {
            None
        };

        let is_member_access = matches!(prev, Some(Token::Dot | Token::QuestionDot));
        let is_decl_name = matches!(
            prev,
            Some(
                Token::Fn | Token::Const | Token::Let | Token::Struct | Token::Enum | Token::Union
            )
        ) || matches!(next, Some(Token::Colon));

        match tok {
            Token::Ident(id) if id == "orelse" && !is_member_access && !is_decl_name => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0301",
                            "legacy Zig operator 'orelse' is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses '??' instead of Zig's 'orelse' for optional fallback.")
                        .with_help("replace 'orelse' with '??'.")
                        .with_suggestion("const y = x ?? 0;"),
                    );
                }
            }
            Token::Ident(id) if id == "and" && !is_member_access && !is_decl_name => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0302",
                            "legacy Zig operator 'and' is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses '&&' instead of Zig's 'and' for logical conjunction.")
                        .with_help("replace 'and' with '&&'.")
                        .with_suggestion("if (a && b) { ... }"),
                    );
                }
            }
            Token::Ident(id) if id == "or" && !is_member_access && !is_decl_name => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0303",
                            "legacy Zig operator 'or' is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses '||' instead of Zig's 'or' for logical disjunction.")
                        .with_help("replace 'or' with '||'.")
                        .with_suggestion("if (a || b) { ... }"),
                    );
                }
            }
            Token::Try if !matches!(next, Some(Token::LBrace)) => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0304",
                            "prefix 'try' is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses postfix '?' error propagation instead of Zig prefix 'try'.")
                        .with_help("move error propagation to postfix '?'.")
                        .with_suggestion("const v = fallibleCall()?;"),
                    );
                }
            }
            Token::Ident(id) if id == "self" && !is_member_access && !is_decl_name => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0305",
                            "legacy receiver 'self' is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses '_' as the unified implicit instance receiver.")
                        .with_help("omit explicit 'self' parameter and use '_' inside methods.")
                        .with_suggestion("return _.field;"),
                    );
                }
            }
            Token::Ident(id) if id == "this" && !is_member_access && !is_decl_name => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0306",
                            "legacy receiver 'this' is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses '_' as the unified implicit instance receiver.")
                        .with_help("omit explicit 'this' parameter and use '_' inside methods.")
                        .with_suggestion("return _.field;"),
                    );
                }
            }
            Token::Pipe => {
                // Detect Zig-style payload capture: `) |ident|`, `) |*ident|`, `catch |err|`, `else |err|`
                let after_capture_trigger =
                    matches!(prev, Some(Token::RParen | Token::Catch | Token::Else));
                if after_capture_trigger {
                    let mut j = i + 1;
                    let mut valid_capture = false;
                    while j < len && j <= i + 8 {
                        match &tokens[j].0 {
                            Token::Pipe => {
                                if j > i + 1 {
                                    valid_capture = true;
                                }
                                break;
                            }
                            Token::Ident(_) | Token::Star | Token::Comma => {
                                j += 1;
                            }
                            _ => break,
                        }
                    }
                    if valid_capture {
                        let cap_span = Span {
                            start: span.start,
                            end: tokens[j].1.end,
                        };
                        if !has_overlapping_diag(cap_span) {
                            diags.push(
                                Diagnostic::new(
                                    "E0307",
                                    "legacy Zig payload capture '|...|' is prohibited in --strict mode",
                                )
                                .with_span(cap_span)
                                .with_cause("CP uses 'if (let v = opt)', 'for (v in slice)', and 'catch (err)' instead of '|capture|'.")
                                .with_help("rewrite '|v|' capture to modern CP binding syntax.")
                                .with_suggestion("if (let v = opt) { ... }"),
                            );
                        }
                        i = j + 1;
                        continue;
                    }
                }
            }
            Token::ZigEscape if !is_member_access && matches!(next, Some(Token::LBrace)) => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0308",
                            "raw 'zig { ... }' escape block is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("purified CP modules must not contain raw 'zig { ... }' escape hatches.")
                        .with_help("rewrite the block using pure CP syntax."),
                    );
                }
            }
            Token::Var => {
                if !has_overlapping_diag(*span) {
                    diags.push(
                        Diagnostic::new(
                            "E0309",
                            "legacy 'var' declaration is prohibited in --strict mode",
                        )
                        .with_span(*span)
                        .with_cause("CP uses 'let' for mutable variables and 'const' for immutable bindings.")
                        .with_help("replace 'var' with 'let'.")
                        .with_suggestion("let x = 0;"),
                    );
                }
            }
            Token::Question => {
                // Detect prefix '?T' (catches cases inside `return struct { ... }` skipped by outer parser)
                let after_type_intro = matches!(
                    prev,
                    Some(
                        Token::Colon
                            | Token::Assign
                            | Token::Comma
                            | Token::LParen
                            | Token::LBracket
                            | Token::Arrow
                            | Token::Return
                    )
                );
                let before_type_start = matches!(
                    next,
                    Some(
                        Token::Ident(_)
                            | Token::Star
                            | Token::LBracket
                            | Token::I8
                            | Token::U8
                            | Token::I16
                            | Token::U16
                            | Token::I32
                            | Token::U32
                            | Token::I64
                            | Token::U64
                            | Token::I128
                            | Token::U128
                            | Token::ISize
                            | Token::USize
                            | Token::F32
                            | Token::F64
                            | Token::Bool
                            | Token::String
                            | Token::CStr
                            | Token::Void
                            | Token::Any
                            | Token::Const
                    )
                );
                if after_type_intro && before_type_start {
                    let q_span = Span {
                        start: span.start,
                        end: tokens[i + 1].1.end,
                    };
                    if !has_overlapping_diag(q_span) {
                        diags.push(
                            Diagnostic::new(
                                "E0310",
                                "prefix optional type '?T' is prohibited in --strict mode",
                            )
                            .with_span(q_span)
                            .with_cause(
                                "CP uses postfix optional syntax 'T?' instead of Zig prefix '?T'.",
                            )
                            .with_help("place '?' at the end of the type (e.g. 'i32?' or '(*T)?').")
                            .with_suggestion("replace '?T' with 'T?'"),
                        );
                    }
                }
            }
            Token::Void => {
                // Detect redundant `: void` function return type annotations (e.g. `fn foo(): void { ... }`).
                // Do NOT flag union/struct zero-sized fields (`none: void,`) or anonymous fn types (`fn(): void`).
                let prev_prev = if i >= 2 { Some(&tokens[i - 2].0) } else { None };
                let is_fn_return_void = matches!(prev, Some(Token::Colon))
                    && matches!(prev_prev, Some(Token::RParen))
                    && (matches!(next, Some(Token::LBrace))
                        || (matches!(next, Some(Token::Semicolon)) && {
                            let mut depth = 0usize;
                            let mut j = i - 2;
                            let mut is_named_fn = false;
                            loop {
                                match &tokens[j].0 {
                                    Token::RParen => depth += 1,
                                    Token::LParen => {
                                        depth = depth.saturating_sub(1);
                                        if depth == 0 {
                                            is_named_fn =
                                                j > 0 && !matches!(tokens[j - 1].0, Token::Fn);
                                            break;
                                        }
                                    }
                                    _ => {}
                                }
                                if j == 0 {
                                    break;
                                }
                                j -= 1;
                            }
                            is_named_fn
                        }));
                if is_fn_return_void {
                    let v_span = Span {
                        start: tokens[i - 1].1.start,
                        end: span.end,
                    };
                    if !has_overlapping_diag(v_span) {
                        diags.push(
                            Diagnostic::new(
                                "E0311",
                                "redundant ': void' return type is prohibited in --strict mode",
                            )
                            .with_span(v_span)
                            .with_cause("in CP, void functions omit ': void'.")
                            .with_help("remove ': void' from the function signature.")
                            .with_suggestion("fn foo() { ... }"),
                        );
                    }
                }
            }
            _ => {}
        }

        i += 1;
    }

    diags
}
