# CP 编译器 (`cpc`) 演进升级技术规格书 (`CPC_UPGRADE_SPEC.md`)

- **目标执行体**：上游 `cp agy`（负责 `/data/dev/cp/cpc/src` 编译器核心演进）
- **设计目标**：在不破坏现有 CP 纯化语法体系与零开销原则的前提下，彻底消除历史 AST Lowering 限制、大幅降低下游智能体（`bun_agy` / `capsurf_agy`）的代码生成 Token 消耗与排障步数，并补齐面向百万级 AI Agent 与 Nest 容器高并发场景的杀手级语言特性。
- **实施原则**：分三个阶段（Phase 1 ~ Phase 3）推进；每完成一项特性，必须补充对应的编译器单元测试（`cpc/tests/`）并确保全量存量 `.cp` 代码零回归。

---

## 目录：12 项核心升级全景清单

- **Phase 1：消除历史 Lowering 限制与 `cpc fix` 语义级自愈（立竿见影，最高优先级）**
  - **Spec 1**：二级嵌套子结构体/枚举与复杂常量自动提升（Auto-Hoisting）与单参 `.slice(arg)` 消歧
  - **Spec 2**：`cpc fix` 升级为语义级（Sema-Driven）AST 自愈引擎（`E0130` ~ `E0135`）
- **Phase 2：高频人体工程学语法糖（极致节省 AI Output Tokens）**
  - **Spec 3**：`const/let ... := opt else ...` 守卫解包语法（Guard Unwrapping）
  - **Spec 4**：原生可选链（`?.`）与空值合并赋值（`??=`）
  - **Spec 5**：一等公民级 UTF-8 字符串内插模板（`` `...${expr}...` ``）
  - **Spec 6**：结构化编译期派生与序列化控制（`@derive` 与 `@json`）
- **Phase 3：面向百万 Agent 与 Nest 云原生运行时的系统级黑科技**
  - **Spec 7**：语言级一等公民 `@arena` 作用域内存区域（Scoped Arena Regions）
  - **Spec 8**：编译期跨协程/异步边界安全检查（`Send` / `Isolated` 零悬垂守卫）
  - **Spec 9**：`@is_nest` 编译期 Reductions 抢占式循环回边与函数入口自动插桩
  - **Spec 10**：原生 SoA（Struct of Arrays）数据布局转换修饰符 `@soa`
  - **Spec 11**：原生 SIMD 向量化切片循环与内建向量类型（`@simd`）
  - **Spec 12**：编译期契约式设计、消除边界检查与内建 Fuzzing 块（`@requires` / `@ensures` / `fuzz`）

---

## Phase 1：消除历史 Lowering 限制与 `cpc fix` 语义级自愈

### Spec 1：嵌套子类型自动提升（Auto-Hoisting）与 `.slice(arg)` 方法调用消歧

#### 1.1 背景与痛点

当前 `cpc` 在处理 `@root struct`、`struct` 或 `enum` 时，若内部嵌套了二级子结构体（如 `pub struct Sub { ... }`）或含 CP 语法的复杂 `const` 块，Codegen 会因未递归执行完整顶层 Lowering 而报错，迫使下游智能体耗费大量步数手动将子结构体剪切提升到文件顶层。同时，自定义类型的单参数方法 `obj.slice(arg)` 会被词法/语法分析器误判为数组切片 `obj[arg..]`。

#### 1.2 编译器改造规范

1. **AST 预处理 Pass（`hoist_nested_decls`）**：
   - 在 `Parser` 生成 AST 后、`Codegen` 之前，遍历所有 `@root struct Name`、`struct Name`、`enum Name`、`union(enum) Name` 节点。
   - 若发现内部包含嵌套的 `struct Sub`、`enum Sub`、`union Sub` 声明：
     1. 自动将其剪切提升至当前模块 AST 顶层，并重命名为 `__Hoisted_<ParentName>_<SubName>`；
     2. 对提升后的顶层结构体完整执行标准 CP 语法 Lowering（包括默认 `pub`、`static fn`、接收者 `_`/`*_`、返回类型 `: T` 等）；
     3. 在原父结构体内部原位保留一行类型别名绑定：`pub const <SubName> = __Hoisted_<ParentName>_<SubName>;`。
   - 对嵌套在容器内的复杂 `const` 初始化表达式，强制递归调用 `lowerExpr()`，严禁原样输出。
