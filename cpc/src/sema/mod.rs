mod coerce;
mod decl;
mod macros;
mod safety;
mod stmt_expr;
mod type_infer;

use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;
use thiserror::Error;

#[derive(Error, Debug, Clone)]
pub enum SemaError {
    #[error("Diagnostic: {0:?}")]
    Diagnostic(Diagnostic),
    #[error("Undefined symbol: {0} at {1:?}")]
    UndefinedSymbol(String, Span),
    #[error("Type mismatch: expected {0:?}, found {1:?} at {2:?}")]
    TypeMismatch(Type, Type, Span),
    #[error("Not a function at {0:?}")]
    NotAFunction(Span),
    #[error("Const assignment to {0} at {1:?}")]
    ConstAssignment(String, Span),
    #[error("Invalid await at {0:?}")]
    InvalidAwait(Span),
    #[error("Await can only be used in async functions at {0:?}")]
    AwaitOutsideAsync(Span),
    #[error("Use of moved value '{0}' at {1:?}")]
    UseOfMovedValue(String, Span),
    #[error("Cannot mutate a read-only borrow '{0}' at {1:?}")]
    CannotMutateReadOnlyBorrow(String, Span),
    #[error("Cannot reassign to read-only parameter '{0}' at {1:?}")]
    ParameterReassignment(String, Span),
    #[error("Reserved identifier '{0}' at {1:?}")]
    ReservedIdentifier(String, Span, bool),
    #[error("Cannot borrow '{0}' as mutable because it is also borrowed as immutable in the current scope at {1:?}")]
    MutableBorrowConflict(String, Span),
    #[error(
        "Compile Error: Meta '@{0}' can only be applied to {1}, but was applied to {2} at {3:?}"
    )]
    MetaInvalidTarget(String, String, String, Span),
    #[error("Compile Error: Meta '@{0}' is marked with '@once' and cannot be applied multiple times at {1:?}")]
    MetaDuplicateOnce(String, Span),
    #[error("Compile Error: Argument to meta '@{0}' must be a compile-time constant at {1:?}")]
    MetaArgNotConstant(String, Span),
    #[error("Compile Error: Type '{0}' cannot be reflected at runtime. Add '@reflect' to the class definition to enable runtime reflection at {1:?}")]
    ReflectNotEnabled(String, Span),
    #[error("Compile Error: Decorator '@{0}' can only be applied to {1}, but was applied to {2} at {3:?}")]
    DecInvalidTarget(String, String, String, Span),
    #[error(
        "Compile Error: Macro cycle detected: '@{0}' recursively invoked on class '{1}' at {2:?}"
    )]
    MacroCycleDetected(String, String, Span),
    #[error("Compile Error: Macro recursion limit (128) exceeded in '@{0}' at {1:?}")]
    MacroRecursionLimitExceeded(String, Span),
    #[error(
        "Compile Error: Macro '@{0}' can only be applied to {1}, but was applied to {2} at {3:?}"
    )]
    MacroInvalidTarget(String, String, String, Span),
    #[error("Compile Error: Type-directed macro '{0}' cannot create new top-level types (classes or structs) at {1:?}")]
    MacroProhibitedTypeCreation(String, Span),
    #[error("Compile Error: Macro hygiene violation: generated code from '@{0}' cannot carry macro annotations at {1:?}")]
    MacroHygieneViolation(String, Span),
    #[error("Compile Error: Pipeline placeholder '.' can only be used on the right-hand side of a pipeline ('|>') operator at {0:?}")]
    PipePlaceholderOutsidePipeline(Span),
    #[error(
        "Compile Error: Cannot mutate '{0}' while it is borrowed in 'for ... of' loop at {1:?}"
    )]
    MutateWhileBorrowedInFor(String, Span),
    #[error("Compile Error: {0} at {1:?}")]
    UnsafeRequired(String, Span),
    #[error("Compile Error: {0} at {1:?}")]
    InvalidCast(String, Span),
    #[error("Compile Error: Cannot redeclare block-scoped variable '{0}' at {1:?}")]
    DuplicateVarDecl(String, Span),
    #[error(
        "Compile Error: Destructuring pattern cannot contain multiple '...' rest elements at {0:?}"
    )]
    MultipleRestInPattern(Span),
    #[error("Compile Error: {0} at {1:?}")]
    PrivateAccess(String, Span),
    #[error("Compile Error: Cannot instantiate static class or struct '{0}' at {1:?}")]
    CannotInstantiateStatic(String, Span),
    #[error("Compile Error: {0} at {1:?}")]
    InvalidPointerArithmetic(String, Span),
    #[error(
        "Compile Error: Index {index} is out of bounds for buffer of length {len} at {span:?}"
    )]
    CompileTimeIndexOutOfBounds { index: i64, len: usize, span: Span },
    #[error("Compile Error: Negative index {index} at {span:?}")]
    NegativeIndex { index: i64, span: Span },
    #[error("Compile Error: Mutation via unbounded raw pointer or cstr '{0}' requires 'unsafe' block at {1:?}")]
    UnboundedRawPointerMutation(String, Span),
    #[error(
        "Compile Error: Type '{type_name}' does not satisfy trait bound '{trait_name}' at {span:?}"
    )]
    TraitBoundNotSatisfied {
        type_name: String,
        trait_name: String,
        missing_method: Option<String>,
        span: Span,
    },
    #[error(
        "Compile Error: Class '{class_name}' does not satisfy trait '{trait_name}' at {span:?}"
    )]
    TraitMissingMethod {
        class_name: String,
        trait_name: String,
        missing_method: String,
        span: Span,
    },
    #[error(
        "Compile Error: borrowed reference cannot cross async/coroutine boundary without 'move' or 'own' at {span:?}"
    )]
    AsyncIsolationViolation { name: String, span: Span },
}

