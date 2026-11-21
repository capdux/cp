use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

impl<'a> Parser<'a> {
    pub(super) fn parse_pattern(&mut self) -> ParseResult<Pattern> {
        let first = self.parse_single_pattern()?;
        if self.peek() == Some(&Token::Pipe)
            || (self.in_switch_arm > 0
                && self.in_paren_pattern == 0
                && self.peek() == Some(&Token::Comma))
        {
            let mut pats = vec![first];
            while self.peek() == Some(&Token::Pipe)
                || (self.in_switch_arm > 0
                    && self.in_paren_pattern == 0
                    && self.peek() == Some(&Token::Comma))
            {
                self.bump()?;
                pats.push(self.parse_single_pattern()?);
            }
            return Ok(Pattern::Or(pats));
        }
        Ok(first)
    }

    pub(super) fn try_parse_range_bound(&mut self) -> Option<RangeBound> {
        let (tok, span) = self.bump().ok()?;
        match tok {
            Token::Minus => {
                let (next_tok, next_span) = self.bump().ok()?;
                match next_tok {
                    Token::IntLiteral(v) => Some(RangeBound::Int(-v)),
                    Token::Ident(s) => Some(RangeBound::NegIdent(Ident {
                        span: Span {
                            start: span.start,
                            end: next_span.end,
                        },
                        sym: s,
                    })),
                    _ => None,
                }
            }
            Token::IntLiteral(v) => Some(RangeBound::Int(v)),
            Token::Ident(s) => Some(RangeBound::Ident(Ident { span, sym: s })),
            _ => None,
        }
    }