2. **`.slice(arg)` 语义消歧**：
   - 在 `Parser` / `Sema` 中，仅当显式指定了切片语法或目标类型为原生数组/切片 `[]T` / `[*]T` 且不存在同名自定义方法时，才将 `.slice(start)` 视为切片糖；
   - 若接收者为自定义 `struct`/`enum` 或带 `.slice` 方法的实例，一律生成标准方法调用 `obj.slice(arg)`，不再误转为 `[arg..]`。

#### 1.3 输入与 Lowering 转换示例

```cp
// 输入 CP 源码：
@root struct Parser {
    pos: usize,
    struct State {
        depth: u32,
        fn reset(*_) { _.depth = 0; }
    }
    fn slice(start: usize): usize { return _.pos + start; }
}
```

```zig
// cpc Lowering 输出目标：
pub const __Hoisted_Parser_State = struct {
    depth: u32,
    pub fn reset(_: *@This()) void { _.depth = 0; }
};
// (@root struct 展开为文件级成员)
pos: usize,
pub const State = __Hoisted_Parser_State;
pub fn slice(_: *const @This(), start: usize) usize { return _.pos + start; }
```

---

### Spec 2：`cpc fix` 升级为语义级（Sema-Driven）AST 自愈引擎

#### 2.1 背景与痛点

现有 `cpc fix` 已支持 `E0101`..`E0126` 的表层词法/语法修复，但下游智能体在迁移大型代码库时，仍有约 15% 的报错集中在接收者可变性不匹配、未修改变量误用 `let`、关键字冲突未转义等语义层问题。

#### 2.2 新增自动修复诊断码与规则

- 升级 cpc Sema 双向类型推导，直接消灭 E0207 误报）：
  其实 const x: u32 = switch (tag) { .a => 0, * => 1 }; 之所以会报 E0207（把 0 默认推导成 i32），是因为目前 cpc 在推导 switch 分支表达式时没有把左值的目标类型（Expected Type u32）向下传递给各个分支。
  编译器修复规格：在 sema 检查 switch / if 表达式时，如果上下文已经明确了左值目标整数类型（如 u8、u16、u32、u64、usize），直接将该目标类型作为 Expected Type 注入所有分支的未定型整数字面量（Untyped Integer Literal），让 0 原生推导为 u32，根本不需要写任何显式类型转换！

确认该问题是否真实存在， 需要优化？

- 在 `/data/dev/cp/cap/bin/cpc fix <file.cp>` 中扩充以下 6 项基于 AST/Sema 的就地自动修复能力：

| 诊断码      | 触发场景描述                                                                       | `cpc fix` 自动重写动作                                          |
| :---------- | :--------------------------------------------------------------------------------- | :-------------------------------------------------------------- |
| **`E0130`** | 只读实例方法（省略首参）内部修改了 `_.field = ...` 或将 `_` 传给了要求 `*_` 的方法 | 自动在方法签名首参位置插入 `*_`（若已有其他形参则插入 `*_ , `） |
| **`E0131`** | 实例方法声明了可变接收者 `*_`，但整个函数体仅对 `_` 进行只读访问                   | 自动移除首参 `*_`（收窄可变性权限）                             |
| **`E0132`** | 局部变量声明为 `let x = ...`，但作用域内从未发生二次赋值或可变借用 `*x`            | 自动将 `let` 替换为 `const`                                     |
| **`E0133`** | 函数形参声明为只读 `v: T`，但在函数体内直接对 `v` 进行了重新赋值 `v = ...`         | 给出双分支 = suggestion:                                        |

- 需修改调用者数据，请将形参改为可变借用 v: *T；
- 函数内局部游标，请在首行显式声明 let v_local = v; |
  | **`E0134`** | 变量、形参或 `catch \(e\)` 载荷使用了保留字 `err`、`in`、`as`、`type`、`arena`、`macro` | 若为错误载荷 `err` 自动重命名为 `e`；若为结构体字段/枚举名自动包裹为 `@"in"`、`@"type"` 等 |
  | **`E0135`** | `switch` 分支返回字面量整数赋给无符号类型（如 `u8`/`u32`）触发 `E0207` 推导为 `i32` | 自动根據目标左值类型将分支整数字面量包裹为 `i32(<lit>)` |

---

## Phase 2：高频人体工程学语法糖（极致节省 AI Output Tokens）

### Spec 3：`const/let ... := opt else ...` 守卫解包语法（Guard Unwrapping）

#### 3.1 语法定义

支持在语句层级直接解包可空类型 `T?` 或错误联合类型 `T!`，当值为 `null` 或 `err` 时必须执行发散控制流（`return`、`break`、`continue`、`unreachable`）：