impl SemaError {
    pub fn to_diagnostic(&self, known_symbols: &[String]) -> Diagnostic {
        match self {
            SemaError::Diagnostic(d) => d.clone(),
            SemaError::TraitBoundNotSatisfied {
                type_name,
                trait_name,
                missing_method,
                span,
            } => {
                let (cause, help, suggestion) = if let Some(method) = missing_method {
                    (
                        format!(
                            "type '{}' is missing required method '{}' from trait '{}'.",
                            type_name, method, trait_name
                        ),
                        format!(
                            "trait '{}' requires method '{}'. Ensure '{}' implements it.",
                            trait_name, method, type_name
                        ),
                        format!(
                            "implement method '{}' on '{}':\n  fn {}(...): void {{ ... }}",
                            method, type_name, method
                        ),
                    )
                } else {
                    (
                        format!(
                            "type '{}' does not satisfy trait bound '{}'.",
                            type_name, trait_name
                        ),
                        format!(
                            "generic parameter requires a type satisfying trait '{}'.",
                            trait_name
                        ),
                        format!(
                            "pass an argument whose type satisfies trait '{}'.",
                            trait_name
                        ),
                    )
                };
                Diagnostic::new(
                    "E0301",
                    format!(
                        "type '{}' does not satisfy trait bound '{}'",
                        type_name, trait_name
                    ),
                )
                .with_span(*span)
                .with_cause(cause)
                .with_help(help)
                .with_suggestion(suggestion)
            }
            SemaError::TraitMissingMethod {
                class_name,
                trait_name,
                missing_method,
                span,
            } => Diagnostic::new(
                "E0301",
                format!("class '{}' does not satisfy trait '{}'", class_name, trait_name),
            )
            .with_span(*span)
            .with_cause(format!(
                "class '{}' declares 'implements {}' but is missing required method '{}'.",
                class_name, trait_name, missing_method
            ))
            .with_help(format!(
                "all methods declared in trait '{}' without default implementations must be implemented.",
                trait_name
            ))
            .with_suggestion(format!(
                "implement method '{}' in class '{}':\n  fn {}(...): void {{ ... }}",
                missing_method, class_name, missing_method
            )),
            SemaError::CannotMutateReadOnlyBorrow(name, span) => {
                let code = if name == "_" || name == "this" || name == "self" {
                    "E0130"
                } else {
                    "E0201"
                };
                Diagnostic::new(
                    code,
                    if code == "E0130" {
                        "cannot perform mutable operation in a read-only method".to_string()
                    } else {
                        format!("cannot mutate read-only borrow '{}'", name)
                    },
                )
                .with_span(*span)
                .with_cause(if code == "E0130" {
                    "method receiver is read-only by default in CP.".to_string()
                } else {
                    format!(
                        "parameter '{}' is a read-only borrow (immutable by default in CP).",
                        name
                    )
                })
                .with_help(if code == "E0130" {
                    "methods that modify instance fields must declare a mutable receiver parameter '*_'."
                } else {
                    "in CP, function parameters are read-only by default. Use '*_' for mutable method receivers or 'v: *T' for mutable parameters."
                })
                .with_suggestion(if code == "E0130" {
                    "add '*_' as the first parameter of the method: fn method(*_) { ... }".to_string()
                } else {
                    format!(
                        "parameter '{}' is a read-only borrow. Add '*' to declare it as mutable: '{}: *<Type>'",
                        name, name
                    )
                })
            }
            SemaError::ParameterReassignment(name, span) => Diagnostic::new(
                "E0133",
                format!("cannot reassign to read-only parameter '{}'", name),
            )
            .with_span(*span)
            .with_cause(format!(
                "function parameter '{}' is a read-only borrow (immutable by default in CP).",
                name
            ))
            .with_help("in CP, function parameters are immutable by default.")
            .with_suggestion(format!(
                "- 需修改调用者数据，请将形参改为可变借用: '{}: *<Type>'\n- 函数内局部游标，请在首行显式声明: 'let {}_local = {};'",
                name, name, name
            )),
            SemaError::ReservedIdentifier(name, span, is_catch_err) => {
                if *is_catch_err {
                    Diagnostic::new(
                        "E0134",
                        format!("reserved identifier '{}' used as catch payload", name),
                    )
                    .with_span(*span)
                    .with_cause(format!("'{}' is a reserved identifier in CP/Zig.", name))
                    .with_help("error catch payload should use 'e' or a descriptive non-reserved identifier.")
                    .with_suggestion("replace 'err' with 'e'")
                } else {
                    Diagnostic::new(
                        "E0134",
                        format!("reserved keyword '{}' used as identifier", name),
                    )
                    .with_span(*span)
                    .with_cause(format!("'{}' is a reserved language keyword in CP/Zig.", name))
                    .with_help("escape the identifier with @\"...\" or rename it.")
                    .with_suggestion(format!("replace with @\"{}\"", name))
                }
            }

            SemaError::MutableBorrowConflict(name, span) => Diagnostic::new(
                "E0202",
                format!("cannot borrow '{}' as mutable because it is already borrowed as immutable", name),
            )
            .with_span(*span)
            .with_cause(format!(
                "variable '{}' is already borrowed as immutable in the current scope.",
                name
            ))
            .with_help("CP enforces aliasing XOR mutability at compile time (single writer OR multiple readers).")
            .with_suggestion(format!(
                "ensure immutable borrows of '{}' end before taking a mutable borrow, or clone the value.",
                name
            )),

            SemaError::UseOfMovedValue(name, span) => Diagnostic::new(
                "E0203",
                format!("use of moved value '{}'", name),
            )
            .with_span(*span)
            .with_cause(format!(
                "value of '{}' was moved and its ownership was transferred.",
                name
            ))
            .with_help("variables moved via 'own' or 'move' transfer ownership and cannot be accessed afterwards.")
            .with_suggestion(format!(
                "avoid moving '{}' if you need to use it later, or pass by borrow '&{}'",
                name, name
            )),

            SemaError::ConstAssignment(name, span) => Diagnostic::new(
                "E0204",
                format!("cannot assign to const variable '{}'", name),
            )
            .with_span(*span)
            .with_cause(format!(
                "variable '{}' is declared with 'const' and cannot be reassigned.",
                name
            ))
            .with_help("variables declared with 'const' are immutable bindings.")
            .with_suggestion(format!(
                "change 'const {}' to 'let {}' to allow reassignment",
                name, name
            )),

            SemaError::AwaitOutsideAsync(span) => Diagnostic::new(
                "E0205",
                "'await' can only be used in async functions",
            )
            .with_span(*span)
            .with_cause("'await' pauses execution and requires state-machine CPS transformation.")
            .with_help("in CP, 'await' can only be used inside functions marked with 'async fn'.")
            .with_suggestion("declare the enclosing function as async: 'async fn ...'"),

            SemaError::UndefinedSymbol(sym, span) => {
                let mut diag = Diagnostic::new("E0206", format!("undefined symbol '{}'", sym))
                    .with_span(*span)
                    .with_cause(format!("identifier '{}' is not declared in current scope or imported modules.", sym));

                if let Some(&(_, mod_path)) = STANDARD_BUILTIN_IMPORTS.iter().find(|(name, _)| *name == sym.as_str()) {
                    diag.help = Some(format!("'{}' is a standard library symbol in '{}'.", sym, mod_path));
                    diag.suggestion = Some(format!("import {{ {} }} from \"{}\";", sym, mod_path));
                } else {
                    let cand_refs: Vec<&str> = known_symbols.iter().map(|s| s.as_str()).collect();
                    if let Some(closest) = find_closest_match(sym, &cand_refs, 3) {
                        diag.help = Some(format!("a symbol with a similar name '{}' exists in scope.", closest));
                        diag.suggestion = Some(format!("did you mean '{}'?", closest));
                    } else {
                        diag.help = Some("declare the identifier before use with 'let' / 'const' or import it from a module.".to_string());
                        diag.suggestion = Some(format!("let {} = ...;", sym));
                    }
                }
                diag
            }

            SemaError::TypeMismatch(expected, found, span) => {
                let mut diag = Diagnostic::new(
                    "E0207",
                    format!("type mismatch: expected '{:?}', found '{:?}'", expected, found),
                )
                .with_span(*span)
                .with_cause(format!(
                    "expression has type '{:?}', but the target requires '{:?}'.",
                    found, expected
                ));

                match (expected, found) {
                    (target, Type::Optional(inner)) if target == inner.as_ref() => {
                        diag.help = Some("the value is an Optional ('T?'). It may be null.".to_string());
                        diag.suggestion = Some("unwrap the optional using '?? <default_value>' or check for null using 'if (val != null)'".to_string());
                    }
                    (target, Type::ErrorUnion(inner)) if target == inner.as_ref() => {
                        diag.help = Some("the value is an Error Union ('T ! Error'). It may contain an error.".to_string());
                        diag.suggestion = Some("propagate the error using '?' (e.g. 'expr ?') or handle it using 'catch'".to_string());
                    }
                    (Type::USize, Type::I32) => {
                        diag.help = Some("CP does not perform implicit integer widening conversions.".to_string());
                        diag.suggestion = Some("cast explicitly using '@as(usize, val)'".to_string());
                    }
                    (target, Type::Optional(inner)) if matches!(inner.as_ref(), Type::Unknown | Type::Any) => {
                        diag.help = Some(format!("'null' cannot be assigned to non-optional type '{:?}'.", target));
                        diag.suggestion = Some(format!("change the type annotation to '{:?}?' to allow null", target));
                    }
                    _ => {
                        diag.help = Some("types must match exactly in CP's strong static type system.".to_string());
                        diag.suggestion = Some(format!("ensure the expression evaluates to '{:?}'", expected));
                    }
                }
                diag
            }

            SemaError::ReflectNotEnabled(type_name, span) => Diagnostic::new(
                "E0208",
                format!("type '{}' cannot be reflected at runtime", type_name),
            )
            .with_span(*span)
            .with_cause(format!("reflection metadata for '{}' was omitted during compilation.", type_name))
            .with_help("CP strips runtime metadata by default for zero memory overhead. Runtime reflection requires the '@reflect' attribute.")
            .with_suggestion(format!("add '@reflect' decorator before the definition: '@reflect class {} {{ ... }}'", type_name)),

            SemaError::UnsafeRequired(msg, span) => Diagnostic::new(
                "E0209",
                msg.clone(),
            )
            .with_span(*span)
            .with_cause("memory-unsafe operation (e.g. raw pointer dereference or C-FFI call) detected outside unsafe block.")
            .with_help("all operations that may violate memory safety must be enclosed in 'unsafe { ... }' blocks or 'unsafe fn'.")
            .with_suggestion("wrap the operation in 'unsafe { ... }'"),

            SemaError::MutateWhileBorrowedInFor(name, span) => Diagnostic::new(
                "E0210",
                format!("cannot mutate '{}' while it is borrowed in 'for ... of' loop", name),
            )
            .with_span(*span)
            .with_cause(format!("collection '{}' is actively borrowed by the iterator.", name))
            .with_help("mutating a container during iteration causes iterator invalidation undefined behavior.")
            .with_suggestion("iterate over a cloned collection or use an index-based 'while' loop"),

            SemaError::PrivateAccess(msg, span) => Diagnostic::new(
                "E0211",
                msg.clone(),
            )
            .with_span(*span)
            .with_cause("attempted to access a private member outside of its defining class.")
            .with_help("members are private by default in CP. Only members with 'pub' are accessible externally.")
            .with_suggestion("add 'pub' modifier to the member declaration, or provide a public getter/method"),

            SemaError::CannotInstantiateStatic(cls_name, span) => Diagnostic::new(
                "E0212",
                format!("cannot instantiate static class '{}'", cls_name),
            )
            .with_span(*span)
            .with_cause(format!("class '{}' is marked with 'static' and cannot be instantiated with 'new'.", cls_name))
            .with_help("static classes only act as namespaces for static functions and constants.")
            .with_suggestion(format!("call static methods directly via '{}.method(...)' without 'new'", cls_name)),

            SemaError::InvalidCast(msg, span) => Diagnostic::new(
                "E0213",
                msg.clone(),
            )
            .with_span(*span)
            .with_cause("semantic type conversions are prohibited via raw 'as' cast.")
            .with_help("use dedicated parser methods for string-to-number parsing.")
            .with_suggestion("use '<Type>.parse(str) ?' instead of 'as'"),

            SemaError::DuplicateVarDecl(name, span) => Diagnostic::new(
                "E0214",
                format!("cannot redeclare variable '{}'", name),
            )
            .with_span(*span)
            .with_cause(format!("identifier '{}' is already bound in the current block scope.", name))
            .with_help("block-scoped variables cannot be redeclared with 'let' in the same scope.")
            .with_suggestion(format!("remove 'let' to reassign to '{}', or rename the new variable", name)),

            SemaError::MultipleRestInPattern(span) => Diagnostic::new(
                "E0215",
                "destructuring pattern cannot contain multiple '...' rest elements",
            )
            .with_span(*span)
            .with_cause("a pattern can only have at most one rest element '...' to avoid ambiguity in slice length.")
            .with_help("ensure only one rest element exists in array or object pattern.")
            .with_suggestion("remove additional '...' rest elements from the pattern"),

            SemaError::NotAFunction(span) => Diagnostic::new(
                "E0216",
                "expression is not a callable function",
            )
            .with_span(*span)
            .with_cause("attempted to invoke a non-function value with '(...)' call syntax.")
            .with_help("verify that the callee is a function, method, or callable object."),

            SemaError::CompileTimeIndexOutOfBounds { index, len, span } => Diagnostic::new(
                "E0217",
                format!("index out of bounds: index {} is out of bounds for buffer of length {}", index, len),
            )
            .with_span(*span)
            .with_cause(format!("index {} exceeds compile-time known length {} of the buffer.", index, len))
            .with_help("accessing a fixed-size array or known-length buffer beyond its bound is a compile-time fatal error.")
            .with_suggestion(format!("ensure index is within 0..{}, or use safe slicing: 'slice = s[0..max_len]'", len)),

            SemaError::NegativeIndex { index, span } => Diagnostic::new(
                "E0218",
                format!("negative index {} is invalid for array indexing", index),
            )
            .with_span(*span)
            .with_cause(format!("array index {} is negative.", index))
            .with_help("array and buffer indices must be non-negative unsigned integers.")
            .with_suggestion("use a non-negative index value: e.g. 0"),

            SemaError::UnboundedRawPointerMutation(name, span) => Diagnostic::new(
                "E0209",
                format!("mutation via unbounded raw pointer or cstr '{}' requires 'unsafe' block", name),
            )
            .with_span(*span)
            .with_cause(format!("modifying memory via unbounded pointer/cstr '{}' without statically known length is unsafe.", name))
            .with_help("raw pointers and unbounded cstr buffers do not carry runtime length metadata. Modifying memory through them can lead to buffer overflows.")
            .with_suggestion(format!("wrap the mutation in 'unsafe {{ {}[...] = ...; }}' or slice it first: 'let slice = {}[0..len]; slice[...] = ...;'", name, name)),

            SemaError::AsyncIsolationViolation { name, span } => {
                let mut diag = Diagnostic::new(
                    "E0305",
                    "borrowed reference cannot cross async/coroutine boundary without 'move' or 'own'",
                )
                .with_span(*span)
                .with_help("cross-coroutine boundary transfer requires owned data (move/own) or concurrency-safe types (Shared<T>, Atomic<T>).");
                if !name.is_empty() {
                    diag = diag
                        .with_cause(format!(
                            "variable '{}' is a borrowed reference or slice and cannot safely cross async/coroutine boundary.",
                            name
                        ))
                        .with_suggestion(format!(
                            "transfer ownership with 'move {}' or wrap in thread-safe container 'Shared.new({})'",
                            name, name
                        ));
                } else {
                    diag = diag
                        .with_cause("borrowed reference or slice cannot safely cross async/coroutine boundary.")
                        .with_suggestion("transfer ownership with 'move' or wrap in thread-safe container 'Shared<T>'");
                }
                diag
            }

            _ => Diagnostic::new("E0290", format!("semantic error: {}", self))
                .with_help("check semantic rules and types according to CP specification."),
        }
    }
}

