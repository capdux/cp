use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

impl<'a> Parser<'a> {
    pub(super) fn parse_meta_ident(&mut self) -> ParseResult<Ident> {
        let (tok, span) = self.bump()?;
        let sym = match tok {
            Token::Ident(s) => s,
            Token::Class => "class".to_string(),
            Token::Struct => "struct".to_string(),
            Token::Fn => "fn".to_string(),
            Token::Auto => "auto".to_string(),
            Token::Ref => "ref".to_string(),
            Token::Let => "let".to_string(),
            Token::Const => "const".to_string(),
            Token::Meta => "meta".to_string(),
            Token::Dec => "dec".to_string(),
            Token::Macro => "macro".to_string(),
            Token::Async => "async".to_string(),
            Token::Static => "static".to_string(),
            Token::Mut => "mut".to_string(),
            Token::Move => "move".to_string(),
            Token::Own => "own".to_string(),
            Token::Import => "import".to_string(),
            Token::Export => "export".to_string(),
            Token::TypeKw => "type".to_string(),
            Token::CStr => "cstr".to_string(),
            Token::As => "as".to_string(),
            Token::From => "from".to_string(),
            Token::Unsafe => "unsafe".to_string(),
            Token::Arena => "arena".to_string(),
            _ => {
                return Err(ParseError::ExpectedToken(
                    Token::Ident("identifier".to_string()),
                    tok,
                    span,
                ))
            }
        };
        Ok(Ident { span, sym })
    }

    pub(super) fn parse_meta_arg(&mut self) -> ParseResult<MetaArg> {
        let is_ident_like = matches!(
            self.peek(),
            Some(Token::Ident(_))
                | Some(Token::Class)
                | Some(Token::Struct)
                | Some(Token::Fn)
                | Some(Token::Ref)
                | Some(Token::Let)
                | Some(Token::Const)
                | Some(Token::Meta)
                | Some(Token::Dec)
                | Some(Token::Macro)
                | Some(Token::Async)
                | Some(Token::Static)
                | Some(Token::Mut)
                | Some(Token::Move)
                | Some(Token::Own)
                | Some(Token::Import)
                | Some(Token::Export)
                | Some(Token::TypeKw)
                | Some(Token::CStr)
                | Some(Token::As)
                | Some(Token::From)
                | Some(Token::Unsafe)
        );
        if is_ident_like && self.peek_next() == Some(&Token::Colon) {
            let ident = self.parse_meta_ident()?;
            self.bump()?; // consume Colon
            let expr = self.parse_expr()?;
            return Ok(MetaArg::Named(ident, expr));
        }
        let expr = self.parse_expr()?;
        Ok(MetaArg::Positional(expr))
    }