```ebnf
GuardDecl := ("const" | "let") ("*"? IDENT) ":=" Expr "else" ("|" IDENT "|")?DivergentStmtOrBlock
```

#### 3.2 代码示例与 Lowering 规则

```cp
// 1. 可空类型守卫解包
const node := doc.getNode(id) else return null;
let *mut_node := doc.getMutNode(id) else {
    log.warn("missing node");
    return Err.NodeNotFound;
};

// 2. 错误联合类型守卫解包（带错误载荷捕获）
const parsed := StyleSheet.parse(raw_css) else （e) {
    return e;
};
```

**Lowering 规则**：

- 对于 `const v := opt_expr else <divergent>`：
  直接转化为 Zig 的 `const v = opt_expr orelse <divergent>;`（指针解包 `*v` 对应 `&*` 捕获）。
- 对于错误联合类型 `const v := res_expr else (e) <divergent>`：
  直接转化为 `const v = res_expr catch |e| <divergent>;`。

---

### Spec 4：空值合并赋值（`??=`）

#### 4.1 语法定义

**空值合并赋值（Nullish Coalescing Assignment `??=`）**：

- `lhs ??= rhs`：当且仅当可空左值 `lhs` 为 `null` 时，计算 `rhs` 并赋给 `lhs`，同时返回解包后的非空值或更新原变量。

#### 4.2 Lowering 规则示例

```cp
// 输入 CP 源码：
const w = doc.getRoot()?.firstChild()?.rect.width ?? 0.0;
cache.entry ??= computeHeavyEntry();
```

```zig
// cpc Lowering 输出目标（利用带标签块实现零开销短路求值）：
const w = (__opt_blk_1: {
    const __tmp_1 = doc.getRoot() orelse break :__opt_blk_1 null;
    const __tmp_2 = __tmp_1.firstChild() orelse break :__opt_blk_1 null;
    break :__opt_blk_1 __tmp_2.rect.width;
}) orelse 0.0;

if (cache.entry == null) {
    cache.entry = computeHeavyEntry();
}
```

---

### Spec 5：一等公民级 UTF-8 字符串内插模板（`` `...${expr}...` ``）

#### 5.1 语法与类型自适应推导

支持使用反引号 `` ` `` 定义模板字符串，内部通过 `${expr}` 嵌入任意表达式（根据表达式类型自动匹配格式化占位符：字符串/切片用 `{s}`，整数/浮点用 `{d}`，布尔/枚举/结构体用 `{any}`）：

1. **逃逸分配模式**：若模板字符串赋值给变量或作为返回值，默认使用当前作用域内的 `allocator`（若处于 `@arena` 函数内则自动使用隐式 Arena；否则查找当前作用域名为 `allocator` / `_.allocator` 的分配器）：
   ```cp
   const msg = `Node #${id} at (${rect.x}, ${rect.y}) tag=${tag_name}`;
   ```
   Lowering 为：`try std.fmt.allocPrint(allocator, "Node #{d} at ({d}, {d}) tag={s}", .{ id, rect.x, rect.y, tag_name })`。
2. **零分配流直写模式（Zero-Allocation Stream Write）**：若模板字符串直接作为 `print`、`writer.write`、`log.info` 的实参，编译器直接将其展开为 `writer.print("...", .{ ... })`，**不产生任何堆内存分配**。
3. **编译期常量折叠**：若所有 `${expr}` 均为编译期常量，直接折叠为静态字符串字面量。

---

### Spec 6：结构化编译期派生与序列化控制（`@[]` 与 `@json`）

#### 6.1 语法规范

为 `struct`、`enum`、`union(enum)` 提供零运行时反射开销的编译期方法自动生成：

```cp
@[JSON, Clone, Eq, Hash, Default]
pub struct CdpBoxModel {
    @json("nodeId")
    node_id: u32,

    @json("borderBox")
    border_box: [8]f32,