pub type SemaResult<T> = Result<T, SemaError>;

#[derive(Clone, Debug)]
pub struct MetaDefInfo {
    pub allowed_targets: std::collections::HashSet<String>,
    pub is_once: bool,
}

#[derive(Clone)]
pub enum SymbolKind {
    Var {
        ty: Type,
        kind: VarDeclKind,
    },
    Func {
        params: Vec<Type>,
        ret: Option<Type>,
        is_async: bool,
        is_unsafe: bool,
    },
    Class(Class),
    Trait(TraitDecl),
    Enum(EnumDecl),
}

pub struct Analyzer {
    scopes: Vec<HashMap<String, SymbolKind>>,
    classes: HashMap<String, Class>,
    pub traits: HashMap<String, TraitDecl>,
    pub impls: Vec<ImplDecl>,
    pub functions: HashMap<String, Function>,
    pub active_generic_bounds: HashMap<String, Vec<String>>,
    current_fn_is_async: bool,
    current_fn_is_unsafe: bool,
    unsafe_depth: usize,
    pub symbol_map: HashMap<Span, SymbolKind>,
    pub meta_defs: HashMap<String, MetaDefInfo>,
    pub dec_defs: HashMap<String, DecDef>,
    pub macro_defs: HashMap<String, MacroDef>,
    pub macro_call_stack: Vec<(String, String)>,
    pub loop_borrowed_vars: std::collections::HashSet<String>,
    pub moved_vars: std::collections::HashSet<String>,
    pub current_class: Option<String>,
    pub type_aliases: HashMap<String, Type>,
    pub known_lengths_stack: Vec<HashMap<String, usize>>,
    pub const_int_vars_stack: Vec<HashMap<String, i64>>,
    pub fn_param_vars: Vec<std::collections::HashSet<String>>,
    pub local_borrow_vars: std::collections::HashSet<String>,
    pub current_method_is_mut: bool,
    pub errors: Vec<SemaError>,
}

