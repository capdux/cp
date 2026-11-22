use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

impl<'a> Parser<'a> {
    pub(super) fn parse_primary_expr(&mut self) -> ParseResult<Expr> {
        let (token, span) = self.bump()?;
        match token {
            Token::Try => {
                let inner = self.parse_postfix_expr()?;
                Ok(Expr::Question(Box::new(inner)))
            }
            Token::Undefined => Ok(Expr::Lit(Lit::Undefined)),
            Token::Null => Ok(Expr::Lit(Lit::Null)),
            Token::Minus => {
                if let Some(Token::IntLiteral(_)) = self.peek() {
                    if let (Token::IntLiteral(v), _) = self.bump()? {
                        return Ok(Expr::Lit(Lit::Int(-v)));
                    }
                }
                if let Some(Token::FloatLiteral(_)) = self.peek() {
                    if let (Token::FloatLiteral(f), _) = self.bump()? {
                        return Ok(Expr::Lit(Lit::Float(-f)));
                    }
                }
                let inner = self.parse_primary_expr()?;
                Ok(Expr::Bin(BinExpr {
                    span,
                    op: BinaryOp::Sub,
                    left: Box::new(Expr::Lit(Lit::Int(0))),
                    right: Box::new(inner),
                }))
            }
            Token::Dot => {
                if self.peek() == Some(&Token::LBrace) {
                    self.bump()?;
                    let fields = self.parse_object_fields(true)?;
                    return Ok(Expr::Object(fields));
                }
                if self.in_pipeline_rhs > 0 {
                    let is_member_prop = matches!(
                        self.peek(),
                        Some(Token::Ident(_)) | Some(Token::ErrorKw) | Some(Token::Auto)
                    ) || (self.peek().map_or(false, Self::is_ident_token)
                        && self.peek_span().start == span.end);
                    if is_member_prop {
                        let prop = self.parse_ident()?;
                        if prop
                            .sym
                            .trim_start_matches('_')
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_uppercase())
                            && self.peek() == Some(&Token::LBrace)
                        {
                            self.bump()?;
                            let mut fields = Vec::new();
                            while self.peek() != Some(&Token::RBrace) {
                                let key = self.parse_ident()?;
                                self.expect(Token::Colon)?;
                                let val = self.parse_expr()?;
                                fields.push(EnumInitField { key, val });
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                            }
                            let end = self.expect(Token::RBrace)?.end;
                            Ok(Expr::EnumInit(EnumInitExpr {
                                span: Span {
                                    start: span.start,
                                    end,
                                },
                                enum_name: Ident {
                                    span,
                                    sym: String::new(),
                                },
                                variant_name: prop,
                                fields: Some(fields),
                                tuple_args: None,
                            }))
                        } else {
                            Ok(Expr::Member(MemberExpr {
                                span: Span {
                                    start: span.start,
                                    end: prop.span.end,
                                },
                                obj: Box::new(Expr::PipePlaceholder(span)),
                                prop,
                            }))
                        }
                    } else {
                        Ok(Expr::PipePlaceholder(span))
                    }
                } else {
                    if self.peek().map_or(false, Self::is_ident_token) {
                        let variant_name = self.parse_ident()?;
                        let mut fields = None;
                        let mut tuple_args = None;
                        let mut end = variant_name.span.end;
                        if self.peek() == Some(&Token::LBrace) {
                            self.bump()?;
                            let mut f_list = Vec::new();
                            while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                                if self.peek() == Some(&Token::Dot) {
                                    self.bump()?;
                                }
                                let key = self.parse_ident()?;
                                if self.peek() == Some(&Token::Colon)
                                    || self.peek() == Some(&Token::Assign)
                                {
                                    self.bump()?;
                                } else {
                                    self.expect(Token::Colon)?;
                                }
                                let val = self.parse_expr()?;
                                f_list.push(EnumInitField { key, val });
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                            }
                            end = self.expect(Token::RBrace)?.end;
                            fields = Some(f_list);
                        } else if self.peek() == Some(&Token::LBracket) {
                            self.bump()?;
                            let mut args = Vec::new();
                            while self.peek() != Some(&Token::RBracket) && self.peek().is_some() {
                                args.push(self.parse_expr()?);
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                            }
                            end = self.expect(Token::RBracket)?.end;
                            tuple_args = Some(args);
                        } else if self.peek() == Some(&Token::LParen) {
                            self.bump()?;
                            let mut args = Vec::new();
                            while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                                args.push(self.parse_expr()?);
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                            }
                            end = self.expect(Token::RParen)?.end;
                            tuple_args = Some(args);
                        }
                        Ok(Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            enum_name: Ident {
                                span,
                                sym: String::new(),
                            },
                            variant_name,
                            fields,
                            tuple_args,
                        }))
                    } else {
                        return Err(ParseError::Generic(
                            "Pipeline placeholder '.' can only be used on the right-hand side of a pipeline ('|>') operator".to_string(),
                            span,
                        ));
                    }
                }
            }
            Token::Meta => Ok(Expr::Ident(Ident {
                span,
                sym: "meta".to_string(),
            })),
            Token::Super => Ok(Expr::Ident(Ident {
                span,
                sym: "super".to_string(),
            })),
            Token::String => Ok(Expr::Ident(Ident {
                span,
                sym: "string".to_string(),
            })),
            Token::ErrorKw => {
                let text = &self.input[span.start..span.end];
                if self.peek() == Some(&Token::LParen) {
                    self.bump()?;
                    let first = self.parse_expr()?;
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                        let second = self.parse_expr()?;
                        self.expect(Token::RParen)?;
                        return Ok(Expr::ErrWrap(Box::new(Expr::Call(CallExpr {
                            span: Span {
                                start: span.start,
                                end: self.peek_span().start,
                            },
                            callee: Box::new(Expr::Member(MemberExpr {
                                span,
                                obj: Box::new(Expr::Ident(Ident {
                                    span,
                                    sym: "Err".to_string(),
                                })),
                                prop: Ident {
                                    span,
                                    sym: "init".to_string(),
                                },
                            })),
                            args: vec![first, second],
                            is_comptime: false,
                        }))));
                    }
                    self.expect(Token::RParen)?;
                    return Ok(Expr::ErrWrap(Box::new(first)));
                }
                if ((text == "packed" || text == "extern")
                    && (self.peek() == Some(&Token::Struct)
                        || self.peek() == Some(&Token::Union)
                        || self.peek() == Some(&Token::Enum)))
                    || (text == "error" && self.peek() == Some(&Token::LBrace))
                {
                    let start_idx = span.start;
                    let mut brace_depth: i32 = 0;
                    let mut paren_depth: i32 = 0;
                    let mut bracket_depth: i32 = 0;
                    let mut had_brace = false;
                    let mut end_idx = span.end;
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
                                    let s = self.bump()?.1;
                                    end_idx = s.end;
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
                            _ => {}
                        }
                        let s = self.bump()?.1;
                        end_idx = s.end;
                    }
                    let raw_code = self.input[start_idx..end_idx].to_string();
                    return Ok(Expr::Zig(ZigEscapeExpr {
                        span: Span {
                            start: start_idx,
                            end: end_idx,
                        },
                        code: raw_code,
                    }));
                }
                let is_error_literal = (text == "err" || text == "error")
                    && self.peek() == Some(&Token::Dot)
                    && matches!(self.peek_next(), Some(Token::Ident(id)) if id.chars().next().map_or(false, |c| c.is_ascii_uppercase()));
                if is_error_literal {
                    Ok(Expr::Ident(Ident {
                        span,
                        sym: "error".to_string(),
                    }))
                } else {
                    let sym = text.to_string();
                    if sym
                        .trim_start_matches('_')
                        .chars()
                        .next()
                        .map_or(false, |c| c.is_uppercase())
                        && self.peek() == Some(&Token::LBrace)
                    {
                        self.bump()?;
                        let mut fields = Vec::new();
                        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                            if self.peek() == Some(&Token::Dot) {
                                self.bump()?;
                            }
                            let key = self.parse_ident()?;
                            if self.peek() == Some(&Token::Colon)
                                || self.peek() == Some(&Token::Assign)
                            {
                                self.bump()?;
                            } else {
                                self.expect(Token::Colon)?;
                            }
                            let val = self.parse_expr()?;
                            fields.push(EnumInitField { key, val });
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RBrace)?.end;
                        return Ok(Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            enum_name: Ident { span, sym },
                            variant_name: Ident {
                                span: Span { start: 0, end: 0 },
                                sym: String::new(),
                            },
                            fields: Some(fields),
                            tuple_args: None,
                        }));
                    }
                    Ok(Expr::Ident(Ident { span, sym }))
                }
            }
            Token::Unsafe => {
                let block = self.parse_block_stmt()?;
                Ok(Expr::Unsafe(block))
            }
            Token::Arena => {
                if self.peek() == Some(&Token::LBrace) {
                    let block = self.parse_block_stmt()?;
                    Ok(Expr::Arena(block))
                } else {
                    Ok(Expr::Ident(Ident {
                        span,
                        sym: "arena".to_string(),
                    }))
                }
            }
            Token::Question => {
                let start_idx = span.start;
                let mut brace_depth: i32 = 0;
                let mut paren_depth: i32 = 0;
                let mut bracket_depth: i32 = 0;
                let mut end_idx = span.end;
                while let Some(tok) = self.peek() {
                    match tok {
                        Token::Semicolon | Token::Comma => {
                            if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                break;
                            }
                        }
                        Token::LBrace => brace_depth += 1,
                        Token::RBrace => {
                            if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                break;
                            }
                            brace_depth -= 1;
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
                        _ => {}
                    }
                    let s = self.bump()?.1;
                    end_idx = s.end;
                }
                let raw_code = self.input[start_idx..end_idx].to_string();
                Ok(Expr::Zig(ZigEscapeExpr {
                    span: Span {
                        start: start_idx,
                        end: end_idx,
                    },
                    code: raw_code,
                }))
            }
            Token::Struct | Token::Union | Token::Enum => {
                let start_idx = span.start;
                let mut brace_depth: i32 = 0;
                let mut paren_depth: i32 = 0;
                let mut bracket_depth: i32 = 0;
                let mut had_brace = false;
                let mut end_idx = span.end;
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
                                let s = self.bump()?.1;
                                end_idx = s.end;
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
                        _ => {}
                    }
                    let s = self.bump()?.1;
                    end_idx = s.end;
                }
                let raw_code = self.input[start_idx..end_idx].to_string();
                Ok(Expr::Zig(ZigEscapeExpr {
                    span: Span {
                        start: start_idx,
                        end: end_idx,
                    },
                    code: raw_code,
                }))
            }
            Token::Star => {
                if self.in_pipeline_rhs > 0
                    && matches!(
                        self.peek(),
                        Some(
                            Token::Comma
                                | Token::RParen
                                | Token::RBracket
                                | Token::RBrace
                                | Token::Semicolon
                                | Token::Pipe
                        ) | None
                    )
                {
                    return Ok(Expr::PipePlaceholder(span));
                }
                if self.peek() == Some(&Token::Const) || self.peek() == Some(&Token::Fn) {
                    let start_idx = span.start;
                    let mut brace_depth: i32 = 0;
                    let mut paren_depth: i32 = 0;
                    let mut bracket_depth: i32 = 0;
                    let mut end_idx = span.end;
                    while let Some(tok) = self.peek() {
                        match tok {
                            Token::Semicolon
                            | Token::Comma
                            | Token::Eq
                            | Token::NotEq
                            | Token::Less
                            | Token::LessEq
                            | Token::Greater
                            | Token::GreaterEq
                            | Token::AmpAmp
                            | Token::PipePipe
                            | Token::Question
                            | Token::Colon
                            | Token::FatArrow
                            | Token::Assign => {
                                if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                    break;
                                }
                            }
                            Token::LBrace => brace_depth += 1,
                            Token::RBrace => {
                                if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                    break;
                                }
                                brace_depth -= 1;
                            }
                            Token::LParen => paren_depth += 1,
                            Token::RParen => {
                                if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                    break;
                                }
                                if paren_depth > 0 {
                                    paren_depth -= 1;
                                }
                            }
                            Token::LBracket => bracket_depth += 1,
                            Token::RBracket => {
                                if brace_depth == 0 && paren_depth == 0 && bracket_depth == 0 {
                                    break;
                                }
                                if bracket_depth > 0 {
                                    bracket_depth -= 1;
                                }
                            }
                            Token::Ident(id)
                                if (id == "else" || id == "and" || id == "or")
                                    && brace_depth == 0
                                    && paren_depth == 0
                                    && bracket_depth == 0 =>
                            {
                                break;
                            }
                            _ => {}
                        }
                        let s = self.bump()?.1;
                        end_idx = s.end;
                    }
                    let raw_code = self.input[start_idx..end_idx].to_string();
                    return Ok(Expr::Zig(ZigEscapeExpr {
                        span: Span {
                            start: start_idx,
                            end: end_idx,
                        },
                        code: raw_code,
                    }));
                }
                let mut depth = 1;
                while self.peek() == Some(&Token::Star) {
                    self.bump()?;
                    depth += 1;
                }
                if self.peek() == Some(&Token::Bang)
                    && matches!(
                        self.peek_next(),
                        Some(
                            Token::Comma
                                | Token::RParen
                                | Token::RBracket
                                | Token::RBrace
                                | Token::Semicolon
                                | Token::Question
                                | Token::Eq
                                | Token::NotEq
                        ) | None
                    )
                {
                    let mut end = self.bump()?.1.end;
                    let mut ty = Type::RawPtr {
                        inner: Box::new(Type::Any),
                        depth,
                    };
                    if self.peek() == Some(&Token::Question) {
                        end = self.bump()?.1.end;
                        ty = Type::Optional(Box::new(ty));
                    }
                    return Ok(Expr::TypeVal {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        ty,
                    });
                }
                if matches!(
                    self.peek(),
                    Some(
                        Token::U8
                            | Token::I8
                            | Token::U16
                            | Token::I16
                            | Token::U32
                            | Token::I32
                            | Token::U64
                            | Token::I64
                            | Token::U128
                            | Token::I128
                            | Token::USize
                            | Token::ISize
                            | Token::F32
                            | Token::F64
                            | Token::Bool
                            | Token::Void
                            | Token::String
                    )
                ) && self.peek_next() != Some(&Token::LParen)
                {
                    let mut ty = self.parse_base_type()?;
                    let end = self.peek_span().end;
                    for _ in 0..depth {
                        ty = Type::BorrowType {
                            inner: Box::new(ty),
                            is_mut: true,
                            lifetimes: vec![],
                        };
                    }
                    return Ok(Expr::TypeVal {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        ty,
                    });
                }
                let inner = self.parse_postfix_expr()?;
                if self.peek() == Some(&Token::Bang) {
                    let mut end = self.bump()?.1.end;
                    let mut is_optional = false;
                    if self.peek() == Some(&Token::Question) {
                        end = self.bump()?.1.end;
                        is_optional = true;
                    }
                    let mut target_ty = Type::RawPtr {
                        inner: Box::new(Type::Any),
                        depth,
                    };
                    if is_optional {
                        target_ty = Type::Optional(Box::new(target_ty));
                    }
                    return Ok(Expr::Cast {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        expr: Box::new(Expr::Borrow(Box::new(inner), true)),
                        target_ty,
                    });
                } else if self.peek() == Some(&Token::Question)
                    && self.peek_next() == Some(&Token::Bang)
                {
                    self.bump()?;
                    let end = self.bump()?.1.end;
                    let target_ty = Type::Optional(Box::new(Type::RawPtr {
                        inner: Box::new(Type::Any),
                        depth,
                    }));
                    return Ok(Expr::Cast {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        expr: Box::new(Expr::Borrow(Box::new(inner), true)),
                        target_ty,
                    });
                }
                Ok(Expr::Borrow(Box::new(inner), true))
            }
            Token::Bang => {
                let inner = self.parse_postfix_expr()?;
                Ok(Expr::Builtin(BuiltinCall {
                    span,
                    name: "@not".to_string(),
                    args: vec![inner],
                }))
            }
            Token::Tilde => {
                let inner = self.parse_postfix_expr()?;
                Ok(Expr::BitNot(Box::new(inner)))
            }
            Token::U8
            | Token::I8
            | Token::U16
            | Token::I16
            | Token::U32
            | Token::I32
            | Token::U64
            | Token::I64
            | Token::U128
            | Token::I128
            | Token::USize
            | Token::ISize
            | Token::F32
            | Token::F64
            | Token::Bool
            | Token::Void
            | Token::CStr
            | Token::Any => {
                let sym = match token {
                    Token::U8 => "u8",
                    Token::I8 => "i8",
                    Token::U16 => "u16",
                    Token::I16 => "i16",
                    Token::U32 => "u32",
                    Token::I32 => "i32",
                    Token::U64 => "u64",
                    Token::I64 => "i64",
                    Token::U128 => "u128",
                    Token::I128 => "i128",
                    Token::USize => "usize",
                    Token::ISize => "isize",
                    Token::F32 => "f32",
                    Token::F64 => "f64",
                    Token::Bool => "bool",
                    Token::Void => "void",
                    Token::CStr => "cstr",
                    Token::Any => "anytype",
                    _ => unreachable!(),
                }
                .to_string();
                Ok(Expr::Ident(Ident { span, sym }))
            }
            Token::Ident(mut sym) => {
                if sym == "asm" {
                    let mut is_volatile = true;
                    if matches!(self.peek(), Some(Token::Ident(id)) if id == "volatile") {
                        self.bump()?;
                        is_volatile = true;
                    }
                    if self.peek() == Some(&Token::LParen) {
                        self.bump()?; // consume '('
                        let inner_start = self.peek_span().start;
                        let mut paren_depth = 1;
                        let mut end = inner_start;
                        while paren_depth > 0 && self.peek().is_some() {
                            let (tok, t_span) = self.bump()?;
                            match tok {
                                Token::LParen => paren_depth += 1,
                                Token::RParen => {
                                    paren_depth -= 1;
                                    if paren_depth == 0 {
                                        end = t_span.start;
                                        break;
                                    }
                                }
                                _ => {}
                            }
                        }
                        let content = self.input[inner_start..end].trim().to_string();
                        let asm_end = self.peek_span().start;
                        return Ok(Expr::InlineAsm {
                            span: Span {
                                start: span.start,
                                end: asm_end,
                            },
                            is_volatile,
                            content,
                        });
                    }
                }
                if ((sym == "packed" || sym == "extern")
                    && (self.peek() == Some(&Token::Struct)
                        || self.peek() == Some(&Token::Union)
                        || self.peek() == Some(&Token::Enum)
                        || self.peek() == Some(&Token::LParen)))
                    || (sym == "opaque"
                        && (self.peek() == Some(&Token::LBrace)
                            || self.peek() == Some(&Token::Struct)
                            || self.peek() == Some(&Token::Union)))
                {
                    let start_idx = span.start;
                    let mut brace_depth: i32 = 0;
                    let mut paren_depth: i32 = 0;
                    let mut bracket_depth: i32 = 0;
                    let mut had_brace = false;
                    let mut end_idx = span.end;
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
                                    let s = self.bump()?.1;
                                    end_idx = s.end;
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
                            _ => {}
                        }
                        let s = self.bump()?.1;
                        end_idx = s.end;
                    }
                    let raw_code = self.input[start_idx..end_idx].to_string();
                    return Ok(Expr::Zig(ZigEscapeExpr {
                        span: Span {
                            start: start_idx,
                            end: end_idx,
                        },
                        code: raw_code,
                    }));
                }
                if self.peek() == Some(&Token::Colon) && self.peek_next() == Some(&Token::LBrace) {
                    self.bump()?; // consume ':'
                    let block = self.parse_block_stmt()?;
                    return Ok(Expr::LabeledBlock {
                        label: Some(Ident { span, sym }),
                        block,
                    });
                }
                if self.peek() == Some(&Token::Colon) && self.peek_next() == Some(&Token::While) {
                    self.bump()?; // consume ':'
                    let mut w = self.parse_while_stmt()?;
                    w.label = Some(Ident { span, sym });
                    let else_expr = if self.peek() == Some(&Token::Else) {
                        self.bump()?;
                        let e = if self.peek() == Some(&Token::LBrace) {
                            let block = self.parse_block_stmt()?;
                            Expr::LabeledBlock { label: None, block }
                        } else {
                            self.parse_expr()?
                        };
                        Some(Box::new(e))
                    } else {
                        None
                    };
                    return Ok(Expr::While {
                        stmt: Box::new(w),
                        else_expr,
                    });
                }
                if (sym == "Err" || sym == "err") && self.peek() == Some(&Token::LParen) {
                    self.bump()?;
                    let inner = self.parse_expr()?;
                    self.expect(Token::RParen)?;
                    return Ok(Expr::ErrWrap(Box::new(inner)));
                }
                if sym
                    .trim_start_matches('_')
                    .chars()
                    .next()
                    .map_or(false, |c| c.is_uppercase())
                {
                    if self.peek() == Some(&Token::Less) {
                        let mut probe = self.clone_lookahead();
                        if probe.bump().is_ok() {
                            let mut depth = 1;
                            let mut found_greater = false;
                            while let Ok((tok, _)) = probe.bump() {
                                if tok == Token::Less {
                                    depth += 1;
                                } else if tok == Token::Greater {
                                    depth -= 1;
                                    if depth == 0 {
                                        found_greater = true;
                                        break;
                                    }
                                } else if tok == Token::Semicolon || tok == Token::RBrace {
                                    break;
                                }
                            }
                            if found_greater
                                && (probe.peek() == Some(&Token::LBrace)
                                    || probe.peek() == Some(&Token::Dot))
                            {
                                self.bump()?;
                                sym.push('<');
                                let mut angle_depth = 1;
                                let mut prev_end: Option<usize> = None;
                                while angle_depth > 0 && self.peek().is_some() {
                                    let (tok, t_span) = self.bump()?;
                                    match tok {
                                        Token::Less => angle_depth += 1,
                                        Token::Greater => angle_depth -= 1,
                                        _ => {}
                                    }
                                    if angle_depth == 0 {
                                        sym.push('>');
                                        break;
                                    }
                                    if let Some(pe) = prev_end {
                                        if t_span.start > pe {
                                            sym.push(' ');
                                        }
                                    }
                                    sym.push_str(&self.input[t_span.start..t_span.end]);
                                    prev_end = Some(t_span.end);
                                }
                            }
                        }
                    }
                    if self.peek() == Some(&Token::LBrace) {
                        self.bump()?;
                        let mut fields = Vec::new();
                        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                            if self.peek() == Some(&Token::Dot) {
                                self.bump()?;
                            }
                            let key = self.parse_ident()?;
                            if self.peek() == Some(&Token::Colon)
                                || self.peek() == Some(&Token::Assign)
                            {
                                self.bump()?;
                            } else {
                                self.expect(Token::Colon)?;
                            }
                            let val = self.parse_expr()?;
                            fields.push(EnumInitField { key, val });
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RBrace)?.end;
                        return Ok(Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            enum_name: Ident { span, sym },
                            variant_name: Ident {
                                span,
                                sym: String::new(),
                            },
                            fields: Some(fields),
                            tuple_args: None,
                        }));
                    }
                }
                if sym.starts_with('@') && !sym.starts_with("@\"") {
                    if sym == "@is_beam" || sym == "@is_nest" {
                        return Ok(Expr::Ident(Ident { span, sym }));
                    }
                    let name = sym.clone();
                    self.expect(Token::LParen)?;
                    let mut args = Vec::new();
                    while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                        args.push(self.parse_bin_expr(0)?);
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    let end = self.expect(Token::RParen)?.end;
                    if name == "@This" && args.is_empty() && self.peek() == Some(&Token::LBrace) {
                        self.bump()?; // consume '{'
                        let mut fields = Vec::new();
                        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                            if self.peek() == Some(&Token::Dot) {
                                self.bump()?;
                            }
                            let key = self.parse_ident()?;
                            let val = if self.peek() == Some(&Token::Colon)
                                || self.peek() == Some(&Token::Assign)
                            {
                                self.bump()?;
                                self.parse_expr()?
                            } else if matches!(
                                self.peek(),
                                Some(Token::Comma) | Some(Token::RBrace)
                            ) {
                                Expr::Ident(key.clone())
                            } else {
                                self.expect(Token::Colon)?;
                                self.parse_expr()?
                            };
                            fields.push(EnumInitField { key, val });
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let init_end = self.expect(Token::RBrace)?.end;
                        return Ok(Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: span.start,
                                end: init_end,
                            },
                            enum_name: Ident {
                                span: Span {
                                    start: span.start,
                                    end,
                                },
                                sym: "@This()".to_string(),
                            },
                            variant_name: Ident {
                                span: Span { start: end, end },
                                sym: String::new(),
                            },
                            fields: Some(fields),
                            tuple_args: None,
                        }));
                    }
                    Ok(Expr::Builtin(BuiltinCall {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        name,
                        args,
                    }))
                } else if sym == "comptime"
                    && !matches!(
                        self.peek(),
                        Some(
                            Token::Comma
                                | Token::Semicolon
                                | Token::RParen
                                | Token::RBracket
                                | Token::RBrace
                                | Token::Colon
                                | Token::Assign
                                | Token::Dot
                        ) | None
                    )
                {
                    let inner = self.parse_bin_expr(4)?;
                    Ok(Expr::ComptimeExpr(Box::new(inner)))
                } else {
                    Ok(Expr::Ident(Ident { span, sym }))
                }
            }
            Token::At => {
                if self.peek() == Some(&Token::LBrace) {
                    let block = self.parse_block_stmt()?;
                    return Ok(Expr::Comptime(block));
                }
                if self.peek() == Some(&Token::If) {
                    self.bump()?; // consume 'if'
                    self.expect(Token::LParen)?;
                    let test = self.parse_expr()?;
                    self.expect(Token::RParen)?;
                    if self.peek() == Some(&Token::FatArrow) {
                        self.bump()?;
                    }
                    let cons = self.parse_expr()?;
                    self.expect(Token::Else)?;
                    let alt = self.parse_expr()?;
                    let end = right_span(&alt).end;
                    return Ok(Expr::If {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        test: Box::new(test),
                        cons: Box::new(cons),
                        alt: Some(Box::new(alt)),
                        is_comptime: true,
                        capture: None,
                        is_ptr_capture: false,
                    });
                }
                let ident = self.parse_ident()?;
                let name = format!("@{}", ident.sym);
                if name == "@is_beam" || name == "@is_nest" {
                    return Ok(Expr::Ident(Ident {
                        span: Span {
                            start: span.start,
                            end: ident.span.end,
                        },
                        sym: name,
                    }));
                }
                if name == "@c" {
                    if self.peek() == Some(&Token::Fn) {
                        let ty = self.parse_fn_type_sig(true)?;
                        let end = self.peek_span().start;
                        return Ok(Expr::TypeVal {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            ty,
                        });
                    }
                    if self.peek() == Some(&Token::LParen) {
                        let mut clone = self.clone_lookahead();
                        let _ = clone.bump();
                        if clone.peek() == Some(&Token::LParen) {
                            self.bump()?; // outer '('
                            let ty = self.parse_fn_type_sig_body(true)?;
                            let end = self.expect(Token::RParen)?.end;
                            return Ok(Expr::TypeVal {
                                span: Span {
                                    start: span.start,
                                    end,
                                },
                                ty,
                            });
                        }
                        let mut depth = 1i32;
                        while depth > 0 {
                            let Ok((tok, _)) = clone.bump() else {
                                break;
                            };
                            match tok {
                                Token::LParen => depth += 1,
                                Token::RParen => depth -= 1,
                                _ => {}
                            }
                        }
                        if matches!(clone.peek(), Some(Token::Colon | Token::Arrow)) {
                            let ty = self.parse_fn_type_sig_body(true)?;
                            let end = self.peek_span().start;
                            return Ok(Expr::TypeVal {
                                span: Span {
                                    start: span.start,
                                    end,
                                },
                                ty,
                            });
                        }
                    }
                }
                if name == "@as" {
                    self.expect(Token::LParen)?;
                    let target_ty = self.parse_type()?;
                    self.expect(Token::Comma)?;
                    let expr = self.parse_expr()?;
                    let end = self.expect(Token::RParen)?.end;
                    return Ok(Expr::Cast {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        expr: Box::new(expr),
                        target_ty,
                    });
                }
                if name == "@asm" {
                    let mut is_volatile = true;
                    if matches!(self.peek(), Some(Token::Ident(id)) if id == "volatile") {
                        self.bump()?;
                        is_volatile = true;
                    }
                    self.expect(Token::LParen)?;
                    let inner_start = self.peek_span().start;
                    let mut paren_depth = 1;
                    let mut end = inner_start;
                    while paren_depth > 0 && self.peek().is_some() {
                        let (tok, t_span) = self.bump()?;
                        match tok {
                            Token::LParen => paren_depth += 1,
                            Token::RParen => {
                                paren_depth -= 1;
                                if paren_depth == 0 {
                                    end = t_span.start;
                                    break;
                                }
                            }
                            _ => {}
                        }
                    }
                    let content = self.input[inner_start..end].trim().to_string();
                    let asm_end = self.peek_span().start;
                    return Ok(Expr::InlineAsm {
                        span: Span {
                            start: span.start,
                            end: asm_end,
                        },
                        is_volatile,
                        content,
                    });
                }
                if name == "@extern" {
                    self.expect(Token::LParen)?;
                    let ty_start = self.peek_span().start;
                    let target_ty = self.parse_type()?;
                    let ty_end = self.peek_span().start;
                    let mut args = vec![Expr::TypeVal {
                        span: Span {
                            start: ty_start,
                            end: ty_end,
                        },
                        ty: target_ty,
                    }];
                    while self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                        if self.peek() == Some(&Token::RParen) {
                            break;
                        }
                        args.push(self.parse_bin_expr(0)?);
                    }
                    let end = self.expect(Token::RParen)?.end;
                    return Ok(Expr::Builtin(BuiltinCall {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        name,
                        args,
                    }));
                }
                if name == "@map" || name == "@comptimeMap" || name == "@comptime_map" {
                    self.expect(Token::LParen)?;
                    let (target_ty, entries) = if self.peek() == Some(&Token::LBrace) {
                        self.bump()?; // consume '{'
                        let fields = self.parse_object_fields(false)?;
                        (None, fields)
                    } else {
                        let mut ty = self.parse_type()?;
                        self.expect(Token::Comma)?;
                        if self.peek() != Some(&Token::LBrace) {
                            ty = self.parse_type()?;
                            self.expect(Token::Comma)?;
                        }
                        self.expect(Token::LBrace)?;
                        let fields = self.parse_object_fields(false)?;
                        (Some(ty), fields)
                    };
                    let end = self.expect(Token::RParen)?.end;
                    return Ok(Expr::ComptimeMap {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        target_ty,
                        entries,
                    });
                }
                self.expect(Token::LParen)?;
                let mut args = Vec::new();
                while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                    args.push(self.parse_bin_expr(0)?);
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                    }
                }
                let end = self.expect(Token::RParen)?.end;
                if name == "@This" && args.is_empty() && self.peek() == Some(&Token::LBrace) {
                    self.bump()?; // consume '{'
                    let mut fields = Vec::new();
                    while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                        if self.peek() == Some(&Token::Dot) {
                            self.bump()?;
                        }
                        let key = self.parse_ident()?;
                        let val = if self.peek() == Some(&Token::Colon)
                            || self.peek() == Some(&Token::Assign)
                        {
                            self.bump()?;
                            self.parse_expr()?
                        } else if matches!(self.peek(), Some(Token::Comma) | Some(Token::RBrace)) {
                            Expr::Ident(key.clone())
                        } else {
                            self.expect(Token::Colon)?;
                            self.parse_expr()?
                        };
                        fields.push(EnumInitField { key, val });
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    let init_end = self.expect(Token::RBrace)?.end;
                    return Ok(Expr::EnumInit(EnumInitExpr {
                        span: Span {
                            start: span.start,
                            end: init_end,
                        },
                        enum_name: Ident {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            sym: "@This()".to_string(),
                        },
                        variant_name: Ident {
                            span: Span { start: end, end },
                            sym: String::new(),
                        },
                        fields: Some(fields),
                        tuple_args: None,
                    }));
                }
                let _is_known_builtin = matches!(
                    name.as_str(),
                    "@print"
                        | "@print_err"
                        | "@not"
                        | "@spawn"
                        | "@worker_spawn"
                        | "@net_accept_async"
                        | "@net_read_async"
                        | "@net_write_async"
                        | "@net_listen"
                        | "@net_close"
                        | "@net_connect"
                        | "@net_send"
                        | "@yield_to_beam"
                        | "@io_write_stdout"
                        | "@io_write_stderr"
                        | "@io_read_stdin_line"
                        | "@http_fetch"
                        | "@http_serve"
                        | "@fs_read_async"
                        | "@fs_write_file"
                        | "@fs_mkdir"
                        | "@fs_stat_size"
                        | "@fs_is_dir"
                        | "@fs_remove"
                        | "@fs_rmdir"
                        | "@fs_rename"
                        | "@fs_copy_file"
                        | "@fs_exists"
                        | "@fs_read_file"
                        | "@crypto_uuid"
                        | "@crypto_hash"
                        | "@hash"
                        | "@self"
                        | "@sender"
                        | "@reply"
                        | "@send"
                        | "@receive"
                        | "@task_join"
                        | "@create_pending_task"
                        | "@complete_task"
                        | "@alloc"
                        | "@free"
                        | "@get_arena"
                        | "@shared_tensor_init"
                        | "@release"
                        | "@json_parse"
                        | "@json_stringify"
                        | "@compileError"
                        | "@compileLog"
                        | "@typeOf"
                        | "@typeName"
                        | "@fieldsOf"
                        | "@field"
                        | "@hasField"
                        | "@sizeOf"
                        | "@alignOf"
                        | "@as"
                        | "@test"
                        | "@expect_eq"
                );
                Ok(Expr::Builtin(BuiltinCall {
                    span: Span {
                        start: span.start,
                        end,
                    },
                    name,
                    args,
                }))
            }
            Token::Match | Token::Switch => {
                let match_expr = self.parse_match_expr()?;
                Ok(Expr::Match(Box::new(match_expr)))
            }
            Token::Fn => {
                let params = self.parse_params()?;
                let mut _return_type = None;
                if self.peek() == Some(&Token::Bang) {
                    self.bump()?;
                    if self.is_error_type_rhs_start() {
                        let err = self.parse_error_type_rhs()?;
                        _return_type = Some(Type::ResultType {
                            ok: Box::new(Type::Void),
                            err,
                        });
                    } else {
                        _return_type = Some(Type::ErrorUnion(Box::new(Type::Void)));
                    }
                } else if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
                    self.bump()?;
                    _return_type = Some(self.parse_type()?);
                } else if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && self.peek() != Some(&Token::LBrace)
                    && self.peek() != Some(&Token::Semicolon)
                {
                    _return_type = Some(self.parse_type()?);
                }

                if self.peek() != Some(&Token::LBrace) {
                    let end_idx = self.peek_span().start;
                    if let Some(ret_ty) = _return_type {
                        let param_types: Vec<Type> = params.into_iter().map(|p| p.ty).collect();
                        return Ok(Expr::TypeVal {
                            span: Span {
                                start: span.start,
                                end: end_idx,
                            },
                            ty: Type::Fn(param_types, Box::new(ret_ty)),
                        });
                    }
                    let raw_code = self.input[span.start..end_idx].trim().to_string();
                    let clean_code = raw_code.replace("): ", ") ").replace(") : ", ") ");
                    return Ok(Expr::Zig(ZigEscapeExpr {
                        span: Span {
                            start: span.start,
                            end: end_idx,
                        },
                        code: clean_code,
                    }));
                }
                let body = ArrowBody::Block(self.parse_block_stmt()?);
                let pat_params = params
                    .into_iter()
                    .map(|p| Pattern::Ident(p.ident))
                    .collect();
                Ok(Expr::Arrow(Box::new(ArrowExpr {
                    span: Span {
                        start: span.start,
                        end: self.peek_span().end,
                    },
                    is_async: false,
                    params: pat_params,
                    body: Box::new(body),
                })))
            }
            Token::LParen => {
                if self.peek() == Some(&Token::LBrace) {
                    let block = self.parse_block_stmt()?;
                    self.expect(Token::RParen)?;
                    return Ok(Expr::LabeledBlock { label: None, block });
                }
                let checkpoint = self.lexer.clone();
                let peeked_checkpoint = self.peeked.clone();
                let peeked2_checkpoint = self.peeked2.clone();
                let err_len = self.errors.len();

                // Try parsing as arrow function parameters
                if let Ok(params) = self.parse_arrow_params() {
                    if self.peek() == Some(&Token::FatArrow) {
                        self.bump()?;
                        let body = if self.peek() == Some(&Token::LBrace) {
                            ArrowBody::Block(self.parse_block_stmt()?)
                        } else {
                            ArrowBody::Expr(self.parse_expr()?)
                        };
                        return Ok(Expr::Arrow(Box::new(ArrowExpr {
                            span: Span {
                                start: span.start,
                                end: self.peek_span().end,
                            },
                            is_async: false,
                            params,
                            body: Box::new(body),
                        })));
                    }
                }

                // Fallback to grouped expression
                self.lexer = checkpoint;
                self.peeked = peeked_checkpoint;
                self.peeked2 = peeked2_checkpoint;
                self.errors.truncate(err_len);
                let expr = self.parse_expr()?;
                self.expect(Token::RParen)?;
                Ok(expr)
            }
            Token::Async => {
                if self.peek() == Some(&Token::Fn) {
                    self.bump()?;
                    let params = self.parse_params()?;
                    let mut _return_type = None;
                    if self.peek() == Some(&Token::Bang) {
                        self.bump()?;
                        if self.is_error_type_rhs_start() {
                            let err = self.parse_error_type_rhs()?;
                            _return_type = Some(Type::ResultType {
                                ok: Box::new(Type::Void),
                                err,
                            });
                        } else {
                            _return_type = Some(Type::ErrorUnion(Box::new(Type::Void)));
                        }
                    } else if self.peek() == Some(&Token::Colon)
                        || self.peek() == Some(&Token::Arrow)
                    {
                        self.bump()?;
                        _return_type = Some(self.parse_type()?);
                    }
                    let body = ArrowBody::Block(self.parse_block_stmt()?);
                    let pat_params = params
                        .into_iter()
                        .map(|p| Pattern::Ident(p.ident))
                        .collect();
                    Ok(Expr::Arrow(Box::new(ArrowExpr {
                        span: Span {
                            start: span.start,
                            end: self.peek_span().end,
                        },
                        is_async: true,
                        params: pat_params,
                        body: Box::new(body),
                    })))
                } else if self.peek() == Some(&Token::LParen) {
                    self.bump()?;
                    let params = self.parse_arrow_params()?;
                    self.expect(Token::FatArrow)?;
                    let body = if self.peek() == Some(&Token::LBrace) {
                        ArrowBody::Block(self.parse_block_stmt()?)
                    } else {
                        ArrowBody::Expr(self.parse_expr()?)
                    };
                    Ok(Expr::Arrow(Box::new(ArrowExpr {
                        span: Span {
                            start: span.start,
                            end: self.peek_span().end,
                        },
                        is_async: true,
                        params,
                        body: Box::new(body),
                    })))
                } else {
                    let ident = self.parse_ident()?;
                    self.expect(Token::FatArrow)?;
                    let body = if self.peek() == Some(&Token::LBrace) {
                        ArrowBody::Block(self.parse_block_stmt()?)
                    } else {
                        ArrowBody::Expr(self.parse_expr()?)
                    };
                    Ok(Expr::Arrow(Box::new(ArrowExpr {
                        span: Span {
                            start: span.start,
                            end: self.peek_span().end,
                        },
                        is_async: true,
                        params: vec![Pattern::Ident(ident)],
                        body: Box::new(body),
                    })))
                }
            }
            Token::HashLBrace => {
                let fields = self.parse_object_fields(false)?;
                Ok(Expr::Dict(fields))
            }
            Token::LBrace => {
                let fields = self.parse_object_fields(false)?;
                Ok(Expr::Object(fields))
            }
            Token::Await => {
                let expr = self.parse_expr()?;
                Ok(Expr::Await(Box::new(expr)))
            }
            Token::Move | Token::Own => {
                let expr = self.parse_postfix_expr()?;
                Ok(Expr::Move(Box::new(expr)))
            }
            Token::Ampersand => {
                let mut is_mut = false;
                if self.peek() == Some(&Token::Mut) {
                    self.bump()?;
                    is_mut = true;
                }
                let expr = self.parse_postfix_expr()?;
                if self.peek() == Some(&Token::Bang) {
                    let mut end = self.bump()?.1.end;
                    let mut is_optional = false;
                    if self.peek() == Some(&Token::Question) {
                        end = self.bump()?.1.end;
                        is_optional = true;
                    }
                    let mut target_ty = Type::RawPtr {
                        inner: Box::new(Type::Any),
                        depth: 1,
                    };
                    if is_optional {
                        target_ty = Type::Optional(Box::new(target_ty));
                    }
                    return Ok(Expr::Cast {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        expr: Box::new(Expr::Borrow(Box::new(expr), is_mut)),
                        target_ty,
                    });
                } else if self.peek() == Some(&Token::Question)
                    && self.peek_next() == Some(&Token::Bang)
                {
                    self.bump()?;
                    let end = self.bump()?.1.end;
                    let target_ty = Type::Optional(Box::new(Type::RawPtr {
                        inner: Box::new(Type::Any),
                        depth: 1,
                    }));
                    return Ok(Expr::Cast {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        expr: Box::new(Expr::Borrow(Box::new(expr), is_mut)),
                        target_ty,
                    });
                }
                Ok(Expr::Borrow(Box::new(expr), is_mut))
            }
            Token::IntLiteral(v) => Ok(Expr::Lit(Lit::Int(v))),
            Token::FloatLiteral(v) => Ok(Expr::Lit(Lit::Float(v))),
            Token::StringLiteral(s) => Ok(Expr::Lit(Lit::Str(s))),
            Token::TemplateStringRaw(s) => {
                let start = span.start;
                let mut quasis = Vec::new();
                let mut expressions = Vec::new();
                let mut current_quasi = String::new();
                let chars: Vec<char> = s.chars().collect();
                let mut i = 0;
                while i < chars.len() {
                    if chars[i] == '\\' && i + 1 < chars.len() {
                        let next_c = chars[i + 1];
                        if next_c == '$' {
                            current_quasi.push('$');
                            i += 2;
                            continue;
                        } else if next_c == '`' {
                            current_quasi.push('`');
                            i += 2;
                            continue;
                        } else if next_c == '\\' {
                            current_quasi.push('\\');
                            i += 2;
                            continue;
                        }
                    }

                    if chars[i] == '$' && i + 1 < chars.len() && chars[i + 1] == '{' {
                        quasis.push(current_quasi.clone());
                        current_quasi.clear();
                        i += 2;
                        let mut brace_depth = 1;
                        let mut expr_str = String::new();
                        let mut in_single_quote = false;
                        let mut in_double_quote = false;
                        let mut escape = false;

                        while i < chars.len() {
                            let c = chars[i];
                            if escape {
                                escape = false;
                            } else if c == '\\' {
                                escape = true;
                            } else if c == '"' && !in_single_quote {
                                in_double_quote = !in_double_quote;
                            } else if c == '\'' && !in_double_quote {
                                in_single_quote = !in_single_quote;
                            } else if !in_single_quote && !in_double_quote {
                                if c == '{' {
                                    brace_depth += 1;
                                } else if c == '}' {
                                    brace_depth -= 1;
                                    if brace_depth == 0 {
                                        break;
                                    }
                                }
                            }

                            expr_str.push(c);
                            i += 1;
                        }
                        if brace_depth != 0 {
                            return Err(ParseError::UnexpectedToken(
                                Token::Error,
                                Span {
                                    start,
                                    end: span.end,
                                },
                            ));
                        }
                        i += 1;

                        let mut sub_parser = Parser::new(&expr_str);
                        let expr = sub_parser.parse_expr()?;
                        expressions.push(expr);
                    } else {
                        current_quasi.push(chars[i]);
                        i += 1;
                    }
                }
                quasis.push(current_quasi);
                Ok(Expr::Template(TemplateLit {
                    span: Span {
                        start,
                        end: span.end,
                    },
                    quasis,
                    expressions,
                }))
            }
            Token::New => {
                if !matches!(self.peek(), Some(Token::Ident(_))) {
                    return Ok(Expr::Ident(Ident {
                        span,
                        sym: "new".to_string(),
                    }));
                }
                let callee_expr = self.parse_primary_expr()?;
                if let Expr::Ident(mut ident) = callee_expr {
                    if self.peek() == Some(&Token::Less) {
                        self.bump()?;
                        ident.sym.push('<');
                        let mut depth = 1;
                        let mut prev_end: Option<usize> = None;
                        while depth > 0 && self.peek().is_some() {
                            let (tok, t_span) = self.bump()?;
                            match tok {
                                Token::Less => depth += 1,
                                Token::Greater => depth -= 1,
                                _ => {}
                            }
                            if depth == 0 {
                                ident.sym.push('>');
                                break;
                            }
                            if let Some(pe) = prev_end {
                                if t_span.start > pe {
                                    ident.sym.push(' ');
                                }
                            }
                            ident.sym.push_str(&self.input[t_span.start..t_span.end]);
                            prev_end = Some(t_span.end);
                        }
                    }
                    self.expect(Token::LParen)?;
                    let mut args = Vec::new();
                    while self.peek() != Some(&Token::RParen) {
                        args.push(self.parse_expr()?);
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    let end = self.expect(Token::RParen)?.end;
                    Ok(Expr::New(CallExpr {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        callee: Box::new(Expr::Ident(ident)),
                        args,
                        is_comptime: false,
                    }))
                } else {
                    Err(ParseError::UnexpectedToken(Token::New, span))
                }
            }

            Token::LBracket => {
                if self.peek() == Some(&Token::RBracket) {
                    self.bump()?;
                    let (qual, has_const) = self.parse_ptr_qualifiers()?;
                    if !qual.is_empty()
                        || has_const
                        || matches!(
                            self.peek(),
                            Some(
                                Token::U8
                                    | Token::I8
                                    | Token::U16
                                    | Token::I16
                                    | Token::U32
                                    | Token::I32
                                    | Token::U64
                                    | Token::I64
                                    | Token::U128
                                    | Token::I128
                                    | Token::USize
                                    | Token::ISize
                                    | Token::F32
                                    | Token::F64
                                    | Token::Bool
                                    | Token::String
                                    | Token::Void
                                    | Token::Any
                                    | Token::CStr
                                    | Token::LBracket
                                    | Token::Star
                                    | Token::Ident(_)
                            )
                        )
                    {
                        let elem_ty = self.parse_base_type()?;
                        let ty = if !qual.is_empty() {
                            Type::Array(Box::new(Self::apply_ptr_qualifiers(
                                elem_ty, qual, has_const,
                            )))
                        } else if has_const {
                            Type::BorrowType {
                                inner: Box::new(Type::Array(Box::new(elem_ty))),
                                is_mut: false,
                                lifetimes: vec![],
                            }
                        } else {
                            Type::Array(Box::new(elem_ty))
                        };
                        let ty = self.parse_postfix_type_modifiers(ty)?;
                        let end = self.peek_span().start;
                        return Ok(Expr::TypeVal {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            ty,
                        });
                    }
                    return Ok(Expr::Array(Vec::new()));
                }
                if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Star) {
                    let is_sentinel_slice = self.peek() == Some(&Token::Colon);
                    self.bump()?; // consume ':' or '*'
                    let mut is_c_ptr = false;
                    let mut sentinel = String::new();
                    if is_sentinel_slice {
                        sentinel = match self.bump()?.0 {
                            Token::IntLiteral(n) => n.to_string(),
                            Token::Ident(s) => s,
                            Token::Null => "null".to_string(),
                            _ => "0".to_string(),
                        };
                    } else {
                        if matches!(self.peek(), Some(Token::Ident(id)) if id == "c") {
                            self.bump()?;
                            is_c_ptr = true;
                        } else if self.peek() == Some(&Token::Colon) {
                            self.bump()?;
                            sentinel = match self.bump()?.0 {
                                Token::IntLiteral(n) => n.to_string(),
                                Token::Ident(s) => s,
                                Token::Null => "null".to_string(),
                                _ => "0".to_string(),
                            };
                        }
                    }
                    self.expect(Token::RBracket)?;
                    let has_const = if self.peek() == Some(&Token::Const) {
                        self.bump()?;
                        true
                    } else {
                        false
                    };
                    let elem_ty = self.parse_base_type()?;
                    let elem_str = Self::type_to_str(&elem_ty);
                    let const_prefix = if has_const { "const " } else { "" };
                    let ty_sym = if is_sentinel_slice {
                        format!("[:{}]{}{}", sentinel, const_prefix, elem_str)
                    } else if is_c_ptr {
                        format!("[*c]{}{}", const_prefix, elem_str)
                    } else if !sentinel.is_empty() {
                        format!("[*:{sentinel}]{}{}", const_prefix, elem_str)
                    } else {
                        format!("[*]{}{}", const_prefix, elem_str)
                    };
                    let end = self.peek_span().start;
                    return Ok(Expr::TypeVal {
                        span: Span {
                            start: span.start,
                            end,
                        },
                        ty: Type::Ref(Ident { span, sym: ty_sym }),
                    });
                }
                let size_start = self.peek_span().start;
                let first = self.parse_expr()?;
                if self.peek() == Some(&Token::Semicolon) {
                    self.bump()?;
                    let count = self.parse_expr()?;
                    self.expect(Token::RBracket)?;
                    return Ok(Expr::ArrayRepeat(Box::new(first), Box::new(count)));
                }
                let mut elements = vec![first];
                let mut had_comma = false;
                if self.peek() == Some(&Token::Comma) {
                    had_comma = true;
                    self.bump()?;
                    while self.peek() != Some(&Token::RBracket) {
                        elements.push(self.parse_expr()?);
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                }
                let rbracket_span = self.expect(Token::RBracket)?;
                if elements.len() == 1 && !had_comma {
                    let is_adj_type = matches!(
                        self.peek(),
                        Some(
                            Token::U8
                                | Token::I8
                                | Token::U16
                                | Token::I16
                                | Token::U32
                                | Token::I32
                                | Token::U64
                                | Token::I64
                                | Token::U128
                                | Token::I128
                                | Token::USize
                                | Token::ISize
                                | Token::F32
                                | Token::F64
                                | Token::Bool
                                | Token::String
                                | Token::Const
                                | Token::Void
                                | Token::Any
                                | Token::CStr
                                | Token::LBracket
                        ) | Some(Token::Ident(_))
                    ) || (self.peek() == Some(&Token::Star)
                        && matches!(
                            self.peek_next(),
                            Some(
                                Token::Const
                                    | Token::Bang
                                    | Token::LBracket
                                    | Token::Star
                                    | Token::Ident(_)
                                    | Token::U8
                                    | Token::I8
                                    | Token::U16
                                    | Token::I16
                                    | Token::U32
                                    | Token::I32
                                    | Token::U64
                                    | Token::I64
                                    | Token::U128
                                    | Token::I128
                                    | Token::USize
                                    | Token::ISize
                                    | Token::F32
                                    | Token::F64
                                    | Token::Bool
                                    | Token::String
                                    | Token::Void
                                    | Token::Any
                                    | Token::CStr
                            )
                        ));
                    if is_adj_type {
                        let size_raw = self.input[size_start..rbracket_span.start]
                            .trim()
                            .to_string();
                        let arr_size = match &elements[0] {
                            Expr::Lit(Lit::Int(n)) => ArraySize::Literal(*n as usize),
                            Expr::Ident(id) => ArraySize::Ident(id.clone()),
                            _ => ArraySize::Expr(size_raw),
                        };
                        let elem_ty = self.parse_type()?;
                        let end = self.peek_span().start;
                        return Ok(Expr::TypeVal {
                            span: Span {
                                start: span.start,
                                end,
                            },
                            ty: Type::FixedArray(Box::new(elem_ty), arr_size),
                        });
                    }
                }
                Ok(Expr::Array(elements))
            }
            Token::Continue => Ok(Expr::Continue(span)),
            Token::Break => {
                if self.peek() == Some(&Token::Colon) {
                    self.bump()?;
                    let label = self.parse_ident()?;
                    let val = if !matches!(
                        self.peek(),
                        Some(Token::Semicolon | Token::RBrace | Token::RParen | Token::Comma)
                            | None
                    ) {
                        Some(Box::new(self.parse_expr()?))
                    } else {
                        None
                    };
                    Ok(Expr::LabeledBreak(span, label, val))
                } else {
                    let has_val = !matches!(
                        self.peek(),
                        Some(Token::Semicolon | Token::RBrace | Token::RParen | Token::Comma)
                            | None
                    );
                    let val = if has_val {
                        Some(Box::new(self.parse_expr()?))
                    } else {
                        None
                    };
                    Ok(Expr::Break(span, val))
                }
            }
            Token::Return => {
                let has_val = !matches!(
                    self.peek(),
                    Some(
                        Token::Semicolon
                            | Token::RBrace
                            | Token::RParen
                            | Token::Comma
                            | Token::RBracket
                            | Token::Else
                    ) | None
                ) && !(self.peek_span().start >= span.end
                    && self.input[span.end..self.peek_span().start].contains('\n'));
                let val = if has_val {
                    Some(Box::new(self.parse_expr()?))
                } else {
                    None
                };
                Ok(Expr::Return(span, val))
            }
            Token::If => {
                let start = span.start;
                self.expect(Token::LParen)?;
                let mut is_comptime = false;
                if matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime") {
                    self.bump()?;
                    is_comptime = true;
                }
                let mut capture = None;
                let mut is_ptr_capture = false;
                if (self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut))
                    && matches!(
                        self.peek_next(),
                        Some(Token::Ident(_)) | Some(Token::ErrorKw)
                    )
                {
                    let cp_lexer = self.lexer.clone();
                    let cp_p1 = self.peeked.clone();
                    let cp_p2 = self.peeked2.clone();
                    let cp_errs = self.errors.len();
                    self.bump()?;
                    if let Ok(cap_id) = self.parse_ident() {
                        if self.peek() == Some(&Token::ColonAssign) {
                            self.bump()?;
                            is_ptr_capture = true;
                            capture = Some(cap_id);
                        } else {
                            self.lexer = cp_lexer;
                            self.peeked = cp_p1;
                            self.peeked2 = cp_p2;
                            self.errors.truncate(cp_errs);
                        }
                    } else {
                        self.lexer = cp_lexer;
                        self.peeked = cp_p1;
                        self.peeked2 = cp_p2;
                        self.errors.truncate(cp_errs);
                    }
                } else if matches!(self.peek(), Some(Token::Ident(_)) | Some(Token::ErrorKw))
                    && self.peek_next() == Some(&Token::ColonAssign)
                {
                    capture = Some(self.parse_ident()?);
                    self.expect(Token::ColonAssign)?;
                }
                let test = self.parse_expr()?;
                self.expect(Token::RParen)?;
                if capture.is_none() && self.peek() == Some(&Token::Pipe) {
                    let pipe_start = self.bump()?.1;
                    if self.peek() == Some(&Token::Star) || self.peek() == Some(&Token::Mut) {
                        self.bump()?;
                        is_ptr_capture = true;
                    }
                    let cap_id = self.parse_ident()?;
                    let pipe_end = self.expect(Token::Pipe)?;
                    self.errors.push(
                        Diagnostic::new(
                            "E0118",
                            "Zig-style '|capture|' syntax is abolished in 'if' expression",
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
                            "use 'if ({}{} := expr) ...'",
                            if is_ptr_capture { "*" } else { "" },
                            cap_id.sym
                        )),
                    );
                    capture = Some(cap_id);
                }
                let cons = if self.peek() == Some(&Token::LBrace) {
                    let block = self.parse_block_stmt()?;
                    Expr::LabeledBlock { label: None, block }
                } else {
                    self.parse_expr()?
                };
                let mut alt = None;
                if self.peek() == Some(&Token::Else) {
                    self.bump()?;
                    let alt_expr = if self.peek() == Some(&Token::LBrace) {
                        let block = self.parse_block_stmt()?;
                        Expr::LabeledBlock { label: None, block }
                    } else {
                        self.parse_expr()?
                    };
                    alt = Some(Box::new(alt_expr));
                }
                let end = self.peek_span().end;
                Ok(Expr::If {
                    span: Span { start, end },
                    test: Box::new(test),
                    cons: Box::new(cons),
                    alt,
                    is_comptime,
                    capture,
                    is_ptr_capture,
                })
            }
            Token::While => {
                let w = self.parse_while_stmt_after_kw(span.start)?;
                let else_expr = if self.peek() == Some(&Token::Else) {
                    self.bump()?;
                    let e = if self.peek() == Some(&Token::LBrace) {
                        let block = self.parse_block_stmt()?;
                        Expr::LabeledBlock { label: None, block }
                    } else {
                        self.parse_expr()?
                    };
                    Some(Box::new(e))
                } else {
                    None
                };
                Ok(Expr::While {
                    stmt: Box::new(w),
                    else_expr,
                })
            }
            Token::Slash => {
                let bytes = self.input.as_bytes();
                let mut pos = span.end;
                let mut in_class = false;
                let mut escaped = false;
                let mut closed = false;
                while pos < bytes.len() {
                    let b = bytes[pos];
                    if escaped {
                        escaped = false;
                        pos += 1;
                        continue;
                    }
                    if b == b'\\' {
                        escaped = true;
                        pos += 1;
                        continue;
                    }
                    if b == b'\n' || b == b'\r' {
                        break;
                    }
                    if b == b'[' {
                        in_class = true;
                        pos += 1;
                        continue;
                    }
                    if b == b']' && in_class {
                        in_class = false;
                        pos += 1;
                        continue;
                    }
                    if b == b'/' && !in_class {
                        closed = true;
                        break;
                    }
                    pos += 1;
                }
                if !closed {
                    return Err(ParseError::UnexpectedToken(Token::Slash, span));
                }
                let raw_pat = &self.input[span.end..pos];
                let mut pattern = String::with_capacity(raw_pat.len());
                let mut chars = raw_pat.chars().peekable();
                while let Some(c) = chars.next() {
                    if c == '\\' {
                        if chars.peek() == Some(&'/') {
                            chars.next();
                            pattern.push('/');
                        } else if let Some(next_c) = chars.next() {
                            if matches!(next_c, 'n' | 'r' | 't' | '0') {
                                pattern.push('\\');
                                pattern.push(next_c);
                            } else if next_c == '"' {
                                pattern.push('\\');
                                pattern.push('"');
                            } else {
                                pattern.push('\\');
                                pattern.push('\\');
                                pattern.push(next_c);
                            }
                        }
                    } else {
                        pattern.push(c);
                    }
                }
                pos += 1; // skip closing '/'
                let flag_start = pos;
                while pos < bytes.len() && bytes[pos].is_ascii_alphabetic() {
                    pos += 1;
                }
                let flags = self.input[flag_start..pos].to_string();
                self.lexer.reset_at(self.input, pos);
                self.peeked = None;
                self.peeked2 = None;
                self.prev_token_end = pos;
                Ok(Expr::New(CallExpr {
                    span: Span {
                        start: span.start,
                        end: pos,
                    },
                    callee: Box::new(Expr::Ident(Ident {
                        span: Span {
                            start: span.start,
                            end: pos,
                        },
                        sym: "RegExp".to_string(),
                    })),
                    args: vec![Expr::Lit(Lit::Str(pattern)), Expr::Lit(Lit::Str(flags))],
                    is_comptime: false,
                }))
            }
            _ => Err(ParseError::UnexpectedToken(token, span)),
        }
    }

    pub(super) fn parse_object_fields(
        &mut self,
        is_dot_brace: bool,
    ) -> ParseResult<Vec<ObjectField>> {
        let mut fields = Vec::new();
        let mut shorthand_indices: Vec<usize> = Vec::new();
        let mut has_positional = false;
        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
            if self.peek() == Some(&Token::DotDotDot) {
                self.bump()?;
                let val = self.parse_expr()?;
                fields.push(ObjectField {
                    key: None,
                    val,
                    is_spread: true,
                });
            } else if is_dot_brace {
                let is_dot_keyed = if self.peek() == Some(&Token::Dot)
                    && self.peek_next() != Some(&Token::LBrace)
                {
                    let checkpoint = self.lexer.clone();
                    let peeked_checkpoint = self.peeked.clone();
                    let peeked2_checkpoint = self.peeked2.clone();
                    let _ = self.bump();
                    let has_key = if self.parse_ident().is_ok() {
                        matches!(self.peek(), Some(Token::Colon) | Some(Token::Assign))
                    } else {
                        false
                    };
                    self.lexer = checkpoint;
                    self.peeked = peeked_checkpoint;
                    self.peeked2 = peeked2_checkpoint;
                    has_key
                } else {
                    false
                };
                let is_bare_keyed = if !is_dot_keyed {
                    let checkpoint = self.lexer.clone();
                    let peeked_checkpoint = self.peeked.clone();
                    let peeked2_checkpoint = self.peeked2.clone();
                    let has_key = if self.parse_ident().is_ok() {
                        self.peek() == Some(&Token::Colon)
                    } else {
                        false
                    };
                    self.lexer = checkpoint;
                    self.peeked = peeked_checkpoint;
                    self.peeked2 = peeked2_checkpoint;
                    has_key
                } else {
                    false
                };

                if is_dot_keyed || is_bare_keyed {
                    if is_dot_keyed {
                        self.bump()?;
                    }
                    let key = self.parse_ident()?;
                    self.bump()?;
                    let val = self.parse_expr()?;
                    fields.push(ObjectField {
                        key: Some(key),
                        val,
                        is_spread: false,
                    });
                } else {
                    let val = self.parse_expr()?;
                    fields.push(ObjectField {
                        key: None,
                        val,
                        is_spread: false,
                    });
                }
            } else {
                let checkpoint = self.lexer.clone();
                let peeked_checkpoint = self.peeked.clone();
                let peeked2_checkpoint = self.peeked2.clone();
                let prev_end_checkpoint = self.prev_token_end;

                let is_str = matches!(self.peek(), Some(Token::StringLiteral(_)));
                let parsed_key = if is_str {
                    let (tok, span) = self.bump()?;
                    if let Token::StringLiteral(s) = tok {
                        Some(Ident { span, sym: s })
                    } else {
                        unreachable!()
                    }
                } else {
                    self.parse_ident().ok()
                };

                if let Some(key) = parsed_key {
                    if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Assign) {
                        self.bump()?;
                        let val = self.parse_expr()?;
                        fields.push(ObjectField {
                            key: Some(key),
                            val,
                            is_spread: false,
                        });
                    } else if matches!(self.peek(), Some(Token::Comma) | Some(Token::RBrace))
                        && !is_str
                    {
                        // Shorthand `{ key }` -> `{ key: key }` (or positional if mixed with tuple exprs)
                        shorthand_indices.push(fields.len());
                        fields.push(ObjectField {
                            key: Some(key.clone()),
                            val: Expr::Ident(key),
                            is_spread: false,
                        });
                    } else {
                        // Positional tuple element (e.g. `{ fd, bufs.ptr, bufs.len }`)
                        self.lexer = checkpoint;
                        self.peeked = peeked_checkpoint;
                        self.peeked2 = peeked2_checkpoint;
                        self.prev_token_end = prev_end_checkpoint;
                        let val = self.parse_expr()?;
                        has_positional = true;
                        fields.push(ObjectField {
                            key: None,
                            val,
                            is_spread: false,
                        });
                    }
                } else {
                    self.lexer = checkpoint;
                    self.peeked = peeked_checkpoint;
                    self.peeked2 = peeked2_checkpoint;
                    self.prev_token_end = prev_end_checkpoint;
                    let val = self.parse_expr()?;
                    has_positional = true;
                    fields.push(ObjectField {
                        key: None,
                        val,
                        is_spread: false,
                    });
                }
            }
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
            }
        }
        self.expect(Token::RBrace)?;
        if has_positional {
            for idx in shorthand_indices {
                fields[idx].key = None;
            }
        }
        Ok(fields)
    }

    pub(super) fn parse_arrow_params(&mut self) -> ParseResult<Vec<Pattern>> {
        let mut params = Vec::new();
        while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
            if self.peek() == Some(&Token::Star)
                || self.peek() == Some(&Token::Mut)
                || self.peek() == Some(&Token::Let)
            {
                self.bump()?;
            }
            let ident = self.parse_ident()?;
            params.push(Pattern::Ident(ident));
            if self.peek() == Some(&Token::Colon) {
                self.bump()?;
                self.parse_type()?;
            }
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
            }
        }
        self.expect(Token::RParen)?;
        Ok(params)
    }
}