    pub(super) fn parse_single_meta_inner(&mut self, start: usize) -> ParseResult<MetaItem> {
        let name = self.parse_meta_ident()?;
        let mut args = Vec::new();
        if self.peek() == Some(&Token::LParen) {
            self.bump()?;
            while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                args.push(self.parse_meta_arg()?);
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
            }
            self.expect(Token::RParen)?;
        }
        let end = self.peek_span().start;
        Ok(MetaItem {
            span: Span { start, end },
            name,
            args,
        })
    }

    #[allow(dead_code)]
    pub(super) fn parse_meta_item(&mut self) -> ParseResult<MetaItem> {
        let start = self.expect(Token::At)?.start;
        self.parse_single_meta_inner(start)
    }

    pub(super) fn parse_meta_list(&mut self) -> ParseResult<Vec<MetaItem>> {
        let mut list = Vec::new();
        while self.peek() == Some(&Token::At) {
            if self.peek_next() == Some(&Token::LBracket) {
                let _ = self.bump()?; // consume '@'
                self.expect(Token::LBracket)?; // consume '['
                while self.peek() != Some(&Token::RBracket) && self.peek().is_some() {
                    let start = if self.peek() == Some(&Token::At) {
                        self.bump()?.1.start
                    } else {
                        self.peek_span().start
                    };
                    let item = self.parse_single_meta_inner(start)?;
                    list.push(item);
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                    }
                }
                self.expect(Token::RBracket)?;
            } else {
                let start = self.expect(Token::At)?.start;
                let item = self.parse_single_meta_inner(start)?;
                list.push(item);
            }
        }
        Ok(list)
    }

    pub(super) fn parse_meta_def(&mut self, meta_list: Vec<MetaItem>) -> ParseResult<MetaDef> {
        let start = self.expect(Token::Meta)?.start;
        let ident = self.parse_ident()?;
        let mut params = Vec::new();
        if self.peek() == Some(&Token::LParen) {
            self.bump()?;
            while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                let p_start = self.peek_span().start;
                let p_ident = self.parse_ident()?;
                self.expect(Token::Colon)?;
                let ty = self.parse_type()?;
                let mut default = None;
                if self.peek() == Some(&Token::Assign) {
                    self.bump()?;
                    default = Some(self.parse_primary_expr()?);
                }
                params.push(MetaParam {
                    span: Span {
                        start: p_start,
                        end: self.peek_span().start,
                    },
                    ident: p_ident,
                    ty,
                    default,
                });
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
            }
            self.expect(Token::RParen)?;
        }
        if self.peek() == Some(&Token::Semicolon) {
            self.bump()?;
        }
        let end = self.peek_span().start;
        Ok(MetaDef {
            span: Span { start, end },
            ident,
            params,
            meta: meta_list,
        })
    }

    pub(super) fn parse_dec_def(&mut self, meta_list: Vec<MetaItem>) -> ParseResult<DecDef> {
        let start = self.expect(Token::Dec)?.start;
        let ident = self.parse_ident()?;
        let params = self.parse_params()?;
        let mut return_type = None;
        if self.peek() == Some(&Token::Bang) {
            self.bump()?;
            if self.is_error_type_rhs_start() {
                let err = self.parse_error_type_rhs()?;
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return_type = Some(Type::ResultType {
                    ok: Box::new(ok_ty),
                    err,
                });
            } else {
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return_type = Some(Type::ErrorUnion(Box::new(ok_ty)));
            }
        } else if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
            self.bump()?;
            return_type = Some(self.parse_type()?);
        } else if self.peek().map(Self::is_type_start).unwrap_or(false)
            && self.peek() != Some(&Token::LBrace)
            && self.peek() != Some(&Token::Semicolon)
        {
            return_type = Some(self.parse_type()?);
        }
        let body = self.parse_block_stmt()?;
        let end = body.span.end;
        Ok(DecDef {
            span: Span { start, end },
            ident,
            params,
            return_type,
            body,
            meta: meta_list,
        })
    }

    pub(super) fn parse_macro_def(&mut self, meta_list: Vec<MetaItem>) -> ParseResult<MacroDef> {
        let start = self.expect(Token::Macro)?.start;
        let ident = self.parse_ident()?;
        let params = self.parse_params()?;
        let mut return_type = None;
        if self.peek() == Some(&Token::Bang) {
            self.bump()?;
            if self.is_error_type_rhs_start() {
                let err = self.parse_error_type_rhs()?;
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return_type = Some(Type::ResultType {
                    ok: Box::new(ok_ty),
                    err,
                });
            } else {
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return_type = Some(Type::ErrorUnion(Box::new(ok_ty)));
            }
        } else if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
            self.bump()?;
            return_type = Some(self.parse_type()?);
        } else if self.peek().map(Self::is_type_start).unwrap_or(false)
            && self.peek() != Some(&Token::LBrace)
            && self.peek() != Some(&Token::Semicolon)
        {
            return_type = Some(self.parse_type()?);
        }
        self.expect(Token::LBrace)?;
        let mut methods = Vec::new();
        let mut body_stmts = Vec::new();
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            let mut is_pub = true;
            if self.peek() == Some(&Token::Private) {
                self.bump()?;
                is_pub = false;
            } else if self.peek() == Some(&Token::Export) {
                self.bump()?;
                is_pub = true;
            }
            if self.peek() == Some(&Token::Fn) || self.peek() == Some(&Token::Async) {
                let mut func = self.parse_function()?;
                func.is_pub = is_pub;
                methods.push(func);
            } else {
                body_stmts.push(self.parse_stmt()?);
            }
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(MacroDef {
            span: Span { start, end },
            ident,
            params,
            return_type,
            body: BlockStmt {
                span: Span { start, end },
                body: body_stmts,
            },
            methods,
            meta: meta_list,
        })
    }

    pub(super) fn parse_module_item(&mut self) -> ParseResult<Vec<ModuleItem>> {
        let mut is_pub = false;
        let mut has_pub_or_priv = false;
        while self.peek() == Some(&Token::Export) || self.peek() == Some(&Token::Private) {
            has_pub_or_priv = true;
            if self.peek() == Some(&Token::Export) {
                is_pub = true;
            }
            self.bump()?;
        }
        if is_pub {
            if self.peek() == Some(&Token::LBrace) {
                return Ok(vec![ModuleItem::Export(self.parse_export_stmt()?)]);
            }
            if self.peek() == Some(&Token::Star) {
                return Ok(vec![ModuleItem::Export(self.parse_export_star_stmt()?)]);
            }
        }
        if self.peek() == Some(&Token::At) {
            // Check for @c { ... } or @c("lib") { ... } block
            let mut clone = self.clone_lookahead();
            let _ = clone.bump(); // consume '@'
            let is_c_block = if matches!(clone.peek(), Some(Token::Ident(s)) if s == "c") {
                let _ = clone.bump(); // consume 'c'
                if clone.peek() == Some(&Token::LParen) {
                    let _ = clone.bump();
                    let mut p_depth = 1;
                    while p_depth > 0 && clone.peek().is_some() {
                        match clone.bump() {
                            Ok((Token::LParen, _)) => p_depth += 1,
                            Ok((Token::RParen, _)) => p_depth -= 1,
                            _ => {}
                        }
                    }
                }
                clone.peek() == Some(&Token::LBrace)
            } else {
                false
            };
            if is_c_block {
                let _ = self.bump(); // consume '@'
                let c_ident = self.parse_ident()?; // consume 'c'
                let mut c_args = Vec::new();
                if self.peek() == Some(&Token::LParen) {
                    self.bump()?;
                    while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                        c_args.push(self.parse_meta_arg()?);
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    self.expect(Token::RParen)?;
                }
                self.expect(Token::LBrace)?;
                let mut items = Vec::new();
                while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                    let mut item_meta = self.parse_meta_list()?;
                    let mut item_is_pub = is_pub;
                    while self.peek() == Some(&Token::Export)
                        || self.peek() == Some(&Token::Private)
                    {
                        if self.peek() == Some(&Token::Export) {
                            item_is_pub = true;
                        } else {
                            item_is_pub = false;
                        }
                        self.bump()?;
                    }
                    if matches!(self.peek(), Some(Token::StringLiteral(_))) {
                        let (tok, span) = self.bump()?;
                        if let Token::StringLiteral(header) = tok {
                            items.push(ModuleItem::Decl(Decl::CInclude(CIncludeDecl {
                                span,
                                header,
                            })));
                            if self.peek() == Some(&Token::Comma)
                                || self.peek() == Some(&Token::Semicolon)
                            {
                                self.bump()?;
                            }
                        }
                    } else if self.peek() == Some(&Token::TypeKw) {
                        let start_ta = self.bump()?.1.start;
                        let ident = self.parse_ident()?;
                        self.expect(Token::Assign)?;
                        let target_ty = self.parse_type()?;
                        if self.peek() == Some(&Token::Semicolon) {
                            self.bump()?;
                        }
                        let end_ta = self.peek_span().start;
                        items.push(ModuleItem::Decl(Decl::TypeAlias(TypeAliasDecl {
                            span: Span {
                                start: start_ta,
                                end: end_ta,
                            },
                            ident,
                            target_ty,
                            is_pub: item_is_pub,
                        })));
                    } else if matches!(
                        self.peek(),
                        Some(Token::Struct | Token::Union | Token::Class)
                    ) {
                        let mut cls = self.parse_class()?;
                        cls.is_pub = item_is_pub;
                        if !cls
                            .meta
                            .iter()
                            .any(|m| m.name.sym == "extern" || m.name.sym == "opaque")
                        {
                            cls.meta.push(MetaItem {
                                span: c_ident.span,
                                name: Ident {
                                    span: c_ident.span,
                                    sym: "extern".to_string(),
                                },
                                args: vec![],
                            });
                        }
                        cls.meta.extend(item_meta);
                        items.push(ModuleItem::Decl(Decl::Class(cls)));
                    } else if self.peek() == Some(&Token::Enum) {
                        let mut en = self.parse_enum()?;
                        en.meta.extend(item_meta);
                        items.push(ModuleItem::Decl(Decl::Enum(en)));
                    } else if matches!(
                        self.peek(),
                        Some(Token::Const) | Some(Token::Var) | Some(Token::Let)
                    ) {
                        let mut v = self.parse_var_decl()?;
                        v.is_c = true;
                        v.is_extern =
                            v.init.is_none() || matches!(v.init, Some(Expr::Lit(Lit::Undefined)));
                        v.is_pub = item_is_pub;
                        items.push(ModuleItem::Stmt(Stmt::Var(v)));
                    } else if matches!(self.peek(), Some(Token::Ident(s)) if s == "extern")
                        && matches!(
                            self.peek_next(),
                            Some(Token::Const)
                                | Some(Token::Var)
                                | Some(Token::Let)
                                | Some(Token::ThreadLocal)
                        )
                    {
                        self.bump()?; // consume 'extern'
                        let mut v = self.parse_var_decl()?;
                        v.is_c = true;
                        v.is_extern = true;
                        v.is_pub = item_is_pub;
                        items.push(ModuleItem::Stmt(Stmt::Var(v)));
                    } else {
                        let mut f = self.parse_function()?;
                        f.is_pub = item_is_pub;
                        f.meta.append(&mut item_meta);
                        if !f.meta.iter().any(|m| m.name.sym == "c") {
                            f.meta.push(MetaItem {
                                span: c_ident.span,
                                name: Ident {
                                    span: c_ident.span,
                                    sym: "c".to_string(),
                                },
                                args: c_args.clone(),
                            });
                        }
                        items.push(ModuleItem::Decl(Decl::Func(f)));
                    }
                }
                self.expect(Token::RBrace)?;
                return Ok(items);
            }

            let mut clone = self.clone_lookahead();
            let mut has_decl = false;
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
            if let Some(tok) = clone.peek() {
                if matches!(
                    tok,
                    Token::Let | Token::Const | Token::Var | Token::ThreadLocal
                ) {
                    let meta_list = self.parse_meta_list()?;
                    if is_extern {
                        self.bump()?; // consume 'extern'
                    }
                    let is_tl = meta_list
                        .iter()
                        .any(|m| m.name.sym == "tls" || m.name.sym == "threadlocal");
                    let is_c = meta_list.iter().any(|m| m.name.sym == "c");
                    let mut v = self.parse_var_decl()?;
                    if has_pub_or_priv {
                        v.is_pub = is_pub;
                    }
                    if is_tl {
                        v.is_threadlocal = true;
                    }
                    if is_extern {
                        v.is_extern = true;
                        v.is_c = is_c || true;
                    } else if is_c {
                        v.is_c = true;
                    }
                    return Ok(vec![ModuleItem::Stmt(Stmt::Var(v))]);
                }
                if Self::is_decl_keyword(tok) || is_extern {
                    has_decl = true;
                }
            }
            if has_decl {
                let meta_list = self.parse_meta_list()?;
                return Ok(vec![ModuleItem::Decl(self.parse_decl(meta_list, is_pub)?)]);
            }
        }
        match self.peek() {
            Some(Token::Import) => Ok(vec![ModuleItem::Import(self.parse_import_stmt()?)]),
            Some(Token::ThreadLocal) => {
                let mut v = self.parse_var_decl()?;
                if has_pub_or_priv {
                    v.is_pub = is_pub;
                }
                v.is_threadlocal = true;
                Ok(vec![ModuleItem::Stmt(Stmt::Var(v))])
            }
            Some(Token::Let) | Some(Token::Const) | Some(Token::Var) => {
                let mut v = self.parse_var_decl()?;
                if has_pub_or_priv {
                    v.is_pub = is_pub;
                }
                if v.kind == VarDeclKind::Const && v.ty.is_none() {
                    if let Pattern::Ident(ident) = &v.pat {
                        if let Some(Expr::Zig(z)) = &v.init {
                            let trimmed = z.code.trim();
                            if trimmed.starts_with("fn ") || trimmed.starts_with("fn(") {
                                let mut sub_parser = Parser::new(trimmed);
                                if let Ok(target_ty) = sub_parser.parse_fn_type_sig(false) {
                                    return Ok(vec![ModuleItem::Decl(Decl::TypeAlias(
                                        TypeAliasDecl {
                                            span: Span {
                                                start: v.span.start,
                                                end: v.span.end,
                                            },
                                            ident: ident.clone(),
                                            target_ty,
                                            is_pub: v.is_pub,
                                        },
                                    ))]);
                                }
                            }
                        }
                    }
                }
                Ok(vec![ModuleItem::Stmt(Stmt::Var(v))])
            }
            Some(Token::Meta)
            | Some(Token::Dec)
            | Some(Token::Macro)
            | Some(Token::Fn)
            | Some(Token::Class)
            | Some(Token::Struct)
            | Some(Token::Union)
            | Some(Token::Ref)
            | Some(Token::Interface)
            | Some(Token::ErrorKw)
            | Some(Token::Trait)
            | Some(Token::Impl)
            | Some(Token::Enum)
            | Some(Token::Async)
            | Some(Token::Static)
            | Some(Token::Fuzz)
            | Some(Token::TypeKw) => Ok(vec![ModuleItem::Decl(self.parse_decl(vec![], is_pub)?)]),
            Some(Token::Ident(ref id)) if id == "extern" || id == "inline" || id == "noinline" => {
                if id == "extern"
                    && matches!(
                        self.peek_next(),
                        Some(Token::Const)
                            | Some(Token::Var)
                            | Some(Token::Let)
                            | Some(Token::ThreadLocal)
                    )
                {
                    self.bump()?; // consume 'extern'
                    let mut v = self.parse_var_decl()?;
                    v.is_extern = true;
                    v.is_c = true;
                    if has_pub_or_priv {
                        v.is_pub = is_pub;
                    }
                    return Ok(vec![ModuleItem::Stmt(Stmt::Var(v))]);
                }
                Ok(vec![ModuleItem::Decl(self.parse_decl(vec![], is_pub)?)])
            }
            Some(Token::Unsafe) => {
                if self.peek_next() == Some(&Token::LBrace) {
                    Ok(vec![ModuleItem::Stmt(self.parse_stmt()?)])
                } else {
                    Ok(vec![ModuleItem::Decl(self.parse_decl(vec![], is_pub)?)])
                }
            }
            _ => Ok(vec![ModuleItem::Stmt(self.parse_stmt()?)]),
        }
    }

    pub(super) fn parse_import_stmt(&mut self) -> ParseResult<ImportStmt> {
        let start = self.expect(Token::Import)?.start;
        let mut specifiers = Vec::new();
        let mut default_specifier = None;
        if self.peek() == Some(&Token::LBrace) {
            self.bump()?;
            while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                specifiers.push(self.parse_ident()?);
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
            }
            self.expect(Token::RBrace)?;
        } else if self.peek() == Some(&Token::Star) {
            self.bump()?;
            self.expect(Token::As)?;
            default_specifier = Some(self.parse_ident()?);
        } else if matches!(self.peek(), Some(Token::Ident(_))) {
            default_specifier = Some(self.parse_ident()?);
        }
        self.expect(Token::From)?;
        let (token, span) = self.bump()?;
        let source = if let Token::StringLiteral(s) = token {
            s
        } else {
            return Err(ParseError::ExpectedToken(
                Token::StringLiteral("source".to_string()),
                token,
                span,
            ));
        };
        if self.peek() == Some(&Token::Semicolon) {
            self.bump()?;
        }
        let end = span.end;
        Ok(ImportStmt {
            span: Span { start, end },
            specifiers,
            default_specifier,
            source,
        })
    }

    pub(super) fn parse_export_stmt(&mut self) -> ParseResult<ExportStmt> {
        let start = self.expect(Token::LBrace)?.start;
        let mut specifiers = Vec::new();
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            specifiers.push(self.parse_ident()?);
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
            }
        }
        let mut end = self.expect(Token::RBrace)?.end;
        let mut source = None;
        if self.peek() == Some(&Token::From) {
            self.bump()?;
            let (token, span) = self.bump()?;
            let s = if let Token::StringLiteral(str_val) = token {
                str_val
            } else {
                return Err(ParseError::ExpectedToken(
                    Token::StringLiteral("source".to_string()),
                    token,
                    span,
                ));
            };
            end = span.end;
            source = Some(s);
        }
        if self.peek() == Some(&Token::Semicolon) {
            end = self.bump()?.1.end;
        }
        Ok(ExportStmt {
            span: Span { start, end },
            specifiers,
            source,
        })
    }

    pub(super) fn parse_export_star_stmt(&mut self) -> ParseResult<ExportStmt> {
        let start = self.expect(Token::Star)?.start;
        self.expect(Token::From)?;
        let (token, span) = self.bump()?;
        let source = if let Token::StringLiteral(s) = token {
            s
        } else {
            return Err(ParseError::ExpectedToken(
                Token::StringLiteral("source".to_string()),
                token,
                span,
            ));
        };
        let mut end = span.end;
        if self.peek() == Some(&Token::Semicolon) {
            end = self.bump()?.1.end;
        }
        Ok(ExportStmt {
            span: Span { start, end },
            specifiers: vec![],
            source: Some(source),
        })
    }

    pub(super) fn parse_fuzz_decl(&mut self) -> ParseResult<FuzzDecl> {
        let (_, start_span) = self.bump()?;
        let name = match self.peek() {
            Some(Token::StringLiteral(s)) => {
                let name_str = s.clone();
                self.bump()?;
                name_str
            }
            _ => "fuzz_test".to_string(),
        };

        let mut param_name = "input".to_string();
        let mut param_type = None;
        if self.peek() == Some(&Token::LParen) {
            self.bump()?;
            if self.peek() != Some(&Token::RParen) {
                let p_ident = self.parse_ident()?;
                param_name = p_ident.sym;
                if self.peek() == Some(&Token::Colon) {
                    self.bump()?;
                    param_type = Some(self.parse_type()?);
                }
            }
            self.expect(Token::RParen)?;
        }

        let body = self.parse_block_stmt()?;
        let span = Span {
            start: start_span.start,
            end: body.span.end,
        };

        Ok(FuzzDecl {
            span,
            name,
            param_name,
            param_type,
            body,
        })
    }

    pub(super) fn parse_decl(
        &mut self,
        meta: Vec<MetaItem>,
        mut is_pub: bool,
    ) -> ParseResult<Decl> {
        while self.peek() == Some(&Token::Export) || self.peek() == Some(&Token::Private) {
            if self.peek() == Some(&Token::Export) {
                is_pub = true;
            } else if self.peek() == Some(&Token::Private) {
                is_pub = false;
            }
            self.bump()?;
        }
        match self.peek() {
            Some(Token::Meta) => Ok(Decl::Meta(self.parse_meta_def(meta)?)),
            Some(Token::Dec) => Ok(Decl::Dec(self.parse_dec_def(meta)?)),
            Some(Token::Macro) => Ok(Decl::Macro(self.parse_macro_def(meta)?)),
            Some(Token::Fuzz) => Ok(Decl::Fuzz(self.parse_fuzz_decl()?)),
            Some(Token::Static) => {
                if matches!(
                    self.peek_next(),
                    Some(Token::Class) | Some(Token::Struct) | Some(Token::Ref)
                ) {
                    let mut c = self.parse_class()?;
                    c.meta.extend(meta);
                    if is_pub {
                        c.is_pub = true;
                    }
                    Ok(Decl::Class(c))
                } else {
                    let mut f = self.parse_function()?;
                    f.meta.extend(meta);
                    if is_pub {
                        f.is_pub = true;
                    }
                    Ok(Decl::Func(f))
                }
            }
            Some(Token::Fn) | Some(Token::Async) | Some(Token::Unsafe) => {
                let mut f = self.parse_function()?;
                f.meta.extend(meta);
                if is_pub {
                    f.is_pub = true;
                }
                Ok(Decl::Func(f))
            }
            Some(Token::Ident(ref id)) if id == "extern" || id == "inline" || id == "noinline" => {
                let mut f = self.parse_function()?;
                f.meta.extend(meta);
                if is_pub {
                    f.is_pub = true;
                }
                Ok(Decl::Func(f))
            }
            Some(Token::Class) | Some(Token::Struct) | Some(Token::Union) | Some(Token::Ref) => {
                let mut c = self.parse_class()?;
                c.meta.extend(meta);
                if is_pub {
                    c.is_pub = true;
                }
                Ok(Decl::Class(c))
            }
            Some(Token::Trait) => {
                let mut t = self.parse_trait()?;
                t.meta = meta;
                Ok(Decl::Trait(t))
            }
            Some(Token::Impl) => Ok(Decl::Impl(self.parse_impl()?)),
            Some(Token::Enum) => {
                let mut e = self.parse_enum()?;
                e.meta = meta;
                Ok(Decl::Enum(e))
            }
            Some(Token::ErrorKw) => Ok(Decl::ErrorSet(self.parse_error_set()?)),
            Some(Token::TypeKw) => {
                let start = self.bump()?.1.start;
                let ident = self.parse_ident()?;
                self.expect(Token::Assign)?;
                let target_ty = self.parse_type()?;
                if self.peek() == Some(&Token::Semicolon) {
                    self.bump()?;
                }
                let end = self.peek_span().start;
                Ok(Decl::TypeAlias(TypeAliasDecl {
                    span: Span { start, end },
                    ident,
                    target_ty,
                    is_pub,
                }))
            }
            _ => Err(ParseError::UnexpectedToken(
                self.bump()?.0,
                self.peek_span(),
            )),
        }
    }

    pub(super) fn parse_function(&mut self) -> ParseResult<Function> {
        let start = self.peek_span().start;
        let mut is_static = false;
        let mut is_async = false;
        let mut is_unsafe = false;
        let mut is_pub = false;
        let mut meta = Vec::new();
        while matches!(
            self.peek(),
            Some(Token::Static)
                | Some(Token::Async)
                | Some(Token::Unsafe)
                | Some(Token::Export)
                | Some(Token::Private)
        ) || matches!(self.peek(), Some(Token::Ident(id)) if id == "inline" || id == "extern" || id == "noinline")
        {
            if matches!(self.peek(), Some(Token::Ident(id)) if id == "inline" || id == "extern" || id == "noinline")
            {
                let (tok, span) = self.bump()?;
                let sym = match tok {
                    Token::Ident(s) => s,
                    _ => "inline".to_string(),
                };
                meta.push(MetaItem {
                    span,
                    name: Ident { span, sym },
                    args: vec![],
                });
            } else if self.peek() == Some(&Token::Export) {
                let span = self.bump()?.1;
                is_pub = true;
                meta.push(MetaItem {
                    span,
                    name: Ident {
                        span,
                        sym: "export".to_string(),
                    },
                    args: vec![],
                });
            } else if self.peek() == Some(&Token::Private) {
                self.bump()?;
                is_pub = false;
            } else if self.peek() == Some(&Token::Static) {
                self.bump()?;
                is_static = true;
            } else if self.peek() == Some(&Token::Async) {
                self.bump()?;
                is_async = true;
            } else if self.peek() == Some(&Token::Unsafe) {
                self.bump()?;
                is_unsafe = true;
            }
        }
        self.expect(Token::Fn)?;
        let ident = self.parse_ident()?;
        let generic_params = self.parse_generic_params()?;
        let mut params = self.parse_params()?;
        // Absorb Zig-style function qualifiers placed before return type (e.g. `fn cb(x: i32) callconv(.c) {`)
        self.parse_zig_fn_qualifiers(&mut meta)?;
        let mut return_type = None;
        if self.peek() == Some(&Token::Bang) {
            self.bump()?;
            if self.is_error_type_rhs_start() {
                let err = self.parse_error_type_rhs()?;
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return_type = Some(Type::ResultType {
                    ok: Box::new(ok_ty),
                    err,
                });
            } else {
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return_type = Some(Type::ErrorUnion(Box::new(ok_ty)));
            }
        } else if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
            let (_, colon_span) = self.bump()?;
            let ret_start = self.peek_span().start;
            let ty = self.parse_type()?;
            let ret_end = self.prev_token_end.max(ret_start);
            if ty == Type::Void {
                self.push_error(
                    Diagnostic::new("E0122", "redundant ': void' return type annotation")
                        .with_span(Span {
                            start: colon_span.start,
                            end: ret_end,
                        })
                        .with_cause("in CP, functions with no return value default to 'void' when the return type is omitted.")
                        .with_help("omit ': void' from all void functions (including '@c' extern declarations) to keep signatures concise.")
                        .with_suggestion("remove ': void'"),
                );
            }
            return_type = Some(ty);
        } else if self.peek().map(Self::is_type_start).unwrap_or(false)
            && self.peek() != Some(&Token::LBrace)
            && self.peek() != Some(&Token::Semicolon)
        {
            let ret_start = self.peek_span().start;
            let ty = self.parse_type()?;
            let ret_end = self.prev_token_end.max(ret_start);
            if ty == Type::Void {
                self.push_error(
                    Diagnostic::new("E0122", "redundant 'void' return type annotation")
                        .with_span(Span {
                            start: ret_start,
                            end: ret_end,
                        })
                        .with_cause("in CP, functions with no return value default to 'void' when the return type is omitted.")
                        .with_help("omit 'void' from all void functions to keep signatures concise.")
                        .with_suggestion("remove 'void'"),
                );
            } else {
                self.push_error(
                    Diagnostic::new("E0116", "missing ':' before function return type")
                        .with_span(Span {
                            start: ret_start,
                            end: ret_end,
                        })
                        .with_cause("Zig-style bare return type without ':' is abolished in CP.")
                        .with_help("in CP, all non-void function return types must be preceded by ':' (e.g. 'fn foo(): ReturnType').")
                        .with_suggestion(format!(
                            "insert ':' before the return type: 'fn {}(...): {}'",
                            ident.sym,
                            Self::type_to_str(&ty)
                        )),
                );
            }
            return_type = Some(ty);
        }

        let trailing_meta = self.parse_meta_list()?;
        meta.extend(trailing_meta);
        // Also absorb Zig-style function qualifiers placed after return type (e.g. `fn cb(): i32 callconv(.c) {`)
        self.parse_zig_fn_qualifiers(&mut meta)?;

        let body;
        let end;
        if self.peek() == Some(&Token::Semicolon) {
            let span = self.bump()?.1;
            body = BlockStmt { span, body: vec![] };
            end = span.end;
        } else {
            body = self.parse_block_stmt()?;
            end = body.span.end;
        }

        // Check receiver & auto-deduce mutable value copy vs mutable reference for 'v: *T'
        if let Some(first) = params.first() {
            if first.ident.sym == "self" || first.ident.sym == "this" {
                let mutates_rec = Self::block_mutates_receiver(&body) || ident.sym == "init";
                let suggestion = if first.is_move {
                    format!(
                        "replace '{}' with 'move _' and access members via '_.'",
                        first.ident.sym
                    )
                } else if mutates_rec {
                    format!("receiver is mutated in '{}'; replace '{}' with '*_' and access members via '_.'", ident.sym, first.ident.sym)
                } else {
                    format!("receiver is read-only in '{}'; omit '{}' from parameter list and access members via '_.'", ident.sym, first.ident.sym)
                };
                let replacement = if first.is_move {
                    "move _"
                } else if mutates_rec || first.is_mut {
                    "*_"
                } else {
                    "_"
                };
                self.push_error(
                    Diagnostic::new(
                        "E0121",
                        format!("abolished receiver '{}' in method '{}'", first.ident.sym, ident.sym),
                    )
                    .with_span(first.span)
                    .with_cause("Zig 'self' and TypeScript 'this' are abolished in CP to enforce a single unified receiver syntax.")
                    .with_help("in CP, omit the receiver for read-only methods, use '*_' for mutable methods, or 'move _' for consuming methods, and access instance members via '_.field'.")
                    .with_suggestion(suggestion)
                    .with_edit(first.span.start, first.span.end, replacement),
                );
            }
        } else if let Some((bad_sym, bad_span)) = Self::block_find_ident_span(&body, "self")
            .or_else(|| Self::block_find_ident_span(&body, "this"))
        {
            self.push_error(
                Diagnostic::new(
                    "E0121",
                    format!("abolished receiver '{}' in function/method '{}'", bad_sym, ident.sym),
                )
                .with_span(bad_span)
                .with_cause("Zig 'self' and TypeScript 'this' are abolished in CP.")
                .with_help("in CP, access current instance members via '_.' instead of 'self.' or 'this.'.")
                .with_suggestion(format!("replace '{}' with '_'", bad_sym))
                .with_edit(bad_span.start, bad_span.end, "_"),
            );
        }

        if !body.body.is_empty() {
            for p in &mut params {
                if p.ident.sym != "_" && p.ident.sym != "self" && p.ident.sym != "this" {
                    if let Type::BorrowType {
                        inner,
                        is_mut: true,
                        ..
                    } = &p.ty
                    {
                        if Self::block_assigns_ident_directly(&body, &p.ident.sym)
                            && !Self::block_derefs_ident(&body, &p.ident.sym)
                        {
                            p.ty = (**inner).clone();
                            p.is_mut = true;
                        }
                    }
                }
            }
        }

        Ok(Function {
            span: Span { start, end },
            ident,
            generic_params,
            params,
            return_type,
            body,
            is_async,
            is_static,
            is_unsafe,
            meta,
            is_pub,
        })
    }

    pub(super) fn parse_zig_fn_qualifiers(&mut self, meta: &mut Vec<MetaItem>) -> ParseResult<()> {
        loop {
            let is_qual = matches!(
                self.peek(),
                Some(Token::Ident(s)) if matches!(s.as_str(), "callconv" | "align" | "linksection" | "addrspace")
            ) && self.peek_next() == Some(&Token::LParen);
            if !is_qual {
                break;
            }
            let start = self.peek_span().start;
            let item = self.parse_single_meta_inner(start)?;
            meta.push(item);
        }
        Ok(())
    }

    pub(super) fn block_assigns_ident_directly(block: &BlockStmt, target: &str) -> bool {
        block
            .body
            .iter()
            .any(|s| Self::stmt_assigns_ident_directly(s, target))
    }

    pub(super) fn stmt_assigns_ident_directly(stmt: &Stmt, target: &str) -> bool {
        match stmt {
            Stmt::Expr(e) => Self::expr_assigns_ident_directly(e, target),
            Stmt::Block(b)
            | Stmt::UnsafeBlock(b)
            | Stmt::ArenaBlock(b)
            | Stmt::ComptimeBlock(b) => Self::block_assigns_ident_directly(b, target),
            Stmt::If(i) => {
                Self::expr_assigns_ident_directly(&i.test, target)
                    || Self::stmt_assigns_ident_directly(&i.cons, target)
                    || i.alt
                        .as_ref()
                        .map_or(false, |a| Self::stmt_assigns_ident_directly(a, target))
            }
            Stmt::While(w) => {
                Self::expr_assigns_ident_directly(&w.test, target)
                    || w.continue_expr
                        .as_ref()
                        .map_or(false, |c| Self::expr_assigns_ident_directly(c, target))
                    || Self::stmt_assigns_ident_directly(&w.body, target)
            }
            Stmt::For(f) => Self::stmt_assigns_ident_directly(&f.body, target),
            Stmt::Match(m) => m
                .arms
                .iter()
                .any(|a| Self::stmt_assigns_ident_directly(&a.body, target)),
            Stmt::Defer(s) | Stmt::ErrDefer(s) => Self::stmt_assigns_ident_directly(s, target),
            Stmt::TryCatch {
                try_block,
                catch_body,
                ..
            } => {
                Self::block_assigns_ident_directly(try_block, target)
                    || catch_body
                        .as_ref()
                        .map_or(false, |c| Self::stmt_assigns_ident_directly(c, target))
            }
            _ => false,
        }
    }

    pub(super) fn expr_assigns_ident_directly(expr: &Expr, target: &str) -> bool {
        match expr {
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Assign {
                    if let Expr::Ident(id) = &*bin.left {
                        if id.sym == target {
                            return true;
                        }
                    }
                }
                Self::expr_assigns_ident_directly(&bin.left, target)
                    || Self::expr_assigns_ident_directly(&bin.right, target)
            }
            Expr::LabeledBlock { block, .. }
            | Expr::Unsafe(block)
            | Expr::Arena(block)
            | Expr::Comptime(block) => Self::block_assigns_ident_directly(block, target),
            _ => false,
        }
    }

    pub(super) fn block_derefs_ident(block: &BlockStmt, target: &str) -> bool {
        block
            .body
            .iter()
            .any(|s| Self::stmt_derefs_ident(s, target))
    }

    pub(super) fn stmt_derefs_ident(stmt: &Stmt, target: &str) -> bool {
        match stmt {
            Stmt::Expr(e) => Self::expr_derefs_ident(e, target),
            Stmt::Var(v) => v
                .init
                .as_ref()
                .map_or(false, |e| Self::expr_derefs_ident(e, target)),
            Stmt::Return(r) => r
                .arg
                .as_ref()
                .map_or(false, |e| Self::expr_derefs_ident(e, target)),
            Stmt::Block(b)
            | Stmt::UnsafeBlock(b)
            | Stmt::ArenaBlock(b)
            | Stmt::ComptimeBlock(b) => Self::block_derefs_ident(b, target),
            Stmt::If(i) => {
                Self::expr_derefs_ident(&i.test, target)
                    || Self::stmt_derefs_ident(&i.cons, target)
                    || i.alt
                        .as_ref()
                        .map_or(false, |a| Self::stmt_derefs_ident(a, target))
            }
            Stmt::While(w) => {
                Self::expr_derefs_ident(&w.test, target) || Self::stmt_derefs_ident(&w.body, target)
            }
            Stmt::For(f) => Self::stmt_derefs_ident(&f.body, target),
            Stmt::Match(m) => m
                .arms
                .iter()
                .any(|a| Self::stmt_derefs_ident(&a.body, target)),
            _ => false,
        }
    }

    pub(super) fn expr_derefs_ident(expr: &Expr, target: &str) -> bool {
        match expr {
            Expr::Deref(inner) => {
                if let Expr::Ident(id) = &**inner {
                    if id.sym == target {
                        return true;
                    }
                }
                Self::expr_derefs_ident(inner, target)
            }
            Expr::Bin(bin) => {
                Self::expr_derefs_ident(&bin.left, target)
                    || Self::expr_derefs_ident(&bin.right, target)
            }
            Expr::Call(c) | Expr::OptCall(c) | Expr::New(c) => {
                Self::expr_derefs_ident(&c.callee, target)
                    || c.args.iter().any(|a| Self::expr_derefs_ident(a, target))
            }
            Expr::Member(m) | Expr::OptMember(m) => Self::expr_derefs_ident(&m.obj, target),
            Expr::Index { obj, index } => {
                Self::expr_derefs_ident(obj, target) || Self::expr_derefs_ident(index, target)
            }
            Expr::LabeledBlock { block, .. }
            | Expr::Unsafe(block)
            | Expr::Arena(block)
            | Expr::Comptime(block) => Self::block_derefs_ident(block, target),
            _ => false,
        }
    }

    pub(super) fn block_mutates_receiver(block: &BlockStmt) -> bool {
        block.body.iter().any(Self::stmt_mutates_receiver)
    }

    pub(super) fn stmt_mutates_receiver(stmt: &Stmt) -> bool {
        match stmt {
            Stmt::Expr(e) => Self::expr_mutates_receiver(e),
            Stmt::Block(b)
            | Stmt::UnsafeBlock(b)
            | Stmt::ArenaBlock(b)
            | Stmt::ComptimeBlock(b) => Self::block_mutates_receiver(b),
            Stmt::If(i) => {
                Self::stmt_mutates_receiver(&i.cons)
                    || i.alt
                        .as_ref()
                        .map_or(false, |a| Self::stmt_mutates_receiver(a))
            }
            Stmt::While(w) => Self::stmt_mutates_receiver(&w.body),
            Stmt::For(f) => Self::stmt_mutates_receiver(&f.body),
            Stmt::Match(m) => m.arms.iter().any(|a| Self::stmt_mutates_receiver(&a.body)),
            _ => false,
        }
    }

    pub(super) fn expr_is_receiver_root(expr: &Expr) -> bool {
        match expr {
            Expr::Ident(id) => id.sym == "_" || id.sym == "self" || id.sym == "this",
            Expr::Deref(inner) => Self::expr_is_receiver_root(inner),
            Expr::Member(m) => Self::expr_is_receiver_root(&m.obj),
            Expr::Index { obj, .. } => Self::expr_is_receiver_root(obj),
            _ => false,
        }
    }

    pub(super) fn expr_mutates_receiver(expr: &Expr) -> bool {
        match expr {
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Assign && Self::expr_is_receiver_root(&bin.left) {
                    return true;
                }
                Self::expr_mutates_receiver(&bin.left) || Self::expr_mutates_receiver(&bin.right)
            }
            Expr::LabeledBlock { block, .. }
            | Expr::Unsafe(block)
            | Expr::Arena(block)
            | Expr::Comptime(block) => Self::block_mutates_receiver(block),
            _ => false,
        }
    }

    pub(super) fn block_find_ident_span(block: &BlockStmt, target: &str) -> Option<(String, Span)> {
        for s in &block.body {
            if let Some(found) = Self::stmt_find_ident_span(s, target) {
                return Some(found);
            }
        }
        None
    }

    pub(super) fn stmt_find_ident_span(stmt: &Stmt, target: &str) -> Option<(String, Span)> {
        match stmt {
            Stmt::Expr(e) => Self::expr_find_ident_span(e, target),
            Stmt::Var(v) => v
                .init
                .as_ref()
                .and_then(|e| Self::expr_find_ident_span(e, target)),
            Stmt::Return(r) => r
                .arg
                .as_ref()
                .and_then(|e| Self::expr_find_ident_span(e, target)),
            Stmt::Block(b)
            | Stmt::UnsafeBlock(b)
            | Stmt::ArenaBlock(b)
            | Stmt::ComptimeBlock(b) => Self::block_find_ident_span(b, target),
            Stmt::If(i) => Self::expr_find_ident_span(&i.test, target)
                .or_else(|| Self::stmt_find_ident_span(&i.cons, target))
                .or_else(|| {
                    i.alt
                        .as_ref()
                        .and_then(|a| Self::stmt_find_ident_span(a, target))
                }),
            Stmt::While(w) => Self::expr_find_ident_span(&w.test, target)
                .or_else(|| Self::stmt_find_ident_span(&w.body, target)),
            Stmt::For(f) => Self::expr_find_ident_span(&f.expr, target)
                .or_else(|| Self::stmt_find_ident_span(&f.body, target)),
            _ => None,
        }
    }

    pub(super) fn expr_find_ident_span(expr: &Expr, target: &str) -> Option<(String, Span)> {
        match expr {
            Expr::Ident(id) if id.sym == target => Some((id.sym.clone(), id.span)),
            Expr::Member(m) | Expr::OptMember(m) => Self::expr_find_ident_span(&m.obj, target),
            Expr::Bin(bin) => Self::expr_find_ident_span(&bin.left, target)
                .or_else(|| Self::expr_find_ident_span(&bin.right, target)),
            Expr::Call(c) | Expr::OptCall(c) | Expr::New(c) => {
                Self::expr_find_ident_span(&c.callee, target).or_else(|| {
                    c.args
                        .iter()
                        .find_map(|a| Self::expr_find_ident_span(a, target))
                })
            }
            Expr::Index { obj, index } => Self::expr_find_ident_span(obj, target)
                .or_else(|| Self::expr_find_ident_span(index, target)),
            Expr::Deref(inner) | Expr::Unwrap(inner) | Expr::Question(inner) => {
                Self::expr_find_ident_span(inner, target)
            }
            _ => None,
        }
    }

    pub fn parse_generic_params(&mut self) -> ParseResult<Vec<GenericParam>> {
        let mut generic_params = Vec::new();
        if self.peek() == Some(&Token::Less) {
            self.bump()?;
            while self.peek() != Some(&Token::Greater) && self.peek().is_some() {
                if self.peek() == Some(&Token::Const) {
                    self.bump()?;
                    let p_ident = self.parse_ident()?;
                    self.expect(Token::Colon)?;
                    let ty = self.parse_type()?;
                    generic_params.push(GenericParam::Const { ident: p_ident, ty });
                } else {
                    let p_ident = self.parse_ident()?;
                    let mut bounds = Vec::new();
                    if self.peek() == Some(&Token::Colon) {
                        self.bump()?;
                        loop {
                            let bound_ident = self.parse_ident()?;
                            bounds.push(bound_ident);
                            if self.peek() == Some(&Token::Plus) {
                                self.bump()?;
                            } else {
                                break;
                            }
                        }
                    }
                    generic_params.push(GenericParam::Type {
                        ident: p_ident,
                        bounds,
                    });
                }
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
            }
            self.expect(Token::Greater)?;
        }
        Ok(generic_params)
    }

    pub(super) fn parse_params(&mut self) -> ParseResult<Vec<Param>> {
        self.expect(Token::LParen)?;
        let mut params = Vec::new();
        while self.peek() != Some(&Token::RParen) {
            let meta = self.parse_meta_list()?;
            if self.peek() == Some(&Token::DotDotDot) {
                let dot_span = self.bump()?.1;
                params.push(Param {
                    span: dot_span,
                    ident: Ident {
                        span: dot_span,
                        sym: "...".to_string(),
                    },
                    ty: Type::Any,
                    is_comptime: false,
                    is_mut: false,
                    is_move: false,
                    default: None,
                    meta,
                });
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
                continue;
            }
            let mut is_mut = false;
            let mut is_move = false;
            let mut is_comptime = meta.iter().any(|m| m.name.sym == "comptime");
            if matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime") {
                self.bump()?;
                is_comptime = true;
            }

            let mut had_prefix_mut_or_let = false;
            if self.peek() == Some(&Token::Mut) {
                let (_, mut_span) = self.bump()?;
                is_mut = true;
                had_prefix_mut_or_let = true;
                self.push_error(
                    Diagnostic::new("E0104", "abolished keyword 'mut' in parameter declaration")
                        .with_span(mut_span)
                        .with_cause("'mut' is Rust legacy syntax and is abolished in CP.")
                        .with_help("in CP, use '*_' for mutable method receivers, or 'param: *T' for mutable parameters (compiler automatically deduces value copy vs mutable reference).")
                        .with_suggestion("replace 'mut _' with '*_', or 'mut x: T' with 'x: *T'"),
                );
            } else if self.peek() == Some(&Token::Let) {
                let (_, let_span) = self.bump()?;
                is_mut = true;
                had_prefix_mut_or_let = true;
                self.push_error(
                    Diagnostic::new("E0104", "'let' modifier is not allowed in function parameters")
                        .with_span(let_span)
                        .with_cause("'let' is only used for mutable local variables inside function bodies.")
                        .with_help("in CP, declare mutable parameters using 'param: *T' (compiler automatically deduces value copy vs mutable reference).")
                        .with_suggestion("replace 'let x: T' with 'x: *T'"),
                );
            } else if self.peek() == Some(&Token::Move) || self.peek() == Some(&Token::Own) {
                self.bump()?;
                is_move = true;
            } else if self.peek() == Some(&Token::Star) {
                let (_, star_span) = self.bump()?;
                is_mut = true;
                let p_ident = self.parse_ident()?;
                if p_ident.sym != "_"
                    && p_ident.sym != "this"
                    && p_ident.sym != "self"
                    && p_ident.sym != "next"
                    && p_ident.sym != "target"
                {
                    self.push_error(
                        Diagnostic::new(
                            "E0125",
                            format!("prefix '*' on named parameter '*{}' is not allowed", p_ident.sym),
                        )
                        .with_span(Span {
                            start: star_span.start,
                            end: p_ident.span.end,
                        })
                        .with_cause("prefix '*_' is reserved exclusively for container method receivers; named parameters must place '*T' in the type position.")
                        .with_help("write 'param: *T' for mutable parameters (compiler automatically deduces value copy vs mutable reference).")
                        .with_suggestion(format!("change '*{}: T' to '{}: *T'", p_ident.sym, p_ident.sym)),
                    );
                }
                let mut ty = if (p_ident.sym == "this"
                    || p_ident.sym == "_"
                    || p_ident.sym == "next"
                    || p_ident.sym == "target")
                    && self.peek() != Some(&Token::Colon)
                {
                    Type::Any
                } else if self.peek() == Some(&Token::Colon) {
                    self.bump()?;
                    self.parse_type()?
                } else {
                    Type::Any
                };
                if p_ident.sym != "this"
                    && p_ident.sym != "_"
                    && !matches!(
                        ty,
                        Type::BorrowType { .. } | Type::RawPtr { .. } | Type::Any
                    )
                {
                    ty = Type::BorrowType {
                        inner: Box::new(ty),
                        is_mut: true,
                        lifetimes: vec![],
                    };
                }
                let default = if self.peek() == Some(&Token::Assign) {
                    self.bump()?;
                    Some(self.parse_primary_expr()?)
                } else {
                    None
                };
                params.push(Param {
                    span: p_ident.span,
                    ident: p_ident,
                    ty,
                    is_comptime,
                    is_mut,
                    is_move,
                    default,
                    meta,
                });
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
                continue;
            } else if self.peek() == Some(&Token::Ampersand) {
                self.bump()?;
                if self.peek() == Some(&Token::Mut) {
                    self.bump()?;
                    is_mut = true;
                }
                let p_ident = self.parse_ident()?;
                let mut ty = if (p_ident.sym == "this"
                    || p_ident.sym == "_"
                    || p_ident.sym == "next"
                    || p_ident.sym == "target")
                    && self.peek() != Some(&Token::Colon)
                {
                    Type::Any
                } else if self.peek() == Some(&Token::Colon) {
                    self.bump()?;
                    self.parse_type()?
                } else {
                    Type::Any
                };
                if p_ident.sym != "this"
                    && p_ident.sym != "_"
                    && !matches!(
                        ty,
                        Type::BorrowType { .. } | Type::RawPtr { .. } | Type::Any
                    )
                {
                    ty = Type::BorrowType {
                        inner: Box::new(ty),
                        is_mut,
                        lifetimes: vec![],
                    };
                }
                let default = if self.peek() == Some(&Token::Assign) {
                    self.bump()?;
                    Some(self.parse_primary_expr()?)
                } else {
                    None
                };
                params.push(Param {
                    span: p_ident.span,
                    ident: p_ident,
                    ty,
                    is_comptime,
                    is_mut,
                    is_move,
                    default,
                    meta,
                });
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
                continue;
            }

            let p_ident = self.parse_ident()?;
            if p_ident.sym == "_"
                && !is_mut
                && !is_move
                && !had_prefix_mut_or_let
                && self.peek() != Some(&Token::Colon)
            {
                self.push_error(
                    Diagnostic::new("E0123", "redundant read-only receiver '_' in method parameter list")
                        .with_span(p_ident.span)
                        .with_cause("in CP, instance methods have read-only receiver '_' by default when omitted.")
                        .with_help("omit '_' from the parameter list for read-only methods (e.g. 'fn foo()'), or use '*_' if the method mutates the instance.")
                        .with_suggestion("remove '_' from the parameter list"),
                );
            }
            let ty = if (p_ident.sym == "this"
                || p_ident.sym == "_"
                || p_ident.sym == "next"
                || p_ident.sym == "target")
                && self.peek() != Some(&Token::Colon)
            {
                Type::Any
            } else if self.peek() == Some(&Token::Colon) {
                self.bump()?;
                if self.peek() == Some(&Token::Move) || self.peek() == Some(&Token::Own) {
                    let (_, mv_span) = self.bump()?;
                    is_move = true;
                    self.push_error(
                        Diagnostic::new("E0125", "'move'/'own' modifier must be placed before the parameter name")
                            .with_span(mv_span)
                            .with_cause("'move' and 'own' are ownership transfer modifiers, not part of the type.")
                            .with_help("place 'move' or 'own' before the parameter name: 'move v: T' or 'own v: *T'.")
                            .with_suggestion(format!("change '{}: move T' to 'move {}: T'", p_ident.sym, p_ident.sym)),
                    );
                }
                self.parse_type()?
            } else {
                Type::Any
            };
            if matches!(ty, Type::BorrowType { is_mut: true, .. }) {
                is_mut = true;
            }
            let default = if self.peek() == Some(&Token::Assign) {
                self.bump()?;
                Some(self.parse_primary_expr()?)
            } else {
                None
            };
            params.push(Param {
                span: p_ident.span,
                ident: p_ident,
                ty,
                is_comptime,
                is_mut,
                is_move,
                meta,
                default,
            });
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
            }
        }
        self.expect(Token::RParen)?;
        Ok(params)
    }
}