pub const RESERVED_WORDS: &[&str] = &["in", "as", "type", "arena", "macro"];

impl Analyzer {
    pub fn get_diagnostics(&self) -> Vec<Diagnostic> {
        let known = self.get_known_symbols();
        self.errors
            .iter()
            .map(|e| e.to_diagnostic(&known))
            .collect()
    }

    pub fn get_known_symbols(&self) -> Vec<String> {
        let mut symbols = std::collections::HashSet::new();
        for scope in &self.scopes {
            for key in scope.keys() {
                symbols.insert(key.clone());
            }
        }
        for key in self.classes.keys() {
            symbols.insert(key.clone());
        }
        symbols.into_iter().collect()
    }

    pub fn new() -> Self {
        let mut meta_defs = HashMap::new();
        let mut target_targets = std::collections::HashSet::new();
        target_targets.insert("meta".to_string());
        meta_defs.insert(
            "target".to_string(),
            MetaDefInfo {
                allowed_targets: target_targets,
                is_once: true,
            },
        );
        let mut once_targets = std::collections::HashSet::new();
        once_targets.insert("meta".to_string());
        meta_defs.insert(
            "once".to_string(),
            MetaDefInfo {
                allowed_targets: once_targets,
                is_once: true,
            },
        );
        let mut reflect_targets = std::collections::HashSet::new();
        reflect_targets.insert("class".to_string());
        reflect_targets.insert("struct".to_string());
        meta_defs.insert(
            "reflect".to_string(),
            MetaDefInfo {
                allowed_targets: reflect_targets,
                is_once: true,
            },
        );
        let mut meta_targets = std::collections::HashSet::new();
        meta_targets.insert("class".to_string());
        meta_targets.insert("struct".to_string());
        meta_targets.insert("function".to_string());
        meta_targets.insert("method".to_string());
        meta_targets.insert("field".to_string());
        meta_targets.insert("param".to_string());
        meta_targets.insert("trait".to_string());
        meta_targets.insert("enum".to_string());
        meta_defs.insert(
            "meta".to_string(),
            MetaDefInfo {
                allowed_targets: meta_targets,
                is_once: false,
            },
        );

        let mut macro_defs = HashMap::new();
        let mock_span = Span { start: 0, end: 0 };
        for name in [
            "Json",
            "Table",
            "Clone",
            "ToString",
            "Comparable",
            "Hash",
            "Debug",
            "Default",
        ] {
            macro_defs.insert(
                name.to_string(),
                MacroDef {
                    span: mock_span.clone(),
                    ident: Ident {
                        span: mock_span.clone(),
                        sym: name.to_string(),
                    },
                    params: vec![],
                    return_type: None,
                    body: BlockStmt {
                        span: mock_span.clone(),
                        body: vec![],
                    },
                    methods: vec![],
                    meta: vec![],
                },
            );
        }

        Self {
            scopes: vec![HashMap::new()],
            classes: HashMap::new(),
            traits: HashMap::new(),
            impls: Vec::new(),
            functions: HashMap::new(),
            active_generic_bounds: HashMap::new(),
            current_fn_is_async: false,
            current_fn_is_unsafe: false,
            unsafe_depth: 0,
            symbol_map: HashMap::new(),
            meta_defs,
            dec_defs: HashMap::new(),
            macro_defs,
            macro_call_stack: Vec::new(),
            loop_borrowed_vars: std::collections::HashSet::new(),
            moved_vars: std::collections::HashSet::new(),
            current_class: None,
            type_aliases: HashMap::new(),
            known_lengths_stack: vec![HashMap::new()],
            const_int_vars_stack: vec![HashMap::new()],
            fn_param_vars: Vec::new(),
            local_borrow_vars: std::collections::HashSet::new(),
            current_method_is_mut: false,
            errors: Vec::new(),
        }
    }

