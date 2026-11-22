use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

impl<'a> Parser<'a> {
    #[allow(dead_code)]
    pub(super) fn is_type_start(t: &Token) -> bool {
        matches!(
            t,
            Token::U8
                | Token::I8
                | Token::U16
                | Token::I16
                | Token::I32
                | Token::U32
                | Token::I64
                | Token::U64
                | Token::I128
                | Token::U128
                | Token::F32
                | Token::F64
                | Token::ISize
                | Token::USize
                | Token::String
                | Token::Void
                | Token::Any
                | Token::Bool
                | Token::CStr
                | Token::Ident(_)
                | Token::LBracket
                | Token::LBrace
                | Token::Ampersand
                | Token::Star
                | Token::Question
                | Token::Fn
                | Token::At
                | Token::ErrorKw
                | Token::Bang
                | Token::Struct
                | Token::Union
                | Token::Enum
        )
    }

    pub(super) fn is_error_type_rhs_start(&mut self) -> bool {
        if matches!(self.peek(), Some(Token::Ident(_)) | Some(Token::ErrorKw)) {
            return true;
        }
        if self.peek() == Some(&Token::LParen) {
            let checkpoint = self.lexer.clone();
            let p1 = self.peeked.clone();
            let p2 = self.peeked2.clone();
            let _ = self.bump();
            let mut depth = 1i32;
            let mut has_union_pipe = false;
            while let Some(tok) = self.peek() {
                match tok {
                    Token::LParen => depth += 1,
                    Token::RParen => {
                        depth -= 1;
                        if depth == 0 {
                            break;
                        }
                    }
                    Token::Pipe | Token::PipePipe if depth == 1 => {
                        has_union_pipe = true;
                        break;
                    }
                    Token::Semicolon => break,
                    _ => {}
                }
                let _ = self.bump();
            }
            self.lexer = checkpoint;
            self.peeked = p1;
            self.peeked2 = p2;
            if has_union_pipe {
                return true;
            }
        }
        if self.peek() == Some(&Token::LBrace) {
            let checkpoint = self.lexer.clone();
            let p1 = self.peeked.clone();
            let p2 = self.peeked2.clone();
            let _ = self.bump();
            let mut is_anon = false;
            if matches!(self.peek(), Some(Token::Ident(_))) {
                let _ = self.bump();
                if matches!(self.peek(), Some(Token::Comma) | Some(Token::RBrace)) {
                    is_anon = true;
                }
            }
            self.lexer = checkpoint.clone();
            self.peeked = p1.clone();
            self.peeked2 = p2.clone();
            return is_anon;
        }
        false
    }