    pub(super) fn parse_single_pattern(&mut self) -> ParseResult<Pattern> {
        if self.in_switch_arm > 0 && self.in_paren_pattern == 0 {
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
                        | Token::TypeKw
                        | Token::String
                        | Token::CStr
                        | Token::Any
                        | Token::Auto
                )
            ) {
                let ty = self.parse_type()?;
                return Ok(Pattern::Type(ty));
            }
            if matches!(
                self.peek(),
                Some(Token::Star | Token::LBracket | Token::Question)
            ) && !(self.peek() == Some(&Token::Star)
                && matches!(
                    self.peek_next(),
                    Some(Token::FatArrow | Token::Comma | Token::Pipe | Token::If) | None
                ))
            {
                let checkpoint = self.lexer.clone();
                let p1 = self.peeked.clone();
                let p2 = self.peeked2.clone();
                let err_len = self.errors.len();
                if let Ok(ty) = self.parse_type() {
                    if matches!(
                        self.peek(),
                        Some(Token::FatArrow | Token::Comma | Token::Pipe | Token::If)
                    ) {
                        return Ok(Pattern::Type(ty));
                    }
                }
                self.lexer = checkpoint;
                self.peeked = p1;
                self.peeked2 = p2;
                self.errors.truncate(err_len);
            }
        }
        match self.peek() {
            Some(Token::At) => {
                let expr = self.parse_postfix_expr()?;
                Ok(Pattern::Expr(Box::new(expr)))
            }
            Some(Token::Mut) => {
                let (_, span) = self.bump()?;
                return Err(ParseError::UnexpectedToken(Token::Mut, span));
            }
            Some(Token::Null) => {
                self.bump()?;
                Ok(Pattern::Null)
            }
            Some(Token::DotDotDot) => {
                self.bump()?;
                if !matches!(
                    self.peek(),
                    Some(
                        Token::Comma
                            | Token::RBracket
                            | Token::RBrace
                            | Token::RParen
                            | Token::Pipe
                            | Token::FatArrow
                    ) | None
                ) {
                    let id = self.parse_ident()?;
                    Ok(Pattern::Rest(Box::new(Pattern::Ident(id))))
                } else {
                    Ok(Pattern::Rest(Box::new(Pattern::Wildcard)))
                }
            }
            Some(Token::LBracket) => {
                let mut elements = Vec::new();
                self.bump()?;
                self.in_paren_pattern += 1;
                while self.peek() != Some(&Token::RBracket) {
                    elements.push(self.parse_pattern()?);
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                    }
                }
                self.expect(Token::RBracket)?;
                self.in_paren_pattern -= 1;
                Ok(Pattern::Tuple(elements))
            }
            Some(Token::LBrace) => {
                let _start = self.bump()?.1.start;
                self.in_paren_pattern += 1;
                let mut props = Vec::new();
                while self.peek() != Some(&Token::RBrace) {
                    if self.peek() == Some(&Token::DotDotDot) {
                        let dot_span = self.bump()?.1;
                        let (key, val) =
                            if !matches!(self.peek(), Some(Token::Comma | Token::RBrace) | None) {
                                if self.peek() == Some(&Token::Star) {
                                    let (_, s) = self.bump()?;
                                    (
                                        Ident {
                                            span: s,
                                            sym: "*".to_string(),
                                        },
                                        Some(Pattern::Wildcard),
                                    )
                                } else {
                                    let rest_ident = self.parse_ident()?;
                                    (rest_ident, None)
                                }
                            } else {
                                (
                                    Ident {
                                        span: dot_span,
                                        sym: "*".to_string(),
                                    },
                                    Some(Pattern::Wildcard),
                                )
                            };
                        props.push(ObjectPatProp {
                            key,
                            val,
                            is_rest: true,
                            mode: DestructureMode::ReadOnly,
                        });
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                        continue;
                    }
                    let prop_mode = if self.peek() == Some(&Token::Star) {
                        self.bump()?;
                        DestructureMode::MutBorrow
                    } else if matches!(self.peek(), Some(Token::Move | Token::Own)) {
                        self.bump()?;
                        DestructureMode::Move
                    } else {
                        DestructureMode::ReadOnly
                    };
                    let key = self.parse_ident()?;
                    if key.sym == "mut"
                        && self.peek() != Some(&Token::Colon)
                        && matches!(self.peek(), Some(Token::Ident(_)))
                    {
                        return Err(ParseError::UnexpectedToken(Token::Mut, key.span));
                    }
                    let mut val = None;
                    if self.peek() == Some(&Token::Colon) {
                        self.bump()?;
                        val = Some(self.parse_pattern()?);
                    }
                    props.push(ObjectPatProp {
                        key,
                        val,
                        is_rest: false,
                        mode: prop_mode,
                    });
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                    }
                }
                let _end = self.expect(Token::RBrace)?.end;
                self.in_paren_pattern -= 1;
                Ok(Pattern::Object(props))
            }
            Some(Token::Dot) => {
                self.bump()?;
                let variant_name = self.parse_ident()?;
                let mut fields = None;
                let mut tuple_pats = None;
                if self.peek() == Some(&Token::LParen) {
                    self.bump()?;
                    self.in_paren_pattern += 1;
                    let mut pats = Vec::new();
                    while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                        pats.push(self.parse_pattern()?);
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    self.expect(Token::RParen)?;
                    self.in_paren_pattern -= 1;
                    tuple_pats = Some(pats);
                } else if self.peek() == Some(&Token::LBracket) {
                    self.bump()?;
                    self.in_paren_pattern += 1;
                    let mut pats = Vec::new();
                    while self.peek() != Some(&Token::RBracket) && self.peek().is_some() {
                        pats.push(self.parse_pattern()?);
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    self.expect(Token::RBracket)?;
                    self.in_paren_pattern -= 1;
                    tuple_pats = Some(pats);
                } else if self.peek() == Some(&Token::LBrace) {
                    self.bump()?;
                    self.in_paren_pattern += 1;
                    let mut props = Vec::new();
                    while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                        if self.peek() == Some(&Token::DotDotDot) {
                            let dot_span = self.bump()?.1;
                            let (key, val) = if !matches!(
                                self.peek(),
                                Some(Token::Comma | Token::RBrace) | None
                            ) {
                                let id = self.parse_ident()?;
                                (id, None)
                            } else {
                                (
                                    Ident {
                                        span: dot_span,
                                        sym: "_".to_string(),
                                    },
                                    None,
                                )
                            };
                            props.push(ObjectPatProp {
                                key,
                                val,
                                is_rest: true,
                                mode: DestructureMode::ReadOnly,
                            });
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                            continue;
                        }
                        let key = self.parse_ident()?;
                        let mut val = None;
                        if self.peek() == Some(&Token::Colon) {
                            self.bump()?;
                            val = Some(self.parse_pattern()?);
                        }
                        props.push(ObjectPatProp {
                            key,
                            val,
                            is_rest: false,
                            mode: DestructureMode::ReadOnly,
                        });
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    self.expect(Token::RBrace)?;
                    self.in_paren_pattern -= 1;
                    fields = Some(props);
                } else if let Some(Token::Ident(_)) = self.peek() {
                    let key = self.parse_ident()?;
                    tuple_pats = Some(vec![Pattern::Ident(key)]);
                }
                Ok(Pattern::Enum(EnumPattern {
                    span: Span { start: 0, end: 0 },
                    enum_name: Ident {
                        span: Span { start: 0, end: 0 },
                        sym: "".to_string(),
                    },
                    variant_name,
                    fields,
                    tuple_pats,
                }))
            }
            _ => {
                if self.peek() == Some(&Token::Star)
                    || self.peek() == Some(&Token::Bang)
                    || self.peek() == Some(&Token::Else)
                {
                    self.bump()?;
                    return Ok(Pattern::Wildcard);
                }
                // Try parsing range pattern e.g. 1...ARES_ENOSERVER or -ARES_ENOSERVER...-1 or 1..=10
                {
                    let mut probe = self.clone_lookahead();
                    if let Some(start_bound) = probe.try_parse_range_bound() {
                        if matches!(
                            probe.peek(),
                            Some(Token::DotDotDot) | Some(Token::DotDotEq) | Some(Token::DotDot)
                        ) {
                            let op = probe.bump().unwrap().0;
                            let inclusive = op == Token::DotDotDot || op == Token::DotDotEq;
                            if let Some(end_bound) = probe.try_parse_range_bound() {
                                *self = probe;
                                return Ok(Pattern::Range {
                                    start: start_bound,
                                    end: end_bound,
                                    inclusive,
                                });
                            }
                        }
                    }
                }
                if self.peek() == Some(&Token::Minus) {
                    let mut probe = self.clone_lookahead();
                    let _ = probe.bump();
                    if let Some(Token::IntLiteral(_)) = probe.peek() {
                        self.bump()?;
                        if let (Token::IntLiteral(v), _) = self.bump()? {
                            return Ok(Pattern::Lit(Lit::Int(-v)));
                        }
                    }
                }
                if let Some(Token::IntLiteral(_)) = self.peek() {
                    if let (Token::IntLiteral(v), _) = self.bump()? {
                        return Ok(Pattern::Lit(Lit::Int(v)));
                    }
                }
                if let Some(Token::StringLiteral(_)) = self.peek() {
                    if let (Token::StringLiteral(s), _) = self.bump()? {
                        return Ok(Pattern::Lit(Lit::Str(s)));
                    }
                }
                let first_id = self.parse_ident()?;
                if self.peek() == Some(&Token::ColonAssign) {
                    self.bump()?;
                    let subpat = self.parse_pattern()?;
                    return Ok(Pattern::Binding {
                        ident: first_id,
                        subpat: Box::new(subpat),
                    });
                }
                if self.in_switch_arm > 0 && self.peek() == Some(&Token::Colon) {
                    self.bump()?;
                    let ty = self.parse_type()?;
                    return Ok(Pattern::TypeMatch {
                        ident: first_id,
                        ty,
                    });
                }
                if first_id.sym == "true" {
                    return Ok(Pattern::Lit(Lit::Bool(true)));
                }
                if first_id.sym == "false" {
                    return Ok(Pattern::Lit(Lit::Bool(false)));
                }
                if (first_id.sym == "Err" || first_id.sym == "err" || first_id.sym == "error")
                    && self.peek() == Some(&Token::LParen)
                {
                    self.bump()?;
                    self.in_paren_pattern += 1;
                    let first_p = self.parse_pattern()?;
                    if self.peek() == Some(&Token::Comma) {
                        self.bump()?;
                        let mut pats = vec![first_p];
                        while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                            pats.push(self.parse_pattern()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        self.expect(Token::RParen)?;
                        self.in_paren_pattern -= 1;
                        return Ok(Pattern::Err(Box::new(Pattern::Tuple(pats))));
                    } else {
                        self.expect(Token::RParen)?;
                        self.in_paren_pattern -= 1;
                        return Ok(Pattern::Err(Box::new(first_p)));
                    }
                }
                if self.peek() == Some(&Token::Dot) {
                    self.bump()?;
                    let variant_name = self.parse_ident()?;
                    let mut fields = None;
                    let mut tuple_pats = None;
                    if self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        self.in_paren_pattern += 1;
                        let mut pats = Vec::new();
                        while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                            pats.push(self.parse_pattern()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        self.expect(Token::RParen)?;
                        self.in_paren_pattern -= 1;
                        tuple_pats = Some(pats);
                    } else if self.peek() == Some(&Token::LBracket) {
                        self.bump()?;
                        self.in_paren_pattern += 1;
                        let mut pats = Vec::new();
                        while self.peek() != Some(&Token::RBracket) && self.peek().is_some() {
                            pats.push(self.parse_pattern()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        self.expect(Token::RBracket)?;
                        self.in_paren_pattern -= 1;
                        tuple_pats = Some(pats);
                    } else if self.peek() == Some(&Token::LBrace) {
                        self.bump()?;
                        self.in_paren_pattern += 1;
                        let mut props = Vec::new();
                        while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                            if self.peek() == Some(&Token::DotDotDot) {
                                let dot_span = self.bump()?.1;
                                let (key, val) = if !matches!(
                                    self.peek(),
                                    Some(Token::Comma | Token::RBrace) | None
                                ) {
                                    let id = self.parse_ident()?;
                                    (id, None)
                                } else {
                                    (
                                        Ident {
                                            span: dot_span,
                                            sym: "_".to_string(),
                                        },
                                        None,
                                    )
                                };
                                props.push(ObjectPatProp {
                                    key,
                                    val,
                                    is_rest: true,
                                    mode: DestructureMode::ReadOnly,
                                });
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                                continue;
                            }
                            let key = self.parse_ident()?;
                            let mut val = None;
                            if self.peek() == Some(&Token::Colon) {
                                self.bump()?;
                                val = Some(self.parse_pattern()?);
                            }
                            props.push(ObjectPatProp {
                                key,
                                val,
                                is_rest: false,
                                mode: DestructureMode::ReadOnly,
                            });
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        self.expect(Token::RBrace)?;
                        self.in_paren_pattern -= 1;
                        fields = Some(props);
                    } else if let Some(Token::Ident(_)) = self.peek() {
                        let key = self.parse_ident()?;
                        tuple_pats = Some(vec![Pattern::Ident(key)]);
                    }
                    return Ok(Pattern::Enum(EnumPattern {
                        span: first_id.span,
                        enum_name: first_id,
                        variant_name,
                        fields,
                        tuple_pats,
                    }));
                }
                if first_id
                    .sym
                    .chars()
                    .next()
                    .map_or(false, |c| c.is_uppercase())
                    && self.peek() == Some(&Token::LBrace)
                {
                    self.bump()?;
                    self.in_paren_pattern += 1;
                    let mut props = Vec::new();
                    while self.peek() != Some(&Token::RBrace) && self.peek().is_some() {
                        if self.peek() == Some(&Token::DotDotDot) {
                            let dot_span = self.bump()?.1;
                            let (key, val) = if !matches!(
                                self.peek(),
                                Some(Token::Comma | Token::RBrace) | None
                            ) {
                                let id = self.parse_ident()?;
                                (id, None)
                            } else {
                                (
                                    Ident {
                                        span: dot_span,
                                        sym: "_".to_string(),
                                    },
                                    None,
                                )
                            };
                            props.push(ObjectPatProp {
                                key,
                                val,
                                is_rest: true,
                                mode: DestructureMode::ReadOnly,
                            });
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                            continue;
                        }
                        let key = self.parse_ident()?;
                        let mut val = None;
                        if self.peek() == Some(&Token::Colon) {
                            self.bump()?;
                            val = Some(self.parse_pattern()?);
                        }
                        props.push(ObjectPatProp {
                            key,
                            val,
                            is_rest: false,
                            mode: DestructureMode::ReadOnly,
                        });
                        if self.peek() == Some(&Token::Comma) {
                            self.bump()?;
                        }
                    }
                    self.expect(Token::RBrace)?;
                    self.in_paren_pattern -= 1;
                    return Ok(Pattern::Enum(EnumPattern {
                        span: first_id.span,
                        enum_name: first_id,
                        variant_name: Ident {
                            span: Span { start: 0, end: 0 },
                            sym: "".to_string(),
                        },
                        fields: Some(props),
                        tuple_pats: None,
                    }));
                }
                Ok(Pattern::Ident(first_id))
            }
        }
    }
}
