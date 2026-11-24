use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

struct ClassMemberAccum {
    fields: Vec<Field>,
    methods: Vec<Function>,
    extra_zig: Vec<String>,
    nested_classes: Vec<Class>,
    test_blocks: Vec<ContainerTest>,
    is_ref: bool,
}

impl<'a> Parser<'a> {
    pub(super) fn parse_class(&mut self) -> ParseResult<Class> {
        let start = self.peek_span().start;
        let mut is_ref = false;
        let mut _is_struct = false;
        let mut is_static = false;
        let mut is_union = false;
        let mut class_meta = Vec::new();
        loop {
            if self.peek() == Some(&Token::Static) {
                self.bump()?;
                is_static = true;
            } else if self.peek() == Some(&Token::Ref) {
                self.bump()?;
                is_ref = true;
            } else {
                break;
            }
        }
        if self.peek() == Some(&Token::Union) {
            self.bump()?;
            is_union = true;
            if self.peek() == Some(&Token::LParen) {
                self.bump()?;
                let tag_expr = if self.peek() == Some(&Token::Enum) {
                    let span = self.bump()?.1;
                    Expr::Ident(Ident {
                        span,
                        sym: "enum".to_string(),
                    })
                } else {
                    self.parse_expr()?
                };
                self.expect(Token::RParen)?;
                class_meta.push(MetaItem {
                    span: left_span(&tag_expr),
                    name: Ident {
                        span: left_span(&tag_expr),
                        sym: "tag".to_string(),
                    },
                    args: vec![MetaArg::Positional(tag_expr)],
                });
            }
        } else if self.peek() == Some(&Token::Struct) {
            let span = self.bump()?.1;
            _is_struct = true;
            if !is_ref {
                class_meta.push(MetaItem {
                    span,
                    name: Ident {
                        span,
                        sym: "value".to_string(),
                    },
                    args: vec![],
                });
            }
        } else {
            self.expect(Token::Class)?;
        }
        let ident = self.parse_ident()?;
        let generic_params = self.parse_generic_params()?;

        let mut extends = None;
        let mut implements = Vec::new();
        if self.peek() == Some(&Token::Extends) {
            self.bump()?;
            let parent = self.parse_ident()?;
            if is_static {
                return Err(ParseError::Generic(
                    format!(
                        "Static class or struct '{}' cannot extend another class",
                        ident.sym
                    ),
                    parent.span,
                ));
            }
            if parent.sym == "Agent" {
                implements.push(parent.clone());
            } else {
                extends = Some(parent);
            }
        }
        if self.peek() == Some(&Token::Implements) {
            self.bump()?;
            implements.push(self.parse_ident()?);
            while self.peek() == Some(&Token::Comma) {
                self.bump()?;
                implements.push(self.parse_ident()?);
            }
        }

        let mut acc = ClassMemberAccum {
            fields: Vec::new(),
            methods: Vec::new(),
            extra_zig: Vec::new(),
            nested_classes: Vec::new(),
            test_blocks: Vec::new(),
            is_ref,
        };
        self.expect(Token::LBrace)?;
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            self.parse_class_member_item(&ident, is_static, false, true, is_union, &mut acc)?;
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(Class {
            span: Span { start, end },
            ident,
            fields: acc.fields,
            methods: acc.methods,
            implements,
            extends,
            is_ref: acc.is_ref,
            meta: class_meta,
            extra_zig: acc.extra_zig,
            is_pub: false,
            is_static,
            is_union,
            nested_classes: acc.nested_classes,
            generic_params,
            test_blocks: acc.test_blocks,
        })
    }