    pub(super) fn parse_error_type_rhs(&mut self) -> ParseResult<ErrorType> {
        if self.peek() == Some(&Token::LParen) {
            self.bump()?;
            let err = self.parse_error_type_rhs()?;
            self.expect(Token::RParen)?;
            return Ok(err);
        }
        if self.peek() == Some(&Token::LBrace) {
            self.bump()?;
            let mut variants = Vec::new();
            while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                variants.push(self.parse_ident()?);
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
            }
            self.expect(Token::RBrace)?;
            return Ok(ErrorType::AnonSet(variants));
        }
        if matches!(self.peek(), Some(Token::Ident(_)) | Some(Token::ErrorKw)) {
            let mut first = self.parse_ident()?;
            if first.sym == "Box" && self.peek() == Some(&Token::Less) {
                self.bump()?;
                if self.peek() == Some(&Token::Dyn) {
                    self.bump()?;
                }
                let tr = self.parse_ident()?;
                self.expect(Token::Greater)?;
                return Ok(ErrorType::BoxDyn(tr));
            }
            while self.peek() == Some(&Token::Dot) {
                self.bump()?;
                let next = self.parse_ident()?;
                first.sym.push('.');
                first.sym.push_str(&next.sym);
            }
            if self.peek() == Some(&Token::Pipe) || self.peek() == Some(&Token::PipePipe) {
                let mut list = vec![first];
                while self.peek() == Some(&Token::Pipe) || self.peek() == Some(&Token::PipePipe) {
                    self.bump()?;
                    if self.peek() == Some(&Token::LBrace) {
                        self.bump()?;
                        let mut variants = Vec::new();
                        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                            variants.push(self.parse_ident()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        self.expect(Token::RBrace)?;
                        let anon_name = format!(
                            "error{{{}}}",
                            variants
                                .iter()
                                .map(|v| v.sym.as_str())
                                .collect::<Vec<_>>()
                                .join(", ")
                        );
                        list.push(Ident {
                            sym: anon_name,
                            span: Span { start: 0, end: 0 },
                        });
                        continue;
                    }
                    let mut next = self.parse_ident()?;
                    if (next.sym == "err" || next.sym == "error")
                        && self.peek() == Some(&Token::LBrace)
                    {
                        self.bump()?;
                        let mut variants = Vec::new();
                        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                            variants.push(self.parse_ident()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        self.expect(Token::RBrace)?;
                        let anon_name = format!(
                            "error{{{}}}",
                            variants
                                .iter()
                                .map(|v| v.sym.as_str())
                                .collect::<Vec<_>>()
                                .join(", ")
                        );
                        list.push(Ident {
                            sym: anon_name,
                            span: next.span,
                        });
                        continue;
                    }
                    while self.peek() == Some(&Token::Dot) {
                        self.bump()?;
                        let sub = self.parse_ident()?;
                        next.sym.push('.');
                        next.sym.push_str(&sub.sym);
                    }
                    list.push(next);
                }
                return Ok(ErrorType::Union(list));
            }
            if first.sym == "Error"
                || first.sym == "error"
                || first.sym == "err"
                || first.sym == "Err"
            {
                return Ok(ErrorType::DefaultError);
            }
            if first.sym == "AnyErr" || first.sym == "anyErr" || first.sym == "anyerror" {
                return Ok(ErrorType::AnyErr);
            }
            return Ok(ErrorType::Named(first));
        }
        Ok(ErrorType::DefaultError)
    }

    pub(super) fn parse_postfix_type_modifiers(&mut self, mut ty: Type) -> ParseResult<Type> {
        loop {
            if self.peek() == Some(&Token::Question) {
                self.bump()?;
                if ty == Type::Void {
                    ty = Type::ErrorUnion(Box::new(Type::Void));
                } else {
                    ty = Type::Optional(Box::new(ty));
                }
            } else if self.peek() == Some(&Token::Bang) {
                if self.peek_next() == Some(&Token::Question) {
                    if matches!(ty, Type::CStr) {
                        self.bump()?;
                        self.bump()?;
                        ty = Type::Optional(Box::new(ty));
                        continue;
                    }
                }
                break;
            } else if self.peek() == Some(&Token::LBracket) {
                self.bump()?;
                if self.peek() == Some(&Token::RBracket) {
                    self.bump()?;
                    ty = Type::Array(Box::new(ty));
                } else if let Some(Token::IntLiteral(n)) = self.peek() {
                    let size = ArraySize::Literal(*n as usize);
                    self.bump()?;
                    self.expect(Token::RBracket)?;
                    ty = Type::FixedArray(Box::new(ty), size);
                } else if let Some(Token::Ident(_)) = self.peek() {
                    let id = self.parse_ident()?;
                    self.expect(Token::RBracket)?;
                    ty = Type::FixedArray(Box::new(ty), ArraySize::Ident(id));
                } else {
                    self.expect(Token::RBracket)?;
                    ty = Type::Array(Box::new(ty));
                }
            } else {
                break;
            }
        }
        Ok(ty)
    }

    pub(super) fn parse_fn_type_sig(&mut self, is_c: bool) -> ParseResult<Type> {
        self.expect(Token::Fn)?;
        self.parse_fn_type_sig_body(is_c)
    }

    pub(super) fn parse_fn_type_sig_body(&mut self, mut is_c: bool) -> ParseResult<Type> {
        self.expect(Token::LParen)?;
        let mut param_types = Vec::new();
        while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
            let mut is_comptime = false;
            if matches!(self.peek(), Some(Token::Ident(id)) if id == "comptime") {
                self.bump()?;
                is_comptime = true;
            }
            if matches!(self.peek(), Some(Token::Ident(_)) | Some(Token::ErrorKw))
                && self.peek_next() == Some(&Token::Colon)
            {
                self.bump()?; // name
                self.bump()?; // colon
            }
            let mut ty = self.parse_type()?;
            if is_comptime {
                ty = Type::Ref(Ident {
                    span: self.peek_span(),
                    sym: format!("comptime {}", Self::type_to_str(&ty)),
                });
            }
            param_types.push(ty);
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
            }
        }
        self.expect(Token::RParen)?;
        if matches!(self.peek(), Some(Token::Ident(s)) if s == "callconv") {
            self.bump()?; // callconv
            if self.peek() == Some(&Token::LParen) {
                self.bump()?; // '('
                let mut conv_tokens = String::new();
                let mut depth = 1usize;
                while depth > 0 && self.peek().is_some() {
                    match self.peek() {
                        Some(Token::LParen) => {
                            depth += 1;
                            conv_tokens.push('(');
                            let _ = self.bump();
                        }
                        Some(Token::RParen) => {
                            depth -= 1;
                            if depth > 0 {
                                conv_tokens.push(')');
                            }
                            let _ = self.bump();
                        }
                        Some(Token::Dot) => {
                            conv_tokens.push('.');
                            let _ = self.bump();
                        }
                        Some(Token::Ident(id)) => {
                            conv_tokens.push_str(id);
                            let _ = self.bump();
                        }
                        _ => {
                            let _ = self.bump();
                        }
                    }
                }
                if conv_tokens == ".c"
                    || conv_tokens == "c"
                    || conv_tokens == ".C"
                    || conv_tokens == "C"
                {
                    is_c = true;
                }
            }
        }
        let ret_ty = if self.peek() == Some(&Token::Colon) || self.peek() == Some(&Token::Arrow) {
            self.bump()?;
            self.parse_type()?
        } else if self.peek().map(Self::is_type_start).unwrap_or(false) {
            self.parse_type()?
        } else {
            Type::Void
        };
        if is_c {
            Ok(Type::CFn(param_types, Box::new(ret_ty)))
        } else {
            Ok(Type::Fn(param_types, Box::new(ret_ty)))
        }
    }

    pub(super) fn parse_ptr_qualifiers(&mut self) -> ParseResult<(String, bool)> {
        let mut qual = String::new();
        let mut has_const = false;
        loop {
            if self.peek() == Some(&Token::Const) {
                self.bump()?;
                has_const = true;
                continue;
            }
            if let Some(Token::Ident(id)) = self.peek().cloned() {
                if (id == "align" || id == "addrspace") && self.peek_next() == Some(&Token::LParen)
                {
                    self.bump()?; // consume 'align' / 'addrspace'
                    self.bump()?; // consume '('
                    let inner_start = self.peek_span().start;
                    let mut paren_depth = 1i32;
                    let mut inner_end = inner_start;
                    while paren_depth > 0 && self.peek().is_some() {
                        let (tok, t_span) = self.bump()?;
                        match tok {
                            Token::LParen => paren_depth += 1,
                            Token::RParen => {
                                paren_depth -= 1;
                                if paren_depth == 0 {
                                    inner_end = t_span.start;
                                    break;
                                }
                            }
                            _ => {}
                        }
                    }
                    let arg_str = self.input[inner_start..inner_end].trim();
                    qual.push_str(&format!("{}({}) ", id, arg_str));
                    continue;
                }
                if id == "volatile" || id == "allowzero" {
                    let mut clone = self.clone_lookahead();
                    let _ = clone.bump();
                    if clone.peek() == Some(&Token::Const)
                        || clone.peek().map(Self::is_type_start).unwrap_or(false)
                    {
                        self.bump()?;
                        qual.push_str(&id);
                        qual.push(' ');
                        continue;
                    }
                }
            }
            break;
        }
        Ok((qual, has_const))
    }

    pub(super) fn apply_ptr_qualifiers(elem_ty: Type, qual: String, has_const: bool) -> Type {
        if qual.is_empty() {
            if has_const {
                Type::BorrowType {
                    inner: Box::new(elem_ty),
                    is_mut: false,
                    lifetimes: vec![],
                }
            } else {
                elem_ty
            }
        } else {
            let full_qual = if has_const {
                format!("{}const ", qual)
            } else {
                qual
            };
            Type::Aligned {
                qual: full_qual,
                inner: Box::new(elem_ty),
            }
        }
    }

    pub(super) fn parse_base_type(&mut self) -> ParseResult<Type> {
        let is_inline_container = matches!(
            self.peek(),
            Some(Token::Switch | Token::Struct | Token::Union | Token::Enum)
        ) || (matches!(
            self.peek(),
            Some(Token::Ident(s)) if s == "packed" || s == "extern"
        ) && matches!(
            self.peek_next(),
            Some(Token::Struct | Token::Union | Token::Enum)
        ));
        if is_inline_container {
            let start = self.peek_span().start;
            let mut brace_depth: i32 = 0;
            let mut paren_depth: i32 = 0;
            let mut had_brace = false;
            let mut end = start;
            while let Some(tok) = self.peek() {
                match tok {
                    Token::LBrace => {
                        brace_depth += 1;
                        had_brace = true;
                    }
                    Token::RBrace => {
                        brace_depth -= 1;
                        if brace_depth == 0 && paren_depth == 0 && had_brace {
                            let span = self.bump()?.1;
                            end = span.end;
                            break;
                        }
                    }
                    Token::LParen => paren_depth += 1,
                    Token::RParen => {
                        if paren_depth > 0 {
                            paren_depth -= 1;
                        }
                    }
                    _ => {}
                }
                let span = self.bump()?.1;
                end = span.end;
            }
            let raw_str = self.input[start..end].trim().to_string();
            let ty = Type::Ref(Ident {
                span: Span { start, end },
                sym: raw_str,
            });
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::Question) {
            let (_, q_span) = self.bump()?;
            let inner = self.parse_base_type()?;
            let inner_str = Self::type_to_str(&inner);
            let sugg_ty = if inner_str.starts_with('[')
                || inner_str.starts_with('*')
                || inner_str.contains(' ')
            {
                format!("({})?", inner_str)
            } else {
                format!("{}?", inner_str)
            };
            self.push_error(
                Diagnostic::new("E0115", "prefix optional type '?T' is abolished in CP")
                    .with_span(Span {
                        start: q_span.start,
                        end: self.prev_token_end.max(q_span.end),
                    })
                    .with_cause("Zig-style prefix '?T' is abolished in CP to unify with postfix optional 'T?'.")
                    .with_help("in CP, optional types place '?' at the end of the type (wrap slice/pointer types in parentheses, e.g. '([]const u8)?').")
                    .with_suggestion(format!("replace '?{}' with '{}'", inner_str, sugg_ty)),
            );
            let ty = Type::Optional(Box::new(inner));
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::Const) {
            self.bump()?;
            let inner = self.parse_base_type()?;
            let ty = Type::BorrowType {
                inner: Box::new(inner),
                is_mut: false,
                lifetimes: vec![],
            };
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::Dyn) {
            self.bump()?;
            let tr = self.parse_ident()?;
            return Ok(Type::BareDyn(tr));
        }
        if self.peek() == Some(&Token::Fn) {
            let ty = self.parse_fn_type_sig(false)?;
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::At) {
            let mut clone = self.clone_lookahead();
            let _ = clone.bump();
            if matches!(clone.peek(), Some(Token::Ident(s)) if s == "c") {
                let _ = clone.bump();
                if clone.peek() == Some(&Token::Fn) {
                    self.bump()?; // '@'
                    self.bump()?; // 'c'
                    let ty = self.parse_fn_type_sig(true)?;
                    return self.parse_postfix_type_modifiers(ty);
                }
                if clone.peek() == Some(&Token::LParen) {
                    let _ = clone.bump(); // consume outer '('
                    if clone.peek() == Some(&Token::LParen) {
                        self.bump()?; // '@'
                        self.bump()?; // 'c'
                        self.bump()?; // outer '('
                        let ty = self.parse_fn_type_sig_body(true)?;
                        self.expect(Token::RParen)?;
                        return self.parse_postfix_type_modifiers(ty);
                    }
                    // Also check @c (params): ret
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
                        self.bump()?; // '@'
                        self.bump()?; // 'c'
                        let ty = self.parse_fn_type_sig_body(true)?;
                        return self.parse_postfix_type_modifiers(ty);
                    }
                }
            }
            self.bump()?; // consume '@'
            let id = self.parse_ident()?;
            let mut sym = format!("@{}", id.sym);
            if self.peek() == Some(&Token::LParen) {
                self.bump()?;
                sym.push('(');
                let mut paren_depth = 1;
                let mut prev_end: Option<usize> = None;
                while paren_depth > 0 && self.peek().is_some() {
                    let (tok, t_span) = self.bump()?;
                    match tok {
                        Token::LParen => paren_depth += 1,
                        Token::RParen => paren_depth -= 1,
                        _ => {}
                    }
                    if paren_depth == 0 {
                        sym.push(')');
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
            while self.peek() == Some(&Token::Dot) {
                self.bump()?;
                let next = self.parse_ident()?;
                sym.push('.');
                sym.push_str(&next.sym);
            }
            let ty = Type::Ref(Ident { span: id.span, sym });
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::LParen) {
            let mut clone = self.clone_lookahead();
            let _ = clone.bump();
            let is_fn_ptr = match clone.peek() {
                Some(Token::Fn) => true,
                Some(Token::At) => {
                    let _ = clone.bump();
                    matches!(clone.peek(), Some(Token::Ident(s)) if s == "c")
                }
                _ => false,
            };
            if is_fn_ptr {
                self.bump()?; // consume '('
                let is_c = if self.peek() == Some(&Token::At) {
                    self.bump()?; // '@'
                    self.bump()?; // 'c'
                    true
                } else {
                    false
                };
                let ty = self.parse_fn_type_sig(is_c)?;
                self.expect(Token::RParen)?;
                return self.parse_postfix_type_modifiers(ty);
            } else {
                self.bump()?; // consume '('
                let inner = self.parse_type()?;
                self.expect(Token::RParen)?;
                return self.parse_postfix_type_modifiers(inner);
            }
        }
        if self.peek() == Some(&Token::Star) {
            let mut depth = 0;
            while self.peek() == Some(&Token::Star) {
                self.bump()?;
                depth += 1;
            }
            if self.peek() == Some(&Token::Bang) {
                self.bump()?; // consume '!'
                let ty = Type::RawPtr {
                    inner: Box::new(Type::Any),
                    depth,
                };
                return self.parse_postfix_type_modifiers(ty);
            }
            let (ptr_qual, has_const) = self.parse_ptr_qualifiers()?;
            let is_mut = !has_const;
            if self.peek() == Some(&Token::Bang) && (ptr_qual.is_empty() || !is_mut) {
                self.bump()?; // consume '!'
                let base_inner = if is_mut {
                    Type::Any
                } else {
                    Self::apply_ptr_qualifiers(Type::Any, ptr_qual, true)
                };
                let ty = Type::RawPtr {
                    inner: Box::new(base_inner),
                    depth,
                };
                return self.parse_postfix_type_modifiers(ty);
            }
            let mut lifetimes = Vec::new();
            if depth == 1 {
                if self.peek() == Some(&Token::LParen) {
                    let checkpoint = self.lexer.clone();
                    let peeked_checkpoint = self.peeked.clone();
                    let peeked2_checkpoint = self.peeked2.clone();
                    self.bump()?;
                    let mut is_lifetime = true;
                    let mut candidate_lifetimes = Vec::new();
                    while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                        if let Some(Token::Ident(_)) = self.peek() {
                            if let Ok(id) = self.parse_ident() {
                                candidate_lifetimes.push(id.sym);
                                if self.peek() == Some(&Token::Pipe) {
                                    self.bump()?;
                                }
                                continue;
                            }
                        }
                        is_lifetime = false;
                        break;
                    }
                    if is_lifetime
                        && self.peek() == Some(&Token::RParen)
                        && !candidate_lifetimes.is_empty()
                    {
                        self.bump()?;
                        if matches!(
                            self.peek(),
                            Some(Token::Ident(_))
                                | Some(Token::Star)
                                | Some(Token::Ampersand)
                                | Some(Token::LBracket)
                                | Some(Token::Const)
                                | Some(Token::String)
                                | Some(Token::I32)
                                | Some(Token::U32)
                                | Some(Token::I64)
                                | Some(Token::U64)
                                | Some(Token::I128)
                                | Some(Token::U128)
                                | Some(Token::F32)
                                | Some(Token::F64)
                                | Some(Token::USize)
                                | Some(Token::Bool)
                        ) {
                            lifetimes = candidate_lifetimes;
                        } else {
                            self.lexer = checkpoint;
                            self.peeked = peeked_checkpoint;
                            self.peeked2 = peeked2_checkpoint;
                        }
                    } else {
                        self.lexer = checkpoint;
                        self.peeked = peeked_checkpoint;
                        self.peeked2 = peeked2_checkpoint;
                    }
                } else if let Some(Token::Ident(_)) = self.peek() {
                    let checkpoint = self.lexer.clone();
                    let peeked_checkpoint = self.peeked.clone();
                    let peeked2_checkpoint = self.peeked2.clone();
                    let first_ident = self.parse_ident()?;
                    if matches!(
                        self.peek(),
                        Some(Token::Ident(_))
                            | Some(Token::String)
                            | Some(Token::I32)
                            | Some(Token::U32)
                            | Some(Token::I64)
                            | Some(Token::U64)
                            | Some(Token::I128)
                            | Some(Token::U128)
                            | Some(Token::F32)
                            | Some(Token::F64)
                            | Some(Token::USize)
                            | Some(Token::Bool)
                    ) {
                        lifetimes.push(first_ident.sym);
                    } else {
                        self.lexer = checkpoint;
                        self.peeked = peeked_checkpoint;
                        self.peeked2 = peeked2_checkpoint;
                    }
                }
            }
            let inner_is_paren = self.peek() == Some(&Token::LParen);
            let inner = self.parse_base_type()?;
            if self.peek() == Some(&Token::Bang) {
                let is_raw_ptr_bang = match self.peek_next() {
                    Some(Token::LBrace)
                    | Some(Token::RBrace)
                    | Some(Token::Comma)
                    | Some(Token::RParen)
                    | Some(Token::RBracket)
                    | Some(Token::Semicolon)
                    | Some(Token::Assign)
                    | Some(Token::Bang)
                    | Some(Token::Pipe)
                    | Some(Token::Pipeline)
                    | Some(Token::Greater)
                    | Some(Token::Question)
                    | None => true,
                    _ => false,
                };
                if is_raw_ptr_bang {
                    self.bump()?; // consume '!'
                    let (inner, has_unparen_opt) = if !inner_is_paren {
                        match inner {
                            Type::Optional(opt_inner) => (*opt_inner, true),
                            other => (other, false),
                        }
                    } else {
                        (inner, false)
                    };
                    let wrapped = if !ptr_qual.is_empty() {
                        Self::apply_ptr_qualifiers(inner, ptr_qual, !is_mut)
                    } else if !is_mut {
                        Type::BorrowType {
                            inner: Box::new(inner),
                            is_mut: false,
                            lifetimes: vec![],
                        }
                    } else {
                        inner
                    };
                    let mut ty = Type::RawPtr {
                        inner: Box::new(wrapped),
                        depth,
                    };
                    if has_unparen_opt {
                        ty = Type::Optional(Box::new(ty));
                    }
                    return self.parse_postfix_type_modifiers(ty);
                }
            }
            let mut ty = if !ptr_qual.is_empty() {
                Self::apply_ptr_qualifiers(inner, ptr_qual, !is_mut)
            } else {
                inner
            };
            for idx in 0..depth {
                ty = Type::BorrowType {
                    inner: Box::new(ty),
                    is_mut,
                    lifetimes: if idx == 0 { lifetimes.clone() } else { vec![] },
                };
            }
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::Ampersand) {
            self.bump()?;
            let mut is_mut = false;
            let mut lifetimes = Vec::new();
            if self.peek() == Some(&Token::Mut) {
                self.bump()?;
                is_mut = true;
            }
            if self.peek() == Some(&Token::LParen) {
                self.bump()?;
                while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                    let id = self.parse_ident()?;
                    lifetimes.push(id.sym);
                    if self.peek() == Some(&Token::Pipe) {
                        self.bump()?;
                    }
                }
                self.expect(Token::RParen)?;
                if self.peek() == Some(&Token::Mut) {
                    self.bump()?;
                    is_mut = true;
                }
            } else if let Some(Token::Ident(_)) = self.peek() {
                let checkpoint = self.lexer.clone();
                let peeked_checkpoint = self.peeked.clone();
                let peeked2_checkpoint = self.peeked2.clone();
                let first_ident = self.parse_ident()?;
                if self.peek() == Some(&Token::Mut) {
                    self.bump()?;
                    is_mut = true;
                    lifetimes.push(first_ident.sym);
                } else if matches!(
                    self.peek(),
                    Some(Token::Ident(_))
                        | Some(Token::String)
                        | Some(Token::I32)
                        | Some(Token::U32)
                        | Some(Token::I64)
                        | Some(Token::U64)
                        | Some(Token::I128)
                        | Some(Token::U128)
                        | Some(Token::F32)
                        | Some(Token::F64)
                        | Some(Token::USize)
                        | Some(Token::Bool)
                ) {
                    lifetimes.push(first_ident.sym);
                } else {
                    self.lexer = checkpoint;
                    self.peeked = peeked_checkpoint;
                    self.peeked2 = peeked2_checkpoint;
                }
            }
            let inner = self.parse_base_type()?;
            let ty = Type::BorrowType {
                inner: Box::new(inner),
                is_mut,
                lifetimes,
            };
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::LBracket) {
            let checkpoint = self.lexer.clone();
            let p1 = self.peeked.clone();
            let p2 = self.peeked2.clone();
            self.bump()?;
            if self.peek() == Some(&Token::Star) {
                self.bump()?;
                if self.peek() == Some(&Token::RBracket) {
                    self.bump()?;
                    let (qual, has_const) = self.parse_ptr_qualifiers()?;
                    let elem_ty = if self.peek() == Some(&Token::Bang) {
                        self.bump()?;
                        Type::Any
                    } else {
                        self.parse_base_type()?
                    };
                    let (elem_ty, has_elem_opt) = match elem_ty {
                        Type::Optional(inner) => (*inner, true),
                        other => (other, false),
                    };
                    let elem_ty = Self::apply_ptr_qualifiers(elem_ty, qual, has_const);
                    let mut is_optional = false;
                    if self.peek() == Some(&Token::Question)
                        && self.peek_next() == Some(&Token::Bang)
                    {
                        self.bump()?;
                        self.bump()?;
                        is_optional = true;
                    } else if self.peek() == Some(&Token::Bang) {
                        self.bump()?;
                        if self.peek() == Some(&Token::Question) {
                            self.bump()?;
                            is_optional = true;
                        }
                    }
                    let mut ty = Type::ManyRawPtr(Box::new(elem_ty));
                    if is_optional || has_elem_opt {
                        ty = Type::Optional(Box::new(ty));
                    }
                    return self.parse_postfix_type_modifiers(ty);
                } else if matches!(self.peek(), Some(Token::Ident(s)) if s == "c") {
                    self.bump()?;
                    if self.peek() == Some(&Token::Colon) {
                        self.bump()?;
                        let sentinel = match self.bump()?.0 {
                            Token::IntLiteral(n) => n.to_string(),
                            Token::Ident(s) => s,
                            Token::Null => "null".to_string(),
                            _ => "0".to_string(),
                        };
                        self.expect(Token::RBracket)?;
                        let (qual, has_const) = self.parse_ptr_qualifiers()?;
                        let elem_ty = self.parse_base_type()?;
                        let mut is_optional = false;
                        if self.peek() == Some(&Token::Question)
                            && self.peek_next() == Some(&Token::Bang)
                        {
                            self.bump()?;
                            self.bump()?;
                            is_optional = true;
                        } else if self.peek() == Some(&Token::Bang) {
                            self.bump()?;
                            if self.peek() == Some(&Token::Question) {
                                self.bump()?;
                                is_optional = true;
                            }
                        }
                        let (elem_ty, has_elem_opt) = match elem_ty {
                            Type::Optional(inner) => (*inner, true),
                            other => (other, false),
                        };
                        let elem_ty = Self::apply_ptr_qualifiers(elem_ty, qual, has_const);
                        let mut ty = Type::SentinelPtr(Box::new(elem_ty), sentinel);
                        if is_optional || has_elem_opt {
                            ty = Type::Optional(Box::new(ty));
                        }
                        return self.parse_postfix_type_modifiers(ty);
                    } else if self.peek() == Some(&Token::RBracket) {
                        self.bump()?;
                        let (qual, has_const) = self.parse_ptr_qualifiers()?;
                        let elem_ty = self.parse_base_type()?;
                        let (elem_ty, has_elem_opt) = match elem_ty {
                            Type::Optional(inner) => (*inner, true),
                            other => (other, false),
                        };
                        let elem_ty = Self::apply_ptr_qualifiers(elem_ty, qual, has_const);
                        let mut is_optional = false;
                        if self.peek() == Some(&Token::Question)
                            && self.peek_next() == Some(&Token::Bang)
                        {
                            self.bump()?;
                            self.bump()?;
                            is_optional = true;
                        } else if self.peek() == Some(&Token::Bang) {
                            self.bump()?;
                            if self.peek() == Some(&Token::Question) {
                                self.bump()?;
                                is_optional = true;
                            }
                        }
                        let mut ty = Type::CPtr(Box::new(elem_ty));
                        if is_optional || has_elem_opt {
                            ty = Type::Optional(Box::new(ty));
                        }
                        return self.parse_postfix_type_modifiers(ty);
                    }
                } else if self.peek() == Some(&Token::Colon) {
                    self.bump()?;
                    let sentinel = match self.bump()?.0 {
                        Token::IntLiteral(n) => n.to_string(),
                        Token::Ident(s) => s,
                        Token::Null => "null".to_string(),
                        _ => "0".to_string(),
                    };
                    self.expect(Token::RBracket)?;
                    let (qual, has_const) = self.parse_ptr_qualifiers()?;
                    let elem_ty = self.parse_base_type()?;
                    let mut is_optional = false;
                    if self.peek() == Some(&Token::Question)
                        && self.peek_next() == Some(&Token::Bang)
                    {
                        self.bump()?;
                        self.bump()?;
                        is_optional = true;
                    } else if self.peek() == Some(&Token::Bang) {
                        self.bump()?;
                        if self.peek() == Some(&Token::Question) {
                            self.bump()?;
                            is_optional = true;
                        }
                    }
                    let (elem_ty, has_elem_opt) = match elem_ty {
                        Type::Optional(inner) => (*inner, true),
                        other => (other, false),
                    };
                    let elem_ty = Self::apply_ptr_qualifiers(elem_ty, qual, has_const);
                    let mut ty = Type::SentinelPtr(Box::new(elem_ty), sentinel);
                    if is_optional || has_elem_opt {
                        ty = Type::Optional(Box::new(ty));
                    }
                    return self.parse_postfix_type_modifiers(ty);
                }
            }
            self.lexer = checkpoint.clone();
            self.peeked = p1.clone();
            self.peeked2 = p2.clone();
            let (_, span) = self.bump()?;
            if let Some(Token::IntLiteral(_)) | Some(Token::Ident(_)) = self.peek() {
                let (size_tok, size_span) = self.bump()?;
                let (array_size, size_str) = match size_tok {
                    Token::IntLiteral(n) => (ArraySize::Literal(n as usize), n.to_string()),
                    Token::Ident(s) => (
                        ArraySize::Ident(Ident {
                            span: size_span,
                            sym: s.clone(),
                        }),
                        s,
                    ),
                    _ => unreachable!(),
                };
                if self.peek() == Some(&Token::RBracket) {
                    self.bump()?;
                    let can_start_elem = !matches!(
                        self.peek(),
                        Some(
                            Token::Comma
                                | Token::RParen
                                | Token::RBracket
                                | Token::RBrace
                                | Token::Semicolon
                                | Token::Assign
                                | Token::Colon
                                | Token::FatArrow
                                | Token::LBrace
                        ) | None
                    );
                    if matches!(array_size, ArraySize::Literal(_)) || can_start_elem {
                        let has_const = if self.peek() == Some(&Token::Const) {
                            self.bump()?;
                            true
                        } else {
                            false
                        };
                        let elem_ty = if self.peek() == Some(&Token::Bang) {
                            self.bump()?;
                            Type::Any
                        } else {
                            self.parse_base_type()?
                        };
                        let ty = if has_const {
                            Type::Ref(Ident {
                                span,
                                sym: format!("[{}]const {}", size_str, Self::type_to_str(&elem_ty)),
                            })
                        } else {
                            Type::FixedArray(Box::new(elem_ty), array_size)
                        };
                        return self.parse_postfix_type_modifiers(ty);
                    }
                }
            }
            self.lexer = checkpoint.clone();
            self.peeked = p1.clone();
            self.peeked2 = p2.clone();
            self.bump()?;
            if self.peek() == Some(&Token::Colon) {
                self.bump()?;
                let sentinel = match self.bump()?.0 {
                    Token::IntLiteral(n) => n.to_string(),
                    Token::Ident(s) => s,
                    Token::Null => "null".to_string(),
                    _ => "0".to_string(),
                };
                self.expect(Token::RBracket)?;
                let has_const = if self.peek() == Some(&Token::Const) {
                    self.bump()?;
                    true
                } else {
                    false
                };
                let elem_ty = self.parse_base_type()?;
                let mut is_optional = false;
                if self.peek() == Some(&Token::Question) && self.peek_next() == Some(&Token::Bang) {
                    self.bump()?;
                    self.bump()?;
                    is_optional = true;
                } else if self.peek() == Some(&Token::Bang) {
                    self.bump()?;
                    if self.peek() == Some(&Token::Question) {
                        self.bump()?;
                        is_optional = true;
                    }
                }
                let (elem_ty, has_elem_opt) = match elem_ty {
                    Type::Optional(inner) => (*inner, true),
                    other => (other, false),
                };
                let s_val = if has_const {
                    format!("{}:const", sentinel)
                } else {
                    sentinel
                };
                let mut ty = Type::SentinelSlice(Box::new(elem_ty), s_val);
                if is_optional || has_elem_opt {
                    ty = Type::Optional(Box::new(ty));
                }
                return self.parse_postfix_type_modifiers(ty);
            }
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
                                | Token::String
                                | Token::Void
                                | Token::Any
                                | Token::Bool
                                | Token::CStr
                                | Token::Ident(_)
                                | Token::LBracket
                                | Token::LBrace
                                | Token::Star
                                | Token::Ampersand
                                | Token::TypeKw
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
                    return self.parse_postfix_type_modifiers(ty);
                }
                let ty = Type::Tuple(Vec::new());
                return self.parse_postfix_type_modifiers(ty);
            }
            if let Some(Token::IntLiteral(n)) = self.peek() {
                let sz = ArraySize::Literal(*n as usize);
                self.bump()?;
                if self.peek() == Some(&Token::RBracket) {
                    self.bump()?;
                    let elem_ty = self.parse_base_type()?;
                    let ty = Type::FixedArray(Box::new(elem_ty), sz);
                    return self.parse_postfix_type_modifiers(ty);
                }
            }
            if let Some(Token::Ident(_)) = self.peek() {
                let mut probe = self.clone_lookahead();
                if probe.bump().is_ok() && probe.peek() == Some(&Token::RBracket) {
                    let _ = probe.bump();
                    if matches!(
                        probe.peek(),
                        Some(
                            Token::U8
                                | Token::U16
                                | Token::U32
                                | Token::U64
                                | Token::I8
                                | Token::I16
                                | Token::I32
                                | Token::I64
                                | Token::USize
                                | Token::ISize
                                | Token::F32
                                | Token::F64
                                | Token::Bool
                                | Token::String
                                | Token::CStr
                                | Token::Star
                                | Token::Ampersand
                                | Token::LBracket
                        )
                    ) || matches!(probe.peek(), Some(Token::Ident(tid)) if tid.chars().next().map_or(false, |c| c.is_uppercase()))
                    {
                        let id = self.parse_ident()?;
                        self.expect(Token::RBracket)?;
                        let elem_ty = self.parse_base_type()?;
                        let ty = Type::FixedArray(Box::new(elem_ty), ArraySize::Ident(id));
                        return self.parse_postfix_type_modifiers(ty);
                    }
                }
            }
            let first = self.parse_type()?;
            if self.peek() == Some(&Token::Semicolon) {
                self.bump()?;
                let size = if let Some(Token::IntLiteral(n)) = self.peek() {
                    let sz = ArraySize::Literal(*n as usize);
                    self.bump()?;
                    sz
                } else if let Some(Token::Ident(_)) = self.peek() {
                    let id = self.parse_ident()?;
                    if self.peek() == Some(&Token::RBracket) {
                        ArraySize::Ident(id)
                    } else {
                        let mut expr_str = id.sym.clone();
                        let mut bracket_depth = 0;
                        while self.peek().is_some() {
                            if self.peek() == Some(&Token::RBracket) && bracket_depth == 0 {
                                break;
                            }
                            let (tok, t_span) = self.bump()?;
                            match tok {
                                Token::LBracket => bracket_depth += 1,
                                Token::RBracket => bracket_depth -= 1,
                                _ => {}
                            }
                            expr_str.push(' ');
                            expr_str.push_str(&self.input[t_span.start..t_span.end]);
                        }
                        ArraySize::Expr(expr_str)
                    }
                } else {
                    let mut expr_str = String::new();
                    let mut bracket_depth = 0;
                    while self.peek().is_some() {
                        if self.peek() == Some(&Token::RBracket) && bracket_depth == 0 {
                            break;
                        }
                        let (tok, t_span) = self.bump()?;
                        match tok {
                            Token::LBracket => bracket_depth += 1,
                            Token::RBracket => bracket_depth -= 1,
                            _ => {}
                        }
                        expr_str.push_str(&self.input[t_span.start..t_span.end]);
                    }
                    ArraySize::Expr(expr_str)
                };
                self.expect(Token::RBracket)?;
                let ty = Type::FixedArray(Box::new(first), size);
                return self.parse_postfix_type_modifiers(ty);
            }
            let mut elements = vec![first];
            if self.peek() == Some(&Token::Comma) {
                self.bump()?;
                while self.peek() != Some(&Token::RBracket) {
                    elements.push(self.parse_type()?);
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                    }
                }
            }
            self.expect(Token::RBracket)?;
            let ty = Type::Tuple(elements);
            return self.parse_postfix_type_modifiers(ty);
        }
        if self.peek() == Some(&Token::LBrace) {
            self.bump()?;
            let mut fields = Vec::new();
            while self.peek() != Some(&Token::RBrace) {
                let f_ident = self.parse_ident()?;
                self.expect(Token::Colon)?;
                let ty = self.parse_type()?;
                let mut f_meta = Vec::new();
                if matches!(self.peek(), Some(Token::Ident(s)) if s == "align")
                    && self.peek_next() == Some(&Token::LParen)
                {
                    let align_ident = self.parse_ident()?;
                    self.expect(Token::LParen)?;
                    let align_expr = self.parse_expr()?;
                    let end_span = self.expect(Token::RParen)?;
                    f_meta.push(MetaItem {
                        span: Span {
                            start: align_ident.span.start,
                            end: end_span.end,
                        },
                        name: align_ident,
                        args: vec![MetaArg::Positional(align_expr)],
                    });
                }
                fields.push(Field {
                    span: f_ident.span,
                    ident: f_ident,
                    ty,
                    meta: f_meta,
                    default: None,
                    is_pub: true,
                    is_static: false,
                });
                if self.peek() == Some(&Token::Comma) {
                    self.bump()?;
                }
            }
            self.expect(Token::RBrace)?;
            return Ok(Type::Object(fields));
        }
        let is_legacy_error_union = if self.peek() == Some(&Token::Bang) {
            self.bump()?;
            true
        } else {
            false
        };
        let (token, span) = self.bump()?;
        let mut ty = match token {
            Token::U8 => Type::U8,
            Token::I8 => Type::I8,
            Token::U16 => Type::U16,
            Token::I16 => Type::I16,
            Token::I32 => Type::I32,
            Token::U32 => Type::U32,
            Token::I64 => Type::I64,
            Token::U64 => Type::U64,
            Token::I128 => Type::I128,
            Token::U128 => Type::U128,
            Token::ISize => Type::ISize,
            Token::USize => Type::USize,
            Token::F32 => Type::F32,
            Token::F64 => Type::F64,
            Token::String => Type::String,
            Token::Void => Type::Void,
            Token::Any => {
                self.push_error(
                    Diagnostic::new("E0119", "'any' type is abolished in CP")
                        .with_span(span)
                        .with_cause("CP is a strongly typed language and does not support dynamic 'any' types.")
                        .with_help("use a concrete type, 'auto' for compile-time generic deduction, or a tagged union / trait.")
                        .with_suggestion("replace 'any' with 'auto' or a concrete type"),
                );
                Type::Auto
            }
            Token::Auto => Type::Auto,
            Token::Bool => Type::Bool,
            Token::CStr => Type::CStr,
            Token::ErrorKw => Type::Ref(Ident {
                span,
                sym: "Error".to_string(),
            }),
            Token::TypeKw => Type::Ref(Ident {
                span,
                sym: "type".to_string(),
            }),
            Token::Meta => {
                let mut sym = "meta".to_string();
                loop {
                    if self.peek() == Some(&Token::Dot) {
                        self.bump()?;
                        let next = self.parse_ident()?;
                        sym.push('.');
                        sym.push_str(&next.sym);
                    } else if self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        sym.push('(');
                        let mut paren_depth = 1;
                        let mut prev_end: Option<usize> = None;
                        while paren_depth > 0 && self.peek().is_some() {
                            let (tok, t_span) = self.bump()?;
                            match tok {
                                Token::LParen => paren_depth += 1,
                                Token::RParen => paren_depth -= 1,
                                _ => {}
                            }
                            if paren_depth == 0 {
                                sym.push(')');
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
                    } else if self.peek() == Some(&Token::Less) {
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
                    } else {
                        break;
                    }
                }
                Type::Ref(Ident { span, sym })
            }
            Token::Ident(mut sym) => {
                if sym == "cstr" {
                    Type::CStr
                } else if sym == "pid" {
                    Type::PID
                } else if sym == "auto" {
                    Type::Auto
                } else if sym == "Shared" && self.peek() == Some(&Token::Less) {
                    self.bump()?;
                    let inner = self.parse_type()?;
                    self.expect(Token::Greater)?;
                    Type::Shared(Box::new(inner))
                } else if (sym == "Mutex"
                    || sym == "RwLock"
                    || sym == "Atomic"
                    || sym == "LockGuard"
                    || sym == "ReadGuard"
                    || sym == "WriteGuard")
                    && self.peek() == Some(&Token::Less)
                {
                    self.bump()?;
                    let inner = self.parse_type()?;
                    self.expect(Token::Greater)?;
                    let inner_str = Self::type_to_str(&inner);
                    Type::Ref(Ident {
                        span,
                        sym: format!("{}<{}>", sym, inner_str),
                    })
                } else if sym == "Box"
                    && self.peek() == Some(&Token::Less)
                    && matches!(self.peek_next(), Some(Token::Dyn) | Some(Token::Ident(_)))
                {
                    self.bump()?;
                    let had_dyn = if self.peek() == Some(&Token::Dyn) {
                        self.bump()?;
                        true
                    } else {
                        false
                    };
                    let tr = self.parse_ident()?;
                    if had_dyn || self.peek() == Some(&Token::Greater) {
                        self.expect(Token::Greater)?;
                        Type::BoxDyn(tr)
                    } else {
                        let mut full_sym = format!("Box<{}", tr.sym);
                        let mut angle_depth = 1;
                        let mut prev_end: Option<usize> = Some(tr.span.end);
                        while angle_depth > 0 && self.peek().is_some() {
                            let (tok, t_span) = self.bump()?;
                            match tok {
                                Token::Less => angle_depth += 1,
                                Token::Greater => angle_depth -= 1,
                                _ => {}
                            }
                            if angle_depth == 0 {
                                full_sym.push('>');
                                break;
                            }
                            if let Some(pe) = prev_end {
                                if t_span.start > pe {
                                    full_sym.push(' ');
                                }
                            }
                            full_sym.push_str(&self.input[t_span.start..t_span.end]);
                            prev_end = Some(t_span.end);
                        }
                        Type::Ref(Ident {
                            span,
                            sym: full_sym,
                        })
                    }
                } else if (sym == "Map" || sym == "Dict" || sym == "HashMap" || sym == "BTreeMap")
                    && self.peek() == Some(&Token::Less)
                {
                    self.bump()?;
                    let k = self.parse_type()?;
                    self.expect(Token::Comma)?;
                    let v = self.parse_type()?;
                    self.expect(Token::Greater)?;
                    if sym == "Dict" {
                        Type::Dict(Box::new(k), Box::new(v))
                    } else {
                        Type::Map(Box::new(k), Box::new(v))
                    }
                } else if (sym == "Set" || sym == "HashSet" || sym == "BTreeSet")
                    && self.peek() == Some(&Token::Less)
                {
                    self.bump()?;
                    let elem = self.parse_type()?;
                    self.expect(Token::Greater)?;
                    Type::Set(Box::new(elem))
                } else if (sym == "Array" || sym == "Vec") && self.peek() == Some(&Token::Less) {
                    self.bump()?;
                    let elem = self.parse_type()?;
                    self.expect(Token::Greater)?;
                    Type::Array(Box::new(elem))
                } else {
                    let mut sym = sym;
                    loop {
                        if self.peek() == Some(&Token::Dot) {
                            self.bump()?;
                            let next = self.parse_ident()?;
                            sym.push('.');
                            sym.push_str(&next.sym);
                        } else if self.peek() == Some(&Token::LParen) {
                            self.bump()?;
                            sym.push('(');
                            let mut paren_depth = 1;
                            let mut prev_end: Option<usize> = None;
                            while paren_depth > 0 && self.peek().is_some() {
                                let (tok, t_span) = self.bump()?;
                                match tok {
                                    Token::LParen => paren_depth += 1,
                                    Token::RParen => paren_depth -= 1,
                                    _ => {}
                                }
                                if paren_depth == 0 {
                                    sym.push(')');
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
                        } else if self.peek() == Some(&Token::Less) {
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
                        } else {
                            break;
                        }
                    }
                    Type::Ref(Ident { span, sym })
                }
            }
            _ => return Err(ParseError::UnexpectedToken(token, span)),
        };
        ty = self.parse_postfix_type_modifiers(ty)?;
        if is_legacy_error_union {
            ty = Type::ErrorUnion(Box::new(ty));
        }
        Ok(ty)
    }

    pub(super) fn parse_type(&mut self) -> ParseResult<Type> {
        if self.peek() == Some(&Token::If) {
            self.bump()?; // consume 'if'
            self.expect(Token::LParen)?;
            let cond = self.parse_expr()?;
            self.expect(Token::RParen)?;
            let cons = self.parse_type()?;
            self.expect(Token::Else)?;
            let alt = self.parse_type()?;
            return Ok(Type::Conditional(
                Box::new(cond),
                Box::new(cons),
                Box::new(alt),
            ));
        }
        if self.peek() == Some(&Token::At) {
            let mut clone = self.clone_lookahead();
            let _ = clone.bump();
            if clone.peek() == Some(&Token::If)
                || matches!(clone.peek(), Some(Token::Ident(s)) if s == "if")
            {
                self.bump()?; // consume '@'
                self.bump()?; // consume 'if'
                self.expect(Token::LParen)?;
                let cond = self.parse_expr()?;
                self.expect(Token::RParen)?;
                let cons = self.parse_type()?;
                self.expect(Token::Else)?;
                let alt = self.parse_type()?;
                return Ok(Type::Conditional(
                    Box::new(cond),
                    Box::new(cons),
                    Box::new(alt),
                ));
            }
        }
        if self.peek() == Some(&Token::Bang) {
            self.bump()?;
            if self.peek() == Some(&Token::LParen) && self.is_error_type_rhs_start() {
                let err = self.parse_error_type_rhs()?;
                let ok_ty = if self.peek().map(Self::is_type_start).unwrap_or(false)
                    && !matches!(
                        self.peek(),
                        Some(Token::LBrace | Token::Semicolon | Token::Comma | Token::RParen)
                    ) {
                    self.parse_type()?
                } else {
                    Type::Void
                };
                return Ok(Type::ResultType {
                    ok: Box::new(ok_ty),
                    err,
                });
            }
            if matches!(
                self.peek(),
                Some(Token::LBrace)
                    | Some(Token::Semicolon)
                    | Some(Token::Comma)
                    | Some(Token::RParen)
                    | Some(Token::RBracket)
                    | Some(Token::Assign)
                    | None
            ) {
                return Ok(Type::ErrorUnion(Box::new(Type::Void)));
            }
            let inner = self.parse_type()?;
            return Ok(Type::ErrorUnion(Box::new(inner)));
        }
        let base = self.parse_base_type()?;
        if self.peek() == Some(&Token::Bang) {
            self.bump()?;
            // Check if LHS is an error set (Zig-style: anyerror!T or SomeSet.Error!T or error{...}!T)
            if let Type::Ref(id) = &base {
                if id.sym == "anyerror" || id.sym == "AnyErr" {
                    let ok_ty = self.parse_type()?;
                    return Ok(Type::ResultType {
                        ok: Box::new(ok_ty),
                        err: ErrorType::AnyErr,
                    });
                }
                let rhs_is_non_error_value_type = matches!(
                    self.peek(),
                    Some(
                        Token::Void
                            | Token::Bool
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
                            | Token::String
                            | Token::CStr
                            | Token::Star
                            | Token::Question
                            | Token::LBracket
                    )
                );
                if id.sym.ends_with("Error")
                    || id.sym.ends_with("Err")
                    || id.sym.ends_with("OOM")
                    || id.sym.starts_with("error{")
                    || rhs_is_non_error_value_type
                {
                    let ok_ty = self.parse_type()?;
                    return Ok(Type::ResultType {
                        ok: Box::new(ok_ty),
                        err: ErrorType::Named(id.clone()),
                    });
                }
            }
            if self.is_error_type_rhs_start() {
                let err = self.parse_error_type_rhs()?;
                return Ok(Type::ResultType {
                    ok: Box::new(base),
                    err,
                });
            } else {
                return Ok(Type::ErrorUnion(Box::new(base)));
            }
        }
        Ok(base)
    }

    pub fn type_to_str(ty: &Type) -> String {
        match ty {
            Type::U8 => "u8".to_string(),
            Type::I8 => "i8".to_string(),
            Type::U16 => "u16".to_string(),
            Type::I16 => "i16".to_string(),
            Type::I32 => "i32".to_string(),
            Type::U32 => "u32".to_string(),
            Type::I64 => "i64".to_string(),
            Type::U64 => "u64".to_string(),
            Type::I128 => "i128".to_string(),
            Type::U128 => "u128".to_string(),
            Type::ISize => "isize".to_string(),
            Type::USize => "usize".to_string(),
            Type::F32 => "f32".to_string(),
            Type::F64 => "f64".to_string(),
            Type::Bool => "bool".to_string(),
            Type::String => "string".to_string(),
            Type::Void => "void".to_string(),
            Type::Any => "any".to_string(),
            Type::Auto => "auto".to_string(),
            Type::PID => "PID".to_string(),
            Type::Array(inner) => format!("[{}]", Self::type_to_str(inner)),
            Type::FixedArray(inner, n) => format!("[{}; {}]", Self::type_to_str(inner), n),
            Type::RawPtr { inner, depth } => {
                let inner_str = match inner.as_ref() {
                    Type::Optional(_) => format!("({})", Self::type_to_str(inner)),
                    Type::BorrowType {
                        inner: b_in,
                        is_mut: false,
                        ..
                    } if matches!(b_in.as_ref(), Type::Optional(_)) => {
                        format!("const ({})", Self::type_to_str(b_in))
                    }
                    Type::Aligned { qual, inner: a_in }
                        if matches!(a_in.as_ref(), Type::Optional(_)) =>
                    {
                        format!("{}({})", qual, Self::type_to_str(a_in))
                    }
                    _ => Self::type_to_str(inner),
                };
                format!("{}{}!", "*".repeat(*depth), inner_str)
            }
            Type::BorrowType { inner, is_mut, .. } => {
                if *is_mut {
                    format!("*{}", Self::type_to_str(inner))
                } else {
                    format!("const {}", Self::type_to_str(inner))
                }
            }
            Type::ManyRawPtr(inner) => format!("[*]{}", Self::type_to_str(inner)),
            Type::CPtr(inner) => format!("[*c]{}", Self::type_to_str(inner)),
            Type::SentinelPtr(inner, n) => format!("[*:{n}]{}", Self::type_to_str(inner)),
            Type::SentinelSlice(inner, n) => {
                if let Some(s) = n.strip_suffix(":const") {
                    format!("[:{s}]const {}", Self::type_to_str(inner))
                } else {
                    format!("[:{n}]{}", Self::type_to_str(inner))
                }
            }
            Type::Optional(inner) => match inner.as_ref() {
                Type::BorrowType { .. } => format!("({})?", Self::type_to_str(inner)),
                _ => format!("{}?", Self::type_to_str(inner)),
            },
            Type::Shared(inner) => format!("Shared<{}>", Self::type_to_str(inner)),
            Type::Aligned { qual, inner } => format!("{}{}", qual, Self::type_to_str(inner)),
            Type::Ref(id) => id.sym.clone(),
            Type::Conditional(_cond, cons, alt) => {
                format!(
                    "@if (...) {} else {}",
                    Self::type_to_str(cons),
                    Self::type_to_str(alt)
                )
            }
            _ => "any".to_string(),
        }
    }

    pub(crate) fn parse_standalone_type(input: &'a str) -> Option<Type> {
        let mut parser = Self::new(input);
        let ty = parser.parse_type().ok()?;
        if parser.peek().is_none() {
            Some(ty)
        } else {
            None
        }
    }
}