    pub fn is_in_unsafe(&self) -> bool {
        self.current_fn_is_unsafe || self.unsafe_depth > 0
    }

    pub fn analyze_module(&mut self, module: &mut Module) -> SemaResult<()> {
        let mock_span = Span { start: 0, end: 0 };
        let regex_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "RegExp".to_string(),
            },
            fields: vec![],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: false,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: false,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes
            .insert("RegExp".to_string(), regex_class.clone());
        self.scopes[0].insert("RegExp".to_string(), SymbolKind::Class(regex_class));
        let set_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "Set".to_string(),
            },
            fields: vec![],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: false,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: false,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes.insert("Set".to_string(), set_class.clone());
        self.scopes[0].insert("Set".to_string(), SymbolKind::Class(set_class));

        let map_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "Map".to_string(),
            },
            fields: vec![],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: false,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: false,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes.insert("Map".to_string(), map_class.clone());
        self.scopes[0].insert("Map".to_string(), SymbolKind::Class(map_class));

        let dict_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "Dict".to_string(),
            },
            fields: vec![],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: false,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: false,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes.insert("Dict".to_string(), dict_class.clone());
        self.scopes[0].insert("Dict".to_string(), SymbolKind::Class(dict_class));

        let error_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "Error".to_string(),
            },
            fields: vec![
                crate::ast::Field {
                    span: mock_span.clone(),
                    ident: crate::ast::Ident {
                        span: mock_span.clone(),
                        sym: "code".to_string(),
                    },
                    ty: Type::I32,
                    meta: Vec::new(),
                    default: None,
                    is_pub: true,
                    is_static: false,
                },
                crate::ast::Field {
                    span: mock_span.clone(),
                    ident: crate::ast::Ident {
                        span: mock_span.clone(),
                        sym: "msg".to_string(),
                    },
                    ty: Type::String,
                    meta: Vec::new(),
                    default: None,
                    is_pub: true,
                    is_static: false,
                },
            ],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: false,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: false,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes
            .insert("Error".to_string(), error_class.clone());
        self.scopes[0].insert("Error".to_string(), SymbolKind::Class(error_class.clone()));
        self.classes.insert("Err".to_string(), error_class.clone());
        self.scopes[0].insert("Err".to_string(), SymbolKind::Class(error_class));

        let box_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "Box".to_string(),
            },
            fields: vec![],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: false,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: false,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes.insert("Box".to_string(), box_class.clone());
        self.scopes[0].insert("Box".to_string(), SymbolKind::Class(box_class));

        let json_class = crate::ast::Class {
            span: mock_span.clone(),
            ident: crate::ast::Ident {
                span: mock_span.clone(),
                sym: "JSON".to_string(),
            },
            fields: vec![],
            methods: vec![],
            implements: vec![],
            extends: None,
            is_ref: true,
            meta: Vec::new(),
            extra_zig: vec![],
            is_pub: true,
            is_static: true,
            is_union: false,
            nested_classes: vec![],
            generic_params: vec![],
        };
        self.classes.insert("JSON".to_string(), json_class.clone());
        self.scopes[0].insert("JSON".to_string(), SymbolKind::Class(json_class));
        self.scopes[0].insert(
            "std".to_string(),
            SymbolKind::Var {
                ty: Type::Any,
                kind: VarDeclKind::Const,
            },
        );
        self.scopes[0].insert(
            "builtin".to_string(),
            SymbolKind::Var {
                ty: Type::Any,
                kind: VarDeclKind::Const,
            },
        );

        for reflect_name in &["TypeInfo", "FieldInfo", "MethodInfo"] {
            let cls = crate::ast::Class {
                span: mock_span.clone(),
                ident: crate::ast::Ident {
                    span: mock_span.clone(),
                    sym: reflect_name.to_string(),
                },
                fields: vec![],
                methods: vec![],
                implements: vec![],
                extends: None,
                is_ref: false,
                extra_zig: vec![],
                meta: vec![crate::ast::MetaItem {
                    span: mock_span.clone(),
                    name: crate::ast::Ident {
                        span: mock_span.clone(),
                        sym: "reflect".to_string(),
                    },
                    args: vec![],
                }],
                is_pub: true,
                is_static: false,
                is_union: false,
                nested_classes: vec![],
                generic_params: vec![],
            };
            self.classes.insert(reflect_name.to_string(), cls.clone());
            self.scopes[0].insert(reflect_name.to_string(), SymbolKind::Class(cls));
        }

        for name in &[
            "Task",
            "Event",
            "EventEmitter",
            "Set",
            "VecDeque",
            "BinaryHeap",
            "BTreeMap",
            "BTreeSet",
            "HashMap",
            "HashSet",
        ] {
            let cls = crate::ast::Class {
                span: mock_span.clone(),
                ident: crate::ast::Ident {
                    span: mock_span.clone(),
                    sym: name.to_string(),
                },
                fields: vec![],
                methods: vec![],
                implements: vec![],
                extends: None,
                is_ref: false,
                extra_zig: vec![],
                meta: Vec::new(),
                is_pub: true,
                is_static: false,
                is_union: false,
                nested_classes: vec![],
                generic_params: vec![],
            };
            self.classes.insert(name.to_string(), cls.clone());
            self.scopes[0].insert(name.to_string(), SymbolKind::Class(cls));
        }

        for (fn_name, is_async, ret_ty) in &[
            (
                "spawn",
                false,
                Some(Type::Ref(Ident {
                    span: mock_span.clone(),
                    sym: "Task".to_string(),
                })),
            ),
            (
                "@task_spawn",
                false,
                Some(Type::Ref(Ident {
                    span: mock_span.clone(),
                    sym: "Task".to_string(),
                })),
            ),
            (
                "@task_spawn_cpu",
                false,
                Some(Type::Ref(Ident {
                    span: mock_span.clone(),
                    sym: "Task".to_string(),
                })),
            ),
            ("join", true, Some(Type::Any)),
            ("@task_join", true, Some(Type::Any)),
            (
                "@create_pending_task",
                false,
                Some(Type::Ref(Ident {
                    span: mock_span.clone(),
                    sym: "Task".to_string(),
                })),
            ),
            ("@complete_task", false, Some(Type::Void)),
        ] {
            self.scopes[0].insert(
                fn_name.to_string(),
                SymbolKind::Func {
                    params: vec![],
                    ret: ret_ty.clone(),
                    is_async: *is_async,
                    is_unsafe: false,
                },
            );
        }

        for item in &module.body {
            if let ModuleItem::Stmt(stmt) = item {
                self.extract_zig_code_symbols(stmt);
            }
        }

        for item in &module.body {
            match item {
                ModuleItem::Decl(Decl::Meta(m)) => {
                    self.register_meta_def(m)?;
                }
                ModuleItem::Decl(Decl::Dec(d)) => {
                    self.dec_defs.insert(d.ident.sym.clone(), d.clone());
                    self.define(
                        d.ident.sym.clone(),
                        SymbolKind::Func {
                            params: d.params.iter().map(|p| p.ty.clone()).collect(),
                            ret: d.return_type.clone(),
                            is_async: false,
                            is_unsafe: false,
                        },
                        d.ident.span,
                    )?;
                }
                ModuleItem::Decl(Decl::Macro(m)) => {
                    self.macro_defs.insert(m.ident.sym.clone(), m.clone());
                    self.define(
                        m.ident.sym.clone(),
                        SymbolKind::Func {
                            params: m.params.iter().map(|p| p.ty.clone()).collect(),
                            ret: Some(crate::ast::Type::Void),
                            is_async: false,
                            is_unsafe: false,
                        },
                        m.ident.span,
                    )?;
                }
                ModuleItem::Decl(Decl::Class(c)) => {
                    self.classes.insert(c.ident.sym.clone(), c.clone());
                    self.define(
                        c.ident.sym.clone(),
                        SymbolKind::Class(c.clone()),
                        c.ident.span,
                    )?;
                    self.symbol_map
                        .insert(c.ident.span, SymbolKind::Class(c.clone()));
                }
                ModuleItem::Decl(Decl::Trait(t)) => {
                    self.traits.insert(t.ident.sym.clone(), t.clone());
                    self.define(
                        t.ident.sym.clone(),
                        SymbolKind::Trait(t.clone()),
                        t.ident.span,
                    )?;
                    self.symbol_map
                        .insert(t.ident.span, SymbolKind::Trait(t.clone()));
                }
                ModuleItem::Decl(Decl::Impl(i)) => {
                    self.impls.push(i.clone());
                }
                ModuleItem::Decl(Decl::Enum(e)) => {
                    self.define(
                        e.ident.sym.clone(),
                        SymbolKind::Enum(e.clone()),
                        e.ident.span,
                    )?;
                    self.symbol_map
                        .insert(e.ident.span, SymbolKind::Enum(e.clone()));
                }
                ModuleItem::Decl(Decl::Func(f)) => {
                    self.functions.insert(f.ident.sym.clone(), f.clone());
                    let params = f.params.iter().map(|p| p.ty.clone()).collect();
                    let kind = SymbolKind::Func {
                        params,
                        ret: f.return_type.clone(),
                        is_async: f.is_async,
                        is_unsafe: f.is_unsafe,
                    };
                    self.define(f.ident.sym.clone(), kind.clone(), f.ident.span)?;
                    self.symbol_map.insert(f.ident.span, kind);
                }
                ModuleItem::Decl(Decl::TypeAlias(ta)) => {
                    self.type_aliases
                        .insert(ta.ident.sym.clone(), ta.target_ty.clone());
                    let _ = self.define(
                        ta.ident.sym.clone(),
                        SymbolKind::Var {
                            ty: Type::Any,
                            kind: VarDeclKind::Const,
                        },
                        ta.ident.span,
                    );
                }
                ModuleItem::Decl(Decl::CInclude(ci)) => {
                    let _ = self.define(
                        "c".to_string(),
                        SymbolKind::Var {
                            ty: Type::Any,
                            kind: VarDeclKind::Const,
                        },
                        ci.span,
                    );
                }
                ModuleItem::Import(imp) => {
                    if let Some(def_spec) = &imp.default_specifier {
                        let _ = self.define(
                            def_spec.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Const,
                            },
                            def_spec.span,
                        );
                    }
                    for spec in &imp.specifiers {
                        let _ = self.define(
                            spec.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Const,
                            },
                            spec.span,
                        );
                    }
                }
                ModuleItem::Export(exp) => {
                    for spec in &exp.specifiers {
                        let _ = self.define(
                            spec.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Const,
                            },
                            spec.span,
                        );
                    }
                }
                ModuleItem::Stmt(Stmt::Var(v)) => {
                    if let Pattern::Ident(id) = &v.pat {
                        let _ = self.define(
                            id.sym.clone(),
                            SymbolKind::Var {
                                ty: v.ty.clone().unwrap_or(Type::Any),
                                kind: v.kind.clone(),
                            },
                            id.span,
                        );
                    }
                }
                _ => {}
            }
        }

        // Pass 2: Phase 2 Type-Directed Macro Expansion
        self.expand_macros(module)?;

        // Update self.classes, symbol_map, and scopes with classes that had methods added by macros
        for item in &module.body {
            if let ModuleItem::Decl(Decl::Class(c)) = item {
                self.classes.insert(c.ident.sym.clone(), c.clone());
                self.symbol_map
                    .insert(c.ident.span, SymbolKind::Class(c.clone()));
                if let Some(scope_entry) = self.scopes[0].get_mut(&c.ident.sym) {
                    *scope_entry = SymbolKind::Class(c.clone());
                }
            }
        }

        // Pass 3: Full Semantic Analysis & Type Checking
        for item in &module.body {
            if let ModuleItem::Decl(Decl::TypeAlias(ta)) = item {
                self.type_aliases
                    .insert(ta.ident.sym.clone(), ta.target_ty.clone());
            }
        }
        for item in &module.body {
            if let ModuleItem::Stmt(stmt) = item {
                self.extract_zig_code_symbols(stmt);
            }
        }

        for item in &module.body {
            let res = match item {
                ModuleItem::Decl(decl) => self.analyze_decl(decl),
                ModuleItem::Stmt(stmt) => self.analyze_stmt(stmt),
                ModuleItem::Import(_) => Ok(()),
                ModuleItem::Export(_) => Ok(()),
            };
            if let Err(e) = res {
                self.errors.push(e);
            }
        }
        if let Some(first) = self.errors.first().cloned() {
            return Err(first);
        }
        Ok(())
    }

}

pub(super) fn inner_span(expr: &Expr) -> Span {
    match expr {
        Expr::Ident(i) => i.span,
        Expr::Lit(_) => Span { start: 0, end: 0 },
        Expr::Bin(b) => b.span,
        Expr::ReflectType { span, .. } | Expr::ReflectOf { span, .. } => *span,
        Expr::PipePlaceholder(span) => *span,
        Expr::Range { span, .. } => *span,
        Expr::Template(t) => t.span,
        _ => Span { start: 0, end: 0 },
    }
}