    fn parse_class_member_item(
        &mut self,
        ident: &Ident,
        is_static_container: bool,
        in_static_block: bool,
        default_member_pub: bool,
        is_union: bool,
        acc: &mut ClassMemberAccum,
    ) -> ParseResult<()> {
        if self.peek() == Some(&Token::ZigEscape) {
            let (_, code) = self.parse_raw_zig_block()?;
            acc.extra_zig.push(code);
            return Ok(());
        }
        let mut meta_list = self.parse_meta_list()?;
        let mut member_is_pub = default_member_pub;
        let mut has_pub_or_priv = false;
        while self.peek() == Some(&Token::Export)
            || self.peek() == Some(&Token::Pub)
            || self.peek() == Some(&Token::Private)
        {
            has_pub_or_priv = true;
            if self.peek() == Some(&Token::Export) || self.peek() == Some(&Token::Pub) {
                member_is_pub = true;
            } else if self.peek() == Some(&Token::Private) {
                member_is_pub = false;
            }
            self.bump()?;
        }
        if self.peek() == Some(&Token::Static) && self.peek_next() == Some(&Token::LBrace) {
            if !meta_list.is_empty() {
                return Err(ParseError::Generic(
                    "Metadata attributes cannot be attached directly to a 'static { ... }' block; place attributes on individual members inside the block".to_string(),
                    meta_list[0].span,
                ));
            }
            self.bump()?; // consume `static`
            self.bump()?; // consume `{`
            let block_default_pub = if has_pub_or_priv {
                member_is_pub
            } else {
                default_member_pub
            };
            while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                self.parse_class_member_item(
                    ident,
                    is_static_container,
                    true,
                    block_default_pub,
                    is_union,
                    acc,
                )?;
            }
            self.expect(Token::RBrace)?;
            if self.peek() == Some(&Token::Semicolon) {
                self.bump()?;
            }
            return Ok(());
        }
        let effective_static = is_static_container || in_static_block;
        let mut is_field_static = effective_static
            || meta_list
                .iter()
                .any(|m| m.name.sym == "tls" || m.name.sym == "threadlocal");
        if self.peek() == Some(&Token::Static) {
            if !matches!(
                self.peek_next(),
                Some(Token::Fn) | Some(Token::Async) | Some(Token::Colon)
            ) && !matches!(self.peek_next(), Some(Token::Ident(id)) if id == "inline" || id == "extern" || id == "noinline")
            {
                self.bump()?;
                is_field_static = true;
            }
        }
        if self.peek_next() != Some(&Token::Colon)
            && matches!(
                self.peek(),
                Some(Token::Struct | Token::Union | Token::Class)
            )
        {
            let mut nested = self.parse_class()?;
            nested.meta.extend(meta_list);
            nested.is_pub = member_is_pub;
            nested.is_static = true;
            acc.nested_classes.push(nested);
            return Ok(());
        }
        let is_test_decl = matches!(self.peek(), Some(Token::Ident(id)) if id == "test")
            && matches!(
                self.peek_next(),
                Some(Token::LBrace) | Some(Token::StringLiteral(_)) | Some(Token::Ident(_))
            );
        if is_test_decl {
            acc.test_blocks.push(self.parse_container_test_decl()?);
            return Ok(());
        }
        let is_comptime_block = matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime")
            && self.peek_next() == Some(&Token::LBrace);
        let is_brace_terminated_decl = is_comptime_block;
        if self.peek_next() != Some(&Token::Colon)
            && (self.peek() == Some(&Token::Const)
                || self.peek() == Some(&Token::Var)
                || self.peek() == Some(&Token::ThreadLocal)
                || self.peek() == Some(&Token::TypeKw)
                || self.peek() == Some(&Token::Enum)
                || self.peek() == Some(&Token::Interface)
                || self.peek() == Some(&Token::Trait)
                || is_comptime_block
                || matches!(self.peek(), Some(Token::Ident(id)) if id == "usingnamespace"))
        {
            let start_idx = self.peek_span().start;
            let mut brace_depth: i32 = 0;
            let mut paren_depth: i32 = 0;
            let mut bracket_depth: i32 = 0;
            let mut had_brace = false;
            let mut end_idx = start_idx;
            while let Some(tok) = self.peek() {
                match tok {
                    Token::LBrace => {
                        brace_depth += 1;
                        had_brace = true;
                    }
                    Token::RBrace => {
                        if brace_depth == 0 {
                            break;
                        }
                        brace_depth -= 1;
                        if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 && had_brace {
                            let span = self.bump()?.1;
                            end_idx = span.end;
                            if is_brace_terminated_decl {
                                if self.peek() == Some(&Token::Semicolon) {
                                    let s_span = self.bump()?.1;
                                    end_idx = s_span.end;
                                }
                                break;
                            }
                            while let Some(tok) = self.peek() {
                                if *tok == Token::Semicolon {
                                    let s_span = self.bump()?.1;
                                    end_idx = s_span.end;
                                    break;
                                }
                                if matches!(tok, Token::RBrace) {
                                    break;
                                }
                                let s_span = self.bump()?.1;
                                end_idx = s_span.end;
                            }
                            break;
                        }
                    }
                    Token::LParen => paren_depth += 1,
                    Token::RParen => {
                        if paren_depth > 0 {
                            paren_depth -= 1;
                        }
                    }
                    Token::LBracket => bracket_depth += 1,
                    Token::RBracket => {
                        if bracket_depth > 0 {
                            bracket_depth -= 1;
                        }
                    }
                    Token::Semicolon => {
                        if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                            let span = self.bump()?.1;
                            end_idx = span.end;
                            break;
                        }
                    }
                    _ => {}
                }
                let span = self.bump()?.1;
                end_idx = span.end;
            }
            let raw_code = &self.input[start_idx..end_idx];
            let pub_prefix = if member_is_pub
                && (has_pub_or_priv || in_static_block)
                && !is_brace_terminated_decl
            {
                "pub "
            } else {
                ""
            };
            acc.extra_zig
                .push(format!("{}{}", pub_prefix, raw_code.trim()));
            return Ok(());
        }
        if self.peek() == Some(&Token::Let) {
            self.bump()?;
        }
        if self.peek() == Some(&Token::Tilde) {
            let tilde_span = self.bump()?.1;
            let dtor_ident = self.parse_ident()?;
            let (dtor_sym, dtor_label) = match dtor_ident.sym.as_str() {
                "init" => ("__cp_deinit", "~init()"),
                "errinit" => ("__cp_errdeinit", "~errinit()"),
                _ => {
                    return Err(ParseError::Generic(
                        format!(
                            "CP Syntax Error: destructor must be named '~init()' or '~errinit()', found '~{}'\n  = cause: CP uses '~init()' (unconditional destructor) and '~errinit()' (error-path destructor) to avoid collisions with business method names.\n  = help: declare the destructor as '~init() {{ ... }}' or '~errinit() {{ ... }}'.\n  = suggestion: rename '~{}' to '~init' or '~errinit'",
                            dtor_ident.sym, dtor_ident.sym
                        ),
                        dtor_ident.span,
                    ));
                }
            };
            if effective_static {
                let ctx_desc = if in_static_block {
                    "Static block in"
                } else {
                    "Static class or struct"
                };
                return Err(ParseError::Generic(
                    format!(
                        "{} '{}' cannot declare destructor '{}'",
                        ctx_desc, ident.sym, dtor_label
                    ),
                    tilde_span,
                ));
            }
            if acc
                .methods
                .iter()
                .any(|m: &Function| m.ident.sym == dtor_sym)
            {
                return Err(ParseError::Generic(
                    format!("Duplicate destructor '{}' in '{}'", dtor_label, ident.sym),
                    tilde_span,
                ));
            }
            self.expect(Token::LParen)?;
            if self.peek() != Some(&Token::RParen) {
                let bad_span = self.peek_span();
                return Err(ParseError::Generic(
                    format!(
                        "CP Syntax Error: destructor '{}' takes no parameters.\n  = cause: '{}' implicitly receives mutable exclusive access to '_' and cannot accept explicit parameters.\n  = help: remove all parameters from '{}'.\n  = suggestion: {} {{ ... }}",
                        dtor_label, dtor_label, dtor_label, dtor_label
                    ),
                    bad_span,
                ));
            }
            self.expect(Token::RParen)?;
            if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
                let bad_span = self.peek_span();
                return Err(ParseError::Generic(
                    format!(
                        "CP Syntax Error: destructor '{}' cannot declare a return type.\n  = cause: destructors always return void and cannot fail.\n  = help: remove the return type annotation from '{}'.\n  = suggestion: {} {{ ... }}",
                        dtor_label, dtor_label, dtor_label
                    ),
                    bad_span,
                ));
            }
            let body = self.parse_block_stmt()?;
            let end_span = body.span.end;
            let recv_param = Param {
                span: tilde_span,
                ident: Ident {
                    span: tilde_span,
                    sym: "_".to_string(),
                },
                ty: Type::BorrowType {
                    inner: Box::new(Type::Ref(ident.clone())),
                    is_mut: true,
                    lifetimes: vec![],
                },
                is_comptime: false,
                is_noalias: false,
                is_mut: true,
                is_move: false,
                meta: vec![],
                default: None,
            };
            let dtor_fn = Function {
                span: Span {
                    start: tilde_span.start,
                    end: end_span,
                },
                ident: Ident {
                    span: Span {
                        start: tilde_span.start,
                        end: dtor_ident.span.end,
                    },
                    sym: dtor_sym.to_string(),
                },
                generic_params: vec![],
                params: vec![recv_param],
                return_type: None,
                body,
                is_async: false,
                is_static: false,
                is_unsafe: false,
                meta: meta_list,
                is_pub: true,
            };
            acc.methods.push(dtor_fn);
            return Ok(());
        }
        let is_method = match self.peek() {
            Some(Token::Fn) | Some(Token::Async) => true,
            Some(Token::Static) => {
                matches!(self.peek_next(), Some(Token::Fn) | Some(Token::Async))
                    || matches!(self.peek_next(), Some(Token::Ident(id)) if id == "inline" || id == "extern" || id == "noinline")
            }
            Some(Token::Ident(id)) if id == "inline" || id == "extern" || id == "noinline" => {
                matches!(
                    self.peek_next(),
                    Some(Token::Fn)
                        | Some(Token::Async)
                        | Some(Token::Static)
                        | Some(Token::Unsafe)
                )
            }
            _ => false,
        };
        if is_method {
            let mut method = self.parse_function()?;
            if effective_static {
                let ctx_desc = if in_static_block {
                    "Static block in"
                } else {
                    "Static class or struct"
                };
                if let Some(first) = method.params.first() {
                    if first.ident.sym == "this" || first.ident.sym == "_" {
                        return Err(ParseError::Generic(
                            format!(
                                "{} '{}' cannot declare instance method '{}' with receiver 'this' or '_'",
                                ctx_desc, ident.sym, method.ident.sym
                            ),
                            first.span,
                        ));
                    }
                }
                if method.ident.sym == "init" || method.ident.sym == "release" {
                    return Err(ParseError::Generic(
                        format!(
                            "{} '{}' cannot declare instance lifecycle method '{}'",
                            ctx_desc, ident.sym, method.ident.sym
                        ),
                        method.span,
                    ));
                }
                method.is_static = true;
            }
            method.meta.extend(meta_list);
            let is_extern_fn = method.meta.iter().any(|m| m.name.sym == "extern");
            let is_c_fn = method.meta.iter().any(|m| m.name.sym == "c");
            let has_explicit_receiver = method.params.first().map_or(false, |p| {
                p.ident.sym == "this"
                    || p.ident.sym == "self"
                    || (p.ident.sym == "_"
                        && (matches!(p.ty, Type::Auto | Type::Any)
                            || (!method.body.body.is_empty()
                                && matches!(&p.ty, Type::Ref(id) if id.sym == ident.sym || id.sym == "This" || id.sym == "Self" || id.sym == "@This()"))))
            });
            if is_extern_fn || (is_c_fn && !has_explicit_receiver) {
                method.is_static = true;
            }
            method.is_pub = member_is_pub;
            acc.methods.push(method);
        } else {
            let f_ident = self.parse_ident()?;
            let ty = if is_union
                && matches!(
                    self.peek(),
                    Some(Token::Comma | Token::RBrace | Token::Semicolon)
                ) {
                Type::Void
            } else {
                self.expect(Token::Colon)?;
                self.parse_type()?
            };
            if matches!(ty, Type::BorrowType { .. }) {
                acc.is_ref = true;
            }
            if matches!(self.peek(), Some(Token::Ident(s)) if s == "align")
                && self.peek_next() == Some(&Token::LParen)
            {
                let align_ident = self.parse_ident()?;
                self.expect(Token::LParen)?;
                let align_expr = self.parse_expr()?;
                let end_span = self.expect(Token::RParen)?;
                meta_list.push(MetaItem {
                    span: Span {
                        start: align_ident.span.start,
                        end: end_span.end,
                    },
                    name: align_ident,
                    args: vec![MetaArg::Positional(align_expr)],
                });
            }
            let default = if self.peek() == Some(&Token::Assign) {
                self.bump()?;
                Some(self.parse_expr()?)
            } else {
                None
            };
            if self.peek() == Some(&Token::Comma) || self.peek() == Some(&Token::Semicolon) {
                self.bump()?;
            }
            acc.fields.push(Field {
                span: f_ident.span,
                ident: f_ident,
                ty,
                meta: meta_list,
                default,
                is_pub: member_is_pub,
                is_static: is_field_static,
            });
        }
        Ok(())
    }

    pub(super) fn parse_container_test_decl(&mut self) -> ParseResult<ContainerTest> {
        let start_idx = self.peek_span().start;
        self.bump()?; // consume `test`
        if matches!(
            self.peek(),
            Some(Token::StringLiteral(_)) | Some(Token::Ident(_))
        ) {
            self.bump()?;
            while self.peek() == Some(&Token::Dot) {
                self.bump()?;
                if matches!(self.peek(), Some(Token::Ident(_))) {
                    self.bump()?;
                }
            }
        }
        let header_end = self.peek_span().start;
        let header = self.input[start_idx..header_end].trim().to_string();
        let body = self.parse_block_stmt()?;
        let mut end = body.span.end;
        if self.peek() == Some(&Token::Semicolon) {
            end = self.bump()?.1.end;
        }
        Ok(ContainerTest {
            span: Span {
                start: start_idx,
                end,
            },
            header,
            body,
        })
    }

    pub(super) fn parse_trait(&mut self) -> ParseResult<TraitDecl> {
        let start = self.expect(Token::Trait)?.start;
        let ident = self.parse_ident()?;
        self.expect(Token::LBrace)?;
        let mut methods = Vec::new();
        while self.peek() != Some(&Token::RBrace) {
            let m_start = self.peek_span().start;
            let mut is_unsafe = false;
            if self.peek() == Some(&Token::Unsafe) {
                self.bump()?;
                is_unsafe = true;
            }
            self.expect(Token::Fn)?;
            let m_ident = self.parse_ident()?;
            let generic_params = self.parse_generic_params()?;
            let params = self.parse_params()?;
            let mut return_type = None;
            if self.peek() == Some(&Token::Bang) {
                self.bump()?;
                if self.is_error_type_rhs_start() {
                    let err = self.parse_error_type_rhs()?;
                    return_type = Some(Type::ResultType {
                        ok: Box::new(Type::Void),
                        err,
                    });
                } else {
                    return_type = Some(Type::ErrorUnion(Box::new(Type::Void)));
                }
            } else if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
                self.bump()?;
                return_type = Some(self.parse_type()?);
            }
            let default_body = if self.peek() == Some(&Token::LBrace) {
                Some(self.parse_block_stmt()?)
            } else {
                self.expect(Token::Semicolon)?;
                None
            };
            methods.push(MethodSig {
                span: Span {
                    start: m_start,
                    end: self.peek_span().start,
                },
                ident: m_ident,
                generic_params,
                params,
                return_type,
                default_body,
                is_unsafe,
            });
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(TraitDecl {
            span: Span { start, end },
            ident,
            methods,
            meta: Vec::new(),
        })
    }

    pub(super) fn parse_impl(&mut self) -> ParseResult<ImplDecl> {
        let start = self.expect(Token::Impl)?.start;
        let trait_name = self.parse_ident()?;
        self.expect(Token::For)?;
        let target_name = self.parse_ident()?;
        self.expect(Token::LBrace)?;
        let mut methods = Vec::new();
        while self.peek() != Some(&Token::RBrace) {
            methods.push(self.parse_function()?);
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(ImplDecl {
            span: Span { start, end },
            trait_name,
            target_name,
            methods,
        })
    }

    pub(super) fn parse_raw_zig_block(&mut self) -> ParseResult<(Span, String)> {
        let start = self.bump()?.1.start;
        self.expect(Token::LBrace)?;
        let code_start = self.peek_span().start;
        let mut brace_count = 1;
        let mut code_end = code_start;
        while brace_count > 0 && self.peek().is_some() {
            let (t, span) = self.bump()?;
            match t {
                Token::LBrace => brace_count += 1,
                Token::RBrace => brace_count -= 1,
                _ => {}
            }
            if brace_count == 0 {
                code_end = span.start;
            }
        }
        let end = self.peek_span().end;
        let raw_code = self.input[code_start..code_end].trim().to_string();
        Ok((Span { start, end }, raw_code))
    }

    pub(super) fn parse_enum(&mut self) -> ParseResult<EnumDecl> {
        let start = self.expect(Token::Enum)?.start;
        let ident = self.parse_ident()?;
        let mut tag_type = None;
        if self.peek() == Some(&Token::Colon) {
            self.bump()?;
            tag_type = Some(self.parse_type()?);
        } else if self.peek() == Some(&Token::LParen) {
            self.bump()?;
            tag_type = Some(self.parse_type()?);
            self.expect(Token::RParen)?;
        }
        self.expect(Token::LBrace)?;
        let mut variants = Vec::new();
        let mut methods = Vec::new();
        let mut extra_zig = Vec::new();
        let mut test_blocks = Vec::new();
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            if self.peek() == Some(&Token::ZigEscape) {
                let (_, code) = self.parse_raw_zig_block()?;
                extra_zig.push(code);
                continue;
            }
            let mut meta_list = self.parse_meta_list()?;
            let mut method_is_pub = true;
            while self.peek() == Some(&Token::Export)
                || self.peek() == Some(&Token::Pub)
                || self.peek() == Some(&Token::Private)
            {
                if self.peek() == Some(&Token::Export) || self.peek() == Some(&Token::Pub) {
                    method_is_pub = true;
                } else {
                    method_is_pub = false;
                }
                self.bump()?;
            }
            let is_test_decl = matches!(self.peek(), Some(Token::Ident(id)) if id == "test")
                && matches!(
                    self.peek_next(),
                    Some(Token::LBrace) | Some(Token::StringLiteral(_)) | Some(Token::Ident(_))
                );
            if is_test_decl {
                test_blocks.push(self.parse_container_test_decl()?);
                continue;
            }
            let is_comptime_block = matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime")
                && self.peek_next() == Some(&Token::LBrace);
            let is_brace_terminated_decl = is_comptime_block;
            if self.peek_next() != Some(&Token::Colon)
                && (self.peek() == Some(&Token::Const)
                    || self.peek() == Some(&Token::Var)
                    || self.peek() == Some(&Token::ThreadLocal)
                    || self.peek() == Some(&Token::TypeKw)
                    || self.peek() == Some(&Token::Union)
                    || self.peek() == Some(&Token::Struct)
                    || self.peek() == Some(&Token::Enum)
                    || self.peek() == Some(&Token::Class)
                    || self.peek() == Some(&Token::Interface)
                    || self.peek() == Some(&Token::Trait)
                    || is_comptime_block
                    || matches!(self.peek(), Some(Token::Ident(id)) if id == "usingnamespace"))
            {
                let start_idx = self.peek_span().start;
                let mut brace_depth: i32 = 0;
                let mut paren_depth: i32 = 0;
                let mut bracket_depth: i32 = 0;
                let mut had_brace = false;
                let mut end_idx = start_idx;
                while let Some(tok) = self.peek() {
                    match tok {
                        Token::LBrace => {
                            brace_depth += 1;
                            had_brace = true;
                        }
                        Token::RBrace => {
                            if brace_depth == 0 {
                                break;
                            }
                            brace_depth -= 1;
                            if brace_depth == 0
                                && paren_depth == 0
                                && bracket_depth == 0
                                && had_brace
                            {
                                let span = self.bump()?.1;
                                end_idx = span.end;
                                if is_brace_terminated_decl {
                                    if self.peek() == Some(&Token::Semicolon) {
                                        let s_span = self.bump()?.1;
                                        end_idx = s_span.end;
                                    }
                                    break;
                                }
                                while let Some(tok) = self.peek() {
                                    if *tok == Token::Semicolon {
                                        let s_span = self.bump()?.1;
                                        end_idx = s_span.end;
                                        break;
                                    }
                                    if matches!(tok, Token::RBrace) {
                                        break;
                                    }
                                    let s_span = self.bump()?.1;
                                    end_idx = s_span.end;
                                }
                                break;
                            }
                        }
                        Token::LParen => paren_depth += 1,
                        Token::RParen => {
                            if paren_depth > 0 {
                                paren_depth -= 1;
                            }
                        }
                        Token::LBracket => bracket_depth += 1,
                        Token::RBracket => {
                            if bracket_depth > 0 {
                                bracket_depth -= 1;
                            }
                        }
                        Token::Semicolon => {
                            if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                let span = self.bump()?.1;
                                end_idx = span.end;
                                break;
                            }
                        }
                        _ => {}
                    }
                    let span = self.bump()?.1;
                    end_idx = span.end;
                }
                let raw_code = &self.input[start_idx..end_idx];
                let pub_prefix = if method_is_pub && !is_brace_terminated_decl {
                    "pub "
                } else {
                    ""
                };
                extra_zig.push(format!("{}{}", pub_prefix, raw_code.trim()));
                continue;
            }
            if self.peek() == Some(&Token::Static) && self.peek_next() == Some(&Token::LBrace) {
                if !meta_list.is_empty() {
                    return Err(ParseError::Generic(
                        "Metadata attributes cannot be attached directly to a 'static { ... }' block; place attributes on individual members inside the block".to_string(),
                        meta_list[0].span,
                    ));
                }
                self.bump()?; // consume `static`
                self.bump()?; // consume `{`
                while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                    let inner_meta = self.parse_meta_list()?;
                    let mut inner_pub = method_is_pub;
                    while self.peek() == Some(&Token::Export)
                        || self.peek() == Some(&Token::Pub)
                        || self.peek() == Some(&Token::Private)
                    {
                        if self.peek() == Some(&Token::Export) || self.peek() == Some(&Token::Pub) {
                            inner_pub = true;
                        } else {
                            inner_pub = false;
                        }
                        self.bump()?;
                    }
                    let mut method = self.parse_function()?;
                    if let Some(first) = method.params.first() {
                        if first.ident.sym == "this" || first.ident.sym == "_" {
                            return Err(ParseError::Generic(
                                format!(
                                    "Static block in enum '{}' cannot declare instance method '{}' with receiver 'this' or '_'",
                                    ident.sym, method.ident.sym
                                ),
                                first.span,
                            ));
                        }
                    }
                    method.is_static = true;
                    method.meta.extend(inner_meta);
                    if inner_pub {
                        method.is_pub = true;
                    }
                    methods.push(method);
                }
                self.expect(Token::RBrace)?;
                if self.peek() == Some(&Token::Semicolon) {
                    self.bump()?;
                }
                continue;
            }
            let is_method = match self.peek() {
                Some(Token::Fn) | Some(Token::Async) | Some(Token::Static)
                | Some(Token::Unsafe) => true,
                Some(Token::Ident(id)) if id == "inline" || id == "extern" || id == "noinline" => {
                    matches!(
                        self.peek_next(),
                        Some(Token::Fn)
                            | Some(Token::Async)
                            | Some(Token::Static)
                            | Some(Token::Unsafe)
                    )
                }
                _ => false,
            };
            if is_method {
                let mut method = self.parse_function()?;
                method.meta.extend(meta_list);
                let is_extern_fn = method.meta.iter().any(|m| m.name.sym == "extern");
                let is_c_fn = method.meta.iter().any(|m| m.name.sym == "c");
                let has_explicit_receiver = method.params.first().map_or(false, |p| {
                    p.ident.sym == "this"
                        || p.ident.sym == "self"
                        || (p.ident.sym == "_" && matches!(p.ty, Type::Auto | Type::Any))
                });
                if is_extern_fn || (is_c_fn && !has_explicit_receiver) {
                    method.is_static = true;
                }
                if method_is_pub {
                    method.is_pub = true;
                }
                methods.push(method);
                continue;
            }
            let v_ident = self.parse_ident()?;
            let mut fields = None;
            let mut tuple_type = None;
            let mut from_err = None;
            if self.peek() == Some(&Token::Colon) {
                self.bump()?;
                if self.peek() == Some(&Token::LBrace) {
                    self.bump()?;
                    let mut f_list = Vec::new();
                    while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                        let f_ident = self.parse_ident()?;
                        self.expect(Token::Colon)?;
                        let ty = self.parse_type()?;
                        f_list.push(Field {
                            span: f_ident.span,
                            ident: f_ident,
                            ty,
                            meta: Vec::new(),
                            default: None,
                            is_pub: true,
                            is_static: false,
                        });
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    self.expect(Token::RBrace)?;
                    fields = Some(f_list);
                } else {
                    tuple_type = Some(self.parse_type()?);
                }
            } else if self.peek() == Some(&Token::LParen) {
                self.bump()?;
                tuple_type = Some(self.parse_type()?);
                self.expect(Token::RParen)?;
            } else if self.peek() == Some(&Token::LBrace) {
                self.bump()?;
                let mut f_list = Vec::new();
                while self.peek() != Some(&Token::RBrace) {
                    let f_ident = self.parse_ident()?;
                    self.expect(Token::Colon)?;
                    let ty = self.parse_type()?;
                    f_list.push(Field {
                        span: f_ident.span,
                        ident: f_ident,
                        ty,
                        meta: Vec::new(),
                        default: None,
                        is_pub: true,
                        is_static: false,
                    });
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                    }
                }
                self.expect(Token::RBrace)?;
                fields = Some(f_list);
            }
            if self.peek() == Some(&Token::From) {
                self.bump()?;
                from_err = Some(self.parse_ident()?);
            }
            let mut value = None;
            if self.peek() == Some(&Token::Assign) {
                self.bump()?;
                value = Some(self.parse_expr()?);
            }
            variants.push(EnumVariant {
                span: v_ident.span,
                ident: v_ident,
                fields,
                value,
                tuple_type,
                from_err,
            });
            if self.peek() == Some(&Token::Comma) || self.peek() == Some(&Token::Semicolon) {
                self.bump()?;
            }
        }
        let end = self.expect(Token::RBrace)?.end;
        Ok(EnumDecl {
            span: Span { start, end },
            ident,
            variants,
            meta: Vec::new(),
            methods,
            tag_type,
            extra_zig,
            test_blocks,
        })
    }

    pub(super) fn parse_error_set(&mut self) -> ParseResult<ErrorSetDecl> {
        let start = self.expect(Token::ErrorKw)?.start;
        let ident = self.parse_ident()?;
        self.expect(Token::LBrace)?;
        let mut variants = Vec::new();
        while self.peek() != Some(&Token::RBrace) {
            variants.push(self.parse_ident()?);
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
            }
        }
        let end = self.expect(Token::RBrace)?.end;
        let mut unions = Vec::new();
        while self.peek() == Some(&Token::PipePipe) || self.peek() == Some(&Token::Pipe) {
            self.bump()?;
            let mut u_name = self.parse_ident()?.sym;
            while self.peek() == Some(&Token::Dot) {
                self.bump()?;
                let next = self.parse_ident()?;
                u_name.push('.');
                u_name.push_str(&next.sym);
            }
            unions.push(u_name);
        }
        if self.peek() == Some(&Token::Semicolon) {
            self.bump()?;
        }
        Ok(ErrorSetDecl {
            span: Span { start, end },
            ident,
            variants,
            unions,
        })
    }
}
