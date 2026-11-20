use super::*;
use crate::ast::*;
use crate::diagnostic::Diagnostic;
use crate::lexer::{Lexer, Token};
use crate::Span;

impl<'a> Parser<'a> {
    pub(super) fn parse_expr(&mut self) -> ParseResult<Expr> {
        self.parse_bin_expr(0)
    }

    pub(super) fn parse_bin_expr(&mut self, min_prec: u8) -> ParseResult<Expr> {
        let mut left = self.parse_primary_expr()?;
        loop {
            match self.peek() {
                Some(Token::LParen) | Some(Token::Bang) => {
                    let mut is_comptime = false;
                    if self.peek() == Some(&Token::Bang) {
                        if self.peek_next() == Some(&Token::LParen) {
                            self.bump()?;
                            is_comptime = true;
                        } else {
                            break;
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
                    left = Expr::Call(CallExpr {
                        span: Span {
                            start: left_span(&left).start,
                            end,
                        },
                        callee: Box::new(left),
                        args,
                        is_comptime,
                    });
                }
                Some(Token::QuestionDot) => {
                    self.bump()?;
                    let prop = self.parse_ident()?;
                    if self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        let mut args = Vec::new();
                        while self.peek() != Some(&Token::RParen) {
                            args.push(self.parse_expr()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RParen)?.end;
                        let opt_mem = Expr::OptMember(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                        left = Expr::OptCall(CallExpr {
                            span: Span {
                                start: left_span(&opt_mem).start,
                                end,
                            },
                            callee: Box::new(opt_mem),
                            args,
                            is_comptime: false,
                        });
                    } else {
                        left = Expr::OptMember(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                    }
                }
                Some(Token::DotQuestion) => {
                    self.bump()?;
                    left = Expr::Unwrap(Box::new(left));
                    continue;
                }
                Some(Token::Dot) => {
                    if self.peek_next() == Some(&Token::Star) {
                        self.bump()?; // consume .
                        self.bump()?; // consume *
                        left = Expr::Deref(Box::new(left));
                        continue;
                    }
                    if self.peek_next() == Some(&Token::Question) {
                        self.bump()?; // consume .
                        self.bump()?; // consume ?
                        left = Expr::Unwrap(Box::new(left));
                        continue;
                    }
                    if self.peek_next() == Some(&Token::QuestionDot) {
                        self.bump()?; // consume .
                        self.bump()?; // consume ?.
                        left = Expr::Unwrap(Box::new(left));
                        let prop = self.parse_ident()?;
                        left = Expr::Member(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                        continue;
                    }
                    self.bump()?;
                    let prop = self.parse_ident()?;
                    if let Expr::Ident(ref obj_id) = left {
                        if obj_id.sym == "Box"
                            && prop.sym == "new"
                            && self.peek() == Some(&Token::LParen)
                        {
                            self.bump()?;
                            let inner = self.parse_expr()?;
                            self.expect(Token::RParen)?;
                            left = Expr::BoxNew(Box::new(inner));
                            continue;
                        }
                        if obj_id.sym == "JSON"
                            && (prop.sym == "parse" || prop.sym == "decode")
                            && self.peek() == Some(&Token::Less)
                        {
                            let obj_start = obj_id.span.start;
                            let l_span = self.bump()?.1; // consume '<'
                            let target_ty = self.parse_type()?;
                            let r_span = self.expect(Token::Greater)?;
                            self.expect(Token::LParen)?;
                            let mut args = vec![Expr::TypeVal {
                                span: Span {
                                    start: l_span.start,
                                    end: r_span.end,
                                },
                                ty: target_ty,
                            }];
                            while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                                args.push(self.parse_expr()?);
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                            }
                            let end = self.expect(Token::RParen)?.end;
                            let callee = Expr::Member(MemberExpr {
                                span: Span {
                                    start: obj_start,
                                    end: prop.span.end,
                                },
                                obj: Box::new(left),
                                prop,
                            });
                            left = Expr::Call(CallExpr {
                                span: Span {
                                    start: obj_start,
                                    end,
                                },
                                callee: Box::new(callee),
                                args,
                                is_comptime: false,
                            });
                            continue;
                        }
                        if obj_id.sym == "reflect" {
                            if prop.sym == "type" && self.peek() == Some(&Token::Less) {
                                self.bump()?; // consume '<'
                                let target_ty = self.parse_type()?;
                                self.expect(Token::Greater)?;
                                self.expect(Token::LParen)?;
                                let end_tok = self.expect(Token::RParen)?;
                                left = Expr::ReflectType {
                                    span: Span {
                                        start: obj_id.span.start,
                                        end: end_tok.end,
                                    },
                                    target_ty,
                                };
                                continue;
                            } else if prop.sym == "of" && self.peek() == Some(&Token::LParen) {
                                self.bump()?; // consume '('
                                let inner = self.parse_expr()?;
                                let end_tok = self.expect(Token::RParen)?;
                                left = Expr::ReflectOf {
                                    span: Span {
                                        start: obj_id.span.start,
                                        end: end_tok.end,
                                    },
                                    expr: Box::new(inner),
                                };
                                continue;
                            }
                        }
                    }
                    if (prop.sym == "downcast" || prop.sym == "downcast_mut")
                        && self.peek() == Some(&Token::Less)
                    {
                        self.bump()?;
                        let target_ty = self.parse_ident()?;
                        self.expect(Token::Greater)?;
                        self.expect(Token::LParen)?;
                        self.expect(Token::RParen)?;
                        left = Expr::Downcast {
                            obj: Box::new(left),
                            target_ty,
                            is_mut: prop.sym == "downcast_mut",
                        };
                        continue;
                    }
                    let is_prop_type_like =
                        prop.sym.chars().next().map_or(false, |c| c.is_uppercase());
                    let is_enum_variant_init = if let Expr::Ident(ref obj_id) = left {
                        obj_id
                            .sym
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_uppercase())
                            && is_prop_type_like
                    } else {
                        false
                    };

                    if (is_enum_variant_init || is_prop_type_like)
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
                        let (enum_name, variant_name) = if is_enum_variant_init {
                            let enum_name = match &left {
                                Expr::Ident(id) => id.clone(),
                                _ => Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Unknown".to_string(),
                                },
                            };
                            (enum_name, prop)
                        } else {
                            let left_str =
                                expr_to_member_str(&left).unwrap_or_else(|| "Unknown".to_string());
                            let full_ty_sym = format!("{}.{}", left_str, prop.sym);
                            let full_ty_ident = Ident {
                                span: Span {
                                    start: left_span(&left).start,
                                    end: prop.span.end,
                                },
                                sym: full_ty_sym,
                            };
                            let empty_variant = Ident {
                                span: prop.span,
                                sym: String::new(),
                            };
                            (full_ty_ident, empty_variant)
                        };
                        left = Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end,
                            },
                            enum_name,
                            variant_name,
                            fields: Some(fields),
                            tuple_args: None,
                        });
                        continue;
                    } else if is_enum_variant_init && self.peek() == Some(&Token::LBracket) {
                        self.bump()?;
                        let mut args = Vec::new();
                        while self.peek() != Some(&Token::RBracket) && self.peek().is_some() {
                            args.push(self.parse_expr()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RBracket)?.end;
                        let enum_name = match &left {
                            Expr::Ident(id) => id.clone(),
                            _ => Ident {
                                span: Span { start: 0, end: 0 },
                                sym: "Unknown".to_string(),
                            },
                        };
                        left = Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end,
                            },
                            enum_name,
                            variant_name: prop,
                            fields: None,
                            tuple_args: Some(args),
                        });
                        continue;
                    } else if is_enum_variant_init && self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        let mut args = Vec::new();
                        while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                            args.push(self.parse_expr()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RParen)?.end;
                        let enum_name = match &left {
                            Expr::Ident(id) => id.clone(),
                            _ => Ident {
                                span: Span { start: 0, end: 0 },
                                sym: "Unknown".to_string(),
                            },
                        };
                        left = Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end,
                            },
                            enum_name,
                            variant_name: prop,
                            fields: None,
                            tuple_args: Some(args),
                        });
                        continue;
                    } else {
                        left = Expr::Member(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                    }
                }
                Some(Token::Question) => {
                    self.bump()?;
                    if self.peek() == Some(&Token::Continue) {
                        let span = self.bump()?.1;
                        left = Expr::QuestionFallback {
                            expr: Box::new(left),
                            fallback: Box::new(Expr::Continue(span)),
                            is_return_fallback: false,
                        };
                    } else if self.peek() == Some(&Token::Break) {
                        let span = self.bump()?.1;
                        left = Expr::QuestionFallback {
                            expr: Box::new(left),
                            fallback: Box::new(Expr::Break(span, None)),
                            is_return_fallback: false,
                        };
                    } else if self.peek() == Some(&Token::Return) {
                        let span = self.bump()?.1;
                        let has_arg = if matches!(
                            self.peek(),
                            Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                        ) {
                            false
                        } else {
                            let next_start = self.peek_span().start;
                            if next_start >= span.end
                                && self.input[span.end..next_start].contains('\n')
                            {
                                false
                            } else {
                                true
                            }
                        };
                        let val = if has_arg {
                            Some(Box::new(self.parse_expr()?))
                        } else {
                            None
                        };
                        left = Expr::QuestionFallback {
                            expr: Box::new(left),
                            fallback: Box::new(Expr::Return(span, val)),
                            is_return_fallback: true,
                        };
                    } else if self.peek() == Some(&Token::LBrace) {
                        let block = self.parse_block_stmt()?;
                        let fallback = Expr::LabeledBlock { label: None, block };
                        left = Expr::QuestionFallback {
                            expr: Box::new(left),
                            fallback: Box::new(fallback),
                            is_return_fallback: false,
                        };
                    } else if matches!(
                        self.peek(),
                        Some(Token::IntLiteral(_))
                            | Some(Token::FloatLiteral(_))
                            | Some(Token::StringLiteral(_))
                            | Some(Token::TemplateStringRaw(_))
                            | Some(Token::Ident(_))
                            | Some(Token::Null)
                            | Some(Token::New)
                            | Some(Token::LParen)
                            | Some(Token::LBracket)
                            | Some(Token::Minus)
                    ) {
                        let fallback = self.parse_expr()?;
                        left = Expr::QuestionFallback {
                            expr: Box::new(left),
                            fallback: Box::new(fallback),
                            is_return_fallback: false,
                        };
                    } else {
                        left = Expr::Question(Box::new(left));
                    }
                }
                Some(Token::QuestionQuestion) => {
                    self.bump()?;
                    let right = if self.peek() == Some(&Token::Continue) {
                        let span = self.bump()?.1;
                        Expr::Continue(span)
                    } else if self.peek() == Some(&Token::Break) {
                        let span = self.bump()?.1;
                        Expr::Break(span, None)
                    } else if self.peek() == Some(&Token::Return) {
                        let span = self.bump()?.1;
                        let has_arg = if matches!(
                            self.peek(),
                            Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                        ) {
                            false
                        } else {
                            let next_start = self.peek_span().start;
                            if next_start >= span.end
                                && self.input[span.end..next_start].contains('\n')
                            {
                                false
                            } else {
                                true
                            }
                        };
                        let val = if has_arg {
                            Some(Box::new(self.parse_expr()?))
                        } else {
                            None
                        };
                        Expr::Return(span, val)
                    } else {
                        self.parse_expr()?
                    };
                    left = Expr::NullCoalesce {
                        left: Box::new(left),
                        right: Box::new(right),
                    };
                }
                Some(Token::Catch) => {
                    self.bump()?;
                    let mut is_err_mut = false;
                    let mut err_ident = None;
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
                    let body = if self.peek() == Some(&Token::LBrace) {
                        Stmt::Block(self.parse_block_stmt()?)
                    } else if self.peek() == Some(&Token::Return) {
                        Stmt::Return(self.parse_return_stmt()?)
                    } else {
                        Stmt::Expr(self.parse_expr()?)
                    };
                    left = Expr::Catch {
                        expr: Box::new(left),
                        err_ident,
                        is_err_mut,
                        body: Box::new(body),
                    };
                }
                Some(Token::LBracket) => {
                    let _start_span = self.bump()?.1;
                    let mut start_idx = None;
                    let mut end_idx = None;
                    let mut sentinel = None;
                    let mut is_slice = false;
                    if self.peek() == Some(&Token::DotDot) {
                        self.bump()?;
                        is_slice = true;
                        if self.peek() != Some(&Token::RBracket)
                            && self.peek() != Some(&Token::Colon)
                        {
                            end_idx = Some(Box::new(self.parse_expr()?));
                        }
                    } else {
                        let idx = self.parse_expr()?;
                        if self.peek() == Some(&Token::DotDot) {
                            self.bump()?;
                            is_slice = true;
                            start_idx = Some(Box::new(idx));
                            if self.peek() != Some(&Token::RBracket)
                                && self.peek() != Some(&Token::Colon)
                            {
                                end_idx = Some(Box::new(self.parse_expr()?));
                            }
                        } else {
                            start_idx = Some(Box::new(idx));
                        }
                    }
                    if is_slice && self.peek() == Some(&Token::Colon) {
                        self.bump()?;
                        sentinel = Some(Box::new(self.parse_expr()?));
                    }
                    let end_span = self.expect(Token::RBracket)?;
                    if is_slice {
                        left = Expr::Slice(SliceExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: end_span.end,
                            },
                            obj: Box::new(left),
                            start: start_idx,
                            end: end_idx,
                            sentinel,
                        });
                    } else {
                        left = Expr::Index {
                            obj: Box::new(left),
                            index: start_idx.unwrap(),
                        };
                    }
                }
                Some(Token::FatArrow) => {
                    if self.in_match_guard || !matches!(left, Expr::Ident(_) | Expr::Array(_)) {
                        break;
                    }
                    self.bump()?;
                    let body = if self.peek() == Some(&Token::LBrace) {
                        ArrowBody::Block(self.parse_block_stmt()?)
                    } else {
                        ArrowBody::Expr(self.parse_expr()?)
                    };
                    let params = match left {
                        Expr::Ident(ref id) => vec![Pattern::Ident(id.clone())],
                        Expr::Array(ref elements) => {
                            let mut pats = Vec::new();
                            for e in elements {
                                if let Expr::Ident(id) = e {
                                    pats.push(Pattern::Ident(id.clone()));
                                } else {
                                    pats.push(Pattern::Wildcard);
                                }
                            }
                            pats
                        }
                        _ => vec![],
                    };
                    left = Expr::Arrow(Box::new(ArrowExpr {
                        span: Span {
                            start: left_span(&left).start,
                            end: self.peek_span().end,
                        },
                        is_async: false,
                        params,
                        body: Box::new(body),
                    }));
                }
                Some(Token::As) => {
                    self.bump()?;
                    let target_ty = self.parse_type()?;
                    left = Expr::Cast {
                        span: Span {
                            start: left_span(&left).start,
                            end: self.peek_span().end,
                        },
                        expr: Box::new(left),
                        target_ty,
                    };
                }
                _ => {
                    if matches!(self.peek(), Some(Token::Ident(id)) if id == "orelse") {
                        self.bump()?;
                        let right = if self.peek() == Some(&Token::LBrace) {
                            let block = self.parse_block_stmt()?;
                            Expr::LabeledBlock { label: None, block }
                        } else if self.peek() == Some(&Token::Continue) {
                            let span = self.bump()?.1;
                            Expr::Continue(span)
                        } else if self.peek() == Some(&Token::Break) {
                            let span = self.bump()?.1;
                            if self.peek() == Some(&Token::Colon) {
                                self.bump()?; // consume ':'
                                let label = self.parse_ident()?;
                                let val = if !matches!(
                                    self.peek(),
                                    Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                                ) {
                                    Some(Box::new(self.parse_expr()?))
                                } else {
                                    None
                                };
                                Expr::LabeledBreak(span, label, val)
                            } else {
                                let has_val = !matches!(
                                    self.peek(),
                                    Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                                );
                                let val = if has_val {
                                    Some(Box::new(self.parse_expr()?))
                                } else {
                                    None
                                };
                                Expr::Break(span, val)
                            }
                        } else if self.peek() == Some(&Token::Return) {
                            let span = self.bump()?.1;
                            let has_arg = if matches!(
                                self.peek(),
                                Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                            ) {
                                false
                            } else {
                                let next_start = self.peek_span().start;
                                !(next_start >= span.end
                                    && self.input[span.end..next_start].contains('\n'))
                            };
                            let val = if has_arg {
                                Some(Box::new(self.parse_expr()?))
                            } else {
                                None
                            };
                            Expr::Return(span, val)
                        } else {
                            self.parse_expr()?
                        };
                        left = Expr::NullCoalesce {
                            left: Box::new(left),
                            right: Box::new(right),
                        };
                        continue;
                    }
                    break;
                }
            }
        }
        while self.peek().is_some() {
            let compound_op = match self.peek() {
                Some(Token::PlusAssign) => Some(BinaryOp::Add),
                Some(Token::MinusAssign) => Some(BinaryOp::Sub),
                Some(Token::StarAssign) => Some(BinaryOp::Mul),
                Some(Token::SlashAssign) => Some(BinaryOp::Div),
                Some(Token::PercentAssign) => Some(BinaryOp::Mod),
                Some(Token::AmpAssign) => Some(BinaryOp::BitAnd),
                Some(Token::PipeAssign) => Some(BinaryOp::BitOr),
                Some(Token::CaretAssign) => Some(BinaryOp::BitXor),
                Some(Token::ShlAssign) => Some(BinaryOp::Shl),
                Some(Token::ShrAssign) => Some(BinaryOp::Shr),
                _ => None,
            };
            if let Some(inner_op) = compound_op {
                if 1 < min_prec {
                    break;
                }
                self.bump()?;
                let right = self.parse_bin_expr(1)?;
                let start = left_span(&left).start;
                let end = right_span(&right).end;
                let desugared_rhs = Expr::Bin(BinExpr {
                    span: Span { start, end },
                    op: inner_op,
                    left: Box::new(left.clone()),
                    right: Box::new(right),
                });
                left = Expr::Bin(BinExpr {
                    span: Span { start, end },
                    op: BinaryOp::Assign,
                    left: Box::new(left),
                    right: Box::new(desugared_rhs),
                });
                continue;
            }
            if self.peek() == Some(&Token::QuestionQuestionAssign) {
                if 1 < min_prec {
                    break;
                }
                self.bump()?;
                let right = self.parse_bin_expr(1)?;
                let start = left_span(&left).start;
                let end = right_span(&right).end;
                let desugared_rhs = Expr::NullCoalesce {
                    left: Box::new(left.clone()),
                    right: Box::new(right),
                };
                left = Expr::Bin(BinExpr {
                    span: Span { start, end },
                    op: BinaryOp::Assign,
                    left: Box::new(left),
                    right: Box::new(desugared_rhs),
                });
                continue;
            }
            let op = match self.peek_op() {
                Some(op) => op,
                None => break,
            };
            let prec = self.op_precedence(&op);
            if prec < min_prec {
                break;
            }
            self.bump()?;
            let is_pipe = op == BinaryOp::Pipe;
            if is_pipe {
                self.in_pipeline_rhs += 1;
            }
            let right = if op == BinaryOp::LogicalOr && self.peek() == Some(&Token::Continue) {
                let span = self.bump()?.1;
                Expr::Continue(span)
            } else if op == BinaryOp::LogicalOr && self.peek() == Some(&Token::Break) {
                let span = self.bump()?.1;
                Expr::Break(span, None)
            } else if op == BinaryOp::LogicalOr && self.peek() == Some(&Token::Return) {
                let span = self.bump()?.1;
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
                let val = if has_arg {
                    Some(Box::new(self.parse_expr()?))
                } else {
                    None
                };
                Expr::Return(span, val)
            } else {
                self.parse_bin_expr(prec + 1)?
            };
            if is_pipe {
                self.in_pipeline_rhs -= 1;
            }
            let start = left_span(&left).start;
            let end = right_span(&right).end;
            if op == BinaryOp::Assign {
                if let Expr::Ident(ref id) = left {
                    if id.sym == "_" {
                        self.push_error(
                            Diagnostic::new(
                                "E0120",
                                "cannot use '_ = ...' to discard expression result; '_' is reserved for the current instance receiver",
                            )
                            .with_span(id.span)
                            .with_cause("in CP, '_' exclusively represents the current struct/class/enum instance ('self'/'this'). Using '_ = expr' causes severe ambiguity inside methods.")
                            .with_help("use '* = expr;' to discard return values.")
                            .with_suggestion("replace '_ = ...;' with '* = ...;'"),
                        );
                    }
                }
            }
            left = Expr::Bin(BinExpr {
                span: Span { start, end },
                op,
                left: Box::new(left),
                right: Box::new(right),
            });
        }
        Ok(left)
    }

    pub(super) fn parse_postfix_expr(&mut self) -> ParseResult<Expr> {
        let mut left = self.parse_primary_expr()?;
        loop {
            match self.peek() {
                Some(Token::LParen) | Some(Token::Bang) => {
                    let next_start = self.peek_span().start;
                    if next_start >= self.prev_token_end
                        && self.input[self.prev_token_end..next_start].contains('\n')
                    {
                        break;
                    }
                    let mut is_comptime = false;
                    if self.peek() == Some(&Token::Bang) {
                        if self.peek_next() == Some(&Token::LParen) {
                            self.bump()?;
                            is_comptime = true;
                        } else {
                            break;
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
                    left = Expr::Call(CallExpr {
                        span: Span {
                            start: left_span(&left).start,
                            end,
                        },
                        callee: Box::new(left),
                        args,
                        is_comptime,
                    });
                }
                Some(Token::QuestionDot) => {
                    self.bump()?;
                    let prop = self.parse_ident()?;
                    if self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        let mut args = Vec::new();
                        while self.peek() != Some(&Token::RParen) {
                            args.push(self.parse_expr()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RParen)?.end;
                        let opt_mem = Expr::OptMember(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                        left = Expr::OptCall(CallExpr {
                            span: Span {
                                start: left_span(&opt_mem).start,
                                end,
                            },
                            callee: Box::new(opt_mem),
                            args,
                            is_comptime: false,
                        });
                    } else {
                        left = Expr::OptMember(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                    }
                }
                Some(Token::DotQuestion) => {
                    self.bump()?;
                    left = Expr::Unwrap(Box::new(left));
                    continue;
                }
                Some(Token::Dot) => {
                    if self.peek_next() == Some(&Token::Star) {
                        self.bump()?; // consume .
                        self.bump()?; // consume *
                        left = Expr::Deref(Box::new(left));
                        continue;
                    }
                    if self.peek_next() == Some(&Token::Question) {
                        self.bump()?; // consume .
                        self.bump()?; // consume ?
                        left = Expr::Unwrap(Box::new(left));
                        continue;
                    }
                    if self.peek_next() == Some(&Token::QuestionDot) {
                        self.bump()?; // consume .
                        self.bump()?; // consume ?.
                        left = Expr::Unwrap(Box::new(left));
                        let prop = self.parse_ident()?;
                        left = Expr::Member(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                        continue;
                    }
                    self.bump()?;
                    let prop = self.parse_ident()?;
                    if let Expr::Ident(ref obj_id) = left {
                        if obj_id.sym == "Box"
                            && prop.sym == "new"
                            && self.peek() == Some(&Token::LParen)
                        {
                            self.bump()?;
                            let inner = self.parse_expr()?;
                            self.expect(Token::RParen)?;
                            left = Expr::BoxNew(Box::new(inner));
                            continue;
                        }
                        if obj_id.sym == "JSON"
                            && (prop.sym == "parse" || prop.sym == "decode")
                            && self.peek() == Some(&Token::Less)
                        {
                            let obj_start = obj_id.span.start;
                            let l_span = self.bump()?.1; // consume '<'
                            let target_ty = self.parse_type()?;
                            let r_span = self.expect(Token::Greater)?;
                            self.expect(Token::LParen)?;
                            let mut args = vec![Expr::TypeVal {
                                span: Span {
                                    start: l_span.start,
                                    end: r_span.end,
                                },
                                ty: target_ty,
                            }];
                            while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                                args.push(self.parse_expr()?);
                                if self.peek() == Some(&Token::Comma) {
                                    self.bump()?;
                                }
                            }
                            let end = self.expect(Token::RParen)?.end;
                            let callee = Expr::Member(MemberExpr {
                                span: Span {
                                    start: obj_start,
                                    end: prop.span.end,
                                },
                                obj: Box::new(left),
                                prop,
                            });
                            left = Expr::Call(CallExpr {
                                span: Span {
                                    start: obj_start,
                                    end,
                                },
                                callee: Box::new(callee),
                                args,
                                is_comptime: false,
                            });
                            continue;
                        }
                        if obj_id.sym == "reflect" {
                            if prop.sym == "type" && self.peek() == Some(&Token::Less) {
                                self.bump()?; // consume '<'
                                let target_ty = self.parse_type()?;
                                self.expect(Token::Greater)?;
                                self.expect(Token::LParen)?;
                                let end_tok = self.expect(Token::RParen)?;
                                left = Expr::ReflectType {
                                    span: Span {
                                        start: obj_id.span.start,
                                        end: end_tok.end,
                                    },
                                    target_ty,
                                };
                                continue;
                            } else if prop.sym == "of" && self.peek() == Some(&Token::LParen) {
                                self.bump()?; // consume '('
                                let inner = self.parse_expr()?;
                                let end_tok = self.expect(Token::RParen)?;
                                left = Expr::ReflectOf {
                                    span: Span {
                                        start: obj_id.span.start,
                                        end: end_tok.end,
                                    },
                                    expr: Box::new(inner),
                                };
                                continue;
                            }
                        }
                    }
                    if (prop.sym == "downcast" || prop.sym == "downcast_mut")
                        && self.peek() == Some(&Token::Less)
                    {
                        self.bump()?;
                        let target_ty = self.parse_ident()?;
                        self.expect(Token::Greater)?;
                        self.expect(Token::LParen)?;
                        self.expect(Token::RParen)?;
                        left = Expr::Downcast {
                            obj: Box::new(left),
                            target_ty,
                            is_mut: prop.sym == "downcast_mut",
                        };
                        continue;
                    }
                    let is_prop_type_like =
                        prop.sym.chars().next().map_or(false, |c| c.is_uppercase());
                    let is_enum_variant_init = if let Expr::Ident(ref obj_id) = left {
                        obj_id
                            .sym
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_uppercase())
                            && is_prop_type_like
                    } else {
                        false
                    };

                    if (is_enum_variant_init || is_prop_type_like)
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
                        let (enum_name, variant_name) = if is_enum_variant_init {
                            let enum_name = match &left {
                                Expr::Ident(id) => id.clone(),
                                _ => Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Unknown".to_string(),
                                },
                            };
                            (enum_name, prop)
                        } else {
                            let left_str =
                                expr_to_member_str(&left).unwrap_or_else(|| "Unknown".to_string());
                            let full_ty_sym = format!("{}.{}", left_str, prop.sym);
                            let full_ty_ident = Ident {
                                span: Span {
                                    start: left_span(&left).start,
                                    end: prop.span.end,
                                },
                                sym: full_ty_sym,
                            };
                            let empty_variant = Ident {
                                span: prop.span,
                                sym: String::new(),
                            };
                            (full_ty_ident, empty_variant)
                        };
                        left = Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end,
                            },
                            enum_name,
                            variant_name,
                            fields: Some(fields),
                            tuple_args: None,
                        });
                        continue;
                    } else if is_enum_variant_init && self.peek() == Some(&Token::LBracket) {
                        self.bump()?;
                        let mut args = Vec::new();
                        while self.peek() != Some(&Token::RBracket) && self.peek().is_some() {
                            args.push(self.parse_expr()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RBracket)?.end;
                        let enum_name = match &left {
                            Expr::Ident(id) => id.clone(),
                            _ => Ident {
                                span: Span { start: 0, end: 0 },
                                sym: "Unknown".to_string(),
                            },
                        };
                        left = Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end,
                            },
                            enum_name,
                            variant_name: prop,
                            fields: None,
                            tuple_args: Some(args),
                        });
                        continue;
                    } else if is_enum_variant_init && self.peek() == Some(&Token::LParen) {
                        self.bump()?;
                        let mut args = Vec::new();
                        while self.peek() != Some(&Token::RParen) && self.peek().is_some() {
                            args.push(self.parse_expr()?);
                            if self.peek() == Some(&Token::Comma) {
                                self.bump()?;
                            }
                        }
                        let end = self.expect(Token::RParen)?.end;
                        let enum_name = match &left {
                            Expr::Ident(id) => id.clone(),
                            _ => Ident {
                                span: Span { start: 0, end: 0 },
                                sym: "Unknown".to_string(),
                            },
                        };
                        left = Expr::EnumInit(EnumInitExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end,
                            },
                            enum_name,
                            variant_name: prop,
                            fields: None,
                            tuple_args: Some(args),
                        });
                        continue;
                    } else {
                        left = Expr::Member(MemberExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: prop.span.end,
                            },
                            obj: Box::new(left),
                            prop,
                        });
                    }
                }
                Some(Token::LBracket) => {
                    let _start_span = self.bump()?.1;
                    let mut start_idx = None;
                    let mut end_idx = None;
                    let mut sentinel = None;
                    let mut is_slice = false;
                    if self.peek() == Some(&Token::DotDot) {
                        self.bump()?;
                        is_slice = true;
                        if self.peek() != Some(&Token::RBracket)
                            && self.peek() != Some(&Token::Colon)
                        {
                            end_idx = Some(Box::new(self.parse_expr()?));
                        }
                    } else {
                        let idx = self.parse_expr()?;
                        if self.peek() == Some(&Token::DotDot) {
                            self.bump()?;
                            is_slice = true;
                            start_idx = Some(Box::new(idx));
                            if self.peek() != Some(&Token::RBracket)
                                && self.peek() != Some(&Token::Colon)
                            {
                                end_idx = Some(Box::new(self.parse_expr()?));
                            }
                        } else {
                            start_idx = Some(Box::new(idx));
                        }
                    }
                    if is_slice && self.peek() == Some(&Token::Colon) {
                        self.bump()?;
                        sentinel = Some(Box::new(self.parse_expr()?));
                    }
                    let end_span = self.expect(Token::RBracket)?;
                    if is_slice {
                        left = Expr::Slice(SliceExpr {
                            span: Span {
                                start: left_span(&left).start,
                                end: end_span.end,
                            },
                            obj: Box::new(left),
                            start: start_idx,
                            end: end_idx,
                            sentinel,
                        });
                    } else {
                        left = Expr::Index {
                            obj: Box::new(left),
                            index: start_idx.unwrap(),
                        };
                    }
                }
                _ => {
                    if matches!(self.peek(), Some(Token::Ident(id)) if id == "orelse") {
                        self.bump()?;
                        let right = if self.peek() == Some(&Token::LBrace) {
                            let block = self.parse_block_stmt()?;
                            Expr::LabeledBlock { label: None, block }
                        } else if self.peek() == Some(&Token::Continue) {
                            let span = self.bump()?.1;
                            Expr::Continue(span)
                        } else if self.peek() == Some(&Token::Break) {
                            let span = self.bump()?.1;
                            if self.peek() == Some(&Token::Colon) {
                                self.bump()?; // consume ':'
                                let label = self.parse_ident()?;
                                let val = if !matches!(
                                    self.peek(),
                                    Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                                ) {
                                    Some(Box::new(self.parse_expr()?))
                                } else {
                                    None
                                };
                                Expr::LabeledBreak(span, label, val)
                            } else {
                                let has_val = !matches!(
                                    self.peek(),
                                    Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                                );
                                let val = if has_val {
                                    Some(Box::new(self.parse_expr()?))
                                } else {
                                    None
                                };
                                Expr::Break(span, val)
                            }
                        } else if self.peek() == Some(&Token::Return) {
                            let span = self.bump()?.1;
                            let has_arg = if matches!(
                                self.peek(),
                                Some(Token::Semicolon | Token::RBrace | Token::RParen) | None
                            ) {
                                false
                            } else {
                                let next_start = self.peek_span().start;
                                !(next_start >= span.end
                                    && self.input[span.end..next_start].contains('\n'))
                            };
                            let val = if has_arg {
                                Some(Box::new(self.parse_expr()?))
                            } else {
                                None
                            };
                            Expr::Return(span, val)
                        } else {
                            self.parse_expr()?
                        };
                        left = Expr::NullCoalesce {
                            left: Box::new(left),
                            right: Box::new(right),
                        };
                        continue;
                    }
                    break;
                }
            }
        }
        Ok(left)
    }

    pub(super) fn parse_ident(&mut self) -> ParseResult<Ident> {
        let (token, span) = self.bump()?;
        let sym = match token {
            Token::Ident(s) => s,

            Token::Match => "match".to_string(),
            Token::ZigEscape => "zig".to_string(),
            Token::Switch => "switch".to_string(),
            Token::Try => "try".to_string(),
            Token::Catch => "catch".to_string(),
            Token::Null => "null".to_string(),
            Token::Dyn => "dyn".to_string(),
            Token::Super => "super".to_string(),
            Token::ErrorKw => {
                let text = &self.input[span.start..span.end];
                text.to_string()
            }
            Token::Await => "await".to_string(),
            Token::Async => "async".to_string(),
            Token::If => "if".to_string(),
            Token::While => "while".to_string(),
            Token::For => "for".to_string(),
            Token::Return => "return".to_string(),
            Token::Class => "class".to_string(),
            Token::Struct => "struct".to_string(),
            Token::Ref => "ref".to_string(),
            Token::Trait => "trait".to_string(),
            Token::Enum => "enum".to_string(),
            Token::Impl => "impl".to_string(),
            Token::Interface => "interface".to_string(),
            Token::Extends => "extends".to_string(),
            Token::Any => "any".to_string(),
            Token::Void => "void".to_string(),
            Token::String => "string".to_string(),
            Token::Bool => "bool".to_string(),
            Token::U8 => "u8".to_string(),
            Token::I8 => "i8".to_string(),
            Token::U16 => "u16".to_string(),
            Token::I16 => "i16".to_string(),
            Token::I32 => "i32".to_string(),
            Token::U32 => "u32".to_string(),
            Token::I64 => "i64".to_string(),
            Token::U64 => "u64".to_string(),
            Token::I128 => "i128".to_string(),
            Token::U128 => "u128".to_string(),
            Token::F32 => "f32".to_string(),
            Token::F64 => "f64".to_string(),
            Token::ISize => "isize".to_string(),
            Token::USize => "usize".to_string(),
            Token::New => "new".to_string(),
            Token::From => "from".to_string(),
            Token::As => "as".to_string(),
            Token::Meta => "meta".to_string(),
            Token::Dec => "dec".to_string(),
            Token::Macro => "macro".to_string(),
            Token::Unsafe => "unsafe".to_string(),
            Token::Of => "of".to_string(),
            Token::In => "in".to_string(),
            Token::TypeKw => "type".to_string(),
            Token::Import => "import".to_string(),
            Token::Export => "export".to_string(),
            Token::Const => "const".to_string(),
            Token::Let => "let".to_string(),
            Token::Var => "var".to_string(),
            Token::ThreadLocal => "threadlocal".to_string(),
            Token::Mut => "mut".to_string(),
            Token::Static => "static".to_string(),
            Token::CStr => "cstr".to_string(),
            Token::Else => "else".to_string(),
            Token::Break => "break".to_string(),
            Token::Continue => "continue".to_string(),
            Token::Move => "move".to_string(),
            Token::Own => "own".to_string(),
            Token::Fn => "fn".to_string(),
            Token::Auto => "auto".to_string(),
            _ => {
                return Err(ParseError::ExpectedToken(
                    Token::Ident("Ident".to_string()),
                    token,
                    span,
                ))
            }
        };
        Ok(Ident { span, sym })
    }

    pub(super) fn peek_op(&mut self) -> Option<BinaryOp> {
        match self.peek() {
            Some(Token::Plus) => Some(BinaryOp::Add),
            Some(Token::PlusPlus) => Some(BinaryOp::Concat),
            Some(Token::Minus) => Some(BinaryOp::Sub),
            Some(Token::Star) => Some(BinaryOp::Mul),
            Some(Token::WrappingAdd) => Some(BinaryOp::WrappingAdd),
            Some(Token::WrappingSub) => Some(BinaryOp::WrappingSub),
            Some(Token::WrappingMul) => Some(BinaryOp::WrappingMul),
            Some(Token::SaturatingAdd) => Some(BinaryOp::SaturatingAdd),
            Some(Token::SaturatingSub) => Some(BinaryOp::SaturatingSub),
            Some(Token::Slash) => Some(BinaryOp::Div),
            Some(Token::Percent) => Some(BinaryOp::Mod),
            Some(Token::Ampersand) => Some(BinaryOp::BitAnd),
            Some(Token::Pipe) => Some(BinaryOp::BitOr),
            Some(Token::Caret) => Some(BinaryOp::BitXor),
            Some(Token::Shl) => Some(BinaryOp::Shl),
            Some(Token::Shr) => Some(BinaryOp::Shr),
            Some(Token::AmpAmp) => Some(BinaryOp::LogicalAnd),
            Some(Token::PipePipe) => Some(BinaryOp::LogicalOr),
            Some(Token::Ident(ref id)) if id == "and" => Some(BinaryOp::LogicalAnd),
            Some(Token::Ident(ref id)) if id == "or" => Some(BinaryOp::LogicalOr),
            Some(Token::Assign) => Some(BinaryOp::Assign),
            Some(Token::Eq) => Some(BinaryOp::Eq),
            Some(Token::NotEq) => Some(BinaryOp::NotEq),
            Some(Token::Less) => Some(BinaryOp::Lt),
            Some(Token::LessEq) => Some(BinaryOp::LtEq),
            Some(Token::Greater) => Some(BinaryOp::Gt),
            Some(Token::GreaterEq) => Some(BinaryOp::GtEq),
            Some(Token::Pipeline) => Some(BinaryOp::Pipe),
            _ => None,
        }
    }

    pub(super) fn op_precedence(&self, op: &BinaryOp) -> u8 {
        match op {
            BinaryOp::Assign => 1,
            BinaryOp::LogicalOr => 2,
            BinaryOp::LogicalAnd => 3,
            BinaryOp::BitOr => 4,
            BinaryOp::BitXor => 5,
            BinaryOp::BitAnd => 6,
            BinaryOp::Eq | BinaryOp::NotEq => 7,
            BinaryOp::Lt | BinaryOp::LtEq | BinaryOp::Gt | BinaryOp::GtEq => 8,
            BinaryOp::Pipe => 9,
            BinaryOp::Shl | BinaryOp::Shr => 10,
            BinaryOp::Add
            | BinaryOp::Concat
            | BinaryOp::Sub
            | BinaryOp::WrappingAdd
            | BinaryOp::WrappingSub
            | BinaryOp::SaturatingAdd
            | BinaryOp::SaturatingSub => 11,
            BinaryOp::Mul | BinaryOp::Div | BinaryOp::Mod | BinaryOp::WrappingMul => 12,
        }
    }
}
