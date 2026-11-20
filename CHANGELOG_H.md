## 2026-10-06

- **Doc**: Refactored `cp_ref.md` to incorporate recently developed CP features, including Move semantics, Context Lifting (CPS Await), OOP destructors (`init`/`release`), automatic `defer`, cross-thread workers, and ES6 template strings.
- **Doc**: Added future architecture proposals (Typed Mailboxes, Pattern Matching Guards, Pipeline Operator, Comptime Macros) to `cp_ref.md`.

## 2026-10-06 (Re-Execution)

- **Doc**: Safely appended recent CP language implementations (Move semantics, CPS Context Lifting, RAII with auto-defer, OOP init/release) to `cp_ref.md` using compliant file editing tools.
- **Doc**: Appended architectural evolution proposals (Typed Mailboxes, Guard Clauses, Pipeline Operator, Comptime Macros) to `cp_ref.md` without destructive file operations.

## 2026-10-06 晚间 - 清理根目录残留实验代码

- **删除**: 删除了根目录下约 54 个 `test_*.zig` 实验性草稿文件（如 `test_linux_accept.zig`, `test_channel.zig` 等）。
- **删除**: 删除了上述 `.zig` 文件对应的同名无后缀 ELF 编译二进制可执行文件。
- **删除**: 删除了残留的测试日志与输出文件 `test_data.txt` 和 `test_io.txt`。
- **删除**: 删除了废弃的历史工作区 `test_project` 目录（其源文件早已迁入 `cap/test/`）。
- 2026-10-06: Added `RegExp` class and ES6 regular expression methods (`.test`, `.match`, `.exec`, `.replace`). Fixed and implemented all String methods (`indexOf`, `slice`, `toUpperCase`, etc) using Zig native functions. Modified `cap/src/main.zig` to automatically link libc (`-lc`) for POSIX regex support.
- 2026-10-06: Fully encapsulated and implemented the global `Math` object, mapping it to Zig's `std.math` functions. Fixed CP Parser to correctly handle `FloatLiteral` inside expressions. Fixed `cp_eq` operator to properly evaluate float equality. Added `cap/test/test_math.cp`.
- 2026-10-06: Implemented `Map` and `Set` advanced data structures. Extended the `Message` union to include `boolean`. Integrated custom hash contexts in Zig to dynamically hash and equate `Message` types allowing mapping of strings, floats, ints, and bools seamlessly. Fixed a critical pointer stability panic in `Expr::New` object allocation where memory was erroneously reset to `undefined` after proper initialization.

- 2026-10-11:
  - **cpc/src/lexer.rs, ast.rs, parser.rs, sema.rs**: Added `mut` and `move` keywords and tokens, removed `let mut`. Added Semantic errors for borrowing and moving conflicts.
  - **cpc/src/codegen.rs**: Enhanced State Machine generation to track variable ownership (Move semantics). Injected `__var_moved` flags and `defer release()` for Automatic Resource Management (Auto-Drop).
  - **cap/test/test_ownership_pass.cp**: Added positive test case for RAII / Auto-Drop execution.
  - **cap/test/test_ownership_fail.cp**: Added negative test case ensuring Use-After-Move triggers a compile-time panic.
  - **cp_ref.md**: Updated memory safety specification guidelines.

## 2026-10-16 - 零摩擦生命周期与栈绑定借用视图 (`ref struct`)

- **修改**: `cp_ref.md` — 完善第十章内存安全模型，新增零摩擦生命周期（NLL、三级函数返回生命周期推导、`&param` 绑定）、`Shared<T>` 与 C# 风格 `ref struct`（栈绑定借用视图）规范。
- **修改**: `cpc/src/lexer.rs` — 新增 `Token::Ref` (`ref`) 与 `Token::Struct` (`struct`) 词法标记。
- **修改**: `cpc/src/ast.rs` — `Class` 新增 `is_ref: bool` 标记；`Expr` 新增 `Borrow(Box<Expr>, bool)`；`Type` 新增 `BorrowType { inner, is_mut, lifetimes }` 与 `Shared(Box<Type>)`。
- **修改**: `cpc/src/parser.rs` — 支持 `ref struct` / `ref class` 解析（及字段含借用时自动推导为 `is_ref`），支持 `->` 与 `:` 返回类型，支持 `&expr` / `&mut expr` 表达式及 `&x T` / `&(x | y) T` 参数生命周期类型标注，支持未初始化变量声明 `let r: &T;`。
- **修改**: `cpc/src/ast_visitor.rs`, `cpc/src/dcp.rs`, `cpc/src/sema.rs` — 适配 `Expr::Borrow`、`Type::BorrowType`、`Type::Shared` 及 `Class::is_ref`。
- **修改**: `cpc/src/codegen.rs` — 实装 NLL 非词法生命周期预扫描 (`compute_last_uses_block`)、作用域深度逃逸检查 (`var_scope_depth`)、三级函数签名生命周期溯源 (`extract_expr_borrow_info`)、以及 `ref struct` 跨作用域/跨函数/跨 `@spawn` 逃逸静态拦截。
- **新增**: `cap/test/test_lifetime_pass.cp` — 生命周期、NLL、`&x T` 绑定与 `ref struct` 综合正向测试。
- **新增**: `cap/test/test_lifetime_fail_dangling.cp` — 内部短生命周期变量借用逃逸至外部变量报错测试。
- **新增**: `cap/test/test_lifetime_fail_nll.cp` — 借用尚未结束（后续仍有读取）时提前 Move 报错测试。
- **新增**: `cap/test/test_ref_struct_fail_escape.cp` — 栈绑定 `ref struct` 试图通过 `return` 逃逸出函数报错测试。

---

## 2026-10-18 (Trait、单继承 Class 与 `_`/`this` 实例所有权、借用及生命周期)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 1 节 `match` 模式匹配通配符为 `*`，避免与自身实例别名 `_` 冲突；
  - 更新第 2 节 `trait` 与 `class` 规范，明确支持单继承 `extends` 与 `trait` 默认方法实现；
  - 新增第 10.5 节《`trait`、单继承 `class` 与自身实例 `_` / `this` 的所有权、借用与生命周期》，完整定义 `_`/`this` 同义符号、省略 `_.` 前缀规则、首参缺省只读借用与 `mut _` / `move _` 接收者标注、方法返回借用的 Tier 1 生命周期自动穿透，以及 `class extends` 与 `trait` 静态多态的借用安全契约。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 新增 `Token::Extends` (`extends`) 与 `Token::Super` (`super`) 词法标记。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `TraitMethod` 新增 `body: Option<BlockStmt>` 以支持 `trait` 默认方法实现；
  - `ClassDecl` 新增 `super_class: Option<Ident>` 以支持 `class Child extends Parent` 单继承。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_params` 支持成员方法首参 `_`、`mut _`、`move _`（以及 `this`、`mut this`、`move this`）无类型简写解析；
  - `parse_trait` 支持解析带函数体的默认方法与不带函数体的契约签名；
  - `parse_class` 支持解析 `extends Parent` 单继承子句与 `implements Trait1, Trait2`；
  - `parse_pattern` 将模式匹配通配符切换为 `*` (`Token::Star`)，使 `_` 在 `match` 分支内依然指向自身实例 `this`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 完善 `TraitDecl`、`ClassDecl`、`ImplDecl` 语义遍历，注册 `this`、`_`、`super` 隐式实例上下文并跳过接收者首参的类型计数偏移。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 新增 `ReceiverMode` (`ReadOnly`、`Mut`、`Move`、`Static`) 推导与检查体系；
  - 实现 `resolve_class_hierarchy_and_traits` 预处理：单继承 `extends` 字段与方法展开、父类被重写方法生成 `__super_xxx`、子类重写接收者权限一致性检查、`trait` 默认方法自动注入，以及 `implements` / `impl Trait for Type` 契约完整性与接收者模式匹配校验；
  - 实现成员函数内 `_` / `this` 到 Zig `self` 的映射、同类字段与兄弟成员方法省略 `_.` 前缀自动解析，以及 `super.method()` 调用生成；
  - 实现只读方法（缺省 `_`）内禁止修改字段或调用 `mut _` / `move _` 方法的编译期拦截；
  - 实现调用点 `obj.mutMethod()` 的 NLL 借用冲突检查与只读借用/只读参数越权调用拦截；
  - 实现调用点 `obj.moveMethod()` 与构造器 `move` 参数的所有权转移追踪（自动标记 `__obj_moved = true` 并在后续使用时报错 `Use of moved value`）；
  - 实现成员方法返回 `&T` 或 `ref struct` 时自动绑定接收者生命周期（Tier 1 零标注穿透）并在调用点接入 NLL 借用冲突检查。

### 新增文件 (Added)

- [`cap/test/test_trait_class_pass.cp`](file:///data/dev/cp/cap/test/test_trait_class_pass.cp): `trait`、单继承 `class extends`、`_` 接收者、省略 `_.` 互调、`*` 通配符、`mut _` / `move _` 接收者与方法返回借用 NLL 正向综合测试。
- [`cap/test/test_trait_class_fail_readonly_mutate.cp`](file:///data/dev/cp/cap/test/test_trait_class_fail_readonly_mutate.cp): 缺省只读方法内修改 `_.val` 编译期拦截反向测试。
- [`cap/test/test_trait_class_fail_nll_conflict.cp`](file:///data/dev/cp/cap/test/test_trait_class_fail_nll_conflict.cp): 方法返回 `&Tensor` 借用存活期间对宿主对象调用 `mut _` 方法的 NLL 借用冲突拦截反向测试。
- [`cap/test/test_trait_class_fail_use_after_move_self.cp`](file:///data/dev/cp/cap/test/test_trait_class_fail_use_after_move_self.cp): 调用 `move _` 方法吞噬实例后再次使用原实例的编译期拦截反向测试。
- [`cap/test/test_trait_class_fail_trait_mut.cp`](file:///data/dev/cp/cap/test/test_trait_class_fail_trait_mut.cp): 通过只读 `Trait` 参数越权调用 `mut _` 成员方法的编译期拦截反向测试。

---

## 2026-10-18 (Part 2: 函数三态返回类型 `?T ! E`、错误冒泡、`? default`、`try/catch`、可选链、`Box<dyn Trait>` 胖指针与 `switch` 模式匹配)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 1 节模式匹配关键字为 `switch`，通配符为 `*`；
  - 新增第 11 节《函数返回类型（三态模型）、错误冒泡与捕获、可选链、`Box<dyn Trait>` 胖指针与 `switch` 模式匹配》，完整记录三态返回类型、`?` / `? default` / `? return default` / `??` 运算符分工、`try { ... } catch` 自动冒泡块、声明式 `from` 错误转换、内置 `Error` 结构体、`Box<dyn Trait>` 胖指针与 `downcast<T>()` 规范及安全边界。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 新增 `Token::Switch` (`switch`)、`Token::Try` (`try`)、`Token::Catch` (`catch`)、`Token::Null` (`null`)、`Token::Dyn` (`dyn`)、`Token::QuestionQuestion` (`??`)、`Token::QuestionDot` (`?.`)、`Token::DotDotEq` (`..=`)。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 新增 `ErrorType` (`Named`, `Union`, `AnonSet`, `BoxDyn`, `DefaultError`)；
  - `Type` 新增 `ResultType { ok: Box<Type>, err: ErrorType }`、`BoxDyn(Ident)`、`BareDyn(Ident)`；
  - `EnumVariant` 新增 `tuple_type: Option<Type>` 与 `from_err: Option<Ident>`（支持 `Io(FileErr) from FileErr`）；
  - `Stmt` 新增 `TryCatch { span, try_block, catch_param, catch_body }`；
  - `Pattern` 新增 `Null`, `Err(Box<Pattern>)`, `Range { start, end, inclusive }`, `Or(Vec<Pattern>)`；`EnumPattern` 新增 `tuple_pats`；`MatchArm` 新增 `guard: Option<Expr>`；
  - `Expr` 新增 `OptMember`, `OptCall`, `QuestionFallback { expr, fallback, is_return_fallback }`, `NullCoalesce`, `Catch`, `ErrWrap`, `BoxNew`, `Downcast`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 适配所有新增 `Stmt` 与 `Expr` 变体的闭包变量引用与声明扫描。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 支持解析 `: ?T ! E`、`: !E`、`E1 | E2`、`!{ a, b }`、`Box<dyn Trait>`、`dyn Trait`、`?&T` / `?&mut T`；
  - 支持枚举变体 `Variant(Payload) from SourceErr` 声明解析；
  - 支持 `try { ... } catch err => { ... }` 语句块解析；
  - 支持 `switch` 表达式与模式解析（`Err(...)`, `null`, `*`, `..=`, `|`, `if` 守卫，并通过 `in_match_guard` 消解 `=>` 与箭头函数的词法歧义）；
  - 支持 `?.` 可选链、`? default` / `? return default`、`??`、`catch err => ...`、`Err(...)`、`Error { ... }`、`Box.new(...)`、`.downcast<T>()` / `.downcast_mut<T>()` 解析。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 注册内置 `Error` 与 `Box` 类型，适配所有新增 AST 节点的语义检查与 `.d.cp` 类型签名格式化。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 注入 `CpResult(T, E)`、内置 `Error`、`cp_convert_err`、`cp_has_ok_val`、`cp_unwrap_ok_val`、`cp_is_null_val`、`CpOptType` 等 Zig 0.17 运行时辅助原语；
  - 实现编译期裸 `dyn Trait` 栈分配拦截、匿名错误集 `__AnonErr_` 与合并错误集 `__UnionErr_` 自动合成、以及枚举 `from` 自动转换生成；
  - 实现 `Trait` 自动生成 `VTable`（含 `__type_id` 与 `_drop`）、`Box<dyn Trait>` 胖指针结构体、类编译期 64 位 FNV-1a `__TYPE_ID`、`downcast<T>()` 运行时校验与自动 Drop 释放；
  - 实现 `infer_cp_type` 完整类型推导、`check_switch_exhaustiveness` 编译期穷尽检查、`generate_switch` 模式匹配生成、`try { ... } catch` 区域自动冒泡包裹，以及 `?` / `? default` / `? return default` / `??` / `catch` / `?.` 代码生成。

### 新增文件 (Added)

- [`cap/test/test_error_box_switch_pass.cp`](file:///data/dev/cp/cap/test/test_error_box_switch_pass.cp): 三态返回类型、`?` 冒泡、`? default`、`? return default`、`??`、`catch`、`try/catch` 自动冒泡块、匿名错误集、声明式 `from`、统一 `Error`、`?.` 可选链、`Box<dyn Trait>` 胖指针与 `downcast<T>()`、`switch` 模式匹配全特性正向测试。
- [`cap/test/test_error_fail_null_coalesce_on_err.cp`](file:///data/dev/cp/cap/test/test_error_fail_null_coalesce_on_err.cp): 对未处理错误的 `T ! E` 直接使用 `??` 的编译期报错反向测试。
- [`cap/test/test_error_fail_non_exhaustive_switch.cp`](file:///data/dev/cp/cap/test/test_error_fail_non_exhaustive_switch.cp): `switch` 非穷尽匹配（缺失 `null` 与错误变体分支）的编译期报错反向测试。
- [`cap/test/test_box_fail_bare_dyn.cp`](file:///data/dev/cp/cap/test/test_box_fail_bare_dyn.cp): 裸 `dyn Trait` 置于栈上的编译期报错反向测试。
- [`cap/test/test_error_fail_incompatible_from.cp`](file:///data/dev/cp/cap/test/test_error_fail_incompatible_from.cp): 跨异构错误集冒泡未声明 `from` 转换时的编译期报错反向测试。
- [`cap/test/example_showcase.cp`](file:///data/dev/cp/cap/test/example_showcase.cp): CP 语言全特性综合示例程序，覆盖三态返回 `?T ! E`、直接返回、`null`、`Err(.Variant)`、`? default`、`? return default`、`catch` + `??`、匿名错误集、`?.` 可选链、`Box<dyn Trait>` 胖指针虚表与自动 Drop、`downcast<T>()`、声明式 `from` 错误提升、`try/catch` 自动冒泡区与 `switch` 高级模式匹配（范围、守卫、或模式）。

## [2026-10-18] 错误兜底语法精炼 (Error Fallback Refinement)

### 修改文件 (Modified)

- [`cap/test/example_showcase.cp`](file:///data/dev/cp/cap/test/example_showcase.cp):
  - 简化 `safeGetConfig` 中的错误兜底为纯粹的 `let cfg = readConfigFile(filename) ? "DEFAULT_CONFIG";`，省略冗余的 `return` 关键字，确保变量初始化与表达式级降级语义保持严格一致。

## [2026-10-19] 静态匿名对象与动态字典 (Static Anonymous Objects & Dynamic Dict)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 新增 `Token::DotDotDot` (`...`) 与 `Token::HashLBrace` (`#{`)。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `ObjectField` 结构体重构：增加 `is_spread: bool` 与 `key: Option<Ident>`，同时支持 `...spread` 与常规 `key: val` / 简写 `{ key }`；
  - `Expr` 枚举新增 `Dict(Vec<ObjectField>)` 动态字典表达式与 `Index { obj: Box<Expr>, index: Box<Expr> }` 索引访问表达式；
  - `Type` 枚举新增 `Dict(Box<Type>, Box<Type>)` 与 `Set(Box<Type>)`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 增加对 `Expr::Dict` 与 `Expr::Index` 节点的作用域与变量引用扫描访问支持。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 增加 `Type::Dict` 与 `Type::Set` 的类型声明格式化输出支持。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 支持解析 `Dict<K, V>` 与 `Set<T>` 类型注解；
  - 重构 `parse_object_fields`，支持 `...spread` 展开、`key: val` 及 `{ key }` 简写；
  - 将 `{ ... }` 识别为静态匿名对象 `Expr::Object`，将 `#{ ... }` 识别为动态字典 `Expr::Dict`；
  - 将中括号下标访问 `obj[idx]` 统一解析为 `Expr::Index`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - `infer_type` 支持 `Expr::Object` 字段展开与覆盖合并、`Expr::Dict`（推导为动态类型 `Type::Dict(String, Any)`）以及 `Expr::Index`；
  - 完善 `Pattern::Object` 解构赋值类型推导。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 注册 `Dict` 全局标识符；重构 Zig 运行时 `Map` 与 `Set` 底层；
  - 废除未命中时的 undefined UB，`get()` 安全返回 `?Message`，并提供 `keys()`, `values()`, `entries()`, `put_all()` 完整 ES6 集合方法；
  - 定义 `const Dict = Map;`，为 `Map` / `Set` / `Dict` 注入 `pub fn release(self: *Self) void` 实现作用域退出自动析构；
  - 注入 `CpIndexReturnType`、`cp_index_get` 与 `cp_index_assign`，实现透明的容器索引读取与赋值；
  - 注入 `CpNullCoalesceReturn` 与 `cp_null_coalesce`，优雅处理 `dict["key"] ?? fallback` 的跨类型空值合并；
  - 升级 `cp_eq` 自动解包 `?Message` 并支持多基本类型比较；
  - 增强 `infer_cp_type` 与 `infer_zig_type` 支持布尔字面量标识符 `true` / `false`、静态匿名对象展开与属性覆盖，并在 Zig 端生成零开销原生匿名结构体 `.{ .a = ... }`；
  - 支持 CPS 状态机与常规函数中的对象解构绑定，自动从右侧表达式推导字段类型；
  - 优化 `@print` 单参数安全打印，防止格式化注入错误。

### 新增文件 (Added)

- [`cap/test/test_static_object.cp`](file:///data/dev/cp/cap/test/test_static_object.cp): 静态匿名对象字面量、`.` 属性访问、解构赋值与 `...` 展开覆盖测试。
- [`cap/test/test_map_set_dict.cp`](file:///data/dev/cp/cap/test/test_map_set_dict.cp): `Map` 与 `Set` 完整 ES6 接口、`#{ ... }` 动态字典、下标索引读写、字典展开及 `??` 空值合并测试。

---

## [2026-10-21] 阶段一：MetaAnnotation 极简元注解系统 (Phase 1: MetaAnnotation Core System)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 新增 `Token::Meta` (`meta` 关键字)。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 新增 `MetaDef`（元注解定义节点，含无参及参数列表）、`MetaParam`、`MetaItem` 与 `MetaArg`（支持位置参数与具名参数）；
  - `Decl` 枚举新增 `Decl::Meta(MetaDef)` 变体；
  - 为 `Class`、`Function`、`Field`、`Param`、`TraitDecl`、`EnumDecl` 统一注入 `#[serde(default)] pub meta: Vec<MetaItem>` 字段。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 支持解析顶层 `meta Name;`（无参免写空括号）与 `meta Name(params);`（支持默认值）；
  - 实现 `parse_meta_list` 与 `parse_meta_item`，支持解析 `@Ident` 与 `@Ident(args)`，支持参数省略键名、位置传参和具名传参；
  - 实现 `clone_lookahead` 与 `is_decl_keyword` 预测分析，精准消除顶层 `@builtin(...)` 表达式语句与元注解 `@Meta` 的文法歧义；
  - 在类成员（方法、字段）和函数参数（`parse_params`）中支持前置 `@meta` 解析并挂载至 AST。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::MetaInvalidTarget`、`SemaError::MetaDuplicateOnce`、`SemaError::MetaArgNotConstant` 语义诊断错误；
  - 新增 `MetaDefInfo` 结构并在 `Analyzer::new` 预置内置元注解 `@target`、`@once`、`@reflect`；
  - 在第一遍 AST 扫描中自动收集用户定义的 `meta` 并解析提取 `@target(class, method, field, param)` 允许目标与 `@once` 唯一性约束（未声明目标时默认全通）；
  - 在声明与成员检查阶段，严密校验：
    1. 元注解参数必须为编译期常量字面量（`Expr::Lit`、常量标识符、字面量数组、字面量匿名对象）；
    2. 校验元注解应用目标（如 `@target(method)` 挂在类上精准报错拦截）；
    3. 校验 `@once` 唯一性，同名多次书写直接报错拦截；
    4. 允许多次书写同名注解，编译器天生支持自动聚合为数组，废除繁琐的 `@Repeatable`；
    5. 支持脚本及快速原型免预定义直接打注解（双轨制）。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 增加对 `Decl::Meta` 的 `.d.cp` 类型定义输出支持。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 适配 `Decl::Meta`，阶段一保持 100% 零运行时代码输出、零开销、零体积膨胀。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十二章《MetaAnnotation 极简元注解系统规范》，完整记录 `meta` 关键字、`@target`、`@once`、`@reflect`、三免法则与多值聚合规范。

### 新增文件 (Added)

- [`cap/test/test_meta_pass.cp`](file:///data/dev/cp/cap/test/test_meta_pass.cp): 元注解 `meta` 声明、`@target` 目标约束、`@reflect`、`@once`、无参免 `()`、单参免键名、同名注解天然多重收集为数组及 Ad-hoc 免声明即用正向综合测试。
- [`cap/test/test_meta_fail_target.cp`](file:///data/dev/cp/cap/test/test_meta_fail_target.cp): 将 `@target(method)` 元注解非法附着在类上触发编译期语义拦截反向测试。
- [`cap/test/test_meta_fail_once.cp`](file:///data/dev/cp/cap/test/test_meta_fail_once.cp): 将 `@once` 元注解重复多次标注触发编译期语义拦截反向测试。

## [Unreleased] - 2026-10-22

### 阶段二：Dual-State Reflection 双态反射模型规范与实现

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 在 `Expr` 枚举中新增 `ReflectType { span: Span, target_ty: Type }` 与 `ReflectOf { span: Span, expr: Box<Expr> }` 表达式节点。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在后缀成员解析中支持解析 `reflect.type<T>()` 与 `reflect.of(val)`；
  - 在 `parse_ident` 中加入 `Token::Meta => "meta".to_string()`，允许 `meta` 作为合法成员属性名（如 `info.meta`）。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 适配 `Expr::ReflectType` 与 `Expr::ReflectOf` 的变量引用收集。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::ReflectNotEnabled` 错误变体；
  - 针对 `reflect.type<T>()` 与 `reflect.of(val)` 执行编译期反射开关硬检验：若目标类/结构体未标记 `@reflect`，直接报错拦截；
  - 预置 `TypeInfo`、`FieldInfo`、`MethodInfo` 类型识别并将 `reflect` 设为合法内置标识符。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 Zig 运行时前导声明中引入 `FieldInfo`、`MethodInfo`、`TypeInfo`（含 `getField` 与 `getMethod` 方法）；
  - 升级 `Message` 与 `MessageContext` 支持 `.array: []Message`，支持下标直接读取多值注解数组；
  - 实现 `generate_reflection_tables`：为每个 `@reflect` 标记的类生成惰性分配的单例 `_get_typeinfo_<ClassName>()` 与 `_typeinfo_<ClassName>`；
  - 彻底复用 CP 原生动态字典 `Dict<string, any>`（`*Map`），同名多次出现的注解自动聚合为 `Message.array`，单值注解自动解构为标量；
  - 在 `generate_expr`、`infer_zig_type`、`infer_cp_type` 中打通 `Expr::ReflectType`、`Expr::ReflectOf` 及相关字段/方法类型推导。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十三章《Dual-State Reflection 双态反射模型规范》，详细记录反射入口、扁平反射模型、原生字典复用与零成本安全屏障。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 代码格式化适配。

### 新增文件 (Added)

- [`lib/cap/reflect.cp`](file:///data/dev/cp/lib/cap/reflect.cp): CP 标准库反射声明模块（`cap:reflect`）。
- [`cap/test/test_reflect_pass.cp`](file:///data/dev/cp/cap/test/test_reflect_pass.cp): 双态反射正向综合测试，验证 `reflect.type<T>()`、`reflect.of(val)`、`info.name`、`info.isRef`、`info.meta` 字典读取与存在性判断、`info.fields`、`info.methods`、同名注解数组收集、`getField` 与 `getMethod` 以及 `ref class` 反射。
- [`cap/test/test_reflect_fail_unmarked.cp`](file:///data/dev/cp/cap/test/test_reflect_fail_unmarked.cp): 未标记 `@reflect` 的类调用 `reflect.type<T>()` 触发编译期精准拦截反向测试。
- [`cap/test/test_reflect_fail_of_unmarked.cp`](file:///data/dev/cp/cap/test/test_reflect_fail_of_unmarked.cp): 未标记 `@reflect` 的实例调用 `reflect.of(val)` 触发编译期精准拦截反向测试。

## [Unreleased] - 2026-10-22

### 阶段三：极简装饰器系统与 AOP 静态合成规范与实现

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `Token::Dec` (`dec` 3 字母关键字)。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 在 `Decl` 中新增 `Decl::Dec(DecDef)` 变体；
  - 为 `Param` 新增可选参数默认值字段 `pub default: Option<Expr>`。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `is_decl_keyword` 与顶层解析放行 `Token::Dec`；
  - 实现 `parse_dec_def`，支持 `dec name(params) { body }`；
  - `parse_params` 支持 untyped `next` 参数（免写 `: any`）与参数默认值 `= expr` 解析；
  - `parse_primary_expr` 支持 `Token::Fn` 解析匿名函数表达式 `fn(params) { body }` 为 `Expr::Arrow`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::DecInvalidTarget` 错误变体；
  - 在 `Analyzer` 中增加 `dec_defs` 注册表，第一遍扫描记录所有 `dec` 声明并分配函数符号；
  - 严格校验装饰器挂载目标合法性（当前阶段支持方法与函数，修饰字段等非法目标时精准报错）；
  - 独立作用域分析 `dec` 内部逻辑与参数定义。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 实现 `generate_function_decorated` 与 `prepare_decorator_stmts`；
  - 建立编译期静态合成 AOP 机制：原逻辑保留为 `_raw_<name>`，逐层展开装饰器切面代码合成为 `_wrap_k_<name>` 与最终公开入口 `<name>`；
  - 实现洋葱执行模型（Onion Execution Model）：外层装饰器先进入、最后退出；
  - `next()` 调用自动透传目标函数的参数与返回值；支持直接带参风格与高阶闭包工厂风格参数绑定；
  - 在 `infer_zig_type` 与 `infer_cp_type` 中打通 `next()` 返回类型推导。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 增加对 `Decl::Dec` 的 `.d.cp` 类型声明输出支持。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十四章《Decorators 极简装饰器与 AOP 静态合成规范》，详细记录 `dec` 关键字、三免法则、洋葱模型、双轨参数风格与静态合成机制。

### 新增文件 (Added)

- [`cap/test/test_dec_pass.cp`](file:///data/dev/cp/cap/test/test_dec_pass.cp): 极简装饰器正向综合测试，验证无参直接装饰器 `@doubleResult`、直接带参装饰器 `@tag("SERVICE")`、高阶工厂装饰器 `@prefixFactory("GLOBAL")`、洋葱模型多装饰器堆叠 `@outerWrap` + `@innerWrap`、类方法修饰、顶层独立函数修饰及与 `@reflect` / `@Route` 元注解共存。
- [`cap/test/test_dec_fail_target.cp`](file:///data/dev/cp/cap/test/test_dec_fail_target.cp): 将方法装饰器非法附着在类字段上触发编译期语义拦截反向测试。

## [Unreleased] - 2026-10-23

### 阶段四：类型感知宏与循环安全系统 (Type-Directed Macro & Anti-Cycle System)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `Token::Macro` (`macro` 关键字)。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 在 `Decl` 中新增 `Decl::Macro(MacroDef)` 变体；
  - 新增 `MacroDef` 结构体，支持包含元注解、标识符、参数列表及展开函数体。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `is_decl_keyword` 与顶层声明解析放行 `Token::Macro`；
  - 实现 `parse_macro_def`，支持 `macro Name(params) { body }`；
  - `parse_params` 允许 untyped `target` 参数（无类型注解直通）。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 增加对 `Decl::Macro` 的 `.d.cp` 符号声明格式化支持。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 顶层跳过 `Decl::Macro` 代码生成（宏纯编译期展开，零运行时二进制开销）；
  - 新增 `escape_zig_string` 转义工具函数，精准处理字符串字面量中既有转义与宏合成双引号；
  - 修复方法内本地变量/参数遮蔽（Shadowing）类字段的符号优先级问题；
  - 升级 `cp_add` 格式化支持 `{s}{any}` / `{any}{s}`，无缝支持字符串与多类型数据高效拼接。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `MacroCycleDetected`、`MacroRecursionLimitExceeded`、`MacroInvalidTarget`、`MacroProhibitedTypeCreation`、`MacroHygieneViolation` 语义错误变体；
  - 构建三阶段编译管线：Pass 1 顶层符号扫描 -> Pass 2 Phase 2 宏代码派生展开 (`expand_macros`) -> Pass 3 完整语义检查；
  - 维护 `macro_call_stack`，提供 128 层递归熔断与递归展开环路探测；
  - 严格校验宏挂载目标（仅允许挂载于 `class` 或 `struct`）；
  - 实装 `@Json` 强类型序列化派生宏（合成 `toJson(): string` 方法）、`@Table` ORM 映射派生宏（合成 `getTableName(): string` 方法）、`@ToString` 结构打印派生宏以及用户自定义宏派生方法；
  - 严守 Macro Hygiene 铁律，强校验派生产物严禁携带任何宏注解。
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs):
  - `analyze_module` 适配 `&mut Module` 签名，确保宏展开修改 AST 直接回传 codegen。
- [`cpc/src/lsp.rs`](file:///data/dev/cp/cpc/src/lsp.rs):
  - `analyze_module` 适配 `&mut Module` 签名。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十五章《类型感知宏与循环安全系统》，详细规范宏语法、内建派生宏、四大防死锁循环安全铁律。

### 新增文件 (Added)

- [`cap/test/test_macro_pass.cp`](file:///data/dev/cp/cap/test/test_macro_pass.cp): 类型感知宏与循环安全正向综合测试，验证 `@Json` 序列化派生、`@Table("t_users")` 表名映射、无参 `@Table` 默认类名映射、`@ToString` 结构打印、用户自定义 `macro Tag` 与 `macro Environment` 派生方法、以及互引类型（Tier 1 TypeDeclInfo）安全内省实测。
- [`cap/test/test_macro_fail_target.cp`](file:///data/dev/cp/cap/test/test_macro_fail_target.cp): 将类宏 `@Json` 非法附着在类方法上触发编译期语义拦截反向测试。

## [Unreleased] - 2026-10-24

### 管道操作符与主题占位符系统 (Pipeline Operator `|>` & Topic Placeholder `.`)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `Token::Pipeline` (`|>` 操作符)，排在 `Token::Pipe` (`|`) 之前以优先匹配长符号。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 在 `BinaryOp` 中新增 `BinaryOp::Pipe`；
  - 在 `Expr` 中新增 `Expr::PipePlaceholder(Span)` 变体；
  - 实现 `has_pipe_placeholder`、`substitute_pipe_placeholder` 与 `transform_pipeline_rhs` 工具函数，完备支持三级降糖策略（规则 1 裸标识符、规则 2 默认首参、规则 3 显式占位符 `.`、方法调用与多字段计算）。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `Parser` 中增加 `in_pipeline_rhs` 状态栈追踪；
  - 设定 `|>` 优先级为 3（高于比较操作符 2，低于加减法 4）；
  - `parse_bin_expr` 在解析管道右侧表达式时维护 `in_pipeline_rhs` 计数；
  - `parse_primary_expr` 区分 `.` 的上下文语义：管道右侧识别为 `Expr::PipePlaceholder` 或前置成员访问，非管道右侧拦截非法使用并报错 `Pipeline placeholder '.' can only be used on the right-hand side of a pipeline ('|>') operator`；
  - `ParseError` 新增 `Generic` 变体以规范化清晰的错误诊断。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - `ASTVisitor` 适配 `Expr::PipePlaceholder`，防止未绑定变量误报。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `PipePlaceholderOutsidePipeline` 错误；
  - 适配 `BinaryOp::Pipe` 与 `Expr::PipePlaceholder` 的类型检查与类型推断。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 升级 `Expr::Ident` 生成逻辑，识别 SSA 局部管道临时变量（`__pipe_` 前缀）直通输出；
  - 实现 `generate_pipeline`，将 `left |> right` 转换为 Zig 单次求值标签块 `(blk_p_N: { const __pipe_N = <left>; break :blk_p_N <right>; })`，彻底杜绝双重求值副作用；
  - 实现 `infer_cp_type_with_mock`，建立管道推断上下文环境，精准推导流水线各阶段以及对象方法/字段访问的返回类型，消除 Zig `*anyopaque` 类型失配。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十六章《管道操作符与主题占位符》，规范操作符优先级、3-Tier 降糖规则、单次求值保证与 AI Agent 数据流工程范式。

### 新增文件 (Added)

- [`cap/test/test_pipe_pass.cp`](file:///data/dev/cp/cap/test/test_pipe_pass.cp): 管道与占位符完整正向综合功能测试，覆盖裸函数、默认首参、显式占位符槽位、对象成员方法流、字段访问算术、多行链式流水线、副作用单次求值（`get_seed() |> combine(., .)` 执行次数严格为 1）以及 AI Agent RAG 认知流水线实测。
- [`cap/test/test_pipe_fail_placeholder.cp`](file:///data/dev/cp/cap/test/test_pipe_fail_placeholder.cp): 在管道操作符外部非法使用主题占位符 `.` 的编译期语义拦截反向测试。

## [Unreleased] - 2026-10-25

### Unsafe 与裸指针系统规范与实现 (Unsafe & Raw Pointer System: *T!, **T!, unsafe fn, unsafe {}, Deref & Cast)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `Token::Unsafe` (`unsafe` 关键字)。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 在 `Type` 中新增 `Type::RawPtr { inner: Box<Type>, depth: usize }` 变体，支持多级非托管裸指针；
  - 在 `Expr` 中新增 `Expr::Cast { span, expr, target_ty }`、`Expr::Deref(Box<Expr>)` 与 `Expr::Unsafe(BlockStmt)` 变体；
  - 在 `Stmt` 中新增 `Stmt::UnsafeBlock(BlockStmt)` 变体；
  - 在 `Function` 与 `MethodSig` 中增加 `is_unsafe: bool` 标识。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 适配 `Expr::Cast`、`Expr::Deref`、`Expr::Unsafe` 与 `Stmt::UnsafeBlock` 的遍历与变量引用收集。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 新增双 Token 前瞻缓冲区 `peeked2` 与 `peek_next`；
  - 实现裸指针类型解析（`*T!`、`**T!`、`***T!`），利用双向 lookahead 彻底消除与返回类型错误联合分隔符 `!` 的语法混淆；
  - 提取独立前缀一元/后缀解析器 `parse_postfix_expr` 与 `parse_postfix_from_left`，确保 `&user as *User!` 严谨按照 `(&user) as *User!` 进行优先级绑定，避免一元借用贪婪吞入类型转换；
  - 实现 `unsafe fn` 函数声明与 `unsafe { ... }` 表达式/语句块解析；
  - 实现显式类型强转语法 `expr as TargetType`；
  - 实现标量与多级解引用语法 `p.*` 与 `pp.*.field`；
  - 支持大写值类型结构体字面量实例化 `User{age: 20}` 并将 `struct` 正确声明为 `is_ref = true`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::UnsafeRequired(String, Span)` 编译期安全错误；
  - 在 `SymbolKind::Func` 中记录 `is_unsafe: bool`；
  - 引入 `unsafe_depth` 与 `current_fn_is_unsafe` 上下文追踪；
  - 编译期严格检查并硬性拦截在 unsafe 外部调用 unsafe 函数、强转裸指针或解引用裸指针的操作；
  - 增强 `infer_type` 对 `Expr::Cast`、`Expr::Deref`、`Expr::Unsafe` 的类型推导。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 `map_type` 中正确将 `self.ref_classes`（包括 `struct`）映射为值类型，多级裸指针 `Type::RawPtr` 映射为 Zig 裸指针（`*T`、`**T`）；
  - 实现 `Expr::Cast` 映射为 Zig `@as(Target, @ptrCast(@alignCast(&(...))))`；
  - 实现 `Expr::Deref` 映射为 Zig 原生解引用 `p.*`；
  - 修复状态机局部变量收集（`extract_locals_simple` 与 `extract_local_vars_to_str`），支持深入 `Stmt::UnsafeBlock` 发现变量并正确注入 `cp_main_Context`；
  - 修复函数参数字段赋值检查，允许对裸指针和可变借用参数的目标字段（pointee field）进行赋值修改。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs) 与 [`cpc/src/lsp.rs`](file:///data/dev/cp/cpc/src/lsp.rs):
  - 适配 `Type::RawPtr` 穷尽模式匹配与补全。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十八章《Unsafe 与裸指针系统规范》，完整记录指针语法、解引用规则、与错误联合解耦、显式强转、安全拦截与生产级代码示例。

### 新增文件 (Added)

- [`cap/test/test_unsafe_pass.cp`](file:///data/dev/cp/cap/test/test_unsafe_pass.cp): 涵盖一级裸指针参数修改、二级裸指针解引用修改、标量裸指针修改、三态错误联合共存以及 switch 模式匹配的全量正向功能测试。
- [`cap/test/test_unsafe_fail_call.cp`](file:///data/dev/cp/cap/test/test_unsafe_fail_call.cp): 验证在 unsafe 块外部调用 `unsafe fn` 被编译期硬性拦截并报错的反向测试。
- [`cap/test/test_unsafe_fail_cast.cp`](file:///data/dev/cp/cap/test/test_unsafe_fail_cast.cp): 验证在 unsafe 块外部执行 `&x as *T!` 强转被编译期硬性拦截并报错的反向测试。
- [`cap/test/test_unsafe_fail_deref.cp`](file:///data/dev/cp/cap/test/test_unsafe_fail_deref.cp): 验证在 unsafe 块外部执行裸指针解引用 `p.*` 被编译期硬性拦截并报错的反向测试。

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `Token::Of` (`of`) 与 `Token::In` (`in`) 关键字。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 新增 `IterMode` 枚举 (`Borrow`, `MutBorrow`, `Move`)；
  - 新增 `ForStmt` 抽象语法树节点 (`span`, `mode`, `pat`, `expr`, `body`) 与 `Stmt::For(ForStmt)`；
  - 新增 `Expr::Range { span, start, end, inclusive }` 表达式节点；
  - 适配管道占位符检测与替换逻辑。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 实现 `parse_for_stmt`，支持三维借用修饰符 (`for x`, `for &x`, `for move x`)、可选声明关键字 (`let`/`const`)、可选括号风格 (`for (...)`) 以及区间范围语法 (`0..10` / `0..=10`)；
  - 在 `parse_ident` 中加入 `Token::Of` 与 `Token::In` 上下文关键字支持，保证 `reflect.of` 等已有代码不发生词法冲突。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 适配 `Expr::Range` 与 `Stmt::For` 的变量引用收集与局部变量声明提取。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::MutateWhileBorrowedInFor` 错误变体；
  - 引入 `loop_borrowed_vars` 集合并在循环体内对被迭代集合施加只读借用锁，拦截任何 `push`、`clear`、`delete`、重赋值等结构性改变；
  - 针对 `IterMode::Move` 将集合登记至 `moved_vars`，禁止循环后再次使用（拦截 Use-After-Move）；
  - 适配模式解构作用域绑定与 `Expr::Range` 语义检查。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 Zig 运行时前导声明中注入 `cp_iter_slice` 与 `cp_mut_slice`（带 `@constCast` 支持），无缝支持切片、静态数组及不可变数组的原地安全可变切片迭代；
  - 统一区间循环为独立局部块 `{ var i: i32 = start; while (i < end) : (i += 1) { ... } }`，彻底杜绝多次区间循环的局部变量命名重定义；
  - 优化循环变量为块级原生捕获 `for (target) |item|` 与 `for (target) |*item|`，避免状态机 `ctx` 上下文全局命名冲突；
  - 引入 `mut_borrow_loop_vars` 作用域集合，在表达式和赋值中自动生成解引用操作（`x.* = ...` 与 `(x.*)`），支持标量及结构体的就地原地修改；
  - 在 `Expr::Array` 中统一输出为 `&.{ ... }`，完美兼顾静态元组解构访问与切片类型自动转换。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十七章《迭代系统》，详尽记录三维借用语义表、语法包容性、区间范围、模式解构与编译期安全铁律。

### 新增文件 (Added)

- [`cap/test/test_for_pass.cp`](file:///data/dev/cp/cap/test/test_for_pass.cp): 涵盖只读借用 `for x of list`、原地可变借用 `for &x of list`、所有权转移 `for move x of list`、半开区间 `0..5`、闭区间 `1..=5`、动态变量区间、对象模式解构以及可选小括号与 `let` 语法糖的正向综合功能测试。
- [`cap/test/test_for_fail_mutate.cp`](file:///data/dev/cp/cap/test/test_for_fail_mutate.cp): 在 `for x of list` 只读借用循环体内试图调用 `list.push(...)` 触发 `MutateWhileBorrowedInFor` 语义拦截反向测试。
- [`cap/test/test_for_fail_use_after_move.cp`](file:///data/dev/cp/cap/test/test_for_fail_use_after_move.cp): 在 `for move x of list` 所有权转移后试图再次读取 `list` 触发 `UseOfMovedVar` 语义拦截反向测试。

---

## 2026-10-25 (语言设计规范 `cp_ref.md` 全量同步与补全)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **修正第 3 节对象模型描述**：消除旧文档“不支持动态增删属性”的表述矛盾，重构为显式的“静态匿名对象 `{ ... }`（编译期零成本）与动态字典 `#{ ... }`（运行时动态哈希）”双轨分立机制。
  - **新增第十九章《静态匿名对象与动态字典系统》**：完整定义静态匿名对象 `{ ... }` 的编译期强类型推导、模式解构、展开与覆盖语法；完整定义动态字典 `#{ ... }` / `Dict`、运行时下标索引读写 `d[k] = v`、字典展开与空值合并运算符 `??`。
  - **新增第二十章《高级集合数据结构规范 (Map & Set)》**：详尽收录 ES6 标准 `Map`（`set`, `get`, `has`, `delete`, `clear`, `size`, `keys`, `values`, `entries`、`map[k]` 下标语法糖）与 `Set`（`add`, `has`, `delete`, `clear`, `size`, `values`），以及底层的零 GC Arena 内存管理与生命周期。
  - **新增第二十一章《文本与数学核心内建对象》**：全面记录字符串方法套件（`indexOf`, `slice`, `replace`, `split`, `trim` 等 14+ API）、反引号模板字符串 `` `Hello ${name}` ``、POSIX `RegExp` 正则表达式引擎（`test`, `match`）、以及全局 `Math` 高性能数学运算对象（常量、浮点极值、三角函数、随机数）。
  - **新增第二十二章《高并发服务、多线程与网络系统》**：收录对标 Bun.js 的原生 `serve({ port, fetch })` 高性能 HTTP 反应堆、`Request`、`Response`、`json(...)`；收录轻量 Actor 协程（`@spawn`, `@receive`, `@reply`, `@self`）；收录 OS 多线程 Worker（`@worker_spawn`）与无锁跨线程环形信箱；收录 AI 大张量跨进程零拷贝共享内存 `SharedTensor`。
  - **新增第二十三章《系统标准库生态索引》**：提供系统级 `cap:*` 模块索引总表（涵盖 `cap:io`, `cap:http`, `cap:actor`, `cap:worker`, `cap:fs`, `cap:net`, `cap:crypto`, `cap:sqlite`, `cap:json`, `cap:mem`, `cap:test`, `cap:beam`）。

---

## 2026-10-25 (模板字符串系统端到端打通、转义加固与编译期常量折叠)

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 增强 `Token::TemplateStringRaw` 解析逻辑，支持 `\$`（字面量美元符号）、`` \` ``（字面量反引号）与 `\\`（字面量反斜杠）的转义，杜绝价格符号与格式化冲突；
  - 引入引号状态机（`in_single_quote`, `in_double_quote`, `escape`），在 `${...}` 提取内部表达式时，智能忽略字符串字面量内部的花括号，彻底修复包含 `{` / `}` 的嵌套字符串提前闭合 Bug。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `analyze_expr` 中深度递归分析 `Expr::Template` 内部的所有插值表达式；
  - 在 `infer_type` 中将 `Expr::Template` 正确推导为 `Type::String`；
  - 在 `inner_span` 中关联 `Expr::Template` 的源码位置。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 注入 Zig 运行时格式化万能适配器 `cp_fmt(v)`，无缝支持字符串切片、布尔值、整数、浮点数、动态 `Message`、可选类型及任意结构体；
  - 实现 `Expr::Template` 编译期常量折叠（Constant Folding）：若所有插值均为常量字面量，编译期直接折叠为纯 Zig 静态切片，零运行时开销、零堆分配；
  - 实现动态模板字符串的高速格式化代码生成，利用 `escape_zig_fmt_string` 对非动态文本槽位执行 Zig 级别 `{` -> `{{`、`}` -> `}}` 转义，配合 `std.fmt.allocPrint` 达成极致原生的字符串合成；
  - 在 `infer_cp_type_with_mock` 与 `infer_zig_type` 中注册 `Expr::Template => Type::String` / `"[]const u8"`。
- [`cap/test/test_template.cp`](file:///data/dev/cp/cap/test/test_template.cp):
  - 扩充综合测试用例，覆盖基础插值、跨类型多插值、表达式计算、纯文本常量折叠、`\$` 与 `` \` `` 转义、原生 JSON 结构化负载、内嵌大括号表达式、链式方法调用（`.toUpperCase()`）及同步函数返回模板字符串全套场景。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 扩充第二十一章第 2 节，新增模板字符串转义规则、零转义 JSON 友好设计、常量折叠与链式调用规范说明。

---

## 2026-10-25 (`_` / `this` 首参三态借用与所有权设计全景验证与 `&_` 增强)

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 扩展 `parse_params` 参数解析，新增对 `Token::Ampersand`（`&_`、`&this`、`&mut _`、`&mut this` 及 `&param`）的首参修饰支持，使成员方法可变借用语法与 `for &x of list` 原地借用和 `&param` 借用参数保持绝对正交统一。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 增强 `Expr::Borrow` 中的 `is_class_ref` 识别，修复访问类字段借用（`&_.field`）产生多余二级指针的隐患。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 2 节与第 10.5 节规范，完整收录首参三态体系：缺省省略只读借用、`&_`（及兼容 `mut _`）可变借用、`move _`（及兼容 `move this`）所有权消费，详尽定义其语义规则、生命周期联动与编译期安全保障。

### 新增文件 (Added)

- [`cap/test/test_this_underscore_interchange.cp`](file:///data/dev/cp/cap/test/test_this_underscore_interchange.cp):
  - 涵盖缺省只读借用省略、`_` 与 `this` 属性互访、`&_` / `mut _` / `&this` / `mut this` 可变借用变异、`move _` / `move this` 所有权消费、单继承派生调用及异步状态机 `async fn` 上下文穿透综合测试用例，100% 编译运行通过。

---

## 2026-10-28 (异步任务编排 cap:parallel、事件驱动 cap:events 与跨任务借用安全检查)

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 为 `ArrowExpr` 新增 `is_async: bool` 字段，精确区分普通同步闭包与 `async () => { ... }` 异步任务闭包。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `parse_primary` 中支持 `async () => { ... }`、`async fn(...) { ... }` 与 `async ident => ...` 箭头表达式语法；
  - 修复 `parse_bin_expr` 与 `parse_postfix_expr` 中将常规对象属性访问误判为 `EnumInit` 的歧义（限定仅当对象标识符与属性名均为全大写枚举命名规范时方进入枚举初始化解析）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 注册 `Task`、`Event`、`EventEmitter` 内建类型与 `spawn`、`join`、`@task_spawn`、`@task_join`、`@create_pending_task`、`@complete_task` 内建函数；
  - 优化 `Pattern::Tuple` 解构语义，允许从 `Type::Any` 解构出动态元组；
  - 为 `join` / `@task_join` 与 `spawn` / `@task_spawn` 注入精准的语义返回类型推导。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 注入原生并发运行时前置基座：`Task`、`JoinBarrier`、`__cp_task_bridge`、`cp_msg_to_val`、`cp_unpack_elem` 以及泛型 `print(v: anytype)`；
  - 实现 `spawn` / `@task_spawn` 代码生成：分配独立 Arena，实例化 CPS 状态机上下文，通过 `__cp_task_bridge` 接入协程调度器；
  - 实现严密的跨任务内存借用检查网：严格禁止临时借用（`&`、`&mut`）与栈绑定 `ref struct` 跨越 `spawn` 边界，强制使用 `move` 或 `Shared<T>` 消除数据竞争；
  - 实现 `await` 多任务等待与单任务解析，支持 `let [msg1, msg2] = await join(task1, task2)` 元组智能解包与字符串自动推导；
  - 支持闭包/Lambda 对 `this` / `_` / `self` 的环境捕获与上下文自动传递；
  - 适配同步函数返回 Lambda 函数指针时的 `@ptrCast(@constCast(&lambda_X))` 自动转换；
  - 过滤标准库 `lib/cap/parallel.cp` 中 `class Task` 与 `print`、`spawn`、`join` 的重复顶层符号声明，避免 Zig 命名碰撞。
- [`lib/cap/io.cp`](file:///data/dev/cp/lib/cap/io.cp):
  - 导出 `export fn print(msg: any): void`，供非 `@print` 宏规范调用。
- [`lib/cap/parallel.cp`](file:///data/dev/cp/lib/cap/parallel.cp):
  - 实现并导出 `Task`、`spawn`、`join` 并发核心任务管理组件。
- [`lib/cap/events.cp`](file:///data/dev/cp/lib/cap/events.cp):
  - 实现并导出 `EventEmitter`（`on`, `off`, `emit`, `once`, `listenerCount`, `removeAllListeners`）与 `Event`（`isSet`, `set`, `reset`, `wait`），支持 `defer unsub()` 自动退订。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 扩充第二十二章，新增第 5 节《异步任务编排与并行任务池 (`cap:parallel`)》与第 6 节《事件驱动与协程同步子系统 (`cap:events`)》；
  - 更新第二十三章系统标准库总表，收录 `cap:parallel` 与 `cap:events`。

### 新增文件 (Added)

- [`cap/test/test_parallel_join.cp`](file:///data/dev/cp/cap/test/test_parallel_join.cp): 跨任务并发 `spawn`、`join` 屏障汇聚与 `let [msg1, msg2] = await join(...)` 元组解包测试，原生全绿通过。
- [`cap/test/test_parallel_fail_borrow.cp`](file:///data/dev/cp/cap/test/test_parallel_fail_borrow.cp): 验证跨 `spawn` 边界非法捕获局部临时借用触发编译期严密拦截。
- [`cap/test/test_events_pass.cp`](file:///data/dev/cp/cap/test/test_events_pass.cp): `EventEmitter` 事件监听分发与 `Event` 协程同步信号标志测试，原生全绿通过。

---

## 2026-10-28 (`@parallel` 多核 Worker 线程池、`spawn.cpu` 专用计算线程与跨任务只读/可变借用及所有权检查)

### 修改文件 (Modified)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 注册 `@task_spawn_cpu` 内建函数并返回 `Type::Ref("Task")`；
  - 在 `infer_type` 的 `Expr::Call` 分支识别 `spawn.cpu(...)` 属性调用语法，推导返回 `Task` 类型。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 `generate_module` 中解析 `main` 函数上的 `@parallel` / `@parallel(N)` / `@parallel(threads: N)` 与 `@single_thread` 元注解，并支持读取 `cap.json` 中的 `"mode": "parallel"` 配置；
  - 注入线程安全的 `GlobalTaskQueue`（带 `Spinlock` 保护的 2048 槽位环形队列）、`_WorkerPoolRunner` 工作线程池循环、`global_active_tasks` 与 `global_running_workers` 原子计数器；
  - 升级 `Task` 与 `JoinBarrier` 为自旋锁（`Spinlock`）并发安全结构，引入 `remaining = total + 1` 与 `finish_setup_and_should_suspend()` 机制消除主线程注册屏障与 Worker 线程并发完成任务之间的竞态条件；
  - 实现 `spawn.cpu(...)`（及 `@task_spawn_cpu`）代码生成，自动拉起独立 OS 计算线程执行重度 CPU 密集型闭包并通过 `__cp_task_bridge` 唤醒 `await` / `join`；
  - 强化跨任务闭包捕获的内存安全检查：只读捕获外部变量时调用 `check_borrow_conflict(r, false)` 并禁止在闭包内直接修改外部变量（报错 `Cannot mutate read-only variable or captured outer variable`）；支持在闭包内使用 `move` 转移外部变量所有权并在外层实施 Use-After-Move 拦截。
- [`lib/cap/parallel.cp`](file:///data/dev/cp/lib/cap/parallel.cp):
  - 更新 `spawn` 与 `@parallel`、`spawn.cpu` 注释说明。
- [`cap/test/test_parallel_join.cp`](file:///data/dev/cp/cap/test/test_parallel_join.cp):
  - 升级为 `@parallel(4)` 多核并行测试，覆盖 `spawn` 只读捕获、`spawn` + `move` 所有权转移、`spawn.cpu` 重度计算线程派发及 `await join(task1, task2, cpu_task)` 并发汇聚。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 22.5 节，完整收录 `@parallel` / `@single_thread` / `cap.json` 多核运行时配置、`spawn.cpu` 专用计算线程原语及跨任务只读借用、禁止可变修改与 `move` 所有权检查规范。

### 新增文件 (Added)

- [`cap/test/test_parallel_fail_mutate_capture.cp`](file:///data/dev/cp/cap/test/test_parallel_fail_mutate_capture.cp): 验证在 `spawn` 并发任务中直接修改捕获的外部变量时触发编译期拦截。
- [`cap/test/test_parallel_fail_use_after_move.cp`](file:///data/dev/cp/cap/test/test_parallel_fail_use_after_move.cp): 验证在 `spawn` 并发任务中 `move` 转移外部变量所有权后，外层再次访问触发 `Use of moved value` 编译期拦截。

---

## 2026-10-28 (全局统一只读借用 `&`、可变借用 `*`、Unsafe 裸指针 `*... !` 与所有权转移 `move` 符号标准)

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_params`：将前置 `*`（如 `*x: T`、`*_`、`*this`）及 `: *T` 类型参数统一解析为可变借用（`is_mut = true`）；将前置 `&`（如 `&x: T`、`&_`、`&this`）统一解析为显式只读借用（`is_mut = false`）；
  - `parse_for_stmt`：将 `for *x of list` 统一解析为原地可变借用（`IterMode::MutBorrow`），将 `for x of list` 与 `for &x of list` 统一解析为只读借用（`IterMode::Borrow`）；
  - `parse_base_type`：为可变借用类型 `*T` 增加第 3 级参数名生命周期绑定支持（如 `: *x Tensor`、`: *(x | y) Tensor`、`: *_ Tensor`）；
  - `parse_primary_expr`：支持 `*b!` 与 `**b!` 直接裸指针获取表达式（前置 `*` 层级 + 尾置 `!`），自动映射为对应层级的 `Type::RawPtr` 转换。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `infer_type` 的 `Expr::Cast` 分支中支持对 `*b!` / `**b!` 表达式进行自动目标裸指针类型推导并强制校验 `unsafe` 上下文。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 `infer_cp_type_with_mock` 与 `Expr::Cast` 代码生成中支持 `*b!` / `**b!` 的内部类型自动解析与 Zig 指针强转生成。
- [`cap/test/test_this_underscore_interchange.cp`](file:///data/dev/cp/cap/test/test_this_underscore_interchange.cp):
  - 更新并验证 `&_` 显式只读借用接收者与 `*_` / `*this` 可变借用接收者。
- [`cap/test/test_for_pass.cp`](file:///data/dev/cp/cap/test/test_for_pass.cp):
  - 更新并验证 `for &x of nums` 显式只读迭代与 `for *x of mut_list` 原地可变借用迭代。
- [`cap/test/test_unsafe_pass.cp`](file:///data/dev/cp/cap/test/test_unsafe_pass.cp):
  - 验证 `*user!`、`**p_copy!`、`*num!` 裸指针获取语法。
- [`cap/test/test_lifetime_pass.cp`](file:///data/dev/cp/cap/test/test_lifetime_pass.cp):
  - 验证 `ref struct` 中 `: *Tensor` 字段与 `*t1` 可变借用传参。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 在第十章新增《0. 全局借用、所有权与指针符号统一标准 (Global Symbol Matrix)》，并全面同步更新第 2.5 节、第十章、第十七章与第十八章示例与规范表。

## [Unreleased] - 2026-10-28 (Part 3)

### 可变借用变量操作 (`*v`, `p.* = 20`, `p.a = 10`, `p.push(30)`) 与容器改变操作全面纳入可变借用检查 (`Array`/`Vec`, `Map`, `Set`, `Dict`)

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_params` 将非接收者参数的前置 `*x: T` 与 `&x: T` 统一包装为 `Type::BorrowType { inner, is_mut, lifetimes }`，与 `x: *T` / `x: &T` 保持完全等价的 AST 表示；
  - `parse_base_type` 支持 `Array<T>` 与 `Vec<T>` 泛型容器类型语法并映射为 `Type::Array(Box::new(elem))`；
  - `Token::New` 支持 `new Map<K, V>()` / `new Set<T>()` / `new Dict<K, V>()` 可选泛型实参解析。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - `analyze_module` 注册内置 `Dict` 类符号；
  - `analyze_expr` (`Expr::Call`) 将 `"set"`、`"put_all"`、`"add"` 与 `"push"`、`"pop"`、`"delete"`、`"clear"` 共同纳入 `loop_borrowed_vars` 循环借用期间的可变拦截；
  - `types_eq` 允许 `Type::Array` 与同构 `Type::Tuple` 兼容；
  - `infer_type` (`Expr::Index`) 支持自动解包 `Type::BorrowType` 后进行下标类型推导。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 新增 `loop_borrowed_containers` 与 `anon_obj_structs` 状态跟踪；
  - 在 Zig 运行时前导中新增 `cp_array_lit`、`CpArrayElemType`、`cp_array_push`、`cp_array_pop`、`cp_container_clear`、`cp_index_of`、`cp_last_index_of`、`cp_includes`，并升级 `CpIndexReturnType`、`cp_index_get`、`cp_index_assign`、`cp_iter_slice`、`cp_mut_slice` 支持借用数组指针 `*[]const T` 与原地自动解引用修改；
  - 实现函数内同名重声明变量 Alpha-Renaming（`rename_shadowed_vars_in_block`），完美支持同一作用域连续多次声明 `let v = ...; let p = *v;` 且具有不同底层类型；
  - 将匿名对象字面量类型 `Type::Object` 规范化映射为顶层具名结构体 `const __CpObj_... = struct { ... };`，解决跨状态机/函数的 Zig 匿名结构体名义类型兼容问题；
  - 统一实现 `check_mutation_allowed` 与 `check_read_allowed`：全面覆盖 `p.* = val`、`p.a = val`、`p[i] = val`、`for *x of v` 以及容器方法 `push`、`pop`、`set`、`put_all`、`add`、`delete`、`clear` 的可变借用权限、NLL 活跃借用读写互斥与 `for` 循环借用锁定检查；
  - 修复 `compute_last_uses_stmt` 对控制流块（`If`/`While`/`For`/`Match`/`TryCatch`）的语句计数对齐，确保 NLL 最后一次使用（Last Use）语句序号与 `generate_stmt` 100% 精确同步。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 在第十章第 3 节补充可变借用变量的三类核心操作（`p.* = 20`、`p.a = 10`、`p.push(30)`）及 `Array`/`Vec`、`Map`、`Set`、`Dict` 容器改变操作的四重编译期可变借用检查规范与代码示例。

### 新增文件 (Added)

- [`cap/test/test_mut_borrow_ops.cp`](file:///data/dev/cp/cap/test/test_mut_borrow_ops.cp): 覆盖基础类型 `*v` (`p.* = 20`)、对象字面量 `*v` (`p.a = 100`)、数组 `*v` (`p.push(30)`、`p.pop()`、`p[1] = 25`、`for *item of v`) 以及 `Map`/`Set`/`Dict` 全套可变借用操作的正向综合测试。
- [`cap/test/test_mut_borrow_fail_readonly.cp`](file:///data/dev/cp/cap/test/test_mut_borrow_fail_readonly.cp): 验证通过只读借用 `let r = &v` 调用 `r.push(30)` 被编译器静态拦截的反向测试。
- [`cap/test/test_mut_borrow_fail_conflict.cp`](file:///data/dev/cp/cap/test/test_mut_borrow_fail_conflict.cp): 验证当 `Map` 存在活跃只读借用 `let r = &m` 时调用 `m.set("k", 1)` 被编译器静态拦截的反向测试。

## [Unreleased] - 2026-10-28 (Part 4)

### 严禁同作用域重复声明变量 (Disallow Same-Scope Variable Redeclaration)

### 修改文件 (Modified)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::DuplicateVarDecl(String, Span)`（`Compile Error: Cannot redeclare block-scoped variable '{0}' at {1:?}`）；
  - 在 `analyze_pattern` 中严格检查当前块作用域（`self.scopes.last()`）是否已存在同名变量绑定（`SymbolKind::Var`，排除通配符 `_`），若在同一 `{}` 作用域重复声明则立即报错拦截；
  - 将类方法中的类字段作用域与方法参数/方法体局部块作用域分离为双层作用域，允许方法内局部变量合法遮蔽同名类字段，同时禁止在方法顶层块内重声明方法参数或同名局部变量。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 新增 `check_and_record_scope_pat`，在 `rename_shadowed_vars_in_stmt` / `rename_shadowed_vars_in_block` 中按词法块维护独立的 `scope_vars: HashSet<String>`：对同一词法块内的重复变量声明执行编译期拦截，同时对合法的不同/平行子块（如 `{ let v = 10; } { let v = [10, 20]; }`）保留 CPS 状态机上下文字段的 Alpha-Renaming 消歧。
- [`cap/test/test_mut_borrow_ops.cp`](file:///data/dev/cp/cap/test/test_mut_borrow_ops.cp):
  - 将三段独立可变借用演示代码包裹在各自独立的 `{ ... }` 块作用域中。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 在第十章第 1 节新增“严禁同作用域重复声明变量（No Same-Scope Redeclaration）”规范，并同步更新第 3 节示例。

### 新增文件 (Added)

- [`cap/test/test_var_fail_redeclare.cp`](file:///data/dev/cp/cap/test/test_var_fail_redeclare.cp): 验证在同一 `{}` 块作用域内重复声明 `let v = 10; let v = [10, 20];` 时触发编译期报错拦截。

## [Unreleased] - 2026-10-28 (Part 5)

### 彻底移除 `codegen.rs` 同名遮蔽重命名代码 (`rename_shadowed_vars_*`) 并统一 `*` 模式匹配通配符同名检查

### 修改文件 (Modified)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 彻底删除此前误判同名重复 `let` 定义时引入的 `check_and_record_scope_pat`、`rename_shadowed_vars_in_pat`、`rename_shadowed_vars_in_expr`、`rename_shadowed_vars_in_stmt`、`rename_shadowed_vars_in_block`（共 420+ 行冗余代码）；
  - 在 `Stmt::Var`、`extract_locals_simple` 与 `extract_local_vars_to_str` 中完善 `Pattern::Wildcard`（`*`）支持（涵盖 `let * = expr;`、`let [*, x] = arr;`、`let {a: *, b} = obj;`）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `analyze_pattern` 中移除对 `_` 的豁免（在 CP 中通配符为 `*` 即 `Pattern::Wildcard`，而 `_` 是合法的变量/接收者标识符，重复绑定 `_` 同样报错 `DuplicateVarDecl`）；
  - 升级 `bind_match_pattern` 与 `bind_for_pattern` 返回 `SemaResult<()>`，对 `match` 模式与 `for` 循环解构中的同名变量（如 `[a, a]`、`[_, _]`）及循环体/分支块内的同名重声明统一执行 `DuplicateVarDecl` 报错拦截。
- [`cap/test/test_mut_borrow_ops.cp`](file:///data/dev/cp/cap/test/test_mut_borrow_ops.cp):
  - 将三段独立演示拆分为 `demo_primitive()`、`demo_object()`、`demo_array()` 三个独立函数，并增加 `let [*, second, *] = v;` 通配符 `*` 解构验证。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 明确说明模式匹配与解构通配符为 `*`（而非 `_`），重复绑定 `_` 或任意同名变量均触发编译期错误。

### 新增文件 (Added)

- [`cap/test/test_pat_fail_duplicate.cp`](file:///data/dev/cp/cap/test/test_pat_fail_duplicate.cp): 验证在模式解构中重复使用 `_`（`let [_, _] = [10, 20];`）时触发 `DuplicateVarDecl` 编译期报错拦截。

## [Unreleased] - 2026-10-28 (Part 6)

### 解构与模式匹配全面支持 `*`（单元素通配符）与 `...`（首/中/尾多元素忽略与零拷贝切片收集）

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `parse_single_pattern` 中新增 `Token::DotDotDot` (`...`) 解析：当后接标识符时解析为具名剩余切片模式 `Pattern::Rest(Box::new(Pattern::Ident(id)))`（如 `...rest`、`...middle`），当为裸 `...` 时解析为多元素忽略模式 `Pattern::Rest(Box::new(Pattern::Wildcard))`（如 `let [first, ..., last] = arr;`），并在对象模式 `{ a, ... }` 中支持尾部 `...` 忽略。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::MultipleRestInPattern(Span)`，严格限制同一数组/元组解构模式中最多只能包含一个 `...`；
  - 在 `analyze_pattern`、`bind_match_pattern`、`bind_for_pattern` 中全面支持 `Pattern::Rest` 位于开头（`[..., last]`）、中间（`[first, ..., last]` / `[head, ...middle, tail]`）或末尾（`[first, ...rest]`），自动计算前缀与后缀索引（反向偏移 `len - offset_from_end`），并将具名 `...rest` 推导为切片类型 `Type::Array(elem_ty)`。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 新增 Zig 运行时编译期辅助函数 `CpUnpackTailType` / `cp_unpack_tail`（按尾部偏移提取元素）与 `CpUnpackSliceType` / `cp_unpack_slice`（按前/后缀边界生成零拷贝子切片 `v[start .. v.len - suffix_len]`），并支持解包借用切片 `*[]const T`；
  - 在 `Stmt::Var`、`generate_pattern_match`、`Stmt::For`、`extract_locals_simple` 与 `extract_local_vars_to_str` 中全面支持 `Pattern::Wildcard` (`*`) 与 `Pattern::Rest` (`...` / `...rest`)。
- [`cap/test/test_mut_borrow_ops.cp`](file:///data/dev/cp/cap/test/test_mut_borrow_ops.cp):
  - 新增 `let [*, second, *] = v;`、`let [first, ..., last] = nums;` 与 `let [head, ...middle, tail] = nums;` 解构测试。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 补充单元素通配符 `*`、多元素剩余/忽略模式 `...`（裸 `...` 与具名 `...rest`）以及与范围符号 `..` / `..=` 正交分离的语法规范与示例。

## [Unreleased] - 2026-10-28 (Part 7)

### 全面重构《CP 语言官方设计规范与架构白皮书》（cp_ref.md）并升级编译器管道通配占位符与 Match 推导

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 彻底清洗重构历史遗留对话草稿（清除旧的 `Result/Option`、`let mut`、`loop` 及冲突通配符），升级为 11 篇体系完备的工业级金牌参考规范；
  - 全面收录 18 项核心要求：Nest 智能体容器（更名与定义）、双轨派生模型（`spawn` 协程池 vs `spawn.cpu` Worker 物理线程池）、`cap.json` 与 `@parallel` / `@single_thread` 并发配置、3-State 原生错误体系、容器写保护铁律（只读借用期间禁扩容与禁元素原地修改）、双轨对象模型（静态匿名结构体 vs 动态 `#{}` 字典）、`Box<dyn Trait>` 胖指针与 `downcast<T>()`、解构 `*` 与 `...`、管道占位符 `*` 与接收者 `_` 彻底解耦、`unsafe` 与裸指针、`macro` 宏系统、类继承 `init`/`release` 钩子等。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `in_pipeline_rhs > 0` 管道表达式右侧解析中，支持 `*`（`Token::Star`）作为通用通配占位符（如 `data |> process(5, *)`），与方法接收者 `_` 彻底解耦。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `infer_type` 中增加对 `Expr::Match`（`switch` 表达式）分支返回值的静态类型推导，避免类型退化为 `Type::Any`。

### 新增文件 (Added)

- [`cap/test/test_pipe_star.cp`](file:///data/dev/cp/cap/test/test_pipe_star.cp): 验证管道操作符配合 `*` 通配占位符与 `.` 占位符的正向编译与执行测试。

## [Unreleased] - 2026-10-29 (Part 8)

### 对象解构全链路支持 `...rest` 剩余属性变量绑定与 `...` / `...*` 忽略通配

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `ObjectPatProp` 新增 `is_rest: bool` 标识符，精准表示具名剩余属性（`...rest`）或匿名忽略剩余属性（`...` / `...*`）。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 在 `Pattern::Object` 递归提取闭包变量时，准确支持识别并注册解构对象中的具名 `...rest` 变量。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 重构对象模式解析：当遇到 `Token::DotDotDot` (`...`) 时，若后接标识符解析为 `ObjectPatProp { key: rest_id, val: None, is_rest: true }`；若后接 `*` 或直接为末尾/逗号，解析为通配忽略 `ObjectPatProp { key: "*", val: Some(Wildcard), is_rest: true }`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `bind_match_pattern` 与 `bind_for_pattern` 中支持对象解构模式中最多只能有 1 个 `...` 的合法性校验，并为具名 `...rest` 定义作用域变量；
  - 在 `analyze_pattern` 中支持剩余字段强类型计算：自动筛除已被非 rest 属性提取的键，将剩余属性构建为强类型匿名对象 `Type::Object(remaining_fields)` 并绑定到 `rest` 变量中。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 `Stmt::Var`（`let { a, ...rest } = obj;`）中计算剩余字段，为 `rest` 生成强类型匿名结构体 `.{ .b = _tmp_obj.b, .c = _tmp_obj.c }`；
  - 在 `generate_pattern_match`（`switch (obj) { { a, ...extra } => extra.b }`）中生成剩余字段结构体绑定；
  - 在 `Stmt::For`（`for { a, ...rest } of list`）中支持循环迭代解构绑定剩余字段；
  - 在 `extract_locals_simple` 与 `extract_local_vars_to_str` 中支持 CPS 状态机注册 `rest` 变量及其 `__CpObj` 类型。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 3.2 节与生态路线图，正式标记对象解构 `...rest` 剩余绑定已全面实装。

### 新增文件 (Added)

- [`cap/test/test_obj_destructure_rest.cp`](file:///data/dev/cp/cap/test/test_obj_destructure_rest.cp): 验证对象解构具名 `...rest`、裸 `...`、`...*`、`switch` 分支与 `for` 循环解构的全链路正向测试。
- [`cap/test/test_obj_rest_fail_multiple.cp`](file:///data/dev/cp/cap/test/test_obj_rest_fail_multiple.cp): 验证对象解构中包含多个 `...` 时的编译期错误拦截反向测试。

## [Unreleased] - 2026-10-29 (Part 9)

### 扩充《CP 语言官方设计规范与架构白皮书》（cp_ref.md）：新增附录《CP 在 AI 与 AI 智能体领域的独到优势与设计哲学》

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 目录（TOC）更新：新增《附录：CP 在 AI 与 AI 智能体领域的独到优势与设计哲学》三级索引（A.1、A.2、A.3）；
  - 全文末尾系统性增补附录 A，涵盖：
    - **A.1 主流语言物理死穴全景对比**：深度剖析 Python（GIL 锁死/asyncio 卡顿/内存暴涨）、TypeScript（单线程阻塞/无原生多核/V8 堆上限）、Go（百万 Agent 记忆引发 STW 垃圾回收停顿）、Rust（async 状态管理地狱/Arc-Mutex 泛滥/无抢占调度）、Zig（纯手工内存管理/缺乏并发信箱原语）在海量智能体场景下的致命瓶颈；
    - **A.2 CP 语言六大核心独到优势**：
      1. 独创“双轨派生模型”（`spawn` 纳秒协程池 vs `spawn.cpu` Worker 物理线程池）；
      2. 彻底无 GC（No GC）+ CPS 状态机切分（百微秒级冷启动、单机支撑百万活跃 Agent 驻留）；
      3. `SharedTensor` 全局共享堆与零拷贝原子引用穿透通信；
      4. 抢占式调度（Reductions 计数时间片，杜绝复杂 Prompt/CoT 思维链饿死集群）；
      5. 双轨对象模型（静态匿名结构体 vs 动态字典 `#{}`）与 3-State 原生错误体系（LLM Tool Calling 零阻抗与极简容错）；
      6. Crash-Proof 故障隔离与“任其崩溃”哲学（Supervisor Tree 热拉起与 NIF Resource Objects 资源兜底）；
    - **A.3 典型真实业务场景落地实测对比**：万人同域 NPC 仿真 (Virtual Town)、高频金融量化群智决策、边缘嵌入式 Agent 设备矩阵对比。
- [`.gitignore`](file:///data/dev/cp/.gitignore):
  - 添加 `*-bak*.md` 规则，防止规范文件历史备份草稿污染 git 工作区。

## [Unreleased] - 2026-10-29 (Part 10)

### 配合用户对 `cp_ref.md` 的内容更新，重新整理并同步文档目录索引 (TOC)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 仅重新整理文档开头的 `## 目录索引 (Table of Contents)`，未改动任何正文内容；
  - 第一篇目录全量同步用户新增的各大章节：
    - `1.2 CP/Rust/Zig/Go/TypeScript 精简对比表`（包含 1.2.1 差异化标签与 1.2.2 优势）；
    - `1.3 为AI而生：CP在 AI/AI智能体 领域的独到优势与设计哲学`（包含 1.3.1 物理死穴对比、1.3.2 六大独到优势与核心架构解析、1.3.3 典型场景实测直观对比）；
    - `1.4 高性能服务后端（替代部分 Go/C++/C/Rust/Zig）`；
    - `1.5 CLI工具、开发工具链（替代 Go / Zig）`；
    - `1.6 嵌入式、裸机、RTOS（Zig原生主场，CP是加分版）`；
    - `1.7 游戏引擎底层、高性能引擎脚本/绑定层（替代 C++、部分Rust）`；
    - `1.8 跨平台系统软件、Daemon、系统组件（替代C / C++）`；
    - `1.9 高性能WebAssembly（Wasm）`；
    - `2.0 教育、入门系统级编程（这个是很特别的差异化赛道）`；
    - `2.1 现有Zig项目的上层迁移层（Zig生态内部的升级选择）`；
    - `2.2 边界：哪些场景CP不适合主推（心里要有数）`；
    - 以及构建与包管理器相关章节索引；
  - 移除已合入第一篇的原末尾附录目录索引条目。

## [Unreleased] - 2026-10-29 (Part 11)

### 重构《CP 语言官方设计规范与架构白皮书》（cp_ref.md）：独立升格第二篇《`cap` 统一工具链与模块生态体系》并全面对标 Bun.js

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 全局章节升格：将规范手册由原 11 篇系统性扩充重组为 **12 篇完整宏伟架构**，原第二篇至第十一篇依次顺延为第三篇至第十二篇，对应子章节编号（`3.1`~`12.2`）全链路对齐；
  - 第一篇定名为《第一篇：CP 语言哲学、核心愿景与全景场景矩阵》，纯粹聚焦于核心定位、竞品对比、AI/智能体独到优势与 9 大落地场景；
  - 正式独立升格设立《第二篇：`cap` 统一工具链与模块生态体系 (The `cap` All-in-One Toolchain & Ecosystem)》：
    - **2.1 架构愿景与品牌标识**：引入 CAP ($C^P$) 官方 ASCII 标志徽章，深度确立对标 Bun.js 的“全生态归一”哲学（CAP 之于 CP 犹如 Bun 之于 TS/JS）；
    - **2.2 CLI 全指令全景矩阵**：系统梳理 21 项指令，包含已实装指令（`run`, `build`, `-r`, `-f`, `-s`, `--exe`, `--obj`, `--lib`, `-b`, `add`, `-g`, `test`）、正在演进指令（`init`, `remove`, `update`, `bench`）与规划中指令（`fmt`, `lint`, `publish`, `cache clean`）；
    - **2.3 全局缓存与软链接零拷贝注入**：绘制 Mermaid 架构流向图，阐释 `~/.cap/cache/` 全局单实例存储与 `cap_modules/` 毫秒级符号软链接注入机制，终结 `node_modules` 磁盘黑洞；
    - **2.4 `cap.json` 权威清单规范**：详述工程清单、并发模式、依赖 Semver、Monorepo `workspaces`、编译控制与 Nest 容器专有参数；
    - **2.5 ES6 风格模块系统与别名权威裁决**：
      - 深度论证并裁决模块更名为何坚守 `as`（`import { a as b }`）而非 `:`，确立 `:` 作为类型注解的神圣正交性、保持 `* as math` 的纯粹自洽，并对齐主流工程直觉；
      - 详解三层向上攀升依赖查找机制（Upward Resolution）与目录缺省入口推断优先级；
    - **2.6 CAP 特性演进与对标路线图**：以对照表清晰展示 CAP 对标 Bun / Cargo 的全景落地现状与未来路线；
  - 同步全量更新全文顶部的 `## 目录索引 (Table of Contents)`。

---

## [Unreleased] - 2026-10-29 (Part 12)

### CAP ($C^P$) 统一工具链工业级重构：配置驱动构建、多模式优化矩阵、测试套件与脚手架全面落地

### 修改文件 (Modified)

- [`cap/src/project/manifest.zig`](file:///data/dev/cp/cap/src/project/manifest.zig):
  - 深度扩充 `cap.json` 清单数据模型：新增 `description`、`mode`、`dev_dependencies`、`build_config`（`opt`, `out_dir`, `target`, `targets`, `c_flags`, `link_libs`）与 `nest_config`（`mailbox_capacity`, `reduction_slice`）；
  - 引入 `OptimizeMode` 枚举（`Debug`, `ReleaseSafe`, `ReleaseFast`, `ReleaseSmall`）及字符串双向映射方法；
  - 增强 `addDependency` 支持开发依赖（`is_dev` 参数），新增 `removeDependency` 实现依赖全生命周期管理；
  - 升级 `save()` 保证清单全字段高保真格式化输出，支持数组与对象树序列化。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - 路由中枢全面升级：实装 `build`, `run`, `init`, `add`, `remove` (`rm`), `update` (`up`), `install`, `test`, `bench`, `sniff`, `help` 全指令矩阵；
  - 构建系统打通 `cap.json` 配置解析与 CLI 参数覆盖：确立 `CLI 参数 > cap.json build 配置 > 内置默认值` 决策优先级链；
  - 完整支持 Zig 4 级优化模式切换（`-f`/`--fast`, `-s`/`--small`, `--debug`, `--safe`, `--opt=<mode>`）；
  - 支持单目标交叉编译覆盖（`-t`/`--target=<triple>`）与配置驱动的多平台发布矩阵（`-r`/`--release`）；
  - 实装 `cap test [dir]`：自动递归扫描 `test/` 发现所有测试用例，在 `Debug` 模式下秒级编译执行并输出美观的进度耗时与汇总统计面板；
  - 实装 `cap bench <file.cp>`：在 `ReleaseFast` 极致算力模式下编译并执行，使用 `std.Io.Timestamp` 进行纳秒级高精度基准计时；
  - 实装 `cap init [app|agent|lib]`：开箱即用多模板脚手架，一键初始化规范目录结构、对应 `cap.json` 与代码样例；
  - 实装 `cap remove` 与 `cap update` 依赖卸载与升级。
- [`cap/build.zig`](file:///data/dev/cp/cap/build.zig):
  - 增加对 `../bin` 的安装步骤，确保 `zig build` 能够同步生成并刷新 `cap/bin/cap` 与 `cap/zig-out/cap` 二进制文件。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第二篇 2.2 全指令全景矩阵，将 `cap init`, `cap remove`, `cap update`, `cap bench` 状态升格为 **已全面实装**；
  - 更新 2.4 `cap.json` 规范章节，详述 Zig 4 大编译优化级别（`debug`, `safe`, `fast`, `small`）、`targets` 平台矩阵以及构建参数覆盖三层优先级决策链。

---

## [Unreleased] - 2026-10-29 (Part 14)

### 品牌标识与徽标体系重塑：确立 CP 语言专属双字母徽标与 CAP 工具链方案 A 向上跃升箭头徽标（C ^ P => CAP）

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 开篇正文引入 **CP 语言原生专属徽标**（`C` + `P` 经典双字母对称架构），代表 CP 语言本体（并发算力与精密掌控）；
  - 第二篇 2.1 正式装配 **CAP 工具链方案 A 专属徽标**（`C` + `^` + `P`）；
  - 彻底重塑品牌定义：明确 CAP 绝非“C 的 P 次方”，而是以代表并发算力向上跃迁、智能涌现与极速调度的向上大箭头 **`^` 代替字母 `A`**，直观牢固地连接 `C` 与 `P`，在视觉与语意上拼写出 **`CAP`**。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - 全面更新终端 ASCII 艺术徽标为方案 A：包括 `cap help` 帮助头、`cap test` 测试套件运行头以及 `cap bench` 微基准评测运行头。

---

## [Unreleased] - 2026-10-29 (Part 15)

### 官方命名体系全景统一：确立 CP (Concurrent Plus) 与 CAP (Concurrent Application Plus) 宏大格局

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 规范开篇与核心愿景全量统一为 **CP (Concurrent Plus)**；
  - 第二篇 2.1 节正式定名为 **CAP (Concurrent Application Plus)**；
  - 详述 `CP` 与 `CAP` 的同源血脉与几何同构：在 `C`（Concurrent 调度基座）与 `P`（Plus 性能安全增益）之间，注入字母 `A`（`Application`），由向上跃迁大箭头 `^`（Ascend）生动连接，象征由语言规范迈向生产级应用交付的全景赋能。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - 终端 `usage()`, `handleTest()`, `handleBench()` 欢迎横幅全量更新为 `CAP : Concurrent Application Plus v0.1.0`；
  - 重新编译输出最新原生二进制，全面通过实测。

---

## [Unreleased] - 2026-10-29 (Part 16)

### 增量编译流水线 (Incremental Build)、编译耗时度量 (Build Profiling) 与缓存清理 (cap clean) 实装

### 修改文件 (Modified)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 移除 8 处调试遗留的 `[TRACE]` 标准输出，净化前端转译终端输出。
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs):
  - 增加转译内容一致性预检：若输出 `.zig` 内容与已有文件一致，跳过磁盘写入以保留文件修改时间戳（`mtime`），供下游构建工具进行增量缓存判定。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - **增量构建系统 (Incremental Build)**：检查输出二进制与源码 `.cp` 的 `mtime`，未发生变更时 0ms 极速跳过转译与 LLVM 链接，对标 Bun/Go 的即时开发体验；
  - **中间态感知复用**：若中间态 `.cap/zig/` 源码新于 `.cp` 源码，自动跳过 `cpc` 前端转译；
  - **编译耗时度量 (Build Profiling)**：在编译过程中精确度量 `cpc` 前端转译毫秒数、Zig/LLVM 原生链接秒数与全流程总耗时；
  - **强制构建参数 (`-B` / `--force`)**：支持跳过增量缓存直接强制重新编译；
  - **缓存与产物清理 (`cap clean`)**：一键安全清理 `.cap/zig/` 中间源码、`.cap/zig-cache/` 编译器缓存、`.cap/obj/` 与产物目录 `bin/`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新 2.2 CLI 全景矩阵表格，正式收录 `cap clean` 与 `cap build -B` 选项。

---

## [Unreleased] - 2026-10-29 (Part 17)

### 官方用户门户与主自述文件 (README.md) 工业级重构与全景技术展示

### 修改文件 (Modified)

- [`README.md`](file:///data/dev/cp/README.md):
  - 引入完整的 **CP 语言双字母徽标** 与 **CAP 工具链方案 A 向上大箭头徽标**（`C • ^ • P`）；
  - 全面系统地剖析 Python、Node.js、Go、Rust、Erlang 在百万级 AI Agent 并发下的痛点与 CP 终极破局之道；
  - 阐述“脑、神经、肌肉、血液”四位一体终极架构并嵌入 Mermaid 架构图；
  - 新增 Actor 消息信箱、Bun 风格极速 HTTP 服务端与 `SharedTensor` 硬件原子零拷贝三大代码实战展示；
  - 详述 CAP 工具链从调试（`cap <file>`）、增量编译（0ms）、极限性能（`-f`）、边缘微体积（`-s`）、矩阵发布（`-r`）、测试、基准到一键缓存清理（`cap clean`）的全流程工作流；
  - 制作全方位横向对比矩阵，直观对比 Python、Node.js、Go、Rust 与 CP 在 8 大维度的核心指标。

---

## [Unreleased] - 2026-10-31 (Part 18)

### 批量标签收拢语法 (`@[...]`) 与弱类型即席元标注 (`@meta(...)`) 零成本正交设计实装

### 新增文件 (Added)

- [`cap/test/test_batch_meta_pass.cp`](file:///data/dev/cp/cap/test/test_batch_meta_pass.cp):
  - 覆盖类、方法、字段级批量收拢容器 `@[...]`，切面装饰器与元注解混排；
  - 验证弱类型即席元标注 `@meta(...)` 搭配 `@[reflect, ...]` 时属性扁平直读与命名空间嵌套读取；
  - 验证未加 `@reflect` 的纯元数据类拥有零运行时开销。
- [`cap/test/test_meta_fail_unreflected.cp`](file:///data/dev/cp/cap/test/test_meta_fail_unreflected.cp):
  - 反向测试：未标注 `@reflect` 的类在调用 `reflect.type<T>()` 时，编译器精准拦截并输出诊断引导。

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_meta_ident` 与 `parse_meta_arg`：扩展支持 `Token::Meta`, `Token::Dec`, `Token::Macro` 等关键字作为元标注标识符，解决关键字作为标注名的语法解析问题；
  - `parse_single_meta_inner`：严格遵循函数式括号传参规范 `(...)`，纠正方括号参数误区（`[]` 专属于批量集合容器 `@[...]`）；
  - `parse_meta_list`：实装 `@[...]` 批量语法，一口气收拢声明多个元注解、装饰器与即席标注，末尾逗号自动容错；
  - `parse_module_item`：前瞻分析器支持 `@[...]` 括号深度精准匹配，确保声明关键字安全解析。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 注册内建 `@meta` 弱类型元注解契约（允许多目标全覆盖：类、方法、字段、参数、Trait、Enum，支持多实例重复标注）；
  - 践行“零成本抽象（Zero-Cost Abstraction）”方案 A：严格保持正交性，`@meta` 专注于元数据挂载，运行时反射必须由 `@reflect` 统一显式授权激活，未授权时精确报错。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - `generate_meta_population`：为 `@meta` 实现平铺注入顶层属性（`info.meta["author"]`）与嵌套命名空间（`info.meta["meta"]["author"]`）的双重读取支持；
  - `generate_reflection_tables`：仅对显式标注 `reflect` 的类生成运行时反射单例字典，杜绝无谓的二进制代码膨胀与 Tree-Shaking 阻碍；
  - `generate_class`：增加对 `TypeInfo`、`FieldInfo`、`MethodInfo` 等内建类型的重复生成过滤，防止导入时符号重复定义冲突；
  - `cp_message_from` & `cp_index_get`：修复指针类型包装时取栈局部变量地址导致的悬垂指针缺陷，并让 `cp_index_get` 支持对 `Message.any` 动态 Map 的链式属性递归读取。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 完整规范化第 3.6 节《弱类型/即席元标注（Ad-hoc Metadata：`@meta(...)`）》与第 3.7 节《批量标签收拢语法（`@[...]`）》，强调 `[]` 集合语义与零成本正交设计。

---

## [Unreleased] - 2026-11-01

### 标载荷代数数据类型 (Tagged Union Enum) 与 Switch 全模式解构变量捕获体系实装

### 新增文件 (Added)

- [`cap/test/test_switch_binding_pass.cp`](file:///data/dev/cp/cap/test/test_switch_binding_pass.cp):
  - 覆盖带载荷枚举（无载荷 `Idle`、单载荷 `Text: string`、元组载荷 `Move: [i32, i32]`、结构体载荷 `Compute: { batch: usize, priority: i32 }`）；
  - 覆盖上下文推导前缀省略构造（`.Idle`, `.Text("hello")`, `.Move [15, 25]`, `.Compute { ... }`）；
  - 覆盖 `switch` 元组方括号解构模式匹配（`.Move [x, y]`）；
  - 覆盖 `switch` 结构体字段解构匹配（`.Compute { batch, priority }`）；
  - 覆盖 `switch` 结构体剩余/全捕获匹配（`.Compute { ...fields }`）；
  - 覆盖 `switch` 海象操作符区间变量绑定（`s := 90..=100`）；
  - 验证运行结果 100% 符合预期。

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 新增 `#[token(":=")] ColonAssign` 词法标记，支持 Walrus（海象操作符）变量绑定语法。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `Pattern` 枚举新增 `Pattern::Binding { ident: Ident, subpat: Box<Pattern> }`（变量与子模式绑定）与 `Pattern::TypeMatch { ident: Ident, ty: Type }`（动态类型匹配分支）；
  - `EnumInitExpr` 增加 `#[serde(default)] pub tuple_args: Option<Vec<Expr>>`，原生支持 `.Move [x, y]` 元组构造。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 在 `extract_idents_from_pat` 中适配 `Pattern::Binding` 与 `Pattern::TypeMatch`，确保变量分析与借用检查完整捕获绑定标识符。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `Parser` 引入 `in_switch_arm: usize` 状态追踪，精准消除 `switch` 分支中 `ident: Type` 类型分支与对象字面量键值对的文法歧义；
  - `parse_single_pattern`：支持 `.Variant` 匹配时直接接 `{ ... }`（含 `...rest` 字段解构）、`[ ... ]`（元组解构）、`(...)`（括号解构）与单个 `Ident` 载荷捕获；
  - 支持 `ident := subpat` 海象操作符绑定与 `ident: Type` 动态类型匹配；
  - 彻底移除旧版 Zig 风格的 `|fields|` 管道捕获语法；
  - `parse_enum`：支持 `Ident: Type` 语法定义枚举载荷变体；
  - `parse_primary_expr` & `parse_postfix_expr`：支持在已知类型上下文中直接使用 `.Variant { ... }`、`.Variant [ ... ]`、`.Variant( ... )` 进行隐式推导构造。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `bind_match_pattern`、`bind_for_pattern` 与 `analyze_pattern` 中支持 `Pattern::Binding` 与 `Pattern::TypeMatch` 的作用域符号注入与类型推导；
  - 放行前缀为空的隐式推导枚举构造（推断为 `Type::Any` 或上下文目标类型）。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - `generate_pattern_match`：支持带载荷枚举多元素元组解构绑定（`cp_unpack_elem`）、结构体属性提取与剩余字段全量打包绑定（`...fields`）；
  - 支持 `Pattern::Binding` 生成 `const ident = val_expr;` 绑定，以及 `Pattern::TypeMatch` 的运行时类型判断与指针安全窄化转换；
  - 修复 `Expr::Cast` 对数值类型（整数、浮点数）无脑生成 `@ptrCast(@alignCast(...))` 的底层缺陷，精确按目标类型生成 Zig `@intCast`、`@floatCast`、`@intFromFloat` 与 `@floatFromInt`；
  - 重命名内部异步任务结构为 `CpTask`，避免用户定义 `enum Task` 时与 Zig 预置上下文名称冲突。
- [`cap/test/test_traits_enums.cp`](file:///data/dev/cp/cap/test/test_traits_enums.cp):
  - 语法升级：将旧版遗留的 `match (task) { .Compute |fields| => ... }` 重构为规范的 `switch (task) { .Compute { batch } => ... }`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 增补第 3.5 节《枚举与代数数据类型（Enum & Tagged Union）》，规范化定义、上下文推导与构造语法；
  - 增补第 4.5 节《枚举载荷解构、Walrus 绑定与动态类型匹配》，详细记录结构体解构、元组解构、海象绑定、类型分支匹配及废除 `|fields|` 的设计。

---

## 2026-11-01 (全量编译期元编程系统 Comptime、自动单态化与静态内省原语)

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 为 `IfStmt` 与 `ForStmt` 增加 `is_comptime: bool` 字段，支持静态分支修剪与循环展开；
  - 新增 `Expr::Comptime(BlockStmt)` 表达式变体，支持 `@{ ... }` 编译期求值代码块；
  - 新增 `Type::Auto` 类型变体，支持 `auto` 编译期参数类型推导。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 新增 `Token::Auto` 关键字词元。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_params`：收敛形参约束语法，仅保留统一规范的 `@comptime param: Type`（严格契合“带 @ 为编译期”铁律），切除 bare `comptime`；
  - `parse_base_type`：收敛自动推导语法，仅保留统一规范的 `auto`，彻底切除冗余的 `anytype`；
  - `parse_stmt`：识别 `@{ ... }` 与 `!{ ... }` 产生 `Stmt::ComptimeBlock`；识别 `@if` 与 `@for` 并置 `is_comptime = true`；
  - `parse_primary_expr`：支持 `@{ ... }` 产生 `Expr::Comptime`；将非内置宏指令的 `@fn(...)` 统一解析为 `Expr::Call { is_comptime: true, ... }` 编译期函数显式调用；将 `@test`、`@expect_eq` 以及其他标准库宏指令纳入内置宏保护清单。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 增强 `infer_type` 对 `Expr::Comptime` 与 `Expr::Unsafe` 的类型推导容错，支持块内局部变量推导并在未解析时优雅降级；
  - 完善 `Stmt::ComptimeBlock` 与 `Expr::Comptime` 块内语义检查与作用域管理。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 将 `Type::Auto` 映射为 Zig 原生 `anytype`；
  - `Expr::Comptime` 与 `Stmt::ComptimeBlock`：生成 `comptime (blk: { ... })` 与 `comptime { ... }`，生成期间自动将 `is_state_machine` 置为 false 并置 `is_comptime` 为 true，确保局部变量直接生成为编译期变量而非状态机字段；
  - `Stmt::If`：当 `is_comptime` 时，生成 `if (comptime ...) { ... } else { ... }` 实现编译期静态分支修剪；
  - `Stmt::For`：当 `is_comptime` 时，生成 `inline for (...) |...| { ... }` 实现编译期静态循环展开；
  - 函数形参：`is_comptime` 形参生成 `comptime ` 前缀，跳过取地址操作；
  - 函数调用：`is_comptime` 函数调用生成 `comptime callee(args)`；
  - 内建编译期内省指令映射：支持 `@typeOf`、`@typeName`、`@fieldsOf`、`@field`、`@hasField`、`@sizeOf`、`@alignOf`、`@compileError`、`@compileLog`；
  - `infer_cp_type_with_mock` 与 `infer_zig_type`：递归分析 `Expr::Comptime` 与 `Expr::Unsafe` 内部局部变量推断真实返回类型；
  - `cp_add`：增强多整数类型混用的原生加法支持（自动通过 `@intCast` 适配 `i32` 与 `usize` 等混合运算）。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 支持 `Type::Auto` 格式化为 `"auto"`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 支持 `Expr::Comptime` 遍历与访问。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 11.1 节《编译期元编程系统（Comptime Metaprogramming）》，详细记录 `@{ }` 编译期代码块、`@fn()` 编译期函数调用、`@comptime` 参数约束、`val: auto` 单态化、`@if` 静态分支修剪、`@for` 静态循环展开及 9 种内建编译期内省指令。

### 新增文件 (Added)

- [`cap/test/test_comptime.cp`](file:///data/dev/cp/cap/test/test_comptime.cp):
  - 涵盖 `@{ }` 编译期求值、`@add()` 编译期函数调用、`@comptime` 参数约束、`val: auto` 编译期推导参数、`@if / @else` 分支修剪与 `@for` 循环展开的完整自动化回归测试用例，100% 测试通过。

---

## 2026-11-01 (工业级宏元编程系统：内置派生宏、聚合派生 @derive 与第三方 AST 模板宏完整实装)

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 为 `MacroDef` 结构体新增 `methods: Vec<Function>`，支持宏体内定义可注入目标类型的模板方法。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 扩展 `Ident` 正则词元规则，原生支持识别 `${field}` 与 `get_${field}` 等 AST 模板插值标识符；
  - 注册 `#[token("$")] Dollar` 词元。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 重构 `parse_macro_def`：支持解析宏体花括号 `{ ... }` 内声明的函数（`fn` / `pub fn`）与语句，注入 `MacroDef.methods`；
  - 在 `is_known_builtin` 注册清单中补全 `@hash` 原语以及全部底层系统内置宏（`@io_*`、`@fs_*`、`@net_*`、`@http_*`、`@crypto_*` 等）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 全量内置派生宏支持：
    - `@Clone`：自动合成 `fn clone(_): Self` 深拷贝方法（返回 `new Class(_.f1, _.f2, ...)`）；
    - `@Comparable`：自动合成字典序比较 `fn cmp(_, other: &Self): i32` 与等价性比对 `fn eq(_, other: &Self): bool`；
    - `@Hash`：自动合成 `fn hash(_): usize`，结合全字段级联 FNV-1a 计算稳定哈希值；
    - `@ToString`、`@Json`、`@Table`：稳定继承并支持多方法批量衍生；
  - `@derive(...)` 聚合宏与 `@[...]` 批量语法：统一展开引擎，支持任意组合派生（如 `@derive(Clone, Comparable, Hash, ToString, Json)`）；
  - 第三方 AST 模板插值宏（Template Macros）：
    - 支持宏定义形参到实参的映射绑定与 AST 变量替换（函数名 `${field}`、属性访问 `_.${field}`、返回类型及字面量文本）；
    - 保留无体空宏向后兼容性（如 `macro Tag(target) {}` 自动回退生成 `getTag(): string`）；
  - 零样板构造函数自动合成：当类中包含字段且开发者未显式编写 `fn init(...)` 时，编译期 Sema Phase 2 自动生成成员级 `init` 构造函数。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 Zig 运行时基座注入 `cp_lt`、`cp_gt`、`cp_lte`、`cp_gte` 与 `cp_hash_val` 强类型原生辅助函数；
  - `BinaryOp`：`<`、`>`、`<=`、`>=` 自动映射为 `cp_lt` / `cp_gt` / `cp_lte` / `cp_gte`，支持字符串与复杂结构的原生字典序比对；
  - `Expr::Builtin`：为 `@hash` 原语生成零开销 FNV-1a 编译期级联哈希内联块 `(blk: { var _h: usize = ...; break :blk _h; })`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 全面更新第 11.2 节《宏元编程（Macro Metaprogramming）》，详尽记录内置派生宏清单（`@Clone`、`@Comparable`、`@Hash`、`@ToString`、`@Json`、`@Table`）、`@derive(...)` 聚合宏、`@[...]` 批量注解语法、第三方 AST 模板宏（`${param}` 插值）与 Comptime 过程宏的最佳实践与代码范式。

---

## 2026-11-01 (语法纯净化：切除遗留 @derive，全面归一为原生批量注解 @[...])

### 修改文件 (Modified)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 彻底切除并移除 Rust 遗留的 `@derive` 冗余宏名称与 `expand_derive_macro` 展开逻辑；
  - 消除语法冗余与表达割裂，唯一标准统一为 CP 原生一等公民的批量元注解体系 `@[Clone, Comparable, Hash, ToString, Json]`。
- [`cap/test/test_macro_pass.cp`](file:///data/dev/cp/cap/test/test_macro_pass.cp):
  - 将 `OrderItem` 上的 `@derive(...)` 替换为规范的 `@[Clone, Comparable, Hash, ToString, Json]`，全量测试 100% 绿灯。

---

## 2026-11-01 (Debug 派生宏与双轨 for...of 流式迭代器架构完整落地)

### 修改文件 (Modified)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 注册并实装内置派生宏 `Debug`，自动合成 `fmt_debug(&_): string` 与 `toString(_): string` 双重调试打印方法；
  - 完美打通 `@[Debug]` 与 `trait Debug` 的契约装配，支持标准 Trait 格式化约束。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - `Stmt::For` 迭代引擎升级为**双轨智能分流架构**：
    - **快轨（Fast Path）**：针对连续切片与数组，保持极致的 `cp_iter_slice` 裸机遍历与 `while` 计数器循环；
    - **流式轨（Iterator Stream Path）**：针对包含 `next(*_): ?Item` 契约的对象或数据流，全自动展开为 `while (__iter.next()) |item|` 循环，当 `next()` 返回 `null` 时零开销自动终止，并支持元组/结构体解构模式。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 增补第 11.2.9 节《迭代器契约与双轨 `for ... of` 迭代机制》，详尽规范快轨连续内存遍历与流式迭代器实现；
  - 增补第 11.2.10 节《泛型 Trait Bound 契约约束》，阐述 `fn log(item: &Debug)` 与 `@[Debug]` 协作范式。
- [`cap/test/test_macro_pass.cp`](file:///data/dev/cp/cap/test/test_macro_pass.cp):
  - 新增测试用例 9：`@[Debug, Clone]` 自动合成 `fmt_debug` 与 `toString`；
  - 新增测试用例 10：自定义流式迭代器 `NumberStream` 经由 `for n of stream` 成功遍历求和，全量 100% 绿灯通过。

---

## 2026-11-01 (语法纯净化：全面清理 Rust 风格 &mut，统一为 CP 正统 *_ / *this 可变借用)

### 修改文件 (Modified)

- [`cap/test/test_macro_pass.cp`](file:///data/dev/cp/cap/test/test_macro_pass.cp):
  - 将流式迭代器 `NumberStream` 中的 `fn next(&mut _): ?i32` 彻底纠正为 CP 正规正统的 `fn next(*_): ?i32`；
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 将 11.2.9 节迭代器流式契约从 `next(&mut _): ?Item` 纠正为 `next(*_): ?Item`；
  - 清理 11.2 节宏示例中的 `set_${field}(*_)` 与 `reset(*_)` 接收者修饰符；
  - 清除全篇遗留的 Rust 风格 `&mut` 痕迹，强化 CP 中 `*_`（可变借用）与 `&_`（只读借用）的正交语义。

---

## 2026-11-02 (完备标准容器矩阵构建：实装 VecDeque、BinaryHeap、BTreeMap、BTreeSet 及完整借用检查)

### 新增文件 (Added)

- [`lib/cap/collections.cp`](file:///data/dev/cp/lib/cap/collections.cp):
  - 正式发布 CP 官方标准容器库 `cap:collections`，统一导出 `VecDeque`、`BinaryHeap`、`BTreeMap`、`BTreeSet`、`HashMap`、`HashSet`、`Map`、`Set`、`Dict`；
- [`cap/test/test_collections_matrix.cp`](file:///data/dev/cp/cap/test/test_collections_matrix.cp):
  - 覆盖双端环形队列、优先队列大顶堆、B 树有序键值映射、B 树有序集合、Rust 对齐别名以及可变/只读借用参数传递的全套 7 项综合测试，100% 绿灯全通。

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_base_type` 支持 `HashMap<K, V>`、`BTreeMap<K, V>` 映射为 `Type::Map`，`HashSet<T>`、`BTreeSet<T>` 映射为 `Type::Set`，并支持任意通用泛型类型参数消费与类型推断；
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 注册 `VecDeque`、`BinaryHeap`、`BTreeMap`、`BTreeSet`、`HashMap`、`HashSet` 为顶层预置全局符号；
  - 在 `analyze_expr` 中将 `push_back`、`push_front`、`pop_back`、`pop_front`、`insert`、`remove` 纳入 `loop_borrowed_vars` 循环借用锁定检查，杜绝循环遍历中的迭代器失效；
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 Zig 运行时前导中注入高性能 `VecDeque`（环形缓冲区）、`BinaryHeap`（堆排优先队列）、`BTreeMap`（有序平衡二分表）、`BTreeSet`（有序集合）及 `cp_msg_cmp` 全序比对函数；
  - 增强 `check_mutation_allowed` 与 `check_read_allowed`：全面对齐新容器的写入与只读方法，支持剥离 `BorrowType` 精准推导容器实体类型；
  - 增强 `CpAddReturn` 与 `cp_add`：支持 `Message` 与 `i32` 混合算术运算；
  - 升级 `Stmt::For`：无缝将四类新容器纳入双轨流式迭代路径 `while (it.next()) |item|`；
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新 6.3 节容器写保护铁律，纳安全监管矩阵；
  - 更新 12.1 节已实装标准库索引表，增补 `cap:collections`；
  - 新增 12.2 节《完备标准容器矩阵（cap:collections）与借用/所有权安全体系》，详尽规范时间复杂度、内存布局与生产级代码示例。

---

## 2026-11-02 (编译器工具链进化：默认编译指令、--lib 模块模式与 zig {} 原生逃逸舱)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 词法解析器新增 `Token::ZigEscape` 对 `zig` 关键字的支持，与 `__zig__` 双轨并存；
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `parse_ident` 中允许 `Token::ZigEscape` 作为上下文标识符，兼顾变量名使用与块级逃逸舱；
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 新增 `pub is_lib_mode: bool`；在 `--lib` 模式或无 `main()` 函数时，跳过 `pub fn main` 入口与事件循环的生成；
  - 将所有用户定义的 `class`、`enum`、`error set`、`trait`、匿名对象等以 `pub const` 导出，确保外部 Zig 模块（如 Bun.js 原生模块）可以通过 `@import` 无缝直接访问；
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs):
  - 重构 CLI 解析架构：支持直接 `cpc <INPUT> [-o <OUTPUT>] [--lib]` 执行默认编译动作，废除强制写 `compile` 的冗余心智负担，同时保留 `compile` 作为向后兼容别名；
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增 11.6 节《Native Zig 逃逸舱（`zig { ... }`）》；
  - 新增 13.1 节《`cpc` 独立编译器前端》CLI 指令规范。

---

## 2026-11-02 (Bun 迁移工作流指南规范发布：创建 BUN-GEMINI.md 与编译器直通授权)

### 新增文件 (Added)

- [`BUN-GEMINI.md`](file:///data/dev/cp/BUN-GEMINI.md):
  - 针对负责 Bun.js 1.3.14 迁移任务的 AGY 定制的全局指令规范与架构白皮书；
  - 开放 `/data/dev/cp` 编译器上游源码的读取与修改权限，制定遇编译问题直接修改 `cpc` 源码并热更新的自愈工作流；
  - 明确软链接路径映射（`/root/.cap/bin/cpc -> /data/dev/cp/cap/bin/cpc`），将可执行文件覆盖路径规范为 `/data/dev/cp/cap/bin/cpc`；
  - 明确单系统构建无需交叉编译，确立“日常高频迁移首选 Debug 模式（1~2秒快速热更新且带调用栈）、阶段性收尾可选 Release”的高效提速工作流；
  - 提供完整 Zig → CP 语法与内存安全映射字典、`zig { ... }` 原生逃逸舱范式及四阶段迁移路线图。

---

## 2026-11-03 (四层类型转换系统实现：无损隐式流转、显式 as 截断、裸指针 unsafe 隔离与高阶方法解析)

### 修改文件 (Modified)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `SemaError` 中新增 `InvalidCast(String, Span)` 错误变体；
  - 新增 `can_coerce`、`int_fits_in_type`、`is_safe_widening`、`is_subclass`、`type_repr`、`is_numeric_type` 等静态类型流转与判定方法；
  - 落地第一层无损自然流转（Zero-Cost Coercion）：
    - 字面量自适应推导（Literal Fitting）：字面量数值在目标类型范围内自动适配（如 `let a: u64 = 100;`），超出范围静态拦截；
    - 数值无损拓宽（Safe Widening）：无符号小转大（`u8 -> u16 -> u32 -> u64 -> usize`）、有符号小转大（`i8 -> i16 -> i32 -> i64 -> isize`）、小整数/浮点转大浮点（`i32 -> f64`, `f32 -> f64`）自动流转，严禁要求 `as`；
    - 安全结构向上包装：具体值自动包装进可选类型 `T -> T?`、错误联合 `T -> ?T`；
    - 借用权限收紧：独占可变借用 `*T` 自动流转为只读借用 `&T`；
    - 面向对象与 Trait 多态：子类自动流转为基类，实现 Trait 的结构体借用自动流转为 `&dyn Trait`；
    - 符号跨作用域同名匹配：消除 `Ident.span` 差异对类型名相等性判定的干扰；
  - 落地第三层裸指针安全隔离：
    - `Expr::Cast` 全面检查源类型与目标类型，若任一方为裸指针且不在 `unsafe` 作用域内，静态报错 `Raw pointer casting is only allowed inside unsafe block`；
  - 落地第四层高阶语义转换约束：
    - 静态拦截 `string` 与数字之间的 `as` 强转，输出友好引导信息（提示使用 `.parse(...) ?` 或 `.to_string()`）；
  - 在变量声明 `Stmt::Var` 与赋值表达式 `BinaryOp::Assign` 中深度接入 `can_coerce` 静态校验。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 Zig 运行时前导代码中注入 `cp_cast_int(comptime Dest: type, val: anytype) Dest` 辅助函数，完美解决 Zig `@intCast` 运行期溢出 panic 及跨符号位 `@truncate` 限制，实现底层零开销位截断与符号位重新解释；
  - 在 `generate_expr` 的 `Expr::Cast` 中：
    - 目标为整数时：浮点源使用 `@intFromFloat`，裸指针源使用 `@intFromPtr`，枚举源使用 `@intFromEnum`，其余整数使用 `cp_cast_int`；
    - 目标为浮点数时：整数源使用 `@floatFromInt`，浮点源使用 `@floatCast`；
    - 目标为裸指针时：整数源使用 `@ptrFromInt(@as(usize, @truncate(...)))`，指针源使用 `@ptrCast(@alignCast(...))`；
    - 目标为枚举时：使用 `@enumFromInt(...)`。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs), [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs), [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs), [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs), [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 运行 `cargo fmt` 保持代码风格完全一致。

### 新增文件 (Added)

- [`cap/test/test_type_conversion.cp`](file:///data/dev/cp/cap/test/test_type_conversion.cp):
  - 四层类型转换系统长期回归测试套件，全面覆盖字面量适配、无损拓宽、可选包装、有损截断、符号位反转、浮点整型互转、枚举互转与裸指针在 unsafe 内外的行为验证。

---

## 2026-11-03 (Bun 迁移赋能：循环控制 break/continue、定长数组重复初始化与类型流转增强)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增 9.3.1 节《循环控制与跳转语句 (`while`, `break`, `continue`)》；
  - 在第 8 章与第 9 章规范中新增定长数组类型 `[T; N]` / `[N]T` 与重复初始化语法 `[val; count]`（如 `let zero_buf: [u8; 256] = [0; 256];`）。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `Stmt` 枚举中新增 `Break` 与 `Continue` 语句变体；
  - `Expr` 枚举中新增 `ArrayRepeat(Box<Expr>, Box<Expr>)` 表达式；
  - 适配管道操作符占位符检测与代换。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 在变量引用遍历中适配 `Stmt::Break`、`Stmt::Continue` 与 `Expr::ArrayRepeat`。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 词法解析器注册 `break` (`Token::Break`) 与 `continue` (`Token::Continue`) 关键字。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 支持解析 `break;` 与 `continue;` 语句；
  - 支持解析定长数组类型声明 `[T; N]`；
  - 支持在方括号中解析重复初始化表达式 `[val; count]`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 AST 代换、语义分析及类型推导中全面支持 `Stmt::Break`、`Stmt::Continue` 与 `Expr::ArrayRepeat`；
  - `Expr::ArrayRepeat` 推导为 `Type::FixedArray`（count 为整型字面量时）或 `Type::Array`；
  - 在 `can_coerce` 中增强对定长数组 `Type::FixedArray` 及动态数组 `Type::Array` 的递归推导支持（元素字面量自适应与重复初始化支持）。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 增强 `cp_cast_int`：完善 `comptime_int` 负数向无符号类型的底层位解释截断；
  - 为 `Stmt::Break` 生成 `break;`，为 `Stmt::Continue` 生成 `continue;`；
  - 新增 `expected_array_elem_type` 上下文推导，在 `Expr::ArrayRepeat` 中生成与 Zig 0.17 完美兼容的高性能定长广播 `@as([{count}]{elem_ty}, @splat(@as({elem_ty}, {val})))`。

### 新增文件 (Added)

- [`cap/test/test_bun_features.cp`](file:///data/dev/cp/cap/test/test_bun_features.cp):
  - 验证 `while` 循环中的 `break` / `continue` 跳转控制，以及定长数组 `[val; count]` 重复初始化的编译与运行。

---

## 2026-11-03 (编译器字段推导、空值合并字面量收敛与错误联合 Catch 增强)

### 修改文件 (Modified)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 增强 `can_coerce` 字面量按需收敛（Layer 1.1）：支持在 `Expr::NullCoalesce`（`??`）与 `Expr::QuestionFallback` 表达式中递归对 fallback 分支进行字面量收敛推导；
  - 修正 `infer_type` 对 `Expr::Ident` 中接收者标识符 `_` 与 `this` 的处理，使其优先通过符号表作用域解析出具体类类型 `Type::Ref`；
  - 增强 `Expr::Member` 结构体/类成员类型推断：通过遍历类字段与其继承链，精准获取访问字段的声明类型；同时对字符串与数组的 `.len` / `.length` 推导为 `Type::USize`。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 修正 `Expr::Catch` 在 `infer_cp_type` 中的解包逻辑：当被捕获表达式返回 `Type::ErrorUnion(t)` 时，正确解包为载荷类型 `*t`，杜绝变量局部推断类型残留 `anyerror!T`；
  - 增强 `Expr::Catch` 的 Zig 代码生成：识别原生 `anyerror!T` 错误联合与 `T ! E` 结果类型，生成 Zig 原生 `({exp}) catch |err| { ... }` 语法，并为 fallback 表达式注入 `@as(TargetType, val)` 显式类型收敛保护。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 标量类型表补充 `isize` 说明；
  - 完善第 5.3 节与 5.4 节：明确类方法中只读接收者 `_` 的缺省规则、方法体内直接访问属性/兄弟方法的无前缀语法规则（命名遮蔽时使用 `_.` 消除歧义）、以及 `!T`（极简推导错误联合，缺省 `Error`）与 `T ! ErrorSet`（强类型显式错误联合）的双轨制规范。

### 新增文件 (Added)

- [`cap/test/test_slice_calc.cp`](file:///data/dev/cp/cap/test/test_slice_calc.cp):
  - 针对 `calcSizeForSlice` 场景的长期测试用例，覆盖实例接收者省略（默认只读借用）、类方法内成员直接无前缀访问、极简 `!usize` 错误联合签名、字面量按需收敛 `pad_char ?? 0`、默认可变 `let result`（无需 mut）、可选类型安全判空 `!= null` 以及错误捕获 `catch 0`。

---

## 2026-11-03 (Bun AGY 极简 `err` 关键字、128 位整型支持与代码生成零成本抽象树摇优化)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 词法解析器注册 `err` 关键字（作为 `Token::ErrorKw` 别名），实现 3 字母极简大小写镜像对称哲学（`err` vs `Err` vs `errdefer`）；
  - 注册 `i128` 与 `u128` 词法标记；
  - 优化 16 进制、2 进制、8 进制以及大整数字面量解析，防止 64 位整型边界溢出。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) & [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 在基础类型枚举 `Type` 中新增 `I128` 与 `U128` 标量变体，适配分布式哈希与超高精度中间计算。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 错误集声明与返回联合支持 `err` 关键字（如 `err SetName { ... }`、`return err.Tag;`、`Type ! err`）；
  - 基础类型解析支持 `i128` 与 `u128`；
  - 模式匹配支持 `else =>` 作为后备通配分支（与 `* =>` 等价）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 语义分析与类型检查适配 `i128` / `u128`；
  - 增强 `can_coerce` 与字面量收敛适配超宽整型；
  - 支持定长数组与动态数组字面量自适应（`FixedArray`/`Array` 与 `Tuple` 解构互转推导）。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 实装模块级精细化摇树按需注入（`ModuleFeatures`）：纯计算模块（如 `wyhash`）彻底消除死代码（生成体积由 2400 行剧降至 250 行）；
  - 修正入口函数 `main` 与调度器按需注入逻辑：当存在 `main` 时确保调度内核与 `PID`/`schedule`/`cp_fmt` 正常发射；
  - 标量与无析构类型 Drop Guard 彻底豁免；
  - 修复整数字面量生成，保持负数正常十进制输出；
  - 原生 Zig `switch` 模式平铺发射。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 标量类型表增加 `i128` 与 `u128` 规格定义；
  - 新增 8.2.1 节阐述 3 字母极简哲学（`err` vs `Err` vs `errdefer`）；
  - 新增 11.7 节《零成本转译抽象：按需摇树注入与原生发射》，详解摇树机制、Drop Guard 豁免与紧凑 Switch 生成。

### 新增文件 (Added)

- [`cap/test/test_err_features.cp`](file:///data/dev/cp/cap/test/test_err_features.cp):
  - 长期回归测试用例，覆盖 `err` 关键字声明与返回、`!T` 推导错误联合、`128` 位整型运算、以及 `switch ... else =>` 语法，经 `./cap/bin/cap` 编译执行 100% 成功。

---

## 2026-11-04 (Bun 迁移深水区支持：转义标识符、纯枚举定长整型标签、方法/嵌入逃逸舱与多模式 Switch 匹配)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `Token::Ident` 转义标识符正则 `@"..."`，无缝支持网络协议、HTTP 标头与 Web 标准等带连字符名称（如 `@"same-origin"`、`@"no-cors"`）。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `EnumDecl` 增添 `methods: Vec<Function>`（实例方法）、`tag_type: Option<Type>`（底层定长整型类型）与 `extra_zig: Vec<String>`（内嵌原生 Zig 逃逸舱）；
  - `Field` 增添 `default: Option<Expr>`，支持字段默认初值赋值。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 实现通用 `parse_raw_zig_block`，支持在枚举体内直接内嵌 `zig { ... }` 原生逃逸舱注入编译期常量映射；
  - 支持无载荷纯枚举定义定长整型底层标签（如 `enum FetchRequestMode: u2 { ... }`）；
  - 支持在枚举体内定义实例方法与静态方法；
  - 模式匹配全面支持在 `in_switch_arm` 中以逗号 `,` 分隔多模式合并（如 `case .@"same-origin", .@"no-cors" =>`）；
  - 彻底修正 `Token::Bang` 与 `Token::Tilde` 一元运算符优先级绑定，调用 `parse_postfix_expr` 优先绑定后置方法与属性访问。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 适配并深度分析枚举内实例方法与静态方法，建立独立局部作用域并注入 `this`、`_`、`self` 接收者；
  - 修复匿名结构体推导中构造 `ast::Field` 时遗漏的 `default: None`。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 纯枚举直接发射为 Zig 原生 `pub const Name = enum(Tag) { ... };`；
  - 纯枚举变体带点访问（`Expr::Member` 与 `Expr::EnumInit`）统一生成 `Name.Variant`，杜绝结构体初始化语法报错；
  - 修正枚举实例方法生成逻辑，以值拷贝而非指针作为接收者（`self: EnumName`），并跳过形参列表中重复的 `self`；
  - 修复摇树中 `scan_expr` 遗漏 `Expr::Array` 导致 `cp_array_lit` 未定义的问题；
  - 智能识别纯枚举 Switch 穷尽匹配（`is_exhaustive_enum`），全覆盖时自动剔除不可达通配分支并禁止发射 `else => unreachable`；
  - 修复匿名结构体推导中构造 `ast::Field` 时遗漏的 `default: None`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 3.5.4 节《特殊符号变体、底层标签与嵌套逃逸舱》，规范转义标识符 `@"..."`、定长整型底层标签（`: u2`）、枚举内嵌 `zig { ... }` 块与实例方法规范；
  - 完善第 4.3 节关于联合多模式匹配（支持 `|` 与 `,` 逗号合并）说明。

### 新增文件 (Added)

- [`cap/test/test_bun_enums.cp`](file:///data/dev/cp/cap/test/test_bun_enums.cp):
  - 涵盖转义标识符 `@"same-origin"`、定长整型底层标签 `: u2`、枚举内实例方法调用、枚举内嵌 `zig { ... }` 逃逸舱函数、逗号多模式合并匹配以及穷尽 Switch 校验的长期回归测试用例，经 `./cap/bin/cap` 编译执行 100% 成功。
- [`cap/test/test_root_inline.cp`](file:///data/dev/cp/cap/test/test_root_inline.cp):
  - 验证 `@inline` 实例方法调用与数组字面量格式化输出的长期回归测试用例，经 `./cap/bin/cap` 编译执行 100% 成功。

---

## 2026-11-04 (原生逃逸舱类型值传递与函数未使用参数精准抑制)

### 修改文件 (Modified)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 顶层 `zig { ... }` 块符号扫描时，将所有声明的类型与别名自动加入 `ref_classes` 与 `value_classes` 白名单，确保跨模块引用时按原生值类型传递而非强加堆指针 `*T`；
  - 新增 `block_contains_ident` 辅助函数，递归扫描函数体内所有内嵌 `zig { ... }` 块是否包含指定形参标识符；
  - 重构函数形参未使用抑制逻辑：已引用的形参不生成任何抑制代码，真正未使用的形参生成符合 Zig 规范的 `_ = param;`（取代无条件取地址 `_ = &param;`）。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 13.2 节《原生逃逸舱类型映射与参数引用优化》，详细阐明外部原生类型直接发射与参数引用优化的工程规范。

### 新增文件 (Added)

- [`cap/test/test_escape_param_opt.cp`](file:///data/dev/cp/cap/test/test_escape_param_opt.cp):
  - 长期回归测试用例，验证逃逸舱原生结构体纯值传递、内嵌原生块中形参引用感知以及未使用参数抑制，经 `./cap/bin/cap` 编译执行 100% 成功。

---

## 2026-11-04 (结构体内嵌逃逸舱、多级命名空间类型、后缀推导错误联合与错误成员访问)

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `Class` 结构体新增 `extra_zig: Vec<String>`，用于存储类或结构体内部内嵌的 `zig { ... }` 原生逃逸舱源码片段。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `parse_class` 中支持解析类与结构体作用域内的 `zig { ... }` 块并存入 `extra_zig`；
  - 在 `parse_base_type` 中支持后缀 `T!` 语法：当后置 `!` 之后无特定错误标识符或 `err` 关键字时，将类型解析为推导错误联合 `Type::ErrorUnion(Box::new(base))`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 为所有内置 Mock 类（`RegExp`, `Set`, `Map`, `Dict`, `Error`, `Box` 等）补全 `extra_zig: vec![]` 初始化字段。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 扫描所有类的 `extra_zig` 声明，将 `pub const T = ...` 或 `const T = ...` 常量名称自动加入 `ref_classes` 与 `value_classes`；
  - 在结构体定义体内原样发射内嵌的 `class.extra_zig` 原生代码段；
  - `Expr::EnumInit`: 支持匿名空结构体 `.{{}}` 与字段紧凑结构体 `.{{ .field = val }}`；
  - `Expr::Member`: 支持 `err.Variant` 与 `error.Variant` 直接发射为 Zig 原生 `error.Variant`；
  - 静态对象字面量：空对象 `{}` 直接发射为 Zig 原生紧凑结构体 `.{}`；
  - `map_type`: 多级点路径类型（如 `jsc.ZigString.Slice`、`std.mem.Allocator`）按值类型直接发射完整路径，不添加 `*` 堆指针前缀。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 完善所有权与借用符号矩阵；
  - 新增第 13.3 节《结构体扩展逃逸舱、多级命名空间类型与标准 TS 静态结构体字面量》，详尽规范内嵌 `zig` 块、`{}` 结构体字面量、多级点路径类型与后缀 `T!` 体系。

### 新增文件 (Added)

- [`cap/test/test_class_extra_zig.cp`](file:///data/dev/cp/cap/test/test_class_extra_zig.cp):
  - 长期回归测试用例，覆盖结构体内嵌原生 `zig` 块、后缀 `T!` 推导错误联合、`err.Variant` 错误成员访问以及静态空结构体 `{}` 与匿名结构体 `{ count: 123 }`，经 `./cap/bin/cap` 编译执行 100% 成功。

---

## 2026-11-04 (海象操作符条件解包、函数缺省返回类型简写与前缀可选兼容优化)

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - `IfStmt` 结构体新增 `capture: Option<Ident>` 字段，用于支持 `if (binding := expr)` 的条件绑定标识符。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `parse_if_stmt` 与 `parse_while_stmt` 中全面支持海象操作符语法 `(binding := expr)`，同时对旧版 Zig 管道捕获 `|capture|` 输出清晰的迁移指引报错；
  - 函数、箭头函数与方法返回类型解析中，支持 `!` 与 `!Error` 简写形式（自动将 ok 类型补全为 `Type::Void`）；
  - 保留前缀 `?T` 向下兼容解析，杜绝历史测试用例与标准库代码被破坏性中断。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 在 `Stmt::If` 的语义分析中，若存在 `capture` 则自动创建局部子作用域并登记不可变绑定符号，支持后续表达式直接引用。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 `Stmt::If` 与 `Stmt::While` 的代码生成中，将海象操作符转译为 Zig 原生的 `if (expr) |binding|` 与 `while (expr) |binding|`；
  - 自动注入 `_ = &binding;` 抑制语句，避免 Zig 编译器在捕获未被全部分支引用时抛出 `unused capture` 错误；
  - 在 `type_to_display_string` 中将 `Type::ErrorUnion` 统一输出为现代后缀 `T!` 格式。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 9.3.2 节与第 9.3.3 节，详尽规范 `while (binding := expr)` 与 `if (binding := expr)` 的类型收窄、作用域隔离与零成本转译机制。

### 新增文件 (Added)

- [`cap/test/test_walrus_binding.cp`](file:///data/dev/cp/cap/test/test_walrus_binding.cp):
  - 长期回归测试用例，全量验证 `if (val := find())` 成功与 else 分支、`while (item := iter.next())` 循环解包、以及 `fn foo() !` 错误联合简写，经 `./cap/bin/cap` 编译执行 100% 成功。

---

## 2026-11-04 (全面废弃前缀 ?T 统一收敛为后缀 T?、匿名错误集前瞻消歧与测试库代码全量升级)

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 在 `parse_base_type` 中对前缀 `?` 实施编译期硬报错，强行拦截 `?T` 并给出迁移至 `T?` 或 `T?!` 的指导；
  - 新增 `is_error_type_rhs_start` 前瞻探测方法，精准判定匿名错误集 `!{ ... }`，消除函数体/代码块 `{` 与匿名错误集之间的语法冲突；
  - 恢复 `try/catch` 表达式中错误变量标识符的准确解析。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 引入 `cp_match_tag` 宏，支持 Zig 纯枚举 (`enum`) 与标签联合 (`union(enum)`) 在 `switch` 匹配中的无缝兼容；
  - 确保所有类/结构体始终发射 `pub fn _create(allocator)` 实例构造函数；
  - 统一 `Type::ResultType` 搭配 `Error` 时的转译逻辑为 `CpResult(ok, Error)`；
  - 在 `Expr::Builtin` 中正式支持 `@expect_eq` 与 `@test` 内建断言与运行宏。
- [`lib/cap/sqlite.cp`](file:///data/dev/cp/lib/cap/sqlite.cp):
  - 全量升级函数返回类型与字段类型为后缀 `T?` / `T?!` / `void!`。
- [`lib/cap/test.cp`](file:///data/dev/cp/lib/cap/test.cp):
  - 将 `test` 与 `expect_eq` 参数类型升级为泛型 `auto`，映射为 Zig `anytype`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 清理并统一所有示例与规范表述中的可选类型，全面采用后缀 `T?` / `T?!`。
- `cap/test/` (25+ 测试文件全量更新):
  - 包含 `hello.cp`, `main.cp`, `test_actor.cp`, `test_advanced.cp`, `test_all.cp`, `test_async_tuple.cp`, `test_builtins.cp`, `test_comptime.cp`, `test_dependency.cp`, `test_dist.cp`, `test_error_box_switch_pass.cp`, `test_error_fail_non_exhaustive_switch.cp`, `test_error_fail_null_coalesce_on_err.cp`, `test_errors.cp`, `test_features.cp`, `test_fs_async.cp`, `test_macro_pass.cp`, `test_map_set_dict.cp`, `test_matmul.cp`, `test_object.cp`, `test_spawn_arrow.cp`, `test_sqlite.cp`, `test_stdlib.cp`, `test_switch_binding_pass.cp`, `test_tensor.cp`, `test_traits_enums.cp`, `test_undefined.cp`, `test_zero_copy.cp`, `example_showcase.cp`。

---

## 2026-11-05 (评估并整合 Bun AGY 编译器升级、修复未变动变量判定 Bug、支持 CPC_BIN 环境变量路由与 err/move 语法双轨完善)

### 修改文件 (Modified)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - **关键修复**：回滚了导致不可变 `let` 变量在 Zig 中触发 `error: local variable is never mutated` 的误判逻辑，恢复对非状态机变量始终发射 `_ = &ident;` 取地址标记，彻底修复 `example_showcase.cp` 等测试用例编译失败问题；
  - 接入 `is_lib_mode`，优化库编译模式下 Zig 代码生成精简度；
  - 支持 `cstring` 映射为 Zig `[:0]const u8`（指针为 `[:0]u8`）；
  - 支持未注册 `@builtin` 宏透明透传至底层 Zig/C 函数调用。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - **双轨错误值构造器与匹配**：在 `parse_primary_expr` 与 `parse_pattern` 中支持小写 `err(...)`，同时 100% 保持对大写 `Err(...)` 兼容；
  - **灵活 move 参数位置**：在 `parse_params` 中同时支持 `fn f(move u: User)`（标识符前置）与 `fn f(u: move User)`（类型位置前置）；
  - 支持声明前置 `export`（如 `export fn`、`export class`）；
  - 支持 `[N]T` 固定长度数组类型语法。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 注册 `ident.sym == "err"` 为内建符号标识符；
  - 对 `@packed` 与 `@extern` 结构体免除成员逐项 `init` 构造器生成，保证二进制紧凑布局与 C ABI 对齐；
  - 增加编译期整型常量折叠计算 `eval_const_int_expr`。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - 增加 `getCpcBin()` 辅助函数，优先读取环境变量 `CPC_BIN`，未配置时安全回退至默认正式编译器路径 `/data/dev/cp/cpc/target/release/cpc`，支持多 Agent 并行开发；
  - 替换全部 5 处死绑定的编译器硬编码路径。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 补充 `cstring` 标量类型规范（对接 C/Bun 原生库，映射为 Zig `[:0]const u8`）；
  - 补充 `[N]T` 定长数组语法；
  - 补充 `export fn` 符号导出修饰符说明；
  - 补充 `@packed` 与 `@extern` 内存布局元属性；
  - 同步更新第 6 节（参数所有权转移语法）与第 8.2.1 节（`err(...)` 主推与 `Err(...)` 兼容）。
- [`.gitignore`](file:///data/dev/cp/.gitignore):
  - 将 `cpc_dev/` 目录加入版本忽略，防止并发沙盒本地改动污染主仓库。
- [`GEMINI.md`](file:///data/dev/cp/GEMINI.md):
  - 用户更新：明确 rm 指令针对 temp/test 目录及其文件的免申请操作权限。

### 新增文件 (Added)

- [`cap/test/test_err_and_move.cp`](file:///data/dev/cp/cap/test/test_err_and_move.cp):
  - 包含 `fn f(move u: User)`、`fn f(u: move User)`、`err(...)` 错误构造与模式匹配、大写 `Err(...)` 兼容性的全景回归测试，经 `./cap/bin/cap` 执行 100% 成功。

---

## 2026-11-05 (成员可见性 pub/priv 默认私有封装与统一错误体系 Err/AnyErr/T! 全景落地)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 注册 `priv` 与 `private` 关键字，解析为 `Token::Private`；
  - 识别 `Err` 与 `Error` 关键字，统一归入 `Token::ErrorKw`。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 在 `Field`、`Function`、`Class` AST 结构中增加 `is_pub: bool` 可见性修饰标记；
  - 在 `ErrorType` 枚举中新增 `AnyErr` 变体，表示映射 Zig 底层 `anyerror` 的全局开放错误集合。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - `parse_class`: 字段与方法默认标记为私有（`is_pub = false`），遇到 `pub` / `export` 设置 `is_pub = true`，遇到 `priv` 设置 `is_pub = false`；
  - `parse_macro_def`: 宏展开方法支持 `export` 与 `priv` 修饰符，内部辅助方法默认保持公开；
  - `parse_error_type_rhs`: 映射 `AnyErr` / `anyerror` 为 `ErrorType::AnyErr`，映射 `Err` / `Error` 为 `ErrorType::DefaultError`；
  - `parse_pattern` 与 `parse_primary_expr`: 全面支持多元动态错误 `err(code, msg)` / `err(msg)` 的语法解析与元组模式匹配。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 新增 `SemaError::PrivateAccess(String, Span)`；
  - 维护 `current_class: Option<String>` 上下文跟踪当前分析所在的类作用域；
  - 在 `Expr::Member`（属性访问）与 `Expr::Call`（方法调用）中严格校验成员可见性：跨类访问非 `is_pub` 成员立即在编译期抛出清晰错误；
  - **特权豁免**：生命周期方法（`init`、`release`、`_create`、`__*`）以及类自身内部访问（`this` / `_` / 裸标识符）均合法放行；
  - 修复宏模板替换按长度降序排序（`subst_ordered`），彻底消除 `${field}` 与 `field` 子串替换碰撞冲突。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 适配 `ErrorType::AnyErr => "AnyErr"` 与 `DefaultError => "Err"` 的反向格式化输出。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 发射 Zig 原生 `Err` 结构体（包含 `code: i32`、`msg: []const u8`、`cause: ?*anyopaque`，以及 `init`、`with_cause`、`downcast` 方法），并注入 `pub const Error = Err;` 兼容别名；
  - 生成 `cp_convert_err` 辅助函数，优先通过 `@hasDecl(TargetErr, "from")` 执行用户自定义声明式转换，或将任意错误包装提升为动态 `Err`；
  - 生成 `cp_has_ok_val`、`cp_unwrap_ok_val` 与 `cp_unwrap_err_val`，抹平 Zig 原生 `anyerror!T` 与 `CpResult` 在 Switch 模式匹配中的语法差异；
  - 在 `Stmt::Return` 中设立静态安全防线：若函数返回类型为 `T!`（`AnyErr`），严禁返回动态 `err(...)`，编译器直接拦截报错；
  - 修复类方法中异步回调状态机名称格式化（`ComplexAgent.onMessage`）；
  - 优化函数丢弃语句为 `_ = &self;`，防止 Zig 报 `pointless discard`；
  - 跟踪 `drop_flag_vars`，仅在真正声明过 drop flag 的变量上生成 `__{}_moved = true;`。
- [`lib/cap/http.cp`](file:///data/dev/cp/lib/cap/http.cp):
  - 依据类成员默认私有规则，为 `Request`、`Response`、`Server` 成员属性与方法补齐 `pub` 公开修饰符。
- [`lib/cap/sqlite.cp`](file:///data/dev/cp/lib/cap/sqlite.cp):
  - 补齐导出类的 `pub` 成员修饰符。
- [`cap/test/test_all_libs.cp`](file:///data/dev/cp/cap/test/test_all_libs.cp):
  - 适配公开访问属性的 `pub` 声明。
- [`cap/test/test_macro_pass.cp`](file:///data/dev/cp/cap/test/test_macro_pass.cp):
  - 适配宏生成方法的 `pub` 声明。
- [`cap/test/test_trait_class_pass.cp`](file:///data/dev/cp/cap/test/test_trait_class_pass.cp):
  - 适配测试类的 `pub` 声明。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 5.12.2 节详述类与结构体成员 `pub` / `priv` 访问控制与默认私有封装规范及代码示例；
  - 第 8.2 节全景重构为统一错误体系规范（`Err` 与 `Error` 别名、`T!` 作为 `T ! AnyErr` 省略、具名错误集 vs 普通枚举、自动转换与 `from` 桥接、全景 `switch` 模式匹配与 `downcast` 胖指针还原）。

### 新增文件 (Added)

- [`cap/test/test_pub_priv.cp`](file:///data/dev/cp/cap/test/test_pub_priv.cp):
  - 长期回归测试用例，覆盖属性/方法 `pub` 导出、默认私有封装、内部方法/属性自由访问、生命周期构造器豁免、以及外部私有访问编译期精准拦截。
- [`cap/test/test_unified_errors.cp`](file:///data/dev/cp/cap/test/test_unified_errors.cp):
  - 长期回归测试用例，覆盖具名错误集 `FileErr`、普通枚举错误 `HttpError`、业务动态错误 `Err`、全局推导错误 `AnyErr` / `T!`、`?` 自动转换、声明式 `from`、全景 `switch` 错误匹配解构与 `downcast` 动态向下转型。
- [`cap/test/test_error_union_downcast.cp`](file:///data/dev/cp/cap/test/test_error_union_downcast.cp):
  - 长期回归测试用例，覆盖具名与匿名即席联合声明（`FileErr || err{Timeout, Canceled}`）、具名与动态 Err 联合声明（`FileErr || Err`）、以及 `switch` 下对 `err(e: ConcreteError)` 与 `err(e := ConcreteError)` 动态胖指针双轨解构。

### 修改文件 (Modified - cpc 正式合并与 Release 发布)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 新增 `in_paren_pattern: usize` 状态计数，精确区分 Switch 分支顶层逗号（多模式 `case A, B =>`）与括号内部模式（`err(code, msg)` 元组模式）逗号，杜绝多元错误模式被误判为 `Pattern::Or`；
  - 扩展 `parse_error_type_rhs`：支持 `||` 运算符并允许在错误类型联合中直接使用 `error{A, B}` 或 `err{A, B}` 匿名集合；
  - 修复 `Token::ErrorKw` 解析：仅当小写 `err.` / `error.` 时替换为 `error` 命名空间，严格保留大写 `Err.` / `Error.` 作为标识符调用 `Err.init(...)`。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 统一映射多错误联合：全原生错误集合并发射为 Zig 零开销 `(Set1 || Set2)!T`；包含 `Err` 时发射为统一契约 `CpResult(T, Err)`；
  - 支持 `err(e := ConcreteError)` 与 `err(e: ConcreteError)` 双轨 Walrus / Colon 动态向下转型与实例绑定；
  - 增强 `Err.with_cause` 与 `downcast`：智能识别 Trait Object（`.ptr` 与 `.vtable`）并记录 `type_id`，安全解构真实堆实例；
  - 扩展 `collect_err_variants_for_type` 自动解析联合集合中包含的匿名 `error{...}` 变体，消除 switch 匹配歧义。
- [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc):
  - 正式发布 release 版本独立编译器二进制。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 补充 5 种函数错误签名规范（`T!`、`T!Set`、`T!S1 || S2`、`T!S1 || err{...}`、`T!S1 || Err`）；
  - 详述 Switch 模式匹配对 `err(e: T)` 与 `err(e := T)` 的双轨支持；
  - 完善第 8.2.5 节已完全实装的标量错误自动打包与 From 自动化提升机制规范。

---

## 2026-11-05 (结构体错误、类错误与复杂枚举载荷错误架构实装)

### 修改文件 (Modified - cpc 正式合并与 Release 发布)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 全量合入 `cpc_dev` 的结构体错误映射、类错误指针与 `cause` 挂载、枚举载荷解构匹配（元组/结构体/整包）、点推断返回优化与 Zig 0.17 运行时指针 `@hasField` 限制突破；
  - 完整保留 Bun agy 对库模式 `needs_actors = false`、可选数组/字符串指针映射 `?[]T` 与 `prop_map` 修复。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 全量合入结构体解构模式 `MyStructError { ... }` 语法支持。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 确认并合并 Bun agy 对 `@packed` / `@extern` 结构体豁免生成 `init` 的优化。
- [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc):
  - 重新编译 Release 版本（`cargo build --release`）并替换安装。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - `getCpcBin`: 支持通过 `.cap/cpc_bin` 安全读取本地影子调试编译器路径，使 `cap run` / `cap test` 无需额外 env 前缀直接调用开发版本。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 8.2.6 节《自定义结构体错误（Struct Errors）与类错误（Class Errors）深度对比与实战》；
  - 新增第 8.2.7 节《复杂枚举载荷错误（Enum Payload Errors）：定义、返回与多维解构匹配》。

### 新增文件 (Added)

- [`cap/test/test_struct_and_enum_payload_errors.cp`](file:///data/dev/cp/cap/test/test_struct_and_enum_payload_errors.cp):
  - 系统长期回归测试用例，涵盖 9 大测试场景：结构体错误直接解构匹配、结构体自动提升为统一 `Err`、类错误直接匹配、类错误通过 `: MyClassError` downcast 还原、类错误通过 `:= MyClassError` Walrus 绑定 downcast 还原、枚举标量载荷匹配 `.Text`、枚举元组载荷匹配 `.Move [x, y]`、枚举结构体属性精确解构 `.Compute { batch, priority }`、枚举结构体属性整包捕获 `.Compute { ...fields }`、以及枚举点推断返回错误与模式匹配。

---

## 2026-11-06 (多线程并发原语体系：`Mutex<T>`、`RwLock<T>` 与 `Atomic<T>` 实装)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 6.8.2(3) 节：精简 `Shared(value)` 构造语法，确立 `Shared(Mutex(data))` 与 `Shared(RwLock(data))` 内部可变性（Interior Mutability）标准范式；
  - 更新第 10.5 节：新增《并发线程同步原语：`Mutex<T>`、`RwLock<T>` 与 `Atomic<T>`》官方设计规范，对齐 TS 驼峰风格命名（`withLock`、`lock`、`tryLock`、`isLocked`、`withRead`、`withWrite`、`read`、`write`、`tryRead`、`tryWrite`、`isReadLocked`、`isWriteLocked`、`compareAndSwap`、`cas`、`load`、`store`、`swap`、`add`、`sub`、`inc`、`dec`、`and`、`or`、`xor`、`testAndSet`、`clear`）。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 新增 `Mutex<T>`、`RwLock<T>`、`Atomic<T>`、`LockGuard<T>`、`ReadGuard<T>`、`WriteGuard<T>` 类型参数化解析与 `type_to_str` 转换；
  - `parse_arrow_params` 适配首参包含 `*` 或 `mut`（如 `(*s) => ...`）的闭包签名；
  - `parse_ident_internal` 增加对 `Token::Dec` 与 `Token::Macro` 作为成员方法标识符（如 `atom.dec()`）的解析兼容；
  - 同步合入主干 `cpc/src/parser.rs` 的 `Expr::Unwrap`（`.?` 操作符）、`*const` 裸借用以及 `parse_postfix_type_modifiers` 语法支持。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - `ModuleFeatures` 扫描 `cap:sync`、`Mutex`、`RwLock`、`Atomic` 导入及方法调用，按需注入 `needs_sync` 运行时依赖；
  - 运行时代码生成层注入底层自旋锁 `NativeMutex`、读写锁 `NativeRwLock`、`LockGuard`、`ReadGuard`、`WriteGuard`、`CpMutex(T)`、`CpRwLock(T)`、`CpAtomic(T)` 与无载荷裸锁 `CpBareMutex`、`CpBareRwLock`；
  - 针对 `withLock`、`withRead`、`withWrite` 实现内联状态机展开，自动管理 `lock` 与 `defer unlock`；
  - `inline_closure_params` 隔离闭包局部变量，避免状态机 `ctx.param` 误捕获；
  - `infer_cp_type_with_mock` 支持内联闭包参数类型自动下推与返回值类型推断；
  - `CpAtomicConcreteType` 解决 Zig 0.17 下 `comptime_int` / `comptime_float` 无法直接传入 `std.atomic.Value` 的问题；
  - 自动解包守卫成员透明访问（`guard.count` 直接映射至 `guard.data.count`）；
  - 同步合入主干 `cpc/src/codegen.rs` 的 `Expr::Unwrap` 代码生成与数组原生容器方法判断。
- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs), [`cpc_dev/src/ast_visitor.rs`](file:///data/dev/cp/cpc_dev/src/ast_visitor.rs):
  - 同步合入主干 `Expr::Unwrap(Box<Expr>)` 语法节点及其遍历逻辑。

### 新增文件 (Added)

- [`cap/test/test_mutex_atomic.cp`](file:///data/dev/cp/cap/test/test_mutex_atomic.cp):
  - 长期回归测试用例，覆盖 7 大测试场景：
    1. `Mutex<T>`：`withLock` 闭包安全修改、`lock()` RAII 守卫访问、`tryLock()` 抢锁与 `isLocked()` 探测；
    2. 无载荷裸互斥锁 `Mutex()`：`lock()`、`unlock()`、`tryLock()`、`isLocked()`；
    3. `RwLock<T>`：`withRead` 只读访问、`withWrite` 独占写访问、`read()`、`write()`；
    4. 无载荷裸读写锁 `RwLock()`：`read()`、`write()`、`unlock()`、`unlockShared()`；
    5. `Atomic<i32>`：算术增减（`add`、`sub`、`inc`、`dec`）、原子位运算（`and`、`or`、`xor`）、原子交换（`swap`）与比较交换 CAS（`cas`、`compareAndSwap`）；
    6. `Atomic<bool>`：测试并置位 `testAndSet()` 与清除 `clear()`；
    7. `Shared(Mutex(T))`：内部可变性标准范式（只读句柄包装内部受锁保护的局部可变数据）。

### 文档更新 (Docs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 10.5.6 节《内存边界对比：为什么跨线程/跨协程必须用 `Shared<T>` 而不能用 `&T`？》；
  - 新增第 10.5.7 节《多线程数据保护机制全景对比（CP vs Rust vs Go vs Zig）》；
  - 新增第 10.5.8 节《工业级并发实战指南：5 大经典并发场景代码示例》（包含只读配置共享、高频可变写入、读写锁缓存、无锁原子计数器、跨资源裸锁协同）。

---

## 2026-11-06 (静态类与静态结构体 `static class` / `static struct` 实装与并发内存模型规范)

### 修改文件 (Modified)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 为 `ast::Class` 与 `ast::Field` 结构体新增 `#[serde(default)] pub is_static: bool` 标识。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 在 `parse_decl` 中将 `Token::Static` 后接 `Class` / `Struct` / `Ref` 导向 `parse_class(..., is_static = true)`；
  - 自动将类内字段与方法标记为静态；
  - 严格拦截 `static class` 声明 `extends` 继承；
  - 严格拦截 `static class` / `static struct` 方法声明 `this` / `_` / `*_` / `move _` 接收者；
  - 严格拦截声明 `init` 或 `release` 实例生命周期方法；
  - 兼容解析字段可选的前置 `let` 与 `const` 修饰符。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 新增 `SemaError::CannotInstantiateStatic(String, Span)`，编译期严格拦截对静态类与静态结构体的 `new` 实例化表达式；
  - 在 `expand_macros` 阶段豁免 `!cls.is_static`，彻底杜绝静态类自动注入带 `self: *Class` 的实例 `init` 构造函数；
  - 适配 mock 结构体初始化的 `is_static: false` 默认值。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 静态类生成时豁免 `_create` 堆分配器函数，字段直接发射为容器级 `pub var field: type = default;`（常量发射为 `pub const`）；
  - `infer_cp_type_with_mock` 支持静态类静态字段访问（`Expr::Member`）的类型推导；
  - `Expr::New` 与 `Expr::EnumInit` 生成阶段针对静态目标增加硬防御报错拦截。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 5.15 节《静态类与静态结构体（static class 与 static struct）》：全员自动静态、免除生命周期与实例负担、编译期五重拦截、以及 C#、Rust、Go、TS、Python 横向对比；
  - 新增第 7.6 节《Trait 静态契约与关联构造函数（Trait Static Contracts）》：支持无接收者静态工厂与泛型关联创建契约；
  - 新增第 7.7 节《现代系统语言的方法容器与数据解耦演进（CP vs Rust vs Zig vs TS）》：struct 默认 pub、class 默认 priv 原理，以及 Zig 与 Rust 废除继承的原因；
  - 新增第 10.5.9 节《并发与多线程内存模型权威问答（Concurrency & Memory Model FAQ）》：涵盖 `Shared<T>` clone 时机、原子引用计数增减与为 0 即刻释放、引用克隆 vs 深拷贝 `mut()` 代价、裸互斥锁 `Mutex()` 生命周期与死锁防范、编译期借用逃逸分析、`move` 物理零竞态保证、以及 Rust `Send`/`Sync` 极简结构化对比。

### 新增文件 (Added)

- [`cap/test/test_static_class.cp`](file:///data/dev/cp/cap/test/test_static_class.cp):
  - 长期回归测试用例，涵盖：
    1. `static class MathUtils` 纯静态工具类方法调用（`add`、`max`）与静态状态读写（`PI`、`call_count`）；
    2. `static struct StringHelper` 纯静态结构体常量（`TAG`）与辅助方法（`is_empty`、`format_pair`）。

---

## 2026-11-06 (正式主干合并与 Release 发布：并发原语 Mutex/RwLock/Atomic 与 static class/struct 合入 cpc 生产版)

### 修改文件 (Modified - cpc 正式合并与 Release 发布)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 全量合入 `is_static: bool` 字段至 `ast::Class` 与 `ast::Field`；
  - 完整保留主干 `Expr::Unwrap` 与 `ImportStmt.default_specifier`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 保持与主干 AST 遍历一致性。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 保留主干 `#?` 标识符规则并确保与沙盒词元一致。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 全量合入并发类型参数解析（`Mutex<T>`、`RwLock<T>`、`Atomic<T>`、Guard 等）；
  - 全量合入 `static class` / `static struct` 解析与四重防御性语法拦截；
  - 完整保留主干 `Expr::Unwrap`（`.?` 操作符）、`*const` 裸借用以及 `parse_postfix_type_modifiers` 统一后缀修饰。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 全量合入 `SemaError::CannotInstantiateStatic` 静态实例化拦截；
  - `expand_macros` 阶段对静态容器豁免自动注入实例 `init` 方法。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 全量合入并发运行时基座：`NativeMutex`、`NativeRwLock`、`LockGuard`、`ReadGuard`、`WriteGuard`、`CpMutex(T)`、`CpRwLock(T)`、`CpAtomic(T)`、`CpBareMutex`、`CpBareRwLock`；
  - 全量合入 `withLock` / `withRead` / `withWrite` 内联生成与闭包参数作用域隔离（`inline_closure_params`）；
  - 全量合入 `static class` / `static struct` 无堆容器代码生成与字段访问推导；
  - 完整保留主干对 `Expr::Unwrap` 的转译支持。
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs):
  - 保持与主干模块解析及编译入口一致。
- [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc):
  - 通过 `cargo build --release --manifest-path cpc/Cargo.toml` 重新生成正式 Release 二进制（耗时 35.85s）并发布更新。
- [`GEMINI.md`](file:///data/dev/cp/GEMINI.md):
  - 架构白皮书与双智能体并行规范全面升级同步。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 同步更新第 5.12.2 节（`struct` 默认 `pub`、`class` 默认 `priv`）、第 5.15 节（`static class` / `static struct` 规范与跨语言对比）、第 7.6 节（Trait 静态契约）、第 7.7 节（现代系统语言方法容器解耦哲学）、第 10.5.8 节（并发场景对称代码示例）与第 10.5.9 节（并发与内存模型 FAQ）。

---

## 2026-11-06 (C-FFI 原生声明、C 指针体系映射、常量泛型单态化、编译期反射与枚举内省实装)

### 修改文件 (Modified - cpc_dev 沙盒实现)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 新增 `Decl::TypeAlias(TypeAliasDecl)`，支持 `type Name = Target;` 别名声明；
  - 新增 `GenericParam` (`Type(Ident)` 与 `Const { ident, ty }`)；
  - `Class` 新增 `generic_params: Vec<GenericParam>`，支持常量泛型参数（如 `class App<const ssl: bool>`）；
  - `Expr` 新增 `If { span, test, cons, alt, is_comptime: bool }`，支持 `@if (cond) cons else alt` 编译期三元表达式；
  - `Type` 新增 `CStr`（0 结尾 C 字符串）、`CFn(Vec<Type>, Box<Type>)`（C 调用约定函数指针）、`ManyRawPtr(Box<Type>)`（未定长多项指针 `[*]T!`）。
- [`cpc_dev/src/lexer.rs`](file:///data/dev/cp/cpc_dev/src/lexer.rs):
  - 新增 `Token::TypeKw` (`type`) 与 `Token::CStr` (`cstr`) 词法标记。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - `parse_module` 与 `parse_module_item` 支持 `@c { ... }` 块级 C 外部函数声明批量展开；
  - `parse_decl` 支持 `type Name = Target;` 类型别名解析；
  - 新增 `parse_fn_type_sig`，支持 `@c fn(...) : Ret`、`(@c fn(...) : Ret)?` 及普通函数指针类型标注；
  - `parse_base_type` 支持 `[*]T!`（未定长多项指针）、`cstr` 词元与标识符、以及泛型尖括号 `<...>` 捕获；
  - `parse_primary_expr` 支持 `@if (cond) cons else alt`（支持可选 `=>`）、`string` 标识符表达式；
  - 调整类成员默认导出可见性：类成员（字段与方法）默认为 `pub`（兼容 TypeScript 规范，支持 `private` 显式私有标注），确保 `@hasDecl` 与外部探测正常运作。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 单项裸指针安全防御：`analyze_bin_expr` 严格拦截对单项裸指针 `*T!` 执行 `+` 或 `-` 的指针算术运算，提示：`Cannot perform pointer arithmetic on single-item pointer '*T!'; use '[*]T!' instead`；
  - `analyze_expr` 与 `infer_type` 支持 `Expr::If`（含编译期三元推导）；
  - `Type::ManyRawPtr` 与 `Type::CStr` 接入索引与切片类型推导；
  - 支持 `string.from(...)`、`reflect.has*`、`Enum.fromName`、`Enum(code)` 及 `Type::CFn` 调用的类型推导；
  - `expand_macros` 阶段豁免 `!cls.generic_params.is_empty()`，避免对常量泛型单态化模板注入实例堆构造函数。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 注入前导通用转换函数 `pub fn cp_string_from(val: anytype)`；
  - 结构体/类级 `@align(N)` 发射 `_align: [0]u8 align(N) = undefined,`，字段级 `@align(N)` 发射 `align(N)`；
  - `@c` 无函数体声明发射 `pub extern "c" fn ...;`（支持重命名别名），有函数体发射 `callconv(.c)`；
  - `reflect.hasMethod` / `reflect.hasField` / `reflect.has` 分别映射为 Zig `@hasDecl`、`@hasField`、`(@hasDecl or @hasField)`；
  - `string.from(x)` 统一直通发射为 `cp_string_from(x)`；
  - 枚举构造器 `Enum(code)` 发射为 `@as(Enum, @enumFromInt(code))`，枚举属性 `.name` 发射为 `@tagName(e)`，`.value` 发射为 `@intFromEnum(e)`，`Enum.fromName(str)` 发射为 `std.meta.stringToEnum(Enum, str)`；
  - `cstr` 属性 `.len` / `.length` 发射为 `std.mem.span(obj).len`；
  - `Type::CStr` 映射为 `[*:0]const u8`，`Type::ManyRawPtr` 映射为 `[*]T`，`Type::CFn` 映射为 `*const fn (...) callconv(.c) Ret`；
  - 常量泛型类展开为 Zig 单态化函数 `pub fn Class(comptime params) type { return struct { ... }; }`，方法接收者在泛型结构体内自适应使用 `@This()`；
  - `type_aliases` 跨上下文克隆至闭包状态机代码生成器（`lambda_gen`），在 `map_type` 中对泛型类别名自适应映射为值类型结构体；
  - `ModuleFeatures::scan_expr` 补全 `Expr::Arrow`、`Expr::Deref`、`Expr::BitNot`、`Expr::Unsafe`、`Expr::Comptime`，解决闭包内内联 `@not` 导致 `cp_not` 未声明问题；
  - `infer_cp_type_with_mock` 对 `Expr::Slice` 准确推导为 `Type::Array(elem)` 切片类型。

### 新增测试 (Added)

- [`cap/test/test_c_ffi_pointers.cp`](file:///data/dev/cp/cap/test/test_c_ffi_pointers.cp):
  - 覆盖全部 11 项新特性的端到端集成测试，100% 通过：
    1. `@align(16)` 结构体与字段内存物理对齐；
    2. `@c` 块与单个函数声明及 C ABI 导出；
    3. `VTable` 中带 `@c fn(...) : Ret` 的函数指针字段；
    4. 常量泛型类 `class App<const ssl: bool>` 与 `type AppTCP = App<false>;`；
    5. `cstr` 基础类型及其 `.len` 属性与 `string.from(cs)`；
    6. `[*]T!` 多项裸指针下标访问与切片；
    7. `reflect.hasMethod`、`reflect.hasField`、`reflect.has` 编译期反射探测；
    8. `@if` 编译期三元条件求值；
    9. 枚举 `.name` 与 `.value` 属性；
    10. `string.from(...)` 多态字符串转换；
    11. 枚举 `TaskState(2)` 构造器、`1 as TaskState` 转换及 `fromName("Idle")`。

---

## 2026-11-06 (生产目录 cpc/src 正式合并、LLVM Release 优化构建与全量端到端验证)

### 合并与修改文件 (Merged & Modified - 正式生产目录)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs):
  - 正式合入 `TypeAliasDecl`、`GenericParam`、`Class.generic_params`、`Expr::If` 以及 `Type::CStr`、`Type::CFn`、`Type::ManyRawPtr`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs):
  - 正式合入 `Expr::If` 变量引用遍历。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs):
  - 正式合入 `Type::ManyRawPtr`、`Type::CStr`、`Type::CFn` 的声明打印格式化。
- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs):
  - 正式合入 `Token::TypeKw` 与 `Token::CStr`。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 正式合入 `@c { ... }` 块级声明、`type` 类型别名解析、`@c fn(...) : Ret` 函数指针签名、`[*]T!` 与 `cstr` 基础类型解析、常量泛型参数解析、类成员默认可见性调整与 `@if` 编译期三元表达式。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 正式合入单项指针 `*T!` 算术拦截（`SemaError::InvalidPointerArithmetic`）、泛型类堆初始化豁免、`Expr::If` 语义分析与推导、`Type::ManyRawPtr` / `Type::CStr` 索引与切片推导、枚举内省与反射内建推导。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 正式合入 `@align(N)`、`@c`、常量泛型类单态化与 `@This()` 接收者自适应、`reflect.has*`、`string.from(...)`、枚举 `.name` / `.value` / 转换以及类型映射与切片推导。
- [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc):
  - 基于正式生产目录源码通过 `cargo build --release --manifest-path cpc/Cargo.toml` 完成 LLVM 优化构建（历时 35.36s）并发布至 `cap/bin/cpc`。

### 测试验证 (Verified)

- [`cap/test/test_c_ffi_pointers.cp`](file:///data/dev/cp/cap/test/test_c_ffi_pointers.cp): 端到端转译耗时 8ms，11/11 项测试 100% 通过；
- [`cap/test/test_static_class.cp`](file:///data/dev/cp/cap/test/test_static_class.cp): 端到端转译耗时 5ms，2/2 项测试 100% 通过；
- [`cap/test/test_bun_enums.cp`](file:///data/dev/cp/cap/test/test_bun_enums.cp): 端到端转译耗时 5ms，全绿通过。

---

## 2026-11-06 (控制流敏感类型智能收窄 Flow-Sensitive Type Narrowing 实装)

### 修改文件 (Modified - cpc_dev 调试沙盒)

- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 新增 `unwrap_optional_type`、`extract_not_null_vars` 与 `extract_is_null_vars` 提取辅助函数；
  - 增强 `analyze_stmt`（`Stmt::If` 与 `Stmt::While`）与 `analyze_expr`（`Expr::If` 与 `BinaryOp::LogicalAnd`）：在 `if (v != null)`、`if (null != v)`、`if (v != null && ...)` 分支作用域内，将可选类型 `T?` 自动收窄为不可空类型 `T`；在 `if (v == null) ... else { ... }` 分支作用域内自动在 else 块中收窄为 `T`；
  - 改造 `infer_type` 为 `&mut self`，使其具备流敏感作用域下的类型智能推断能力。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 引入 `narrowed_null_vars: HashSet<String>` 控制流收窄变量集合，并在 `generate_stmt`（`Stmt::If`、`Stmt::While`）中进行分支级动态跟踪与作用域维护；
  - `Expr::Ident`：对处于 `narrowed_null_vars` 中的标识符自动发射 `({}.?)` 表达式，实现零样板直接字段访问（`u.name`）、方法调用（`u.get_age()`）、原生算术运算与非可选形参传递；
  - 赋值安全防御：在 `Stmt::Expr(BinaryOp::Assign)` 与 `Expr::Bin(BinaryOp::Assign)` 中，生成赋值左值时临时撤除收窄，防止左值被错误解包为 `(ctx.count.?) = 100;`；若右值为 `null`，则永久解除该变量的收窄状态；
  - 显式解包幂等保护：在 `Expr::Unwrap` 中，若解包对象已被收窄，剥除收窄注入后直接输出单层 `.?`，杜绝 `(u.?).?` 二次解包；
  - `extract_local_vars_to_str`：补全对 `Stmt::If`、`Stmt::While` 与 `Stmt::TryCatch` 的递归遍历，确保闭包与测试 lambda 状态机中块级局部变量正确进入上下文结构体。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 增补第 6.7 节《借用与裸指针边界（unsafe 逃逸与指针转换体系）》，系统规范 6.7.1 ~ 6.7.4：`*T!` 与 `*T` 的本质区别、`const p` 与 `*const T!` 的变量常量性与指针只读性解耦、取裸指针三大语法（`*v!`、`&v as *T!`、`*v as *T!`）以及 Unsafe 裸指针完整类型转换矩阵；
  - 增补第 8.3 节《错误传播链与解包原语》及第 8.3.1 节，正式将强制显式解包 `.?`、可选安全链 `?.` 与空值合并 `??` 纳入对比规范表；
  - 增补第 9.3.3.1 节《控制流类型智能收窄 (Flow-Sensitive Type Narrowing: `if (v != null)`)》，详细规范正向非空收窄、反向 else 收窄、短路与收窄及赋值安全性。

### 测试强化与新增测试 (Test Enhanced & Added)

- [`cap/test/test_unsafe_pass.cp`](file:///data/dev/cp/cap/test/test_unsafe_pass.cp):
  - 扩展验证标量裸指针三大获取形式：`*num!`、`&num as *i32!`、`*num as *i32!`，全部 100% 验证通过。
- [`cap/test/test_if_null_narrowing.cp`](file:///data/dev/cp/cap/test/test_if_null_narrowing.cp):
  - 完整回归测试套件，覆盖对象流式收窄、字段/方法免解包调用、可变借用方法调用、基本类型可空收窄与原生运算、非可选传参、短路与组合、反向 `null != u` 语法、else 分支反向收窄以及嵌套 if 收窄，5/5 测试用例 100% 通过。

---

## 2026-11-06 (长上下文交接资产沉淀与 PROGRESS.md 编制)

### 新增与重构文档 (Added & Refactored Documentation)

- [`PROGRESS.md`](file:///data/dev/cp/PROGRESS.md):
  - 针对单会话超过 45,000 行交互的场景，编写高度浓缩的核心技术资产交接文档；
  - 完整梳理 CP 项目定位、GEMINI.md 双缓冲隔离机制（`cpc_dev` vs `cpc`）、流式智能收窄、解包原语体系、裸指针转换矩阵及当前待合并状态；
  - 提供详尽的接手 Agent 操作指引（Runbook）与快速验证指令。

---

## 2026-11-07 (uSockets 与 picohttpparser 底层 build.zig 架构落地与 CP 原生测试)

### 新增文件 (Added)

- [`build.zig`](file:///data/dev/cp/build.zig):
  - 基于 Zig 0.17 现代 Module 架构实现底层构建脚本；
  - 自动编译 `vendor/usockets` 与 `vendor/picohttpparser` 为静态库 `libusockets.a` 并发布 C 头文件至 `zig-out/include/`；
  - 提供 `-Dcp-entry` 一体化构建管线，将 CP 转译出的 Zig 状态机与 C 库内联编译为独立可执行文件（`server`）。
- [`vendor/usockets/src/cp_stubs.c`](file:///data/dev/cp/vendor/usockets/src/cp_stubs.c):
  - 为 Bun's fork of uSockets 提供轻量级独立运行适配桩层，补齐 Linux epoll_pwait2、锁及分发回调符号，消除 45 处链接未定义错误。
- [`cap/test/test_usockets_picohttp.cp`](file:///data/dev/cp/cap/test/test_usockets_picohttp.cp):
  - 端到端长期回归测试用例，覆盖 `picohttpparser` 纳秒级 HTTP 报文解析与 `uSockets` 原生事件循环创建与释放。

### 测试验证 (Verified)

- 执行 `zig build` 成功输出 `zig-out/lib/libusockets.a` 与对应头文件；
- 执行 `zig build -Dcp-entry=.cap/zig/cap/test/test_usockets_picohttp.zig run`：HTTP 解析与 uSockets 事件循环 100% 绿色通过。

---

## 2026-11-07 (深度重构 C 外部函数声明体系：全面实装 @c/@c("alias")、unsafe 裸指针体系与独立第 16 章规范)

### 修改文件 (Modified - cpc_dev 调试沙盒)

- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 增强 `parse_base_type` 中 `is_raw_ptr_bang` 匹配分支，加入 `Some(Token::Question)`，使 `*T!?`、`*any!?` 完美被识别为纯后缀可选裸指针，并经 `parse_postfix_type_modifiers` 映射为 `Type::Optional(RawPtr)`（底层对应 Zig `?*T` 与 `?*anyopaque`）。

### 修改与重构测试 (Refactored Test)

- [`cap/test/test_usockets_picohttp.cp`](file:///data/dev/cp/cap/test/test_usockets_picohttp.cp):
  - 彻底废除在顶层使用 `zig {}` 或旧式 `__zig__ {}` 声明外部 C 符号的原生反模式；
  - 采用 CP 原生 `@extern struct phr_header`（保证物理内存对齐）与 `@opaque struct us_loop_t`（C 不透明句柄）；
  - 全面使用 `@c("phr_parse_request") fn phr_parse_req(...)` 验证符号重命名（别名映射）特性；
  - 全面使用 `@c { ... }` 批量声明与 unsafe 裸指针（`cstr`、`*cstr!`、`*usize!`、`*i32!`、`[*]phr_header!`、`*any!?`、`*us_loop_t!?`）；
  - 函数体内内联代码块统一采用 `zig {}`（杜绝 `__zig__`）；
  - 运行 `cpc compile` 转译后经 `zig build -Dcp-entry=... run` 实测 100% 绿色全通。

### 规范手册升级 (Updated Specification)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 升级第 14.8 节：全面规范 C-FFI 原生声明铁律，明确 CP 统一使用 `zig {}` 且严禁 `__zig__`；确立“CP 中不应该使用 `zig {}` 声明 C 外部符号，而应该统一使用原生的 `@c`、`@c("alias")` 与 `@c { ... }` 原语”；
  - 移除原 14.14 节，消除在外部 C 符号声明中错误使用 `__zig__` 的历史示例；
  - 在第十五章后设立全新独立章节《十六、高性能 C 网络反应堆集成：`uSockets` 与 `picohttpparser` 原生架构 (High-Performance C Network Integration)》，详尽给出架构愿景、Zig 0.17 根目录 `build.zig` 现代模块管线、CP 原生 `@c` 导入规范与端到端实测验证。

---

## 2026-11-07 (纯正 CP 语法彻底废除测试函数体内联 zig {}，实现 100% 纯正 CP 外部 C-ABI 调用)

### 修改文件 (Modified - cpc_dev 调试沙盒)

- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增强 `can_coerce` 类型检查：
    - 在字面量匹配分支中支持 `Lit::Str` 直接兼容赋值给 `Type::CStr`，使 `let req: cstr = "..."` 与 `let mut method: cstr = ""` 获得编译期原生支持；
    - 在类型检查主流程中放行 `Type::String` 向 `Type::CStr` 的兼容传递（`matches!(target, Type::CStr) && matches!(src, Type::String)`）。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 在 `Expr::Cast` 代码生成中：
    - 支持未初始化栈缓冲区（`let mut header_buf: [16]phr_header = undefined;`）在指针强转（`header_buf as [*]phr_header!`）时解除 `uninitialized_vars` 限制，允许作为 C 函数的底层输出缓冲使用；
    - 增加 `matches!(src_ty, Some(Type::String))` 强转至 `is_target_ptr` 的代码生成分支，发射 `@as(target, @ptrCast(str.ptr))`。

### 重构正式回归测试用例 (Refactored Test)

- [`cap/test/test_usockets_picohttp.cp`](file:///data/dev/cp/cap/test/test_usockets_picohttp.cp):
  - 全面重构测试用例，彻底删除 `test_pico_http()` 与 `test_usockets_event_loop()` 中全部残留的内联 `zig {}` 块；
  - 100% 采用纯正 CP 语法：
    - 字符串指针与长度声明：`let req: cstr = "..."; let req_len: usize = req.len;`；
    - 裸指针取地址与传递：`*method!`、`*method_len!`、`*path!`、`*path_len!`、`*minor_version!`、`*num_headers!`；
    - 栈内存缓冲传递：`header_buf as [*]phr_header!`；
    - 零拷贝切片与格式化打印：`method[0..method_len]`、`path[0..path_len]`、`h0.name[0..h0.name_len]`、`h0.value[0..h0.value_len]` 直接配合 `@print` 输出；
    - 原生事件循环安全收窄解包：`let loop = us_create_loop(null, null, null, null, 0); if (loop != null) { us_loop_free(loop); }`。

### 测试验证 (Verified)

- 运行 `/data/dev/cp/cpc_dev/target/debug/cpc compile cap/test/test_usockets_picohttp.cp --output .cap/zig/cap/test/test_usockets_picohttp.zig` 编译成功；
- 运行 `zig build -Dcp-entry=.cap/zig/cap/test/test_usockets_picohttp.zig run`：
  - 成功纳秒级解析 HTTP 请求并精准打印 Method (`POST`)、Path (`/api/v1/agent/chat`)、HTTP/1.1 以及全部 3 个 Header 键值对；
  - 原生 uSockets 事件循环创建与释放 100% 绿灯；
  - 控制台输出 `ALL PURE CP USOCKETS & PICOHTTPPARSER TESTS PASSED!`。

---

## 2026-11-07 (cpc_dev 正式合并至正式版 cpc，Bun 迁移改动深度审核与 Release 发布)

### 审核与合并 (Audited & Merged - Bun Agent Modifications)

- 经与并行执行 Bun.js 迁移任务的智能体代码比对，全面审核其在 `cpc/src` 中的提交：
  1. [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs): `Check` 命令解析报错增加行列号与源码切片上下文定位预览，显著提升开发者诊断体验（审核结果：极其合理且完善）；
  2. [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs): `parse_ident_name` 支持 `Token::TypeKw => "type"` 作为常规字段或属性标识符解析（审核结果：兼容 JS/TS 常见对象模型，极其合理）；
  3. [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs): Pass 1 符号扫描中增加 `ModuleItem::Decl(Decl::TypeAlias)` 与 `ModuleItem::Import` 顶层符号预录入，支持后向声明引用（审核结果：极大增强模块解耦性，非常合理）；
  4. [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
     - `CpOptType` 声明无条件生成（`if true`），杜绝因特征扫描盲区导致编译期工具函数未定义（审核结果：极其稳健）；
     - `Expr::Catch` 中对 `@panic` 与 `@compileError` 做特殊处理，防止在类型注解包进 `break :blk @as(T, @panic(...))` 触发 Zig noreturn 语法错误（审核结果：极其关键且合理）；
     - `Expr::Catch` 外层输出防御性括号 `(({exp}) catch ...)`，隔离运算符优先级冲突（审核结果：极大增强鲁棒性）；
     - `Stmt::If` 解包捕获变量 `cap.sym` 引入 `saved_cap_local`，在退出 if 作用域时精准恢复或移除局部变量，杜绝局部变量作用域泄漏（审核结果：极其合理）。

### 正式编译器合并与构建发布 (Release Build & Deployment)

- 同步合并 `cpc_dev/src/` 与 `cpc/src/` 的双向功能增量，消除两个沙盒的全部差异；
- 成功编译正式 Release 优化版本 `cpc`：`cargo build --release --manifest-path /data/dev/cp/cpc/Cargo.toml`（单文件转译速度提升至 2~3ms）；
- 发布并更新可执行文件至 [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc)；
- 更新 [`.cap/cpc_bin`](file:///data/dev/cp/.cap/cpc_bin) 环境变量指回正式版，所有基于 `cap` 的编译命令无缝切换至最新发布版。

### 语言设计规范文档补全 (Updated Specification)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **新增第 4.6 节《流式类型智能收窄与 Fail-Fast 解包体系 (Flow-Sensitive Type Narrowing)》**：详尽定义正向条件收窄（`if (u != null)`）、复合条件短路收窄（`u != null && ...`）、负向早期卫语句收窄（`if (u == null) return;`）以及可选裸指针收窄（`*T!?` -> `*T!`）；
  - **新增第 16.5 节《纯 CP 零 Unsafe 架构：面向对象分层封装 (Zero-Unsafe Architecture)》**：总结 Bun 项目 `uws_sys` 的最佳实践，确立 C-ABI 映射层、局部蹦床适配层、面向对象 Safe 封装层与业务纯安全层的四层无 unsafe 架构体系；
  - **新增第 16.6 节《C 类型透明兼容与底层栈缓冲区安全规范》**：完整收录 `cstr` 与 `string` 零拷贝转换、未初始化定长栈输出缓冲区安全强转放行，以及后缀裸指针规范表。

### 自动化验证与测试全通 (Verified)

- [`cap/test/test_if_null_narrowing.cp`](file:///data/dev/cp/cap/test/test_if_null_narrowing.cp): 5/5 测试用例在正式版 `cpc` 下 100% 绿灯全通；
- [`cap/test/test_unsafe_pass.cp`](file:///data/dev/cp/cap/test/test_unsafe_pass.cp): 裸指针赋值与解引用测试 100% 绿灯全通；
- [`cap/test/test_usockets_picohttp.cp`](file:///data/dev/cp/cap/test/test_usockets_picohttp.cp): 纯 CP 原生 C 库调用端到端集成测试，转译耗时 2ms，HTTP 解析与 uSockets 事件循环 100% 绿色全通。

---

## 2026-11-07 (纯正 CP 语法净化、cap 一键直通链接 C 库升级与 Bun uws_sys 零 unsafe 架构解密)

### 语法净化与规范对齐 (Syntax Refinements)

- [`cap/test/test_usockets_picohttp.cp`](file:///data/dev/cp/cap/test/test_usockets_picohttp.cp):
  - 彻底移除全部非 CP 规范的 `let mut` 声明（Rust 残留习惯），回归标准纯正 CP 的 `let` 语法声明可变局部变量；
  - 确认 `undefined` 关键字（`[16]phr_header = undefined`）作为栈分配未初始化底层缓冲的原生合法性。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 16 章中的全部示例代码同步净化，彻底剔除 `mut` 关键字。

### 构建与运行系统增强 (Build Pipeline Enhancement)

- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - 在 `runFileWithOpt`（即 `cap <file.cp>`）执行链路上，自动检测当前项目下是否存在 C 静态库 `zig-out/lib/libusockets.a`；
  - 若存在，自动附加 `-Lzig-out/lib -lusockets` 链接器标志，彻底终结此前“先 cpc 转译、再 zig run”的两步流程；
  - 达成统一：直接执行 `/data/dev/cp/cap/bin/cap cap/test/test_usockets_picohttp.cp` 一键秒级转译、链接、运行全部搞定。

### 架构调研与资产复用 (Architecture Research & Bun uws_sys Asset Reuse)

- 深度拆解 `/data/dev/bun/src/uws_sys` 24 个 `.cp` 源码，全面解密其 100% 零 `unsafe` 架构：
  - 调用 `@c` 导入函数属于 CP 安全规范调用；
  - 裸指针作为不透明句柄在方法间以参数流转，避免非法解引用（Deref）；
  - 蹦床适配器（Trampoline）与面向对象 Class 守门人，向外暴露纯安全 API；
- 确立标准库引入路线：不全盘生搬硬套（避免 `bun` 内部私有命名空间耦合），而是基于其纯 CP 抽象，打造 CP 原生自主可控的 `cap:http` 与 `cap:ws` 引擎。

---

## 2026-11-07 深度完善与发布 (@c/@fix struct 内存排布、undefined 规范、零拷贝原理与标准库拓扑规划)

### 编译器增强与正式构建发布 (Compiler Enhancements & Release Deployment)

- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs) & [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 扩展结构体内存布局修饰宏，不仅支持 `@extern struct`，同时全面支持更直观的 `@c struct` 与 `@fix struct`；
  - 只要包含 `@c`、`@fix` 或 `@extern` 任意一个元属性，均精准转译为 Zig 的 `extern struct`，保持 C-ABI 内存排布对齐；
  - 编译构建正式 Release 版本并覆盖到 [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc)。

### 规范文档补全与架构拓扑规划 (Specification & Architecture Roadmap)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **新增第 3.3.3 节《`undefined` 关键字与未初始化内存语义》**：正式界定系统级 `undefined` 与 TypeScript `undefined` 的本质区别，明确其对齐 Zig `undefined` 并用于栈分配 C-FFI 输出缓冲区的规范与安全边界；
  - **更新第 16.3/16.4 节**：将结构体布局修饰符更新推荐为 `@c struct` / `@fix struct`；
  - **新增第 16.7 节《CP 标准库网络与密码学生态拓扑规划》**：彻底脱离 Bun 私有命名空间，规划自主可控的 `cap:crypto`、`cap:net`、`cap:http`、`cap:ws` 四大模块及其分层实现架构。

### 测试用例升级与全流程验证 (Tests Verified)

- [`cap/test/test_usockets_picohttp.cp`](file:///data/dev/cp/cap/test/test_usockets_picohttp.cp):
  - 升级为 `@c struct phr_header` 与 `@fix struct phr_fix_sample`；
  - 运行 `/data/dev/cp/cap/bin/cap cap/test/test_usockets_picohttp.cp` 一键运行秒级通过验证（转译 6ms，链接 646ms，测试 100% PASS）。

---

## 2026-11-07 标准库目录拓扑重构与解析器升级 (Lib Directory Flattening & Compiler Resolver)

### 标准库拓扑重构 (Standard Library Layout Flattened)

- 彻底清理移除了历史冗余目录 `lib/cap/`；
- 将原 `lib/cap/*` 的全部标准库文件（`actor.cp`, `events.cp`, `fs.cp`, `io.cp`, `json.cp`, `mem.cp`, `net.cp`, `parallel.cp`, `reflect.cp`, `regex.cp`, `sqlite.cp`, `test.cp`, `worker.cp` 等）直接平铺移入唯一官方标准库根目录 [`lib/`](file:///data/dev/cp/lib/)；
- 保留并完善子系统模块目录（如 `lib/crypto/`、`lib/http/`、`lib/net/`）。

### 编译器模块解析引擎升级 (Resolver Overhaul)

- [`cpc_dev/src/resolver.rs`](file:///data/dev/cp/cpc_dev/src/resolver.rs) & [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs):
  - `resolve_dependencies` 与 `resolve_imports` 中，直接剥离 `cap:` 前缀，优先通过 `include_paths` 在官方标准库根目录 `lib/` 下检索目标模块；
  - 增强 `resolve_file_or_dir`：若目标路径为目录，优先匹配检索目录入口（`index.d.cp`, `index.cp`, `main.d.cp`, `main.cp`），确保复合目录标准模块（如 `cap:http` 对应 `lib/http/index.cp`）秒级命中；若非目录则按单文件匹配 `lib/<name>.cp`；
  - 成功完成 Release 优化构建（耗时 36.44s），更新部署至 [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc)。

### 设计规范更新 (Specification Updated)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 2.5 节新增 1.1 小节《内置标准库 `cap:*` 协议映射规则》，正式规范 `cap:*` 直通 `lib/` 根目录单文件与复合目录入口的设计标准。

### 回归验证通过 (Verified)

- 新增 [`cap/test/test_lib_reorg.cp`](file:///data/dev/cp/cap/test/test_lib_reorg.cp)，验证 `import { print } from "cap:io"` 与 `import { json } from "cap:json"` 秒级命中并正常执行通过；
- 既有测试用例全量兼容通过。

---

## 2026-11-07 控制流守卫三剑客（||、??、? 发散控制流）、if/while强制括号规范与可选分号ASI体系

### 编译器增强 (cpc_dev 沙盒并行研发调试)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 为 `Expr` 引入 `Continue(Span)`、`Break(Span, Option<Box<Expr>>)`、`Return(Span, Option<Box<Expr>>)` 三种控制流发散表达式节点，将语句原语升格为表达式一等公民。
- [`cpc_dev/src/ast_visitor.rs`](file:///data/dev/cp/cpc_dev/src/ast_visitor.rs):
  - 适配 `Expr::Continue`、`Expr::Break`、`Expr::Return` 的引用变量扫描。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - **`if` 与 `while` 条件圆括号强制规范**：取消无括号宽容解析，当缺失 `(` 时，提供三段式 AI 友好自愈报错（含 `= suggestion: wrap the condition in parentheses: if (condition) { ... }`），彻底杜绝与全大写常量结构体字面量的语法歧义；
  - **`||`、`??`、`?` 全面支持发散控制流**：支持在 `||`、`??`、`?` 右侧书写 `continue`、`break`、`return [expr]`；
  - **可选分号（ASI）与换行安全边界防御**：在 `Parser` 中引入 `prev_token_end` 物理位置记录；判定 `return` 语句换行不吞入下一行表达式；换行后的首字符 `(` 严禁被解析为前一表达式的后缀调用（`CallExpr`），彻底消除省略分号造成的跨行粘连。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 在 `infer_type` 中为 `BinaryOp::LogicalOr` 与 `BinaryOp::LogicalAnd` 正式确立推导类型为 `Type::Bool`，完美放行右侧发散控制流。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 实现通用辅助方法 `build_ret_stmt_for_fallback`，根据当前函数返回类型（`ResultType`、`Optional` 或常规标量）自动生成精确的 Zig `return` 语句；
  - 在 `Stmt::Expr` 中特化 `cond || continue / break / return`，直接生成高效原生 Zig 语句 `if (!cond) action;`；
  - 在 `generate_expr` 中为 `||`、`??` 与 `?` 生成合法的块表达式与 Zig 内建 `orelse` / `catch` 发散跳转代码；
  - 在 `generate_expr` 结尾为 `Expr::Continue`、`Expr::Break`、`Expr::Return` 兜底生成代码。

### 语言设计规范手册更新 (Updated Specification)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **新增第 8.7 节《守卫控制流三剑客（The Guard Trinity）：`||`、`??` 与 `?` 的发散控制与解包》**：详尽定义三大维度的条件触发、解包行为与零开销映射表；
  - **新增第 8.8 节《控制流条件圆括号强制规范与 AI-First 智能自愈提示》**：明确 `if (cond)`、`while (cond)` 规范，解析二义性根除原理及智能体 0 秒自愈报错规则；
  - **新增第 8.9 节《行末分号可选（Optional Semicolons）与 ASI 换行断句规范》**：确认 100% 可选分号与基于换行的物理断句防御机制。

### 自动化测试全绿通过 (Tests Verified)

- [`cap/test/test_control_flow_coalesce.cp`](file:///data/dev/cp/cap/test/test_control_flow_coalesce.cp):
  - 覆盖验证 `?? continue`、`?? break`、`?? return`；
  - 覆盖验证 `? continue`、`? break`、`? return`；
  - 覆盖验证 `|| continue`、`|| break`、`|| return`；
  - 覆盖验证 `if (cond)` 与 `while (cond)`；
  - 通过 `CPC_BIN=/data/dev/cp/cpc_dev/target/debug/cpc /data/dev/cp/cap/bin/cap cap/test/test_control_flow_coalesce.cp` 编译运行，转译耗时 7ms，测试用例全部绿灯通过（输出 `ALL_CONTROL_FLOW_COALESCE_TESTS_PASSED`）；
  - 验证遗漏括号的 `if cond` 与 `while cond` 精确触发带 `suggestion` 的自愈报错。

---

## 2026-11-07 AI-First 智能可自愈诊断系统全景实装 (AI-Friendly Self-Healing Diagnostics & E-Code Architecture)

### 编译器增强 (cpc_dev 沙盒并行研发调试)

- [`cpc_dev/src/diagnostic.rs`](file:///data/dev/cp/cpc_dev/src/diagnostic.rs):
  - **结构化诊断引擎**：新增 `Diagnostic` 核心结构体，规范错误码（`code`）、主信息（`message`）、物理位置（`span`）、根因分析（`cause`）、规则释义（`help`）与修改建议（`suggestion`）；
  - **Rustc/TypeScript 级渲染器**：实现 `Diagnostic::render`，输出精确包含行号、列号、源码代码切片高亮（`^^^^`）、行前缀对齐及三段式自愈诊断的纯净文本；
  - **智能联想与编辑距离计算**：实装 `levenshtein_distance` 与 `find_closest_match` 算法，在未定义符号场景下智能探测当前作用域内的相近候选符号并输出 `did you mean '...'?`；
  - **内置标准库智能匹配**：收录 `STANDARD_BUILTIN_IMPORTS` 映射字典，当直接使用 `print`、`json`、`fs`、`spawn` 等官方符号时，自动输出具体的 `import { ... } from "cap:..."` 建议；
  - **借用检查 Panic 格式化**：实装 `from_compiler_error_msg`，将 Codegen 阶段的借用检查 panic 错误信息结构化还原为可自愈的 `Diagnostic`。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 为 `ParseError` 增加 `Diagnostic(Diagnostic)` 变体与 `to_diagnostic()` 转换方法；
  - 深度适配 TS 习惯写法自愈：
    - `Token::Interface` -> 输出 `E0102` 并建议替换为 `trait`；
    - `Token::Try` -> 输出 `E0103` 并建议改为 `?` 错误传播或行内 `catch` 表达式；
    - `Token::Mut` -> 输出 `E0104` 并说明 CP 中 `let` 默认可变，参数使用 `*param`；
    - `Token::Any` -> 输出 `E0105` 并建议使用具体类型或泛型 `<T>`；
    - 缺少圆括号 `LParen` -> 输出 `E0101` 并给出括号包裹建议。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 为 `SemaError` 增加 `to_diagnostic()` 转换实现，覆盖全量语义分析与借用错误：
    - `CannotMutateReadOnlyBorrow` -> 输出 `E0201` 并给出 `*param` 独占可变借用声明建议；
    - `MutableBorrowConflict` -> 输出 `E0202` 并给出借用作用域分离建议；
    - `UseOfMovedValue` -> 输出 `E0203` 并给出引用借用传参建议；
    - `ConstAssignment` -> 输出 `E0204` 并建议改为 `let` 声明；
    - `AwaitOutsideAsync` -> 输出 `E0205` 并建议函数标注 `async fn`；
    - `UndefinedSymbol` -> 输出 `E0206` 并附带 Levenshtein 拼写智能联想或标准库导入提示；
    - `TypeMismatch` -> 输出 `E0207` 并针对 `Optional`、`ErrorUnion`、`usize/i32` 给出解包与转换建议；
    - `ReflectNotEnabled` -> 输出 `E0208` 并建议添加 `@reflect` 属性；
    - `UnsafeRequired` -> 输出 `E0209` 并建议包裹 `unsafe { ... }`；
    - `MutateWhileBorrowedInFor` -> 输出 `E0210` 迭代器失效保护建议；
    - `PrivateAccess` -> 输出 `E0211` 访问权限与 `pub` 建议；
    - `CannotInstantiateStatic` -> 输出 `E0212` 静态命名空间调用建议；
    - `InvalidCast` -> 输出 `E0213` 类型解析器建议；
    - `DuplicateVarDecl` -> 输出 `E0214` 变量重声明建议。
- [`cpc_dev/src/main.rs`](file:///data/dev/cp/cpc_dev/src/main.rs):
  - 在 `main()` 开头设置静默 panic hook（`std::panic::set_hook`），消除 Rust 原生栈回溯干扰；
  - 使用 `std::panic::catch_unwind` 包装 `Codegen::generate_module`，捕获借用检查 panic 并将其转换为结构化 `Diagnostic` 渲染输出；
  - 统一接管 `ParseError`、`SemaError` 与 Codegen 错误，终端仅输出美观工整的 E-Code 三段式自愈诊断。

### 语言设计规范手册更新 (Updated Specification)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **新增第十七章《AI-First 智能可自愈诊断系统与 E-Code 规范体系》**：正式确立 CP 语言三段式自愈诊断标准（Error Title、Code Slice、`= cause:`、`= help:`、`= suggestion:`）与 E01xx / E02xx / E03xx 标准错误码全景速查矩阵。

### 自动化测试全绿通过 (Tests Verified)

- [`cap/test/test_ai_diagnostics.cp`](file:///data/dev/cp/cap/test/test_ai_diagnostics.cp):
  - 新增正式回归测试，覆盖条件括号校验与独占可变借用（`*buf`）传参调用；
  - 运行 `CPC_BIN=/data/dev/cp/cpc_dev/target/debug/cpc /data/dev/cp/cap/bin/cap cap/test/test_ai_diagnostics.cp`，转译耗时 6ms，链接并运行成功（输出 `ALL_AI_DIAGNOSTICS_COMPATIBILITY_TESTS_PASSED`）；
  - 全面测试验证了：条件缺少括号、修改只读参数、使用 `interface`、声明 `mut`、未导入 `print`、变量拼写错误 `addr_srt`、类型不匹配、常量重新赋值等场景，终端均精准打印出带 `suggestion` 的三段式自愈诊断。

---

## 2026-11-07 (Bun agy 修改审核与双向合并、C-ABI 兼容指针系统、发散控制流空合并与正式 Release 发布)

### 审核与合并总结 (Audit & Merge Summary)

- 经逐行审核，Bun agy 在 `cpc/src` 中对 C 指针 `[*c]T` / 哨兵指针 `[*:<sentinel>]T`、`const_vars` 常量保护、`c_*` 兼容类型与有符号位移安全检查的修改 100% 合理且完整；
- 双向合并完成：将沙盒 `cpc_dev` 的 AI 三段式自愈诊断、发散控制流（`??/||/? return/break/continue`）、可选分号 ASI 断句与条件强制括号规范，与 Bun agy 的指针系统完美融合。
- `cpc/src` 与 `cpc_dev/src` 达成 100% 毫无差异代码对齐。

### 新增文件 (Added)

- [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs): AI-First 三段式自愈诊断核心引擎，包含 Levenshtein 编辑距离拼写建议与标准库映射。
- [`cap/test/test_cptr_and_ai.cp`](file:///data/dev/cp/cap/test/test_cptr_and_ai.cp): 综合测试用例，覆盖 C-ABI 指针、可选提升、借用隐式转换与发散空合并。
- [`cap/test/test_control_flow_coalesce.cp`](file:///data/dev/cp/cap/test/test_control_flow_coalesce.cp): 守卫三剑客完整控制流测试。

### 修改文件 (Modified)

- [`cpc/src/lib.rs`](file:///data/dev/cp/cpc/src/lib.rs): 导出 `pub mod diagnostic;`。
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs): 集成静默 panic hook 与 catch_unwind 诊断渲染。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs): 新增 `Type::CPtr` 与 `Type::SentinelPtr`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs): 新增对发散表达式 `Continue/Break/Return` 的变量追踪。
- [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs): 导出 `[*c]T` 与 `[*:<sentinel>]T` 的 `.d.cp` 格式声明。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs): 支持 `[*c]T`、`[*:<sentinel>]T`、可选后置优先级提升 `Optional(CPtr(T))`、守卫三剑客语法与换行 ASI 断句。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs): `can_coerce` 支持 `BorrowType` 与 `RawPtr` 隐式转 `CPtr`；`types_eq` 支持 C 指针比较；`to_diagnostic` 覆盖全量 E02xx 诊断。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs): 支持发散控制流 Zig 代码生成、`build_ret_stmt_for_fallback` 辅助函数、`const_vars` 常量保护与 `[*:0]u8` 形参映射。
- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs): 模块解析阶段支持三段式诊断渲染。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md): 更新 17.3 节系统类型字典与发散控制流规范。

### 正式 Release 交付 (Release Artifacts)

- 构建完成 `cpc` 正式 release 二进制，耗时 35.73s，覆盖至 `/data/dev/cp/cap/bin/cpc`；
- 所有回归测试 100% 绿灯全通。

---

## 2026-11-07 (cpc_dev 沙盒实现 C-ABI 联合体 `@c union`、`switch` 范围放宽、`@if` 条件类型别名与带值块级表达式)

### 新增文件 (Added)

- [`cap/test/test_c_union_and_comptime.cp`](file:///data/dev/cp/cap/test/test_c_union_and_comptime.cp): 4 大底层系统特性全覆盖测试套件（`@c union`、标识符范围匹配模式、`@if` 平台条件别名、标签块带值跳出与匿名块表达式）。

### 修改文件 (Modified)

- [`cpc_dev/src/lexer.rs`](file:///data/dev/cp/cpc_dev/src/lexer.rs):
  - 新增 `Token::Union` (`union`) 词法关键字。
- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - `Class` 新增 `pub is_union: bool` 标识；
  - 新增 `RangeBound`（`Int(i64)`, `Ident(Ident)`, `NegIdent(Ident)`），扩展 `Pattern::Range { start: RangeBound, end: RangeBound, inclusive: bool }`；
  - `Expr` 新增 `LabeledBreak(Span, Ident, Option<Box<Expr>>)` 与 `LabeledBlock { label: Option<Ident>, block: BlockStmt }`；
  - `Type` 新增 `Conditional(Box<Expr>, Box<Type>, Box<Type>)` 表达编译期条件类型别名；
  - 适配 `has_pipe_placeholder`。
- [`cpc_dev/src/ast_visitor.rs`](file:///data/dev/cp/cpc_dev/src/ast_visitor.rs):
  - `find_referenced_vars_in_expr` 支持 `LabeledBreak` 与 `LabeledBlock` 的变量引用收集。
- [`cpc_dev/src/dcp.rs`](file:///data/dev/cp/cpc_dev/src/dcp.rs):
  - `Decl::Class` 支持输出 `union` 声明，`format_type` 支持 `Type::Conditional`。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - `parse_decl` 支持 `union` / `@c union` / `@packed union`；
  - `parse_single_pattern` 与 `try_parse_range_bound` 放宽端点限制，支持常量标识符、负数常量标识符及 `...` / `..=` / `..` 符号；
  - `parse_type` 支持 `@if (cond) TypeA else TypeB` 平台条件类型别名；
  - `parse_stmt` 支持 `break :label [val];` 与 `break val;`；
  - `parse_primary_expr` 支持标签块表达式 `label: { ... }` 与匿名块表达式 `({ ... })`；
  - 完善 `left_span` 与 `right_span`。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 屏蔽 union 的成员自动 `init` 构造与堆分配方法注入；
  - 新增 `resolve_type` 实现类型别名递归自动解开；
  - `can_coerce`、`types_eq`、`is_int_type`、`int_fits_in_type` 全面支持 `Type::Conditional` 与类型别名展开；
  - `infer_type` 与 `analyze_expr` 适配 `LabeledBlock` 与 `LabeledBreak`。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - `struct_kind` 将 `is_union` 结构体转译为 Zig 原生 `extern union` / `packed union` / `union`，跳过 `_create`；
  - `generate_pattern_match` 与 `format_simple_pat` 适配 `RangeBound`；
  - `Codegen` 新增 `current_block_label` 追踪，精准将块内带值跳出转换为 Zig 原生 `break :label val;`；
  - `Stmt::Expr` 中对 `Expr::LabeledBreak` / `Expr::Break` 消除 `_ = ` 冗余包装；
  - `map_type` 配合 `expr_to_zig_pure` 输出 `if (cond) TypeA else TypeB`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增 17.4 节《底层系统级互操作与高级表达式扩展规范》，完整详述四项扩展的语法规则与设计理念。

### 自动化测试验证 (Tests Verified)

- 运行 `CPC_BIN=/data/dev/cp/cpc_dev/target/debug/cpc /data/dev/cp/cap/bin/cap test cap/test/test_c_union_and_comptime.cp`，607ms 通过全部断言。
- 核心回归测试 `test_all_libs.cp`、`test_cptr_and_ai.cp`、`test_bun_features.cp` 100% 绿灯全通。

### 正式主干合并与 Release 交付 (Mainline Merge & Official Release Delivery)

- **代码同步**：将沙盒 `cpc_dev/src/` 中的底层系统特性（`@c union`、`switch` 范围放宽、`@if` 平台条件别名、带值块表达式、深层命名空间类型注解）全量同步至正式主干 `cpc/src/`，代码对齐率 100%；
- **代码规范**：运行 `cargo fmt --manifest-path cpc/Cargo.toml` 与 `cargo check --manifest-path cpc/Cargo.toml` 格式化并验证零错误；
- **构建交付**：执行 `cargo build --release --manifest-path cpc/Cargo.toml` 完成 LLVM 深度优化，二进制覆盖安装至 `/data/dev/cp/cap/bin/cpc`；
- **全量测试**：使用正式版 `cap/bin/cpc` 重新验证全部回归测试套件（`test_c_union_and_comptime.cp`、`test_all_libs.cp`、`test_cptr_and_ai.cp`），100% 绿灯全通。

---

## 2026-11-07 (cpc_dev 沙盒实现 C 回调跳板工厂与局部函数单态化 C-Trampoline)

### 新增文件 (Added)

- [`cap/test/test_c_trampoline.cp`](file:///data/dev/cp/cap/test/test_c_trampoline.cp): C 回调跳板（C-Trampoline）、泛型包装工厂、单态化 `callconv(.c)` 局部函数与弱类型上下文转换综合验证测试。

### 修改文件 (Modified)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - `Stmt` 枚举新增 `Func(Box<Function>)` 变体以支持局部作用域内声明静态函数与跳板函数。
- [`cpc_dev/src/ast_visitor.rs`](file:///data/dev/cp/cpc_dev/src/ast_visitor.rs):
  - `find_referenced_vars_in_stmt` 与 `get_declared_vars_in_block` 完整适配 `Stmt::Func`，支持嵌套函数作用域的变量引用递归扫描与声明提取。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - `parse_params` 支持形参前置 `comptime` 及 `@comptime` 注解标记（`Param.is_comptime = true`）；
  - `parse_base_type` 支持 `Type: type` 映射为 `Type::Ref("type")`，并新增 `*!` 与 `*!?` / `[*]!` 通用裸指针极简缩写（自动映射基础类型为 `Type::Any`）；
  - `parse_var_decl` 遇到 `mut` 关键字立即触发 `E0104` 三段式自愈诊断拦截，提示 CP 中 `let` 默认可变；
  - `parse_stmt` 支持在函数局部块内解析 `@c fn` 与普通 `fn` 语句并包装为 `Stmt::Func`；
  - 函数类型签名 `parse_fn_type_sig` 全面支持省略参数名（如 `@c fn (*!?, i32, i32, *!?)`）及缺省返回类型直接省略 `: void`。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - `analyze_stmt` 递归遍历并类型检查局部 `Stmt::Func`；
  - `infer_type` 在 `Expr::Call` 中支持 `Type::Auto`（如 `comptime callback: anytype`）与类型别名展开，将其视为一等可调用函数并放行。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - `Expr::Cast`（`as` 运算符）扩展识别 `Type::BorrowType`（`*Type`），强转指针时自动安全生成底层 `@as(Target, @ptrCast(@alignCast(source)))`；
  - 新增 `local_trampolines: HashSet<String>` 跟踪局部 C-ABI 跳板符号；
  - 遇到局部 `@c fn` 时，自动生成单态化 Zig 静态包装结构体 `const __cp_trampoline_{name} = struct { pub fn {name}(...) callconv(.c) ... { ... } };`；
  - 遇到取地址操作 `&handle` 时，自动提升并转译为 `&__cp_trampoline_{name}.{name}`；
  - 在局部变量生命周期逃逸分析与 `return` 检查中，对 `local_trampolines` 中的符号豁免局部逃逸拦截；
  - 升级 `block_contains_ident` 接入 `ast_visitor::find_referenced_vars_in_block`，精准分析局部函数参数使用情况，杜绝 Zig 0.17 的 `pointless discard of function parameter` 编译报错；
  - 状态机内部对局部函数指针的调用精准转译为 `ctx.{ident.sym}(...)`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 17.9 节《C-ABI 回调跳板工厂与局部函数单态化架构规范》，详细记录 C-Trampoline 原理、局部 `@c fn` 提升、`@comptime` 参数、极简指针 `*!?`、无参名函数签名及规范 `as` 运算符。

### 自动化测试验证 (Tests Verified)

- 运行 `env CPC_BIN=/data/dev/cp/cpc_dev/target/debug/cpc cap cap/test/test_c_trampoline.cp`，6ms 转译、596ms 链接运行，断言 100% 通过（Status: 200, Timeouts: 15, C-Trampoline test passed!）；
- 正向验证：`let` 声明默认可变无需 `mut`、`let cb = ...` 自动推断、`as *Type` 自动生成对齐指针转换、`*!?` 极简指针、`&handle` 静态 C 函数指针只读借用、`@c fn (*!?, i32, i32, *!?)` 省略参数名与 `: void`、`@comptime` 参数修饰全部验证通过；
- 反向自愈验证：故意书写 `let mut x = ...` 精准触发 `error[E0104]: 'mut' keyword is not used for local variable declarations` 三段式自愈诊断拦截；
- 核心回归测试用例 `cap/test/test_bun_enums.cp` 绿灯通过。

---

## 2026-11-07 (阶段续：取裸指针语法架构裁定与 C 跳板编译期全链路剖析)

### 文档与设计规范 (Documentation & Specs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 17.9 节详述 C-ABI 回调跳板工厂与局部函数单态化规范，明晰 `pub type ares_host_callback = @c fn (*!?, i32, i32, *!?);`、`@comptime` 参数修饰、`let raw_ctx = &my_ctx as *!?;` 及 `this_ptr = ctx.? as *Type;` 现代标准范式；
- [`DEVLOG.md`](file:///data/dev/cp/DEVLOG.md):
  - 记录 `*expr!` 历史代码（`parser.rs` 4568-4581 行）考古溯源，深度评估其与 Deref 符号倒置、后缀 `!` 二义性冲突及违反显式类型擦除安全原则等 4 大缺陷，裁定确立 `&expr as *!?` 为官方唯一标准语法；
  - 详细剖析 `cpc` 编译器编译期识别局部 `@c fn`、Zig 匿名结构体提升（Struct Lifting）、外层单态化、符号重定向及借用检查静态豁免的全链路机制。

---

## 2026-11-07 (阶段续：CP 裸指针语法镜像同构全套实现与双轨支持)

### 修改文件 (Modified)

- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 在 `Token::Star`（可变借用）与 `Token::Ampersand`（只读借用）表达式解析中，全面支持 `!`、`!?` 与 `?!` 后缀提取裸指针语法糖；
  - `*expr!` 自动构造可变 C 裸指针（`*!`，即 `*anyopaque`）；
  - `*expr!?` 自动构造可选可变 C 裸指针（`*!?`，即 `?*anyopaque`）；
  - `&expr!` 自动构造只读 C 裸指针（`*const anyopaque`）；
  - `&expr!?` 自动构造可选只读 C 裸指针（`?*const anyopaque`）；
  - 与类型系统（`*!`、`*!?`）及显式强转（`*my_ctx as *!?` / `&my_ctx as *!?`）构成 100% 镜像同构与双轨互通。
- [`cap/test/test_c_trampoline.cp`](file:///data/dev/cp/cap/test/test_c_trampoline.cp):
  - 增加四种形式的完整断言测试（`*my_ctx!?`、`&my_ctx!?`、`*my_ctx as *!?`、`&my_ctx as *!?`），100% 绿灯全通。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 17.9 节升级为《裸指针双轨语法：原生 as 强转与类型镜像同构取指针语法糖》，明晰类型与表达式在 `*`、`!`、`!?` 上的对称映射及零二义性保证。

---

## 2026-11-08 (顶层原生声明放宽、透明 Zig 模块默认导入、上下文软关键字与 Release 编译验证)

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs) & [`cpc_dev/src/lexer.rs`](file:///data/dev/cp/cpc_dev/src/lexer.rs):
  - 移除了词法层对 `and`、`or`、`orelse` 的硬编码 Token 替换，彻底规避破坏 `obj.and()`、`obj.or()` 等正常属性与方法调用的问题；
  - 保持 `Token::Let`, `Token::Var`, `Token::Const`, `Token::ThreadLocal` 等顶层声明关键字。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) & [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 实现**上下文软关键字 (Contextual Soft Keywords)** 解析机制：在二元表达式与后缀操作符判定中识别 `and`、`or` 与 `orelse`，而在成员调用/属性访问上下文中保留标识符语义；
  - 支持带哨兵切片解析 `[start..end :sentinel]`；
  - 支持 `import Name from "./path.zig"` 默认规范导入与顶层 `const`/`var` 命名空间属性级联绑定。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) & [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 支持带哨兵切片底层代码生成；
  - 增加 Zig 关键字自动转义：方法与属性如果为 Zig 保留字（如 `and`, `or`, `orelse`），底层转译自动输出 `@"and"`、`@"or"` 转义序列；
  - 增加顶层 `std` 重复导入安全去重机制，防止 Zig 编译器报错 `duplicate struct member name 'std'`；
  - 完善 `(exp catch unreachable)` 紧凑单态发射。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) & [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - `VarDecl` 增加 `is_threadlocal` 与 `is_pub` 字段；
  - `SliceExpr` 增加 `sentinel: Option<Box<Expr>>` 字段。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) & [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 允许切片、空元组与目标数组类型的隐式借用转换，放宽 `unreachable` 标识符校验。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 17.12 节《顶层原生声明、透明 Zig 互操作与上下文软关键字规范》，详细记录顶层免 `zig {}` 声明规范、默认导入语法、软关键字与哨兵切片语义。
- [`cap/bin/cpc`](file:///data/dev/cp/cap/bin/cpc):
  - 使用 `cargo build --release` 编译并同步更新了正式 Release 版编译器二进制。

### 自动化测试验证 (Tests Verified)

- 新增回归测试用例 [`cap/test/test_bun_top_level_import.cp`](file:///data/dev/cp/cap/test/test_bun_top_level_import.cp) 与辅助模块 [`cap/test/mock_sub_module.zig`](file:///data/dev/cp/cap/test/mock_sub_module.zig)；
- 覆盖验证：
  1. 透明引入外部 `.zig` 模块；
  2. 顶层常量与级联命名空间（`const api = MockBun.schema.api;`）；
  3. 深层类型别名（`pub type DirInfo = SubModule.DirInfo;`）；
  4. 顶层可变状态（`var global_counter: i32 = 100;`）；
  5. 上下文软关键字 `and`、`or` 逻辑运算及 `orelse` 空值兜底；
  6. 隔离验证 `LogicCalculator` 的 `and`/`or` 成员方法调用与 Zig 保留字安全转义；
  7. 带哨兵的切片（`buf[0..3 :0]`）。
- 运行 `./cap/bin/cap cap/test/test_bun_top_level_import.cp`（Release 版 `cpc` 全链路从零转译并在 687ms 内完成链接），断言 100% 绿灯全通；
- 回归验证 [`cap/test/test_c_trampoline.cp`](file:///data/dev/cp/cap/test/test_c_trampoline.cp) 转译 3ms 绿灯通过，零功能劣化。

---

## 2026-11-08 (标记块穿透多层循环赋值、cstr/cstring 一等公民与越界防护规范沉淀、文档示例全面纠偏)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **第 17.4.4 节《带值块级作用域表达式与标记块 (Labeled Block Expressions)》大幅扩充**：
    - 补充使用具名标记块（`search: { ... break :search col; }`）一击穿透多层嵌套循环并直接赋值给外部 `const` 常量的标准范例；
    - 深入阐述标记块赋值的两大工程与架构价值：① 从根源确立 `const` 不可变性保障（Immutable by Construction）；② 彻底消除中间计算与循环变量污染外层上下文（Zero Scope Pollution）。
  - **第 6.7.1.1 节升级为《`cstr` / `cstring` 一等公民规范与内存越界防护机制》**：
    - 详尽剖析 `cstring`（带 0 哨兵切片 `[:0]const u8` / `[:0]u8`，Fat Pointer）与 `cstr`（零结尾连续裸指针 `[*:0]const u8` / `[*:0]u8`，Thin Pointer）的双层抽象与一等公民地位；
    - 明确脱离 `unsafe {}` 限制的机制、方法属性全景（`.len` O(1) 瞬时获取长度、`.ptr` 获取 C 原生指针、下标读写、切片重构、`string.from` 零拷贝转换）；
    - 深度剖析下标越界写入的双重防御体系：编译期常量边界分析拦截 + 运行期在 Debug 与 ReleaseSafe 模式下自动注入强制安全边界检查（发生物理写入前即刻抛出确定性安全 panic），从物理上彻底杜绝像 C/C++ 那样的缓冲区溢出（Buffer Overflow）与静默内存破坏。
  - **全文代码示例系统性审查与纠偏**：
    - 彻底清理遗留的 19 处 `let mut` / `mut` 声明，统一重构为标准 `let`（默认可变）；
    - 修正类字段中废弃的 `: any` 弱类型声明为通用非类型化裸指针 `*any!`；
    - 修正第 8.4 节废弃的传统 `try { ... } catch` 异常块为现代行内表达式级 `catch` 与 3-State 错误传播符 `?`；
    - 修正第 11.4 节装饰器内的 `for i in 0..times` 为 CP 标准 `for i of 0..times`；
    - 修正第 17 章 5 处 Zig 风格管道捕获 `|v|` 与 `try` 残留为 CP 现代 `:=` 海象操作符与 `?` 错误传播；
    - 修正 7 处废弃的前置问号类型 `?T` 为标准纯后缀形式 `T?`；
    - 修正局部变量 `var` 残留为 `let`。

---

## 2026-11-08 (控制流强制圆括号铁律加固 E0101 AI 自愈诊断、cstr 三重安全防御网与 Fail-Fast Panic 架构释疑)

### 修改文件 (Modified)

- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 重构 `parse_for_stmt_with_comptime`，强制要求 `for` 关键字后必须紧跟圆括号 `(`，未带圆括号时立即触发 `ParseError::MissingParenthesesForControlFlow("for")` 并映射为标准的 AI 自愈提示 `error[E0101]`；
  - 迭代表达式末尾强制匹配闭合 `)`，实现 `if`、`while`、`for` 语法与 AST 结构的彻底无二义性统一。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - **第 8.8 节升级为《控制流条件与循环迭代圆括号强制铁律（The Parentheses Iron Rule）与 AI-First 智能自愈提示》**：
    - 正式明确 `if (...)`、`while (...)` 以及 `for (...)` 必须强制外包圆括号，展示三段式自愈诊断 `error[E0101]`；
    - 阐述其消除与结构体实例化字面量及模式匹配产生 AST 解析二义性的深层原理。
  - **第 6.7.1.1 节升级为《越界写入与三重安全防御网（The Triple Safety Guardrails）》**：
    - 完整沉淀用户构想的三重防御网：① 编译期已知边界检查（Comptime Bounds Check）；② 切片优先惯用法（Slice-First）；③ 无界裸指针修改强制隔离进 `unsafe {}` 作用域；
    - 新增第 5 小节《Fail-Fast Panic 的本质作用与高可用自愈架构（Fail-Fast & Crash-Proof Architecture）》，深度剖析 Panic vs Exception 的本质区别、为什么绝不能通过 Try/Catch 捕获越界让程序带病运行，以及 CP-Nest 如何通过 Actor 沙箱物理隔离与集群监控树（Supervisor Tree）在微秒级实现 99.999% 高可用自愈。
  - **全文存量代码示例系统性排查与修复**：
    - 修复全篇 16 处无括号 `for` 循环示例与借用符号表格（如 `for (i of 0..1000)`、`for (i of 0..256)`、`@for (i of 0..5)`、`for (n of stream)`、`for (i of 0..times)`、`for (f of info.fields)`、`for (n of deque)`、`for (x of list)` 等）；
    - 修复全篇 19 处无括号 `if` / `@if` 条件示例（如 `if (a > b)`、`if (n <= 1)`、`if (flag == 1)`、`if (path == "missing.txt")`、`if (id < 0)`、`if (emailRegex.test(...))`、`@if (true)` 等）。

### 新增文件 (Added)

- [`cap/test/test_for_paren_fail.cp`](file:///data/dev/cp/cap/test/test_for_paren_fail.cp): 反向测试用例，验证无括号 `for item of list` 被编译器精准拦截并输出 `error[E0101]` 三段式 AI 建议。
- [`cap/test/test_for_paren_success.cp`](file:///data/dev/cp/cap/test/test_for_paren_success.cp): 正向测试用例，验证标准 `for (i of 0..10)` 语法在 5ms 内完成转译、编译链接并正确执行输出 45。

---

## 2026-11-08 (cstr / cstring 编译期边界防御、Safe 域裸指针修改拦截与 AI-First 自愈诊断实装)

### 修改文件 (Modified - cpc_dev 沙盒实现)

- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - **静态边界与常量求值跟踪**：在 `Analyzer` 中实装作用域感知的 `known_lengths_stack`（定长缓冲区物理长度）与 `const_int_vars_stack`（常量整数求值）；
  - **第一重防御（编译期静态越界拦截）**：对已知定长缓冲区的常量下标索引，在语义分析期进行边界检查，越界时精准阻断并抛出 `error[E0217]`（带 `cause`、`help` 和 `suggestion`）；对负索引触发 `error[E0218]`；
  - **第三重防御（Safe 域无界裸指针修改拦截）**：对来源无界或未知长度的裸指针 `[*]T!` / `cstr` 下标写入操作，在 Safe 域强制拦截并触发 `error[E0209]`，强制要求隔离进 `unsafe {}` 作用域或先进行安全切片；
  - **类型兼容打通**：在 `can_coerce` 中全面放行 `cstring` / `cstr` 与 `String`、`BorrowType`、`ManyRawPtr` 的安全隐式兼容；
  - **主干并发代码合并**：无冲突合并 `_ = ...` 表达式赋值忽略与泛型参数类内作用域注册。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 支持从可变数组切片表达式准确推导为可变切片类型 `[]u8`（避免被误标为 `[]const u8`）；
  - 为 `ManyRawPtr` 赋值单项借用指针时自动生成 Zig 原生 `@ptrCast`；
  - 同步合并主干解构切片辅助函数 `start_idx` 与 `anytype` 符号映射。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 同步合并主干关于块级展开闭合括号计数判定。

### 新增测试 (Added)

- [`cap/test/test_bounds_fail.cp`](file:///data/dev/cp/cap/test/test_bounds_fail.cp): 反向拦截测试 1，验证已知长度定长缓冲区越界写入（`s[10] = 99`）被静态拦截并输出三段式 `error[E0217]`。
- [`cap/test/test_raw_ptr_fail.cp`](file:///data/dev/cp/cap/test/test_raw_ptr_fail.cp): 反向拦截测试 2，验证 Safe 域下无界裸指针写入（`ptr[10] = 99`）被严格拦截并输出三段式 `error[E0209]`。
- [`cap/test/test_cstr_bounds_safe.cp`](file:///data/dev/cp/cap/test/test_cstr_bounds_safe.cp): 正向测试，验证合法切片修改、Safe 作用域只读访问与显式 `unsafe {}` 写入全部正常编译并运行通过。

---

## 2026-11-08 (Bun AGY 指针捕获解包合并、状态机块级局部变量解引用修复与端到端测试)

### 修改文件 (Modified - cpc_dev 沙盒实现)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - `IfStmt` 与 `WhileStmt` 引入 `is_ptr_capture: bool`，建立指针借用解包的语法树标识。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 在 `parse_if_stmt` 与 `parse_while_stmt` 中支持海象指针语法（`if (*val := opt)` / `if (mut val := opt)`）与管道指针语法（`if (opt) |*val|` / `if (opt) |mut val|`）；
  - 完善大括号与圆括号多层嵌套平衡判定。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 将指针捕获变量的符号种类设定为 `VarDeclKind::Let`，放行就地赋值修改；
  - 豁免 `_ = ...` 忽略赋值语句的常量限制；
  - 类方法内自动引入类的常量/类型泛型参数。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - **核心 Bug 修复与架构增强**：引入 `block_locals` 集合，防止 CPS 状态机将控制流解包的局部变量误转译为 `ctx.val` 导致结构体字段缺失；
  - **指针解引用透明桥接**：在指针解包作用域内自动维护 `mut_borrow_loop_vars`，将对解包变量的读写透明桥接为 Zig 必须的解引用赋值 `(val.*) = ...`；
  - 大写常量变量自动登记至 `value_classes`；
  - 函数返回类型不匹配导致无法返回 null 的 panic 信息增强，输出当前函数名与返回类型。

### 新增测试 (Added)

- [`cap/test/test_ptr_capture.cp`](file:///data/dev/cp/cap/test/test_ptr_capture.cp): 端到端验证海象语法指针解包就地修改、管道语法指针解包就地修改，以及 `_ = ...` 丢弃赋值，测试 100% 成功。

---

## 2026-11-08 (C-FFI 头文件极简内联吸纳、AI-First 结构化 JSON 诊断自愈与极简 own 关键字实装)

### 修改文件 (Modified - cpc_dev)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 在 `Decl` 枚举中新增 `CInclude(CIncludeDecl)` 变体与 `CIncludeDecl` 结构体，支持记录外部 C 头文件内联声明。
- [`cpc_dev/src/lexer.rs`](file:///data/dev/cp/cpc_dev/src/lexer.rs):
  - 新增 `#[token("own")] Own` 词法标记，确立 3 字母独占所有权关键字。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 在 `@c { ... }` 块解析中全面支持 C 头文件字符串字面量（如 `"stdio.h"`），支持逗号与分号分隔，并支持与自定义函数原型声明混合书写；
  - 在 `parse_param` 与 `parse_method_sig` 中全面支持 `own param: Type`；
  - 在 `parse_expr` 中支持 `own expr` 一元表达式所有权转移。
- [`cpc_dev/src/diagnostic.rs`](file:///data/dev/cp/cpc_dev/src/diagnostic.rs):
  - 实现 `DiagnosticJson`、`DiagnosticReport` 与 `DiagnosticEdit` 序列化结构；
  - 实现基于规则与 span 计算的智能自愈代码编辑引擎（针对缺少括号、const 重新赋值、参数借用等错误输出精确字符偏移与插入替换建议）。
- [`cpc_dev/src/main.rs`](file:///data/dev/cp/cpc_dev/src/main.rs):
  - 为 `Commands::Check` 增加 `--json` 命令行参数支持，打通 CLI 机器自愈通道。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 遇到 `Decl::CInclude` 时在全局顶层作用域自动注册 `c` 全局命名空间符号；
  - 在 `analyze_expr` 中对 `Expr::Move` 激活变量移动标记（`self.moved_vars`），当对已通过 `own` 或 `move` 移交的变量进行再次访问时触发 `E0203` 精准拦截。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 自动收集 `@c { ... }` 中的 C 头文件，生成全局安全的 `pub const c = struct { ... }`，提供常用标准 libc 函数绑定与自定义 C 函数支持；
  - 修复 BTreeMap 内部搜索函数局部变量 `c` 遮蔽全局 `c` 的命名冲突；
  - 在作用域退出 defer 与循环 move 生成中，对结构体值类型通过局部变量解引用调用 `release()`，杜绝 const 限定修饰符丢弃引发的编译错误。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 在 11.9.1 节补充 `@c { "header.h" }` 头文件直接吸纳语法；
  - 在 15.3 节补充 `own` 关键字说明；
  - 增加第十八章：AI-First 机器自愈生态（`cpc check --json`）与无别名单写者保障（`own` 关键字）。

### 新增测试用例 (Added)

- [`cap/test/test_c_inline_header.cp`](file:///data/dev/cp/cap/test/test_c_inline_header.cp): 验证 `@c { "stdio.h" }` 头文件直接内联吸纳并在 5ms 内完成转译与调用 `c.puts` 执行。
- [`cap/test/test_own_keyword.cp`](file:///data/dev/cp/cap/test/test_own_keyword.cp): 验证函数参数 `fn consume(own res: Resource)` 与表达式 `let b = own a` 的所有权转移及自动析构联动。
- [`cap/test/test_own_use_after_move_fail.cp`](file:///data/dev/cp/cap/test/test_own_use_after_move_fail.cp): 反向拦截测试，验证变量被 `own` 移出后再次读取被静态拦截并输出带自愈建议的 `error[E0203]`。
- [`cap/test/test_json_diag_fail.cp`](file:///data/dev/cp/cap/test/test_json_diag_fail.cp): 验证 `cpc check --json` 输出包含精确 `edits` 数组的标准 JSON 诊断。

### 新增文档 (Added)

- [`BUN_AGY_MIGRATION_GUIDE.md`](file:///data/dev/cp/BUN_AGY_MIGRATION_GUIDE.md): 面向 Bun AGY 的 Zig 迁移至 CP 杀手级新语法与高效自愈指南。

---

## 2026-11-09 (原生系统级能力扩展：内联汇编、TLS、辨识联合、ComptimeMap 与枚举智能转换)

### 修改文件 (Modified)

- [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 在 `VarDecl` 中新增 `is_threadlocal: bool` 标识；
  - 在 `Expr` 中新增 `InlineAsm` 与 `ComptimeMap` 表达式变体。
- [`cpc_dev/src/lexer.rs`](file:///data/dev/cp/cpc_dev/src/lexer.rs):
  - 支持 `asm`、`@asm`、`threadlocal` 与 `@tls` 标识符及关键字识别。
- [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 新增内联汇编表达式解析：支持 `@asm(...)` 与 `asm [volatile] (...)` 表达式以及操作数约束；
  - 新增线程局部存储变量解析：支持 `@tls let` 与 `threadlocal let`；若误用 `var` 抛出友好自愈建议；
  - 新增编译期静态查找表解析：支持 `@map(Type, { ... })` 与 `@map({ ... })`；
  - 新增类顶层 `@refCount(deinit)` 装饰器解析。
- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 放宽类型转换检查：允许整型标量与枚举类型之间的显式 `as` 双向转换；
  - 为 `@map` 静态映射注入 `.get()`（返回 `?ValueType`）与 `.has()`（返回 `bool`）方法类型推导；
  - 为所有枚举类型自动注册内置 `.get()`、`.has()` 与 `.name` 方法语义。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 发射 Zig 原生内联汇编 `asm volatile (...)` / `asm (...)`；
  - 发射 Zig 顶层 `threadlocal var name: Type = init;`，修复顶层变量去重缺陷；
  - 发射 `std.StaticStringMap` 静态查找表定义与查找调用；
  - 为枚举生成统一自洽的 `.get()`、`.has()`、`.name` 原生实现；
  - 增强 `Expr::Cast`：整型转枚举生成 `@enumFromInt`，枚举转整型生成 `@intFromEnum`；
  - 完善 `@refCount(deinit)`：在 Bun 环境对接 `bun.ptr.RefCount`，在独立环境生成带原子计数与条件析构的 `ref()` / `deref()` 方法；
  - 修复 `generate_switch` 中模式匹配变量在状态机闭包环境下的 `block_locals` 作用域绑定问题。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第十九章《原生系统级底层能力扩展与编译期元编程规范》，全面详细说明 5 项新语法的设计理念、规范与代码示例。

### 新增测试用例 (Added)

- [`cap/test/test_tls_global.cp`](file:///data/dev/cp/cap/test/test_tls_global.cp): 验证 `@tls let` 与 `threadlocal let` 全局线程局部变量读写与隔离。
- [`cap/test/test_enum_cast_smart.cp`](file:///data/dev/cp/cap/test/test_enum_cast_smart.cp): 验证整型与枚举类型的显式双向智能转换（`kind as i32` 与 `val as SocketKind`）。
- [`cap/test/test_inline_asm.cp`](file:///data/dev/cp/cap/test/test_inline_asm.cp): 验证 `@asm` 与带操作数绑定的 `asm volatile (...)` 原生内联汇编执行。
- [`cap/test/test_comptime_map.cp`](file:///data/dev/cp/cap/test/test_comptime_map.cp): 验证编译期静态查找表 `@map` 与枚举内置 `.get()` / `.has()` / `.name` 自洽映射体系。
- [`cap/test/test_tagged_union_refcount.cp`](file:///data/dev/cp/cap/test/test_tagged_union_refcount.cp): 验证带载荷辨识联合枚举（Tagged Union）匹配解构与 `@refCount` 引用计数自动管理。

---

## 2026-11-08 (彻底废除 Zig 管道解包语法、确立海象指针解包唯一标准与多智能体 CAS 双向合并)

### 修改文件 (Modified)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs):
  - 彻底移除 `if (...) |...|`、`while (...) |...|` 与 `switch` arm `|...|` 管道解包解析分支；
  - 遇到管道捕获语法时触发 `E0101` 智能诊断，输出标准三段式错误与单步重写为海象解包的 `edits` 建议；
  - 修复 `parse_single_pattern` 与 `parse_pattern` 在解析 `LParen`、`LBracket`、`LBrace` 嵌套容器时未维护 `in_paren_pattern` 深度计数器的严重缺陷，杜绝枚举模式内部逗号被误断为 Or-Pattern；
  - 移除多余的 `mut pat` 局部变量修饰。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 在 `Stmt::Match`、`Pattern::Enum` 与 `Expr::Match` 特征扫描中主动标记 `needs_result_error = true`，确保生成强类型匹配与解包辅助函数；
  - 完整合入 Bun AGY 对 `this` / `_` 变量优先级、`Expr::LabeledBreak` 以及 `Expr::Zig` 逃逸表达式支持；
  - 完善枚举实例化中的非枚举静态方法调用兼容。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs):
  - 保持与 Bun AGY 常量表达式求值分支同步。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 8.9 节彻底废除 Zig 风格管道捕获语法，确立海象只读解包 `if (val := opt)` 与可变指针就地解包 `if (*val := opt)` 为唯一标准规范；
  - 增加语法纯洁性保证（Alert Callout）。
- [`BUN_AGY_MIGRATION_GUIDE.md`](file:///data/dev/cp/BUN_AGY_MIGRATION_GUIDE.md):
  - 移除管道语法示例，明确标明 CP 严禁 Zig 管道解包，统一采用海象解包规范。
- [`TASK.md`](file:///data/dev/cp/TASK.md):
  - 同步修订解包语法准则，废除管道语法描述。
- [`cap/test/test_ptr_capture.cp`](file:///data/dev/cp/cap/test/test_ptr_capture.cp):
  - 纯化测试用例，全量采用海象语法验证指针就地覆写更新。

### 新增测试用例 (Added)

- [`cap/test/test_pipe_capture_fail.cp`](file:///data/dev/cp/cap/test/test_pipe_capture_fail.cp): 反向拦截测试，验证误用管道语法 `if (opt) |*val|` 被前端精确识别为 `E0101` 并输出包含海象解包修正建议的标准 JSON 诊断。

---

## 2026-11-09 (函数风格类型构造 Type(val)、通用 .toString() 与弱类型拦截)

### 修改文件 (Modified - cpc_dev)

- [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 在 `analyze_expr` 中添加对 `num(...)` 与 `Number(...)` 的语义硬性拦截，触发带精确修正建议的 `SemaError::InvalidCast`；
  - 在 `infer_type` 中，将标量数值类型标识符（`i8`..`i128`, `u8`..`u128`, `isize`, `usize`, `f32`, `f64`, `bool`）调用推导为对应标量类型；
  - 将 `string(...)` / `String(...)` 调用推导为 `Type::String`；
  - 将枚举标识符单参调用推导为对应枚举类型；
  - 将 `.toString()` 属性访问和调用统一推导为 `Type::String`。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 在 `generate_expr` 中实现标量函数风格类型强转 `Type(val)` 映射至 Zig `@as(TargetZigType, val)`；
  - 实现枚举函数风格构造 `EnumType(val)` 映射至 Zig `@enumFromInt(val)`；
  - 实现布尔函数风格转换：通过 Zig 编译期表达式安全处理数字到布尔值的非零判定；
  - 实现 `string(val)` 与 `String(val)` 生成，按需触发 `needs_fmt` 与 `needs_string_helpers`；
  - 优化 `.toString()` 代码生成：当源类型为枚举时直接发射 Zig 原生 `@tagName(obj)` 实现零堆分配零开销转换；当源类型为标量或对象时调用 `cp_string_from(obj)`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 19.6 节《函数风格类型构造转换 (`Type(val)`) 与通用 `.toString()`》，明确函数风格构造规范、禁止模糊 `num` 的强类型安全设计，以及零开销 `.toString()` 规范。

### 新增测试用例 (Added)

- [`cap/test/test_type_conv_functional.cp`](file:///data/dev/cp/cap/test/test_type_conv_functional.cp): 全面覆盖枚举与整型函数风格双向构造、标量数值类型转换、布尔转换、`string(val)` / `String(val)` 以及枚举与标量的 `.toString()` 方法调用（4/4 测试项全绿灯通过）。

---

## 2026-11-09 (代码生成辅助模板按需 Tree-Shaking 优化与无用代码消除)

### 修改文件 (Modified)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - **消除无条件注入的 Option 解包垫片**：将 `CpOptType`、`CpUnwrapOpt`、`cp_opt_is_null`、`cp_opt_unwrap` 的生成条件由 `if true` 改为受控于 `if features.needs_opt_chaining`，仅在使用可选链操作符 `?.` 时才输出；
  - **消除无条件注入的字符串格式化垫片**：将 `cp_string_from` 改为受控于 `if features.needs_string_helpers || features.needs_fmt`，消除基础结构体与枚举转译为 Zig 时产生的死代码；
  - **完善字符串探测**：在 `scan_expr` 中添加对 `string.from` 与 `String.from` 的特征探测，确保显式字符串化时稳定触发辅助函数生成。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 修复 `parse_postfix_type_modifiers` 在解析可选返回类型后紧跟错误类型联合（如 `string? ! FileErr`）时因非 `CStr` 类型提前中断导致未消费 `?` 的语法解析缺陷。
- [`BUN_AGY_MIGRATION_GUIDE.md`](file:///data/dev/cp/BUN_AGY_MIGRATION_GUIDE.md):
  - 补充第五章《最新升级：系统级底层特性与类型转换规范（消除剩余 25 个模块中的 `zig {}` 块）》。

---

## 2026-11-09 (CP 语言与 cpc 编译器工业级生产化演进路线图全面落地)

### 修改文件 (Modified)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第二十章《CP 语言与 `cpc` 编译器工业级生产化演进路线图 (Production-Grade Roadmap)》，系统阐述调试/DWARF/Source Map 映射、可插拔 Allocator 与 Scoped Arena 作用域内存池、Auto-Reduction 抢占式调度器自动织入与跨 await 安全、模块级增量编译缓存与循环解耦、泛型 Trait 前端拦截与多字段独立借用、零拷贝标准库基石、开发者全家桶工具链（完备 LSP、`cpc fmt`、原生测试与性能评测）以及 AI-First 机器自愈闭环。
- [`DEVLOG.md`](file:///data/dev/cp/DEVLOG.md):
  - 记录生产级别核心设计哲学、八大支柱体系演进技术指标与 P0/P1/P2 三阶段推进规划。

---

## 2026-11-09 (P0 第一项完成：源码行号指示、Source Map 伴随生成与堆栈反解工具 cpc/cap demangle)

### 新增文件 (Added)

- [`cpc/src/source_map.rs`](file:///data/dev/cp/cpc/src/source_map.rs) / [`cpc_dev/src/source_map.rs`](file:///data/dev/cp/cpc_dev/src/source_map.rs):
  - 实现标准 `SourceMap` 与 `SourceMapEntry` 数据结构及其 JSON 序列化/反序列化；
  - 实现二分查找/最近邻区间定位算法 `lookup(zig_line)`；
  - 实现工业级堆栈反解引擎 `demangle_stack_trace`，自动解析 Zig 堆栈日志并还原为原生 `.cp` 文件名、行号与列号，并输出上下文代码切片与指针 `^`。
- [`cap/test/test_source_map_demangle.cp`](file:///data/dev/cp/cap/test/test_source_map_demangle.cp):
  - 新增 Source Map 行号映射与堆栈精准反解端到端回归验证测试用例。

### 修改文件 (Modified)

- [`cpc/src/lib.rs`](file:///data/dev/cp/cpc/src/lib.rs) / [`cpc_dev/src/lib.rs`](file:///data/dev/cp/cpc_dev/src/lib.rs):
  - 导出 `pub mod source_map;`。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 为 `Expr` 和 `Stmt` 全体变体实装 `span(&self) -> Span` 源码范围提取方法。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 新增源码文件与代码行号索引机制（`init_source`、`current_generated_line`、`offset_to_line_col`）；
  - 在 `generate_stmt` 与 `generate_function_decorated` 中织入 `record_source_location` 映射捕获点；
  - 转译输出时自动伴随生成 `<target>.zig.map`。
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs) / [`cpc_dev/src/main.rs`](file:///data/dev/cp/cpc_dev/src/main.rs):
  - 增加 CLI 子命令 `cpc demangle [trace_file] [-m map_file]`；
  - 接入编译输出时的伴随 `.zig.map` 写入。
- [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig):
  - 在 `cap` 工具链中接入 `cap demangle [trace_file]` 命令路由，直通调用编译器堆栈反解器；
  - 更新 `usage()` 帮助信息。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 20.1 节与 20.9 节，标记第一项任务完成并记录技术规范与反解示例。

---

## 2026-11-09 (P0 第二项完成：泛型 Trait 前端契约校验与 Generic Bounds)

### 新增文件 (Added)

- [`cap/test/test_generic_trait_bounds.cp`](file:///data/dev/cp/cap/test/test_generic_trait_bounds.cp):
  - 泛型 Trait 约束端到端全链路测试用例（含自定义 Trait 契约约束 `<T: Printable>`、内置标量约束 `<T: Comparable>`、多态类实例化与泛型单态化执行验证）。

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 升级 `GenericParam::Type` 变体为 `{ ident: Ident, bounds: Vec<Ident> }`，原生支持类型约束列表；
  - 为 `Function` 与 `MethodSig` 增加 `generic_params: Vec<GenericParam>` 字段，支持函数级与方法级泛型定义。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 新增 `parse_generic_params`，完整解析 `<T>`, `<T: Trait>`, `<T: TraitA + TraitB>`, `<const N: usize>`；
  - 将泛型解析器正交接入全局函数（`parse_function`）、类定义（`parse_class`）与 Trait 契约（`parse_trait`）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 新增三段式 AI 自愈诊断错误：`error[E0301]: type 'X' does not satisfy trait bound 'Y'` 与 `class 'X' does not satisfy trait 'Y'`；
  - 实装 `Decl::Class` 的 `implements Trait` 编译期静态方法完备性校验；
  - 实装全局函数与类方法作用域活跃泛型约束跟踪 `active_generic_bounds`；
  - 实装内置 Trait 标量自愈规则（`Comparable`、`ToString`、`Clone`、`Hash`、`Debug`）；
  - 实装 `Expr::Call` 与 `Expr::New` 调用实参前端 Trait 约束拦截检查与三段式报错自愈提示，彻底阻断底层 Zig 编译器 50 行隐晦报错泄露；
  - 在 `infer_type` 中实装基于调用实参的泛型返回值类型动态推导与单态化。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 泛型参数映射为 Zig 原生 `anytype`；泛型返回值映射为 `@TypeOf(param)` 实现零开销编译期单态化；
  - 在 `infer_cp_type_with_mock` 中根据调用实参推导具体泛型返回类型，确保 CPS 状态机上下文变量结构体字段（如 `_Context`）精准单态化（生成 `m1: i32` 而非虚拟类型 `m1: T`）。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 详实扩充第 7.4 节《泛型 Trait 前端契约校验与单态化规范 (Generic Bounds)》，提供完整语法定义、内置 Trait 规则、自愈诊断示例与 Zig 单态化架构说明；
  - 更新第 20.9 节路线图状态表，将第 2 项标记为 `【已完成】`。

---

## 2026-11-10 (P0 第三项完成：Scoped Arena 区域内存池语法与运行时实现)

### 新增文件 (Added)

- [`cap/test/test_scoped_arena.cp`](file:///data/dev/cp/cap/test/test_scoped_arena.cp):
  - 涵盖语句级块作用域 `arena { ... }`、表达式作用域 `let val: i32 = arena { ... }`、多层嵌套 arena、`@alloc` 底层动态申请与 Dict/Map 容器作用域分配的 5 组端到端全量回归测试用例。

### 修改文件 (Modified)

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs) / [`cpc_dev/src/lexer.rs`](file:///data/dev/cp/cpc_dev/src/lexer.rs):
  - 注册 `arena` 核心关键字，新增 `Token::Arena`。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 在 `Stmt` 中新增 `Stmt::ArenaBlock(BlockStmt)`；
  - 在 `Expr` 中新增 `Expr::Arena(BlockStmt)`；
  - 为两者实装 `span(&self)` 方法。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs) / [`cpc_dev/src/ast_visitor.rs`](file:///data/dev/cp/cpc_dev/src/ast_visitor.rs):
  - 递归遍历 `Stmt::ArenaBlock` 与 `Expr::Arena` 内部的变量引用与声明。
- [`cpc/src/lsp.rs`](file:///data/dev/cp/cpc/src/lsp.rs) / [`cpc_dev/src/lsp.rs`](file:///data/dev/cp/cpc_dev/src/lsp.rs):
  - 在补全建议列表中追加 `"arena"` 关键字。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 在 `parse_stmt` 中解析独立语句块 `arena { ... }` 为 `Stmt::ArenaBlock`；
  - 在 `parse_primary_expr` 中解析表达式块 `let v = arena { ... }` 为 `Expr::Arena`；
  - 在 `left_span` / `right_span` 中覆盖 `Expr::Arena`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 在 `analyze_stmt` 中处理 `Stmt::ArenaBlock`；
  - 在 `analyze_expr` 与 `infer_type` 中推导 `Expr::Arena` 返回值类型；
  - 支持 `@alloc` 与 `@arena` 原语类型识别（`@alloc` 返回 `[*]u8` 即 `Type::ManyRawPtr(Box::new(Type::U8))`）。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 新增 `arena_allocator_stack`，提供 `current_allocator(&self)` 动态获取当前活跃 Arena；
  - 为 `Stmt::ArenaBlock` 生成 Zig `std.heap.ArenaAllocator.init(...)` 与 `defer __arena.deinit()`；
  - 为 `Expr::Arena` 生成带标签的代码块 `(blk_arena: { ... break :blk_arena res; })`；
  - 全面穿透 CPS 状态机变量收集：在 `extract_locals_simple` 与 `extract_local_vars_to_str` 中递归遍历 `Stmt::ArenaBlock` 以及表达式内部的 `Expr::Arena`，保证异步函数/`main` 状态机 `_Context` 字段和符号表 100% 完整提取；
  - 修复 `@alloc` 局部常量名导致遮蔽外层 `cp_main` 参数 `ptr` 的问题（重命名为 `__alloc_res`）。

---

## 2026-11-10 (跨模块循环引用解耦 Two-Pass Resolution 验证 & 多字段分离借用 Disjoint Field Borrowing 实现)

### 新增文件 (Added)

- [`cap/test/fixtures/circ_a.cp`](file:///data/dev/cp/cap/test/fixtures/circ_a.cp): 互相循环引用的模块 A 测试夹具 (`NodeA` 包含 `?NodeB`)。
- [`cap/test/fixtures/circ_b.cp`](file:///data/dev/cp/cap/test/fixtures/circ_b.cp): 互相循环引用的模块 B 测试夹具 (`NodeB` 包含 `?NodeA`)。
- [`cap/test/test_circular_modules.cp`](file:///data/dev/cp/cap/test/test_circular_modules.cp): 跨模块循环引用（Two-Pass Resolution）端到端集成测试，验证类型推导与交叉引用无死锁。
- [`cap/test/test_disjoint_field_borrow.cp`](file:///data/dev/cp/cap/test/test_disjoint_field_borrow.cp): 多字段分离借用（Disjoint Field Borrowing）测试用例，覆盖结构体多字段并行可变借用、多级嵌套字段借用、方法内 `this` 正交字段借用及字段级读写冲突防护。

### 修改文件 (Modified)

- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 将 `var_borrow_sources` 从 `HashMap<String, (Vec<String>, bool)>` 升级为 `HashMap<String, (Vec<(String, Option<String>)>, bool)>`，实现细粒度字段级借用源与借用路径跟踪；
  - 新增 `targets_conflict(target_a, field_a, is_mut_a, target_b, field_b, is_mut_b) -> bool`：精准判定正交字段（Disjoint Fields），同一根对象的不同字段互不冲突，允许同时持有多个可变借用；若涉及全对象或同一字段则严格互斥；
  - 新增 `check_borrow_conflict_field` 与 `check_borrow_conflict_field_excluding` 校验函数；
  - 新增 `extract_member_path(&self, expr: &Expr) -> (Option<String>, Option<String>)` 递归提取多级成员路径（如 `header.content_len`）；
  - 新增 `resolve_borrow_source_field` 处理借用别名链的传递性路径结合；
  - 增强 `check_mutation_allowed` 与 `check_read_allowed`：支持字段级借用冲突判定，允许在某一字段被借用时安全读写其他正交字段；
  - 适配 `check_active_borrows_before_move`、`for` 循环容器借用、函数返回值悬垂引用检查等所有调用点。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 第 20.4 节补充《跨模块循环引用解耦 (Two-Pass Resolution)》架构机制、规范定义与示例代码；
  - 第 20.5 节补充《多字段分离借用 (Disjoint Field Borrowing)》正交字段借用规则、互斥防线与示例代码；
  - 纠正第 20.5 节文档中残留的 Rust 风格 `&mut` 笔误，全面统一为标准 CP 可变借用前缀 `*` 语法（`*data.q_len`）。
- [`DEVLOG.md`](file:///data/dev/cp/DEVLOG.md):
  - 纠正开发日志中 `&mut` 语法表述为标准 `*` 借用；
  - 深度复核并记录 7 项语法机制裁定分析（可变借用语法、Two-pass 历史架构、具名循环标签增强、顶层 comptime 上下文兼容、inline for 上下文修饰词、C-FFI 复合布局放行、显式返回类型省略冒号）。

---

## 2026-11-10 (支持裸函数类型声明与 While 步进表达式，解决 task-zig-syntax-compat)

### 新增文件 (Added)

- [`cap/test/test_zig_syntax_compat.cp`](file:///data/dev/cp/cap/test/test_zig_syntax_compat.cp): 函数类型声明赋值调用与 `while (cond) : (step)` 步进循环综合回归测试用例。

### 修改文件 (Modified)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - `WhileStmt` 新增 `continue_expr: Option<Expr>` 字段。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - `parse_while_stmt` 支持解析条件圆括号后的 `: (step_expr)` 步进表达式；
  - `Token::Fn` 表达式分支支持无冒号返回类型；若参数与返回类型后无 `{` 块，识别为纯函数签名类型；
  - 顶层 `const MyFunc = fn (x: u8) bool;` 自动提升为强类型 `Decl::TypeAlias`。
- [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs) / [`cpc_dev/src/ast_visitor.rs`](file:///data/dev/cp/cpc_dev/src/ast_visitor.rs):
  - 递归遍历 `WhileStmt.continue_expr` 中的变量引用。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 适配 `WhileStmt.continue_expr` 的语义分析与泛型变量替换。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 生成 Zig 对齐的 `while (cond) : (step) { ... }` 原生循环代码；
  - 增强 `infer_cp_type_with_mock` 的 `Expr::Call` 分支，支持通过函数类型别名解析被调用的函数指针返回类型，消灭状态机降级为 `*anyopaque` 的类型不匹配报错。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 8.12 节《循环步进表达式 (`while (cond) : (step)`) 与函数类型别名 (`const F = fn(...) Ret`)》。
  - 更新第 17.12.1 节，明确模块顶层常量与变量编译期求值规范 (`Top-Level Comptime Initializer Semantics`)，并纠正顶层变量语法为标准 `pub let`，彻底清理误导性的 `var` 写法。
- [`cap/test/test_zig_syntax_compat.cp`](file:///data/dev/cp/cap/test/test_zig_syntax_compat.cp):
  - 局部变量声明归正为标准 `let`。
- [`cap/test/test_bun_top_level_import.cp`](file:///data/dev/cp/cap/test/test_bun_top_level_import.cp):
  - 顶层全局变量声明归正为标准 `pub let`，切片缓冲区定义归正为标准数组 `[u8; 8]`。

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 审核并同步 Bun AGY 对顶层 `var` 初始值求值置位 `is_comptime = true` 的改动，消除 Zig 顶层 Comptime 作用域内嵌 `comptime` 表达式错误；
  - 增加 `Pattern::Expr` 的 Zig 代码生成支持与 `format_simple_expr` 序列化；
  - `is_simple_literal_switch` 纳入 `Pattern::Expr`，直接生成高效原生 Zig `switch`。
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - `Pattern` 枚举新增 `Expr(Box<Expr>)` 分支。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 拦截回退 Bun AGY 颠倒错误联合类型的危险改动；
  - `parse_single_pattern` 支持 `@` 开头的内建表达式作为模式分支（如 `@field(Tag, "foo")`）。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 4.3.2 节《编译期内建表达式匹配 (`@field` 动态字段与元编程模式)》。
- [`cap/test/test_switch_field_pat.cp`](file:///data/dev/cp/cap/test/test_switch_field_pat.cp):
  - 新增 switch `@field` 模式匹配端到端回归用例。

## [2026-11-11] - 修复 std.mem.endsWith/startsWith 等多级命名空间函数调用参数篡改问题 (task-std-mem-endswith-codegen)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs):
  - 修复 `includes`、`startsWith`、`endsWith`、`toLowerCase`、`toUpperCase`、`replace` 等字符串方法糖的调用匹配条件；
  - 将 `startsWith`、`endsWith`、`includes` 严格限制为单参数 `call.args.len() == 1`，将 `replace` 严格限制为双参数 `call.args.len() == 2`，消除多参数原生命名空间函数调用（如 `std.mem.endsWith(u8, name, "_reply")`）被错误劫持为字符串实例方法并将参数篡改的严重缺陷。
- [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 同步合入上述 codegen 参数匹配严审机制，保护现有常量泛型开发分支。
- [`cap/test/test_std_mem_endswith.cp`](file:///data/dev/cp/cap/test/test_std_mem_endswith.cp):
  - 新增 `std.mem.endsWith` 与 `std.mem.startsWith` 多级命名空间原生函数调用的端到端回归用例。

## [2026-11-11] - struct/匿名结构体内函数体 let 自动降级与冒号消除 (task-struct-let-to-var)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 实现 `lower_struct_body` token 感知安全降级解析器；
  - 自动将 struct 内部函数声明中的 `let` 与 `let mut` 降级为 Zig 的 `var`；
  - 消除 CP 风格函数签名的返回类型冒号（`):` -> `)`）。
- [`cap/test/test_struct_let.cp`](file:///data/dev/cp/cap/test/test_struct_let.cp):
  - 新增 struct 嵌套函数体内 `let` 降级与执行的端到端回归用例。

## [2026-11-11] - 常量泛型（Const Generics）与定长张量/网络缓冲区支持

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 新增 `ArraySize` 枚举（`Literal`, `Ident`, `Expr`）；
  - `Type::FixedArray` 支持常量参数尺寸。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 支持 `[T; N]`、`[N]T` 与表达式维度定长数组解析；
  - 支持泛型结构体字面量与静态成员访问（如 `Embedding.rows`）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 常量泛型参数语义推导与单态化类型展开（`subst_map`）。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 泛型类声明与实例化的 Zig 代码单态化生成；
  - 支持常量泛型类型别名展开。
- [`cap/test/test_const_generics.cp`](file:///data/dev/cp/cap/test/test_const_generics.cp):
  - 新增 `Buffer<const N: usize>` 与 `Tensor<T, const shape: [usize; 2]>` 端到端回归用例。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 完善第 11.9.5 节《常量泛型类（Const Generic Classes）与定长张量/定长缓冲区》规范手册与代码示例。

# CP 变更日志 (CHANGELOG) - 2026-11-11 (热数据)

> 历史变更日志已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本项目采用冷热分离架构，本文件仅保留当日活跃记录。

## [2026-11-11] - 并发 Guard 代理访问、定长数组字面量与 C-FFI 负数生成修复

### 优化与修复 (Fixed)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - **同步 Guard 代理字段**：将 `ReadGuard`、`WriteGuard`、`LockGuard` 的 `.data` 代理访问判定优先级提升至 enum 字段属性匹配之前，使 `rg.value` 正确解引用至 `rg.data.value`，并允许对其直接赋值；
  - **结构体 `value` / `name` 属性冲突消除**：针对明确为 struct/class 的实例，禁用 `(if (@typeInfo(...) == .@"enum") ...)` 伪属性分支，消除赋值语句 LHS 非法表达式错误；
  - **`[N]T` 定长数组类型推导**：修复语句与全局变量推导中对 `Type::Ref("[N]T")` 的识别，使定长数组初始化自动匹配元素类型并生成原生 Zig 数组字面量 `.{ ... }`，消除错误回退为 `cp_array_lit(i32, ...)` 的类型不匹配；
  - **负整数字面量发射**：修复表达式生成中将负整数错误强制格式化为 64 位十六进制（如 `c_abs(0xffffffffffffff85)`）的严重缺陷，恢复为正确十进制形式；
  - **定长数组到多项指针强转**：修复 `arr as [*]T!` 在转译时未对值数组取地址导致 Zig 拒绝 `@ptrCast` 的问题，自动生成 `&arr`。

## [2026-11-11] - 编译器语句级与顶层项 Panic-Mode 语法容错与多错误批量诊断

### 新特性 (Added)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 新增 `ParseError::Multiple(Vec<Diagnostic>)` 复合诊断类型；
  - 实现语句级容错同步器 `recover_stmt`：感知大括号嵌套深度，在遇到语句语法错误时跳过无效 Token 至 `;`、`}` 或控制流关键词（`let`/`if`/`while`/`for`/`return`/`switch`/`break`/`continue`/`defer`/`errdefer`），继续解析后续语句；
  - 实现顶层项容错同步器 `recover_module_item`：跳过出错的函数或类型定义至下一个顶层关键词（`pub`/`export`/`fn`/`class`/`struct`/`let`/`const`/`import`/`@`）；
  - `parse_block_stmt` 与 `parse_module` 升级为非短路式多错误累积模式，严格防止死循环并设有单模块最多 50 个错误上限与重复去重。
- [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs) / [`cpc_dev/src/main.rs`](file:///data/dev/cp/cpc_dev/src/main.rs):
  - `cpc check` 支持一次性输出全部语法的格式化代码切片与自愈建议；
  - `cpc check --json` 输出聚合包含全部诊断的结构化 `DiagnosticReport` 与无损 `edits` 补丁。
- [`cap/test/test_error_recovery_fail.cp`](file:///data/dev/cp/cap/test/test_error_recovery_fail.cp):
  - 新增多错误批量捕获与容错同步端到端负向测试用例。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 18.1.1 节《编译器语句级与顶层项 Panic-Mode 语法容错恢复与多错误批量收集 (Multi-Error Recovery)》。

## [2026-11-11] - 编译器 Prelude 作用域防遮蔽 (Anti-Shadowing) 与 GEMINI 规范升级

### 优化与修复 (Fixed)

- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 解决 `task-prelude-t-shadowing`：将 CodeGen Prelude 中所有泛型辅助函数（`cp_iter_slice`、`is_string_like`、`CpValType`、`cp_string_from`、`cp_opt_is_null`、`cp_opt_unwrap`、`LockGuard`、`ReadGuard`、`WriteGuard`、`CpMutex`、`CpRwLock`、`CpAtomic`、`cp_unpack_elem` 等）中的 `const T` / `comptime T` 全面重命名为私有前缀 `__T`；
  - 将所有自动析构注入（`defer if (!__x_moved) { const __T = @TypeOf(x); ... }`）中的类型标识符重命名为 `__T`；
  - 彻底杜绝底层 Zig 0.17 在用户业务代码声明顶层或局部 `T` 时的作用域遮蔽（`local constant shadows declaration of 'T'`）报错。

### 规则与架构升级 (Changed)

- [`GEMINI.md`](file:///data/dev/cp/GEMINI.md):
  - 全面放开 Bash 终端输出重定向（`>`、`>>`、管道 `|`），允许毫秒级链式自愈验证；
  - 建立四线文档冷热分离机制（当日活跃数据存储于 `SESSION.md`、`CHANGELOG.md`、`DEVLOG.md`，历史冷归档存储于 `*_H.md`）；
  - 建立任务分类驱动机制（Hotfix 极速响应通道 vs Milestone 集中汇总归档）；
  - 制定编译器核心文件（`codegen.rs`、`parser.rs`、`sema.rs`）按功能模块化分拆规范。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 新增第 11.9.5.2 节《运行时 Prelude 辅助函数命名空间隔离与防遮蔽机制（Anti-Shadowing）》，明确规范设计理念与代码示例。

### 新增测试用例 (Added)

- [`cap/test/test_prelude_t_shadowing.cp`](file:///data/dev/cp/cap/test/test_prelude_t_shadowing.cp):
  - 验证用户在模块顶层定义 `pub const T = enum { Alpha, Beta }` 时，切片迭代与辅助函数共存且 100% 编译运行通过。

## [2026-11-20] - `cap:ws` RFC 6455 / RFC 7692 工业级 WebSocket 协议栈升级（3 新模块，5/5 PASS）

### 标准库深度升级 (`lib/ws/`，0 `std.posix`，0 `@import("bun")`，100% 向后兼容)

- 新增 [`lib/ws/subprotocol.cp`](file:///data/dev/cp/lib/ws/subprotocol.cp)：`SubprotocolNegotiator`（RFC 6455 §11.3.4 精确子协议协商）、`WsExtensionHeader`（扩展头解析与序列化）与 `CloseCodeValidator`（RFC 6455 §7.4.1 线缆关闭码与 UTF-8 关闭原因校验）；
- 新增 [`lib/ws/permessage_deflate.cp`](file:///data/dev/cp/lib/ws/permessage_deflate.cp)：RFC 7692 `PerMessageDeflate`（协商响应头、`compressPayload`/`decompressPayload` 自动剥离/补回尾部 `0x00 0x00 0xFF 0xFF`、`MessageTooBig` 资源耗尽防护）；
- 新增 [`lib/ws/session.cp`](file:///data/dev/cp/lib/ws/session.cp)：`WsSession` 完整状态机（`Connecting`/`Open`/`Closing`/`Closed`、掩码强制校验、超限帧/消息预检、压缩分片重组、控制帧穿插与自动 Pong、主动/被动 Close 握手与心跳追踪）；
- 增量导出至 [`lib/ws/index.cp`](file:///data/dev/cp/lib/ws/index.cp)，同步 [`docs/ws/ws.md`](file:///data/dev/cp/docs/ws/ws.md)；新增 [`cap/test/test_ws_production_deep.cp`](file:///data/dev/cp/cap/test/test_ws_production_deep.cp)（单文件与 `-m` 模式均 5/5 PASS，既有 `test_ws.cp` 4/4 与 `test_md_html_json_ws_deep.cp` 4/4 零回归）。

---

## [2026-11-20] - `cap:time` 工业级时间、Cron 调度与限流退避套件落地（5 模块，6/6 PASS）

### 新增标准库 (`lib/time/`，`cap:time`，0 `std.posix`，0 `@import("bun")`)

- 新增 [`lib/time/clock.cp`](file:///data/dev/cp/lib/time/clock.cp)：`TimeUnit`、`Duration` 与基于 `std.os.linux.clock_gettime` 的 `WallClock`（墙钟与单调纳秒/毫秒时钟）；
- 新增 [`lib/time/iso8601.cp`](file:///data/dev/cp/lib/time/iso8601.cp)：`CivilDateTime`（Howard Hinnant 纯算法 Civil<->Epoch 转换，支持 1970 前后与 400 年闰年周期）、`Iso8601`（RFC 3339 严格解析与格式化）及 `IsoDuration`（`P1DT2H30M15S` 双向解析与格式化）；
- 新增 [`lib/time/cron.cp`](file:///data/dev/cp/lib/time/cron.cp)：POSIX 5 字段 `CronExpr`（`*`/`*/n`/`a-b`/`JAN-DEC`/`SUN-SAT`/`@daily` 等别名）、`matches` 与字段跳跃式 `nextAfter`（精准跨闰年与非闰世纪年）；
- 新增 [`lib/time/ratelimit.cp`](file:///data/dev/cp/lib/time/ratelimit.cp)：`TokenBucket`（令牌桶）、`SlidingWindowLimiter`（O(1) 双窗口加权滑动限流）与 `ExponentialBackoff`（SplitMix64 确定性全抖动/等抖动指数退避）；
- 统一导出至 [`lib/time/index.cp`](file:///data/dev/cp/lib/time/index.cp)，同步 [`docs/time/time.md`](file:///data/dev/cp/docs/time/time.md) 与双轨规范；新增 [`cap/test/test_time_production_deep.cp`](file:///data/dev/cp/cap/test/test_time_production_deep.cp)（单文件与 `-m` 模式均 6/6 PASS）。

---

## [2026-11-20] - `cap:net` 工业网络底座升级（+1,891 行）与 `cap:pack` ZIP/PAX-TAR/魔数引擎升级（+1,048 行）双路落地

### `cap:net` 工业网络基础设施升级 (`lib/net/`，0 `std.posix`，0 `@import("bun")`)

- 新增 [`lib/net/cidr.cp`](file:///data/dev/cp/lib/net/cidr.cp)（822 行）：IPv4/IPv6 `Ipv4Cidr`/`Ipv6Cidr`（解析/包含/重叠/网络与广播地址）、`IpAddrCodec`（RFC 5952）与 `IpClass`/`CidrAllowlist`（私网/环回/链路本地/CGNAT 分类与出站边界防护）；
- 新增 [`lib/net/sockopts.cp`](file:///data/dev/cp/lib/net/sockopts.cp)（371 行）：纯 Zig 0.17 `std.os.linux` `SocketOptions`（`TCP_NODELAY`/`SO_KEEPALIVE`/`SO_REUSEADDR`/`SO_REUSEPORT`/`SO_RCVBUF`/`SO_SNDBUF`/收发超时/非阻塞/`SO_ERROR`）与 `pollSingleFd`/`pollManyFds`；
- 新增 [`lib/net/socks5.cp`](file:///data/dev/cp/lib/net/socks5.cp)（417 行）：RFC 1928/1929 `Socks5Codec`（Greeting、UserPass 鉴权、IPv4/Domain/IPv6 CONNECT 编解码）与 `HttpConnectCodec`；
- 新增 [`lib/net/pool.cp`](file:///data/dev/cp/lib/net/pool.cp)（281 行）：`ConnectionPool`（按 `host:port` 分桶复用、TTL 过期与容量上限淘汰、`PoolStats`）；同步更新 [`lib/net/proxy.cp`](file:///data/dev/cp/lib/net/proxy.cp)、[`lib/net/index.cp`](file:///data/dev/cp/lib/net/index.cp)、[`docs/net/net.md`](file:///data/dev/cp/docs/net/net.md) 与 [`cap/test/test_net_production_deep.cp`](file:///data/dev/cp/cap/test/test_net_production_deep.cp)（5/5 PASS，`-m` 5/5 PASS，既有回归 11/11 PASS）。

### `cap:pack` 压缩与归档引擎升级 (`lib/pack/`，0 `std.posix`，0 `@import("bun")`)

- 新增 [`lib/pack/zip_writer.cp`](file:///data/dev/cp/lib/pack/zip_writer.cp)（325 行）：PKWARE APPNOTE 6.3.10 `ZipWriter`（Stored/Deflate 写入、UTF-8 标志位、CRC-32、Local/Central Directory/EOCD 与 `decompressAndVerify` 读回校验）；
- 新增 [`lib/pack/tar_pax.cp`](file:///data/dev/cp/lib/pack/tar_pax.cp)（568 行）：POSIX.1-2001 `PaxTarWriter`/`PaxTarReader`（USTAR + PAX `'x'` 扩展头突破 100B 路径/软链限制）与 `validateArchivePath`（严格拦截绝对路径与 `..` 穿越）；
- 新增 [`lib/pack/magic.cp`](file:///data/dev/cp/lib/pack/magic.cp)（155 行）：`ArchiveSniffer`（Gzip/Zlib/Zstd/ZIP/TAR/XZ/BZ2/LZ4/BrotliHint 魔数探测与 MIME/扩展名映射）；同步更新 [`lib/pack/index.cp`](file:///data/dev/cp/lib/pack/index.cp)、[`docs/pack/pack.md`](file:///data/dev/cp/docs/pack/pack.md) 与 [`cap/test/test_pack_production_deep.cp`](file:///data/dev/cp/cap/test/test_pack_production_deep.cp)（5/5 PASS，`-m` 5/5 PASS，既有回归全部 PASS）。

---

## [2026-11-20] - `cap:crypto` 现代零 C 依赖密码学全栈生产升级（Ed25519/X25519/P-256/HKDF/Scrypt/Argon2id/BLAKE3/SHA3/XChaCha20/JWT/PEM）

### 标准库深度升级 (`lib/crypto/`，+1,167 行纯 CP，保留 `_boringssl.zig` 与既有 API 100% 兼容)

- 新增 [`lib/crypto/asymmetric.cp`](file:///data/dev/cp/lib/crypto/asymmetric.cp)（220 行）：基于 Zig 0.17 `std.crypto` 的 `Ed25519KeyPair`、`X25519KeyPair`（ECDH + 零公钥拒绝）与 `EcdsaP256KeyPair`（支持 33B/65B SEC-1 与 DER 签名）；
- 新增 [`lib/crypto/kdf.cp`](file:///data/dev/cp/lib/crypto/kdf.cp)（533 行）：`hkdfExtract`/`hkdfExpand`/`hkdfDerive`（SHA-256/SHA-512）、`scryptDerive`、`argon2idDerive`/`argon2idHash`/`argon2idVerify`、`Blake3`/`Blake3Stream`、`Sha384`/`Sha3_256`/`Sha3_512`、`HmacSha384`/`HmacSha512`、`XChaCha20Poly1305` 与 `Aes128Gcm`；
- 新增 [`lib/crypto/jwt.cp`](file:///data/dev/cp/lib/crypto/jwt.cp)（364 行）：RFC 7519 `Jwt`（HS256/HS384/HS512/EdDSA，恒定时间验签，拒绝 `alg: none` 与跨算法混淆）、RFC 7468 `Pem`（64 列折行编解码与标签校验）、`constantTimeEql`、`base64UrlEncode`/`Decode`；
- 增量导出至 [`lib/crypto/index.cp`](file:///data/dev/cp/lib/crypto/index.cp)，更新 [`docs/crypto/crypto.md`](file:///data/dev/cp/docs/crypto/crypto.md) 与双轨规范手册；新增 [`cap/test/test_crypto_production_deep.cp`](file:///data/dev/cp/cap/test/test_crypto_production_deep.cp)（8/8 PASS），回归 `test_crypto_features.cp`（4/4）与 `test_crypto_sql_dns.cp`（5/5）全部 100% PASS。

---

## [2026-11-20] - `cap` CLI 构建工具链可靠性与模块化测试升级：`cap test -m`、`cap clean [--all]`、`cap build --timings` 与失败退出码修复

### 构建工具链升级 (`cap/src/main.zig`，ReleaseFast 已双路径同步部署)

- **修复 `cap build` 失败时退出码为 `0` 的隐患**：`compileSingleFile` 追踪 `any_failed`，任一目标构建失败时跳过 `Build completed!` 并返回 `error.BuildFailed`（退出码 `1`）；
- **完善 `cap clean` / `cap clean --all`（兼容 `-a`）**：`cap clean` 安全清理 `.cap/`（含 `.cap/ws/`、`.cap/test-mod/`）保留源码与 `bin/`；`cap clean --all` 同时清理 `bin/` 与安全 `out_dir`（防护误删 `.`/`src`/`test`）；
- **新增 `cap test -m` / `--modular` 模块化增量测试**：对接 `cpc compile --emit-dir .cap/test-mod` 复用共享 `_lib/` 与 `__cp_runtime.zig`，实测多测试文件 15/16 模块缓存命中（3ms 增量转译）；
- **新增 `cap build --timings` 分阶段耗时诊断**：精准输出 `Source mtime scan`、`cpc Transpile`、`Zig Native Compile`、`Atomic Install` 与 `Total` 的毫秒和占比；
- **二进制双路径同步**：原子安装至 `/data/dev/cp/cap/bin/cap` 与 `/root/.cap/bin/cap`（SHA-256 一致），`cap test -m cap/test/test_cap_lazy_hub.cp` 验证通过（`total=7`，868ms）。

---

## [2026-11-20] - 全仓回归大扫荡与跨模块符号卫生治理：清零全仓 `lib/` `std.posix`、58/58 标准库全入 `cap` 惰性路由、185 个正向测试全绿

### 跨模块符号卫生与 `std.posix` 全仓清零

- **解耦 `cap:fs` 与 `cap:sys`/`cap:io`/`cap:stream` 单文件符号冲突**：
  - 拆出 [`lib/sys/io_ops.cp`](file:///data/dev/cp/lib/sys/io_ops.cp) 与 [`lib/sys/fstat.cp`](file:///data/dev/cp/lib/sys/fstat.cp)，让 `lib/io/{file_stream,source,pipe}.cp` 精确导入最小子集，彻底消除与 `lib/fs.cp` 的顶层重名冲突，`test_core_runtime_bundler.cp` 恢复 100% PASS；
  - 消除 [`lib/fs.cp`](file:///data/dev/cp/lib/fs.cp)、[`lib/sys/copy_file.cp`](file:///data/dev/cp/lib/sys/copy_file.cp)、[`lib/which/which.cp`](file:///data/dev/cp/lib/which/which.cp) 顶层 `const builtin` 重复声明。
- **全仓 `lib/` 代码层 `std.posix` 彻底归零（Zig 0.17 兼容）**：
  - 清理最后 4 处残留：[`lib/cli/tty.cp`](file:///data/dev/cp/lib/cli/tty.cp)（`std.c.winsize`）、[`lib/sync/futex.cp`](file:///data/dev/cp/lib/sync/futex.cp)（`std.os.linux.errno`）、[`lib/dotenv/loader.cp`](file:///data/dev/cp/lib/dotenv/loader.cp)（`std.c.open/read`）、[`lib/event/file_poll.cp`](file:///data/dev/cp/lib/event/file_poll.cp)（`std.os.linux.poll*`）。
- **跨模块重名与局部变量遮蔽清理**：修正 `lib/url/`（`urlIndexOfChar`）、`lib/md/`（`MdFlags`）、`lib/event/deferred_task.cp`（`should_repeat`）、`lib/sql/`（`shift`）、`lib/redis/resp.cp`、`lib/shell/exec.cp`，以及 `test_io_streams.cp`/`test_image_all.cp`/`test_hex_url_case_router.cp`/`test_sys_complete.cp`。

### 编译器加固 (`cpc` Release 已部署)

- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：补齐 `"test_runner" => Some("test_runner/index.cp")`，全仓 **58/58（100%）** 标准库目录均已接入 `import cap from "cap"` 编译期按需惰性挂载路由；
- [`cpc/src/codegen/expr/ops.rs`](file:///data/dev/cp/cpc/src/codegen/expr/ops.rs)：在 `BinaryOp::LogicalOr` 的错误集合并判定中排除含比较符/空格的布尔表达式（防止 `kind == RESPType.Error || ...` 被误判为 Zig `||`）。

---

## [2026-11-20] - Phase B-3 `cap:pkg` 工业级包管理四大生产子系统直迁落地（13 模块 7,948 行）

### 标准库深度升级 (`cap:pkg` / `lib/pkg/`)

- 新增 4 个源自 Bun `src/install/` 的核心生产模块（累计 `lib/pkg/` 达 13 模块 **7,948 行**，0 `@import("bun")`，0 `std.posix`）：
  - [`lib/pkg/workspace.cp`](file:///data/dev/cp/lib/pkg/workspace.cp)（723 行）：Monorepo `WorkspaceGraph`/`CatalogMap`，基于 `cap:glob`+`cap:sys` 扫描 `workspaces`（数组/对象/取反排除）、解析 `workspace:*^~` 与 `catalog:`、成员依赖环检测与 DAG 拓扑排序；
  - [`lib/pkg/registry_client.cp`](file:///data/dev/cp/lib/pkg/registry_client.cp)（894 行）：`RegistryHttpClient`/`RetryPolicy`，支持作用域 Bearer/Basic 鉴权、ETag/304 磁盘元数据缓存复用与 429/5xx 指数退避重试；
  - [`lib/pkg/lifecycle.cp`](file:///data/dev/cp/lib/pkg/lifecycle.cp)（567 行）：默认零信任 `TrustedPolicy`（仅 `trustedDependencies` 白名单允许执行，统计拦截数）+ 基于 `PosixSpawn`+`Pipe` 的真实 `LifecycleRunner`；
  - [`lib/pkg/migration.cp`](file:///data/dev/cp/lib/pkg/migration.cp)（1,248 行）：`LockfileMigrator`（一键迁移 `package-lock.json` v2/v3、`yarn.lock` v1、`pnpm-lock.yaml` v6-v9 至原生 `Lockfile`）与 `DependencyAnalyzer`（`why`/`findDuplicates`/`auditIntegrity`）；
- 增量导出至 [`lib/pkg/index.cp`](file:///data/dev/cp/lib/pkg/index.cp)，更新 [`docs/pkg/pkg.md`](file:///data/dev/cp/docs/pkg/pkg.md) 与双轨规范手册；新增 [`cap/test/test_phase_b3_pkg_production.cp`](file:///data/dev/cp/cap/test/test_phase_b3_pkg_production.cp)（4/4 PASS），回归 `test_pkg_install_deep.cp`（6/6 PASS）。

---

## [2026-11-20] - Phase C-3 收官批次落地：`lib/resolver/` 生产缓存与高级特性 + 新建 `lib/test_runner/` 纯 CP 测试框架（+4,195 行）

### 标准库升级与新增 (Phase C-3 Direct Port)

- **`lib/resolver/` 深度升级（源自 Bun `src/resolver/`）**：
  - 新增 [`dir_cache.cp`](file:///data/dev/cp/lib/resolver/dir_cache.cp)（600 行：`DirEntryCache` 基于 `getdents64`+`StatHash` 目录缓存与 `RealpathResolver` 40 级软链环 `ELOOP` 防护）与 [`advanced.cp`](file:///data/dev/cp/lib/resolver/advanced.cp)（791 行：`TsConfigChainLoader` 多级 `extends` 继承合并、`BrowserFieldMap`、`SideEffectsMatcher`、`SelfReferenceResolver`）；更新 [`docs/resolver/resolver.md`](file:///data/dev/cp/docs/resolver/resolver.md)。
- **新建 `lib/test_runner/`（`cap:test_runner`，源自 Bun `src/test_runner/`，零 JSC 耦合）**：
  - [`order.cp`](file:///data/dev/cp/lib/test_runner/order.cp)（155 行：SplitMix64/Xoshiro256++ 确定性种子洗牌 `TestOrder`）、[`suite.cp`](file:///data/dev/cp/lib/test_runner/suite.cp)（631 行：树形嵌套 `describe`、`beforeAll`/`beforeEach`/`afterEach`/`afterAll`、`.only`/`.skip`/`.todo` 过滤与纳秒计时 `TestRunner`）、[`snapshot.cp`](file:///data/dev/cp/lib/test_runner/snapshot.cp)（333 行：Bun 风格 `.snap` 快照引擎 `SnapshotStore` + `DiffFormatter` 彩色差分）、[`reporter.cp`](file:///data/dev/cp/lib/test_runner/reporter.cp)（541 行：`PrettyFormat` 与 TAP v13 / JUnit XML / JSON `TestReporter`）及 [`docs/test_runner/test_runner.md`](file:///data/dev/cp/docs/test_runner/test_runner.md)。
- **测试与规范**：新增 [`cap/test/test_phase_c3_resolver_testrunner_port.cp`](file:///data/dev/cp/cap/test/test_phase_c3_resolver_testrunner_port.cp)（5/5 PASS），回归 `test_meta_platform_sri_deep.cp`（5/5）与 `test_resolver_bundler_deep.cp`（7/7）全部 PASS；同步更新双轨规范手册。

---

## [2026-11-20] - Phase E-1 原版直迁落地：`lib/mem/`、`lib/cli/`、`lib/bundler/` 生产级分配器/终端框架/代码分割增强（+4,043 行）

### 标准库增强 (Phase E-1 Direct Port)

- **`lib/mem/`**（源自 Bun `bun_alloc/`）：新增 [`allocation_scope.cp`](file:///data/dev/cp/lib/mem/allocation_scope.cp)（438 行：`AllocationScope` 泄漏/峰值/重复与悬空释放追踪分配器）与 [`fallback_allocator.cp`](file:///data/dev/cp/lib/mem/fallback_allocator.cp)（566 行：`BufferFallbackAllocator<N>` 栈优先热路径+堆回退、`MaybeOwned<T>`、Linux `memfd_create`+`mmap` `MemFdBuffer`），同步更新 [`docs/mem/mem.md`](file:///data/dev/cp/docs/mem/mem.md)；
- **`lib/cli/`**（源自 Bun `cli/ci_info.cp` & `bun_core/Progress.cp`）：新增 [`ci_info.cp`](file:///data/dev/cp/lib/cli/ci_info.cp)（366 行：15+ CI 厂商/PR/ANSI/OSC-8/`TIOCGWINSZ` 探测）与 [`subcommand.cp`](file:///data/dev/cp/lib/cli/subcommand.cp)（688 行：多级 `SubcommandRouter` + Levenshtein 拼写纠错 + 彩色 `--help` + 原子锁 `MultiProgress`/`Spinner`），同步更新 [`docs/cli/cli.md`](file:///data/dev/cp/docs/cli/cli.md)；
- **`lib/bundler/`**（源自 Bun `bundler/`）：新增 [`metafile.cp`](file:///data/dev/cp/lib/bundler/metafile.cp)（418 行：esbuild/Bun 兼容 `Metafile` JSON 与彩色体积占比树状表）与 [`splitting.cp`](file:///data/dev/cp/lib/bundler/splitting.cp)（403 行：多入口 `u64` 位掩码共享 Chunk 抽取 `CodeSplitter` 与 `AssetManifest`），同步更新 [`docs/bundler/bundler.md`](file:///data/dev/cp/docs/bundler/bundler.md)；
- 新增 [`cap/test/test_phase_e1_mem_cli_bundler_port.cp`](file:///data/dev/cp/cap/test/test_phase_e1_mem_cli_bundler_port.cp)（5/5 PASS），回归 `test_string_blob_image_mem_deep.cp`（5/5）与 `test_ffi_bigint_bench_cli_deep.cp`（4/4）全部 PASS，同步更新双轨规范手册。

---

## [2026-11-20] - Phase D-1 LightningCSS 工业引擎直迁（`lib/css/engine/` 4.8K）与 `bun_agy` 双工单热修结单

### 标准库：`lib/css/engine/` LightningCSS 核心引擎（7 模块 4,803 行，源自 Bun `src/css/`）

- 在零触碰既有 8 个 `lib/css/*.cp` 文件（保障下游 `capsurf_agy` 0 抖动）前提下，新建 [`lib/css/engine/`](file:///data/dev/cp/lib/css/engine/index.cp)：
  - [`targets.cp`](file:///data/dev/cp/lib/css/engine/targets.cp)（853 行）：9 大浏览器 24 位版本矩阵、`Feature` 27 项现代特性兼容判定与精准前缀计算；
  - [`values.cp`](file:///data/dev/cp/lib/css/engine/values.cp)（1,038 行）：`Angle`/`Time`/`Resolution`/`Ratio`/`Position`、`EasingFunction`（`cubic-bezier` 牛顿-二分求值/`steps()`）与 `Gradient`（Linear/Radial/Conic）；
  - [`properties.cp`](file:///data/dev/cp/lib/css/engine/properties.cp)（1,169 行）：`RectShorthand`/`BorderShorthand`/`BorderRadius`/`BoxShadowList`/`TransformList`+`Matrix2D`/`FlexShorthand`/`TransitionItem`/`AnimationItem`/`DisplayValue`/`FilterList` 简写展开与压缩；
  - [`rules.cp`](file:///data/dev/cp/lib/css/engine/rules.cp)（900 行）：`SupportsAst`、`MediaRangeQuery`（L4 范围语法降级）、`ContainerQuery`、`LayerManager`、`FontFaceRule`、`PropertyRule`、`ScopeRule`；
  - [`compiler.cp`](file:///data/dev/cp/lib/css/engine/compiler.cp)（674 行）：`CssModulesHasher` 与 `CssCompiler.compile()` 全管线；增量导出至 [`lib/css/index.cp`](file:///data/dev/cp/lib/css/index.cp)，更新 [`docs/css/css.md`](file:///data/dev/cp/docs/css/css.md)；
  - 新增 [`cap/test/test_phase_d_css_lightning_port.cp`](file:///data/dev/cp/cap/test/test_phase_d_css_lightning_port.cp)（6/6 PASS），全部 4 套 CSS 套件 22/22 PASS。

### 编译器极速热修 (`cpc` Release 已部署，结单 `bun_agy` 2 条工单)

- **`task-bun-20261120-fnptr-ret-union`**：[`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)、[`cpc/src/modular.rs`](file:///data/dev/cp/cpc/src/modular.rs)、[`cpc/src/codegen/types/map_type.rs`](file:///data/dev/cp/cpc/src/codegen/types/map_type.rs) 将 `union`/`union(enum)` 统一登记为值语义类型，不再错误加 `*`（`*const fn (*R) Action` 正确输出）；
- **`task-bun-20261120-fn-callconv-ptr`**：[`cpc/src/parser/types.rs`](file:///data/dev/cp/cpc/src/parser/types.rs) 在 `parse_fn_type_sig_body` 中吸收 `)` 后的 `callconv(.c)` 再解析返回类型；[`cpc/src/codegen/zig_escape.rs`](file:///data/dev/cp/cpc/src/codegen/zig_escape.rs) 在已有 `*const` 前缀时不再重复插入 `*const` 消除双层 `*const *const fn`。

---

## [2026-11-20] - Phase B-2 & Phase C-2 双路大里程碑落地：`lib/shell/`（7.1K）、`lib/http/`（7.5K）与 `lib/conf/`（4.5K）工业级原版直迁

### `lib/shell/` 工业级跨平台 Shell 引擎（10 文件 7,102 行，直迁 Bun `shell/` + `shell_parser/`）

- 新增 [`arith.cp`](file:///data/dev/cp/lib/shell/arith.cp)（442 行，`$((...))` 算术展开）、[`braces.cp`](file:///data/dev/cp/lib/shell/braces.cp)（419 行，大括号/序列展开）、[`builtin_fs.cp`](file:///data/dev/cp/lib/shell/builtin_fs.cp)（781 行）与 [`builtin_text.cp`](file:///data/dev/cp/lib/shell/builtin_text.cp)（773 行），升级 `lexer`/`parser`/`env`/`exec`/`builtin`：
  - 完整 POSIX 展开（`${VAR:-/=/+/ #}`、`$(cmd)`、`$((arith))`、`{a..z}`、Glob）、控制流（`if`/`for`/`while`/`until`/`&&`/`||`/`!`）、子 Shell 环境与 `cwd` 隔离、多级 Builtin+`PosixSpawn` 混合管道、全套重定向（`>`/`>>`/`<`/`2>`/`2>&1`/`<<<`）与 30+ 生产内置命令；
  - 根治孤立 `[` 命令误触 Glob 递归遍历与 `set --`/`shift` 位置参数悬空指针两处深层缺陷；
  - 新增 [`cap/test/test_phase_b2_shell_port.cp`](file:///data/dev/cp/cap/test/test_phase_b2_shell_port.cp)（4/4 PASS），3 套测试共 12/12 PASS，同步更新 [`docs/shell/shell.md`](file:///data/dev/cp/docs/shell/shell.md)。

### `lib/http/`（37 文件 7,492 行）与 `lib/conf/`（9 文件 4,515 行）工业级直迁（源自 Bun `http/`、`http_types/`、`interchange/`）

- **`lib/http/`**：新增 [`status.cp`](file:///data/dev/cp/lib/http/status.cp)（64 项 RFC 9110 状态码表）、[`header_builder.cp`](file:///data/dev/cp/lib/http/header_builder.cp)、[`header_parsers.cp`](file:///data/dev/cp/lib/http/header_parsers.cp)（`CacheControl`/`ContentDisposition`/`Accept`/`Range`/`ContentRange`）、[`wire_http.cp`](file:///data/dev/cp/lib/http/wire_http.cp)（`HttpRequestBuilder`/`HttpResponseParser`）；升级 [`mime.cp`](file:///data/dev/cp/lib/http/mime.cp)（201 项 `MIME_DATABASE` + `sniffMagic`）与 `lib/http/h2/`（RFC 7541 61 静态表 + 257 项 Huffman 编解码 + 动态表驱逐 + H2 `SETTINGS`/`WINDOW_UPDATE`/`GOAWAY`/`PING`/`RST_STREAM` 流控）；
- **`lib/conf/`**：新增 [`value.cp`](file:///data/dev/cp/lib/conf/value.cp)（统一 `ConfValue`/`ConfDateTime` RFC 3339）与 [`yaml_parser.cp`](file:///data/dev/cp/lib/conf/yaml_parser.cp)（完整 YAML 1.2：多文档流、锚点 `&`/别名 `*`/合并键 `<<`、`|`/`>`/`|-`/`>+` 块标量、流式集合、类型标签与双向序列化）；升级 [`toml.cp`](file:///data/dev/cp/lib/conf/toml.cp)（TOML 1.0 内联表、点分/引号键、RFC 3339 日期时间与 `[[array]]`）；
- 新增 [`cap/test/test_phase_c2_http_interchange_port.cp`](file:///data/dev/cp/cap/test/test_phase_c2_http_interchange_port.cp)（4/4 PASS），7 套测试共 31/31 PASS；同步更新 [`docs/http/http.md`](file:///data/dev/cp/docs/http/http.md)、[`docs/conf/conf.md`](file:///data/dev/cp/docs/conf/conf.md) 及双轨规范手册。

---

## [2026-11-20] - Phase C-1 工业级原版直迁落地：`lib/md/`（21 文件 11,735 行完整 MD4C 引擎）与 `lib/sql/`（10 文件 4,718 行多数据库协议栈）

### 标准库原版直迁 (Phase C-1 Direct Port)

- **`lib/md/`（21 文件 11,735 行，完整直迁 Bun `src/md/` 全部 16 模块，0 `@import("bun")`）**：
  - 新增 [`entity.cp`](file:///data/dev/cp/lib/md/entity.cp)（2,167 行，2,125 条 W3C HTML5 命名实体）、[`unicode.cp`](file:///data/dev/cp/lib/md/unicode.cp)（1/2/3 码点 case-folding）、[`helpers.cp`](file:///data/dev/cp/lib/md/helpers.cp)（335 组 Unicode 标点与 `HeadingIdTracker`）、[`blocks.cp`](file:///data/dev/cp/lib/md/blocks.cp)（双遍块解析）、[`inlines.cp`](file:///data/dev/cp/lib/md/inlines.cp)（O(N) 强调栈）、[`links.cp`](file:///data/dev/cp/lib/md/links.cp)、[`autolinks.cp`](file:///data/dev/cp/lib/md/autolinks.cp)、[`ref_defs.cp`](file:///data/dev/cp/lib/md/ref_defs.cp)、[`html_renderer.cp`](file:///data/dev/cp/lib/md/html_renderer.cp)（`heading_ids`/`tag_filter`/`sanitize_html`）、[`ansi_renderer.cp`](file:///data/dev/cp/lib/md/ansi_renderer.cp)（1,829 行：折行/CJK 宽度/Unicode 方框表/OSC 8/Kitty 图片）、[`engine.cp`](file:///data/dev/cp/lib/md/engine.cp) 等 16 个完整模块，保留原有 `Markdown`/`MdTable` 100% 兼容。
- **`lib/sql/`（10 文件 4,718 行，直迁 Bun `src/sql/` 协议状态机，0 `@import("bun")`）**：
  - 新增 [`shared.cp`](file:///data/dev/cp/lib/sql/shared.cp)（15B SSO `SqlData`）、[`pg_types.cp`](file:///data/dev/cp/lib/sql/pg_types.cp)（90+ `PgTypeTag` OID、`PgBinaryCodec` 覆盖 numeric/uuid/jsonb/int4_array）、[`pg_protocol.cp`](file:///data/dev/cp/lib/sql/pg_protocol.cp)（SSL/Cancel/StartupUTF8/Copy/Notify/Notice）、[`mysql_types.cp`](file:///data/dev/cp/lib/sql/mysql_types.cp)、[`mysql_protocol.cp`](file:///data/dev/cp/lib/sql/mysql_protocol.cp)（Native SHA-1、`CachingSha2` FastAuth、ColumnDef41、`COM_STMT_*`、`BinaryRowDecoder`）；增强 [`sqlite.cp`](file:///data/dev/cp/lib/sql/sqlite.cp) 事务与参数绑定。
- **文档、规范与验收**：更新 [`docs/md/md.md`](file:///data/dev/cp/docs/md/md.md)、[`docs/sql/sql.md`](file:///data/dev/cp/docs/sql/sql.md) 及双轨规范手册；新增 [`cap/test/test_phase_c1_md_sql_port.cp`](file:///data/dev/cp/cap/test/test_phase_c1_md_sql_port.cp)（8/8 PASS），5 套新建及存量回归测试共 29/29 用例 100% PASS，31/31 文件 `// Source:` 合规。

---

## [2026-11-20] - Phase B-1 工业级包管理引擎直迁落地：新建 `lib/pkg/`（`cap:pkg`，9 模块 4,512 行，源自 Bun `install/`）

### 新增标准库 (`cap:pkg` / `cap.pkg` / `cap.install`)

- **工业级包管理子系统（9 模块 4,512 行，0 `@import("bun")`，0 `std.posix`）**：
  - [`lib/pkg/types.cp`](file:///data/dev/cp/lib/pkg/types.cp)（496 行）：`SemverString`（8B 内联/外部双态字符串）、`StringBuf`（Wyhash 去重池）、`ExternalString`/`SlicedString`/`ExternalSlice<T>`、`NodeLinker`、`Behavior` 位域；
  - [`lib/pkg/dependency.cp`](file:///data/dev/cp/lib/pkg/dependency.cp)（603 行）：`DependencyVersion.parse` 全 10 种协议（semver、`npm:alias`、`workspace:`、`file:`、`link:`、tarball URL、`github:`、`git+ssh:`、dist-tag、`catalog:`）与 `DependencyList`；
  - [`lib/pkg/resolution.cp`](file:///data/dev/cp/lib/pkg/resolution.cp)（562 行）：`Resolution`、`OperatingSystem`/`Architecture`/`Libc` 平台位掩码（支持 `!` 取反）、`PackageMeta`、`BinLink`；
  - [`lib/pkg/npm.cp`](file:///data/dev/cp/lib/pkg/npm.cp)（540 行）：`.npmrc` 作用域 Registry 与 Bearer/Basic Auth 路由、`NpmPackageManifest.parsePackument` 与结合 `cap:semver` 的 `resolveVersion`；
  - [`lib/pkg/lockfile.cp`](file:///data/dev/cp/lib/pkg/lockfile.cp)（1,057 行）：DAG 依赖图、环检测、拓扑排序与双格式持久化（确定性 JSON `cap.lock` + 紧凑二进制 `CPLK0001` 含 CRC32 完整性校验）；
  - [`lib/pkg/tarball.cp`](file:///data/dev/cp/lib/pkg/tarball.cp)（294 行）：`TarballBuilder`/`TarballExtractor`（gzip + tar + W3C SRI 校验 + 严格 `..`/绝对路径/软链逃逸防护）；
  - [`lib/pkg/linker.cp`](file:///data/dev/cp/lib/pkg/linker.cp)（331 行）：`Hoisted` 与 pnpm 风格 `Isolated` 链接策略、硬链优先+拷贝回退、`BinLinker`（`.bin` `0o755`）与 `patch_install`（先解除硬链防污染共享缓存 + 幂等标记）；
  - [`lib/pkg/manager.cp`](file:///data/dev/cp/lib/pkg/manager.cp)（615 行）与 [`lib/pkg/index.cp`](file:///data/dev/cp/lib/pkg/index.cp)：`PackageManager` 端到端安装闭环。
- **文档、规范与端到端测试**：新增 [`docs/pkg/pkg.md`](file:///data/dev/cp/docs/pkg/pkg.md) 与 [`cap/test/test_pkg_install_deep.cp`](file:///data/dev/cp/cap/test/test_pkg_install_deep.cp)（6/6 PASS，含真实磁盘 Tarball 打包解包、SRI 篡改拦截、路径穿越防护、Hoisted/Isolated 落盘与补丁应用），同步更新 [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) 与 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)。

---

## [2026-11-20] - Phase A 底层三件套 Bun 原版直迁落地：`lib/sys/`、`lib/io/`、`lib/string/` 全面对齐生产契约（34 文件 9,273 行）

### 底层基石原版升级 (Phase A Foundation Direct Port)

- **`lib/sys/`（11 文件，3,118 行，0 `@import("bun")`，0 `std.posix`）**：
  - 新增 [`lib/sys/error.cp`](file:///data/dev/cp/lib/sys/error.cp)（`SyscallTag`、`SysError`、`Maybe<T>`）与 [`lib/sys/dir.cp`](file:///data/dev/cp/lib/sys/dir.cp)（Linux `getdents64` 8KB 栈内联零堆 `DirIterator`）；
  - 升级 [`fd.cp`](file:///data/dev/cp/lib/sys/fd.cp)、[`stat.cp`](file:///data/dev/cp/lib/sys/stat.cp)、[`file.cp`](file:///data/dev/cp/lib/sys/file.cp)、[`copy_file.cp`](file:///data/dev/cp/lib/sys/copy_file.cp)、[`fs_ops.cp`](file:///data/dev/cp/lib/sys/fs_ops.cp)、[`process.cp`](file:///data/dev/cp/lib/sys/process.cp)：全量 `*at` 原子系统调用、`EINTR` 自动重试、`mkdirAll`/`rmdirRecursive`、`CopyFileState`/`sendfile`、`poll`/`epoll`/`pipe2` 及 `PosixSpawn`（支持 `cwd`/`envp`/FD 重定向/`PATH` 搜索/子进程 errno 管道回传）。
- **`lib/io/`（8 文件，2,067 行）**：
  - 新增 [`lib/io/source.cp`](file:///data/dev/cp/lib/io/source.cp)（`IoStatus`/`ReadState`/`WriteStatus`/`PollState`/`IoSource`）与 [`lib/io/file_stream.cp`](file:///data/dev/cp/lib/io/file_stream.cp)（`FileReader`/`FileWriter` 含 `pread`/`pwrite`/`copy_file_range`/`sendfile`）；
  - 升级 [`pipe.cp`](file:///data/dev/cp/lib/io/pipe.cp)（`pipe2(O_CLOEXEC|O_NONBLOCK)` 非阻塞 `EAGAIN`/`BrokenPipe` 状态机）、[`max_buf.cp`](file:///data/dev/cp/lib/io/max_buf.cp)、[`buffer_stream.cp`](file:///data/dev/cp/lib/io/buffer_stream.cp)（`readUntilDelimiter`/`shift`/`peek`）。
- **`lib/string/`（15 文件，4,088 行）**：
  - 新增 [`lib/string/unicode.cp`](file:///data/dev/cp/lib/string/unicode.cp)（`Unicode` 分类/大小写折叠与零堆 `CodepointIterator`）与 [`lib/string/mutable_string.cp`](file:///data/dev/cp/lib/string/mutable_string.cp)（`MutableString`）；
  - 升级 [`utils.cp`](file:///data/dev/cp/lib/string/utils.cp)（全套 Bun `strings.*` 匹配/切分/拼接/修剪）、[`utf.cp`](file:///data/dev/cp/lib/string/utf.cp)（`Wtf8`、Latin-1/UTF-16/UTF-8 互转）、[`builder.cp`](file:///data/dev/cp/lib/string/builder.cp)（`StringBuilder` 预估容量零重分配）、[`escape.cp`](file:///data/dev/cp/lib/string/escape.cp)（`escapeHtml`/`escapeRegExp`/`escapeJsAlloc`）。
- **文档、规范与验证**：同步更新 [`docs/sys/sys.md`](file:///data/dev/cp/docs/sys/sys.md)、[`docs/io/io.md`](file:///data/dev/cp/docs/io/io.md)、[`docs/string/string.md`](file:///data/dev/cp/docs/string/string.md) 与双轨规范手册；新增 [`cap/test/test_phase_a_foundation_port.cp`](file:///data/dev/cp/cap/test/test_phase_a_foundation_port.cp)（3/3 PASS），5 套新建与既有回归共 23/23 用例 100% PASS，34/34 文件 `// Source:` 100% 合规。

---

## [2026-11-20] - 核心架构里程碑：`import cap from "cap"` 编译期按需惰性挂载容器与 `lib/_cap/core.cp` 微核心落地

### 编译器按需惰性挂载引擎 (`cpc`)

- **精准词法扫描与传递不动点按需挂载**：
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：新增 59 个标准库权威路由表 `resolve_cap_hub_target`、词法级 `scan_cap_hub_members`（精确跳过注释/字符串/模板字面量，支持任意别名 `import c from "cap"`）、顶层 `pub` 符号提取器与 `resolve_imports_with_cap_hub`；
  - [`cpc/src/modular.rs`](file:///data/dev/cp/cpc/src/modular.rs)：增量缓存升至 `v2`（追踪 `uses_cap_hub` / `cap_hub_members`），BFS 阶段执行不动点按需发现，动态合成 `<emit_dir>/_lib/_cap/index.zig`（仅挂载本次编译实际被引用的 `cap.<ns>` 子库与 `core` 显式重导出，不用 Zig 0.17 已废弃的 `usingnamespace`，以 `write_if_changed` 护航增量 `mtime`）；
  - [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs)、[`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)、[`cpc/src/codegen/expr/builtin.rs`](file:///data/dev/cp/cpc/src/codegen/expr/builtin.rs)：单文件模式同步支持按需挂载独立命名空间与 `@import("cap")` 映射，并修复 `@expect_eq` 在 `--emit-dir` 模式下通过 `rt_sym("cp_eq")` 引用共享运行时。

### `cap` 极薄微核心底座 (`lib/_cap/`)

- [`lib/_cap/core.cp`](file:///data/dev/cp/lib/_cap/core.cp) & [`lib/_cap/index.cp`](file:///data/dev/cp/lib/_cap/index.cp)：提供零重依赖微核心原语（`OOM`/`default_allocator`/`handleOom`/`Environment`/`assert`/`Global`/`take`/`create`/`destroy`/`copy`/`clamp`/`cast`/`FD`/`invalid_fd`/`Mode`/`PathBuffer`/`O`/`S`/`Output`/`ComptimeStringMap`/`fmtIntSize`）；
- [`docs/core/cap_hub.md`](file:///data/dev/cp/docs/core/cap_hub.md)：完整架构与使用手册；同步更新 [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) 与 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)。

### 铁证验收

- [`cap/test/test_cap_lazy_hub.cp`](file:///data/dev/cp/cap/test/test_cap_lazy_hub.cp)：单文件 `cap test`（2/2 PASS）与 `-m` 模块模式（`total=7`，manifest 与 `_lib/_cap/index.zig` 仅挂载 `bits`/`errno`/`hash` + `core`，其余 50+ 未引用库 0 加载）全部通过。

---

## [2026-11-20] - 热修与实测闭环：修复 `-m` 模块模式 `struct.name()` 方法调用误改写与嵌套定长数组重复字面量降级，真实 CapSurf 81 模块 `-m -d` 全量原生链接 100% 通过

### 编译器修复 (`cpc`)

- **修复结构体 `.name()` / `.value()` 方法调用误触发枚举属性改写**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)、[`cpc/src/codegen/expr/call.rs`](file:///data/dev/cp/cpc/src/codegen/expr/call.rs)、[`cpc/src/codegen/expr/control.rs`](file:///data/dev/cp/cpc/src/codegen/expr/control.rs)：引入 `in_call_callee` 标记（进入 `Expr::Member` 评估 `member.obj` 前即刻清零），确保方法调用 `obj.name()` / `obj.value()` 永远发射为合法 Zig 方法调用，不再误落入枚举属性改写 `(if (@typeInfo(...) == .@"enum") @tagName(...) else obj.name)()`（解决 CapSurf `-m` 模式 `IsolatedWorldEntry.name()` 报错），同时 100% 保持枚举属性 `.name` / `.value` 兼容。
- **修复嵌套固定数组重复字面量（如 `[[0; 128]; 16]`）类型泄露**：
  - [`cpc/src/codegen/expr/mod.rs`](file:///data/dev/cp/cpc/src/codegen/expr/mod.rs)：在 `Expr::ArrayRepeat` 递归降级元素值前逐层剥离外层 `Type::FixedArray`/`Type::Array` 期望类型，解决 CapSurf `session.cp` 中 `binding_names: [16][128]u8 = [[0; 128]; 16]` 内层误套 `[128]u8` 的问题。
- **发布与实测验证**：
  - 已通过 `bash scripts/sync_cpc.sh release` 原子部署 Release 正式版；
  - 新增回归测试 [`cap/test/test_struct_name_and_nested_repeat.cp`](file:///data/dev/cp/cap/test/test_struct_name_and_nested_repeat.cp)（1/1 PASS，978ms）；
  - 真实 CapSurf（81 模块）`-m` 全量验证：冷发射 `2.67s`、热发射 `12ms`、`-O Debug -fno-llvm` 原生链接 `1.86s`（热缓存命中 `0.02s`）并成功运行 `--help`。

---

## [2026-11-20] - Batch 6A（收官批次）：新增 `cap:meta`、`cap:platform` 与 `cap:resolver` SRI/HostedGit，全仓 Bun 资产吸收与对标审核圆满收官

### 新增与补强标准库 (Final Bun Assets Absorption)

- **编译期元编程与位操作套件 (`cap:meta`)**：
  - [`lib/meta/bits.cp`](file:///data/dev/cp/lib/meta/bits.cp)：2 的幂对齐、位计数/前导尾随零、位翻转/旋转、端序打包解包与 `@packed` 位域辅助运算；
  - [`lib/meta/traits.cp`](file:///data/dev/cp/lib/meta/traits.cp)：适配 Zig 0.17 `@typeInfo` 的完整编译期类型特征探测（`isInt`/`isSlice`/`isPointer`/`hasField`/`hasDecl`/`ChildType`/`ItemType` 等）；
  - [`lib/meta/index.cp`](file:///data/dev/cp/lib/meta/index.cp) & [`docs/meta/meta.md`](file:///data/dev/cp/docs/meta/meta.md)。
- **宿主平台与内核特性探测 (`cap:platform`)**：
  - [`lib/platform/os_info.cp`](file:///data/dev/cp/lib/platform/os_info.cp)：`OsKind`/`ArchKind`/`Endianness`、`KernelVersion` 与 `PlatformInfo`（Linux 下直调 `std.os.linux.uname` 解析真实内核版本、`io_uring`/`copy_file_range`/`statx`/`pidfd` 能力开关、`readlink("/proc/self/exe")` 与 `hostname`）；
  - [`lib/platform/index.cp`](file:///data/dev/cp/lib/platform/index.cp) & [`docs/platform/platform.md`](file:///data/dev/cp/docs/platform/platform.md)。
- **`cap:resolver` 增量补强**：
  - [`lib/resolver/integrity.cp`](file:///data/dev/cp/lib/resolver/integrity.cp)：W3C SRI `Integrity`（SHA-1/256/384/512 计算、多值择优解析与恒定时间 XOR 校验）；
  - [`lib/resolver/hosted_git.cp`](file:///data/dev/cp/lib/resolver/hosted_git.cp)：`HostedGitInfo`（GitHub/GitLab/Bitbucket/Gist/SourceHut 短写/HTTPS/SSH 解析与 Tarball URL 生成）；
  - 增量导出至 [`lib/resolver/index.cp`](file:///data/dev/cp/lib/resolver/index.cp) 并更新 [`docs/resolver/resolver.md`](file:///data/dev/cp/docs/resolver/resolver.md)。

### 全仓收官终检与双轨规范定稿

- 新增 [`cap/test/test_meta_platform_sri_deep.cp`](file:///data/dev/cp/cap/test/test_meta_platform_sri_deep.cp)（5/5 PASS）及回归 `test_resolver_bundler_deep.cp`（7/7 PASS）；
- 全仓 `lib/**/*.cp` 100% 首行 `// Source:` 溯源合规、0 处 `@import("bun")`；同步定稿 [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) 与 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)。

---

## [2026-11-20] - Batch 5B：Tier-1 18 大核心标准库终审体检与 7 模块缺口补齐

### 标准库终审与精准补缺 (Tier-1 Parity Audit & Gap Fill)

- **终审确认完备（11 个）**：`cap:pack`、`cap:crypto`、`cap:net`、`cap:conf`、`cap:event`、`cap:security`、`cap:clap`、`cap:hash`、`cap:ptr`、`cap:sync`、`cap:col`；
- **精准补齐缺口（7 个，100% 向后兼容）**：
  - [`lib/s3/`](file:///data/dev/cp/lib/s3/client.cp)：`S3RequestBuilder.buildSignedRequest` 补齐全部 8 项可选头字母序 SigV4 签名，`buildPresignedUrl` 支持 STS `session_token` 与自定义查询串，`S3Client.presignPartUrl` 分片预签名；
  - [`lib/dns/`](file:///data/dev/cp/lib/dns/packet.cp)：`DnsWireCodec` 新增 `decodeQuestion`/`decodeResourceRecord`/A/AAAA/CNAME/MX/SRV/TXT 记录解析与 IPv4/IPv6 格式化，`DnsCache` 新增 IPv6 `putAAAA`/`lookupAAAA`，`HostsTable` 新增 `lookupIPv6`；
  - [`lib/encoding/`](file:///data/dev/cp/lib/encoding/utf16be.cp)：新增 `utf16be.cp`（`Utf16BE` 大端编解码与 `Ascii` 工具）并在 `index.cp` 聚合导出 `PercentEncoding`；
  - [`lib/redis/`](file:///data/dev/cp/lib/redis/resp.cp)：补齐 RESP3 `Double`/`BigNumber`/`BlobError`/`VerbatimString`/`Push`/`Attribute` 编解码、`ValkeyReader.skipFrame`、`PMessage`/`PSubscribe` 与 `HELLO`/PubSub/`INCR`/`TTL`/`SELECT`/`QUIT` 命令；
  - [`lib/url/`](file:///data/dev/cp/lib/url/url.cp)：`URL` 新增 RFC 3986 `normalizePath`/`resolve`/`parseWithBase`/`formatOrigin`/`format`，`URLSearchParams` 新增 `setUnique`/`hasValue`/`deleteValue`/`clear`/`toEncodedString`；
  - [`lib/which/`](file:///data/dev/cp/lib/which/which.cp)：`WhichOptions.pathext`、`endsWithPathext` 与多路径全量检索 `whichAll`；
  - [`lib/watcher/`](file:///data/dev/cp/lib/watcher/watcher.cp)：`WatchOptions` 忽略配置、`WatchFilter` 隐藏文件/通配过滤，并正式接入 `Watcher` 事件分发与 `poll` 流水线。
- **文档、规范与测试**：同步更新 7 份 `docs/<mod>/<mod>.md` 及双轨规范手册 [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)、[`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)；新增 [`cap/test/test_tier1_parity_deep.cp`](file:///data/dev/cp/cap/test/test_tier1_parity_deep.cp)（6/6 PASS），联合 12 套存量回归测试全部 100% PASS。

---

## [2026-11-20] - Batch 5A：`cap:css` 全面去 `@import("bun")` 化重构（清理 94 个死文件，重建 8 模块纯 CP CSS 全栈引擎）

### 标准库重构与清理 (`cap:css`)

- **彻底清除 Bun 耦合死文件**：
  - 清理 `lib/css/` 下 94 个无法独立编译的 Bun/Bundler/JSC 耦合残留文件（`properties/`、`rules/`、`selectors/`、`values/` 及根目录 15 个遗留文件）；全仓 `lib/**/*.cp` 实现 **`@import("bun")` 100% 清零**。
- **8 模块纯 CP 零耦合 CSS 全栈引擎（共 4,157 行，100% 向后兼容）**：
  - [`lib/css/tokenizer.cp`](file:///data/dev/cp/lib/css/tokenizer.cp)（新增，541 行）：CSS Syntax Level 3 零分配流式词法器（25 种 `CssTokenKind`、`url()`、转义、注释与行列追踪）；
  - [`lib/css/variables.cp`](file:///data/dev/cp/lib/css/variables.cp)（新增，211 行）：`CssVariableMap` 与 `CssVarResolver.resolve()`（递归 `var(--name, fallback)` 与 16 层循环防护）；
  - [`lib/css/prefixes.cp`](file:///data/dev/cp/lib/css/prefixes.cp)（重构，321 行）：`CssVendorPrefix`、`CssPrefixer` 与 `CssLogical` 逻辑属性展开；
  - [`lib/css/color.cp`](file:///data/dev/cp/lib/css/color.cp)（589 行）：149 个 W3C 命名色（显式 `[149]` 定长数组二分查找，同步解决 `capsurf_agy` 工单 `task-capsurf-20261120-lib-css-color-tuple`）、`hwb()`/`oklab()`/`oklch()`、角度单位与 `lighten`/`darken`/`toHsl`；
  - [`lib/css/length.cp`](file:///data/dev/cp/lib/css/length.cp)（389 行）：23 种 `CssUnit` 与递归下降 `CssCalc.evalPx`（括号、`min()`/`max()`/`clamp()`、一元符号）；
  - [`lib/css/selector.cp`](file:///data/dev/cp/lib/css/selector.cp)（512 行）：6 种属性选择器运算符、`:is()/:not()/:has()/:where()` 权重计算与 `CssSelectorList`；
  - [`lib/css/stylesheet.cp`](file:///data/dev/cp/lib/css/stylesheet.cp)（1,085 行）：15 种 `CssRuleKind`、`CssMediaQuery.matchesFull`、`CssNesting.flatten` 嵌套展平、`CssStylesheet` AST、`minify` 与 `formatPretty`；
  - [`lib/css/index.cp`](file:///data/dev/cp/lib/css/index.cp)（12 行）与 [`docs/css/css.md`](file:///data/dev/cp/docs/css/css.md)（160 行）。
- **测试与双轨规范**：新增 [`cap/test/test_css_deep.cp`](file:///data/dev/cp/cap/test/test_css_deep.cp)（7/7 PASS），回归 `test_css.cp`（5/5）与 `test_css_full.cp`（4/4）100% PASS；同步更新 [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) 与 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)。

---

## [2026-11-20] - Batch 4 增量高价值 Bun 资产吸收：`cap:resolver`、`cap:aio` 新建与 `cap:bundler`/`cap:safety`/`cap:patch` 深度补强

### 新增标准库 (New Stdlib Modules)

- **模块解析器与包配置子系统 (`cap:resolver`)**：
  - [`lib/resolver/data_url.cp`](file:///data/dev/cp/lib/resolver/data_url.cp)：RFC 2397 `DataURL`（MIME/Charset/Base64/Percent 解码与 Base64 编码）；
  - [`lib/resolver/package_json.cp`](file:///data/dev/cp/lib/resolver/package_json.cp)：`PackageJson` 完整解析与条件 `exports`/`imports`（含数组回退、`null` 禁用与最长前缀通配符）路由；
  - [`lib/resolver/tsconfig.cp`](file:///data/dev/cp/lib/resolver/tsconfig.cp)：JSONC 注释/尾逗号剥离与 `TsConfigJson` `paths` 通配符别名重写；
  - [`lib/resolver/resolver.cp`](file:///data/dev/cp/lib/resolver/resolver.cp) & [`lib/resolver/index.cp`](file:///data/dev/cp/lib/resolver/index.cp)：`StatHash`（基于 `statx` + XxHash64）与统一 `ModuleResolver`（`cap:*`/`node:*`/`bun:*`、`data:`、`#imports`、`tsconfig` 别名与 `cap_modules`/`node_modules` 递归解析）；
  - [`docs/resolver/resolver.md`](file:///data/dev/cp/docs/resolver/resolver.md)：完整 API 手册。
- **异步 I/O 队列与父进程看门狗 (`cap:aio`)**：
  - [`lib/aio/watchdog.cp`](file:///data/dev/cp/lib/aio/watchdog.cp)：`ParentDeathWatchdog`（Linux `PR_SET_PDEATHSIG` + 窗口竞态防护 + `/proc/<pid>/stat` 父进程探活）；
  - [`lib/aio/queue.cp`](file:///data/dev/cp/lib/aio/queue.cp) & [`lib/aio/index.cp`](file:///data/dev/cp/lib/aio/index.cp)：零堆固定容量 `AioQueue<const CAP: usize>` SQ/CQ 环形提交/完成队列，支持 Open/Read/Write/Pread/Pwrite/Stat/Fsync/Close/Unlink/Mkdir；
  - [`docs/aio/aio.md`](file:///data/dev/cp/docs/aio/aio.md)：完整 API 手册。

### 深度补强标准库 (Enhanced Stdlib Modules)

- **`cap:bundler`**：新增 [`lib/bundler/target.cp`](file:///data/dev/cp/lib/bundler/target.cp)（`CompileTarget` 跨平台三元组解析/格式化）、[`lib/bundler/standalone.cp`](file:///data/dev/cp/lib/bundler/standalone.cp)（`StandaloneModuleGraph` 单文件虚拟 FS 二进制序列化与越界防护零拷贝反序列化）、扩展 [`lib/bundler/bundle.cp`](file:///data/dev/cp/lib/bundler/bundle.cp)（`ModuleGraph` 三色 DFS 环检测、拓扑排序与 Tree-Shaking 可达性标记），同步更新 [`docs/bundler/bundler.md`](file:///data/dev/cp/docs/bundler/bundler.md)；
- **`cap:safety`**：新增 [`lib/safety/cpu_features.cp`](file:///data/dev/cp/lib/safety/cpu_features.cp)（x86_64 CPUID+XGETBV / aarch64 硬件特性探测 `CpuFeatures`）与 [`lib/safety/crash.cp`](file:///data/dev/cp/lib/safety/crash.cp)（`OomGuard` 与零堆 `CrashReport` 诊断报告），同步更新 [`docs/safety/safety.md`](file:///data/dev/cp/docs/safety/safety.md)；
- **`cap:patch`**：新增 [`lib/patch/diff_format.cp`](file:///data/dev/cp/lib/patch/diff_format.cp)（`DiffFormatter` / `DiffFormatOptions` 类似 `bun test` 的彩色/字符级高亮与上下文折叠差分渲染器），同步更新 [`docs/patch/patch.md`](file:///data/dev/cp/docs/patch/patch.md)。

### 规范同步与端到端验收

- 同步双轨规范手册 [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) 与 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)；
- 新增深度测试 [`cap/test/test_resolver_bundler_deep.cp`](file:///data/dev/cp/cap/test/test_resolver_bundler_deep.cp)（7/7 PASS）与 [`cap/test/test_aio_safety_diff_format_deep.cp`](file:///data/dev/cp/cap/test/test_aio_safety_diff_format_deep.cp)（3/3 PASS），5 套联合回归全部 100% PASS；Batch 4 涉及 25 个 `.cp` 文件 `// Source:` 100% 合规。

---

## [2026-11-20] - 核心里程碑：`cap` + `cpc` 模块级增量编译、Zig `-fno-llvm` 极速原生后端与多智能体无锁并发构建升级

### 编译器前端模块级增量转译 (`cpc`)

- **按文件发射与 FNV-1a 内容哈希增量缓存 (`--emit-dir` / `--force`)**:
  - [`cpc/src/modular.rs`](file:///data/dev/cp/cpc/src/modular.rs)（新增，1,101 行）与 [`cpc/src/lib.rs`](file:///data/dev/cp/cpc/src/lib.rs)、[`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs)：
    - 新增 `cpc compile --emit-dir <DIR> [--force]` 模式，告别全量内联单一巨型 `.zig`，按依赖图拓扑将每个 `.cp` 模块独立发射为对应 `.zig` 文件（同步复制 `.zig` 垫片，共享 `__cp_runtime.zig`，根目录生成入口包装 `__cp_entry.zig` 与依赖清单 `.cp_modular_manifest.txt`）；
    - 基于文件内容 + 直接导入签名计算 64 位 FNV-1a 增量哈希（`// @cp_inc_hash:<hex>`）并持久化 `.cp_inc_cache.json`，未修改的 `.cp` 文件直接跳过重转译并原样保留 `.zig` 的 `mtime`，彻底激活 Zig 增量编译缓存；
    - 跨模块精确报错定位：Sema 诊断直接指向真实子模块文件与行号，消除巨型单文件行号漂移。
  - [`cpc/src/sema/mod.rs`](file:///data/dev/cp/cpc/src/sema/mod.rs)、[`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)、[`cpc/src/codegen/runtime_emit.rs`](file:///data/dev/cp/cpc/src/codegen/runtime_emit.rs)、[`cpc/src/codegen/expr/`](file:///data/dev/cp/cpc/src/codegen/expr/)：
    - 支持 `ImportedEnv` 跨模块符号预注册、共享 `__cp_runtime.zig` 统一引用（`rt_sym`）及模块间 `@import` 路径重写。
  - 已通过 `bash scripts/sync_cpc.sh release` 编译并原子部署 Release 正式版至 `/data/dev/cp/cap/bin/cpc`。

### 构建工具极速 Debug 后端与并发隔离升级 (`cap`)

- **五大构建提速与防冲突升级 (`cap/src/main.zig`)**:
  - [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig)：
    1. **全工程递归 `mtime` 感知**：`computeMaxSourceMtimeNs` 递归扫描 `src/**/*.cp` 与 `cap.json` 最新修改时间，支持 0 改动毫秒级 `[Up-to-date]` 跳过，彻底告别被迫 `-B` 全量强制重编；
    2. **`-d` / `--debug` 自动启用 Zig `-fno-llvm` 原生 x86_64 后端**：跳过 LLVM 优化/指令选择管线，实测 5.4 万行 CapSurf 后端编译从 47.99s 骤降至 1.34s（14.3× 总提速），同时内置原生后端失败自动回退 LLVM 与 `--llvm` 显式开关兜底；
    3. **原子产物替换消除 `FileBusy (ETXTBSY)`**：编译输出先落盘 `<out>.tmp.<pid>` 再原子 `rename` 覆盖目标二进制，运行期间重编零冲突；
    4. **多智能体工作区与缓存自动隔离**：指定 `-o`/`--out-dir` 或 `--cache-dir` 时自动隔离至 `.cap/ws/<sanitized>/{zig,zig-cache}`，多路并发构建彻底告别 `flock` 串行排队；
    5. **`-m` / `--modular` 模块级增量模式接线**：自动调用 `cpc compile --emit-dir`（配合 `-B` 透传 `--force`），识别 `__cp_entry.zig` 并从 `.cp_modular_manifest.txt` 提取聚合链接参数（`-L`/`-l`）；且对 `build-exe` 跳过冗余 `cpc emit-types`。
  - 已通过 `zig build -Doptimize=ReleaseFast` 编译并原子部署至 `/data/dev/cp/cap/bin/cap` 与 `/root/.cap/bin/cap`。

### 端到端实测验证

- 多文件增量构建测试（`temp/test_modular_e2e/`）：冷编译 `618ms`（`-fno-llvm`）、0 改动快速跳过 `0ms`、改 1 文件增量重编 `transpiled=1 cached=8 (7ms)` 且未改动 `.zig` `mtime` 零变动、双路 `-o bin_p1`/`-o bin_p2` 无锁并发构建均 `~696ms`、原子替换零 `ETXTBSY`；
- 标准库回归：`test_event_loop.cp`（5/5 PASS，703ms）、`test_http_router_sql_deep.cp`（8/8 PASS，1213ms）；`scripts/check_cpc_size.sh` 0 超限。

---

## [2026-11-20] - 核心任务：消除 sys 动态库依赖、集成 vendor/zlib 纯静态编译与工程递归 cap 依赖自动探测

### 工具链与构建系统升级 (Toolchain & Build System)

- **消除 `sys` 动态库依赖与纯静态库内联**:
  - [`lib/cap.json`](file:///data/dev/cp/lib/cap.json)：在 `pack` 与 `http` 下分别配置 `vendor/zlib` 静态编译选项（`opt: "fast"`, `flag: ["-DZLIB_COMPAT", "-DNDEBUG"]`），彻底清空所有 `sys` 动态库映射；
  - [`cap.json`](file:///data/dev/cp/cap.json)：移除根目录模板中的 `"z"` 动态链接项；
  - [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig)：
    - `resolveBinDependency`：直接返回生成的或预编译的静态库文件路径（`.a`）；
    - `appendLinkArgs`：若依赖项以 `.a` 结尾，直接作为静态对象文件传给 Zig 链接器，防止链接器被宿主机 `/lib64/libz.so` 劫持；
    - `scanDirRecursivelyForCaps`：递归扫描工程 `src/` 目录下所有 `.cp` 子模块的 `cap:` 导入，彻底解决子模块（如 `src/render/png.cp`）间接引入 `cap:pack` 时静态库遗漏链接的问题；
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：移除此前临时 Hotfix 硬编码发射的 `// @linkLibrary("z")`，交由构建工具按需静态链接。

### 压缩子系统与静态库验证 (Pack & Vendor Zlib)

- **`vendor/zlib` 兼容模式静态库构建**:
  - 在 `vendor/zlib` 生成兼容模式静态库 `libz.a` 与 `libzlib.a`，全量导出 `compress2`, `uncompress`, `compressBound`, `deflateInit2_` 等 C-ABI 符号；
- **端到端测试与纯静态验证 (Tests)**:
  - [`cap/test/test_pack_zlib_static.cp`](file:///data/dev/cp/cap/test/test_pack_zlib_static.cp)：新增静态 Zlib 压缩/解压缩往返测试，`cap test`（715ms PASS）；
  - `ldd` 验证：原生 ELF 产物完全不存在 `libz.so.1` 依赖，实现 100% 自包含独立二进制。

---

### 工具链与构建系统升级 (Toolchain & Build System)

- **`cap.json` 与 `lib/cap.json` 配置架构精炼升级**:
  - [`lib/cap.json`](file:///data/dev/cp/lib/cap.json)：移除 `global.default_opt`，升级为顶层 `"opt": "small"`；移除 `bin` 项中的冗余 `"type": "static_lib"`；采用精炼字段 `src`、`include`、`flag`；重构 `sys` 为命名字典 `{"z": {"linux": "libz.so.1", "mac": "libz.dylib", "win": "zlib1.dll"}}`；
  - [`cap/src/project/manifest.zig`](file:///data/dev/cp/cap/src/project/manifest.zig)：
    - `BinDependencyConfig`：支持 `src`（回退 `sources`）、`include`（回退 `include_dirs`）、`flag`（回退 `c_flags`）、默认 `static_lib`；
    - `SysDependencyConfig`：全面支持命名字典（多动态库依赖隔离）与扁平对象双模解析，平台键兼容 `mac` 与 `win`（回退 `macos` 与 `windows`）；
    - `CentralLibIndex`：支持顶级 `opt` 统一人体工程学配置；
  - [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig)：
    - `resolveSubPath`：在指定 `path` 作用域下，源文件与头文件路径自动省略 `path` 前缀；省略 `include` 时默认将 `path` 自身作为 include 路径；
    - `buildCWithZig`：源文件与包含路径全量经 `resolveSubPath` 智能解析；
    - `appendLinkArgs`：升级为 `getAllLibFlagsForTarget`，支持自动链接命名字典中定义的所有系统动态库（`-lz` 等）；
    - 重新编译 `cap` 发布版（ReleaseFast）并更新部署到 `/root/.cap/bin/cap`。

### 编译器模块解析强化 (Compiler Module Resolution)

- **`cpc` 模块解析路径精准映射与同名候选**:
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：
    - 实现 `resolve_cap_from_manifest`：读取中央索引 `lib/cap.json` 中的 `path` 字段；
    - **文件精准映射**：当 `path` 指向具体文件（如 `"http/server.cp"`）时，`import ... from "cap:http"` 直接定位到该文件；
    - **目录智能查找**：当 `path` 指向目录时，优先按 `index.cp` / `main.cp` / 与目录同名的 `.cp` 文件进行候选探测；
    - **平滑回退机制**：若配置的路径不存在或不同名映射失效，自动平滑退回到基于 `include_paths` 及 `/data/dev/cp/lib/` 的默认解析；
    - 直通同步到 `cpc_dev` 并运行 `bash scripts/sync_cpc.sh` 原子编译部署生效。

### 测试验证与下游闭环 (Tests & Downstream Verification)

- [`cap/test/test_cap_json_simplified.cp`](file:///data/dev/cp/cap/test/test_cap_json_simplified.cp)：新增端到端测试，验证基于精炼 `lib/cap.json` 触发的 `cap:http`（picohttpparser + libz）与 `cap:net`（usockets）按需编译与运行，`cap test`（867ms）与原生可执行文件直接运行 100% PASS；
- 下游验证：在 `/data/dev/capsurf` 成功构建 `bin/main`（ReleaseFast，耗时 40s），验证 `./bin/main --help` 完美执行。

---

### 标准库增强 (Enhanced Stdlib)

- **`cap:napi` Node-API 初步架构实现**:
  - [`lib/napi/index.cp`](file:///data/dev/cp/lib/napi/index.cp)：定义 `napi` 模块统一对外出口；
  - [`lib/napi/_napi.cp`](file:///data/dev/cp/lib/napi/_napi.cp)：建立 Node-API C-FFI Shim，初步绑定 `napi_get_undefined` 原型；
  - [`lib/napi/napi.cp`](file:///data/dev/cp/lib/napi/napi.cp)：构建高层 CP API 包装器 `Napi.getUndefined`；
  - 架构设计：遵循 CP 架构，FFI Shim 使用 `_` 前缀，高层封装遵循现代 CP 语言规范。

---

## [2026-11-18] - 核心里程碑：`cap:conf` 配置交换子系统（INI/TOML）工业级完整性补齐与首行 Bun 溯源覆盖

### 标准库增强与完备性补齐 (Enhanced Stdlib)

- **`cap:conf` 配置体系深度补齐与谱系溯源**:
  - [`lib/conf/ini.cp`](file:///data/dev/cp/lib/conf/ini.cp)：首行标注 `// Source: bun/src/ini/ini.cp`，全面对齐 Bun 57KB 原生实现；补齐层级与嵌套节区（`[server.database]`）、全局属性、属性元信息内省（`getSections`, `getKeys`）、多形态类型安全提取器（`getInt`, `getFloat`, `getBool` 覆盖全量现代布尔值）、双引号完整转义、单引号字面量、环境变量插值（`${VAR}`）、注释剥离以及双向标准序列化输出；
  - [`lib/conf/toml.cp`](file:///data/dev/cp/lib/conf/toml.cp)：首行标注 `// Source: bun/src/interchange/toml.cp`，全面升级为工业级 TOML 文档状态机；支持 10/16/8/2 进制整数与下划线、科学计数法与特殊浮点数、多行基础字符串 `"""` 与多行字面量 `'''`、点分嵌套表与表数组 `[[array]]`、复合数组安全切分提取以及双向标准序列化输出。

### 文档沉淀与规范同步 (Documentation)

- [`docs/stdlib/conf.md`](file:///data/dev/cp/docs/stdlib/conf.md)：新增全套 `cap:conf` 架构与 API 详细使用指南，提供 INI 与 TOML 完整代码用例；
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) & [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：在 `cap:conf` 标准库矩阵中更新完备特性说明。

### 测试验证 (Tests)

- [`cap/test/test_conf_complete.cp`](file:///data/dev/cp/cap/test/test_conf_complete.cp)：新增综合端到端测试用例，覆盖 INI 节区与类型转换、转义与环境变量、TOML 进制与多行字符串、表数组与双向序列化 Round-trip，2/2 100% 全部 PASS；
- 回归测试：`test_config.cp`（2/2 PASS）、`test_json5.cp`（3/3 PASS）。

---

## [2026-11-18] - 核心里程碑：`cap:col` 核心容器体系工业级完整性补齐与首行 Bun 溯源注释覆盖

### 标准库增强与完备性补齐 (Enhanced Stdlib)

- **`cap:col` 容器家族全量补全与谱系溯源**:
  - [`lib/col/bit_set.cp`](file:///data/dev/cp/lib/col/bit_set.cp)：首行标注 `// Source: bun/src/collections/bit_set.cp`，全面扩充至包含 `BitSet64`, `BitSet128`, `BitSet256`, `BitSet512`, `BitSet1024`, `DynamicBitSet`, `Range`，实装硬件 `@popCount` 与 `@ctz` 极速位扫描、动态重分配 `resize(len, fill)`、全套布尔集合代数与掩码迭代器；
  - [`lib/col/bounded_array.cp`](file:///data/dev/cp/lib/col/bounded_array.cp)：首行标注 `// Source: bun/src/collections/bounded_array.cp`，补齐 `fromSlice`, `slice`, `constSlice`, `unusedCapacitySlice`, `insert`, `insertSlice`, `replaceRange`, `contains`, `findIndex`, `at` 等 10+ 项高阶栈数组操作；
  - [`lib/col/linear_fifo.cp`](file:///data/dev/cp/lib/col/linear_fifo.cp)：首行标注 `// Source: bun/src/collections/linear_fifo.cp`，补齐 `realign`（原地消除环形缓冲回绕展开连续物理切片）、`shrink`、`unget`（逆向反弹未消费数据）、`readableSlice` / `writableSlice` 零拷贝 I/O 契约及 `orderedRemoveItem`；
  - [`lib/col/baby_list.cp`](file:///data/dev/cp/lib/col/baby_list.cp)：首行标注 `// Source: bun/src/collections/baby_list.cp`，补齐 `fromSlice`, `clone`, `memoryCost`, `ensureTotalCapacityPrecise`, `expandToCapacity`, `shrinkAndFree`, `insert`, `insertSlice`, `orderedRemove`, `swapRemove` 及 `OffsetByteList` 结构体；
  - [`lib/col/hive_array.cp`](file:///data/dev/cp/lib/col/hive_array.cp) & [`lib/col/index.cp`](file:///data/dev/cp/lib/col/index.cp)：统一从 `bit_set.cp` 导出 `BitSet64`，首行标注来源注释，对外暴露统一聚合入口。

### 文档沉淀与规范同步 (Documentation)

- [`docs/stdlib/col.md`](file:///data/dev/cp/docs/stdlib/col.md)：新增全套 `cap:col` 架构与 API 详细使用手册，提供 BitSet、BoundedArray、LinearFifo、BabyList、Soa 代码用例；
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) & [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：在 `cap:col` 标准库矩阵中增补全套位图矩阵、范围与紧凑数组符号。

### 测试验证 (Tests)

- [`cap/test/test_col_complete.cp`](file:///data/dev/cp/cap/test/test_col_complete.cp)：新增 5 大综合测试用例，覆盖 BoundedArray、LinearFifo、BabyList、静态位图矩阵与 DynamicBitSet，5/5 100% 全部 PASS；
- 回归测试：`test_collections_bun.cp`（6/6 PASS）、`test_collections_matrix.cp`（7/7 PASS）、`test_col_pack_md_html.cp`（4/4 PASS）。

---

## [2026-11-18] - 核心里程碑：内置全局 JSON.parse 统一（消灭 import 割裂，零导入支持动态 SIMD JsonValue 与强类型反射解析）与下游热修全结单

### 编译器新特性与架构统一 (Compiler Features & Architecture)

- **内置全局 `JSON.parse` 零导入双轨统一**:
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：在 `needs_json && !has_imported_json && !self.is_lib_mode` 时，自动发射自包含 `JsonTag`、`JsonField`、`JsonValue`、`FastJsonScanner`（16 字节 SIMD 引号/转义探测）、`JsonParser` 与全局 `JSON` 结构体；为 `JsonValue` 补齐 `isObject()`, `isArray()`, `isBool()`, `isNumber()`, `isString()` 等人体工程学方法；
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：彻底消除原先依赖 `import { JSON } from "cap:json"` 的分支判断；所有动态 `JSON.parse(str)` / `JSON.parse(alloc, str)` 无条件直通自研 `JSON.parse` 零拷贝引擎；强类型 `JSON.parse(T, str)` / `JSON.parse<T>(str)` 直通 Zig 编译期反射；
  - [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)：增强 AST 扫描，当检测到模块内显式导入或声明了 `JsonTag` / `JsonValue` / `JsonParser` / `JSON` 时，自动将 `has_imported_json` 设为 `true`，防止双重定义冲突；
  - [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs)：动态 `JSON.parse` 语义推导统一返回 `!JsonValue`；强类型调用返回 `!T`。

### 标准库强化与下游热修交付 (Stdlib & Hotfixes)

- **标准库 JSON 契约统一 (`cap:json`)**:
  - [`lib/json/value.cp`](file:///data/dev/cp/lib/json/value.cp)：实例方法补齐 `isObject`, `isArray`, `isBool`, `isNumber`, `isString`，保证与内置 `JsonValue` 100% 契约一致；
  - [`lib/json/parser.cp`](file:///data/dev/cp/lib/json/parser.cp)：结合 16 字节 SIMD `FastJsonScanner` 与动态 AST 构造。
- **下游热修工单全量交付与结单 (Downstream Hotfixes)**:
  - `task-capsurf-20261117-pack-native-allocator`：修复 `lib/pack/native.cp` 的 `std.mem.Allocator` 导入与导出，验证 `test_col_pack_md_html.cp` 4/4 全部 PASS，结单交付；
  - `task-capsurf-20261117-compiler-borrow-conflict`：修复验证 CapSurf `src/js/dom_binding.cp` 裸指针借用冲突，结单交付；
  - 下游任务队列清零（0 pending）。

### 测试验证 (Tests)

- [`cap/test/test_builtin_json_unify.cp`](file:///data/dev/cp/cap/test/test_builtin_json_unify.cp)：新增端到端测试，验证零导入下动态 SIMD 零拷贝与强类型反射解析，2/2 全部 PASS；
- [`cap/test/test_fast_json.cp`](file:///data/dev/cp/cap/test/test_fast_json.cp)：3/3 全部 PASS；
- [`cap/test/test_zero_copy_json.cp`](file:///data/dev/cp/cap/test/test_zero_copy_json.cp)：3/3 全部 PASS；
- [`cap/test/test_json5.cp`](file:///data/dev/cp/cap/test/test_json5.cp)：3/3 全部 PASS；
- [`cap/test/test_json.cp`](file:///data/dev/cp/cap/test/test_json.cp)：1/1 全部 PASS；
- [`cap/test/test_col_pack_md_html.cp`](file:///data/dev/cp/cap/test/test_col_pack_md_html.cp)：4/4 全部 PASS。

---

## [2026-11-18] - 核心任务：补全 JSON5 完整解析能力与规范化标准库来源头部

### 新增与增强标准库 (Enhanced Stdlib)

- **配置与数据交换子系统 JSON5 完备实现 (`cap:conf`)**:
  - [`lib/conf/json5.cp`](file:///data/dev/cp/lib/conf/json5.cp)：完整实现 `JSON5Parser` 与 `JSON5.parse(allocator, src): !JsonValue`，全面支持单行注释（`//`）、多行注释（`/* */`）、对象与数组尾随逗号（trailing comma）、未加引号的 ECMAScript 标识符键（unquoted keys）、单引号字符串（`'...'`）、转义字符与换行折叠、十六进制数字（`0x...`）、显式正负号（`+42`）、特殊常量（`NaN`、`Infinity`）；
  - 保留并向前兼容 `JSON5.stripCommentsAndTrailingCommas(src, out_buf)` 预处理接口；
  - 导出 `Json5Scanner` 供外部复用。
- **规范化 `lib/json/` 来源标注**:
  - [`lib/json/parser.cp`](file:///data/dev/cp/lib/json/parser.cp)：头部注释修正为 `// Source: Custom implementation by Antigravity AI (Zero-copy JSON streaming parser)`；
  - [`lib/json/pool.cp`](file:///data/dev/cp/lib/json/pool.cp)：头部注释修正为 `// Source: Custom implementation by Antigravity AI (Thread-local buffer pool for JSON parsing)`；
  - [`lib/json/value.cp`](file:///data/dev/cp/lib/json/value.cp)：头部注释修正为 `// Source: Custom implementation by Antigravity AI (Zero-copy JSON value AST & enum)`。

### 测试验证 (Tests)

- [`cap/test/test_json5.cp`](file:///data/dev/cp/cap/test/test_json5.cp)：新增 JSON5 专项端到端测试，验证注释与未加引号键、十六进制/带符号数值与尾随逗号、预处理向后兼容性，3/3 用例 100% 全部通过。

---

## [2026-11-17] - 核心里程碑：高性能列式容器 Soa<T> 实装与 Zig 被埋没资产吸收第一阶段（cap:sort 与 cap:hash）

### 新增标准库与容器 (Added Stdlib & Containers)

- **高性能列式结构体数组容器 `Soa<T>` (`cap:col`)**:
  - [`lib/col/soa.cp`](file:///data/dev/cp/lib/col/soa.cp)：基于 Zig `std.MultiArrayList(T)` 实现连续内存列式分列存储（Struct of Arrays），支持 `init`, `initCapacity`, `deinit`, `append`, `get`, `set`, `len`, `capacity`, `popOrNull`, `swapRemove`, `orderedRemove`, `clearRetainingCapacity`；
  - 核心原语 `items(field)` 暴露各字段物理连续切片，彻底消除 AoS 字段跨度缓存污染，直通 SIMD 向量化并行计算；
  - [`lib/col/index.cp`](file:///data/dev/cp/lib/col/index.cp)：对外统一导出 `Soa<T>`；
  - [`docs/stdlib/soa.md`](file:///data/dev/cp/docs/stdlib/soa.md)：编写详尽 API 指南与多类型结构体列式计算示例。
- **工业级排序与对数检索标准库 (`cap:sort`)**:
  - [`lib/sort/index.cp`](file:///data/dev/cp/lib/sort/index.cp)：基于 Orson Peters 的 PDQSort 实现零堆分配原地快排（`Sort.asc`, `Sort.desc`, `Sort.sort`）；基于 BlockSort 实现原地稳定块排序（`Sort.stable`, `Sort.stableAsc`）；基于 HeapSort 实现严格 O(1) 辅助空间堆排序（`Sort.heap`, `Sort.heapAsc`）；
  - 提供有序切片检索：`Sort.isSorted`, `Sort.isSortedAsc`, 二分搜索 `Sort.binarySearch`, 插入上/下界 `Sort.lowerBound` / `Sort.upperBound`, 极值检索 `Sort.minI32` / `maxI32` / `argMin` / `argMax` 以及泛型求值器 `SortOf<T>`；
  - [`docs/stdlib/sort.md`](file:///data/dev/cp/docs/stdlib/sort.md)：独立技术手册与复杂度分析。
- **非密码学极速散列与校验和标准库 (`cap:hash`)**:
  - [`lib/hash/index.cp`](file:///data/dev/cp/lib/hash/index.cp)：集成 30~50 GB/s SIMD 硬件加速 XxHash3 与 128 位超低碰撞散列；工业标准 Murmur3 与 CityHash64；硬件线速 CRC32 与 Adler32 校验和；防 Hash-DoS 攻击伪随机散列 SipHash-2-4 与 SipHash-128；Fnv1a 与 64 位整数雪崩置换 `hashInt`；
  - 导出增量流式散列计算句柄 `XxHash3Stream`；
  - [`docs/stdlib/hash.md`](file:///data/dev/cp/docs/stdlib/hash.md)：独立技术手册与工业标准测试向量。

### 编译器增强 (Compiler Enhancements)

- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：
  - 增强方法调用与普通自由函数调用的缺省参数处理：当调用方传入的实参数量少于形参数量时，自动查找并回填带默认值的参数表达式，消除调用方显式填入冗余默认参数的开销。

### 规范手册与测试验证 (Docs & Tests)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在 12.1 节标准库表格与 20.11 节登记 `Soa<T>`、`cap:sort` 与 `cap:hash`。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：官方标准库数量升级为 34 个，在标准库矩阵中登记 `cap:sort` 与 `cap:hash`。
- [`cap/test/test_soa.cp`](file:///data/dev/cp/cap/test/test_soa.cp)：4/4 用例全部 PASS。
- [`cap/test/test_sort.cp`](file:///data/dev/cp/cap/test/test_sort.cp)：6/6 用例全部 PASS。
- [`cap/test/test_hash.cp`](file:///data/dev/cp/cap/test/test_hash.cp)：5/5 用例全部 PASS。

---

## [2026-11-16] - 核心里程碑：Spec 5 零堆分配流式直写与 Spec 8 跨协程静态隔离检查 E0305 实装

### 编译器新特性与优化 (Compiler Features & Optimizations)

- **零分配模板字符串流式直写 (Spec 5)**:
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：实现 `lower_template_stream`，为 `print`、`println`、`@print`、`writer.write`、`writer.print` 等提供流式直写 Lowering，彻底跳过 `allocPrint` 堆分配，常量内插编译期静态折叠，动态参数包覆 `cp_fmt` 直写底层流。
- **跨协程/异步边界静态隔离检查 E0305 (Spec 8)**:
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：引入 `local_borrow_vars` 追踪与 `check_async_boundary_arg` 检查；拦截 `spawn`、`@spawn`、`spawn.cpu`、`send` 边界传递局部借用指针 `*T`、`&T` 或裸切片，报出三段式 AI-First 智能诊断 `error[E0305]`。

### 双轨规范与文档 (Documentation & Specs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在第 20.11 节增加第 10 小节（流式直写优化）与第 11 小节（跨协程隔离检查 E0305）。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：升级为 Top 29 绝对禁手清单，登记 #29 条（跨协程传递局部借用拦截）。

### 测试验证 (Tests)

- [`cap/test/test_stream_template_and_isolation.cp`](file:///data/dev/cp/cap/test/test_stream_template_and_isolation.cp)：Spec 5 流式直写与 Spec 8 安全并发 100% PASS。
- [`cap/test/test_parallel_fail_borrow.cp`](file:///data/dev/cp/cap/test/test_parallel_fail_borrow.cp)：精准触发 E0305 拦截。
- `test_all.cp` 回归验证：100% PASS。

---

## [2026-11-16] - 核心里程碑：LLVM libFuzzer 驱动接入、AddressSanitizer 内存检查与 cap 模糊测试指令矩阵 (Spec 12 第二部分)

### 编译器与构建工具链 (Compiler & Toolchain)

- **libFuzzer C-ABI 标准符号导出与构建模式**:
  - [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs)：新增 `--fuzz` 与 `--fuzz-target <name>` 编译命令行选项，支持环境标识 `CP_FUZZ`；
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：在 `--fuzz` 模式下抑制普通 `pub fn main` 生成，提取所有 `fuzz` 目标块，导出标准 C-ABI 入口函数 `export fn LLVMFuzzerTestOneInput(data: [*c]const u8, size: usize) callconv(.c) c_int`；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：登记所有 `fuzz` 目标块上下文，支持单独隔离为函数并安全分发。
- **构建工具端 `cap` 命令行指令矩阵与两阶段插桩链接**:
  - [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig)：
    - 扩展 `cap test` 支持 `--fuzz`（及一等子命令 `cap fuzz [target]`）；
    - 采用 `zig build-obj` + `clang -fsanitize=fuzzer,address` 两阶段构建，无缝引入 Coverage-guided 变异与 ASan 内存安全检查；
    - 支持可选参数 `--timeout <sec>` 与 `--jobs <n>` 控制执行时长与并发 Worker；
    - 支持崩溃复现指令 `cap test --repro <crash_file.bin>`，单次喂入输入文件，秒级触发崩溃并回溯堆栈现场；
    - `usage()` 帮助文档同步增补 Fuzz Options 指引。

### 双轨规范与文档 (Documentation & Specs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在第 20.11 节补充 libFuzzer C-ABI 导出与 `cap test --fuzz`、`--repro` 官方规范。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：更新原生代码块矩阵中的 `fuzz` 块契约描述。

### 测试验证 (Tests)

- [`cap/test/test_fuzz_driver.cp`](file:///data/dev/cp/cap/test/test_fuzz_driver.cp)：新增专用模糊测试与崩溃复现器验证用例；
- 实测验证：
  - 常规测试模式 `cap test`: 100% PASS；
  - libFuzzer 模式 `cap test --fuzz`: 100% PASS (1000 轮动态变异)；
  - 超高吞吐测试 `cap fuzz --timeout 1`: 100% PASS (1 秒 220 万次执行，110 万 exec/s)；
  - 崩溃复现 `cap test --repro temp/crash_sample.bin`: 精准复现 panic 与堆栈定位。

---

## [2026-11-16] - 核心里程碑：Fuzz 模糊测试块、函数级 @arena 内存区域修饰器与字段级 @json 注解定制

### 编译器新特性与语法扩展 (Compiler Features)

- **一级 `fuzz` 模糊测试块 (Spec 12.2)**:
  - [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs)：新增保留字 `Token::Fuzz`；
  - [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) & [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs)：新增 `FuzzDecl` 声明与 AST 节点；
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：支持顶层与模块项 `fuzz <name> (<param>) { ... }` 语法解析；
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：语义分析自动将变异样本形参绑定为 `[]const u8`；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)、[`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)、[`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：发射集成 16+ 高危边界样本语料库（Corpus）与 100 轮动态切片变异驱动循环的 `test "fuzz: <name>"`。
- **函数级 `@arena` 作用域内存区域修饰器 (Spec 7)**:
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：在 `parse_meta_ident` 映射 `Token::Arena` 为 `"arena"` 注解；
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：元注解校验增加 `@arena` 函数修饰器豁免；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：在函数序言处初始化栈 Arena 并压入活跃 Arena 作用域栈，退出时自动注入 `defer __fn_arena.deinit()`，使函数内局部临时分配离开时 $O(1)$ 瞬时整块释放，实现零堆碎片与强异常安全。
- **字段级 `@json("alias")` 与 `@json_ignore` 序列化定制 (Spec 6)**:
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：重构 `derive_json_method`，跳过标注 `@json_ignore` 的字段，提取 `@json("alias")` 别名作为 JSON 键，无有效字段时输出 `"{}"`。

### 双轨规范与文档同步 (Documentation & Specs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在第 20.2 节补充函数级 `@arena`，并在第 20.11 节增补字段级 JSON 定制、函数级 `@arena` 与 `fuzz` 模糊测试块权威规范。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：扩充原生代码块矩阵至五大代码块（引入 `fuzz` 块），并同步更新字段级 `@json`、函数级 `@arena` 的精炼可运行契约示例。

### 测试验证 (Tests)

- [`cap/test/test_fuzz_arena_json.cp`](file:///data/dev/cp/cap/test/test_fuzz_arena_json.cp)：字段别名重命名、字段忽略过滤、函数级 `@arena` 作用域计算与 `fuzz` 驱动验证 100% PASS。
- `test_all.cp` 回归验证：100% PASS。

---

## [2026-11-16] - 核心里程碑：Phase 2 人体工程学语法糖实装、契约式设计、SIMD原语与双轨规范同步

### 编译器人体工程学与黑科技语法 (Compiler Ergonomics & Contracts)

- **空值合并赋值 (`??=`)**:
  - [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs) & [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：支持 `Token::QuestionQuestionAssign` 复合赋值解析；
  - [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)：Lowering 为 `if (lhs == null) { lhs = rhs; }`，实现零堆开销天然短路求值。
- **契约式设计注解 (`@pre` 与 `@ret`)**:
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：`parse_meta_arg` 扩展二元断言表达式解析；支持在函数签名后尾随注解 `@pre(cond)` 和 `@ret(cond)`；
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：元注解校验对 `@pre` 和 `@ret` 豁免静态常量检查；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：在函数入口自动发射前置条件断言 `std.debug.assert(cond)`；
  - [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)：在函数体各 `return` 返回处捕获返回值 `ret` 并发射后置条件断言 `std.debug.assert(cond)`。
- **万能降级解包 `?` 多行块与复合类型 `(?T)!` 双重解包**:
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：`?` 降级操作符全面支持后接代码块 `{ ... }`（`Expr::LabeledBlock`）；
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) & [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：对可空且可能报错的复合类型 `(?T)!` 自动发射双重链式解包（先 `catch` 再 `orelse`），若成功直接解包出底层基型 `T`。
- **内建定长 SIMD 向量类型别名**:
  - [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs) & [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：内置 `u8x16`、`u8x32`、`u8x64`、`i16x8`、`i16x16`、`i32x4`、`i32x8`、`u32x4`、`u32x8`、`f32x4`、`f32x8`、`f32x16`、`f64x2`、`f64x4` 映射至底层 `@Vector(N, T)`，原生支持标量广播 `@splat` 与向量算术运算（`+`, `-`, `*`, `/`）。
- **派生宏全家桶扩展 (`@[Default]`)**:
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：登记并实装 `derive_default_method`，为结构体/类自动派生全零值初始化静态工厂 `pub static fn default(): Self`。
- **切片属性语法糖消歧保护 (`.slice`)**:
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：将 `.slice` 改为白名单判定（仅对字符串/数组/切片生效），保留用户自定义类型的同名 `.slice()` 成员方法。

### 双轨规范与文档同步 (Documentation & Specs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：新增第 20.11 小节，全面收录 Phase 2 人体工程学语法糖、契约式设计、SIMD 原语与派生宏规范。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：同步精简更新标量向量类型、`??=` 复合赋值、`@pre`/`@ret` 契约注解、`? { ... }` 万能降级与双重解包、`@[Default]` 派生宏。

### 测试验证 (Tests)

- [`cap/test/test_phase2_ergonomics.cp`](file:///data/dev/cp/cap/test/test_phase2_ergonomics.cp)：6 大特性全量端到端验证通过（100% PASS）。
- 回归测试：`test_all.cp` 验证 100% 通过。

---

## [2026-11-16] - 核心里程碑：Bun 高价值资产全面吸收、图像子系统（PNG截图/BMP/量化）实装与标准库全景扩展

### 图像子系统实装与截图引擎 (Added `cap:image` & `cap:png`)

- **纯 CP PNG 编解码器 (`png.cp`)**:
  - [`lib/image/png.cp`](file:///data/dev/cp/lib/image/png.cp)：为 `capsurf_agy` 网页截图提供专属引擎；
  - `Png.encodeRgba(rgba, width, height)`：支持标准 8 字节魔数签名、IHDR Chunk、扫描线 None 滤波（0x00 前缀）、Zlib 原生 Deflate 流式压缩生成 IDAT、IEND 闭合与 CRC-32 校验和；
  - `Png.decode(bytes)`：支持 IHDR 元数据解析、IDAT 聚合解压、5 种滤波逆运算（None, Sub, Up, Average, Paeth）像素还原；
  - `PngImage` 与 `RgbaColor` 强类型封装，支持坐标级读取与变异；
  - `Png.save(path, rgba, w, h)` 与 `Png.load(path)` 同步文件 I/O 管道。
- **图像格式嗅探与 MIME 推导 (`codecs.cp`)**:
  - [`lib/image/codecs.cp`](file:///data/dev/cp/lib/image/codecs.cp)：8 大图片格式（PNG/JPEG/WebP/BMP/GIF/TIFF/AVIF/HEIC）二进制魔数探测、MIME 字符串映射与文件名后缀推导。
- **Windows BMP DIB 编解码 (`bmp.cp`)**:
  - [`lib/image/bmp.cp`](file:///data/dev/cp/lib/image/bmp.cp)：支持 24/32 位 BMP 文件解码与 32 位 BMP 文件编码，包含 4 字节跨距行对齐转换。
- **纯 CP GIF89a/87a LZW 解码器 (`gif.cp`)**:
  - [`lib/image/gif.cp`](file:///data/dev/cp/lib/image/gif.cp)：4096 节点 LZW 字典流式解压、Logical Screen Descriptor 探测、全局/局部调色板映射、4-pass 交织扫描还原与透明色索引解析。
- **Evan Wallace ThumbHash 模糊占位图算法 (`thumbhash.cp`)**:
  - [`lib/image/thumbhash.cp`](file:///data/dev/cp/lib/image/thumbhash.cp)：将 RGBA8 像素压缩为 ≤25 字节紧凑哈希，并支持逆向解码为 32x32 模糊平滑占位图。
- **EXIF 方向解析与变换矩阵 (`exif.cp`)**:
  - [`lib/image/exif.cp`](file:///data/dev/cp/lib/image/exif.cp)：快速解析 JPEG APP1 / TIFF IFD0 0x0112 旋转标签，映射生成 `ExifTransform` 旋转与镜像动作。
- **高阶图像处理管线 (`pipeline.cp`)**:
  - [`lib/image/pipeline.cp`](file:///data/dev/cp/lib/image/pipeline.cp)：轻量结构体 `Image`，支持 `.flop()`、`.flip()`、`.rotate(90/180/270)`、`.crop(x, y, w, h)`、`.resize(w, h)` 双线性缩放及 `.thumbhash()`。
- **JPEG 与 WebP 容器探测 (`jpeg.cp`, `webp.cp`)**:
  - [`lib/image/jpeg.cp`](file:///data/dev/cp/lib/image/jpeg.cp)：SOF0/SOF2 帧头基准尺寸、分量与渐进式检测；
  - [`lib/image/webp.cp`](file:///data/dev/cp/lib/image/webp.cp)：RIFF WEBP VP8/VP8L/VP8X 容器头嗅探、透明度与动画标志提取。
- **全量标准库 Bun 溯源注释注入 (Provenance Headers)**:
  - 在 `lib/` 下 52 个从 Bun 吸收的文件头部，统一规范注入 `// Absorbed from Bun: /data/dev/bun/src/...` 与 `// Upstream Sync:` 溯源注释。
- **调色板颜色量化器 (`quantize.cp`)**:
  - [`lib/image/quantize.cp`](file:///data/dev/cp/lib/image/quantize.cp)：基于 Median-Cut 算法实现 RGBA 像素向 256 色紧凑调色板与索引图转换。
- **统一聚合出口 (`index.cp`)**:
  - [`lib/image/index.cp`](file:///data/dev/cp/lib/image/index.cp)：门面统一导出 `cap:image`、`cap:png`、`cap:img` 协议全部 10 大子模块。

### Bun 高价值资产吸收与标准库扩展 (Added Stdlib)

- **容器集合库 (`cap:col`)**:
  - [`lib/col/hash_map.cp`](file:///data/dev/cp/lib/col/hash_map.cp)：开放寻址双重散列 Robin Hood 哈希表，配合 `bit_set.cp`, `bounded_array.cp`, `hive_array.cp`, `object_pool.cp` 形成完整工业级集合体系。
- **高性能字符串库 (`cap:string`)**:
  - [`lib/string/smol.cp`](file:///data/dev/cp/lib/string/smol.cp)：24 字节 SSO 小字符串（小于 23 字节纯栈上分配，超长自动堆回退）；
  - [`lib/string/hashed.cp`](file:///data/dev/cp/lib/string/hashed.cp)：内联 Wyhash 预计算的快速比较字符串对象；
  - [`lib/string/joiner.cp`](file:///data/dev/cp/lib/string/joiner.cp) 与 [`lib/string/path.cp`](file:///data/dev/cp/lib/string/path.cp)。
- **线程并发同步库 (`cap:sync`)**:
  - [`lib/sync/pool.cp`](file:///data/dev/cp/lib/sync/pool.cp)：`Guarded<T>` 互斥锁作用域保护包装类与 `WorkPool` 多线程工作者队列；
  - 深度重构 `arc_slice.cp`, `concurrent_map.cp`, `condition.cp`, `futex.cp`, `mutex.cp`, `ring_buffer.cp`, `unbounded_queue.cp`, `wait_group.cp`。
- **动态事件与异步任务库 (`cap:event`)**:
  - [`lib/event/task.cp`](file:///data/dev/cp/lib/event/task.cp)：`AnyTask` 与 `ManagedTask` 动态多态任务调度与状态机切分。
- **轻量嵌入式 SQL 数据库 (`cap:sql`)**:
  - [`lib/sql/sqlite.cp`](file:///data/dev/cp/lib/sql/sqlite.cp)：内存数据库与持久化 SQLite 引擎封装。
- **内嵌微型脚本引擎 (`cap:shell`)**:
  - [`lib/shell/lexer.cp`](file:///data/dev/cp/lib/shell/lexer.cp), [`lib/shell/parser.cp`](file:///data/dev/cp/lib/shell/parser.cp), [`lib/shell/eval.cp`](file:///data/dev/cp/lib/shell/eval.cp), [`lib/shell/index.cp`](file:///data/dev/cp/lib/shell/index.cp)。

### 编译器与构建增强 (Compiler & Build)

- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：注册 `"image" | "img" | "png" => "image"`、`"sync" | "threading" => "sync"`、`"shell" | "sh" => "shell"` 模块路径协议映射。
- [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)：结构体字段默认值表达式扫描、位移检测、异步内建指令预加载标志修复。
- [`cap.json`](file:///data/dev/cp/cap.json)：声明链接动态库 `"sqlite3"` 与 `"z"`。

### 测试套件验证 (Tests)

- [`cap/test/test_image_png.cp`](file:///data/dev/cp/cap/test/test_image_png.cp)：6/6 PASS (100%)。
- [`cap/test/test_bun_absorbed_assets.cp`](file:///data/dev/cp/cap/test/test_bun_absorbed_assets.cp)：6/6 PASS (100%)。
- [`cap/test/test_collections_bun.cp`](file:///data/dev/cp/cap/test/test_collections_bun.cp)：6/6 PASS (100%)。
- [`cap/test/test_sqlite.cp`](file:///data/dev/cp/cap/test/test_sqlite.cp)：PASS (100%)。

---

## [2026-11-13] - 核心里程碑：安全防护与 CSRF Token 体系（cap:security）实装及基于 5 步法的 cp_agent_ref.md 全量重构

### 安全防护体系实装 (Added)

- **CSRF 防护核心库 (`csrf.cp`)**:
  - [`lib/security/csrf.cp`](file:///data/dev/cp/lib/security/csrf.cp)：基于 CSPRNG 随机 Nonce、HMAC-SHA256 签名校验、时钟漂移防重放，以及 `constantTimeEql` 恒定时间缓冲区对比的 CSRF Token 生成与验证；
  - 支持 `Base64Url`、`Base64` 与 `Hex` 三种工业编码格式。
- **模块重导出与单一出口 (`index.cp`)**:
  - [`lib/security/index.cp`](file:///data/dev/cp/lib/security/index.cp)：对外统一导出 `CSRF`、`TokenFormat`、`DEFAULT_EXPIRATION_MS`、`constantTimeEql`。

### 智能体高密度语法契约重构 (Agent Specification)

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：
  - 严格采纳用户 5 步法建议，从权威 `cp_ref.md` 提取单一真理来源，剔除全部历史原理、哲学论据与多语言评测（减重 80%）；
  - 保留纯正 CP 语法与精炼可运行示例（Canonical Examples）；
  - 彻底清除 Zig 语法渗漏（`try`、`for()|i|`、`and/or`、`!T`/`?T`、`Point{ .x = 1 }`、`&.{}` 等）；
  - 与编译器 AST/Parser/Sema 源码核对并经微型综合测试脚本编译验证通过（100% PASS）；
  - 结尾增设《Top 15 绝对禁手清单》。

### 编译器增强与热修自愈 (Compiler & Hotfix)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增强 `Type::Ref` 具名结构体解构赋值支持，自动解析结构体定义字段。
- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) & [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs):
  - 修复未初始化栈缓冲区（`undefined`）切片（`raw[0..]`）与索引赋值（`raw[i] = ...`）误触 `uninitialized_vars` 读取拦截的缺陷。
- [`cpc/src/codegen/pattern.rs`](file:///data/dev/cp/cpc/src/codegen/pattern.rs) & [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs):
  - 修复常量模式解构 Rest 剩余绑定（`const [head, ...tail] = ...`）中不可变切片类型推断与闭包状态机上下文中的类型对齐（`[]const T`）。

### 标准库强化 (Library Updates)

- [`lib/crypto/hmac.cp`](file:///data/dev/cp/lib/crypto/hmac.cp)：新增零堆分配的高性能原生切片签名函数 `hmacSha256Raw(out, key, data)`。
- [`lib/crypto/random.cp`](file:///data/dev/cp/lib/crypto/random.cp)：新增原生直接就地填充随机字节函数 `fillRandom(buf)`。
- [`lib/encoding/base64.cp`](file:///data/dev/cp/lib/encoding/base64.cp)：全面升级为纯正面向对象 `export class Base64`，规范纯正 CP 返回类型与切片位移运算。

### 测试验证 (Tests)

- [`cap/test/test_security_csrf.cp`](file:///data/dev/cp/cap/test/test_security_csrf.cp)：4/4 端到端安全测试 100% PASS。
- 全量回归测试：`test_event_loop.cp`, `test_crypto_features.cp`, `test_encoding.cp`, `test_sync_primitives.cp` 全部 100% PASS。

## [2026-11-13] - 核心里程碑：工业级现代密码学体系（cap:crypto）建设、编译器枚举方法与解构自愈

### 工业级密码学体系实装 (Added)

- **真随机数与 CSPRNG (`random.cp`)**:
  - [`lib/crypto/random.cp`](file:///data/dev/cp/lib/crypto/random.cp)：直通 Linux 内核 `getrandom` 熵池的 CSPRNG，实现 RFC 4122 v4 `randomUUID()` 与无偏高精度 `randomInt(min, max)`。
- **散列哈希与流式增量散列 (`hash.cp`)**:
  - [`lib/crypto/hash.cp`](file:///data/dev/cp/lib/crypto/hash.cp)：`sha256`, `sha512`, `sha1`, `md5` 单次快速哈希计算；面向对象的流式 `Hash` 类，支持 `.update(data)` 增量注入与 `.digest()` 最终十六进制输出。
- **HMAC 消息认证码 (`hmac.cp`)**:
  - [`lib/crypto/hmac.cp`](file:///data/dev/cp/lib/crypto/hmac.cp)：RFC 2104 `hmacSha256(key, data)` 与流式 `Hmac` 类。
- **PBKDF2 口令派生密钥 (`pbkdf2.cp`)**:
  - [`lib/crypto/pbkdf2.cp`](file:///data/dev/cp/lib/crypto/pbkdf2.cp)：`pbkdf2Sync`（返回十六进制字符串）与 `pbkdf2Raw`（直接安全填充目标字节数组）。
- **现代 AEAD 认证对称加密 (`cipher.cp`)**:
  - [`lib/crypto/cipher.cp`](file:///data/dev/cp/lib/crypto/cipher.cp)：现代工业标准 `Aes256Gcm`（NIST SP 800-38D）与 `ChaCha20Poly1305`（RFC 8439），兼具保密性加密与带 16 字节认证标签的防篡改解密校验。
- **模块重导出与单一出口 (`index.cp`)**:
  - [`lib/crypto/index.cp`](file:///data/dev/cp/lib/crypto/index.cp)：对外统一规范重导出全部密码学符号。
- **废弃平铺单文件清理 (Removed)**:
  - 彻底删除旧版平铺单文件 `lib/crypto.cp`，消除同名文件与复合目录歧义。

### 编译器增强与热修自愈 (Compiler & Hotfix)

- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) / [`cpc_dev/src/codegen/expr.rs`](file:///data/dev/cp/cpc_dev/src/codegen/expr.rs):
  - 修复枚举静态方法调用被误降级为结构体/联合体初始化的关键缺陷：在 `Expr::Call` 访问枚举成员时，严格校验 `is_method || !is_variant`，若为方法或非变体则生成标准函数调用 `Enum.method(...)`，杜绝非法 Zig 语法 `Enum{ .method = arg }`。
- [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs):
  - 修复类方法接收者 `self: *T` 在生成参数列表时过滤掉 `first.ident.sym == "self"` 并保留其可变指针类型（`*T` 而非 `*const T`）。
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs):
  - 适配 Zig 0.17 结构体解包，将 `.@"struct".fields` 升级为 `.field_names` 和 `.field_types`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / `task-sema-object-tuple-destructure`:
  - 修复匿名结构体位置字段在推断为 `Type::Object` 时被丢弃的缺陷，打通 `const [a, b] = pair` 对匿名对象的解构模式分析，结单归档下游任务。

### 测试验证 (Tests)

- [`cap/test/test_crypto_features.cp`](file:///data/dev/cp/cap/test/test_crypto_features.cp)：涵盖 CSPRNG/UUID、SHA256/512/1/MD5/Streaming、HMAC/PBKDF2、AES-GCM/ChaCha20-Poly1305 4 大端到端套件，100% PASS。
- [`cap/test/test_object_destructure.cp`](file:///data/dev/cp/cap/test/test_object_destructure.cp)：匿名结构体元组解构测试，100% PASS。
- 全量回归测试：`test_http_full.cp`, `test_sync_primitives.cp`, `test_zero_copy_json.cp`, `test_io_streams.cp`, `test_path_url.cp` 等全部 100% PASS。

---

## [2026-11-13] - 核心里程碑：全面收编 Bun 工业级 HTTP 全协议栈引擎（Step 1/2/3 闭环）与底层编译器自愈

### 工业级 HTTP 核心体系完整迁入 (Added)

- **Step 1: 协议类型与基础设施助手 (Types & Infrastructure)**:
  - [`lib/http/encoding.cp`](file:///data/dev/cp/lib/http/encoding.cp)：`Encoding` 枚举（`Identity`, `Gzip`, `Deflate`, `Brotli`, `Zstd`, `Chunked`）与 `canUseLibDeflate()`、`isCompressed()`；
  - [`lib/http/fetch_modes.cp`](file:///data/dev/cp/lib/http/fetch_modes.cp)：`FetchCacheMode`, `FetchRedirect`, `FetchRequestMode` 及其字符串双向映射；
  - [`lib/http/cert.cp`](file:///data/dev/cp/lib/http/cert.cp)：`HTTPCertError`, `CertificateInfo`, `InitError`；
  - [`lib/http/url_path.cp`](file:///data/dev/cp/lib/http/url_path.cp)：`URLPath` 极速路径与参数切片分析器（`extname`, `path`, `pathname`, `first_segment`, `query_string`, `isRoot()`）；
  - [`lib/http/signals.cp`](file:///data/dev/cp/lib/http/signals.cp)：`Signals` 与 `SignalStore` 原子生命周期状态存储器；
  - [`lib/http/header_iterator.cp`](file:///data/dev/cp/lib/http/header_iterator.cp)：`HeaderValueIterator` 逗号分隔报头零拷贝切片迭代器；
  - [`lib/http/request_body.cp`](file:///data/dev/cp/lib/http/request_body.cp)：`HTTPRequestBody` 与 `HTTPRequestBodyTag` 请求体封装；
  - [`lib/http/internal_state.cp`](file:///data/dev/cp/lib/http/internal_state.cp)：`HTTPVerboseLevel`, `HTTPUpgradeState`, `Protocol`, `ShouldContinue`, `Stage`, `InternalStateFlags`, `InternalState` 客户端状态机。
- **Step 2: HTTP/2 二进制帧与 HTTP/3 客户端子系统 (H2 & H3 Subsystems)**:
  - [`lib/http/h2/frame.cp`](file:///data/dev/cp/lib/http/h2/frame.cp)：RFC 7540 HTTP/2 二进制帧协议（`CLIENT_PREFACE`, `FrameType`, `ErrorCode`, `SettingsType`, `FrameHeader` 9 字节帧头编解码, `SettingsPayloadUnit`）；
  - [`lib/http/h2/client.cp`](file:///data/dev/cp/lib/http/h2/client.cp)：`H2Stream`, `H2StreamState`, `H2ClientSession` 多路复用流会话与窗口控制器；
  - [`lib/http/h2/index.cp`](file:///data/dev/cp/lib/http/h2/index.cp)：HTTP/2 子系统统一聚合导出；
  - [`lib/http/h3/alt_svc.cp`](file:///data/dev/cp/lib/http/h3/alt_svc.cp)：RFC 7838 Alt-Svc 协商报头解析器（`AltSvc`, `AltSvcEntry`）；
  - [`lib/http/h3/client.cp`](file:///data/dev/cp/lib/http/h3/client.cp)：`H3Stream`, `H3StreamType`, `H3ClientSession` QUIC 双向请求流会话；
  - [`lib/http/h3/index.cp`](file:///data/dev/cp/lib/http/h3/index.cp)：HTTP/3 子系统统一聚合导出。
- **Step 3: 工业级核心客户端引擎与生命周期 (Client Engine & Lifecycle)**:
  - [`lib/http/decompressor.cp`](file:///data/dev/cp/lib/http/decompressor.cp)：`DecompressorType`, `Decompressor` 流式解压缩适配器；
  - [`lib/http/proxy.cp`](file:///data/dev/cp/lib/http/proxy.cp)：RFC 7231 `ProxyTunnel` HTTP CONNECT 代理隧道握手与校验器；
  - [`lib/http/sendfile.cp`](file:///data/dev/cp/lib/http/sendfile.cp)：基于 Linux 内核 `sendfile` 的零用户态拷贝文件上传器；
  - [`lib/http/context.cp`](file:///data/dev/cp/lib/http/context.cp)：`HTTPContext`, `PooledSocket` Keep-Alive 跨域套接字连接池与死链接驱逐；
  - [`lib/http/client.cp`](file:///data/dev/cp/lib/http/client.cp)：吸收自 Bun 2843 行工业核心的 `HTTPClient`, `HTTPClientResult`，完整实现 RFC 7230 请求格式化、分块传输解码（`decodeChunkedBody`）与状态解析；
  - [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp)：更新聚合导出全套 HTTP 工业能力。

### 编译器增强与热修自愈 (Compiler & Hotfix)

- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增强类型系统，打通 `Type::String` 与 `Type::Array(Box::new(Type::U8))` / `Type::SentinelSlice`（`[]const u8`）的自动安全双向转换，解决字符串字面量赋值给字节切片字段的类型不匹配阻断。
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs) / [`cpc_dev/src/codegen/mod.rs`](file:///data/dev/cp/cpc_dev/src/codegen/mod.rs):
  - 增强 `lower_struct_body`：原生支持将 CP 原始指针类型 `*!?` 转换为 `?*anyopaque`、`*!` 转换为 `*anyopaque`，将函数类型 `?fn(...)` 转换为 `?*const fn(...)`、`fn(...)` 转换为 `*const fn(...)`。
- [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh):
  - 同步部署更新 `/data/dev/cp/cap/bin/cpc` 与 `/data/dev/cp/cpc/target/release/cpc`，保证 `cap` 工具链始终调用最新构建的编译器。
- 下游 Bun AGY 阻断结单：
  - 极速排查并解决任务 `task-struct-body-lower-rawptr-fn`，测试验证通过，结单归档。

### 测试验证 (Tests)

- [`cap/test/test_http_full.cp`](file:///data/dev/cp/cap/test/test_http_full.cp)：全新端到端全量综合测试套件，覆盖 Step 1, Step 2, Step 3 所有原语与流程，100% PASS。
- [`cap/test/test_http_features.cp`](file:///data/dev/cp/cap/test/test_http_features.cp)：现有基础套件 100% PASS。
- [`cap/test/test_sync_primitives.cp`](file:///data/dev/cp/cap/test/test_sync_primitives.cp)：现代化去除废弃 `try` 语法，8 大并发套件 100% PASS。
- 全量标准库回归：14 个标准库核心测试套件 100% PASS。

---

## [2026-11-12] - 架构清理：全面删除根目录冗余门面文件、规范单一真理来源

### 标准库清理与重构 (Refactored & Removed)

- **彻底删除 14 个根目录同名空壳门面文件**：
  - 删除 `lib/io.cp`, `lib/sync.cp`, `lib/router.cp`, `lib/http.cp`, `lib/path.cp`, `lib/url.cp`, `lib/encoding.cp`, `lib/glob.cp`, `lib/semver.cp`, `lib/dotenv.cp`, `lib/ini.cp`, `lib/which.cp`, `lib/ws.cp`, `lib/json.cp`；
  - 杜绝“既有目录又有同名门面”的双重文件形态，所有多文件复合库一律以 `lib/<module>/index.cp` 作为唯一入口，根目录保持纯正扁平清爽。
- [`lib/http/`](file:///data/dev/cp/lib/http/):
  - 新增 `request.cp`：`Request` 类定义；
  - 新增 `response.cp`：`Response` 类定义；
  - 新增 `server.cp`：`Server`、`fetch`、`json`、`serve` 服务与客户端函数；
  - 更新 `index.cp` 统一对外公开导出全部 HTTP 契约与能力。
- 清理空目录：移除 `lib/io_ext` 与 `lib/json_ext`。

### 编译器修复 (Fixed)

- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) / [`cpc_dev/src/codegen/expr.rs`](file:///data/dev/cp/cpc_dev/src/codegen/expr.rs):
  - 修复 `r#"..."#` 原生字符串中误将 `@import("std")` 写为 `\"std\"` 导致生成的 Zig 语法中带有非法反斜杠的问题。

### 规范手册更新 (Docs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 第 1.1 与 1.2 节全面更新标准库组织规范：明确复合模块由 `lib/<name>/index.cp` 独立负责，杜绝根目录堆积多余门面。

## [2026-11-12] - 阶段审查：原生 Re-export 落地、lib/io 目录化重构与全标准库规范化审计

### 新特性与编译器增强 (Added & Compiler)

- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 在 `ModuleItem` 枚举中新增 `Export(ExportStmt)`，支持记录导出符号列表与可选来源路径 `source`。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 增加 `parse_export_stmt` 与 `parse_export_star_stmt`，完整支持 `export { foo, bar } from "./path";`、`export { foo, bar };` 与 `export * from "./path";`。
- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) / [`cpc_dev/src/resolver.rs`](file:///data/dev/cp/cpc_dev/src/resolver.rs):
  - 提取公共 `resolve_source_path` 模块路径解析函数；在 `resolve_dependencies` 与 `resolve_imports` 中对 `ModuleItem::Export` 的来源文件递归展开解析与依赖收集。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 语义分析符号表中对 `ModuleItem::Export` 导出标识符进行符号声明登记与合法性校验。
- [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs):
  - 在生成运行时预加载时扫描 `ModuleItem::Export`，确保重导出的内部能力得到正确支持。

### 标准库重构与规范化 (Refactored)

- [`lib/io/`](file:///data/dev/cp/lib/io/) & [`lib/io.cp`](file:///data/dev/cp/lib/io.cp):
  - 严格遵循 `xxx_0.cp` 演进规则，将原单文件暂存为 `lib/io_0.cp`，创建模块化目录 `lib/io/`；
  - 拆分为 `traits.cp`、`buffer_stream.cp`、`stdio.cp`、`max_buf.cp`、`pipe.cp` 以及高内聚出口 `index.cp`；
  - 100% 覆盖原功能后安全移除 `lib/io_0.cp`，重写根目录 `lib/io.cp` 门面为规范的 `export * from "./io/index";`。
- 全量标准库重导出语法统一改造：
  - 更新 `lib/sync/index.cp` & `lib/sync.cp`
  - 更新 `lib/router/index.cp` & `lib/router.cp`
  - 更新 `lib/http/index.cp` & `lib/http.cp`
  - 更新 `lib/path/index.cp` & `lib/path.cp`
  - 更新 `lib/url/index.cp` & `lib/url.cp`
  - 更新 `lib/encoding/index.cp` & `lib/encoding.cp`
  - 更新 `lib/glob/index.cp` & `lib/glob.cp`
  - 更新 `lib/semver/index.cp` & `lib/semver.cp`
  - 更新 `lib/dotenv/index.cp` & `lib/dotenv.cp`
  - 更新 `lib/ini/index.cp` & `lib/ini.cp`
  - 更新 `lib/which/index.cp` & `lib/which.cp`
  - 更新 `lib/ws/index.cp` & `lib/ws.cp`
  - 更新 `lib/json/index.cp` & `lib/json.cp` & `lib/serde.cp`
  - 更新 `lib/stream.cp`
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 增加第 1.1 节重导出规范与门面模式使用指南。

## [2026-11-12] - 标准库第五阶段：可执行程序定位器、WebSocket 帧协议与文件系统原子增强

### 新特性 (Added)

- [`lib/which/`](file:///data/dev/cp/lib/which/) & [`lib/which.cp`](file:///data/dev/cp/lib/which.cp):
  - `which.cp`：吸收自 Bun 的跨平台可执行程序快速定位器，支持绝对路径、相对路径以及 `PATH` 环境变量遍历；在 Windows 下自动推导 `.exe`、`.cmd`、`.bat`；基于 `std.c.access(path, 1)`（`X_OK`）检测执行权限；
  - `index.cp` & `lib/which.cp`：官方标准库导出通道 `cap:which`。
- [`lib/ws/`](file:///data/dev/cp/lib/ws/) & [`lib/ws.cp`](file:///data/dev/cp/lib/ws.cp):
  - `opcode.cp`：标准 RFC 6455 `Opcode`（`Text`, `Binary`, `Close`, `Ping`, `Pong`, `Continue`）与 `CloseCode` 状态码；
  - `mask.cp`：RFC 6455 规范 4 字节 XOR 掩码编解码；
  - `frame.cp`：`FrameHeader` 支持 7 位基础、16 位中型与 64 位超大型载荷扩展与掩码解析，`FrameBuilder` 提供便捷帧构造方法；
  - `index.cp` & `lib/ws.cp`：官方标准库导出通道 `cap:ws`。
- [`lib/fs.cp`](file:///data/dev/cp/lib/fs.cp):
  - 扩充 `tmpDir()` 跨平台系统临时目录探测；
  - 扩充 `tmpPath()` 原子序列号与单调时钟纳秒时间戳保证唯一临时文件路径；
  - 扩充 `writeAtomic()` 崩溃安全的原子覆盖写入。
- 测试用例套件：
  - [`cap/test/test_which.cp`](file:///data/dev/cp/cap/test/test_which.cp)：可执行二进制定位与 PATH 解析全部 100% PASS；
  - [`cap/test/test_ws.cp`](file:///data/dev/cp/cap/test/test_ws.cp)：4 大 WebSocket 帧协议与掩码往返套件全部 100% PASS；
  - [`cap/test/test_fs_extensions.cp`](file:///data/dev/cp/cap/test/test_fs_extensions.cp)：临时文件与原子写入套件全部 100% PASS。

### 文档更新 (Docs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 第 12.1 节总表中更新 `cap:which` 与 `cap:ws` 状态为已全面实装；
  - 新增第 12.15 节 `cap:which`、第 12.16 节 `cap:ws` 与第 12.17 节 `cap:fs` 增强能力的详细技术契约与完整代码示例。

## [2026-11-12] - 标准库第四阶段：Unix Glob 模式匹配、Semver 2.0 版本解析与范围匹配、DotEnv 与 INI 配置解析

### 新特性 (Added)

- [`lib/glob/`](file:///data/dev/cp/lib/glob/) & [`lib/glob.cp`](file:///data/dev/cp/lib/glob.cp):
  - `matcher.cp`：基于路径分段（Segment-based）递归回溯匹配算法的工业级 Glob 引擎，支持 `*` 单段任意字符匹配、`**` 多级目录递归匹配、`?` 单字符通配、`[a-z]` / `[0-9]` 字符范围集合匹配、`!` 取反以及 `hasMagic()` 极速通配符探测；
  - `index.cp` & `lib/glob.cp`：官方标准库导出通道 `cap:glob`。
- [`lib/semver/`](file:///data/dev/cp/lib/semver/) & [`lib/semver.cp`](file:///data/dev/cp/lib/semver.cp):
  - `version.cp`：Semver 2.0 规范语义版本解析器与格式化器（支持 `v1.2.3`, `=2.10.5-beta.1+sha.abcdef` 等格式），严格遵循 Semver 2.0 优先级铁律（正式发布版 > 预发布版，点分数字按数值比较，字符按 ASCII 比较）；
  - `range.cp`：支持 Caret 锁版本（`^`）、Tilde 补丁更新（`~`）、不等式（`>=`, `<=`, `>`, `<`）、连字符范围（`1.0.0 - 2.0.0`）、逻辑或（`||`）与通配符（`*` / `x` / `X`）；
  - `index.cp` & `lib/semver.cp`：官方标准库导出通道 `cap:semver`。
- [`lib/dotenv/`](file:///data/dev/cp/lib/dotenv/) & [`lib/dotenv.cp`](file:///data/dev/cp/lib/dotenv.cp):
  - `loader.cp`：100% 栈分配零堆垃圾 `.env` 解析器，支持 `KEY=VALUE`、`KEY: VALUE` 与 `export KEY=VALUE`；支持单引号原样保护、双引号转义解析（`\n`, `\r`, `\t` 等）与内联注释安全截断；提供 `get()`, `getOr()`, `has()`；
  - `index.cp` & `lib/dotenv.cp`：官方标准库导出通道 `cap:dotenv`。
- [`lib/ini/`](file:///data/dev/cp/lib/ini/) & [`lib/ini.cp`](file:///data/dev/cp/lib/ini.cp):
  - `parser.cp`：工业级 INI 解析器，支持全局配置区与 `[section_name]` 独立段空间；支持 `;` 与 `#` 双注释符及行尾注释；提供 `get(section, key)`, `getGlobal(key)`, `hasSection(section)`, `getOr()`；
  - `index.cp` & `lib/ini.cp`：官方标准库导出通道 `cap:ini`。
- 测试用例套件：
  - [`cap/test/test_glob.cp`](file:///data/dev/cp/cap/test/test_glob.cp)：4 大 Glob 通配与模式匹配套件全部 100% PASS；
  - [`cap/test/test_semver.cp`](file:///data/dev/cp/cap/test/test_semver.cp)：4 大 Semver 解析、比较、格式化与范围匹配套件全部 100% PASS；
  - [`cap/test/test_config.cp`](file:///data/dev/cp/cap/test/test_config.cp)：2 大 DotEnv 与 INI 配置解析套件全部 100% PASS。

### 文档更新 (Docs)

- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 第 12.1 节已实装标准库总览表中新增 `cap:glob`, `cap:semver`, `cap:dotenv`, `cap:ini`；
  - 第 12.4 ~ 12.14 节全量深度扩充详细技术契约与全套完整可运行代码示例（涵盖 `cap:sync`, `cap:stream`, `cap:json`/`cap:serde`, `cap:router`, `cap:http`, `cap:path`, `cap:url`, `cap:encoding`, `cap:glob`, `cap:semver`, `cap:dotenv`, `cap:ini`）。

## [2026-11-12] - 标准库第三阶段：Radix-Tree 路由引擎、HTTP 协议栈、跨平台路径与 WHATWG URL、Base64 编码

### 新特性 (Added)

- [`lib/router/`](file:///data/dev/cp/lib/router/) & [`lib/router.cp`](file:///data/dev/cp/lib/router.cp):
  - `param_map.cp`：纯栈 16 项定长零堆分配参数映射字典 `ParamMap`，提供 `get()`, `at()`, `has()`, `append()`, `count()`, `isEmpty()`；
  - `route.cp`：`RouteMatch<T>` 泛型路由匹配结构体；
  - `radix.cp`：8 树 HTTP 动词前缀树 `Router<T>` 与 `RadixNode<T>`，支持静态路径、动态参数（`:param` 与 `[param]`）、通配符（`*path` 与 `[...path]`）与 `ALL` 方法回退；
  - 新增 `lib/router.cp` 作为 `cap:router` 官方通道出口。
- [`lib/http/`](file:///data/dev/cp/lib/http/) & [`lib/http.cp`](file:///data/dev/cp/lib/http.cp):
  - `method.cp`：标准 HTTP `Method` 枚举与 `isIdempotent()`, `hasRequestBody()`, `toString()`, `fromString()`；
  - `headers.cp`：大小写不敏感的高性能 HTTP `Headers` 集合，支持 `get()`, `set()`, `append()`, `delete()`, `count()`, `at()`；
  - `etag.cp`：`ETag` 生成与强弱 `ifNoneMatch()` 条件匹配；
  - `mime.cp`：`MimeType` 常见文件后缀与文件路径极速映射；
  - `lib/http.cp`：导出 `Method`, `Headers`, `HeaderEntry`, `ETag`, `ParsedETag`, `MimeType`。
- [`lib/path/`](file:///data/dev/cp/lib/path/) & [`lib/path.cp`](file:///data/dev/cp/lib/path.cp):
  - `posix.cp`：POSIX 标准路径计算（`basename`, `dirname`, `extname`, `isAbsolute`, `normalize`, `join`）；
  - `windows.cp`：Windows 驱动器盘符、UNC 路径与反斜杠处理；
  - `index.cp`：跨平台统一 `Path`（以及 `path.posix`、`path.win32`）；
  - 新增 `lib/path.cp` 作为 `cap:path` 官方通道出口。
- [`lib/url/`](file:///data/dev/cp/lib/url/) & [`lib/url.cp`](file:///data/dev/cp/lib/url.cp):
  - `query.cp`：`URLSearchParams` 查询字符串解析、修改与序列化；
  - `url.cp`：WHATWG 规范 `URL` 对象（解析 `protocol`, `username`, `password`, `host`, `hostname`, `port`, `pathname`, `search`, `hash`, `origin`, `searchParams`）；
  - 新增 `lib/url.cp` 作为 `cap:url` 官方通道出口。
- [`lib/encoding/`](file:///data/dev/cp/lib/encoding/) & [`lib/encoding.cp`](file:///data/dev/cp/lib/encoding.cp):
  - `base64.cp`：纯 CP 编写的高性能 Base64 编解码器（标准 Standard 模式、URL-Safe 模式、无填充模式、自适应统一解码 `+`/`-` 与 `/`/`_`、无堆分配与内存分配两种模式）；
  - 新增 `lib/encoding.cp` 作为 `cap:encoding` 官方通道出口。
- 测试用例套件：
  - [`cap/test/test_router.cp`](file:///data/dev/cp/cap/test/test_router.cp)：3 大路由套件（静态、动态、通配符与 ALL 回退全部 100% PASS）；
  - [`cap/test/test_http_features.cp`](file:///data/dev/cp/cap/test/test_http_features.cp)：4 大 HTTP 协议特性全部 100% PASS；
  - [`cap/test/test_path_url.cp`](file:///data/dev/cp/cap/test/test_path_url.cp)：4 大 POSIX/Windows 路径与 WHATWG URL 套件全部 100% PASS；
  - [`cap/test/test_encoding.cp`](file:///data/dev/cp/cap/test/test_encoding.cp)：3 大 Base64 编解码套件全部 100% PASS。

## [2026-11-12] - 标准库第二阶段：并发同步基础设施、无锁原语、管道流与零拷贝 JSON

### 新特性 (Added)

- [`lib/sync/`](file:///data/dev/cp/lib/sync/):
  - `futex.cp`：跨平台系统级用户态同步原语，封装 Linux `SYS_futex`、macOS `__ulock`、Windows `WaitOnAddress`，提供纳秒单调时钟 `Deadline` 与超时等待；
  - `mutex.cp`：吸收 Bun 成熟的自旋与 Futex 混合互斥锁（`tryLock`、`lock`、`unlock`、`withLock`）；
  - `condition.cp`：跨平台条件变量（`wait`、`timedWait`、`signal`、`broadcast`）；
  - `wait_group.cp`：并发信号量计数器（`add`、`done`、`wait`、`finish`）；
  - `channel.cp`：线程安全有界 MPMC 数据通道，支持非阻塞与超时阻塞收发；
  - `ring_buffer.cp`：基于 Dmitry Vyukov 经典算法的定长多生产者多消费者（MPMC）无锁环形队列 `MpmcQueue<T>`；
  - `unbounded_queue.cp`：侵入式无界并发无锁链表队列 `UnboundedQueue<T>`；
  - `wyhash.cp`：从 Bun 移植的 64 位极速无状态非加密哈希算法 `Wyhash`；
  - `concurrent_map.cp`：基于 Wyhash 16 桶分片的高并发分段哈希映射表 `ConcurrentMap<V>`；
  - `arc_slice.cp`：原子引用计数切片 `ArcSlice<T>`，支持跨线程零拷贝共享与自动析构；
  - `index.cp` & `lib/sync.cp`：模块导出与对外统一标准库入口。
- [`lib/io.cp`](file:///data/dev/cp/lib/io.cp) & [`lib/stream.cp`](file:///data/dev/cp/lib/stream.cp):
  - 扩充 `MaxBuf`（动态零拷贝切片缓冲守卫）；
  - 扩充 `PipeReader`（实现 `Reader, Closer`）与 `PipeWriter`（实现 `Writer, Closer`）；
  - 扩充 `Pipe.create()` 工厂函数，支持快速创建双端数据管道；
  - 新增 `lib/stream.cp` 作为 `cap:stream` 官方通道出口。
- [`lib/json/`](file:///data/dev/cp/lib/json/), [`lib/json.cp`](file:///data/dev/cp/lib/json.cp), [`lib/serde.cp`](file:///data/dev/cp/lib/serde.cp):
  - `pool.cp`：移植 Bun 内存池机制 `JsonBufferPool`，实现多轮解析内存复用；
  - `value.cp`：零拷贝 AST `JsonValue`，直接借用源输入切片；
  - `parser.cp`：极速流式递归下降 JSON 解析器 `JsonParser`；
  - 新增 `lib/serde.cp` 作为 `cap:serde` 序列化统一出口通道。
- [`cap/test/test_sync_primitives.cp`](file:///data/dev/cp/cap/test/test_sync_primitives.cp):
  - 覆盖 Wyhash、Futex、Mutex、Condition、WaitGroup、MpmcQueue、Channel、ConcurrentMap、ArcSlice 8 大用例套件。
- [`cap/test/test_zero_copy_json.cp`](file:///data/dev/cp/cap/test/test_zero_copy_json.cp):
  - 验证基础标量、源缓冲区指针直通零拷贝借用及嵌套对象/数组检索。

### 优化与修复 (Fixed)

- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs) / [`cpc_dev/src/codegen/mod.rs`](file:///data/dev/cp/cpc_dev/src/codegen/mod.rs):
  - 在生成 Trait 虚表结构体时，若方法返回类型以 `!` 开头（错误联合类型），将其转译为 `anyerror{ret}`，彻底消除 Zig 函数指针类型无法使用裸 `!T` 的语法错误。
- [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs) / [`cpc_dev/src/codegen/stmt.rs`](file:///data/dev/cp/cpc_dev/src/codegen/stmt.rs):
  - 在生成 `__{var}_moved = true;` 标志位赋值时，增加 `self.drop_flag_vars.contains(&id.sym)` 守卫，避免对未注册 drop 标志的变量生成非法标识符引用。
- [`cap/test/test_json.cp`](file:///data/dev/cp/cap/test/test_json.cp):
  - 修复测试用例中将 `const conf` 进行成员字段修改的语义错误，改为 `let conf`。

## [2026-11-12] - 编译器核心 Codegen 模块化分拆与 Trait 作用域强化

### 架构演进 (Changed)

- [`cpc/src/codegen/`](file:///data/dev/cp/cpc/src/codegen/):
  - 彻底废除 1.7 万行巨石单体文件 `codegen.rs`，重构分拆为 8 个高内聚子模块：
    - `mod.rs`（3,398 行）：上下文派发中心、`Codegen` 结构体、代码缩进派发与模块级顶层项（Class/Trait/Enum/Impl/Interface/Reflection）；
    - `prelude.rs`（614 行）：`ModuleFeatures` AST 特性扫描与运行时环境感知；
    - `types.rs`（2,601 行）：类型推导、Zig 原生类型映射与常量泛型单态化；
    - `borrow.rs`（960 行）：NLL 借用检查与跨作用域冲突拦截；
    - `pattern.rs`（1,587 行）：模式匹配、穷尽性分析与 `switch` 展开；
    - `actor.rs`（1,577 行）：CPS 状态机切分、异步闭包变量捕获与 Actor 协程逻辑；
    - `stmt.rs`（2,114 行）：控制流语句生成（`let`、`if`、`while`、`for`、`return`、`defer`、`errdefer`）；
    - `expr.rs`（4,332 行）：表达式求值、运算符重载、方法调用与内联闭包。
  - 单文件规模降至 1,000 ~ 4,000 行，消除跨 800 行分段读取开销，全面赋能多子智能体并行开发。

### 优化与修复 (Fixed)

- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs):
  - **Trait 虚表局部作用域强化**：在 Trait 与 Interface 声明生成中，将虚表指针类型升级为 `vtable: *const @This().VTable`，彻底杜绝与业务代码中的顶层 `class VTable` 发生 Zig 符号歧义。

## [2026-11-12] - 方案 A 工业级统一 I/O 流体系与 C 变量外部绑定支持

### 新特性 (Added)

- [`lib/io.cp`](file:///data/dev/cp/lib/io.cp):
  - 新增统一流错误类型 `IOError`（`EndOfStream`, `UnexpectedEof`, `Closed`, `BufferFull`, `ShortWrite`, `InvalidSeek`）；
  - 新增寻址基准枚举 `SeekOrigin`（`Start`, `Current`, `End`）；
  - 新增统一流 Trait 契约：`Reader`、`Writer`、`Closer`、`Seeker`；
  - 新增零拷贝内存切片只读流 `BufferReader`，支持无堆分配读取与 Seek；
  - 新增内存定长安全写入器 `BufferWriter`（4096 字节），支持字节提取与重置；
  - 新增高性能泛型缓冲读取器 `BufferedReader<R: Reader>`（256 字节内部缓存，支持 `readByte()` 单字节速读）；
  - 新增高性能泛型缓冲写入器 `BufferedWriter<W: Writer>`（256 字节内部缓存，支持自动/手动 `flush()`）；
  - 新增流式全量传输管道 `copyStream<R: Reader, W: Writer>` 与定额流搬运 `copyN`；
  - 100% 保持已有 `Console`、`console`、`Stdio`、`stdio`、`print` 向后兼容。
- [`cap/test/test_io_streams.cp`](file:///data/dev/cp/cap/test/test_io_streams.cp):
  - 包含 6 大端到端用例套件：`BufferReader` 切片读取与 Seek 验证、`BufferWriter` 写入与字节校验、`BufferedReader` 缓冲与单字节速读、`BufferedWriter` 批量写入与 flush、`copyStream` 管道全量传输、`copyN` 定额截断传输。
- [`cap/test/test_extern_var.cp`](file:///data/dev/cp/cap/test/test_extern_var.cp):
  - 验证 `@c extern const` 与顶层 `extern var` 的 C 外部符号绑定。

### 优化与修复 (Fixed)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 扩展 `parse_decl` 支持在 `extern` 之后解析 `const`/`var`/`let`，正确标记 `is_extern` 与 `is_c`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增加对定长数组切片引用的元素类型匹配检查；
  - 在 `Expr::New` 类型检查时剔除泛型参数 `<...>`，精准定位基类定义与方法签名。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 对 `Pattern::Ident("_")` 及通配符模式生成 `_ = expr;` 丢弃语句，消除 Zig 下非法 `var _` 错误；
  - 在异步状态机闭包上下文提取中跳过 `_` 变量，防止生成非法字段名；
  - 泛型类的 `_create` 构造函数统一采用 `@This()` 申请内存，解决单态化类型缺失 `_create` 的问题；
  - `lookup_method` 剥离类型中的泛型参数后缀，正确解析泛型实例的方法返回值类型。

## [未发布] - 2026-11-13

### 新增 (Added)

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 建立专为 AI Agent 设计的高密度（~220行）语法契约手册，涵盖模块导入导出、变量绑定与可变性、类型系统与裸指针、结构体与类、枚举与模式匹配、解构赋值、现代错误处理及 Top 10 禁手清单。
- [`GEMINI.md`](file:///data/dev/cp/GEMINI.md):
  - 增补第 2.4 节双轨语言规范维护机制；
  - 增补第 4 节自主推导持续工作流闭环规范；
  - 增补第 6.5 节配额耗尽倒计时与 `schedule` 挂起协议；
  - 增补第 7.1.3 节双智能体通信透明化与后台守护进程自动启动检查。
- [`lib/event_loop/`](file:///data/dev/cp/lib/event_loop/):
  - 建立工业级统一事件循环标准库（`cap:event_loop`）：
    - `timer.cp`：基于侵入式二叉最小堆的定时器系统（`Timer`, `TimerHeap`），支持精确到毫秒的超时管理与 $O(\log N)$ 节点注销；
    - `concurrent_task.cp`：16 字节跨线程无锁并发任务（`ConcurrentTask`, `ConcurrentQueue`），由 `Mutex` 保护；
    - `deferred_task.cp`：微任务队列（`DeferredTaskQueue`），实现上下文指针自动去重；
    - `file_poll.cp`：基于 `std.posix.poll` 的系统描述符监听引擎（`PollEngine`, `PollEvents`）；
    - `loop.cp`：统一 `EventLoop` 生命周期驱动（`tick` 单步步进与 `run` 常驻循环）；
    - `index.cp`：模块统一出口。
- [`cap/test/test_event_loop.cp`](file:///data/dev/cp/cap/test/test_event_loop.cp):
  - 5 大端到端回归测试套件：二叉最小堆定时器调度、16 字节并发任务队列、微任务队列与去重、I/O 描述符轮询引擎、统一 EventLoop 综合生命周期。

### 优化与修复 (Fixed)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 解析 `[]const T` 时，保留 `has_const` 标志并将其构建为不可变借用切片 `Type::BorrowType { inner: Box::new(Type::Array(...)), is_mut: false }`；未带 `const` 的 `[]T` 保持为可变数组切片 `Type::Array(...)`。
- [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs) / [`cpc_dev/src/codegen/types.rs`](file:///data/dev/cp/cpc_dev/src/codegen/types.rs):
  - `Type::Array(inner)` 映射生成 Zig 可变切片 `[]T`，解除动态分配切片元素只读限制；
  - `Type::BorrowType { is_mut: false, inner: Array(elem) }` 映射生成 Zig 只读切片 `[]const T`，保障字符串切片等参数调用的常量性契约。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 增补第 12.19 节工业级统一事件循环体系（`cap:event_loop`）设计规范与完整使用代码示例；调整路线图章节序号为 12.20。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 全面按照用户 5 步法重构重写，提炼出 605 行完备且纯正的 CP 语法契约速查手册；
  - 彻底清除 `var`、`let mut` 与 `threadlocal`，确立 `let` / `const` 纯净绑定与 `move` / `own` 所有权转移；
  - 完善参数借用与裸指针分类（`&T` 只读借用、`*T` 可变借用、`*T!` 裸指针、`move T`/`own T` 所有权转移）；
  - 规范四大代码块为 `zig { ... }`、`unsafe { ... }`、`arena { ... }` 与 `@c { ... }`；
  - 增补泛型与约束、常量泛型、类型别名及原生运算符矩阵；
  - 升级 Top 15 绝对禁手清单。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 全面审核并清洗第 12 章中遗留的历史代码示例，将旧版 `var` 统一修正为 `let`，将 Zig 前缀 `try` 替换为 CP 后缀 `?`，将 `for()|i|` 修正为 `for (i of ...)`，消除与 `cp_agent_ref.md` 的任何潜在不一致。
- [`lib/security/csrf.cp`](file:///data/dev/cp/lib/security/csrf.cp):
  - 将所有 `__zig__` 替换为标准 `zig` 逃逸块，将所有 `var` 替换为现代 `let` 变量声明。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 全面扩充 17 大核心系统特性与 Bun Zig 迁移必备语法（657 行）：
    - 引入直接透明导入 `.zig` 源码文件语法及前柔后刚两级类型模型；
    - 补充 `@c struct`、`@root struct`、`@value struct`、`@align(N)`、`@refCount(cleanup)` 系统级装饰器；
    - 详述多字段分离借用（Disjoint Field Borrowing）、`ref struct` 栈绑定视图与显式生命周期；
    - 丰富 C 指针矩阵：`[*c]T`、`[*:0]T!`、`[*]([*:0]u8!)?!`、`*!`、`*!?`、`cstr` vs `cstring`；
    - 补全编译期函数与 Comptime 内省原语、AST 模板插值宏与元编程体系；
    - 全景覆盖 `switch` 12 种模式解构（含结构体整包捕获、元组解构、Walrus 绑定、动态类型匹配、三态错误 downcast）；
    - 补充多线程并发容器 `Shared<T>` 与内部可变性容器 `Shared(Mutex(T))` / `Shared(RwLock(T))`；
    - 扩充 Top 18 绝对禁手清单。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 7 章系统级内存布局列表，将 `@extern struct` 统一更正为现代规范 `@c struct`，并补充 `@root struct` 与 `@align(N)`。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 逐章逐节对照 `cp_ref.md` 展开穷尽式查漏补缺，手册全面扩充至 820 行：
    - 补入命名空间全量导入 `import * as math` 与 Zig 风格原生物理导入 `const bun = @import("bun")`；
    - 新增第四章专门阐述 6.7.2 变量常量性与指针只读性的正交解耦（`const p: *T!`、`let p: *const T!`、`const p: *const T!`）；
    - 补全 6.7.3 获取变量裸指针的标准方式（字面量语法 `*v!` / `**v!` 与安全借用转换 `&v as *T!` / `*v as *T!`）；
    - 补全 6.7.4 完整裸指针类型转换矩阵 in `unsafe`（指针互转、物理内存整型地址、通用裸指针与未定长多项指针）；
    - 明确 `zig {}` 内嵌逃逸舱完全脱离 CP 借用检查的系统级定义；
    - 系统重构第九章错误体系：严密区分 `T!`（全局 `AnyErr`，禁止动态装箱）与 `T ! Err`（业务动态结构体，显式签名），补齐具名错误集、多错误联合声明 `T ! E1 || E2` 与 `from` 桥接机制。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 彻底解除行数限制，全面对齐 `cp_ref.md` 语法体系，扩充至 930+ 行权威智能体契约手册：
    - 补全第五章 `static class` 与 `static struct` 纯静态命名空间容器规则（全员自动静态、严禁实例化、无 `init`/`release`、禁止 `extends`）；
    - 补全第六章成员默认可见性差异：`struct` 成员默认 `pub`（公开透明 POD），`class` 成员默认 `priv`（强封装私有）；
    - 补全第一章模块级 `pub` 等价于 `export` 并作为官方首选推荐的规范；
    - 补全第六章 `class` 单继承生命周期体系：`class Child extends Parent`，`super.init(...)` 构造器调用，`pub fn release(*_)` 确定性析构钩子，`super.method(...)` 父类方法调用；
    - 确立 `@c` 全面替代 `@callconv(.c)`（同步更新第 VI.4 节与禁手清单 #9）；
    - 补全第七章字符串模板插值 `${expr}`、原生正则表达式字面量 `/pattern/`、管道操作符 `|>` 与通配占位符 `*`；
    - 补全第八章海象迭代解包循环 `while (binding := expr)` 与自动分号推导（ASI）规范；
    - 补全第九章胖指针动态错误与 `downcast` 双轨解构模式匹配语法（`err(e: T)` 与 `err(e := T)`）；
    - 补全第十章编译期计算函数 `comptime fn` 与 `@func(...)`。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 彻底清除 Zig 繁琐底层强转 Builtins（`@ptrCast`, `@alignCast`, `@intCast`, `@truncate`），确立 CP 现代统一的 `as` 运算符与 `Type(val)` 构造：
    - 明确 `ptr as *TargetType!` 在代码生成时由编译器底层自动发射 `@ptrCast(@alignCast(...))`，无需开发者手工管理对齐断言；
    - 纠正结构体指针/借用字段访问为自动解引用（`alloc.is_active = true;`），彻底杜绝 Zig 风格冗余的 `alloc.*.field`；
    - 清洗内建指令表，仅保留属于 CP 编译期的内省指令（`@sizeOf`, `@alignOf`, `@typeOf`, `@typeName`, `@fieldsOf`, `@field`, `@compileError`, `@as`）；
    - 禁手清单升级为 Top 23：追加 #22 严禁 `ptr.*.field` 冗余解引用、#23 严禁手动书写 `@ptrCast(@alignCast(ptr))`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 清洗第 12 章遗留示例中的 `@ptrCast(@alignCast)` 与 `__zig__`，同步更新为规范 CP `as` 与 `zig {}`。

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) & [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - `parse_params` 与 `parse_arrow_params` 支持 `Token::Let`（`fn foo(let p: T)`）作为可变局部值拷贝形参官方规范，`E0104` 错误提示同步引导使用 `let param: Type` / `*param: Type`。
- [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs) & [`cpc_dev/src/codegen/actor.rs`](file:///data/dev/cp/cpc_dev/src/codegen/actor.rs):
  - 为 `mutable_copy_params`（`let p: T` / `mut p: T`）生成 `_cp_<name>` 底层形参并在函数入口注入 `var <name> = _cp_<name>; _ = &<name>;`，消除 Zig 常量形参不可修改与同名前置遮蔽错误。
- [`cap/test/test_let_param.cp`](file:///data/dev/cp/cap/test/test_let_param.cp):
  - 新增 `fn foo(let p: T)` 标量、结构体值拷贝与循环计数器回归测试。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 修复第 5.2、9.4、11.9.4、14.12 节内部表述冲突，修正第 485/503-504、2660、5894 行的默认导出、`handler.downcast<NetworkHandler>()`、`cap:parallel` 与 `cap:actor` 导入示例错误。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 全面重构全 13 章并经独立审计核对修正，消除全部 Zig 管道残留（`inline for`、`receive {}`）、错误语法（`downcast(T)`、`/regex/`、`pub default class`、`import { @spawn }`）及命名冲突（`toString()`），对齐 **32 大官方 `cap:*` 模块矩阵**，补齐 `cstr?`、`->`、`?.`、`.?`、`orelse`、匿名块 `({ ... break val; })`、开放枚举 `_`、Zig 常量区间、`extern const`/`let`、`inline while`、`@getPageSize(4)`、块级 `@if/@else`、`@This()`/`@is_beam`/`@panic`、`_ = expr;` 忽略返回值、原生类型实参（`std.mem.trim(u8, ...)`）、`macro AutoReset(target)` 完整模板宏示例、表达式级 `arena { @alloc(...) }` 与完整 `cap`/`cpc` CLI（定稿 990 行）。

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs), [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs), [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs), [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs), [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs), [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs), [`cpc/src/codegen/borrow.rs`](file:///data/dev/cp/cpc/src/codegen/borrow.rs), [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs), [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 实现 TypeScript 风格 `/pattern/flags` 正则表达式字面量，降级为内置 `RegExp` 类构造（支持 `i`/`m` 标志位，无需 `import`）；
  - 新增 `Expr::TypeVal` 支持 `@c fn` 函数指针类型实参、`[N]T` 定长数组类型实参（如 `std.mem.zeroes([16]u8)`）及 `*i32` / `*Struct` 指针类型实参；
  - 修复 `[*]const T?!` 多项裸指针可空修饰符剥离顺序，生成 `?[*]const T`；
  - 支持 `@Vector(N, T)` 及 `const V = @Vector(N, T);` 类型别名、定长数组/字面量到向量的类型兼容与原生逐元素向量比较；
  - 支持 `*T!` -> `*const T!` 裸指针常量协变与 `Type::ErrorUnion` 在 `?` / `? fallback` 下的解包类型推导；
  - 完结 Bun AGY 7 项编译热修任务。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md) & [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 完成两轮 Gemini 4 审计项及 Bun AGY Wave 1/2 建议的双向同步：补齐 `.Variant` 载荷简写构造、`move u |> fn(*)` 与管道尾部 `?`/`??`、`*(*T?!)` 二级可空出参裸指针、`(&T)?`/`&(T!E)` 与 `: *x T`/`: *(x | y) T`/`: *_ T` 可变借用返回、`@inline` 与 `unsafe fn`、`let * = expr;`、`Err.init(...).with_cause(move b)`、内置 `/pattern/flags` 与 `RegExp`、`@bitCast` vs `as`、SIMD 与位计数原语（`@Vector`, `@splat`, `@clz`, `@ctz`, `@popCount`, `@reduce`, `@select`, `@byteSwap`, `@bitSizeOf`）、`@typeInfo(T)` 反射、多级点路径值类型直通（`Dotted Path Types`）、`T ! FileErr || err{...}` 匿名即席错误联合、单行 `@c fn`、枚举 `.get`/`.has`、前缀 `try` 迁移兼容说明，并修正 `cp_ref.md` 中 `export` 与 `Trait` 拼写笔误。
- [`lib/net/`](file:///data/dev/cp/lib/net/)（移除单文件 `lib/net.cp`，新建扁平化网络标准库目录）：
  - 新增 [`lib/net/_usockets.cp`](file:///data/dev/cp/lib/net/_usockets.cp)：底层 `@c { ... }` uSockets C-FFI 声明、不透明结构体句柄与监听常量/状态枚举；
  - 新增 [`lib/net/address.cp`](file:///data/dev/cp/lib/net/address.cp)：`AddressFamily`、`Ipv4Addr`、`Ipv6Addr` 与 `SocketAddress` 解析及检测；
  - 新增 [`lib/net/socket.cp`](file:///data/dev/cp/lib/net/socket.cp)：`SocketState`、`SocketStats`、`SafeSocket`（含 `> 240s` 自动拆分 `long_timeout_min` 及 RAII 释放）与兼容层 `Socket`；
  - 新增 [`lib/net/tcp_stream.cp`](file:///data/dev/cp/lib/net/tcp_stream.cp)：16KB `StreamBuffer` 软塞缓冲与 `TcpStream`；
  - 新增 [`lib/net/tcp_server.cp`](file:///data/dev/cp/lib/net/tcp_server.cp)：`ListenConfig`、`TcpServer` 与兼容层 `Server` / `listen()` / `connect()`；
  - 新增 [`lib/net/udp.cp`](file:///data/dev/cp/lib/net/udp.cp)：`UdpPacket` 与 `UdpSocket` 数据报套接字抽象；
  - 新增 [`lib/net/index.cp`](file:///data/dev/cp/lib/net/index.cp)：`cap:net` 统一导出入口。
- [`cap/test/test_net.cp`](file:///data/dev/cp/cap/test/test_net.cp):
  - 扩充为覆盖 `lib/net/` 全组件及兼容 API 的 6 大端到端测试套件（6/6 PASS）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs), [`cpc/src/codegen/borrow.rs`](file:///data/dev/cp/cpc/src/codegen/borrow.rs), [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs), [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 修正 `@net_listen`、`@net_accept_async`、`@net_write_async` 的 `I32` 返回类型推导及类方法状态机回调符号生成，统一 `self`/`this`/`_` 接收者参数过滤。
- [`lib/collections/`](file:///data/dev/cp/lib/collections/)（移除单文件 `lib/collections.cp`，新建扁平化高性能容器标准库目录）：
  - 新增 [`lib/collections/bounded_array.cp`](file:///data/dev/cp/lib/collections/bounded_array.cp)：`BoundedArray<T, const CAP: usize>` 零堆分配栈上定长缓冲容器；
  - 新增 [`lib/collections/baby_list.cp`](file:///data/dev/cp/lib/collections/baby_list.cp)：16 字节紧凑动态数组 `BabyList<T>` 与 `u32` 偏移字节列表 `OffsetByteList`；
  - 新增 [`lib/collections/linear_fifo.cp`](file:///data/dev/cp/lib/collections/linear_fifo.cp)：2 的幂次位掩码环形队列 `LinearFifo<T>`；
  - 新增 [`lib/collections/hive_array.cp`](file:///data/dev/cp/lib/collections/hive_array.cp)：基于硬件 `@ctz`/`@popCount` 的 `BitSet64` 与 $O(1)$ 槽位复用池 `HiveArray<T, const CAP: usize>`；
  - 新增 [`lib/collections/string_map.cp`](file:///data/dev/cp/lib/collections/string_map.cp)：按长度过滤的快速字符串映射表 `StringEntry<V>` 与 `FastStringMap<V, const CAP: usize>`；
  - 新增 [`lib/collections/index.cp`](file:///data/dev/cp/lib/collections/index.cp)：`cap:collections` 统一导出入口。
- [`cap/test/test_collections_bun.cp`](file:///data/dev/cp/cap/test/test_collections_bun.cp) & [`cap/test/test_collections_matrix.cp`](file:///data/dev/cp/cap/test/test_collections_matrix.cp):
  - 新增 `test_collections_bun.cp`（5 大套件 100% PASS），更新 `test_collections_matrix.cp` 为现代无 `mut` / `for (...)` 语法（7 大套件 100% PASS）。
- [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs) & [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 新增 `substitute_generic_type` 实现泛型结构体/类方法返回类型参数替换（支持 `Type::Ref("T")` 与参数化 `Type::Ref("BabyList<T>")`），修复静态泛型工厂方法（`BabyList<i64>.initCapacity`）返回指针类型推导；
  - 升级内置 `VecDeque` 与 `BinaryHeap` 只读方法接收者为 `*const`。
- [`lib/mem/`](file:///data/dev/cp/lib/mem/)（移除单文件 `lib/mem.cp`，新建扁平化内存分配器与智能指针标准库目录）：
  - 新增 [`lib/mem/_mimalloc.cp`](file:///data/dev/cp/lib/mem/_mimalloc.cp)：底层 `@c { ... }` Mimalloc C-FFI 声明与 `MiHeap` 不透明句柄；
  - 新增 [`lib/mem/allocator.cp`](file:///data/dev/cp/lib/mem/allocator.cp)：`StackFallbackAllocator<const CAP: usize>`、`BumpArena`、`MaybeOwnedBuf` 及向下兼容的 `Arena`、`get_arena()`、`SharedTensor`；
  - 新增 [`lib/mem/tagged_ptr.cp`](file:///data/dev/cp/lib/mem/tagged_ptr.cp)：64 位高 16 位标签指针 `TaggedPointer` 与低 3 位标签指针 `LowTagPointer`；
  - 新增 [`lib/mem/cow.cp`](file:///data/dev/cp/lib/mem/cow.cp)：写时复制字节切片 `CowBytes`；
  - 新增 [`lib/mem/rc.cp`](file:///data/dev/cp/lib/mem/rc.cp)：`DecrementResult`、`RefCounter`、`AtomicRefCounter` 与 `WeakControlBlock`；
  - 新增 [`lib/mem/index.cp`](file:///data/dev/cp/lib/mem/index.cp)：`cap:mem` 统一导出入口。
- [`cap/test/test_mem_bun.cp`](file:///data/dev/cp/cap/test/test_mem_bun.cp):
  - 新增覆盖 `lib/mem/` 全组件的 5 大端到端测试套件（5/5 PASS）。
- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)（及 `cpc_dev/src/codegen/expr.rs` 镜像同步）:
  - 为 `cap:test` 的 `test("...", () => { ... })` 用例自动绑定栈上 `_test_arena` 并注入 `defer _test_arena.deinit()`，支持在测试闭包内直接使用 `new` 实例化类对象。
- [`lib/http/_picohttp.cp`](file:///data/dev/cp/lib/http/_picohttp.cp) & [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp):
  - 新增 `lib/http/_picohttp.cp`：吸收 Bun `picohttp_sys/picohttpparser.cp` 与 `picohttp/picohttp.cp`，提供 `@c { ... }` `phr_*` C-FFI 声明、C-ABI 结构体 `phr_header` / `phr_chunked_decoder` 及零拷贝 HTTP/1.x 报文头解析器 `PicoHttpParser`（`parseRequest`, `parseResponse`, `findHeader`），并在 `lib/http/index.cp` 统一导出。
- [`cap/test/test_picohttp.cp`](file:///data/dev/cp/cap/test/test_picohttp.cp):
  - 新增 `PicoHttpParser` 与 C-ABI 布局验证测试套件（3/3 PASS）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) & [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 完善 `Type::String` 与不可变借用切片 `[]const u8`（`Type::BorrowType { inner: Array(U8), is_mut: false }`）及可变切片 `[]T` -> `[]const T` 的类型协变；
  - 修复 `catch <Lit>` 表达式代码生成，保留字面量原生无类型常量语义以兼容 `u16`/`u32` 无符号目标。
- [`lib/string/`](file:///data/dev/cp/lib/string/) & [`lib/encoding/index.cp`](file:///data/dev/cp/lib/encoding/index.cp):
  - 新增 [`lib/string/simd_scan.cp`](file:///data/dev/cp/lib/string/simd_scan.cp)：16 字节 `@Vector(16, u8)` 高速扫描器 `SimdScanner`；
  - 新增 [`lib/string/utf.cp`](file:///data/dev/cp/lib/string/utf.cp)：`Utf8`、`Utf16`、`DecodeResult` 与 `UnicodeWidth`；
  - 新增 [`lib/string/builder.cp`](file:///data/dev/cp/lib/string/builder.cp)：`HashedString`、15 字节栈内联 `SmolStr` 与两阶段零重分配 `StringBuilder`；
  - 新增 [`lib/string/escape.cp`](file:///data/dev/cp/lib/string/escape.cp)：`StringEscape`（HTML 实体与 RegExp 元字符转义）；
  - 新增 [`lib/string/index.cp`](file:///data/dev/cp/lib/string/index.cp) 并扩展 `lib/encoding/index.cp`。
- [`cap/test/test_string_unicode.cp`](file:///data/dev/cp/cap/test/test_string_unicode.cp):
  - 新增覆盖 SIMD 扫描、UTF-8/16 编解码、Unicode/ANSI 宽度、`SmolStr`/`StringBuilder` 与转义的 5 大测试套件（5/5 PASS）。
- [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs)（及 `cpc_dev/src/codegen/types.rs` 镜像同步）:
  - 修复 `is_class_field` 在局部变量或参数与字段同名时的遮蔽判定（结单 `task-20261114-field-shadow-readonly-e0201`）。
- [`lib/dns/`](file:///data/dev/cp/lib/dns/):
  - 新增 [`lib/dns/_cares.cp`](file:///data/dev/cp/lib/dns/_cares.cp)：c-ares C-FFI 绑定、`NSClass`、`NSType` 与 `CaresStatus`；
  - 新增 [`lib/dns/packet.cp`](file:///data/dev/cp/lib/dns/packet.cp)：RFC 1035 12 字节 `DnsHeader`、`DnsRCode`、`DecodedDomain` 与支持 `0xC0` 指针压缩的 `DnsWireCodec`；
  - 新增 [`lib/dns/cache.cp`](file:///data/dev/cp/lib/dns/cache.cp)：`DnsFamily`、`DnsBackend`、`ResolveOptions`、`DnsCacheEntry` 与定容 TTL 缓存 `DnsCache<const CAP: usize>`；
  - 新增 [`lib/dns/index.cp`](file:///data/dev/cp/lib/dns/index.cp)：`cap:dns` 对外统一出口。
- [`cap/test/test_dns.cp`](file:///data/dev/cp/cap/test/test_dns.cp):
  - 新增覆盖 c-ares 类型、RFC 1035 报文头、Wire 域名编解码与指针压缩、TTL 缓存的 4 大测试套件（4/4 PASS）。
- [`lib/compress/`](file:///data/dev/cp/lib/compress/):
  - 新增 [`lib/compress/_codecs.cp`](file:///data/dev/cp/lib/compress/_codecs.cp)：Zlib / Brotli / Zstd C-FFI 声明、`ZlibFlush`、`ZlibStatus`、`CompressionFormat` 与 `CompressBoundEstimator`；
  - 新增 [`lib/compress/checksum.cp`](file:///data/dev/cp/lib/compress/checksum.cp)：IEEE 802.3 `Crc32` 与 RFC 1950 `Adler32` 校验器；
  - 新增 [`lib/compress/gzip.cp`](file:///data/dev/cp/lib/compress/gzip.cp)：RFC 1952 `GzipHeader` 帧头/帧尾编解码与 `RleCodec` 字节压缩器；
  - 新增 [`lib/compress/index.cp`](file:///data/dev/cp/lib/compress/index.cp)：`cap:compress` 对外统一出口。
- [`cap/test/test_compress.cp`](file:///data/dev/cp/cap/test/test_compress.cp):
  - 新增覆盖压缩协商、CRC32/Adler32 标准向量、Gzip 帧头/帧尾与 RleCodec 的 4 大测试套件（4/4 PASS）。
- [`lib/sourcemap/`](file:///data/dev/cp/lib/sourcemap/):
  - 新增 [`lib/sourcemap/vlq.cp`](file:///data/dev/cp/lib/sourcemap/vlq.cp)：7 字节栈内联 Base64 `VLQ` 与 `VLQResult` 编解码器；
  - 新增 [`lib/sourcemap/line_offset.cp`](file:///data/dev/cp/lib/sourcemap/line_offset.cp)：`SourceLocation` 与二分行偏移查找表 `LineOffsetTable<const MAX_LINES: usize>`；
  - 新增 [`lib/sourcemap/mapping.cp`](file:///data/dev/cp/lib/sourcemap/mapping.cp)：`SourceMapping` 与 SourceMap v3 映射表解析器 `ParsedSourceMap<const CAP: usize>`；
  - 新增 [`lib/sourcemap/index.cp`](file:///data/dev/cp/lib/sourcemap/index.cp)：`cap:sourcemap` 对外统一出口。
- [`cap/test/test_sourcemap.cp`](file:///data/dev/cp/cap/test/test_sourcemap.cp):
  - 新增覆盖 Base64 VLQ、二分行偏移表与 SourceMap v3 映射反查的 3 大测试套件（3/3 PASS）。
- [`lib/s3/`](file:///data/dev/cp/lib/s3/):
  - 新增 [`lib/s3/types.cp`](file:///data/dev/cp/lib/s3/types.cp)：`S3Acl`、`S3StorageClass`、`S3SignErrorCode` 与 `S3Error`；
  - 新增 [`lib/s3/credentials.cp`](file:///data/dev/cp/lib/s3/credentials.cp)：`S3Credentials` 凭证校验与 Virtual-Hosted / Path-Style 端点构造；
  - 新增 [`lib/s3/signing.cp`](file:///data/dev/cp/lib/s3/signing.cp)：AWS Signature Version 4 (`AWS4-HMAC-SHA256`) 四级密钥派生与请求签名器 `SigV4Signer`；
  - 新增 [`lib/s3/index.cp`](file:///data/dev/cp/lib/s3/index.cp)：`cap:s3` 对外统一出口。
- [`cap/test/test_s3.cp`](file:///data/dev/cp/cap/test/test_s3.cp):
  - 新增覆盖 S3 ACL/StorageClass、凭证 URL 格式化与 AWS SigV4 官方测试向量的 3 大测试套件（3/3 PASS）。

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs), [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs), [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs), [`cpc/src/codegen/`](file:///data/dev/cp/cpc/src/codegen/)（及 `cpc_dev/src/*` 镜像同步）:
  - 结单 `task-20261114-enum-variant-name-and-top-string-const`（枚举显式 `name` 变体防冲突与顶层无类型字符串常量输出）；
  - 实装正交参数体系（接收者省略 `_` / `*_` / `move _`，具名参数 `v: T` / `v: *T` 编译期自动推导值拷贝或可变借用 / 前置 `move v: T`、`own v: *T`）；
  - 实装 `cpc compile`、`cpc check`、`cpc check --json` 单次全文件多错误聚合诊断输出；
  - 实装免 `import` 一等公民内置 `JSON`（`JSON.stringify`、`JSON.parse<T>`、`JSON.parse`）及 `sema.rs` 全局预注册 `std`/`builtin`；
  - 新增 `--nest`（`-n`）命令行标志与 `@is_nest` 编译期常量（兼容 `--beam` / `@is_beam`）；
  - 支持 `@c { ... }` / `pub @c { ... }` 内部免 `pub extern` 极简声明、`enum` 方法默认 `pub`、`= []` 空切片字面量与类 `_create` 默认字段值初始化、`* = expr;` 返回值丢弃及全场景省略 `: void`；
  - 新增 `E0103`/`E0104`/`E0115`/`E0116`/`E0118`/`E0119`/`E0120`/`E0121`/`E0122`/`E0123`/`E0125` 智能拦截与修复建议（保留 `while (cond) : (step)` 与 `@xxx` 宏）。
- [`lib/crypto/_boringssl.cp`](file:///data/dev/cp/lib/crypto/_boringssl.cp), [`lib/crypto/tls.cp`](file:///data/dev/cp/lib/crypto/tls.cp) & [`lib/crypto/index.cp`](file:///data/dev/cp/lib/crypto/index.cp):
  - 新增 BoringSSL 底垫片引用与 X.509 SAN / DNS 通配符主机名校验库 `tls.cp`，并在 `lib/crypto/index.cp` 统一导出；
  - 完成 `lib/` 下全部 144 个 `.cp` 标准库文件的正交语法净化（144/144 零错误通过 `cpc check`）。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) & [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 同步更新双轨语法规范手册，写入正交参数修饰符、内置 `JSON`、`@c` 块、`@is_nest` 及 Top 16 绝对禁手清单。
- [`cap/test/test_syntax_orthogonality.cp`](file:///data/dev/cp/cap/test/test_syntax_orthogonality.cp):
  - 新增覆盖正交语法、`v: *T` 自动推导、内置 `JSON`、预注册 `std` 与 BoringSSL TLS 证书校验的 4 大端到端测试套件（4/4 PASS）。

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs), [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs), [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 新增 `Token::PlusPlus` (`++`) 与 `BinaryOp::Concat` 支持（结单 `task-20261114-concat-plusplus-operator`）；
  - 为 prelude 辅助函数形参与局部变量统一添加 `__cp_` 前缀防止同名遮蔽（结单 `task-20261114-prelude-ptr-shadow`）；
  - 升级内置 `JSON.stringify`、`JSON.parse<T>`、`JSON.parse` 采用 `std.heap.smp_allocator` 与 `.allocate = .alloc_if_needed` 零拷贝借用；
  - 升级内置 `RegExp`（`emit_regex_prelude`）：内联 16 字节 `@Vector(16, u8)` SIMD 字面量前缀快跳与 `[2048]u8` 栈缓冲区零堆分配快速路径，修复正则字面量反斜杠转义；
  - 删除冗余空壳文件 `lib/regex.cp`；
  - 扩充 [`cap/test/test_syntax_orthogonality.cp`](file:///data/dev/cp/cap/test/test_syntax_orthogonality.cp) 第 5 项 `RegExp` 测试套件（5/5 PASS）。
- [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs), [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh)（及 `cpc_dev/src/*` 镜像同步）:
  - 新增 `cpc check --brief` (`-b`)、`cpc check --json --brief`、`cpc check --max <N>`、`cpc check --fix` 与 `cpc fix` 子命令；
  - 新增 `mark_cascades` 连锁错误识别与 `cascade_of` 标注/折叠，以及 `errors`、`root_errors`、`cascades_hidden`、`fixable` 汇总计数；
  - 为 `E0101`..`E0104`, `E0115`, `E0116`, `E0118`, `E0119`..`E0123`, `E0126` 填充 `{span_start, span_end, replacement}` 机械修复 `edits` 并实装 `apply_fixes` 多轮自动修复；
  - 新增 `if (let v = opt)` / `while (let v = opt)` 解析恢复与 `E0126` 自动转 `:=` 诊断；
  - 修复 `.{ key: val }`、`ErrorSet!void` 与 `error{ ... }` 解析，消除假性连锁报错。

## [2026-11-15] - 核心里程碑：`bun_agy` 4 项编译器急修结单、标准库 4 大目录瘦身合并与 10 大新模块 + 4 大模块增强全面落地

### 编译器协同热修 (Compiler Hotfixes & Resolver)

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) & [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 拦截 `let/const x: auto = undefined;` 组合并输出三段式诊断 `E0212: cannot infer type 'auto' from 'undefined'; explicit concrete type is required`（结单 `task-20261115-ban-auto-undefined-semantic`）。
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs) & [`cpc_dev/src/codegen/mod.rs`](file:///data/dev/cp/cpc_dev/src/codegen/mod.rs):
  - 新增 `lower_extra_zig_member()`：对 `struct`/`enum` 内部 `extra_zig` 常量/变量声明穿透执行完整表达式 lowering（如 `.{ had_flag: false }` -> `.{ .had_flag = false }`），结单 `task-20261115-nested-const-codegen-lowering`。
- [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs) & [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs):
  - 将 `Expr::Zig` 参数引用检测升级为精确词边界匹配 `code_contains_word`，补全非实例方法 `self` 形参、`comptime` 形参的 `_ = param;` 注入以及未引用局部 `const` 的 `_ = &var;` 注入（结单 `task-20261115-unused-param-codegen-silence` 与 `task-20261115-cpc-fix-e0120-e0122`）。
- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) & [`cpc_dev/src/resolver.rs`](file:///data/dev/cp/cpc_dev/src/resolver.rs):
  - 实现 `cap:col`/`cap:collections`、`cap:event`/`cap:event_loop`/`cap:events`、`cap:conf`/`cap:dotenv`/`cap:ini`、`cap:pack`/`cap:compress` 双向兼容模块路径映射。

### 标准库目录瘦身合并 (Phase A: Consolidated Modules)

- [`lib/col/`](file:///data/dev/cp/lib/col/) (`cap:col`)：由 `lib/collections/` 重命名瘦身，新增吸收自 Bun 的 256 位静态位集 [`lib/col/bit_set.cp`](file:///data/dev/cp/lib/col/bit_set.cp) (`BitSet256`)。
- [`lib/event/`](file:///data/dev/cp/lib/event/) (`cap:event`)：合并原 `lib/event_loop/` 与 `lib/events.cp`（迁入 [`lib/event/emitter.cp`](file:///data/dev/cp/lib/event/emitter.cp)）。
- [`lib/conf/`](file:///data/dev/cp/lib/conf/) (`cap:conf`)：合并原 `lib/dotenv/` 与 `lib/ini/`，并吸收 Bun 的 [`lib/conf/toml.cp`](file:///data/dev/cp/lib/conf/toml.cp)、[`lib/conf/yaml.cp`](file:///data/dev/cp/lib/conf/yaml.cp)、[`lib/conf/json5.cp`](file:///data/dev/cp/lib/conf/json5.cp)。
- [`lib/pack/`](file:///data/dev/cp/lib/pack/) (`cap:pack`)：由 `lib/compress/` 升级，保留流式压缩并吸收 Bun 的 [`lib/pack/_archive.cp`](file:///data/dev/cp/lib/pack/_archive.cp) 与 [`lib/pack/tar.cp`](file:///data/dev/cp/lib/pack/tar.cp)（POSIX USTAR 归档打包与零拷贝 `TarReader`）。

### 新建 10 大简洁标准库模块 (Phase B: New Concise Modules)

- [`lib/redis/`](file:///data/dev/cp/lib/redis/) (`cap:redis`)：`resp.cp`、`index.cp`（RESP2/RESP3 零拷贝协议帧解析与编码器）。
- [`lib/cli/`](file:///data/dev/cp/lib/cli/) (`cap:cli`)：`args.cp`、`tty.cp`、`index.cp`（命令行标志解析、ANSI 转义剥离与终端进度条 `ProgressBar`）。
- [`lib/watch/`](file:///data/dev/cp/lib/watch/) (`cap:watch`)：`watcher.cp`、`index.cp`（跨平台文件变更监控掩码与路径表 `FileWatcher`）。
- [`lib/patch/`](file:///data/dev/cp/lib/patch/) (`cap:patch`)：`diff.cp`、`index.cp`（Unified Diff 解析与 Hunk 渲染应用）。
- [`lib/bench/`](file:///data/dev/cp/lib/bench/) (`cap:bench`)：`timer.cp`、`index.cp`（纳秒级硬件单调计时器 `Stopwatch` 与分位数统计 `BenchStats`）。
- [`lib/elf/`](file:///data/dev/cp/lib/elf/) (`cap:elf`)：`format.cp`、`index.cp`（ELF64 / Mach-O / PE 二进制格式嗅探与头部解析）。
- [`lib/md/`](file:///data/dev/cp/lib/md/) (`cap:md`)：`markdown.cp`、`index.cp`（CommonMark/GFM 解析与 HTML/ANSI 渲染）。
- [`lib/html/`](file:///data/dev/cp/lib/html/) (`cap:html`)：`selector.cp`、`index.cp`（CSS 选择器解析与流式 `HtmlRewriter`）。
- [`lib/ffi/`](file:///data/dev/cp/lib/ffi/) (`cap:ffi`)：`_tcc.cp`、`dlopen.cp`、`index.cp`（动态库加载 `DynLib`、FFI 签名布局与 TinyCC 声明）。
- [`lib/sql/`](file:///data/dev/cp/lib/sql/) (`cap:sql`)：`protocol.cp`、`index.cp`（PostgreSQL v3 Wire 帧、MySQL 包头编解码与预编译语句绑定）。

### 现有 4 大模块能力扩充 (Phase C: Module Extensions)

- [`lib/http/`](file:///data/dev/cp/lib/http/)：新增 [`cookie.cp`](file:///data/dev/cp/lib/http/cookie.cp) (`CookieMap`) 与 [`form_data.cp`](file:///data/dev/cp/lib/http/form_data.cp) (`FormData`)。
- [`lib/net/`](file:///data/dev/cp/lib/net/)：新增 [`udp.cp`](file:///data/dev/cp/lib/net/udp.cp) (`UdpSocket`, `UdpPacket`) 与 `SocketAddress.parseIpv4`。
- [`lib/mem/`](file:///data/dev/cp/lib/mem/)：扩充 [`cow.cp`](file:///data/dev/cp/lib/mem/cow.cp) (`CowSlice`) 与 [`tagged_ptr.cp`](file:///data/dev/cp/lib/mem/tagged_ptr.cp) (`TaggedPtr`)。
- [`lib/string/`](file:///data/dev/cp/lib/string/)：新增 [`grapheme.cp`](file:///data/dev/cp/lib/string/grapheme.cp) (`Utf8Grapheme` UTF-8 校验与 UAX #29 字素簇计数)。
- 测试套件：新增 [`cap/test/test_stdlib_phase_b1.cp`](file:///data/dev/cp/cap/test/test_stdlib_phase_b1.cp)（8/8 PASS）与 [`cap/test/test_stdlib_phase_b2_c.cp`](file:///data/dev/cp/cap/test/test_stdlib_phase_b2_c.cp)（9/9 PASS）。

### 2026-11-15（续） 双子智能体并行深化 12 个标准库子模块与第 6 项 `bun_agy` 急修结单

- **编译器热修 (`cpc/src/` & `cpc_dev/src/`)**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：修复 `generate_zig_or_struct` 中泛型函数返回匿名 `struct`（`return struct { ... }`）内省略 `_` 的只读实例方法被误判为 `static` 的问题（结单 `task-20261115-generic-return-struct-readonly-receiver`）。
- **双子智能体并行深化标准库模块 (`lib/`)**：
  - [`lib/sql/postgres.cp`](file:///data/dev/cp/lib/sql/postgres.cp) & [`lib/sql/mysql.cp`](file:///data/dev/cp/lib/sql/mysql.cp)：PostgreSQL v3 认证挑战/数据行/命令完成帧与 MySQL `lenenc_int`/能力位图/OK/ERR 包解析；
  - [`lib/redis/command.cp`](file:///data/dev/cp/lib/redis/command.cp)：RESP2 命令编码器 `RedisCommand` 与批量管道 `RedisPipeline`；
  - [`lib/http/h2/hpack.cp`](file:///data/dev/cp/lib/http/h2/hpack.cp)：RFC 7541 HTTP/2 HPACK 整数前缀、静态表与首部字段编解码器 `HpackCodec`；
  - [`lib/col/object_pool.cp`](file:///data/dev/cp/lib/col/object_pool.cp)：零堆碎片侵入式空闲链表对象池 `ObjectPool<T, CAP>`；
  - [`lib/pack/zip.cp`](file:///data/dev/cp/lib/pack/zip.cp)：PKZIP Local/Central/EOCD 解析与零拷贝 `ZipReader`；
  - [`lib/md/table.cp`](file:///data/dev/cp/lib/md/table.cp)：GFM 表格对齐与 HTML `<table>` 渲染器 `MdTable`；
  - [`lib/html/escape.cp`](file:///data/dev/cp/lib/html/escape.cp)：零拷贝快跳 HTML 实体转义与反转义器 `HtmlEscape`；
  - [`lib/cli/table.cp`](file:///data/dev/cp/lib/cli/table.cp)：终端 ASCII 表格格式化渲染器 `CliTable`；
  - [`lib/watch/filter.cp`](file:///data/dev/cp/lib/watch/filter.cp)：文件监控忽略过滤 `WatchFilter` 与防抖器 `DebounceTracker`；
  - [`lib/patch/myers.cp`](file:///data/dev/cp/lib/patch/myers.cp)：行级差异比对与 Unified Diff 生成器 `LineDiffer`；
  - [`lib/bench/histogram.cp`](file:///data/dev/cp/lib/bench/histogram.cp)：8 桶对数纳秒延迟直方图 `LatencyHistogram`。
- **新增端到端测试套件**：
  - [`cap/test/test_sql_redis_hpack.cp`](file:///data/dev/cp/cap/test/test_sql_redis_hpack.cp)（4/4 PASS）；
  - [`cap/test/test_col_pack_md_html.cp`](file:///data/dev/cp/cap/test/test_col_pack_md_html.cp)（4/4 PASS）；
  - [`cap/test/test_cli_watch_patch_bench.cp`](file:///data/dev/cp/cap/test/test_cli_watch_patch_bench.cp)（4/4 PASS）。

### 2026-11-15（续 2） 第二波双子智能体并行深化 8 个标准库子模块

- **新增与更新标准库文件 (`lib/`)**：
  - [`lib/elf/macho_pe.cp`](file:///data/dev/cp/lib/elf/macho_pe.cp) & [`lib/elf/index.cp`](file:///data/dev/cp/lib/elf/index.cp)：Mach-O 64 位头部与 PE/COFF 头部解析器（`MachHeader64`, `MachoInspector`, `PeCoffHeader`, `PeInspector`）；
  - [`lib/ffi/layout.cp`](file:///data/dev/cp/lib/ffi/layout.cp) & [`lib/ffi/index.cp`](file:///data/dev/cp/lib/ffi/index.cp)：C-ABI 结构体字段偏移与自然对齐计算器（`CStructLayout`, `CStructField`）；
  - [`lib/net/proxy.cp`](file:///data/dev/cp/lib/net/proxy.cp) & [`lib/net/index.cp`](file:///data/dev/cp/lib/net/index.cp)：HTTP `CONNECT` 代理隧道与 SOCKS5 握手/连接帧编解码器（`HttpConnectTunnel`, `Socks5Codec`）；
  - [`lib/conf/getter.cp`](file:///data/dev/cp/lib/conf/getter.cp) & [`lib/conf/index.cp`](file:///data/dev/cp/lib/conf/index.cp)：配置值强类型解析器 `ConfValueParser`（布尔、端口、时长 `ms/s/m/h`、容量 `b/kb/mb/gb`）；
  - [`lib/encoding/hex.cp`](file:///data/dev/cp/lib/encoding/hex.cp) & [`lib/encoding/index.cp`](file:///data/dev/cp/lib/encoding/index.cp)：零堆分配查表十六进制编解码器 `Hex`；
  - [`lib/url/percent.cp`](file:///data/dev/cp/lib/url/percent.cp)、[`lib/url/query.cp`](file:///data/dev/cp/lib/url/query.cp) & [`lib/url/index.cp`](file:///data/dev/cp/lib/url/index.cp)：RFC 3986 URI 百分号编解码器 `PercentEncoding`；
  - [`lib/string/case.cp`](file:///data/dev/cp/lib/string/case.cp) & [`lib/string/index.cp`](file:///data/dev/cp/lib/string/index.cp)：标识符命名风格转换器 `CaseConverter`（`toSnakeCase`, `toKebabCase`, `toCamelCase`, `toPascalCase`, `toScreamingSnakeCase`）；
  - [`lib/router/pattern.cp`](file:///data/dev/cp/lib/router/pattern.cp)、[`lib/router/radix.cp`](file:///data/dev/cp/lib/router/radix.cp) & [`lib/router/index.cp`](file:///data/dev/cp/lib/router/index.cp)：路由模板参数提取与反向 URL 格式化器 `RouteFormatter`。
- **新增端到端测试套件**：
  - [`cap/test/test_elf_ffi_net_conf.cp`](file:///data/dev/cp/cap/test/test_elf_ffi_net_conf.cp)（4/4 PASS）；
  - [`cap/test/test_hex_url_case_router.cp`](file:///data/dev/cp/cap/test/test_hex_url_case_router.cp)（4/4 PASS）。

### 2026-11-15（续 5） 顶层原生类型遮蔽修复、双子智能体拓扑落地与任务全面汇报

- **编译器前端与 Codegen 修复 (`cpc/src/codegen/mod.rs`)**：
  - 在顶层变量遍历输出时增加 Zig 原生基本类型过滤（跳过 `u8`, `i8`, `u16`, `i16`, `u32`, `i32`, `u64`, `i64`, `usize`, `isize`, `f32`, `f64`, `bool`, `void`, `anyopaque`），彻底消除 `const u8 = undefined;` 导致的 `name shadows primitive 'u8'` 编译阻断；
  - 调用 `scripts/sync_cpc.sh` 原子构建并秒级部署至全局 `cap/bin/cpc` 与 `target/release/cpc`。
- **架构拓扑规范确立**：
  - 明确划分 `Hotfix Engineer`（基于 `cpc/` 直修发布，优先释放下游工单）与 `Core Compiler Researcher`（基于安全沙盒与 `cpc_dev` 专心自研攻坚）；
  - 常驻守护进程 `task-32193` 全天候监控下游工单信箱，确保通信零遗漏。

- **下游 CapSurf 紧急热修（3 项 100% 验证结单）**：
  - `task-capsurf-20261115-struct-associated-fn-lowering`：修复 `cpc/src/codegen/actor.rs` 关联方法降级逻辑，使结构体关联方法正确生成为静态函数；
  - `task-capsurf-20261115-enum-method-receiver`：修复 `cpc/src/codegen/mod.rs` 与 `types.rs` 中 enum 声明注册与方法接收者类型推断，避免类型退化为 `*anyopaque`；
  - `task-capsurf-20261115-instance-method-with-params`：修复带参数实例方法 receiver 判定顺序，使 `Point.add(x)` 正确保留 `self: Point`。
- **CSS Parser（7,333 行）核心自研攻坚突破**：
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：
    - 修复类型参数解析中跨 Token 空格保留机制（消灭 `[]constu8` 词法粘连）；
    - 修复 `parse_return_stmt` 支持 `else => return,` 无参带逗号 return；
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：
    - 在 `lower_struct_body` 中新增 `get_receiver_param_info`，全面支持 `*_` / `*this` / `*self` 以及带参数实例方法自动注入 `self: @This(), `；
    - 排除 `static fn` 的 receiver 注入；
    - 将 7,333 行 `css_parser.cp` 的 Zig AST 检查报错从 632+ 彻底消灭至仅剩 1 个借用冲突。
- **架构拓扑与多智能体分工**：
  - 启动后台常驻通信守护进程 `IPC_DAEMON`；
  - 启动热修专属子智能体（基于 `cpc` 目录极速结单下游工单）；
  - 启动自研攻关专属子智能体（基于 `cpc_dev` 目录彻底突破 CSS Parser 终局借用冲突并收敛标准库）。

- **多 AGY 隔离通信协议固化 (`GEMINI.md`)**：
  - [`GEMINI.md`](file:///data/dev/cp/GEMINI.md)：更新多工作区拓扑（新增 `capsurf_agy` `/data/dev/capsurf`）与第 七 章 4 条多 AGY 通信规则（双下游识别 `task-capsurf-*`、多 ID 批量 `resolve` 精准路由、`broadcast` 语法更新双端广播、`watch_task.py --status pending --creator all --timeout 0` 常驻监听）。
- **编译器热修与 CSS 语法自愈 (`cpc/src/` & `cpc_dev/src/`)**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：`has_top_string_decl` 检测 `ModuleItem::Import` 中的 `"string"` / `@"string"`，抑制顶部 `const string = []const u8;` 冲突（结单 `task-20261115-string-import-collision`）；
  - [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs)：新增字符字面量 `\xNN` / `\u{NNNN}` 转义与 `\\` 多行字符串词法支持；
  - [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs)、[`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)、[`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)、[`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：支持匿名嵌套 `struct { ... }` / `struct? { ... }`、内联可选泛型 `SmallList?(T, N)`、无载荷联合体变体、`ArrayList(T){}` / `@This(){}` 构造、`[]const u8` 类型实参、`Expr::If` 条件表达式 Walrus 解包捕获 `if (p := expr)`、`Decl::Class` 内 `extra_zig` 与 `nested_classes` 符号注册；
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：注册 `"css" => "css".to_string()` 标准库通道映射。
- **新增 `cap:css` 标准库 (`lib/css/` 共 99 个 `.cp` 文件)**：
  - 顶层门面 API（5 个）：[`lib/css/index.cp`](file:///data/dev/cp/lib/css/index.cp)、[`lib/css/color.cp`](file:///data/dev/cp/lib/css/color.cp)、[`lib/css/length.cp`](file:///data/dev/cp/lib/css/length.cp)、[`lib/css/selector.cp`](file:///data/dev/cp/lib/css/selector.cp)、[`lib/css/stylesheet.cp`](file:///data/dev/cp/lib/css/stylesheet.cp)；
  - 完整吸收 `/data/dev/bun/src/css/` 全部 94 个 `.cp` 模块文件（根目录 15 个引擎文件 + `selectors/*.cp` 3 个 + `rules/*.cp` 22 个 + `values/*.cp` 22 个 + `properties/*.cp` 32 个），99/99 文件 100% 通过 `cpc check --brief`。
- **新增端到端测试套件**：
  - [`cap/test/test_css.cp`](file:///data/dev/cp/cap/test/test_css.cp)（5/5 100% PASS）。

- **Bun CSS 模块 95 源码文件端到端转译突破（通过率提升至 80.0% / 76 个文件 PASS）**：
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：
    - 在对象字面量解析（`parse_object_fields`）中支持前导 `Token::Dot`，全面兼容 `{ prop: "...", .ty = Value }`；
    - 在类体（`parse_class`）成员识别中放行 `comptime { ... }` 作为 `extra_zig` 原生直通 Zig 结构体；
    - 针对 `struct? { ... }` / `union? { ... }` 自动包装为 `Type::Optional`，解决可选类型匹配；
    - 宽容支持 `?T` 前缀可选类型语法（解除 E0115 硬阻断）；
    - 支持 `@This(){ ... }` 实例化表达式。
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：
    - 在 `can_coerce` 中新增 `BorrowType` 到 `BorrowType` 双向兼容分支，支持解引用多级指针赋值。
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：
    - 修正 `get_func_receiver_mode`：优先判定 `func.is_static`，且过滤 `_: void` 等占位参数作为伪 receiver，杜绝 `self` 变量作用域遮蔽（`angle.cp`, `time.cp` 完全通过）。
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：
    - `has_top_string_decl` 检查 `Import` 声明避免 `duplicate struct member name 'string'`（`values.cp` 通过）；
    - 废除危险的 `: {` 匹配规则，杜绝 `blk: {`、`brk: {` 标签块被误替换为 `.{`；
    - 在 `lower_struct_body` 中将 `static fn` 去关键字化为 `pub fn`（`calc.cp` 通过）；
    - 为省略返回类型的函数精准补充 `void`（`image.cp` 等通过）；
    - 新增 `lower_inline_struct_lit` 自动将单行 `.{ key: val }` 转为 `.{ .key = val }`。

### 2026-11-16

- **CSS 全量 95 源码模块纯正语法清洗与 100% 满贯达成 (95/95 PASS)**：
  - 严格捍卫 CP 纯正语法规范，在 `.cp` 源码端对标原始 `xxx_.zig` 消除全部非法语法；
  - **Values 模块群全面规范化**（9/9 PASS）：
    - [`values/color.cp`](file:///data/dev/bun/src/css/values/color.cp)、[`values/length.cp`](file:///data/dev/bun/src/css/values/length.cp)、[`values/syntax.cp`](file:///data/dev/bun/src/css/values/syntax.cp)：将所有 `pub union(enum)` 全量改造为纯正 CP Tagged Union `pub enum Name { ... }`；
    - [`values/length.cp`](file:///data/dev/bun/src/css/values/length.cp)：消除 Zig 风格 `const value, const unit = ...`，重构为标准元组解构 `let [value, unit] = ...`；具名结构体构造规范为 `TypeName{ field: val }`；
    - [`values/ident.cp`](file:///data/dev/bun/src/css/values/ident.cp)：重命名保留字 `ref` 为 `ref_val`，消除表达式层 `[]const u8` 为 `string`；
    - [`values/syntax.cp`](file:///data/dev/bun/src/css/values/syntax.cp)：拆分内联匿名结构体为 `pub struct RepeatedComponent`，规范状态机单点出口。
  - **Properties 模块群与 SmallList 全面规范化**（33/33 PASS）：
    - [`properties/align.cp`](file:///data/dev/bun/src/css/properties/align.cp) 等：匿名结构体字面量规范为 `.{ .result = ... }`，消除 switch 分支歧义；
    - [`small_list.cp`](file:///data/dev/bun/src/css/small_list.cp)：恢复 `pfi.slice()` 契约；
    - [`properties/animation.cp`](file:///data/dev/bun/src/css/properties/animation.cp)：4 处联合枚举重构为 `pub enum`。
  - **CSS Parser 核心巨构与媒体查询**（3/3 PASS）：
    - [`css_parser.cp`](file:///data/dev/bun/src/css/css_parser.cp)：7,400 行超级解析器核心消灭第 4467 行残留 Zig `if (...) |...|` 解包语法，修复 Tokenizer 闭合断点；
    - [`css_parser.d.cp`](file:///data/dev/bun/src/css/css_parser.d.cp)：废除 TS 箭头语法，重构为标准函数指针类型 `*const fn (*Parser) Result(T)`。
- **`lib/css/` 标准库全量同步**：
  - 完整同步 Bun CSS 成果至 [`lib/css/`](file:///data/dev/cp/lib/css/)，共计 100 个 `.cp` 文件（包含顶层门面 API），100/100 100% 通过 `cpc --lib` 与 `zig ast-check`。
- **两大重点资产群全量吸收与极简标准库落地 (100% PASS)**：
  - **资产群一（基础高并发与容器库）**：
    - [`lib/col/hash_map.cp`](file:///data/dev/cp/lib/col/hash_map.cp)：新增零堆分配 Robin-Hood 线性探查紧凑哈希表 `StaticHashMap<K, V, CAP>`；
    - [`lib/col/bounded_array.cp`](file:///data/dev/cp/lib/col/bounded_array.cp)、[`lib/col/bit_set.cp`](file:///data/dev/cp/lib/col/bit_set.cp)、[`lib/col/hive_array.cp`](file:///data/dev/cp/lib/col/hive_array.cp)、[`lib/col/object_pool.cp`](file:///data/dev/cp/lib/col/object_pool.cp)：规范方法 receiver，消除未使用参数警告与伪 receiver；
    - [`lib/string/smol.cp`](file:///data/dev/cp/lib/string/smol.cp)：新增 15 字节纯栈 SSO 小字符串 `SmolStr` 与堆回退；
    - [`lib/string/hashed.cp`](file:///data/dev/cp/lib/string/hashed.cp)：新增 O(1) 预哈希字符串 `HashedString`；
    - [`lib/string/path.cp`](file:///data/dev/cp/lib/string/path.cp)：新增零分配路径视图 `PathString`；
    - [`lib/string/joiner.cp`](file:///data/dev/cp/lib/string/joiner.cp)：新增高效字符串拼接器 `StringJoiner`；
    - [`lib/sync/pool.cp`](file:///data/dev/cp/lib/sync/pool.cp)：新增线程安全对象包裹器 `Guarded<T>` 与工作线程池队列 `WorkPool`；
    - [`lib/sync/futex.cp`](file:///data/dev/cp/lib/sync/futex.cp)、[`lib/sync/condition.cp`](file:///data/dev/cp/lib/sync/condition.cp)、[`lib/sync/wait_group.cp`](file:///data/dev/cp/lib/sync/wait_group.cp)、[`lib/sync/unbounded_queue.cp`](file:///data/dev/cp/lib/sync/unbounded_queue.cp)：消灭嵌套结构体 Zig 风格分号语法，规范为 CP 纯正语法与借用契约；
    - [`lib/event/task.cp`](file:///data/dev/cp/lib/event/task.cp)：新增动态任务执行器 `AnyTask` 与堆管理任务 `ManagedTask`；
    - 门面 API 同步导出：[`lib/col/index.cp`](file:///data/dev/cp/lib/col/index.cp)、[`lib/string/index.cp`](file:///data/dev/cp/lib/string/index.cp)、[`lib/sync/index.cp`](file:///data/dev/cp/lib/sync/index.cp)、[`lib/event/index.cp`](file:///data/dev/cp/lib/event/index.cp)。
  - **资产群二（应用与网络协议核心库）**：
    - [`lib/sql/sqlite.cp`](file:///data/dev/cp/lib/sql/sqlite.cp)：实现原生 SQLite3 数据库引擎抽象 `SqliteDatabase` 与查询游标 `SqliteRows`；
    - [`lib/sql/index.cp`](file:///data/dev/cp/lib/sql/index.cp)：对外统一聚合导出 MySQL, PostgreSQL, SQLite3 数据库协议与驱动；
    - [`lib/http/server.cp`](file:///data/dev/cp/lib/http/server.cp)：修复 `Request` 符号导入；
    - [`lib/shell/`](file:///data/dev/cp/lib/shell/)（全新吸收）：实现原生轻量嵌入式 Shell 执行引擎与解释器：
      - [`lib/shell/lexer.cp`](file:///data/dev/cp/lib/shell/lexer.cp)：Shell 词法分析器，识别指令、管道、重定向、引号与环境变量；
      - [`lib/shell/parser.cp`](file:///data/dev/cp/lib/shell/parser.cp)：Shell 语法解析器与 AST 语句结构树构建；
      - [`lib/shell/exec.cp`](file:///data/dev/cp/lib/shell/exec.cp)：执行调度引擎，实现多级管道进程间通信与 I/O 重定向；
      - [`lib/shell/builtin.cp`](file:///data/dev/cp/lib/shell/builtin.cp)：内置指令实现（`echo`, `cd`, `pwd`, `which`, `export`, `exit`）；
      - [`lib/shell/index.cp`](file:///data/dev/cp/lib/shell/index.cp)：门面导出极简 `sh(cmd)` 与 `Shell.run(cmd)`。
- **编译器与工程配置增强**：
  - [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)：扫描 `f.default` 字段默认表达式与 `extra_zig` 内位移操作符；对 `@create_pending_task`、`@complete_task`、`@task_spawn`、`@task_join` 标记 `needs_actors = true`；
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：增加 `cap:sync` / `cap:threading` 及 `cap:shell` / `cap:sh` 标准库别名与重映射路径；
  - [`cap.json`](file:///data/dev/cp/cap.json)：根工程加入 `build.link_libs: ["sqlite3"]`；
  - 编译器代码直通同步至 `cpc_dev`。
- **新增综合测试套件**：
  - [`cap/test/test_bun_absorbed_assets.cp`](file:///data/dev/cp/cap/test/test_bun_absorbed_assets.cp)：涵盖 `cap:string`, `cap:sync`, `cap:event`, `cap:sql`, `cap:shell` 综合端到端测试（6/6 100% PASS）；
  - [`cap/test/test_collections_bun.cp`](file:///data/dev/cp/cap/test/test_collections_bun.cp)：6/6 100% PASS。
- **新增原生图像处理标准库 `cap:image` (兼容 `cap:png` / `cap:img`)**：
  - [`lib/image/png.cp`](file:///data/dev/cp/lib/image/png.cp)：
    - 实现工业级纯 CP 原生 PNG 截图编码器 `Png.encodeRgba`（W3C 标准 8 字节魔数签名、IHDR 块构造、扫描线 None 滤波、Zlib 原生 Deflate 压缩生成 IDAT、IEND 闭合与 IEEE 802.3 CRC32 校验和）；
    - 实现 PNG 全滤镜逆运算解码器 `Png.decode`（IHDR 尺寸与颜色类型提取、IDAT 聚合流式解压、支持 None, Sub, Up, Average, Paeth 5 种滤波逆还原为 RGBA8 像素切片）；
    - 实现面向对象 `PngImage` 与像素结构体 `RgbaColor`（支持 `.getPixel(x, y)`、`.setPixel(x, y, r, g, b, a)`、`.encode()`、`.save(path)`）；
    - 实现同步文件系统保存 `Png.save(path, rgba, w, h)` 与加载 `Png.load(path)`；
  - [`lib/image/codecs.cp`](file:///data/dev/cp/lib/image/codecs.cp)：
    - 实现二进制魔数快速探测 `Codecs.sniff`（支持 PNG, JPEG, WebP, BMP, GIF, TIFF, AVIF, HEIC）；
    - 实现标准 MIME 映射 `Codecs.mime` 与文件后缀提取 `Codecs.fromExtension`；
    - 实现 EXIF Orientation 方向解析 `Codecs.getOrientationTransform`；
  - [`lib/image/bmp.cp`](file:///data/dev/cp/lib/image/bmp.cp)：
    - 实现 Windows DIB / BMP 格式解码器 `Bmp.decode` 与 32-bit 编码器 `Bmp.encodeRgba`（支持 4 字节跨距行对齐与倒序 Bottom-Up 扫描转换）；
  - [`lib/image/quantize.cp`](file:///data/dev/cp/lib/image/quantize.cp)：
    - 实现 Median-Cut 颜色量化器 `Quantizer.quantize`（RGBA8 转 256 色紧凑调色板与像素索引矩阵）；
  - [`lib/image/index.cp`](file:///data/dev/cp/lib/image/index.cp)：
    - 对外统一导出 `cap:image`、`cap:png`、`cap:img` 协议；
- **编译器与工程构建增强**：
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：注册 `"image" | "img" | "png" => "image".to_string()`；
  - [`cap.json`](file:///data/dev/cp/cap.json)：引入 `"z"` 动态链接库支持原生硬件极速压缩；
  - 同步部署至 `cpc_dev`。
- **新增综合测试套件**：
  - [`cap/test/test_image_png.cp`](file:///data/dev/cp/cap/test/test_image_png.cp)：包含格式嗅探、RGBA8 截图编码、IHDR 签名校验、像素往返解码比对、文件保存与加载、BMP 编解码、颜色量化 6 大测试用例（**6/6 100% PASS**）。

---

### [2026-11-16] 编译器语义自愈升级、Zig 0.17 全套适配与 BEAM/集合类型深度解耦

- **编译器核心修改**：
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：
    - 解耦非 async 普通函数返回类型与 BEAM 模式，恢复强类型 `ret_ty` 返回签名；
    - 移除普通结构体函数在 BEAM 模式下无 return 时的错误原子发射；
    - 在状态机代码生成结束点（`emit_current_case` 闭合处）重置 `self.is_state_machine = false`，防止后续全局声明误用 `ctx.arena`；
    - 区分引用类（`ref_classes`）与值结构体（`value_classes`）的默认接收者推断；
    - 消除 Zig 0.17 `pointless discard of function parameter`；
  - [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)：
    - 修复 `Stmt::Return(None)`：仅在 `self.is_beam_mode && self.is_state_machine` 时发射 `beam.Beam.makeAtom(ctx.self.env.?, "ok")`，普通函数及非 actor 结构体方法恢复标准 `return;`；
  - [`cpc/src/codegen/pattern.rs`](file:///data/dev/cp/cpc/src/codegen/pattern.rs)：
    - 在 `generate_switch` 顶层增加 `is_result_disc` 拦截，判别式为错误联合类型时禁止进入 Zig 原生 switch，转入安全解包流；
    - 消除 Zig 0.17 `unused capture`，未引用的 arm 绑定自动生成 `else =>`；
    - 降解 `err.Variant` 为 Zig 原生 `error.Variant`；
    - 修复 `generate_err_value` 中 `Type::BoxDyn` 移动语义与变量识别；
  - [`cpc/src/codegen/borrow.rs`](file:///data/dev/cp/cpc/src/codegen/borrow.rs)：
    - 在 `is_zig_heap_ptr_type` 中补齐 `VecDeque`、`BinaryHeap`、`BTreeMap`、`BTreeSet`、`HashMap`、`HashSet` 及 `ref_classes` 堆指针识别，防止集合类型借用传递时发生双重取地址；
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：
    - 无参 `new Class()` 在 comptime 校验 init 参数个数；
    - 修正类型投射与成员借用的指针判定；
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：
    - 解除双指针及多级原始指针（如 `pp.*.age`）字段访问的硬编码拦截；
  - [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs)：
    - 落地 `E0130`（只读接收者自动插入 `*_`）、`E0133`（只读形参赋值双分支诊断建议）、`E0134`（关键字与保留字重命名及转义）；
- **任务协同与热修闭环**：
  - 结单 [`task-capsurf-20261116-beam-non-actor-return`](file:///data/dev/cp/.tasks/resolved/capsurf_agy/task-capsurf-20261116-beam-non-actor-return.json)；
  - 后台任务常驻监听守卫重新启动并保持活跃状态；
- **测试套件回归验证**：
  - `test_unified_errors.cp`、`test_trait_class_pass.cp`、`test_semver.cp`、`test_dec_pass.cp`、`test_unsafe_pass.cp`、`test_blob.cp`、`test_collections_matrix.cp`、`test_all.cp`、`test_stdlib_phase_b2_c.cp`、`test_basic.cp`、`test_string.cp`、`test_json.cp`、`test_crypto.cp` **100% PASS**。

- **热修性能优化 (`task-capsurf-20261116-codegen-quadratic-line-count`)**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：
    - 新增 `OutputBuffer` 结构体，封装 `content` 与 `newline_count`，实现 `std::ops::Deref`、`std::fmt::Write`、`std::fmt::Display` 与 `push_buffer`；
    - 将 `current_generated_line()` 从 O(N) 全量字节扫描重构为 O(1) 增量整型加法，消除语句数膨胀时的 O(N^2) 性能二次爆炸（10,000 条语句转译从 18.58s 缩短至 0.21s，提速 88 倍）；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：
    - 使用 `self.output.push_buffer(&body_str)` 零扫描高效拼接函数体；
  - 结单 `task-capsurf-20261116-codegen-quadratic-line-count` 并同步回写 `cpc_dev`。

### 2026-11-18

- [feat(ptr)]: 从 Bun 全量吸收并实装 `cap:ptr` 智能指针体系 (`lib/ptr/`)，首行附带 `// Source: bun/src/ptr/...` 谱系标注：
  - 新增 `shared.cp`, `weak_ptr.cp`, `cow.cp`, `cow_slice.cp`, `owned.cp`, `tagged_pointer.cp`, `raw_ref_count.cp`, `ref_count.cp`, `external_shared.cp`, `meta.cp`, `index.cp`。
  - 新增独立文档 `docs/stdlib/ptr.md` 与测试 `cap/test/test_ptr.cp` (8/8 PASS)。
- [feat(encoding)]: 全面补全 `lib/encoding/base64.cp`，支持 `decodeInPlace` 零堆分配原地解码与 4 种 `Codecs` 预设工厂；新增 `docs/stdlib/encoding.md` 与测试 `cap/test/test_encoding_complete.cp` (4/4 PASS)。
- [feat(sourcemap)]: 全量补齐 `lib/sourcemap/`（`vlq.cp`, `line_offset.cp`, `mapping.cp`, `internal_source_map.cp`, `chunk.cp`, `parsed_source_map.cp`, `sourcemap.cp`, `index.cp`），新增 `docs/stdlib/sourcemap.md` 与测试 `cap/test/test_sourcemap_complete.cp` (5/5 PASS)。
- [feat(semver)]: 全量补齐 Semver 2.0.0 规范库 `lib/semver/`（`version.cp`, `query.cp`, `range.cp`, `index.cp`），实装点分预发布权重梯级、高级范围表达式（`^`, `~`, `*`, `1.x`, `-`, `||`）、`maxSatisfying` 与版本递增 `increment`，新增 `docs/stdlib/semver.md` 与测试 `cap/test/test_semver_complete.cp` (5/5 PASS)。
- [feat(dotenv)]: 从 Bun 吸收构建生产级环境配置解析库 `lib/dotenv/`（`parser.cp`, `loader.cp`, `index.cp`），支持引号转义、`${VAR:-default}` 变量展开与级联覆盖加载状态机，新增 `docs/stdlib/dotenv.md` 与测试 `cap/test/test_dotenv_complete.cp` (6/6 PASS)。
- [fix(cpc)]: 升级 `cpc/src/resolver.rs`，使 `cap:dotenv` 独立精确映射至 `lib/dotenv/`。
- [feat(path)]: 全量吸收重构 `lib/path/`（`buffer_pool.cp`, `env_path.cp`, `index.cp`, `path.cp`, `posix.cp`, `resolve.cp`, `windows.cp`），完备 POSIX/Windows 规范化、零分配缓冲池与 EnvPath 迭代器，新增 `docs/stdlib/path.md` 与测试 `cap/test/test_path_complete.cp` (5/5 PASS)。
- [feat(event_loop)]: 从 Bun 吸收构建统一事件循环标准库 `lib/event_loop/`（`auto_flusher.cp`, `concurrent_task.cp`, `deferred_task_queue.cp`, `index.cp`, `loop.cp`, `task.cp`, `timer.cp`），新增 `docs/stdlib/event_loop.md` 与测试 `cap/test/test_event_loop_complete.cp` (6/6 PASS)。
- [fix(cpc)]: 升级 `cpc/src/resolver.rs`，使 `cap:event_loop` 独立精确映射至 `lib/event_loop/`。
- [feat(url)]: 全面补全 WHATWG URL 标准库 `lib/url/`（`percent.cp`, `search_params.cp`, `query.cp`, `url.cp`, `index.cp`），支持 IPv6 方括号主机、URLSearchParams 排序与百分号编解码，新增 `docs/stdlib/url.md` 与测试 `cap/test/test_url_complete.cp` (6/6 PASS)。
- [feat(safety)]: 从 Bun 吸收构建生产级并发安全与诊断标准库 `lib/safety/`（`thread_id.cp`, `ThreadLock.cp`, `CriticalSection.cp`, `asan.cp`, `alloc.cp`, `safety.cp`, `index.cp`），新增 `docs/stdlib/safety.md` 与测试 `cap/test/test_safety_complete.cp` (5/5 PASS)。
- [feat(clap)]: 从 Bun 吸收构建 `lib/clap/`（`args.cp`, `streaming.cp`, `comptime.cp`, `clap.cp`, `index.cp`），新增 `docs/stdlib/clap.md` 与测试 `cap/test/test_clap_complete.cp` (6/6 PASS)。
- [feat(s3)]: 全量吸收 Bun S3 运行时构建 `lib/s3/`（`simple_request.cp`, `list_objects.cp`, `multipart.cp`, `download_stream.cp`, `client.cp`, `signing.cp`），新增 `docs/stdlib/s3.md` 与测试 `cap/test/test_s3_complete.cp` (6/6 PASS)。
- [fix(cpc)]: 编译器底层缺陷修复：
  - `cpc/src/codegen/expr.rs`：修复变量类型回退至 `*anyopaque` 时，`starts_with('*')` 误判为已是指针而剥离取地址符 `&` 的问题（增加 `!= "*anyopaque"` 约束）。
  - `cpc/src/codegen/stmt.rs`：修复栈缓冲区数组在 while 循环中被误判为未初始化变量的重置问题。
- [feat(glob)]: 补全 `lib/glob/matcher.cp`（支持花括号展开 `{a,b}` 与字符集），新增 `lib/glob/walker.cp` 原生目录遍历引擎；新增 `docs/stdlib/glob.md` 与测试 `cap/test/test_glob_complete.cp` (4/4 PASS)。
- [fix(cpc)]: 编译器自愈修复：
  - `cpc/src/codegen/mod.rs`：`cp_not` 升级为 `inline fn cp_not`，确保 Zig 0.17 comptime 分支求值正常。
  - `cpc/src/codegen/types.rs`：修复 `.get()` fallback 为 `?Message` 导致误插 `.?` 的类型推导缺陷。
  - `cpc/src/codegen/prelude.rs`：`scan_decl` 增加智能指针声明识别，自动激活 `has_imported_ptr`。
- [feat(col)]: 全面补全 `lib/col/bit_set.cp`, `baby_list.cp`, `linear_fifo.cp`, `bounded_array.cp`，对齐 Bun 原生容器标准库，新增 `docs/stdlib/col.md`。
- [feat(conf)]: 全量补齐 `lib/conf/ini.cp` 与 `toml.cp`，新增 `docs/stdlib/conf.md`。

### 2026-11-17

- [Docs] Standardize all library files with `// Source: ...` lineage annotations.

- 修复 cap:bigint（算术改用 zig 块，避开 Managed 指针传递转译问题），重写 PriorityQueue（最小堆，无需 allocator 参数），Treap 改为可编译占位。相关测试全部通过。

- 修正溯源标注：此前脚本把未识别文件一律标 Custom（误标 323 个）。现改为：有 Bun 来源线索 131 个标 Bun+路径；192 个标 UNVERIFIED 待确认。新增 scripts/fix_lineage.py。test_http_full 为既有失败（*T/*const T），与本次无关。

- 溯源二次校验：按标识符重合度对照 /data/dev/bun/src 自动匹配 101 个文件（标注 needs review）；仅修改各文件第 1 行头部注释（git numstat 验证每文件 +1/-1）。剩余 91 个 UNVERIFIED。

- 溯源三次校验：index.cp 29 个标为 Custom(agy 生成导出入口)；17 个按同名匹配 Bun（name-matched, needs review）；仍有 45 个 UNVERIFIED。仅改第 1 行。

- 溯源四次校验：依据 CHANGELOG/SESSION 记录确认 16 个文件来源（Bun/Zig std/Custom），仅改第1行；剩余 29 个 UNVERIFIED 待用户判断。

- 2026-11-19: Enhanced `H2Stream` in `lib/http/h2/client.cp` by adding state transition methods (`rst`, `sentEndStream`, `recvEndStream`, `localClosed`, `remoteClosed`) ported from Bun's `h2_client/Stream.cp`.

## 2026-11-19

- 完成 `lib/napi/napi.cp` 的架构重构，将 `class Napi` 重构为符合 CP 规范的 `struct` 模块化形式（符合数据/行为解耦）。
- 完成 threading 资产迁移 (channel, condition, futex, mutex, pool, thread_pool, etc.)

## 2026-11-19

- 完成 `lib/threading/` 资产迁移，已实现 FFI shim 隔离与 CP 原生 API 封装。
- 确认 `lib/napi/napi.cp` 已完全符合 CP 模块化规范（struct 封装）。
- 完成 blob 及 encoding 资产迁移

## 2026-11-19

- 完成 `lib/blob/` 和 `lib/encoding/` 资产迁移，已实现 FFI shim 隔离与 CP 原生 API 封装。
- 确认资产符合 CP 架构设计准则，已通过初步审计。

## 2026-11-19

- `lib/blob/` 合规审计通过，确认实现零拷贝引用借用与 RAII 资源管理。
- 完成 url 资产迁移

## 2026-11-19

- 完成 `lib/url/` 资产迁移，已完成 FFI shim 隔离封装，并通过初步自动化构建测试。

## 2026-11-19

- `lib/url/` 合规审计通过，确认实现 RAII 确定性内存释放与协议解析逻辑。
- 完成 valkey 资产迁移

## 2026-11-19

- 完成 `lib/valkey/` 资产迁移，实现高性能 Valkey 协议解析。
- **项目阶段验收完成**：核心资产组全部迁移并审计合规。
- 优化 Valkey 协议解析，减少内存拷贝，适配 RAII

## [2026-11-19] Hotfix: Fix anyerror!void optionality mapping

- 修复 `cpc/src/codegen/types.rs` 中 `type_to_display_string` 对 `ResultType` 的转译 Bug，确保 `anyerror!void` 正确映射为 `anyerror!void`。
- 联动 `Optional` 类型，使得 `?anyerror!void` 能被正确生成。
- 通过 `bash scripts/sync_cpc.sh` 完成原子构建校验，验证修复有效。
- 完成 crypto 资产迁移
- 重构 cipher.cp 和 hash.cp，统一为静态 struct 模块化设计

## [2026-11-19] Hotfix: Fix export { ... } from ... symbol export

- 修复 `cpc/src/codegen/mod.rs`：在生成 Zig 代码时，添加了对 `export { ... } from ...` 语句的显式 `pub const` 别名生成，确保导出的符号在 `main.zig` 中可被正确引用，解决了缺失 import 引用的问题。
- 通过 `bash scripts/sync_cpc.sh` 完成原子构建校验，验证修复有效。

## [2026-11-19] Hotfix: Fix zlib linking failure in capsurf (automatic library path detection)

- 修正 `cpc` 编译器前端：在 `Codegen::new()` 中新增自动探测 linux 环境下的 `/lib64` 和 `/usr/lib64` 标准库搜索路径的逻辑。
- 在 `generate_module` 中新增逻辑，将探测到的库路径以及 `zlib` 的链接需求以 `// @linkPath` 和 `// @linkLibrary` 的形式 emit 到生成的 Zig 代码中，以便下游构建系统（capsurf）正确配置链接器。
- 修复了 Rust 编译器的 borrow checker 错误（通过 clone `linker_paths`）。
- 构建校验：`bash scripts/sync_cpc.sh` 构建通过。

## 2026-11-19

- 修改 `build.zig`，为 vendored 库编译添加 `-DAT_HWCAP=16` 预定义宏以修复 Zig 0.17.0 兼容性。
  hotfix: resolve keyword conflict for "test" and prevent redundant re-export of imports

- 2026-11-19: Initialized `lib/bundler/bundle.cp` and began migrating `BundleV2` struct.

## [2026-11-19] Hotfix: 完善 cap 工具链链接参数与下游 capsurf 编译自愈

- **cap/src/project/manifest.zig**:
  - `Manifest` 结构体新增 `bin_dependencies` 字段及其 JSON 解析、保存与释放逻辑。
  - `BuildConfig` 结构体新增 `entry` 字段支持指定入口文件。
- **cap/src/main.zig**:
  - 新增 `addArg`（去重参数追加）、`scanZigFileForLinkPragmas`（扫描转译生成 Zig 代码头部的 `@linkPath` 与 `@linkLibrary` 指令）。
  - 新增 `appendLinkArgs` 统一收集 uSockets、QuickJS vendor 路径、`bin_dependencies`、`build.c_flags` 和 `build.link_libs` 参数。
  - 在 `handleRun`、`handleBuild`、`compileSingleFile`、`handleTest`、`runFuzzSingleFile` 中接入动态链接参数生成。
  - 修正目标架构为 native 时的输出目录逻辑，使主程序产物精准写入 `bin/main`。
- **/data/dev/cp/TASK_QUEUE.jsonl**:
  - 标记 `task-capsurf-20261119-compiler-bug` 为 `resolved`。
- **/root/.agy/comm/cp-bun-capsurf.jsonl**:
  - 向下游韧性协作总线派发结单通知，解除 capsurf 阻塞状态。

## [2026-11-19] cap bin & sys 依赖按需编译实装与中央索引集成

- **lib/cap.json**:
  - 提炼中央库索引，剔除纯 CP 模块，仅保留含底层 C/二进制与系统动态库依赖的 `http`, `net`, `zlib`。
  - 为 `picohttpparser` 与 `usockets` 配置 Zig 编译所需的 `sources`, `include_dirs`, `c_flags`, `opt` 参数。
- **cap/src/project/manifest.zig**:
  - 增加 `BinDependencyConfig`、`SysDependencyConfig`、`CentralLibEntry`、`CentralLibIndex` 结构体。
  - `Manifest` 的 `bin_dependencies` 升级为支持对象型配置（含 C 编译参数与构建命令）。
  - 实现按 target triple 动态转换系统库为链接参数（如 `libz.so.1` -> `-lz`）。
- **cap/src/main.zig**:
  - 新增 `resolveSearchPath`、`loadCentralLibIndex`、`scanSourceForCapImports`。
  - 新增 `buildCWithZig`：无外部编译器依赖，直接使用 `zig build-lib` 交叉编译 C/C++ 源码输出目标平台 `.a` 静态库。
  - 新增 `resolveBinDependency`：优先检索已有预编译静态库，缺失时现场按需编译。
  - 在 `appendLinkArgs` 中打通源码 `cap:*` 导入与中央索引关联 C/动态库的自动参数注入。
  - 在 `fetchAndLink` 中实装 `precompilePackageBinDeps`，在 `cap install` 安装包时预先编译静态库。
- **cap/test/test_bin_sys_on_demand.cp**:
  - 新增正式回归测试用例，验证导入 `cap:http` 时自动触发 `libpicohttpparser.a` 按需编译与链接。

## [2026-11-20] 工业级 Gzip / Zlib 压缩引擎重构与全景特性吸收

- **lib/pack/\_codecs.cp**:
  - 增加 `pub type ZStream = z_stream_s;` 大写类型别名，符合 CP 结构体字面量语法规范。
  - 新增 `pub struct NativeZlib` 容器，封装 `deflateInit2_`、`deflate`、`deflateEnd`、`deflateBound`、`inflateInit2_`、`inflate`、`inflateEnd`、`crc32` 等 C-FFI 声明，消除与顶层导出函数的命名冲突。
- **lib/pack/gzip.cp**:
  - 重构并扩充为 344 行工业级 Gzip & Deflate 编解码引擎。
  - 修正 `writeTrailer` 参数遮蔽问题。
  - 实现基于 `window_bits = 31` 的全量与流式 `compressWithWindowBits`。
  - 实现带自动倍增扩容保障（`mut_cap * 2`）的 `decompressWithWindowBits`，自动支持任意大尺寸流式解压。
  - 导出顶层标准 API：`gzip`、`gunzip`、`deflate`、`inflate`、`deflateRaw`、`inflateRaw`。
  - 导出面向对象包装器：`Gzip`、`Gunzip`、`DeflateCodec`、`InflateCodec`、`DeflateRawCodec`。
  - 完整保留 `RleCodec` 保证向后兼容性。
- **lib/pack/index.cp**:
  - 收敛对外暴露符号，显式导出 `NativeZlib` 与安全类型，防止内部底层声明污染。
- **cap/test/test_pack_gzip_complete.cp**:
  - 新增全量回归测试套件，全面覆盖 Gzip 往返、RFC 1952 帧头校验、动态倍增扩容、Zlib Deflate/Inflate、Raw Deflate/Inflate、面向对象 API 及 RLE 兼容性。
- **cp_ref.md & cp_agent_ref.md**:
  - 同步更新权威手册与智能体契约手册中 `cap:pack`（`cap:compress`）的功能矩阵说明。
- **lib/pack/gzip.cp（第二阶段流式分块增强）**:
  - 新增 `GzipStream`、`GunzipStream`、`DeflateStream`、`InflateStream` 四大流式分块管道，文件由 344 行扩充至 808 行。
  - 采用堆安全分配机制保证底层 `ZStream` 指针稳定性，消除状态机指针悬空。
- **lib/pack/gzip.cp（DRY 架构收敛）**:
  - 抽离 `ZlibStreamCompressor` 与 `ZlibStreamDecompressor` 通用状态机，消除 160+ 行重复实现。
  - 行数由 808 行精炼收敛至 647 行，保持 100% 接口与行为兼容。
- **docs/pack/zlib.md & docs/pack/zlig.md**:
  - 新建 `docs/pack/` 目录，编写全套 Zlib/Gzip 详细开发手册与使用指南。
- **docs/stdlib/pack_native.md**:
  - 更新索引指向 `docs/pack/zlib.md`。
- *_lib/pack/_ (Brotli & Zstd 吸收与全量溯源)**:
  - 新增 `lib/pack/brotli.cp`（吸收自 `/data/dev/bun/src/brotli/brotli.cp`，实现 `BrotliMode`、`BrotliHeader`、`Brotli`）。
  - 新增 `lib/pack/zstd.cp`（吸收自 `/data/dev/bun/src/zstd/zstd.cp`，实现 `ZstdFrameHeader`、`Zstd`）。
  - 更新 `lib/pack/*.cp` 共 10 个文件的首行 `// Source:` 备注，并在 `index.cp` 统一导出。
  - 新增 `docs/pack/pack.md` 与 `cap/test/test_pack_full.cp`。
- *_lib/col/_ (容器升级与全量溯源)**:
  - 升级 `lib/col/treap.cp` 为完整 1-based 内联节点池平衡树堆 `Treap<K, V>`。
  - 重构 `lib/col/leb128.cp` 为纯正现代 CP 静态编解码引擎。
  - 增强 `lib/col/priority_queue.cp` 与 `lib/col/string_map.cp`，更新全部 13 个文件的首行 `// Source:` 备注。
  - 新增 `docs/col/col.md` 与 `cap/test/test_col_full.cp`。
- _*lib/encoding/* & lib/url/_ (纯正 CP 重构与全量溯源)**:
  - 重构 `lib/encoding/base64_.cp`、`base64.cp`、`hex.cp` 及 `lib/url/url_.cp`、`url.cp`、`percent.cp`、`search_params.cp` 为 100% 纯正现代 CP 语法，更新全部 10 个文件的首行 `// Source:` 备注。
  - 新增 `docs/encoding/encoding.md`、`docs/url/url.md` 与 `cap/test/test_encoding_url_full.cp`。
- **lib/path/_, lib/glob/_, lib/semver/_, lib/which/_ (完备性加固与全量溯源)**:
  - 修复 `PosixPath/WindowsPath.extname("..")` 边界，优化 `GlobMatcher`，增强 `semver/query.cp` 独立算子空格兼容，将 `which.cp` 重写为纯正 CP 语法并增加 `opendir` 目录排除校验，更新全部 16 个文件的首行 `// Source:` 备注。
  - 新增 `docs/path/path.md`、`docs/glob/glob.md`、`docs/semver/semver.md`、`docs/which/which.md` 与 `cap/test/test_path_glob_semver_which.cp`。
- *_lib/sourcemap/*, lib/patch/*, lib/hash/_ (功能增强与全量溯源)**:
  - 为 `LineDiffer` 新增 `editDistance`，为 `Hash` 新增 `wyhash`、`djb2`、`fnv1a32` 与 `WyhashStream`，更新全部 12 个文件的首行 `// Source:` 备注。
  - 新增 `docs/sourcemap/sourcemap.md`、`docs/patch/patch.md`、`docs/hash/hash.md` 与 `cap/test/test_sourcemap_patch_hash.cp`。
- *_lib/dotenv/*, lib/conf/*, lib/clap/_ (配置提取器增强与全量溯源)**:
  - 增强 `EnvMap.parse`、`ConfGetter`、`YamlDoc` 并优化 `clap.cp` 符号隔离，更新全部 15 个文件的首行 `// Source:` 备注。
  - 新增 `docs/dotenv/dotenv.md`、`docs/conf/conf.md`、`docs/clap/clap.md` 与 `cap/test/test_conf_dotenv_clap.cp`。
- *_lib/ptr/*, lib/safety/*, lib/event_loop/_ (缺陷修复与全量溯源)**:
  - 修复 `lib/ptr/ref_count.cp` 中 `ThreadSafeRefCount.deref` 原子递减缺陷，重命名 `PtrThreadLock` 消除跨库导入符号碰撞，更新全部 25 个文件的首行 `// Source:` 备注。
  - 新增 `docs/ptr/ptr.md`、`docs/safety/safety.md`、`docs/event_loop/event_loop.md` 与 `cap/test/test_ptr_safety_event_loop.cp`。
- _*lib/s3/*, lib/security/*, lib/redis/* & lib/valkey/_ (预签名 URL、流式多帧解析器与纯 CP 重构)**:
  - 在 `lib/s3/simple_request.cp` 与 `client.cp` 中新增 `buildPresignedUrl` / `presignUrl`。
  - 在 `lib/security/csrf.cp` 中补齐 `u64` 防溢出守卫与 60s 时钟偏移守卫。
  - 重构 `lib/valkey/valkey_protocol_.cp` 与 `valkey_protocol.cp` 为纯正现代 CP 语法，在 `lib/redis/resp.cp` 新增 `ValkeyReader` 与 RESP3 编码器，更新全部 16 个文件的首行 `// Source:` 备注。
  - 新增 `docs/s3/s3.md`、`docs/security/security.md`、`docs/redis/redis.md` 与 `cap/test/test_s3_security_redis.cp`。
- *_lib/string/*, lib/simd/*, lib/sort/_ (Highway 级 SIMD 向量扩充与全量溯源)**:
  - 修复 `lib/string/builder.cp` 空切片引用，为 `SmolStr` 补齐 `isInline`/`isInlined`。
  - 在 `lib/simd/index.cp` 中完整实装 16/32 字节 SIMD 字节扫描与无分支大小写转换，更新全部 13 个文件的首行 `// Source:` 备注。
  - 新增 `docs/string/string.md`、`docs/simd/simd.md`、`docs/sort/sort.md` 与 `cap/test/test_string_simd_sort.cp`。

- *_lib/crypto/*, lib/sql/*, lib/dns/_ (去 Zig 残留、零堆分配接口与全量溯源)**:
  - 恢复/增强 `hash.cp`、`cipher.cp`，新增 `*Raw`/`pbkdf2Into`/`randomUUIDInto`、`TlsHostnameVerifier`、`PostgresWire.encodeStartupMessage`/`encodePasswordMessage`、`HostsTable`；清理 `boringssl.cp`；18 个文件首行 `// Source:`。
  - 新增 `docs/crypto/crypto.md`、`docs/sql/sql.md`、`docs/dns/dns.md`、`cap/test/test_crypto_sql_dns.cp`。
- *_lib/sync/*, lib/threading/*, lib/mem/_ (并发缺陷修复、MaxHeapAllocator 吸收与全量溯源)**:
  - `UnboundedQueue` 可选指针语法升级，修复 `ConcurrentMap`/`Condition`/`BumpArena`/`MaybeOwnedBuf` 编译问题，`lib/mem/allocator.cp` 新增 `MaxHeapAllocator`；41 个文件首行 `// Source:`。
  - 新增 `docs/sync/sync.md`、`docs/mem/mem.md`、`cap/test/test_sync_mem_full.cp`。
- *_lib/shell/*, lib/watch/*, lib/watcher/*, lib/elf/*, lib/ffi/_ (管道缓冲所有权修复与全量溯源)**:
  - `exec.cp` 管道中转缓冲与所有权移交，`builtin.cp` 新增 `cat`，`parser.cp` 指针接收者修正，watch/watcher 统一事件类型，`layout.cp` 新增 `fieldOffset`；20 个文件首行 `// Source:`。
  - 新增 `docs/shell/shell.md`、`docs/watch/watch.md`、`docs/elf/elf.md`、`docs/ffi/ffi.md`、`cap/test/test_shell_watch_elf_ffi.cp`。
- **cp_ref.md & cp_agent_ref.md**: 同步第三批 9 个标准库专著文档索引与协议矩阵详见链接。
- *_lib/http/_ (含 h2/h3, 31 个文件纯现代 CP 重构与全量溯源)**:
  - 31 个 `.cp` 补齐 `// Source:`，清零遗留 Zig 语法与只读方法误用 `*_` 缺陷；新增 `docs/http/http.md` 与更新 `cap/test/test_http_full.cp`。
- *_lib/net/*, lib/ws/*, lib/router/*, lib/io/*, lib/sys/_ (Zig 0.17 std.c 适配与全量溯源)**:
  - 30 个 `.cp` 补齐 `// Source:`，修复 `ws/frame.cp`、`router/{index,param_map,radix}.cp`、`sys/{fd,stat,file,copy_file,tmp,signal}.cp`；新增 5 份文档与 `cap/test/test_net_io_full.cp`。
- *_lib/json/*, lib/md/*, lib/html/*, lib/image/*, lib/blob/_ (清理死文件、缺陷修复与全量溯源)**:
  - 删除 `lib/blob/` 8 个零引用遗留文件，重构 `JsonBufferPool`，修复 `json/parser.cp` 与 `image/{gif,exif,jpeg,thumbhash,pipeline}.cp`，`lib/cap.json` 新增 `image` 静态 zlib 配置；新增 5 份文档与 `cap/test/test_json_md_html_image_blob.cp`。
- **cp_ref.md & cp_agent_ref.md**: 同步第四批 11 个标准库专著文档索引与协议矩阵（补充 `cap:sys` 与 `cap:blob`）。
- **lib/css/** (100 个文件溯源校准与核心清理)**:
  - 100 个 `.cp` 首行标准化为 `// Source: /data/dev/bun/src/css/...`，清理 `length.cp`/`stylesheet.cp` 遗留 `orelse`；新增 `docs/css/css.md`、`cap/test/test_css_full.cp`。
- *_lib/bench/*, lib/cli/*, lib/napi/*, lib/bigint/*, lib/event/_ (19 个文件现代化改造与全量溯源)**:
  - 重构 `lib/napi/{_napi,napi,index}.cp`（纯 CP Node-API v9 布局），`lib/event/emitter.cp` 新增 64 槽零堆分配 `EventBus`，清理 `bigint`/`event` 旧语法；新增 5 份专著文档与 `cap/test/test_bench_cli_napi_bigint_event.cp`。
- *_lib/bundler/_ & lib/{actor,beam,fs,parallel,reflect,serde,sqlite,stream,test,worker}.cp (收尾全覆盖)**:
  - 升级 `lib/bundler/bundle.cp`（`BundleV2`/`ChunkOrder`）并新增 `lib/bundler/index.cp`，清理 `fs.cp`/`reflect.cp`/`serde.cp`/`stream.cp` 头部与旧语法；新增 4 份专著文档与 `cap/test/test_core_runtime_bundler.cp`。
- **新吸收三大 Bun 高价值资产 (`lib/errno/`, `lib/logger/`, `lib/string/wcwidth.cp`)**:
  - 新增 `lib/errno/{errno,index}.cp`、`lib/logger/{logger,index}.cp`、`lib/string/wcwidth.cp`（导出至 `lib/string/index.cp`）。
  - 新增 `docs/errno/errno.md`、`docs/logger/logger.md`，更新 `docs/string/string.md`，新增 `cap/test/test_bun_new_assets.cp`。
- **cp_ref.md & cp_agent_ref.md**: 同步第五批 12 份专著文档索引与协议矩阵（补充 `cap:errno`/`cap:logger`/`cap:napi`/`cap:bigint`/`cap:bundler` 及 `stringWidth`）。

### 2026-11-20（`cpc` 6 项 `bun_agy` 热修 + `event_loop` 合并回 `event` + 4 组重复目录清理）

- **cpc 编译器加固 (`cpc/src/codegen/{mod,actor,stmt}.rs`, `cpc/src/resolver.rs`)**:
  - 修复 6 条 `bun_agy` 阻断工单：内部运行时错误类型更名为 `const __CpErr` 并按需导出 `Err`/`Error`；prelude、trait `downcast`、RAII `defer` 块及 enum 辅助方法内部形参/局部变量全面加 `__cp_` 前缀根治遮蔽；enum `get`/`fromName` 限定 `@This().fromString` 或直调 `std.meta.stringToEnum(@This(), __cp_s)`。
  - 修复 `async fn` 中 `import { print } from "cap:io"` 丢失 `print` 定义问题；`resolver.rs` 统一映射 `event_loop -> event`、`valkey -> redis`。
- **标准库去重与合并**:
  - **合并 `lib/event_loop/` -> `lib/event/`**：融合配对最小堆 `TimerHeap`、`AutoFlusher`、`ConcurrentTaskQueue`/`ConcurrentQueue`、`DeferredTaskQueue`、`AnyTaskWithExtraContext`、栈/堆双模安全 `ManagedTask` 与懒初始化 `PollEngine` 的 `EventLoop`；合并文档至 `docs/event/event.md` 并删除 `lib/event_loop/`、`docs/event_loop/`。
  - **清理 3 组冗余目录**：合并 `lib/valkey/` -> `lib/redis/protocol.cp`、合并 `lib/watch/` -> `lib/watcher/{filter,file_watcher}.cp`（文档迁至 `docs/watcher/watcher.md`）、清理零引用的历史旧目录 `lib/threading/`。
  - 更新适配 `cap/test/test_event_loop.cp`、`cap/test/test_bench_cli_napi_bigint_event.cp`、`cap/test/test_shell_watch_elf_ffi.cp`，9/9 回归套件 100% PASS。
- **GEMINI.md**: 更新多 AGY SQLite 通信命令为绝对路径 `/root/.agy/comm/task_queue.py`。

### 2026-11-20（12 大核心标准库对标 Bun 深度补全里程碑：glob/semver/dotenv/patch + http/router/sql + path/sourcemap/io/sys/errno）

- **第一路 (`lib/glob`, `lib/semver`, `lib/dotenv`, `lib/patch`)**：
  - `lib/glob/`: 完整递归 `**`、多层嵌套花括号展开 `{a,b}`、UTF-8 码点字符类 `[a-z]`/`[!a-z]`、`GlobMatchOptions`（大小写/路径规范化/dot）与 `Glob.scanSync` 真实目录遍历。
  - `lib/semver/`: 完整 Semver 2.0 `parseStrict`/`parseLoose`、Section 11 大整数安全 prerelease 排序、完整 Range（`~>`/`==`/`1.0.0 - *`/`include_prerelease`）、`sort`/`rsort`/`validRange`。
  - `lib/dotenv/`: BOM/CRLF/UTF-8 空白兼容、`export`、多行引号转义、`${VAR:-default}` vs `${VAR-default}` 与嵌套展开、偏移索引值语义重定位安全。
  - `lib/patch/`: 多文件/多 Hunk Unified Diff 解析（含 `\ No newline at end of file`、新增/删除/重命名/权限）、精确/偏移/模糊三阶容错 `apply`、零堆 Myers $O(ND)$ 差分与 `diffLines` 标准 Unified Diff 生成。
- **第二路 (`lib/http`, `lib/router`, `lib/sql`)**：
  - `lib/http/`: 新增 `chunked.cp`（RFC 9112 流式 1 字节边界解码与 Trailers）、`redirect.cp`（301/302/303/307/308 方法重写、URL 合并、环路检测与跨域脱敏）；补全 `HttpConnectionPool`、`HttpProxyTunnel` + `NoProxyMatcher`（Basic Auth / CIDR）、`ContentEncodingNegotiator` 与对接 `cap:pack` 的 `StreamingDecompressor`。
  - `lib/router/`: 升级 5 级严格优先级回溯 Radix 引擎与 `TrailingSlashMode`；新增 `fs_router.cp`（`FileSystemRouter` 支持 `[slug]`/`[...all]`/`[[...opt]]`/`(group)` 与磁盘扫描）。
  - `lib/sql/`: 补全 Postgres v3 扩展查询（`Parse`/`Bind`/`Describe`/`Execute`/`Sync`）、RFC 5802/7677 `ScramSha256Client` 与全后端帧解码；补全 MySQL v10 握手/lenenc/命令帧编码；新增 SQLite 扩展 C-ABI、`SqlValue`、`SqlConnectionPool` 与多方言 `SqlTransactionStateMachine`。
- **第三路 (`lib/path`, `lib/sourcemap`, `lib/io`, `lib/sys`, `lib/errno`)**：
  - `lib/path/`: 补齐 POSIX/Windows 双模式 `parse`/`format`/`resolve`/`relative`/`normalize`/`toNamespacedPath`、UNC `\\?\UNC\server\share` 与多盘符边界处理。
  - `lib/sourcemap/`: 升级 `u64`/`VLQ64` 全边界编解码、新增 `MappingList`（排序 + 二分查找）、UTF-8 ↔ UTF-16 行列转换、`ParsedSourceMapRecord.toJSON()` 与 `SourceMapChunk` 多段合并。
  - `lib/io/`: 补全 `BufferedReader`（`readLine`/`readUntil`/`peek`/`discard`/`readFull`）、`BufferedWriter`（阈控刷盘与大块直通）、`LimitedReader`/`MultiReader`/`TeeReader`/`DiscardWriter`/`CountingWriter`/`ChunkedBuffer`。
  - `lib/sys/` & `lib/errno/`: 零 `std.posix` 残留全面适配 Zig 0.17 `std.c`/`std.os.linux`，新增 `fs_ops.cp` 与 `process.cp`；`lib/errno/` 扩充至 135 项完整 POSIX/Linux 错误码、全量英文 `description()` 与语义判定。
- **文档与测试**：同步更新 12 份 `docs/<lib>/<lib>.md` 专著文档，新增 3 套综合深度测试（`test_glob_semver_dotenv_patch_deep.cp`、`test_http_router_sql_deep.cp`、`test_path_sourcemap_io_sys_errno_deep.cp`），10/10 联合回归测试套件 100% PASS，全库 405/405 `.cp` 文件 `// Source:` 100% 合规。

### 2026-11-20（`cpc` 编译器五阶段零回归模块化重构收官）

- **Stage 1 (`cpc/src/codegen/mod.rs`: 4,330 -> 1,499 行)**：拆分为 `runtime_emit.rs`、`decl_enum.rs`、`decl_class.rs`、`zig_escape.rs` 并抽离独立 Zig 模板 `cpc/src/codegen/runtime/json.zig`（`include_str!`）。
- **Stage 2 (`cpc/src/codegen/expr.rs`: 4,957 行 -> 6 文件)**：打碎 4,631 行巨型 `generate_expr`，拆为 `expr/{mod,call,builtin,error_flow,ops,control}.rs`。
- **Stage 3 (`cpc/src/parser.rs`: 9,568 行 -> 8 文件)**：拆为 `parser/{mod,decl,container,types,stmt,pattern,expr,primary}.rs`（全员 < 2,000 行）。
- **Stage 4 (`cpc/src/sema.rs`: 7,310 行 -> 7 文件)**：拆为 `sema/{mod,macros,decl,stmt_expr,safety,coerce,type_infer}.rs`（全员 <= 1,407 行）。
- **Stage 5 (`cpc/src/codegen/types.rs`: 2,927 行 -> 4 文件)**：拆为 `codegen/types/{mod,infer_cp,infer_zig,map_type}.rs`（全员 <= 1,305 行）。
- **门禁脚本**：新增 `scripts/check_cpc_size.sh` 文件体积检查脚本；全部 5 个阶段均通过 10 套核心测试 **10/10 确定性 SHA256 逐字节零差异验证** + 端到端 `cap test` 全绿回归。

### 2026-11-20（第二批 8 大核心标准库对标 Bun 深度补全里程碑：md/html/json/ws + shell/napi/elf/logger）

- **第一路 (`lib/md`, `lib/html`, `lib/json`, `lib/ws`)**：
  - `lib/md/`: 新增 `ast.cp` 与 `parser.cp`（1,629 行），实现完整 CommonMark + GFM 引擎（ATX/Setext 标题 + 去重 `headingIds`、围栏/缩进代码、嵌套引用/有序无序列表、GFM 表格含 `\|` 转义、任务列表、删除线、多反引号行内代码、链接/图片/引用定义、自动链接与 XSS 安全转义）。
  - `lib/html/`: 新增 `tokenizer.cp`（HTML5 全量词法器 + `<script>`/`<style>` RawText 模式），升级 `escape.cp`（86 个命名实体 + 十/十六进制 Emoji 实体），升级 `selector.cp`（复合选择器与栈追踪 `HtmlRewriter` 全量元素操作）。
  - `lib/json/`: 升级 `parser.cp`（SIMD 零拷贝快路径 + RFC 8259 完整转义与 UTF-16 代理对 4 字节 UTF-8 Emoji 解码 + 严格拒绝前导零/尾随逗号/非法控制字符 + 嵌套深度保护）；新增 `serializer.cp`（`stringify`/`stringifyPretty`）；`value.cp` 新增 RFC 6901 `pointer` 与键序无关 `equals`。
  - `lib/ws/`: 升级 `opcode.cp`/`mask.cp`（16B SIMD 异或掩码）/`frame.cp`（RSV/64-bit/控制帧校验与 ClosePayload）；新增 `handshake.cp`（基于 `cap:crypto` SHA-1 + Base64 真实计算 `Sec-WebSocket-Accept`）与 `assembler.cp`（多帧分片重组、交错控制帧与 Auto-Pong 状态机）。
- **第二路 (`lib/shell`, `lib/napi`, `lib/elf`, `lib/logger`)**：
  - `lib/shell/`: 新增 `env.cp`（`ShellEnv`、`DirStack`、单/双引号区分与 `${VAR:-/:=/:+/#}` 展开）；升级 `lexer.cp`/`parser.cp`/`builtin.cp`/`exec.cp` 支持 23 条进程内 Builtin（`echo`/`printf`/`cat`/`test`/`[`/`cd`/`pushd`/`popd`/`env`/`export`/`unset`/`head`/`wc`/`mkdir`/`rm` 等）、左结合 `&&`/`||`/`;` 短路及 `<`/`>`/`>>`/`2>`/`2>&1`/`&>` 真实文件与内存重定向。
  - `lib/napi/`: 补齐全部 24 项 `NapiStatus`、`Float16`、完整 N-API v9 结构体；新增 `runtime.cp`（819 行：`HandleScope`、`Reference`、`AsyncWork`、`ThreadsafeFunction` 环形队列与离线 `NapiEnvMock`）。
  - `lib/elf/`: 新增 `elf64.cp`（653 行：`Elf64File` 完整解析 Phdr/Shdr/Symtab/Strtab/Dynamic `DT_NEEDED`/`soname`/`rpath`）与 `ExeFormat.detect`（ELF32/64、Mach-O 32/64/Fat、PE、WASM、Shebang），实测解析当前 `/data/dev/cp/cap/bin/cpc`。
  - `lib/logger/`: 升级 `logger.cp` 支持 `LineIndex` $O(\log N)$ 行缓存、跨文件 `DiagNote` 副定位标尺、`max_errors` 熔断器、`merge` 与 `formatJson` 机器可读输出。
- **文档与测试**：同步更新 8 份 `docs/<lib>/<lib>.md`，新增 `cap/test/test_md_html_json_ws_deep.cp` 与 `cap/test/test_shell_napi_elf_logger_deep.cp`；6/6 联合回归套件 100% PASS，全库 414/414 文件 `// Source:` 100% 合规。

### 2026-11-20（Batch 3 八大标准库深度审核补全里程碑：string/blob/image/mem + ffi/bigint/bench/cli）

- **第一路 (`lib/string`, `lib/blob`, `lib/image`, `lib/mem`)**：
  - `lib/string/`: 补齐 UTF-8 ↔ UTF-16LE 代理对严格编解码、`GraphemeIterator`（UAX #29：CRLF/ZWJ/国旗 Regional Indicator/Hangul Jamo）、`escapeJs` 与新增 `utils.cp`（441 行 SIMD `isAllAscii`/`eqlIgnoreCaseAscii`/`indexOf`/`split`/`replaceAll`/`trim`）。
  - `lib/blob/`: 补齐负索引 `sliceSigned`、`Blob.concat`/`equals`/`stream`（`BlobChunkReader`）；修复 `Blob.fromFile` 安全返回 `null`；补齐 `File` `exists`/`write`/`delete` 与真实 `statx` `lastModified`。
  - `lib/image/`: 全面审核 11 文件并清零隐患：修复 `thumbhash.cp` `toDataUrl` 占位符为真实 Base64 编码、修复 `gif.cp` `Gif.decode` 编译与对齐缺陷、补全 `ImageFormat.detect`/`probeDimensions`（PNG/JPEG/GIF/WebP/BMP/ICO/SVG）、Indexed PNG `PLTE`/`tRNS` 及双线性缩放/滤镜管线。
  - `lib/mem/`: 新增 `primitives.cp`（`volatile` `secureZero` + 对齐原语）、`fixed_buffer.cp`（`FixedBufferAllocator`）、`pool.cp`（防 double-free `SlabPool`）并加固 `BumpArena`/`RefCounter` 边界校验。
- **第二路 (`lib/ffi`, `lib/bigint`, `lib/bench`, `lib/cli`)**：
  - `lib/ffi/`: 补齐 16 种 C-ABI 类型矩阵、真实 POSIX `dlopen`/`dlsym`/`dlclose`/`dlerror`（实测加载调用 `strlen`/`abs`）、嵌套/`#packed` `CStructLayout`、零拷贝 `CString` 与 `FfiCallbackDesc`。
  - `lib/bigint/`: 补全任意精度 `BigInt` 全套四则/`divMod`/`mod`/位运算/`pow`/`modPow`/`gcd`/多进制双向转换，及 `BigInt256` 除模/移位/位运算/溢出检测。
  - `lib/bench/`: 吸收 Bun `src/perf/` 资产，新增 `trace.cp`（零堆 `TraceRingBuffer`/`PerfTrace` Chrome Trace JSON）与 `suite.cp`（`BenchSuite` 预热与 Markdown/JSON 报告），升级 `BenchStats` 至 512 槽 Welford 方差与 P50/P75/P90/P95/P99/P999 分位数。
  - `lib/cli/`: 补齐 16/256/TrueColor RGB 与 `stripAnsi`、`Terminal.getSize`（Linux `ioctl TIOCGWINSZ`）、`Spinner`、`ProgressBar` ETA/吞吐、以及集成 `cap:string` `stringWidth` 精准宽字符对齐的 `CliTable`（Ascii/Box/Markdown 边框）。
- **文档、测试与编译器正式版**：同步更新 8 份 `docs/<lib>/<lib>.md`，新增 `test_string_blob_image_mem_deep.cp` 与 `test_ffi_bigint_bench_cli_deep.cp`；发布 `cpc` Release 优化正式版（`bash scripts/sync_cpc.sh release`）并更新 `GEMINI.md`。

<!-- Archived batch from hot log -->
## [2026-11-21] - 新建 `cap:i18n` 国际化消息目录、CLDR 复数规则与 `Accept-Language` 协商引擎（5/5 PASS，自动动态挂载 `cap.i18n`）

- 新建 [`lib/i18n/`](file:///data/dev/cp/lib/i18n/index.cp)（`catalog.cp`、`index.cp`）：`LocaleNegotiator`（RFC 4647 `Accept-Language` q-value 加权协商与多级回退链）、`PluralRules`（英/法/中/日/俄/波/阿 CLDR 基数复数规则）及 `I18nCatalog`（`loadJson` 嵌套 JSON 扁平导入、单遍 `{key}` 安全插值防二次展开、`tp` 复数与回退渲染）；文档 [`doc/lib/i18n.md`](file:///data/dev/cp/doc/lib/i18n.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_i18n_production_deep.cp`](file:///data/dev/cp/cap/test/test_i18n_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:timeseries` 时序存储、Rollup 降采样与 Z-Score 异常检测引擎（5/5 PASS，自动动态挂载 `cap.timeseries`）

- 新建 [`lib/timeseries/`](file:///data/dev/cp/lib/timeseries/index.cp)（`series.cp`、`index.cp`）：`TimeSeries`（有限值与单调时间戳校验、`retention_ms` 均摊 $O(1)$ 头部淘汰、$O(\log N)$ 二分 `queryRange`、8 种 `AggOp` 步长对齐 `downsample`、支持归零重置的 `rate`、`sma`/`ema` 平滑、Welford `summarize`、`detectAnomaliesZScore` 与 `toJson`）；文档 [`doc/lib/timeseries.md`](file:///data/dev/cp/doc/lib/timeseries.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_timeseries_production_deep.cp`](file:///data/dev/cp/cap/test/test_timeseries_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:guardrail` AI Agent 安全护栏、PII/密钥脱敏与输入输出拦截引擎（5/5 PASS，自动动态挂载 `cap.guardrail`）

- 新建 [`lib/guardrail/`](file:///data/dev/cp/lib/guardrail/index.cp)（`pii.cp`、`policy.cp`、`index.cp`）：`PiiScanner`（Email/IPv4/Luhn 校验银行卡/`sk-`/`ghp_`/`AKIA`/JWT 识别与 `redact`/`redactTyped` 脱敏）及 `GuardrailPolicy`（`Allow`/`Redact`/`Block` 多级流水线、超长/禁用短语/越界特征拦截与 `inspectInput`/`inspectOutput` 防泄漏）；文档 [`doc/lib/guardrail.md`](file:///data/dev/cp/doc/lib/guardrail.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_guardrail_production_deep.cp`](file:///data/dev/cp/cap/test/test_guardrail_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:migrate` 确定性 Schema 迁移规划、防篡改校验与 SQLite 事务执行引擎（5/5 PASS，自动动态挂载 `cap.migrate`）

- 新建 [`lib/migrate/`](file:///data/dev/cp/lib/migrate/index.cp)（`migrator.cp`、`index.cp`）：`MigrationRegistry`（`-- +migrate Up`/`Down` 双段 SQL 解析、乱序自动升序 `planUp`、逆序步进 `planDown`、SHA-256 校验和 `ChecksumMismatch` & `MissingAppliedMigration` 防篡改校验、`statusReport`）及 `SqliteMigrator`（基于 `cap:sqlite` 自动管理 `_cap_migrations` 并支持事务级升级/幂等跳过/回滚）；文档 [`doc/lib/migrate.md`](file:///data/dev/cp/doc/lib/migrate.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_migrate_production_deep.cp`](file:///data/dev/cp/cap/test/test_migrate_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:feature` Feature Flags、确定性单调灰度分桶与 A/B/n 实验引擎（5/5 PASS，自动动态挂载 `cap.feature`）

- 新建 [`lib/feature/`](file:///data/dev/cp/lib/feature/index.cp)（`flags.cp`、`index.cp`）：`FeatureEngine`（`KillSwitch` 一键熔断、`TargetingRule` 多操作符属性靶向、基于 FNV-1a `% 10000` 的确定性单调分桶、多变体 A/B/n 权重校验分流、缺失降级 `FlagNotFound` 与 `toJsonSnapshot`）；文档 [`doc/lib/feature.md`](file:///data/dev/cp/doc/lib/feature.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_feature_production_deep.cp`](file:///data/dev/cp/cap/test/test_feature_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:audit` SHA-256 防篡改哈希链审计日志与合规导出引擎（5/5 PASS，自动动态挂载 `cap.audit`）

- 新建 [`lib/audit/`](file:///data/dev/cp/lib/audit/index.cp)（`trail.cp`、`index.cp`）：`AuditTrail`（基于 `"CP-AUDIT-V1\x00"` 规范编码 + `cap:crypto` SHA-256 链式哈希、`verifyChain` 逐条验链并精确定位 `first_corrupt_seq`、按 `actor`/`action`/时间窗检索、`exportJsonl`/`importJsonl` 往返完整性校验及 RFC-4180 `exportCsv`）；文档 [`doc/lib/audit.md`](file:///data/dev/cp/doc/lib/audit.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_audit_production_deep.cp`](file:///data/dev/cp/cap/test/test_audit_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:di` IoC 依赖注入与生命周期管理服务容器（5/5 PASS，自动动态挂载 `cap.di`）

- 新建 [`lib/di/`](file:///data/dev/cp/lib/di/index.cp)（`container.cp`、`index.cp`）：`ServiceContainer` & `ServiceScope`（`Singleton`/`Scoped`/`Transient` 三态生命周期、严格按实例化逆序 `deinit`、调用栈循环依赖 `CircularDependency` 环路路径追踪、`Singleton` 捕获 `Scoped` 的 `ScopeViolation` 防护、`registerMany` 多实现扩展点与 `build()` 冻结）；文档 [`doc/lib/di.md`](file:///data/dev/cp/doc/lib/di.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_di_production_deep.cp`](file:///data/dev/cp/cap/test/test_di_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:search` 本地全文倒排索引、前缀自动补全与高亮摘要引擎（5/5 PASS，自动动态挂载 `cap.search`）

- 新建 [`lib/search/`](file:///data/dev/cp/lib/search/index.cp)（`trie.cp`、`engine.cp`、`index.cp`）：`PrefixTrie`（频次降序 Top-K 自动补全）及 `SearchIndex`（`title`/`body` 多字段 BM25F 加权、`+must`/`-exclude`/`AND`/`OR`/`NOT` 布尔检索、`"exact phrase"` 连续词序匹配、`upsertDocument`/`removeDocument` 与 `highlightSnippet` 稠密窗口 `<mark>` 高亮摘要）；文档 [`doc/lib/search.md`](file:///data/dev/cp/doc/lib/search.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_search_production_deep.cp`](file:///data/dev/cp/cap/test/test_search_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:table` ANSI/UTF-8 宽度感知终端与 Markdown 表格渲染引擎（5/5 PASS，自动动态挂载 `cap.table`）

- 新建 [`lib/table/`](file:///data/dev/cp/lib/table/index.cp)（`width.cp`、`table.cp`、`index.cp`）：`DisplayWidth`（跳过 ANSI CSI 颜色序列、UTF-8 解码与 CJK 双倍/组合字符零宽计算、安全不截断 UTF-8 字符并保留 `\x1b[0m`）及 `Table`（`Ascii`/`Box`/`Rounded`/`Markdown`/`Borderless` 五大样式、列对齐与 GFM `:---:` 标记、Footer 分隔线、`max_width` 自动折行、文本与数值排序及 `ColumnMismatch` 校验）；文档 [`doc/lib/table.md`](file:///data/dev/cp/doc/lib/table.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_table_production_deep.cp`](file:///data/dev/cp/cap/test/test_table_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:ini` 无损 INI/Systemd/Gitconfig 解析编辑引擎 & 解除 `cpc` `"ini"` 旧路由别名（5/5 PASS）

- 解除 [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) `"ini" => "conf"` 旧别名并原子部署新版 `cpc`（Release），同时在 `lib/ini/index.cp` 保留 `Ini`/`ini` 向后兼容重导出（既有 `test_conf_complete.cp` 2/2 零回归）；
- 新建 [`lib/ini/`](file:///data/dev/cp/lib/ini/index.cp)（`ini.cp`、`index.cp`）：`IniDoc`（支持 Systemd 多值键 `getAll`、`\` 续行、Gitconfig `[sec "sub"]`、引号内 `#`/`;` 保护、类型安全 `getInt`/`getFloat`/`getBool`、保留原始注释与顺序的无损 `set`/`removeKey`/`removeSection` 及 `toJson`）；文档 [`doc/lib/ini.md`](file:///data/dev/cp/doc/lib/ini.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_ini_production_deep.cp`](file:///data/dev/cp/cap/test/test_ini_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:diff` Myers 行/词级差异、Unified Diff 补丁与三方合并引擎（5/5 PASS，自动动态挂载 `cap.diff`）

- 新建 [`lib/diff/`](file:///data/dev/cp/lib/diff/index.cp)（`myers.cp`、`patch.cp`、`index.cp`）：`DiffEngine`（前缀/后缀修剪 + 64-bit FNV-1a Myers O(ND) 行级与词级最短编辑脚本）及 `UnifiedDiff`（多 Hunk 拆分与 `\ No newline at end of file` 处理、`applyPatch` 严格上下文校验防篡改、`merge3` 三方合并自动合入与 `<<<<<<< ours` 冲突标记）；文档 [`doc/lib/diff.md`](file:///data/dev/cp/doc/lib/diff.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_diff_production_deep.cp`](file:///data/dev/cp/cap/test/test_diff_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:eval` AI Agent 自动化评测与 CI 回归门禁引擎（5/5 PASS，自动动态挂载 `cap.eval`）

- 新建 [`lib/eval/`](file:///data/dev/cp/lib/eval/index.cp)（`metrics.cp`、`suite.cp`、`index.cp`）：`EvalScorer`（`exactMatch`/`containsAll`/`forbiddenNone`/SQuAD `tokenF1`/联动 `cap:json` 的 `jsonFieldMatch` 与数值稳定 `passAtK`）及 `EvalReport`（加权评分、精确排序 P95 延迟、Token/成本合计、`compareBaseline` CI 防退化门禁与 Markdown/JSON 报告导出）；文档 [`doc/lib/eval.md`](file:///data/dev/cp/doc/lib/eval.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_eval_production_deep.cp`](file:///data/dev/cp/cap/test/test_eval_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:toml` TOML v1.0.0 完整配置解析与序列化/JSON 转换引擎（5/5 PASS，自动动态挂载 `cap.toml`）

- 新建 [`lib/toml/`](file:///data/dev/cp/lib/toml/index.cp)（`value.cp`、`codec.cp`、`index.cp`）：`TomlParser`（基本/多行基本/字面/多行字面 4 种字符串、`\uXXXX`/`\UXXXXXXXX`、`[table]`/`[[array.of.tables]]`/内联表/多行尾逗号数组、`0x`/`0o`/`0b` 整数、`inf`/`nan` 浮点、RFC 3339 日期时间及 `DuplicateKey`/`TypeConflict`/64 层深度防护）、`TomlDoc`（`getPath` 与递归释放）及 `TomlSerializer`（`stringify` 往返与 RFC 8259 `toJson`）；文档 [`doc/lib/toml.md`](file:///data/dev/cp/doc/lib/toml.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_toml_production_deep.cp`](file:///data/dev/cp/cap/test/test_toml_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:yaml` YAML 1.2 核心子集解析与序列化/JSON 转换引擎（5/5 PASS，自动动态挂载 `cap.yaml`）

- 新建 [`lib/yaml/`](file:///data/dev/cp/lib/yaml/index.cp)（`value.cp`、`codec.cp`、`index.cp`）：`YamlParser`（`---`/`...`、块映射与嵌套序列 `- key: val`、嵌套流式 `[...]`/`{...}`、单双引号转义、`|`/`|-`/`>`/`>-` 块标量、`&anchor`/`*alias` 防环展开、Tab 缩进/重复键/64 层深度拦截）、`YamlDoc`（`getPath` 与递归释放）及 `YamlSerializer`（2 空格 `stringify` 与 RFC 8259 `toJson`）；文档 [`doc/lib/yaml.md`](file:///data/dev/cp/doc/lib/yaml.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_yaml_production_deep.cp`](file:///data/dev/cp/cap/test/test_yaml_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:codec` 多进制与强校验编码套件（5/5 PASS，自动动态挂载 `cap.codec`）

- 新建 [`lib/codec/`](file:///data/dev/cp/lib/codec/index.cp)（`base58_85.cp`、`bech32_crockford.cp`、`index.cp`）：`Base58`（含前导零保留与 Double-SHA256 `Base58Check`）、`Base85`（Adobe ASCII85 含 `<~...~>`/`'z'` 压缩/余数填充/溢出拦截 + RFC 1924）、`Bech32`（BIP-173 & BIP-350 `Bech32m` 大小写混用与校验和拦截）及 `Crockford32`（`I/L/O` 纠错 + 模 37 校验符）；文档 [`doc/lib/codec.md`](file:///data/dev/cp/doc/lib/codec.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_codec_production_deep.cp`](file:///data/dev/cp/cap/test/test_codec_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:prompt` AI Agent 上下文窗口预算管理与 LLM 输出安全提取引擎（5/5 PASS，自动动态挂载 `cap.prompt`）

- 新建 [`lib/prompt/`](file:///data/dev/cp/lib/prompt/index.cp)（`messages.cp`、`extractor.cp`、`index.cp`）：`ContextWindow`（集成 `cap:text` `TokenEstimator` 的多角色 Token 预算裁剪、置顶保护、孤立 `Tool` 结果自动清理、OpenAI & Anthropic 标准 JSON 导出）及 `PromptExtractor`（Markdown 围栏代码块、字符串字面量感知平衡 JSON 块、XML 标签多段提取与 `wrapDelimited` 防提示词边界逃逸）；文档 [`doc/lib/prompt.md`](file:///data/dev/cp/doc/lib/prompt.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_prompt_production_deep.cp`](file:///data/dev/cp/cap/test/test_prompt_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:supervisor` BEAM OTP 监督树状态机与分布式弹性原语（5/5 PASS，自动动态挂载 `cap.supervisor`）

- 新建 [`lib/supervisor/`](file:///data/dev/cp/lib/supervisor/index.cp)（`strategy.cp`、`resilience.cp`、`index.cp`）：`SupervisorTree`（`OneForOne`/`OneForAll`/`RestForOne` OTP 拓扑逆序停止与顺序重启、`Permanent`/`Transient`/`Temporary` 策略与滑动时间窗重启风暴熔断 `EscalateShutdown`）、`BackoffPolicy`（Fixed/Linear/Exponential/FullJitter/DecorrelatedJitter + SplitMix64）、`Bulkhead` 并发舱壁与 `RetryBudget` 滑动窗口重试预算；文档 [`doc/lib/supervisor.md`](file:///data/dev/cp/doc/lib/supervisor.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_supervisor_production_deep.cp`](file:///data/dev/cp/cap/test/test_supervisor_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:openapi` OpenAPI 3.1.0 规范构造与请求契约校验引擎（5/5 PASS，自动动态挂载 `cap.openapi`）

- 新建 [`lib/openapi/`](file:///data/dev/cp/lib/openapi/index.cp)（`spec.cp`、`router.cp`、`index.cp`）：`OpenApiDoc`（OpenAPI 3.1.0 Info/Server/Tag/SecurityScheme/ComponentSchema/Operation 构造、路径模板与 `operationId` 唯一性校验、RFC 8259 JSON 导出）及 `OpenApiValidator`（静态段优先路径匹配、404 `RouteNotFound` vs 405 `MethodNotAllowed` 判别、path/query/header/cookie 必填与基础类型及请求体契约校验）；文档 [`doc/lib/openapi.md`](file:///data/dev/cp/doc/lib/openapi.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_openapi_production_deep.cp`](file:///data/dev/cp/cap/test/test_openapi_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:graphql` GraphQL Oct 2021 词法/AST/复杂度守卫与无损打印引擎（5/5 PASS，自动动态挂载 `cap.graphql`）

- 新建 [`lib/graphql/`](file:///data/dev/cp/lib/graphql/index.cp)（`lexer.cp`、`parser.cp`、`printer.cp`、`index.cp`）：`GqlLexer`（全 14 标点 + `"""` 块字符串 + 行列追踪）、`GqlParser`/`GqlParseOptions`/`GqlDocument`（query/mutation/subscription/变量/指令/别名/片段 + 可配置深度/节点/字段/别名安全守卫与复杂度统计）及 `GqlPrinter`（Pretty/Minify 无损往返等价）；文档 [`doc/lib/graphql.md`](file:///data/dev/cp/doc/lib/graphql.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_graphql_production_deep.cp`](file:///data/dev/cp/cap/test/test_graphql_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:rbac` 分层角色继承与显式拒绝优先策略鉴权引擎（5/5 PASS，自动动态挂载 `cap.rbac`）

- 新建 [`lib/rbac/`](file:///data/dev/cp/lib/rbac/index.cp)（`permission.cp`、`engine.cp`、`index.cp`）：`PermissionMatcher`（冒号分段、`*`/`**` 尾段通配及请求通配符拦截）与 `RbacEngine`（角色继承 + BFS 环路检测 `CycleDetected` + 主体/角色/继承链三层显式拒绝优先 + 默认拒绝与 `AccessDecision` 审计追踪）；文档 [`doc/lib/rbac.md`](file:///data/dev/cp/doc/lib/rbac.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_rbac_production_deep.cp`](file:///data/dev/cp/cap/test/test_rbac_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:asn1` ITU-T X.690 DER 编解码与零 C 依赖 X.509 v3 证书内省引擎（5/5 PASS，自动动态挂载 `cap.asn1`）

- 新建 [`lib/asn1/`](file:///data/dev/cp/lib/asn1/index.cp)（`der.cp`、`x509.cp`、`index.cp`）：严格最短定长 `DerReader`/`DerWriter`（64 层深度防护）、`OidCodec` 与 `X509Inspector`（解析版本/序列号/签发者与主体 CN+O/UTCTime & GeneralizedTime 有效期/SAN DNS/BasicConstraints CA/SHA-256 指纹）；文档 [`doc/lib/asn1.md`](file:///data/dev/cp/doc/lib/asn1.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_asn1_production_deep.cp`](file:///data/dev/cp/cap/test/test_asn1_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:pubsub` NATS/MQTT 通配符主题消息代理与背压/保留消息引擎（5/5 PASS，自动动态挂载 `cap.pubsub`）

- 新建 [`lib/pubsub/`](file:///data/dev/cp/lib/pubsub/index.cp)（`topic.cp`、`broker.cp`、`index.cp`）：零分配 `TopicMatcher`（支持 NATS `.`/`*`/`>` 与 MQTT `/`/`+`/`#` 及畸形模式拦截）与 `PubSubBroker`（多订阅扇出、`DropOldest`/`DropNewest` 有界背压与 `publishRetained` 保留消息回放/清除）；文档 [`doc/lib/pubsub.md`](file:///data/dev/cp/doc/lib/pubsub.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_pubsub_production_deep.cp`](file:///data/dev/cp/cap/test/test_pubsub_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:wasm` WebAssembly 1.0 二进制解析、校验与构造引擎（5/5 PASS，自动动态挂载 `cap.wasm`）

- 新建 [`lib/wasm/`](file:///data/dev/cp/lib/wasm/index.cp)（`leb128.cp`、`module.cp`、`builder.cp`、`index.cp`）：`Leb128`（U32/U64/I32/I64 溢出与截断防护）、`WasmModule.parse`（`\0asm` 魔数/版本/节序递增校验、Type/Import/Func/Memory/Export/Start/Code/Custom 全解析）与 `WasmBuilder`；文档 [`doc/lib/wasm.md`](file:///data/dev/cp/doc/lib/wasm.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_wasm_production_deep.cp`](file:///data/dev/cp/cap/test/test_wasm_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:tui` 终端 UI Flex 布局、ANSI 键盘解析与零闪烁差分渲染引擎（5/5 PASS，自动动态挂载 `cap.tui`）

- 新建 [`lib/tui/`](file:///data/dev/cp/lib/tui/index.cp)（`input.cp`、`layout.cp`、`canvas.cp`、`index.cp`）：`InputParser`（ASCII/UTF-8/Ctrl/Alt/CSI/SS3 键盘解析）、`Rect`/`Constraint`/`Layout`（严格守恒 Flex 分割）、`TuiBuffer`（单/双/圆角边框与 Sparkline）及零闪烁 `TuiDiff.renderDiff`；文档 [`doc/lib/tui.md`](file:///data/dev/cp/doc/lib/tui.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_tui_production_deep.cp`](file:///data/dev/cp/cap/test/test_tui_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:audio` WAV PCM16/F32 编解码、重采样与语音 AI Energy VAD 引擎（5/5 PASS，自动动态挂载 `cap.audio`）

- 新建 [`lib/audio/`](file:///data/dev/cp/lib/audio/index.cp)（`wav.cp`、`dsp.cp`、`index.cp`）：`WavCodec`（RIFF/WAVE PCM16 & Float32 编解码 + `LIST`/`JUNK`/`fact` 扩展块跳过）、`AudioDsp`（48kHz→16kHz 线性重采样、`stereoToMono`、RMS/dBFS/Peak/ZCR、正弦波生成）及 `EnergyVad` 语音活动区间检测；文档 [`doc/lib/audio.md`](file:///data/dev/cp/doc/lib/audio.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_audio_production_deep.cp`](file:///data/dev/cp/cap/test/test_audio_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:git`（5/5）与 `cap:pdf`（5/5）双标准库落地（自动动态挂载 `cap.git` / `cap.pdf`）

- **`cap:git`**（[`lib/git/`](file:///data/dev/cp/lib/git/index.cp)）：`GitOid`/`GitObject`（与 `git hash-object` 逐字节一致的 SHA-1）、`GitTreeCodec`/`GitCommitCodec`、完整 `.gitignore` 规则引擎及 `GitRepo.readHead`（松散/`packed-refs`/worktree），文档 [`doc/lib/git.md`](file:///data/dev/cp/doc/lib/git.md)，`-m` 5/5 PASS；
- **`cap:pdf`**（[`lib/pdf/`](file:///data/dev/cp/lib/pdf/index.cp)）：零依赖 PDF 1.4 多页文档生成 `PdfDocument`/`PdfPage`（Base14 字体、矢量绘图、10 位精确 `xref` 偏移）与 `PdfInspector`（版本/页数/Info 元数据提取），文档 [`doc/lib/pdf.md`](file:///data/dev/cp/doc/lib/pdf.md)，`-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-21] - 新建 `cap:oauth` OAuth 2.0 & RFC 7636 PKCE S256 认证客户端原语（5/5 PASS，自动动态挂载 `cap.oauth`）

- 新建 [`lib/oauth/`](file:///data/dev/cp/lib/oauth/index.cp)（`pkce.cp`、`flow.cp`、`index.cp`）：RFC 7636 `Pkce`（CSPRNG 43B Verifier + SHA-256 Base64URL S256 恒定时间校验 + `generateState`）、RFC 6749 `OAuthFlow`（授权 URL / Token 交换 / 刷新 / Client Credentials）、`OAuthTokenResponse` 与 `AuthHeader`（Bearer/Basic）；文档 [`doc/lib/oauth.md`](file:///data/dev/cp/doc/lib/oauth.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_oauth_production_deep.cp`](file:///data/dev/cp/cap/test/test_oauth_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:intl`（5/5）与 `cap:webhook`（5/5）双标准库落地（自动动态挂载 `cap.intl` / `cap.webhook`）

- **`cap:intl`**（[`lib/intl/`](file:///data/dev/cp/lib/intl/index.cp)）：`NumberFormatter`（美/欧式千分位、紧凑 `K/M/B/T`、序数词、百分比）、`ByteSize`（SI/IEC 格式化与 `u128` 精确防溢出解析）、`HumanDuration` 与 `RelativeTime`，文档 [`doc/lib/intl.md`](file:///data/dev/cp/doc/lib/intl.md)，`-m` 5/5 PASS；
- **`cap:webhook`**（[`lib/webhook/`](file:///data/dev/cp/lib/webhook/index.cp)）：`StandardWebhook`/`GitHubWebhook`/`StripeWebhook` HMAC-SHA256 恒定时间签名验签（含多签名密钥轮换与重放窗口保护）及 TTL 幂等去重 `IdempotencyStore`，文档 [`doc/lib/webhook.md`](file:///data/dev/cp/doc/lib/webhook.md)，`-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-21] - 新建 `cap:query`（5/5）与 `cap:email`（5/5）双标准库落地（自动动态挂载 `cap.query` / `cap.email`）

- **`cap:query`**（[`lib/query/`](file:///data/dev/cp/lib/query/index.cp)）：PostgreSQL/MySQL/SQLite 三方言 `IdentCodec` 防注入标识符转义及 `SelectBuilder`/`InsertBuilder`（批量多行 + `RETURNING`）/`UpdateBuilder`/`DeleteBuilder`，文档 [`doc/lib/query.md`](file:///data/dev/cp/doc/lib/query.md)，`-m` 5/5 PASS；
- **`cap:email`**（[`lib/email/`](file:///data/dev/cp/lib/email/index.cp)）：RFC 5322 `EmailAddress` 防 CRLF 注入、`QuotedPrintable`、`EmailBuilder`/`MimeParser`（`multipart/alternative`/`mixed` + 76 列 Base64 + RFC 2047 UTF-8）及 `SmtpCodec`（`AUTH PLAIN`/多行响应/Dot-Stuffing），文档 [`doc/lib/email.md`](file:///data/dev/cp/doc/lib/email.md)，`-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-21] - 新建 `cap:qrcode` ISO/IEC 18004 二维码编码与终端半块/SVG 渲染引擎（5/5 PASS，自动动态挂载 `cap.qrcode`）

- 新建 [`lib/qrcode/`](file:///data/dev/cp/lib/qrcode/index.cp)（`ecc.cp`、`encoder.cp`、`render.cp`、`index.cp`）：编译期 GF(256) 查表 `ReedSolomon`、ISO/IEC 18004 V1..10 Byte 模式 4 级纠错与 8 掩码评分择优 `QrCode`，以及 UTF-8 半块终端/ASCII/游程合并 SVG `QrRenderer`；文档 [`doc/lib/qrcode.md`](file:///data/dev/cp/doc/lib/qrcode.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_qrcode_production_deep.cp`](file:///data/dev/cp/cap/test/test_qrcode_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:decimal` 128 位十进制金融算术与零分钱丢失货币分账引擎（5/5 PASS，自动动态挂载 `cap.decimal`）

- 新建 [`lib/decimal/`](file:///data/dev/cp/lib/decimal/index.cp)（`decimal.cp`、`money.cp`、`index.cp`）：内置栈上 `U384` 无堆中间算术的 `Decimal`（`0.1 + 0.2 == 0.3`、科学计数法、5 种舍入含银行家舍入 `HalfEven`）与 ISO 4217 `Money`（异币种拦截、Hamilton 最大余数法 `allocateEven`/`allocateRatios`）；文档 [`doc/lib/decimal.md`](file:///data/dev/cp/doc/lib/decimal.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_decimal_production_deep.cp`](file:///data/dev/cp/cap/test/test_decimal_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:color` 色彩科学、WCAG 2.1 无障碍与终端 ANSI 配色引擎（5/5 PASS，自动动态挂载 `cap.color`）

- 新建 [`lib/color/`](file:///data/dev/cp/lib/color/index.cp)（`spaces.cp`、`wcag.cp`、`ansi.cp`、`index.cp`）：`Rgba`/`Hsl`/`Hsv`/`Oklab`/`Oklch` 双向转换与感知插值 `mixOklab`、WCAG 2.1 对比度/AA/AAA/`bestTextColor` 与 `Palette`、ANSI TrueColor/xterm-256 量化与 `stripAnsi`；文档 [`doc/lib/color.md`](file:///data/dev/cp/doc/lib/color.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_color_production_deep.cp`](file:///data/dev/cp/cap/test/test_color_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 新建 `cap:crdt` 分布式无冲突复制数据类型套件（5/5 PASS，自动动态挂载 `cap.crdt`）

- 新建 [`lib/crdt/`](file:///data/dev/cp/lib/crdt/index.cp)（`vclock.cp`、`counters.cp`、`register.cp`、`index.cp`）：`VectorClock`（因果偏序 `Before`/`After`/`Equal`/`Concurrent`）、`GCounter`/`PnCounter`（交换/结合/幂等合并）、`LwwRegister`（`(timestamp, node_id)` 决胜）与 `OrSet`（Observed-Remove 并发 add-wins）；文档 [`doc/lib/crdt.md`](file:///data/dev/cp/doc/lib/crdt.md)，同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_crdt_production_deep.cp`](file:///data/dev/cp/cap/test/test_crdt_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-21] - 3 路新标准库 `cap:cluster`/`cap:geo`/`cap:graph` 全绿落地（15/15 PASS，全部归口 `doc/lib/*.md` 并清零旧 `docs/` 目录）

- **`cap:cluster`**（[`lib/cluster/`](file:///data/dev/cp/lib/cluster/index.cp)）：虚拟节点二分查找 `ConsistentHashRing`（含物理节点去重 `getReplicaNodes`）、SWIM `incarnation` CRDT `MembershipTable` 及单调 `fencing_token` 防脑裂 `LeaderLease`，文档 [`doc/lib/cluster.md`](file:///data/dev/cp/doc/lib/cluster.md)，`-m` 5/5 PASS；
- **`cap:geo`**（[`lib/geo/`](file:///data/dev/cp/lib/geo/index.cp)）：`LatLng`/`BBox`/`Geo`（Haversine 测距/方位角/目标点投影/多边形地理围栏）、Base32 `Geohash`（1..12 位编解码 + 8 邻域）与 `SpatialIndex`（`withinRadius`/`nearestK`），文档 [`doc/lib/geo.md`](file:///data/dev/cp/doc/lib/geo.md)，`-m` 5/5 PASS；
- **`cap:graph`**（[`lib/graph/`](file:///data/dev/cp/lib/graph/index.cp)）：有向无向加权图 `Graph`（BFS/DFS/GraphRAG `kHopSubgraph`）及 `GraphAlgo`（`dijkstra`/`topologicalSort`/迭代式 `tarjanScc`/`kruskalMst`/`pageRank`），文档 [`doc/lib/graph.md`](file:///data/dev/cp/doc/lib/graph.md)，`-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-21] - 文档目录非复数化与扁平化重构：`docs/<mod>/<mod>.md` → `doc/lib/<mod>.md`

- 将原 `docs/` 统一重构为单数 `doc/`，全部 85 份标准库文档扁平化归口至 [`doc/lib/`](file:///data/dev/cp/doc/lib/)（消除 1 文件同名子目录，清理 `docs/pack/zlig.md` 拼写残留及 `docs/stdlib/` 16 份历史重复旧稿，保留 4 份独有文档并迁入 `doc/lib/`）；
- 全量同步更新 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)、[`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md) 与 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 中的文档路径引用（`docs/` 引用 0 残留）。

---

## [2026-11-21] - 新建 `cap:queue` 三堆 O(log N) 优先级/延迟/租约超时/死信任务队列引擎（5/5 PASS，自动动态挂载 `cap.queue`）

- 新建 [`lib/queue/`](file:///data/dev/cp/lib/queue/index.cp)（`job.cp`、`scheduler.cp`、`index.cp`）：`JobPriority`/`RetryPolicy`/`Job` 与 `JobQueue`（ReadyHeap/DelayedHeap/LeaseHeap 三堆 O(log N) 调度、延迟到期回迁、僵尸租约回收、指数退避重试、DLQ 死信与 `requeueDeadLetter`）；同步 [`docs/queue/queue.md`](file:///data/dev/cp/docs/queue/queue.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_queue_production_deep.cp`](file:///data/dev/cp/cap/test/test_queue_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:kv` 零 C 依赖 CRC32 WAL + Bitcask 崩溃安全持久化嵌入式存储（5/5 PASS，自动动态挂载 `cap.kv`）

- 新建 [`lib/kv/`](file:///data/dev/cp/lib/kv/index.cp)（`wal.cp`、`store.cp`、`index.cp`）：`WalCodec`（`"CPKV"` + IEEE CRC32 校验 + `TornTail`/`CorruptRecord` 区分）与 `KvStore`（`open`/`put`/`get`/`delete`/`prefixScan`/`sync`、模拟崩溃尾部 `ftruncate` 自动恢复、`renameat2` 原子 `compact()` 回收死记录）；同步 [`docs/kv/kv.md`](file:///data/dev/cp/docs/kv/kv.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_kv_production_deep.cp`](file:///data/dev/cp/cap/test/test_kv_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:expr`（5/5）与 `cap:protobuf`（5/5）双标准库落地（自动动态挂载 `cap.expr` / `cap.protobuf`）

- **`cap:expr`**（[`lib/expr/`](file:///data/dev/cp/lib/expr/index.cp)）：`ExprValue`/`ExprEnv` 只读点分变量上下文、爬山法优先级求值器（三元/短路 `&&`/`||`/`in`/9 个内置纯函数）及 `MAX_DEPTH=64`/`MAX_STEPS=10000` 沙盒防护，[`test_expr_production_deep.cp`](file:///data/dev/cp/cap/test/test_expr_production_deep.cp) `-m` 5/5 PASS；
- **`cap:protobuf`**（[`lib/protobuf/`](file:///data/dev/cp/lib/protobuf/index.cp)）：Proto3 零拷贝 `ProtoWriter`/`ProtoReader`（Varint/ZigZag/Packed Repeated/`skipField` 与 11B 溢出/保留区防护）及 `GrpcFrameCodec`/`GrpcFrameStream`/`GrpcStatus`/`GrpcTimeout`，[`test_protobuf_production_deep.cp`](file:///data/dev/cp/cap/test/test_protobuf_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 新建 `cap:cbor` RFC 8949 CBOR & MessagePack 紧凑二进制序列化引擎（5/5 PASS，自动动态挂载 `cap.cbor`）

- 新建 [`lib/cbor/`](file:///data/dev/cp/lib/cbor/index.cp)（`value.cp`、`cbor.cp`、`msgpack.cp`、`index.cp`）：共享 `BinValue`/`BinDocument` AST，完整支持 RFC 8949 CBOR Major 0-7（含 Float16/32/64、语义 Tag 与不定长编解码）及 MessagePack 全类型编解码，内置 64 层深度与截断/尾随字节防护；同步 [`docs/cbor/cbor.md`](file:///data/dev/cp/docs/cbor/cbor.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_cbor_production_deep.cp`](file:///data/dev/cp/cap/test/test_cbor_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:xml`（5/5）与 `cap:metrics`（5/5）双标准库落地（自动动态挂载 `cap.xml` / `cap.metrics`）

- **`cap:xml`**（[`lib/xml/`](file:///data/dev/cp/lib/xml/index.cp)）：XML 1.0 DOM 解析（声明/注释/CDATA/自闭合/命名空间/实体解码与 128 层防护）、`findByPath`/`findAllChildren`/`innerText` 查询及紧凑/美化 `XmlWriter` 与 `XmlSerializer`，[`test_xml_production_deep.cp`](file:///data/dev/cp/cap/test/test_xml_production_deep.cp) `-m` 5/5 PASS；
- **`cap:metrics`**（[`lib/metrics/`](file:///data/dev/cp/lib/metrics/index.cp)）：无锁原子 `Counter`、CAS `f64` `Gauge`、累计桶 `MetricsHistogram`（含 `estimateQuantile`）及 `MetricsRegistry`（Prometheus 0.0.4 族聚合/转义导出与 JSON 快照），[`test_metrics_production_deep.cp`](file:///data/dev/cp/cap/test/test_metrics_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 新建 `cap:text` AI RAG 分块、模糊匹配与 LLM Token 预算引擎（5/5 PASS，自动动态挂载 `cap.text`）

- 新建 [`lib/text/`](file:///data/dev/cp/lib/text/index.cp)（`chunker.cp`、`fuzzy.cp`、`tokens.cp`、`index.cp`）：UTF-8 零截断递归/Markdown 章节分块（自动跳过围栏代码块）、Unicode 码点级 Levenshtein/Damerau-Levenshtein/Jaro-Winkler 与 `fuzzyTopK`、快速 LLM `TokenEstimator` 与 UTF-8 安全截断；同步 [`docs/text/text.md`](file:///data/dev/cp/docs/text/text.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_text_production_deep.cp`](file:///data/dev/cp/cap/test/test_text_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:vector` 本地 SIMD 向量索引 + Okapi BM25 + RRF 混合 RAG 搜索引擎（5/5 PASS）

- 新建 [`lib/vector/`](file:///data/dev/cp/lib/vector/index.cp)（`flat_index.cp`、`bm25.cp`、`hybrid.cp`、`index.cp`）：复用 `cap:math` SIMD 算子的 `VectorIndex`（Cosine/Dot/Euclidean + O(1) Swap-Remove）、`Bm25Index`（Okapi BM25 词频饱和与平滑 IDF）及 `RrfFusion`（Reciprocal Rank Fusion 混合检索去重提权）；同步 [`docs/vector/vector.md`](file:///data/dev/cp/docs/vector/vector.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_vector_production_deep.cp`](file:///data/dev/cp/cap/test/test_vector_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:fsm`（5/5）与 `cap:cache`（5/5）双标准库落地（自动动态挂载 `cap.fsm` / `cap.cache`）

- **`cap:fsm`**（[`lib/fsm/`](file:///data/dev/cp/lib/fsm/index.cp)）：确定性 `StateMachine`（审计历史 + `toMermaid`）、三态 `CircuitBreaker`（`Closed`/`Open`/`HalfOpen` + 探针限流与统计）及 `WorkflowDag`（Kahn 分层并行 `executionWaves`、环路拦截、`readyNodes` 与上游失败 BFS 级联 `Skipped`），[`test_fsm_production_deep.cp`](file:///data/dev/cp/cap/test/test_fsm_production_deep.cp) `-m` 5/5 PASS；
- **`cap:cache`**（[`lib/cache/`](file:///data/dev/cp/lib/cache/index.cp)）：O(1) 槽位池 `LruCache<V>`（TTL + `evictExpired` + `CacheStats`）、最优 `m`/`k` 双哈希 `BloomFilter`（含 `unionWith`）及无低估 `CountMinSketch`（含 `merge`），[`test_cache_production_deep.cp`](file:///data/dev/cp/cap/test/test_cache_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 3 路新标准库 `cap:math`/`cap:schema`/`cap:rpc` 全绿落地与子智能体分级模型选型固化

- **分级模型选型固化**（[`GEMINI.md`](file:///data/dev/cp/GEMINI.md)）：明确常规 3 路标准库并行子智能体使用 `Model: "flash"`（Gemini 4 Hydrogen），疑难 `cpc` 编译器热修子智能体使用 `Model: "inherit"`（继承主控模型），修完即收；
- **`cap:math`**（[`lib/math/`](file:///data/dev/cp/lib/math/index.cp)）：`Vec2/3/4` 与列主序 `Mat4`、Welford 单遍统计/分位数/皮尔逊相关与线性回归、8-lane `@Vector(8, f32)` SIMD 向量相似度、`softmaxInPlace`、`topKIndices` 及对称 Int8 量化，[`test_math_production_deep.cp`](file:///data/dev/cp/cap/test/test_math_production_deep.cp) `-m` 5/5 PASS；
- **`cap:schema`**（[`lib/schema/`](file:///data/dev/cp/lib/schema/index.cp)）：JSON Schema Draft 2020-12 流式 `Schema` 与 `Schema.fromJson`（兼容 OpenAI/MCP Tool Calling）、`SchemaValidator`（5 种内置格式、`required`/`additionalProperties`/`uniqueItems`/64 层深度防护与 RFC 6901 JSON Pointer 路径诊断），[`test_schema_production_deep.cp`](file:///data/dev/cp/cap/test/test_schema_production_deep.cp) `-m` 5/5 PASS；
- **`cap:rpc`**（[`lib/rpc/`](file:///data/dev/cp/lib/rpc/index.cp)）：JSON-RPC 2.0 编解码（`JsonRpcCodec`）、LSP/MCP `ContentLengthFramer` 与 `LineRpcFramer` 增量分帧及 `McpServer` 全链路分发器（`initialize`/`ping`/`tools/list`/`tools/call`/`resources/list`），[`test_rpc_production_deep.cp`](file:///data/dev/cp/cap/test/test_rpc_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 新建 `cap:template` Mustache 兼容模板引擎（5/5 PASS，自动动态挂载 `cap.template`）

- 新建 [`lib/template/`](file:///data/dev/cp/lib/template/index.cp)（`value.cp`、`engine.cp`、`index.cp`）：支持 `TemplateValue`/`TemplateContext` 点分路径与父子作用域栈、`{{var}}` HTML 转义与 `{{{raw}}}`/`{{&raw}}`/`renderRaw`、`{{#section}}`/`{{^inverted}}`/`{{! comment}}`/`{{> partial}}`（含最大递归深度防护与独立行空白修剪）；同步 [`docs/template/template.md`](file:///data/dev/cp/docs/template/template.md) 与 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，[`test_template_production_deep.cp`](file:///data/dev/cp/cap/test/test_template_production_deep.cp) `-m` 5/5 PASS。

---

## [2026-11-20] - 新建 `cap:csv`（5/5）与 `cap:uuid`（5/5）双标准库落地（零改动编译器自动动态挂载 `cap.csv` / `cap.uuid`）

- **`cap:csv`**（[`lib/csv/`](file:///data/dev/cp/lib/csv/index.cp)）：RFC 4180 增量 `StreamingCsvReader`（跨 chunk BOM/引号换行/双引号转义/列校验/`max_record_bytes` 防护）、`CsvWriter`（CSV/TSV 与公式注入防护）及 `CsvTable`（表头索引、过滤与 `toJsonLines` NDJSON 导出），[`test_csv_production_deep.cp`](file:///data/dev/cp/cap/test/test_csv_production_deep.cp) `-m` 5/5 PASS；
- **`cap:uuid`**（[`lib/uuid/`](file:///data/dev/cp/lib/uuid/index.cp)）：RFC 9562 `Uuid`（v4 CSPRNG、v5 SHA-1 命名空间、v7 同毫秒单调自旋锁可排序）、`Ulid`（Crockford Base32 26 字符含 `I/L->1` `O->0` 与 UUID 互转）及 `NanoId`（无偏拒绝采样），[`test_uuid_production_deep.cp`](file:///data/dev/cp/cap/test/test_uuid_production_deep.cp) `-m` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)。

---

## [2026-11-20] - 3 路正交并行首批收官：`cap:bench`（9/9）、`cap:security`（9/9）与 `cap:dns`（10/10）全绿验收落地

- **`cap:bench`**（[`lib/bench/`](file:///data/dev/cp/lib/bench/index.cp)）：升级 `suite.cp`（`blackBox<T>`、`BenchOptions`/`BenchResult`/`BenchRunner` 预热与 Welford 分位数统计），新增 `compare.cp`（Welch t-test `BenchComparison` 与 Markdown 报告）及 `alloc_tracker.cp`（`CountingAllocator`/`AllocSnapshot`），[`test_bench_production_deep.cp`](file:///data/dev/cp/cap/test/test_bench_production_deep.cp) `-m` 5/5 + 既有 4/4 PASS；
- **`cap:security`**（[`lib/security/`](file:///data/dev/cp/lib/security/index.cp)）：新增 `totp.cp`（RFC 4648 Base32、RFC 4226 HOTP、RFC 6238 TOTP 与 `otpauth://`）、`cors.cp`（Fetch CORS 策略引擎与凭据+通配符防线）及 `csp.cp`（`CspBuilder` 防 CRLF 注入 + `SecurityHeadersPreset`），[`test_security_production_deep.cp`](file:///data/dev/cp/cap/test/test_security_production_deep.cp) `-m` 5/5 + 既有 `test_security_csrf.cp` 4/4 PASS；
- **`cap:dns`**（[`lib/dns/`](file:///data/dev/cp/lib/dns/index.cp)）：新增 `records.cp`（16 跳防环 `DnsNameCodec` + IPv4/IPv6 PTR、SOA/SRV+权重排序/CAA）、`doh.cp`（RFC 8484 Base64URL GET/POST 与 JSON DoH 解析）及 `resolv_conf.cp`（POSIX `/etc/resolv.conf` 与 `ndots` 搜索域展开），[`test_dns_production_deep.cp`](file:///data/dev/cp/cap/test/test_dns_production_deep.cp) `-m` 5/5 + 既有 `test_crypto_sql_dns.cp` 5/5 PASS；同步 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)（零读写 `cp_ref.md`/`cp_agent_ref.md`）。

---

## [2026-11-20] - 研发提速治理固化（`GEMINI.md` / `cp_lib_ref.md` / `verify_module.sh` / 动态 `cap` 路由）与 `cap:html` + `cap:json` 双路落地

### 研发提速体系与三轨解耦规范落地
- **剥离标准库独立契约手册**：新建 [`cp_lib_ref.md`](file:///data/dev/cp/cp_lib_ref.md)，从 [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md) 第十二章剥离全部内置库索引；后续新增/吸收标准库只改 `cp_lib_ref.md` 与 `docs/<mod>/<mod>.md`，日常不读写 `cp_ref.md`；
- **固化提速与治理铁律**：更新 [`GEMINI.md`](file:///data/dev/cp/GEMINI.md) 消除旧双轨冲突条款，固化三轨解耦规范（`cp_agent_ref.md` 语法修改打 `[SYNC_PENDING]` 标记）、96 核 3 路正交并行、子智能体“最后一步导出 `index.cp`”原子发布纪律与三段式极速调试；
- **工程提速脚本与原子部署**：新增 [`scripts/verify_module.sh`](file:///data/dev/cp/scripts/verify_module.sh) 一键合规与 `-m` 验收脚本；升级 [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh) 为临时文件 + `mv -f` 原子覆盖部署；升级 [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) 支持 `lib/<ns>/index.cp` 自动动态探测挂载（新增标准库 0 改动 Rust、免 45s 重编）。

### `cap:html` HTML5 实体/白名单清洗/AI 网页提取升级 (`lib/html/`，5/5 PASS)
- 新增 [`lib/html/entities.cp`](file:///data/dev/cp/lib/html/entities.cp)（206 个 WHATWG 命名实体二分查找、十/十六进制数字引用、C1 Windows-1252 替换与 U+FFFD 兜底）、[`lib/html/sanitizer.cp`](file:///data/dev/cp/lib/html/sanitizer.cp)（白名单清洗、`on*` 剥离、`javascript:`/`vbscript:`/`data:*` 与实体编码绕过拦截）与 [`lib/html/extractor.cp`](file:///data/dev/cp/lib/html/extractor.cp)（AI Agent 单遍流式提取 title/meta/OG/canonical/links/headings 与干净纯文本）；新增 [`cap/test/test_html_production_deep.cp`](file:///data/dev/cp/cap/test/test_html_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有回归 4/4 PASS）。

### `cap:json` RFC 6901/6902/7396/8785 与 NDJSON 流式引擎升级 (`lib/json/`，5/5 PASS)
- 新增 [`lib/json/pointer.cp`](file:///data/dev/cp/lib/json/pointer.cp)（RFC 6901 `JsonPointer`、RFC 6902 `JsonPatch` 原子应用、RFC 7396 `JsonMergePatch`）、[`lib/json/ndjson.cp`](file:///data/dev/cp/lib/json/ndjson.cp)（跨 chunk `NdjsonReader` 含 `LineTooLong` 防护与 `NdjsonWriter`），升级 [`lib/json/serializer.cp`](file:///data/dev/cp/lib/json/serializer.cp)（RFC 8785 Canonical 排序序列化与 HTML 安全转义）；新增 [`cap/test/test_json_rfc_production_deep.cp`](file:///data/dev/cp/cap/test/test_json_rfc_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有回归 9/9 PASS）。

---

## [2026-11-20] - `cap:http` Web/AI Streaming 原语（+1,713 行）与 `cap:logger` 结构化日志/W3C Trace 追踪（+1,671 行）双路落地

### `cap:http` 生产级 Web / AI Streaming 协议原语升级 (`lib/http/`，0 `std.posix`，0 `@import("bun")`)
- 新增 [`lib/http/sse.cp`](file:///data/dev/cp/lib/http/sse.cp)（486 行）：WHATWG Server-Sent Events / LLM 流式解析器与编码器（`SseEvent`、`SseParser` 跨 chunk 切分/BOM 剥离/多行 `data:` 拼接/`[DONE]` 识别/单事件大小 DoS 上限防护、`SseEncoder`）；
- 新增 [`lib/http/multipart.cp`](file:///data/dev/cp/lib/http/multipart.cp)（417 行）：RFC 7578 `multipart/form-data` 二进制安全打包与解析器（`MultipartBoundary`、`MultipartBuilder`、`MultipartParser`）；
- 升级 [`lib/http/cookie.cp`](file:///data/dev/cp/lib/http/cookie.cp)（810 行）：保留原有 `CookieMap` 兼容，新增 RFC 6265 `SameSite`/`HttpCookie`/`CookieCodec`（`Set-Cookie` 与 IMF-fixdate 编解码）及 `HttpCookieJar`（域名后缀与公共后缀边界防护、路径前缀匹配、`Secure`/过期过滤）；
- 修复 [`lib/http/decompressor.cp`](file:///data/dev/cp/lib/http/decompressor.cp) 模块化模式缺失 `WINDOW_BITS_AUTO` 显式导入，打通 `cap:http` 全量 `-m` 编译；增量导出至 [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp)，更新 [`docs/http/http.md`](file:///data/dev/cp/docs/http/http.md)；新增 [`cap/test/test_http_web_primitives_deep.cp`](file:///data/dev/cp/cap/test/test_http_web_primitives_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有 HTTP 回归 14/14 零回归）。

### `cap:logger` 结构化日志与 W3C Trace Context / OpenTelemetry 追踪升级 (`lib/logger/`，0 `std.posix`，0 `@import("bun")`)
- 新增 [`lib/logger/structured.cp`](file:///data/dev/cp/lib/logger/structured.cp)（713 行）：`StructuredLogger`/`LogFormatter`（RFC 8259 JSON 转义、Logfmt、PrettyAnsi）、`withField` 子 Logger、可插拔 Sink 及不区分大小写敏感字段自动脱敏（`password`/`token`/`authorization`/`secret` -> `"[REDACTED]"`）；
- 新增 [`lib/logger/trace.cp`](file:///data/dev/cp/lib/logger/trace.cp)（691 行）：W3C Trace Context Level 1 `TraceParent`（严格校验/全零拒绝/子 Span 派生）、`TraceState` 及 `Span`/`SpanEvent`/`SpanStatus`（含 OTel JSON 导出）；
- 新增 [`lib/logger/sampler.cp`](file:///data/dev/cp/lib/logger/sampler.cp)（267 行）：`RatioSampler`（FNV-1a+SplitMix64 确定性采样）、`BurstSampler`（防日志风暴降采样）与零堆 `RingLogBuffer<CAP>`；
- 增量导出至 [`lib/logger/index.cp`](file:///data/dev/cp/lib/logger/index.cp)，更新 [`docs/logger/logger.md`](file:///data/dev/cp/docs/logger/logger.md)；新增 [`cap/test/test_logger_production_deep.cp`](file:///data/dev/cp/cap/test/test_logger_production_deep.cp)（单文件与 `-m` 均 5/5 PASS，既有 `test_shell_napi_elf_logger_deep.cp` 4/4 零回归）。
