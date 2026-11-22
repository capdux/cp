use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
    pub(super) fn expand_macros(&mut self, module: &mut Module) -> SemaResult<()> {
        let mut class_modifications: Vec<(String, Vec<Function>)> = Vec::new();

        for item in &module.body {
            if let ModuleItem::Decl(Decl::Class(cls)) = item {
                let mut derived_methods = Vec::new();
                for meta_item in &cls.meta {
                    let macro_name = &meta_item.name.sym;
                    if !self.macro_defs.contains_key(macro_name) {
                        continue;
                    }

                    // Anti-Cycle Guardrails:
                    let call_key = (macro_name.clone(), cls.ident.sym.clone());
                    if self.macro_call_stack.contains(&call_key) {
                        return Err(SemaError::MacroCycleDetected(
                            macro_name.clone(),
                            cls.ident.sym.clone(),
                            meta_item.span,
                        ));
                    }
                    if self.macro_call_stack.len() >= 128 {
                        return Err(SemaError::MacroRecursionLimitExceeded(
                            macro_name.clone(),
                            meta_item.span,
                        ));
                    }

                    self.macro_call_stack.push(call_key);

                    let derived_list: Vec<Function> =
                        self.expand_single_macro(macro_name, cls, meta_item)?;

                    self.macro_call_stack.pop();

                    for derived in derived_list {
                        // Macro Hygiene: generated AST nodes strictly cannot carry macro annotations!
                        if !derived.meta.is_empty() {
                            return Err(SemaError::MacroHygieneViolation(
                                macro_name.clone(),
                                meta_item.span,
                            ));
                        }
                        derived_methods.push(derived);
                    }
                }

                if !derived_methods.is_empty() {
                    class_modifications.push((cls.ident.sym.clone(), derived_methods));
                }
            }
        }

        for item in &mut module.body {
            if let ModuleItem::Decl(Decl::Class(cls)) = item {
                if let Some((_, methods)) = class_modifications
                    .iter()
                    .find(|(name, _)| name == &cls.ident.sym)
                {
                    for m in methods {
                        if !cls
                            .methods
                            .iter()
                            .any(|existing| existing.ident.sym == m.ident.sym)
                        {
                            cls.methods.push(m.clone());
                        }
                    }
                }
                // Auto-generate memberwise init if not present and fields are non-empty (exempting @root, @value, @packed, @extern structs)
                let is_root = cls.meta.iter().any(|m| m.name.sym == "root");
                let is_value = cls.meta.iter().any(|m| m.name.sym == "value");
                let is_packed = cls.meta.iter().any(|m| m.name.sym == "packed");
                let is_extern = cls.meta.iter().any(|m| m.name.sym == "extern");
                let is_generic = !cls.generic_params.is_empty();
                if !cls.is_static
                    && !cls.is_union
                    && !is_root
                    && !is_value
                    && !is_packed
                    && !is_extern
                    && !is_generic
                    && !cls.methods.iter().any(|m| m.ident.sym == "init")
                    && !cls.fields.is_empty()
                {
                    let mock_span = Span { start: 0, end: 0 };
                    let mut init_params = Vec::new();
                    let mut init_stmts = Vec::new();
                    for f in &cls.fields {
                        init_params.push(Param {
                            span: mock_span,
                            ident: f.ident.clone(),
                            ty: f.ty.clone(),
                            is_mut: false,
                            is_move: matches!(f.ty, Type::Ref(_) | Type::BoxDyn(_)),
                            is_comptime: false,
                            is_noalias: false,
                            default: None,
                            meta: vec![],
                        });
                        init_stmts.push(Stmt::Expr(Expr::Bin(BinExpr {
                            span: mock_span,
                            op: BinaryOp::Assign,
                            left: Box::new(Expr::Member(MemberExpr {
                                span: mock_span,
                                obj: Box::new(Expr::Ident(Ident {
                                    span: mock_span,
                                    sym: "_".to_string(),
                                })),
                                prop: f.ident.clone(),
                            })),
                            right: Box::new(Expr::Ident(f.ident.clone())),
                        })));
                    }
                    cls.methods.insert(
                        0,
                        Function {
                            span: mock_span,
                            ident: Ident {
                                span: mock_span,
                                sym: "init".to_string(),
                            },
                            generic_params: vec![],
                            params: init_params,
                            return_type: Some(Type::Void),
                            body: BlockStmt {
                                span: mock_span,
                                body: init_stmts,
                            },
                            is_async: false,
                            is_static: false,
                            is_unsafe: false,
                            meta: vec![],
                            is_pub: true,
                        },
                    );
                }
            }
        }

        Ok(())
    }

    pub(super) fn expand_single_macro(
        &self,
        macro_name: &str,
        cls: &Class,
        meta_item: &MetaItem,
    ) -> SemaResult<Vec<Function>> {
        match macro_name {
            "Json" => Ok(vec![self.derive_json_method(cls)]),
            "Table" => Ok(vec![self.derive_table_method(cls, meta_item)]),
            "ToString" => Ok(vec![self.derive_tostring_method(cls)]),
            "Debug" => Ok(self.derive_debug_methods(cls)),
            "Clone" => Ok(vec![self.derive_clone_method(cls)]),
            "Comparable" => Ok(self.derive_comparable_methods(cls)),
            "Hash" => Ok(vec![self.derive_hash_method(cls)]),
            "Default" => Ok(vec![self.derive_default_method(cls)]),
            _ => self.derive_custom_macro_methods(macro_name, cls, meta_item),
        }
    }

    pub(super) fn derive_debug_methods(&self, cls: &Class) -> Vec<Function> {
        let mock_span = Span { start: 0, end: 0 };
        let to_str = self.derive_tostring_method(cls);
        let mut fmt_debug = to_str.clone();
        fmt_debug.ident = Ident {
            span: mock_span,
            sym: "fmt_debug".to_string(),
        };
        vec![to_str, fmt_debug]
    }

    pub(super) fn derive_clone_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let mut args = Vec::new();
        for f in &cls.fields {
            args.push(Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "_".to_string(),
                })),
                prop: f.ident.clone(),
            }));
        }
        let ret_expr = Expr::New(CallExpr {
            span: mock_span,
            callee: Box::new(Expr::Ident(cls.ident.clone())),
            args,
            is_comptime: false,
        });
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "clone".to_string(),
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::Ref(cls.ident.clone())),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(ret_expr),
                })],
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_default_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let mut args = Vec::new();
        for f in &cls.fields {
            let default_val = match &f.ty {
                Type::I8 | Type::I16 | Type::I32 | Type::I64 | Type::I128 | Type::ISize => {
                    Expr::Lit(Lit::Int(0))
                }
                Type::U8 | Type::U16 | Type::U32 | Type::U64 | Type::U128 | Type::USize => {
                    Expr::Lit(Lit::Int(0))
                }
                Type::F32 | Type::F64 => Expr::Lit(Lit::Float(0.0)),
                Type::Bool => Expr::Lit(Lit::Bool(false)),
                Type::String => Expr::Lit(Lit::Str(String::new())),
                Type::Optional(_) => Expr::Lit(Lit::Null),
                Type::Array(_) => Expr::Array(vec![]),
                _ => Expr::Lit(Lit::Null),
            };
            args.push(default_val);
        }
        let ret_expr = Expr::New(CallExpr {
            span: mock_span,
            callee: Box::new(Expr::Ident(cls.ident.clone())),
            args,
            is_comptime: false,
        });
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "default".to_string(),
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::Ref(cls.ident.clone())),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(ret_expr),
                })],
            },
            is_async: false,
            is_static: true,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_comparable_methods(&self, cls: &Class) -> Vec<Function> {
        vec![self.derive_eq_method(cls), self.derive_cmp_method(cls)]
    }

    pub(super) fn derive_eq_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let mut body = Vec::new();
        for f in &cls.fields {
            let self_access = Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "_".to_string(),
                })),
                prop: f.ident.clone(),
            });
            let other_access = Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "other".to_string(),
                })),
                prop: f.ident.clone(),
            });
            let not_eq = Expr::Bin(BinExpr {
                span: mock_span,
                op: BinaryOp::NotEq,
                left: Box::new(self_access),
                right: Box::new(other_access),
            });
            body.push(Stmt::If(IfStmt {
                span: mock_span,
                test: not_eq,
                cons: Box::new(Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(Expr::Lit(Lit::Bool(false))),
                })),
                alt: None,
                is_comptime: false,
                capture: None,
                is_ptr_capture: false,
            }));
        }
        body.push(Stmt::Return(crate::ast::ReturnStmt {
            span: mock_span,
            arg: Some(Expr::Lit(Lit::Bool(true))),
        }));
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "eq".to_string(),
            },
            generic_params: vec![],
            params: vec![Param {
                span: mock_span,
                ident: Ident {
                    span: mock_span,
                    sym: "other".to_string(),
                },
                ty: Type::Ref(cls.ident.clone()),
                is_mut: false,
                is_move: false,
                is_comptime: false,
                is_noalias: false,
                default: None,
                meta: vec![],
            }],
            return_type: Some(Type::Bool),
            body: BlockStmt {
                span: mock_span,
                body,
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_cmp_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let mut body = Vec::new();
        for f in &cls.fields {
            let self_access = Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "_".to_string(),
                })),
                prop: f.ident.clone(),
            });
            let other_access = Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "other".to_string(),
                })),
                prop: f.ident.clone(),
            });
            let lt = Expr::Bin(BinExpr {
                span: mock_span,
                op: BinaryOp::Lt,
                left: Box::new(self_access.clone()),
                right: Box::new(other_access.clone()),
            });
            body.push(Stmt::If(IfStmt {
                span: mock_span,
                test: lt,
                cons: Box::new(Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(Expr::Lit(Lit::Int(-1))),
                })),
                alt: None,
                is_comptime: false,
                capture: None,
                is_ptr_capture: false,
            }));
            let gt = Expr::Bin(BinExpr {
                span: mock_span,
                op: BinaryOp::Gt,
                left: Box::new(self_access),
                right: Box::new(other_access),
            });
            body.push(Stmt::If(IfStmt {
                span: mock_span,
                test: gt,
                cons: Box::new(Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(Expr::Lit(Lit::Int(1))),
                })),
                alt: None,
                is_comptime: false,
                capture: None,
                is_ptr_capture: false,
            }));
        }
        body.push(Stmt::Return(crate::ast::ReturnStmt {
            span: mock_span,
            arg: Some(Expr::Lit(Lit::Int(0))),
        }));
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "cmp".to_string(),
            },
            generic_params: vec![],
            params: vec![Param {
                span: mock_span,
                ident: Ident {
                    span: mock_span,
                    sym: "other".to_string(),
                },
                ty: Type::Ref(cls.ident.clone()),
                is_mut: false,
                is_move: false,
                is_comptime: false,
                is_noalias: false,
                default: None,
                meta: vec![],
            }],
            return_type: Some(Type::I32),
            body: BlockStmt {
                span: mock_span,
                body,
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_hash_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let mut args = Vec::new();
        for f in &cls.fields {
            args.push(Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "_".to_string(),
                })),
                prop: f.ident.clone(),
            }));
        }
        let ret_expr = Expr::Builtin(BuiltinCall {
            span: mock_span,
            name: "@hash".to_string(),
            args,
        });
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "hash".to_string(),
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::USize),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(ret_expr),
                })],
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_json_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };

        // 过滤 @json_ignore 字段，并提取 @json("alias") 重命名
        let mut active_fields: Vec<(&Field, String)> = Vec::new();
        for f in &cls.fields {
            let is_ignored = f.meta.iter().any(|m| m.name.sym == "json_ignore");
            if is_ignored {
                continue;
            }
            let mut key_name = f.ident.sym.clone();
            for m in &f.meta {
                if m.name.sym == "json" {
                    if let Some(arg) = m.args.first() {
                        match arg {
                            MetaArg::Positional(Expr::Lit(Lit::Str(s))) => key_name = s.clone(),
                            MetaArg::Named(_, Expr::Lit(Lit::Str(s))) => key_name = s.clone(),
                            _ => {}
                        }
                    }
                }
            }
            active_fields.push((f, key_name));
        }

        let ret_expr = if active_fields.is_empty() {
            Expr::Lit(Lit::Str("{}".to_string()))
        } else {
            let mut expr = Expr::Lit(Lit::Str("{".to_string()));
            for (idx, (f, key_name)) in active_fields.iter().enumerate() {
                let prefix = if idx == 0 {
                    format!("\"{}\":", key_name)
                } else {
                    format!(",\"{}\":", key_name)
                };
                expr = Expr::Bin(BinExpr {
                    span: mock_span,
                    op: BinaryOp::Add,
                    left: Box::new(expr),
                    right: Box::new(Expr::Lit(Lit::Str(prefix))),
                });

                let field_access = Expr::Member(MemberExpr {
                    span: mock_span,
                    obj: Box::new(Expr::Ident(Ident {
                        span: mock_span,
                        sym: "_".to_string(),
                    })),
                    prop: f.ident.clone(),
                });

                let val_expr = match &f.ty {
                    Type::String => Expr::Bin(BinExpr {
                        span: mock_span,
                        op: BinaryOp::Add,
                        left: Box::new(Expr::Lit(Lit::Str("\"".to_string()))),
                        right: Box::new(Expr::Bin(BinExpr {
                            span: mock_span,
                            op: BinaryOp::Add,
                            left: Box::new(field_access),
                            right: Box::new(Expr::Lit(Lit::Str("\"".to_string()))),
                        })),
                    }),
                    Type::Ref(id) if self.classes.contains_key(&id.sym) => {
                        let ref_cls = &self.classes[&id.sym];
                        let has_json_macro = ref_cls.meta.iter().any(|m| m.name.sym == "Json")
                            || ref_cls.methods.iter().any(|m| m.ident.sym == "toJson");
                        if has_json_macro {
                            Expr::Call(CallExpr {
                                span: mock_span,
                                callee: Box::new(Expr::Member(MemberExpr {
                                    span: mock_span,
                                    obj: Box::new(field_access),
                                    prop: Ident {
                                        span: mock_span,
                                        sym: "toJson".to_string(),
                                    },
                                })),
                                args: vec![],
                                is_comptime: false,
                            })
                        } else {
                            field_access
                        }
                    }
                    _ => field_access,
                };

                expr = Expr::Bin(BinExpr {
                    span: mock_span,
                    op: BinaryOp::Add,
                    left: Box::new(expr),
                    right: Box::new(val_expr),
                });
            }

            Expr::Bin(BinExpr {
                span: mock_span,
                op: BinaryOp::Add,
                left: Box::new(expr),
                right: Box::new(Expr::Lit(Lit::Str("}".to_string()))),
            })
        };

        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "toJson".to_string(),
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::String),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(ret_expr),
                })],
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![], // Macro Hygiene!
            is_pub: true,
        }
    }

    pub(super) fn derive_table_method(&self, cls: &Class, meta_item: &MetaItem) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let table_name = if let Some(arg) = meta_item.args.first() {
            match arg {
                MetaArg::Positional(Expr::Lit(Lit::Str(s))) => s.clone(),
                MetaArg::Named(_, Expr::Lit(Lit::Str(s))) => s.clone(),
                _ => cls.ident.sym.to_lowercase(),
            }
        } else {
            cls.ident.sym.to_lowercase()
        };
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "getTableName".to_string(),
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::String),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(Expr::Lit(Lit::Str(table_name))),
                })],
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_tostring_method(&self, cls: &Class) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let mut expr = Expr::Lit(Lit::Str(format!("{}(", cls.ident.sym)));
        for (idx, f) in cls.fields.iter().enumerate() {
            let prefix = if idx == 0 {
                format!("{}=", f.ident.sym)
            } else {
                format!(", {}=", f.ident.sym)
            };
            expr = Expr::Bin(BinExpr {
                span: mock_span,
                op: BinaryOp::Add,
                left: Box::new(expr),
                right: Box::new(Expr::Lit(Lit::Str(prefix))),
            });
            let field_access = Expr::Member(MemberExpr {
                span: mock_span,
                obj: Box::new(Expr::Ident(Ident {
                    span: mock_span,
                    sym: "_".to_string(),
                })),
                prop: f.ident.clone(),
            });
            expr = Expr::Bin(BinExpr {
                span: mock_span,
                op: BinaryOp::Add,
                left: Box::new(expr),
                right: Box::new(field_access),
            });
        }
        expr = Expr::Bin(BinExpr {
            span: mock_span,
            op: BinaryOp::Add,
            left: Box::new(expr),
            right: Box::new(Expr::Lit(Lit::Str(")".to_string()))),
        });
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: "toString".to_string(),
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::String),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(expr),
                })],
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_custom_macro_method(
        &self,
        macro_name: &str,
        _cls: &Class,
        meta_item: &MetaItem,
    ) -> Function {
        let mock_span = Span { start: 0, end: 0 };
        let method_name = format!("get{}", macro_name);
        let return_val = if let Some(arg) = meta_item.args.first() {
            match arg {
                MetaArg::Positional(Expr::Lit(Lit::Str(s))) => s.clone(),
                MetaArg::Named(_, Expr::Lit(Lit::Str(s))) => s.clone(),
                _ => macro_name.to_string(),
            }
        } else {
            macro_name.to_string()
        };
        Function {
            span: mock_span,
            ident: Ident {
                span: mock_span,
                sym: method_name,
            },
            generic_params: vec![],
            params: vec![],
            return_type: Some(Type::String),
            body: BlockStmt {
                span: mock_span,
                body: vec![Stmt::Return(crate::ast::ReturnStmt {
                    span: mock_span,
                    arg: Some(Expr::Lit(Lit::Str(return_val))),
                })],
            },
            is_async: false,
            is_static: false,
            is_unsafe: false,
            meta: vec![],
            is_pub: true,
        }
    }

    pub(super) fn derive_custom_macro_methods(
        &self,
        macro_name: &str,
        cls: &Class,
        meta_item: &MetaItem,
    ) -> SemaResult<Vec<Function>> {
        if let Some(macro_def) = self.macro_defs.get(macro_name) {
            if !macro_def.methods.is_empty() {
                let mut subst: HashMap<String, String> = HashMap::new();
                subst.insert("${target}".to_string(), cls.ident.sym.clone());
                subst.insert("target".to_string(), cls.ident.sym.clone());

                let mut arg_idx = 0;
                for (p_idx, p) in macro_def.params.iter().enumerate() {
                    if p_idx == 0
                        && (p.ident.sym == "target" || p.ident.sym == "this" || p.ident.sym == "_")
                    {
                        continue;
                    }
                    let val_opt = meta_item
                        .args
                        .iter()
                        .find_map(|a| match a {
                            MetaArg::Named(id, e) if id.sym == p.ident.sym => {
                                Some(Self::eval_meta_arg_str(e))
                            }
                            _ => None,
                        })
                        .or_else(|| {
                            if arg_idx < meta_item.args.len() {
                                let a = &meta_item.args[arg_idx];
                                arg_idx += 1;
                                match a {
                                    MetaArg::Positional(e) => Some(Self::eval_meta_arg_str(e)),
                                    _ => None,
                                }
                            } else {
                                None
                            }
                        });

                    if let Some(val) = val_opt {
                        subst.insert(format!("${{{}}}", p.ident.sym), val.clone());
                        subst.insert(format!("${}", p.ident.sym), val.clone());
                        subst.insert(p.ident.sym.clone(), val);
                    }
                }

                let mut subst_ordered: Vec<(&String, &String)> = subst.iter().collect();
                subst_ordered.sort_by_key(|(k, _)| std::cmp::Reverse(k.len()));

                let mut expanded = Vec::new();
                for m in &macro_def.methods {
                    let mut instantiated = m.clone();
                    Self::substitute_function(&mut instantiated, &subst_ordered);
                    instantiated.meta.clear(); // Macro Hygiene!
                    expanded.push(instantiated);
                }
                return Ok(expanded);
            }
        }

        Ok(vec![
            self.derive_custom_macro_method(macro_name, cls, meta_item)
        ])
    }

    pub(super) fn eval_meta_arg_str(expr: &Expr) -> String {
        match expr {
            Expr::Lit(Lit::Str(s)) => s.clone(),
            Expr::Ident(id) => id.sym.clone(),
            Expr::Lit(Lit::Int(n)) => n.to_string(),
            Expr::Lit(Lit::Float(f)) => f.to_string(),
            Expr::Lit(Lit::Bool(b)) => b.to_string(),
            _ => "".to_string(),
        }
    }

    pub(super) fn substitute_type(ty: &mut Type, subst: &[(&String, &String)]) {
        match ty {
            Type::Ref(id) => {
                for (k, v) in subst {
                    if id.sym == **k {
                        id.sym = (*v).clone();
                    } else if id.sym.contains(*k) {
                        id.sym = id.sym.replace(*k, v);
                    }
                }
                match id.sym.as_str() {
                    "string" => *ty = Type::String,
                    "i32" => *ty = Type::I32,
                    "u32" => *ty = Type::U32,
                    "i64" => *ty = Type::I64,
                    "u64" => *ty = Type::U64,
                    "i128" => *ty = Type::I128,
                    "u128" => *ty = Type::U128,
                    "f32" => *ty = Type::F32,
                    "f64" => *ty = Type::F64,
                    "usize" => *ty = Type::USize,
                    "bool" => *ty = Type::Bool,
                    "void" => *ty = Type::Void,
                    _ => {}
                    _ => {}
                }
            }
            Type::BorrowType { inner, .. } => Self::substitute_type(inner, subst),
            Type::Optional(inner) => Self::substitute_type(inner, subst),
            Type::Array(inner) => Self::substitute_type(inner, subst),
            _ => {}
            _ => {}
        }
    }

    pub(super) fn substitute_function(func: &mut Function, subst: &[(&String, &String)]) {
        for (k, v) in subst {
            if func.ident.sym == **k {
                func.ident.sym = (*v).clone();
            } else if func.ident.sym.contains(*k) {
                func.ident.sym = func.ident.sym.replace(*k, v);
            }
        }
        if let Some(ret) = &mut func.return_type {
            Self::substitute_type(ret, subst);
        }
        for p in &mut func.params {
            for (k, v) in subst {
                if p.ident.sym == **k {
                    p.ident.sym = (*v).clone();
                } else if p.ident.sym.contains(*k) {
                    p.ident.sym = p.ident.sym.replace(*k, v);
                }
            }
            Self::substitute_type(&mut p.ty, subst);
        }
        for s in &mut func.body.body {
            Self::substitute_stmt(s, subst);
        }
    }

    pub(super) fn substitute_stmt(stmt: &mut Stmt, subst: &[(&String, &String)]) {
        match stmt {
            Stmt::Expr(e) => Self::substitute_expr(e, subst),
            Stmt::Return(r) => {
                if let Some(arg) = &mut r.arg {
                    Self::substitute_expr(arg, subst);
                }
            }
            Stmt::Break | Stmt::Continue => {}
            Stmt::If(i) => {
                Self::substitute_expr(&mut i.test, subst);
                Self::substitute_stmt(&mut i.cons, subst);
                if let Some(alt) = &mut i.alt {
                    Self::substitute_stmt(alt, subst);
                }
            }
            Stmt::While(w) => {
                Self::substitute_expr(&mut w.test, subst);
                if let Some(c) = &mut w.continue_expr {
                    Self::substitute_expr(c, subst);
                }
                Self::substitute_stmt(&mut w.body, subst);
            }
            Stmt::For(f) => {
                Self::substitute_expr(&mut f.expr, subst);
                Self::substitute_stmt(&mut f.body, subst);
            }
            Stmt::Block(b) | Stmt::ComptimeBlock(b) | Stmt::UnsafeBlock(b) => {
                for s in &mut b.body {
                    Self::substitute_stmt(s, subst);
                }
            }
            Stmt::Var(v) => {
                if let Pattern::Ident(id) = &mut v.pat {
                    for (k, v_sub) in subst {
                        if id.sym == **k {
                            id.sym = (*v_sub).clone();
                        } else if id.sym.contains(*k) {
                            id.sym = id.sym.replace(*k, v_sub);
                        }
                    }
                }
                if let Some(ty) = &mut v.ty {
                    Self::substitute_type(ty, subst);
                }
                if let Some(init) = &mut v.init {
                    Self::substitute_expr(init, subst);
                }
            }
            _ => {}
            _ => {}
        }
    }

    pub(super) fn substitute_expr(expr: &mut Expr, subst: &[(&String, &String)]) {
        match expr {
            Expr::Ident(id) => {
                for (k, v) in subst {
                    if id.sym == **k {
                        id.sym = (*v).clone();
                    } else if id.sym.contains(*k) {
                        id.sym = id.sym.replace(*k, v);
                    }
                }
            }
            Expr::Member(m) => {
                Self::substitute_expr(&mut m.obj, subst);
                for (k, v) in subst {
                    if m.prop.sym == **k {
                        m.prop.sym = (*v).clone();
                    } else if m.prop.sym.contains(*k) {
                        m.prop.sym = m.prop.sym.replace(*k, v);
                    }
                }
            }
            Expr::Bin(b) => {
                Self::substitute_expr(&mut b.left, subst);
                Self::substitute_expr(&mut b.right, subst);
            }
            Expr::Neg(inner) | Expr::BitNot(inner) => {
                Self::substitute_expr(inner, subst);
            }
            Expr::Call(c) | Expr::New(c) | Expr::OptCall(c) => {
                Self::substitute_expr(&mut c.callee, subst);
                for a in &mut c.args {
                    Self::substitute_expr(a, subst);
                }
            }
            Expr::Lit(Lit::Str(s)) => {
                for (k, v) in subst {
                    if s.contains(*k) {
                        *s = s.replace(*k, v);
                    }
                }
            }
            Expr::Template(t) => {
                for q in &mut t.quasis {
                    for (k, v) in subst {
                        if q.contains(*k) {
                            *q = q.replace(*k, v);
                        }
                    }
                }
                for e in &mut t.expressions {
                    Self::substitute_expr(e, subst);
                }
            }
            Expr::Builtin(b) => {
                for a in &mut b.args {
                    Self::substitute_expr(a, subst);
                }
            }
            Expr::Array(items) => {
                for item in items {
                    Self::substitute_expr(item, subst);
                }
            }
            Expr::ArrayRepeat(val, count) => {
                Self::substitute_expr(val, subst);
                Self::substitute_expr(count, subst);
            }
            _ => {}
            _ => {}
        }
    }

    pub(super) fn is_const_expr(expr: &Expr) -> bool {
        match expr {
            Expr::Lit(_) => true,
            Expr::Ident(_) => true,
            Expr::EnumInit(ei) => {
                ei.fields
                    .as_ref()
                    .map_or(true, |fs| fs.iter().all(|f| Self::is_const_expr(&f.val)))
                    && ei
                        .tuple_args
                        .as_ref()
                        .map_or(true, |args| args.iter().all(Self::is_const_expr))
            }
            Expr::Member(mem) => Self::is_const_expr(&mem.obj),
            Expr::Array(elements) => elements.iter().all(Self::is_const_expr),
            Expr::ArrayRepeat(val, count) => Self::is_const_expr(val) && Self::is_const_expr(count),
            Expr::Object(fields) => fields.iter().all(|f| Self::is_const_expr(&f.val)),
            Expr::Dict(fields) => fields.iter().all(|f| Self::is_const_expr(&f.val)),
            Expr::Neg(inner) | Expr::BitNot(inner) => Self::is_const_expr(inner),
            _ => false,
        }
    }
}