    @json_ignore
    internal_cache_ptr: *!? = null,
}
```

#### 6.2 编译器 Codegen 行为

- `@Eq`：生成 `pub fn eql(_: *const @This(), other: *const @This()): bool`（递归比对所有非忽略字段/变体）。
- `@Clone`：生成 `pub fn clone(_: *const @This(), allocator: std.mem.Allocator): @This()!`（对切片/动态数组执行深拷贝）。
- `@Hash`：生成 `pub fn hash(_: *const @This(), hasher: anytype)`。
- `@JSON`：生成与全局一等公民 `JSON.stringify` / `JSON.parse<T>` 深度集成的编译期字段映射表（支持 `@json("alias")` 重命名与 `@json_ignore` 跳过字段）。

---

## Phase 3：面向运行时的系统级黑科技

### Spec 7：语言级一等公民 `@arena` 作用域内存区域（Scoped Arena Regions）

#### 7.1 设计动机

在 HTML 解析、CSS 级联、布局计算与单次 HTTP/CDP 请求处理中，90% 的临时对象生命周期与函数调用完全一致。显式层层传递 `allocator` 既繁琐又容易遗漏 `deinit`。

#### 7.2 语法与编译器实现

1. **函数级 `@arena` 修饰符**：
   ```cp
   @arena fn handleCdpRequest(raw_msg: string): string! {
       // 函数体内隐式注入局部变量 `__arena` (ArenaAllocator) 及其接口 `allocator`
       // 所有动态数组 [T] 字面量、字符串内插 `...`、JSON.parse 均自动绑定该 allocator
       const req = JSON.parse<CdpRequest>(raw_msg)?;
       return `{"id": ${req.id}, "result": {}}`;
   }
   ```
2. **逃逸返回值安全晋升（Return Value Promotion）**：
   - 在 `@arena` 块或函数退出前（通过编译器自动插入的 `defer __arena.deinit()`），若函数返回了引用类型（如 `string` 或 `[u8]`），且调用方传入了外部目标分配器或绑定了父级 Arena，编译器自动在 `deinit` 前将返回值深拷贝（Promote）至父级分配器，**从编译期根绝返回悬垂指针（UAF），同时保证内部中间产物 100% 零泄漏一键回收**。
3. **块级 `@arena { ... }`**：支持在循环内部开启局部 `@arena { ... }`，每次循环迭代结束时自动执行 `reset(.retain_capacity)`，实现百万次循环零堆碎片。

---

### Spec 8：编译期跨协程/异步边界安全检查（`Send` / `Isolated` 零悬垂守卫）

#### 8.1 语义检查规则（Sema Pass: `check_async_isolation`）

当代码调用协程创建（如 `cap:event` 的 `spawn`）、向跨协程 Channel 发送消息、或注册异步事件回调时，`cpc` 的语义分析器对传入的参数/闭包捕获变量执行静态分析：

1. **严禁跨异步边界传递非拥有借用**：若参数类型包含局部借用指针 `*T`、`&T` 或未标注所有权转移的切片，直接报编译期错误 **`E0305: Borrowed reference cannot cross async/coroutine boundary without 'move' or 'own'`**。
2. **强制所有权移交或纯值传递**：跨边界数据必须为纯值类型（POD 数值/结构体），或显式通过 `move v` / `own v: T` 移交独占所有权，确保发送方在移交后立即失效（Move Semantics），彻底杜绝多线程/多协程数据竞争与 Use-After-Free。

---

### Spec 10：原生 SoA（Struct of Arrays）数据布局转换修饰符 `@soa`

#### 10.1 语法与内存布局转换

针对 DOM 节点池（`NodeData`）、布局盒数组（`Rect`）等高密度线性扫描场景，提供 `@soa` 修饰符：

```cp
@soa struct NodeMetric {
    parent: u32,
    flags: u8,
    x: f32,
    y: f32,
}

// 声明 SoA 容器：
let metrics: @soa [NodeMetric] = [];
metrics.append({ parent: 0, flags: 1, x: 10.0, y: 20.0 });

