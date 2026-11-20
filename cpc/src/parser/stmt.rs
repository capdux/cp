use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

impl<'a> Parser<'a> {
    pub(super) fn parse_stmt(&mut self) -> ParseResult<Stmt> {
        if matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime" || id == "inline") {
            let is_inline = matches!(self.peek(), Some(Token::Ident(id)) if id == "inline");
            let mut clone = self.clone_lookahead();
            let _ = clone.bump();
            if clone.peek() == Some(&Token::LBrace) {
                self.bump()?; // consume 'comptime'
                return Ok(Stmt::ComptimeBlock(self.parse_block_stmt()?));
            } else if clone.peek() == Some(&Token::For)
                || matches!(clone.peek(), Some(Token::Ident(id)) if id == "for")
            {
                self.bump()?; // consume 'comptime' or 'inline'
                return Ok(Stmt::For(self.parse_for_stmt_with_comptime(true)?));
            } else if clone.peek() == Some(&Token::While) {
                self.bump()?; // consume 'comptime' or 'inline'
                let mut w = self.parse_while_stmt()?;
                w.is_inline = is_inline;
                return Ok(Stmt::While(w));
            } else if matches!(
                clone.peek(),
                Some(Token::Var) | Some(Token::Let) | Some(Token::Const)
            ) {
                return Ok(Stmt::Var(self.parse_var_decl()?));
            }
        }
        if matches!(self.peek(), Some(Token::Ident(id)) if id == "extern")
            && matches!(
                self.peek_next(),
                Some(Token::Var) | Some(Token::Let) | Some(Token::Const) | Some(Token::ThreadLocal)
            )
        {
            self.bump()?; // consume 'extern'
            let mut v = self.parse_var_decl()?;
            v.is_extern = true;
            v.is_c = true;
            return Ok(Stmt::Var(v));
        }
        if matches!(self.peek(), Some(Token::Ident(_))) && self.peek_next() == Some(&Token::Colon) {
            let mut clone = self.clone_lookahead();
            if let Ok(label) = clone.parse_ident() {
                if clone.peek() == Some(&Token::Colon) {
                    let _ = clone.bump(); // consume ':'
                    if clone.peek() == Some(&Token::While) {
                        self.bump()?; // consume ident
                        self.bump()?; // consume ':'
                        let mut w = self.parse_while_stmt()?;
                        w.label = Some(label);
                        return Ok(Stmt::While(w));
                    } else if clone.peek() == Some(&Token::For)
                        || matches!(clone.peek(), Some(Token::Ident(id)) if id == "for")
                    {
                        self.bump()?; // consume ident
                        self.bump()?; // consume ':'
                        let mut f = self.parse_for_stmt()?;
                        f.label = Some(label);
                        return Ok(Stmt::For(f));
                    } else if clone.peek() == Some(&Token::LBrace) {
                        self.bump()?; // consume ident
                        self.bump()?; // consume ':'
                        let block = self.parse_block_stmt()?;
                        return Ok(Stmt::Expr(Expr::LabeledBlock {
                            label: Some(label),
                            block,
                        }));
                    }
                }
            }
        }
        if self.peek() == Some(&Token::Star) && self.peek_next() == Some(&Token::Assign) {
            let (_, star_span) = self.bump()?; // consume '*'
            self.bump()?; // consume '='
            let expr = self.parse_expr()?;
            if self.peek() == Some(&Token::Semicolon) {
                self.bump()?;
            }
            let end = self.prev_token_end.max(star_span.end);
            return Ok(Stmt::Var(VarDecl {
                span: Span {
                    start: star_span.start,
                    end,
                },
                kind: VarDeclKind::Let,
                pat: Pattern::Wildcard,
                ty: None,
                init: Some(expr),
                is_threadlocal: false,
                is_pub: false,
                is_export: false,
                is_comptime: false,
                is_extern: false,
                is_c: false,
            }));
        }
        match self.peek() {
            Some(Token::Fn) => {
                let func = self.parse_function()?;
                Ok(Stmt::Func(Box::new(func)))
            }
            Some(Token::Let) | Some(Token::Const) | Some(Token::Var) | Some(Token::ThreadLocal) => {
                Ok(Stmt::Var(self.parse_var_decl()?))
            }
            Some(Token::While) => Ok(Stmt::While(self.parse_while_stmt()?)),
            Some(Token::For) => Ok(Stmt::For(self.parse_for_stmt()?)),
            Some(Token::If) => Ok(Stmt::If(self.parse_if_stmt()?)),
            Some(Token::Return) => Ok(Stmt::Return(self.parse_return_stmt()?)),
            Some(Token::Break) => {
                let start = self.bump()?.1.start;
                // Check if followed by colon for label: break :label [value]
                if self.peek() == Some(&Token::Colon) {
                    self.bump()?; // consume ':'
                    let label = self.parse_ident()?;
                    let val = if self.peek() != Some(&Token::Semicolon)
                        && self.peek() != Some(&Token::RBrace)
                        && self.peek().is_some()
                    {
                        Some(Box::new(self.parse_expr()?))
                    } else {
                        None
                    };
                    if self.peek() == Some(&Token::Semicolon) {
                        self.bump()?;
                    }
                    let end = self.peek_span().start;
                    return Ok(Stmt::Expr(Expr::LabeledBreak(
                        Span { start, end },
                        label,
                        val,
                    )));
                }
                // Check if break has a value (e.g. break value;)
                if self.peek() != Some(&Token::Semicolon)
                    && self.peek() != Some(&Token::RBrace)
                    && self.peek().is_some()
                {
                    let val = self.parse_expr()?;
                    if self.peek() == Some(&Token::Semicolon) {
                        self.bump()?;
                    }
                    let end = self.peek_span().start;
                    return Ok(Stmt::Expr(Expr::Break(
                        Span { start, end },
                        Some(Box::new(val)),
                    )));
                }
                if self.peek() == Some(&Token::Semicolon) {
                    self.bump()?;
                }
                Ok(Stmt::Break)
            }
            Some(Token::Continue) => {
                let start = self.bump()?.1.start;
                if self.peek() == Some(&Token::Colon) {
                    self.bump()?; // consume ':'
                    let label = self.parse_ident()?;
                    if self.peek() == Some(&Token::Semicolon) {
                        self.bump()?;
                    }
                    let end = self.peek_span().start;
                    return Ok(Stmt::Expr(Expr::LabeledContinue(
                        Span { start, end },
                        label,
                    )));
                }
                if self.peek() == Some(&Token::Semicolon) {
                    self.bump()?;
                }
                Ok(Stmt::Continue)
            }
            Some(Token::Try) => {
                if self.peek_next() != Some(&Token::LBrace) {
                    let expr = self.parse_expr()?;
                    if self.peek() == Some(&Token::Semicolon) {
                        self.bump()?;
                    }
                    return Ok(Stmt::Expr(expr));
                }
                let start = self.expect(Token::Try)?.start;
                let try_block = self.parse_block_stmt()?;
                let mut err_ident = None;
                let mut is_err_mut = false;
                let mut catch_body = None;
                if self.peek() == Some(&Token::Catch) {
                    self.bump()?;
                    if self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        if self.peek() == Some(&Token::Mut) {
                            self.bump()?;
                            is_err_mut = true;
                        }
                        err_ident = Some(self.parse_ident()?);
                        self.expect(Token::RParen)?;
                    } else if self.peek() == Some(&Token::Pipe) {
                        self.bump()?;
                        if self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut) {
                            self.bump()?;
                            is_err_mut = true;
                        }
                        err_ident = Some(self.parse_ident()?);
                        self.expect(Token::Pipe)?;
                    } else if self.peek() != Some(&Token::LBrace) {
                        let checkpoint = self.lexer.clone();
                        let p1 = self.peeked.clone();
                        let p2 = self.peeked2.clone();
                        let mut is_binding = false;
                        if self.peek() == Some(&Token::Mut) {
                            self.bump()?;
                            is_err_mut = true;
                            is_binding = true;
                        }
                        if matches!(
                            self.peek(),
                            Some(Token::Ident(_)) | Some(Token::ErrorKw) | Some(Token::Auto)
                        ) {
                            let id = self.parse_ident()?;
                            if self.peek() == Some(&Token::FatArrow) {
                                self.bump()?;
                                err_ident = Some(id);
                                is_binding = true;
                            } else if is_err_mut {
                                err_ident = Some(id);
                            }
                        }
                        if !is_binding {
                            self.lexer = checkpoint;
                            self.peeked = p1;
                            self.peeked2 = p2;
                            is_err_mut = false;
                        }
                    }
                    let body = self.parse_stmt()?;
                    catch_body = Some(Box::new(body));
                }
                Ok(Stmt::TryCatch {
                    span: Span {
                        start,
                        end: self.peek_span().end,
                    },
                    try_block,
                    err_ident,
                    is_err_mut,
                    catch_body,
                })
            }
            Some(Token::Defer) => {
                self.bump()?;
                Ok(Stmt::Defer(Box::new(self.parse_stmt()?)))
            }
            Some(Token::ErrDefer) => {
                self.bump()?;
                Ok(Stmt::ErrDefer(Box::new(self.parse_stmt()?)))
            }
            Some(Token::Bang) => {
                if self.peek_next() == Some(&Token::LBrace) {
                    self.bump()?;
                    Ok(Stmt::ComptimeBlock(self.parse_block_stmt()?))
                } else {
                    let expr = self.parse_expr()?;
                    if self.peek() == Some(&Token::Semicolon) {
                        self.bump()?;
                    }
                    Ok(Stmt::Expr(expr))
                }
            }
            Some(Token::At) => {
                if matches!(self.peek_next(), Some(Token::Ident(id)) if id == "tls" || id == "threadlocal")
                {
                    return Ok(Stmt::Var(self.parse_var_decl()?));
                }
                if matches!(self.peek_next(), Some(Token::Ident(id)) if id == "comptime") {
                    let mut clone = self.clone_lookahead();
                    let _ = clone.bump();
                    let _ = clone.bump();
                    if clone.peek() == Some(&Token::LBrace) {
                        self.bump()?;
                        self.bump()?;
                        return Ok(Stmt::ComptimeBlock(self.parse_block_stmt()?));
                    }
                }
                if self.peek_next() == Some(&Token::LBrace) {
                    self.bump()?; // consume @
                    Ok(Stmt::ComptimeBlock(self.parse_block_stmt()?))
                } else if self.peek_next() == Some(&Token::If)
                    || matches!(self.peek_next(), Some(Token::Ident(id)) if id == "if")
                {
                    self.bump()?; // consume @
                    Ok(Stmt::If(self.parse_if_stmt_with_comptime(true)?))
                } else if self.peek_next() == Some(&Token::For)
                    || matches!(self.peek_next(), Some(Token::Ident(id)) if id == "for")
                {
                    self.bump()?; // consume @
                    Ok(Stmt::For(self.parse_for_stmt_with_comptime(true)?))
                } else {
                    let mut clone = self.clone_lookahead();
                    let mut is_local_fn = false;
                    while clone.peek() == Some(&Token::At) {
                        if clone.bump().is_err() {
                            break;
                        }
                        if clone.peek() == Some(&Token::LBracket) {
                            if clone.bump().is_err() {
                                break;
                            }
                            let mut depth = 1;
                            while depth > 0 && clone.peek().is_some() {
                                if clone.peek() == Some(&Token::LBracket) {
                                    depth += 1;
                                } else if clone.peek() == Some(&Token::RBracket) {
                                    depth -= 1;
                                }
                                if clone.bump().is_err() {
                                    break;
                                }
                            }
                        } else {
                            if clone.bump().is_err() {
                                break;
                            }
                            if clone.peek() == Some(&Token::LParen) {
                                if clone.bump().is_err() {
                                    break;
                                }
                                let mut depth = 1;
                                while depth > 0 && clone.peek().is_some() {
                                    if clone.peek() == Some(&Token::LParen) {
                                        depth += 1;
                                    } else if clone.peek() == Some(&Token::RParen) {
                                        depth -= 1;
                                    }
                                    if clone.bump().is_err() {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    let mut is_extern = false;
                    if matches!(clone.peek(), Some(Token::Ident(id)) if id == "extern") {
                        let _ = clone.bump();
                        is_extern = true;
                    }
                    if matches!(
                        clone.peek(),
                        Some(Token::Let)
                            | Some(Token::Const)
                            | Some(Token::Var)
                            | Some(Token::ThreadLocal)
                    ) {
                        let meta_list = self.parse_meta_list()?;
                        if is_extern {
                            self.bump()?;
                        }
                        let is_tl = meta_list
                            .iter()
                            .any(|m| m.name.sym == "tls" || m.name.sym == "threadlocal");
                        let is_c = meta_list.iter().any(|m| m.name.sym == "c");
                        let mut v = self.parse_var_decl()?;
                        if is_tl {
                            v.is_threadlocal = true;
                        }
                        if is_extern {
                            v.is_extern = true;
                            v.is_c = is_c || true;
                        } else if is_c {
                            v.is_c = true;
                        }
                        return Ok(Stmt::Var(v));
                    }
                    if matches!(clone.peek(), Some(Token::Fn) | Some(Token::Async)) {
                        is_local_fn = true;
                    }

                    if is_local_fn {
                        let meta_list = self.parse_meta_list()?;
                        let mut func = self.parse_function()?;
                        func.meta = meta_list;
                        return Ok(Stmt::Func(Box::new(func)));
                    }

                    let expr = self.parse_expr()?;
                    if self.peek() == Some(&Token::Semicolon) {
                        self.bump()?;
                    }
                    Ok(Stmt::Expr(expr))
                }
            }
            Some(Token::Unsafe) => {
                self.bump()?;
                Ok(Stmt::UnsafeBlock(self.parse_block_stmt()?))
            }
            Some(Token::Arena) => {
                self.bump()?;
                Ok(Stmt::ArenaBlock(self.parse_block_stmt()?))
            }
            Some(Token::Fuzz) => {
                let f = self.parse_fuzz_decl()?;
                Ok(Stmt::Fuzz(Box::new(f)))
            }
            Some(Token::LBrace) => Ok(Stmt::Block(self.parse_block_stmt()?)),
            Some(Token::ZigEscape) => {
                let (span, code) = self.parse_raw_zig_block()?;
                Ok(Stmt::Expr(Expr::Zig(ZigEscapeExpr { span, code })))
            }
            _ => {
                let expr = self.parse_expr()?;
                if self.peek() == Some(&Token::Semicolon) {
                    self.bump()?;
                }
                Ok(Stmt::Expr(expr))
            }
        }
    }

    pub(super) fn parse_var_decl(&mut self) -> ParseResult<VarDecl> {
        let start = self.peek_span().start;
        let mut is_comptime = false;
        if matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime") {
            let mut clone = self.clone_lookahead();
            let _ = clone.bump();
            if matches!(
                clone.peek(),
                Some(Token::Let) | Some(Token::Var) | Some(Token::Const)
            ) {
                self.bump()?; // consume 'comptime'
                is_comptime = true;
            }
        }
        let mut is_threadlocal = false;
        if self.peek() == Some(&Token::ThreadLocal) {
            self.bump()?;
            is_threadlocal = true;
        } else if self.peek() == Some(&Token::At) {
            let mut clone = self.clone_lookahead();
            let _ = clone.bump();
            if matches!(clone.peek(), Some(Token::Ident(id)) if id == "tls" || id == "threadlocal")
            {
                self.bump()?; // consume @
                self.bump()?; // consume tls / threadlocal
                is_threadlocal = true;
            }
        }
        let (kw_tok, kw_span) = self.bump()?;
        let kind = match kw_tok {
            Token::Let => VarDeclKind::Let,
            Token::Var => {
                self.push_error(
                    Diagnostic::new("E0103", "abolished keyword 'var' in variable declaration")
                        .with_span(kw_span)
                        .with_cause("'var' is Zig/JS legacy syntax and is abolished in CP.")
                        .with_help("in CP, use 'let' for mutable local variables and 'const' for immutable bindings.")
                        .with_suggestion("replace 'var' with 'let'"),
                );
                VarDeclKind::Let
            }
            Token::Const => VarDeclKind::Const,
            tok => return Err(ParseError::ExpectedToken(Token::Let, tok, self.peek_span())),
        };
        if self.peek() == Some(&Token::Mut) {
            let (_, span) = self.bump()?;
            self.push_error(
                Diagnostic::new("E0104", "abolished keyword 'mut' in variable declaration")
                    .with_span(span)
                    .with_cause("'let mut' is Rust legacy syntax; 'let' in CP is already mutable by default.")
                    .with_help("use 'let x = ...' for mutable variables and 'const x = ...' for immutable variables.")
                    .with_suggestion("remove 'mut'"),
            );
        }

        let pat = self.parse_pattern()?;
        if let Pattern::Ident(ref id) = pat {
            if id.sym == "_" {
                self.push_error(
                    Diagnostic::new(
                        "E0120",
                        "cannot use '_' to discard values; '_' is reserved for the current instance receiver",
                    )
                    .with_span(id.span)
                    .with_cause("in CP, '_' exclusively represents the current struct/class/enum instance ('self'/'this'). Using '_' as a discard target causes ambiguity.")
                    .with_help("use '* = expr;' or 'let * = expr;' to discard expression results.")
                    .with_suggestion("replace 'let _ = ...;' with '* = ...;'"),
                );
            }
        }
        let mut ty = None;
        if self.peek() == Some(&Token::Colon) {
            self.bump()?;
            ty = Some(self.parse_type()?);
        }
        let init = if self.peek() == Some(&Token::Assign) {
            self.bump()?;
            Some(self.parse_expr()?)
        } else {
            Some(Expr::Lit(Lit::Undefined))
        };
        if self.peek() == Some(&Token::Semicolon) {
            self.bump()?;
        }
        let end = self.peek_span().end;
        if matches!(ty, Some(Type::Auto) | Some(Type::Any))
            && (matches!(init, Some(Expr::Lit(Lit::Undefined)))
                || matches!(&init, Some(Expr::Ident(id)) if id.sym == "undefined"))
        {
            self.push_error(
                Diagnostic::new(
                    "E0212",
                    "cannot infer type 'auto' from 'undefined'; explicit concrete type is required",
                )
                .with_span(Span { start, end })
                .with_cause("'auto' requires a concrete initializer expression for type inference, but 'undefined' carries no type information and would lower to invalid Zig 'anytype'.")
                .with_help("annotate the variable with an explicit concrete type, e.g. 'let x: u32 = undefined;'.")
                .with_suggestion("replace 'auto' with a concrete type"),
            );
        }
        Ok(VarDecl {
            span: Span { start, end },
            kind,
            pat,
            ty,
            init,
            is_threadlocal,
            is_pub: false,
            is_export: false,
            is_comptime,
            is_extern: false,
            is_c: false,
        })
    }

    pub(super) fn parse_block_stmt(&mut self) -> ParseResult<BlockStmt> {
        let start = self.expect(Token::LBrace)?.start;
        let mut body = Vec::new();
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            let start_pos = self.peek_span().start;
            match self.parse_stmt() {
                Ok(stmt) => body.push(stmt),
                Err(err) => {
                    self.push_error(err.to_diagnostic());
                    self.recover_stmt();
                    if let Some(_) = self.peek() {
                        if self.peek_span().start == start_pos {
                            let _ = self.bump();
                        }
                    }
                }
            }
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(BlockStmt {
            span: Span { start, end },
            body,
        })
    }

    pub(super) fn parse_return_stmt(&mut self) -> ParseResult<ReturnStmt> {
        let span = self.expect(Token::Return)?;
        let mut arg = None;
        let has_arg = if matches!(
            self.peek(),
            Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
        ) {
            false
        } else {
            let next_start = self.peek_span().start;
            if next_start >= span.end && self.input[span.end..next_start].contains('\n') {
                false
            } else {
                true
            }
        };
        if has_arg {
            arg = Some(self.parse_expr()?);
        }
        if self.peek() == Some(&Token::Semicolon) {
            self.bump()?;
        }
        let end = self.peek_span().end;
        Ok(ReturnStmt {
            span: Span {
                start: span.start,
                end,
            },
            arg,
        })
    }

    pub(super) fn parse_match_expr(&mut self) -> ParseResult<MatchExpr> {
        let start = self.peek_span().start;
        let disc = self.parse_expr()?;
        self.expect(Token::LBrace)?;
        let mut arms = Vec::new();
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            let is_inline = if matches!(self.peek(), Some(Token::Ident(id)) if id == "inline") {
                self.bump()?;
                true
            } else {
                false
            };
            self.in_switch_arm += 1;
            let pat = self.parse_pattern()?;
            self.in_switch_arm -= 1;
            let guard = if self.peek() == Some(&Token::If) {
                self.bump()?;
                let prev = self.in_match_guard;
                self.in_match_guard = true;
                let g_res = self.parse_expr();
                self.in_match_guard = prev;
                Some(g_res?)
            } else {
                None
            };
            self.expect(Token::FatArrow)?;
            let (capture, is_capture_ptr) = if self.peek() == Some(&Token::Pipe) {
                self.bump()?; // consume '|'
                let is_ptr =
                    if self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut) {
                        self.bump()?;
                        true
                    } else {
                        false
                    };
                let cap_id = self.parse_ident()?;
                self.expect(Token::Pipe)?;
                (Some(cap_id), is_ptr)
            } else {
                (None, false)
            };
            let body = if self.peek() == Some(&Token::LBrace) {
                Stmt::Block(self.parse_block_stmt()?)
            } else if self.peek() == Some(&Token::Return) {
                Stmt::Return(self.parse_return_stmt()?)
            } else {
                let expr = self.parse_expr()?;
                Stmt::Expr(expr)
            };
            if self.peek() == Some(&Token::Comma) || self.peek() == Some(&Token::Semicolon) {
                self.bump()?;
            }
            arms.push(MatchArm {
                span: Span { start: 0, end: 0 },
                pat,
                guard,
                body: Box::new(body),
                is_inline,
                capture,
                is_capture_ptr,
            });
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(MatchExpr {
            span: Span { start, end },
            disc: Box::new(disc),
            arms,
        })
    }

    pub(super) fn parse_while_stmt(&mut self) -> ParseResult<WhileStmt> {
        let start = self.expect(Token::While)?.start;
        if self.peek() != Some(&Token::LParen) {
            return Err(ParseError::Generic(
                "CP Syntax Error: 'while' condition requires parentheses.\n  = help: in CP, while loops require parentheses around the condition.\n  = suggestion: wrap the condition in parentheses: while (condition) { ... }".to_string(),
                self.peek_span(),
            ));
        }
        self.expect(Token::LParen)?;
        let cond_start = self.peek_span().start;
        let mut capture = None;
        let mut is_ptr_capture = false;
        if self.peek() == Some(&Token::Let) || self.peek() == Some(&Token::Const) {
            let (kw_tok, kw_span) = self.bump()?;
            let kw_str = if kw_tok == Token::Const {
                "const"
            } else {
                "let"
            };
            if self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut) {
                self.bump()?;
                is_ptr_capture = true;
            }
            let cap_id = self.parse_ident()?;
            let assign_span = if self.peek() == Some(&Token::ColonAssign) {
                self.bump()?.1
            } else {
                self.expect(Token::Assign)?
            };
            let replacement = format!("{}{} :=", if is_ptr_capture { "*" } else { "" }, cap_id.sym);
            self.push_error(
                Diagnostic::new(
                    "E0126",
                    format!(
                        "'{}' is not used in 'while' optional/iterator unwrapping; use ':='",
                        kw_str
                    ),
                )
                .with_span(Span {
                    start: kw_span.start,
                    end: assign_span.end,
                })
                .with_cause(
                    "in CP, conditional optional/iterator unwrapping uses Walrus ':=' syntax without 'let' or 'const'.",
                )
                .with_help(
                    "use 'while (v := expr)' for value capture or 'while (*v := expr)' for pointer/mutable capture.",
                )
                .with_suggestion(format!(
                    "replace '{} {} =' with '{}'",
                    kw_str, cap_id.sym, replacement
                ))
                .with_edit(kw_span.start, assign_span.end, replacement),
            );
            capture = Some(cap_id);
        } else if (self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut))
            && matches!(
                self.peek_next(),
                Some(Token::Ident(_)) | Some(Token::ErrorKw)
            )
        {
            self.bump()?;
            is_ptr_capture = true;
            capture = Some(self.parse_ident()?);
            self.expect(Token::ColonAssign)?;
        } else if matches!(self.peek(), Some(Token::Ident(_)) | Some(Token::ErrorKw))
            && self.peek_next() == Some(&Token::ColonAssign)
        {
            capture = Some(self.parse_ident()?);
            self.expect(Token::ColonAssign)?;
        }
        let test = self.parse_expr()?;
        let rparen_span = self.expect(Token::RParen)?;
        let mut continue_expr = None;
        if self.peek() == Some(&Token::Colon) {
            self.bump()?; // consume ':'
            if self.peek() == Some(&Token::LParen) {
                self.bump()?; // consume '('
                continue_expr = Some(self.parse_expr()?);
                self.expect(Token::RParen)?;
            } else {
                continue_expr = Some(self.parse_expr()?);
            }
        }
        if self.peek() == Some(&Token::Pipe) {
            let (_, pipe_start) = self.bump()?; // consume '|'
            if self.peek() == Some(&Token::Star) {
                self.bump()?;
                is_ptr_capture = true;
            }
            let cap_id = self.parse_ident()?;
            let pipe_end = self.expect(Token::Pipe)?;
            let mut diag = Diagnostic::new(
                "E0118",
                "Zig-style '|capture|' syntax is abolished in 'while' statement",
            )
            .with_span(Span {
                start: pipe_start.start,
                end: pipe_end.end,
            })
            .with_cause(
                "trailing '|v|' capture after parentheses is Zig syntax and is abolished in CP.",
            )
            .with_help("in CP, optional/iterator unwrapping uses ':=' inside parentheses.")
            .with_suggestion(format!(
                "use 'while ({}{} := expr) {{ ... }}'",
                if is_ptr_capture { "*" } else { "" },
                cap_id.sym
            ));
            if continue_expr.is_none() && rparen_span.start >= cond_start {
                let cond_text = self.input[cond_start..rparen_span.start].trim();
                diag = diag.with_edit(
                    cond_start,
                    pipe_end.end,
                    format!(
                        "{}{} := {})",
                        if is_ptr_capture { "*" } else { "" },
                        cap_id.sym,
                        cond_text
                    ),
                );
            }
            self.push_error(diag);
            capture = Some(cap_id);
        }
        if continue_expr.is_none() && self.peek() == Some(&Token::Colon) {
            self.bump()?; // consume ':'
            if self.peek() == Some(&Token::LParen) {
                self.bump()?; // consume '('
                continue_expr = Some(self.parse_expr()?);
                self.expect(Token::RParen)?;
            } else {
                continue_expr = Some(self.parse_expr()?);
            }
        }
        let body = self.parse_stmt()?;
        Ok(WhileStmt {
            span: Span { start, end: 0 },
            test,
            body: Box::new(body),
            capture,
            is_ptr_capture,
            label: None,
            continue_expr,
            is_inline: false,
        })
    }

    pub(super) fn parse_for_stmt(&mut self) -> ParseResult<ForStmt> {
        self.parse_for_stmt_with_comptime(false)
    }

    pub(super) fn parse_for_stmt_with_comptime(
        &mut self,
        is_comptime: bool,
    ) -> ParseResult<ForStmt> {
        let start = if self.peek() == Some(&Token::For) {
            self.bump()?.1.start
        } else if matches!(self.peek(), Some(Token::Ident(id)) if id == "for") {
            self.bump()?.1.start
        } else {
            self.expect(Token::For)?.start
        };
        if self.peek() != Some(&Token::LParen) {
            return Err(ParseError::Generic(
                "CP Syntax Error: 'for' loop statement requires parentheses.\n  = cause: 'for' loop header is missing enclosing parentheses: 'for (...)'.\n  = help: in CP, control flow statements ('if', 'while', 'for') must be enclosed in parentheses to eliminate ambiguity with struct literal syntax and maintain syntax consistency.\n  = suggestion: wrap the loop header in parentheses: for (item of collection) { ... }".to_string(),
                self.peek_span(),
            ));
        }
        self.expect(Token::LParen)?;

        // Check if this is Zig-style for loop: for (collection) |capture|
        let is_zig_style = {
            let mut clone = self.clone_lookahead();
            let mut depth = 0;
            let mut found_of_or_in = false;
            let mut has_pipe_after_rparen = false;
            while let Some(tok) = clone.peek() {
                match tok {
                    Token::LParen => depth += 1,
                    Token::RParen => {
                        if depth == 0 {
                            clone.bump().ok();
                            if clone.peek() == Some(&Token::Pipe) {
                                has_pipe_after_rparen = true;
                            }
                            break;
                        }
                        depth -= 1;
                    }
                    Token::Of | Token::In if depth == 0 => {
                        found_of_or_in = true;
                        break;
                    }
                    _ => {}
                }
                clone.bump().ok();
            }
            !found_of_or_in && has_pipe_after_rparen
        };

        if is_zig_style {
            let header_start = self.peek_span().start;
            let mut depth = 0;
            while let Some(tok) = self.peek() {
                if tok == &Token::LParen {
                    depth += 1;
                } else if tok == &Token::RParen {
                    if depth == 0 {
                        break;
                    }
                    depth -= 1;
                }
                self.bump()?;
            }
            let header_end = self.peek_span().start;
            let raw_header = self.input[header_start..header_end].trim().to_string();
            self.expect(Token::RParen)?;
            self.expect(Token::Pipe)?;
            let pat_start = self.peek_span().start;
            let mut pipe_depth = 0;
            while let Some(tok) = self.peek() {
                if tok == &Token::Pipe && pipe_depth == 0 {
                    break;
                }
                self.bump()?;
            }
            let pat_end = self.peek_span().start;
            let raw_pat = self.input[pat_start..pat_end].trim().to_string();
            let pipe_end = self.expect(Token::Pipe)?;
            let mut diag = Diagnostic::new(
                "E0118",
                "Zig-style 'for (collection) |item|' syntax is abolished in CP",
            )
            .with_span(Span {
                start,
                end: pipe_end.end,
            })
            .with_cause("trailing '|item|' capture is Zig syntax and is abolished in CP.")
            .with_help(
                "in CP, 'for' loops use TypeScript-style 'for (item of collection)' inside parentheses.",
            )
            .with_suggestion(format!("use 'for ({} of {}) {{ ... }}'", raw_pat, raw_header));
            if !raw_header.contains(',') && !raw_pat.contains(',') {
                diag = diag.with_edit(
                    header_start,
                    pipe_end.end,
                    format!("{} of {})", raw_pat, raw_header),
                );
            }
            self.push_error(diag);
            let body = self.parse_stmt()?;
            return Ok(ForStmt {
                span: Span { start, end: 0 },
                mode: IterMode::Borrow,
                pat: Pattern::Ident(Ident {
                    span: Span {
                        start: pat_start,
                        end: pat_end,
                    },
                    sym: raw_pat,
                }),
                expr: Expr::Ident(Ident {
                    span: Span {
                        start: header_start,
                        end: header_end,
                    },
                    sym: raw_header.clone(),
                }),
                body: Box::new(body),
                is_comptime,
                label: None,
                raw_header: Some(raw_header),
            });
        }

        if self.peek() == Some(&Token::Let) || self.peek() == Some(&Token::Const) {
            self.bump()?;
        }
        let mode = if self.peek() == Some(&Token::Star) {
            self.bump()?;
            IterMode::MutBorrow
        } else if self.peek() == Some(&Token::Ampersand) {
            self.bump()?;
            if self.peek() == Some(&Token::Mut) {
                self.bump()?;
                IterMode::MutBorrow
            } else {
                IterMode::Borrow
            }
        } else if self.peek() == Some(&Token::Move) {
            self.bump()?;
            IterMode::Move
        } else {
            IterMode::Borrow
        };
        let pat = self.parse_pattern()?;
        if matches!(self.peek(), Some(Token::Of) | Some(Token::In)) {
            self.bump()?;
        } else {
            return Err(ParseError::Generic(
                "Expected 'of' in for loop, e.g. 'for (x of list)'".to_string(),
                self.peek_span(),
            ));
        }
        let mut expr = self.parse_expr()?;
        if self.peek() == Some(&Token::DotDot) || self.peek() == Some(&Token::DotDotEq) {
            let inclusive = self.bump()?.0 == Token::DotDotEq;
            let end_expr = self.parse_expr()?;
            let span = Span {
                start: left_span(&expr).start,
                end: right_span(&end_expr).end,
            };
            expr = Expr::Range {
                span,
                start: Box::new(expr),
                end: Box::new(end_expr),
                inclusive,
            };
        }
        self.expect(Token::RParen)?;
        let body = self.parse_stmt()?;
        Ok(ForStmt {
            span: Span { start, end: 0 },
            mode,
            pat,
            expr,
            body: Box::new(body),
            is_comptime,
            label: None,
            raw_header: None,
        })
    }

    pub(super) fn parse_if_stmt(&mut self) -> ParseResult<IfStmt> {
        self.parse_if_stmt_with_comptime(false)
    }

    pub(super) fn parse_if_stmt_with_comptime(
        &mut self,
        mut is_comptime: bool,
    ) -> ParseResult<IfStmt> {
        let start = if self.peek() == Some(&Token::If) {
            self.bump()?.1.start
        } else if matches!(self.peek(), Some(Token::Ident(id)) if id == "if") {
            self.bump()?.1.start
        } else {
            self.expect(Token::If)?.start
        };
        let mut capture = None;
        let mut is_ptr_capture = false;
        let mut cond_range_for_pipe: Option<(usize, usize)> = None;
        let test = if self.peek() == Some(&Token::LParen) {
            self.bump()?;
            let cond_start = self.peek_span().start;
            if matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime") {
                self.bump()?;
                is_comptime = true;
            }
            if self.peek() == Some(&Token::Let) || self.peek() == Some(&Token::Const) {
                let (kw_tok, kw_span) = self.bump()?;
                let kw_str = if kw_tok == Token::Const {
                    "const"
                } else {
                    "let"
                };
                if self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut) {
                    self.bump()?;
                    is_ptr_capture = true;
                }
                let cap_id = self.parse_ident()?;
                let assign_span = if self.peek() == Some(&Token::ColonAssign) {
                    self.bump()?.1
                } else {
                    self.expect(Token::Assign)?
                };
                let replacement =
                    format!("{}{} :=", if is_ptr_capture { "*" } else { "" }, cap_id.sym);
                self.push_error(
                    Diagnostic::new(
                        "E0126",
                        format!(
                            "'{}' is not used in 'if' optional/error unwrapping; use ':='",
                            kw_str
                        ),
                    )
                    .with_span(Span {
                        start: kw_span.start,
                        end: assign_span.end,
                    })
                    .with_cause(
                        "in CP, conditional optional/error unwrapping uses Walrus ':=' syntax without 'let' or 'const'.",
                    )
                    .with_help(
                        "use 'if (v := opt)' for value capture or 'if (*v := opt)' for pointer/mutable capture.",
                    )
                    .with_suggestion(format!(
                        "replace '{} {} =' with '{}'",
                        kw_str, cap_id.sym, replacement
                    ))
                    .with_edit(kw_span.start, assign_span.end, replacement),
                );
                capture = Some(cap_id);
            } else if (self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut))
                && matches!(
                    self.peek_next(),
                    Some(Token::Ident(_)) | Some(Token::ErrorKw)
                )
            {
                self.bump()?;
                is_ptr_capture = true;
                capture = Some(self.parse_ident()?);
                self.expect(Token::ColonAssign)?;
            } else if matches!(self.peek(), Some(Token::Ident(_)) | Some(Token::ErrorKw))
                && self.peek_next() == Some(&Token::ColonAssign)
            {
                capture = Some(self.parse_ident()?);
                self.expect(Token::ColonAssign)?;
            }
            let t = self.parse_expr()?;
            let rparen_span = self.expect(Token::RParen)?;
            cond_range_for_pipe = Some((cond_start, rparen_span.start));
            t
        } else {
            return Err(ParseError::Generic(
                "CP Syntax Error: 'if' condition requires parentheses.\n  = help: in CP, control flow conditions must be enclosed in parentheses to avoid ambiguity with struct literals.\n  = suggestion: wrap the condition in parentheses: if (condition) { ... }".to_string(),
                self.peek_span(),
            ));
        };
        if self.peek() == Some(&Token::Pipe) {
            let (_, pipe_start) = self.bump()?; // consume '|'
            if self.peek() == Some(&Token::Star) {
                self.bump()?;
                is_ptr_capture = true;
            }
            let cap_id = self.parse_ident()?;
            let pipe_end = self.expect(Token::Pipe)?;
            let mut diag = Diagnostic::new(
                "E0118",
                "Zig-style '|capture|' syntax is abolished in 'if' statement",
            )
            .with_span(Span {
                start: pipe_start.start,
                end: pipe_end.end,
            })
            .with_cause(
                "trailing '|v|' capture after parentheses is Zig syntax and is abolished in CP.",
            )
            .with_help("in CP, optional/error unwrapping uses ':=' inside parentheses.")
            .with_suggestion(format!(
                "use 'if ({}{} := expr) {{ ... }}'",
                if is_ptr_capture { "*" } else { "" },
                cap_id.sym
            ));
            if let Some((cond_start, cond_end)) = cond_range_for_pipe {
                if cond_end >= cond_start {
                    let cond_text = self.input[cond_start..cond_end].trim();
                    diag = diag.with_edit(
                        cond_start,
                        pipe_end.end,
                        format!(
                            "{}{} := {})",
                            if is_ptr_capture { "*" } else { "" },
                            cap_id.sym,
                            cond_text
                        ),
                    );
                }
            }
            self.push_error(diag);
            capture = Some(cap_id);
        }
        let cons = self.parse_stmt()?;
        let mut alt = None;
        if self.peek() == Some(&Token::Else) {
            self.bump()?;
            alt = Some(Box::new(self.parse_stmt()?));
        } else if self.peek() == Some(&Token::At)
            && (self.peek_next() == Some(&Token::Else)
                || matches!(self.peek_next(), Some(Token::Ident(id)) if id == "else"))
        {
            self.bump()?; // consume @
            self.bump()?; // consume else
            alt = Some(Box::new(self.parse_stmt()?));
        }
        Ok(IfStmt {
            span: Span { start, end: 0 },
            test,
            cons: Box::new(cons),
            alt,
            is_comptime,
            capture,
            is_ptr_capture,
        })
    }
}