// 访问语法保持与普通 AoS 完全一致：
const px = metrics[i].x;
```

**Lowering 规则**：

- `cpc` 自动将 `@soa [NodeMetric]` 映射为底层 `std.MultiArrayList(NodeMetric)`；
- 将 `metrics[i].x` 自动改写为 `metrics.items(.x)[i]`；
- 当只遍历其中一两个字段（如 `for (f of metrics.column(.flags))`）时，CPU 缓存行（Cache Line 64 Bytes）利用率提升 4~8 倍，且天然对齐 SIMD 寄存器。

---

### Spec 11：原生 SIMD 向量化切片循环与内建向量类型（`@simd`）

#### 11.1 语法规范

1. **内建定长向量原语**：直接支持 `u8x16`、`u8x32`、`i32x4`、`i32x8`、`f32x4`、`f32x8`（无需写 `@Vec(16, u8)`），直接支持算术运算符 `+`, `-`, `*`, `/` 与按位运算。
2. **`@simd` 循环修饰符**：
   ```cp
   // 用于 HTML Tokenizer 快速扫描字符或 Canvas 像素 Alpha 混合
   @simd for (*px of canvas.pixels) {
       px.* = px.* >> 1;
   }
   ```
   编译器自动按目标架构（x86_64 AVX2/AVX-512 或 ARM64 NEON）生成步长为 16/32 字节的向量化主循环 + 尾部标量余数循环（Scalar Epilogue）。

---

### Spec 12：编译期契约式设计、消除边界检查与内建 Fuzzing 块

#### 12.1 函数级契约标注（精简统一规范：`@pre` / `@post`）

为了极致压缩 Token 并保持工业级形式化验证标准，采用行业公认最精简的 Precondition / Postcondition 命名：

```cp
fn getNode(id: NodeId): *NodeData
    @pre(id > 0 && id < _.nodes.len)
    @post(ret.id == id)
{
    return &_.nodes[id]; // 编译器据此证明 id < _.nodes.len 恒成立，自动消除 Release 模式下的越界检查！
}
```

- **Debug / Test 模式**：在函数入口与出口自动展开为带详细源码表达式与参数值打印的 `std.debug.assert(...)`。
- **Release (`opt: "fast"`) 模式**：转化为 `@setRuntimeSafety(false)` + LLVM `assume` 优化提示，指导后端直接消除切片访问边界检查分支。

#### 12.2 原生模糊测试块（`fuzz`）

与 `test "..." { ... }` 并列，支持一级关键字 `fuzz`：

```cp
fuzz "html5 tokenizer resilience" (input: []const u8) {
    let doc = Document.init(allocator);
    defer doc.deinit();
    * = doc.parseHtml(input) catch {};
}
```

运行 `cap test --fuzz` 时，自动对接 LLVM libFuzzer / 内置覆盖率引导变异器，验证解析器在任意畸形字节流下永不 Panic、永不泄漏内存。

---

## 附：交给 `cp agy` 的实施验收标准与开发顺序

请 `cp agy` 严格按照以下顺序在 `/data/dev/cp/cpc/src` 中迭代实装，每完成一个里程碑立即通知下游智能体：

1. **Milestone A（预计 1~2 轮迭代，最优先交付）**：
   - 完成 **Spec 1**（嵌套子结构体/枚举自动提升 + `.slice(arg)` 消歧）与 **Spec 2**（`cpc fix` 新增 `E0130`~`E0135` 语义级自动修复）。
   - **验收指标**：移除 `CP_PITFALLS.md` 中关于“手动提升二级子结构体”与“手动绕开 `.slice(arg)`”的人工限制，运行 `cpc fix` 可一键修复接收者 `*_` 与 `let`->`const`。
2. **Milestone B（预计 2~3 轮迭代）**：
   - 完成 **Spec 3**（`const v := opt else return` 守卫解包）、**Spec 4**（`?.` 可选链与 `??=`）与 **Spec 5**（`` `...${expr}...` `` 字符串内插）。
   - **验收指标**：在 `cpc/tests/ergonomics_test.cp` 中覆盖全部嵌套可选链短路与守卫提前返回用例。
3. **Milestone C（系统级深化迭代）**：
   - 依次完成 **Spec 6**（`@derive`）、**Spec 7**（`@arena`）、**Spec 8**（跨协程借用隔离检查）、**Spec 9**（`@is_nest` Reductions 插桩）、**Spec 10 ~ 12**（`@soa`、`@simd`、`@requires`/`fuzz`）。

### Spec 9：`@is_nest` 编译期 Reductions 抢占式自动插桩

#### 9.1 设计动机

当 CP 代码以 `cap build -b` 编译为运行在 Nest 多租户容器中的高并发模块时，必须防止单个恶意或死循环协程饿死整个工作线程。

#### 9.2 编译器自动插桩规范

当检测到编译处于 `@is_nest` 模式时，`cpc` 在 Codegen 阶段自动对所有非 `@no_preempt` 标注的普通函数执行轻量级指令注入：

1. **TLS Reductions 计数器**：在运行时线程局部存储（`@tls`）维护 `__cp_reductions_left: i32`（默认时间片配额为 `cap.json` 中的 `reduction_slice: 2000`）。
2. **插桩位置**：
   - **所有循环回边（`while`、`for` 循环体首部）**；
   - **所有非内联递归/普通函数入口**。
3. **注入代码模板**：
   ```zig
   __cp_reductions_left -= 1;
   if (@expect(__cp_reductions_left <= 0, false)) {
       __cp_nest_yield_or_check_cancel();
   }
   ```
   利用 `@expect(..., false)`（对应 LLVM `__builtin_expect` 分支预测提示），在正常执行路径上仅占 1 个 CPU 时钟周期，而在死循环发生时可在微秒级平滑触发抢占挂起或会话终止。

---
