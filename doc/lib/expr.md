# CP 标准库：安全确定性表达式与公式求值引擎 (`cap:expr` / `cap.expr`)

## 一、 架构总览与安全模型

`cap:expr` 是 CP 语言官方内置的纯 CP 零副作用沙盒表达式与公式求值引擎，专为 **AI Agent 安全护栏**、**特性开关（Feature Flags）**、**动态计费/配额公式** 与 **规则策略过滤** 打造。

全模块 **零 C 依赖、零 `std.posix`、零 `@import("bun")`**，严格保证：
- **只读零副作用上下文 (`ExprContext`)**：表达式内部无法修改外部状态；
- **安全算术边界**：除法 `/` 与取模 `%` 严格校验零除数（`DivisionByZero`）；
- **严格递归深度防护**：默认最大嵌套深度 `DEFAULT_MAX_DEPTH = 64`（可通过 `compileWithMaxDepth` 自定义），解析期与求值期双重拦截超深嵌套并返回 `MaxDepthExceeded`，彻底杜绝栈溢出；
- **严格短路与惰性求值**：`&&`、`||` 与三元条件 `cond ? a : b` 仅求值命中的分支，未命中分支不会触发除零或未定义变量错误。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/expr/expr.cp` | `// Source: cp/lib/expr/expr.cp (Safe Deterministic Expression & Formula Evaluator)` | `ExprError`、`ExprValue`、`ExprContext`、`CompiledExpr`、词法/递归下降解析器与求值引擎 |
| `lib/expr/index.cp` | `// Source: cp/lib/expr/index.cp (Unified Exports for cap:expr)` | `cap:expr` 与 `cap.expr` 统一公开导出入口 |

---

## 二、 核心 API 与语法契约

### 1. 错误类型 (`ExprError`)

- `UnexpectedToken`：非法词法符号或语法不匹配
- `UnexpectedEof`：未闭合括号、未闭合字符串或意外输入结束
- `DivisionByZero`：`/` 或 `%` 的右操作数为 `0`
- `UndefinedVariable`：引用未在 `ExprContext` 中绑定的变量或点分路径
- `UnknownFunction`：调用非白名单内置函数
- `InvalidArity`：内置函数参数数量不匹配
- `TypeMismatch`：不支持的操作数类型组合（如 `"abc" + 1`、`true < false`、`clamp` 中 `lo > hi`）
- `MaxDepthExceeded`：嵌套深度超过 `max_depth`（默认 `64`）
- `OutOfMemory`：内存分配失败

### 2. 动态值 (`ExprValue`) 与环境 (`ExprContext`)

- **`ExprValue`**（tagged union）：
  - `number: f64`
  - `boolean: bool`
  - `string: []const u8`
  - `null_val`（void）
  - 辅助构造与访问：`makeNumber(v)`、`makeBool(v)`、`makeString(v)`、`makeNull()`、`isNull()`、`asNumber()`、`asBool()`、`asString()`、`eql(other)`
- **`ExprContext`**：
  - 生命周期：`init(allocator): ExprContext`、`deinit(*_)`
  - 绑定写入：`setNumber(*_, name, v): !void`、`setBool(*_, name, v): !void`、`setString(*_, name, v): !void`、`setNull(*_, name): !void`（支持点分路径如 `"user.score"`）
  - 只读查询：`get(name): ExprValue?`

### 3. 表达式语法与内置纯函数

- **字面量**：整数/浮点数（含科学计数法如 `1.5e-3`）、双引号/单引号字符串（支持 `\n`、`\r`、`\t`、`\\`、`\"`、`\'`、`\0`、`\xHH` 转义）、`true`、`false`、`null`、标识符（支持下划线与点分路径如 `user.tier`）
- **运算符优先级（由低到高）**：
  1. 三元条件：`cond ? a : b`（惰性分支求值）
  2. 逻辑或：`||`（短路求值）
  3. 逻辑与：`&&`（短路求值）
  4. 相等比较：`==`、`!=`
  5. 序比较：`<`、`<=`、`>`、`>=`（支持数值与字符串）
  6. 加减：`+`（数值加法与字符串拼接）、`-`
  7. 乘除模：`*`、`/`、`%`（严格防除零）
  8. 一元前缀：`!`、`-`
  9. 括号 `(...)`、字面量、标识符、函数调用 `fn(...)`
- **内置纯函数**：
  - `min(...)` / `max(...)`：1 个及以上数值参数极值
  - `abs(x)` / `floor(x)` / `ceil(x)` / `round(x)`：数值单目数学函数
  - `clamp(x, lo, hi)`：将 `x` 限制在 `[lo, hi]` 区间
  - `len(s)`：字符串长度（返回 `number`）
  - `contains(haystack, needle)`：子串包含判断
  - `startsWith(s, prefix)`：前缀匹配判断

### 4. 编译与求值入口

- `compile(allocator, source): !CompiledExpr`（默认 `max_depth = 64`）
- `compileWithMaxDepth(allocator, source, max_depth): !CompiledExpr`
- `CompiledExpr.eval(ctx): !ExprValue` & `CompiledExpr.deinit(*_)`
- `eval(source, ctx): !ExprValue`（一次性编译并求值）

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { ExprContext, compile, eval } from "cap:expr";

pub fn evaluateRule(): !bool {
    const alloc = std.heap.page_allocator;
    let ctx = ExprContext.init(alloc);
    defer ctx.deinit();

    ctx.setNumber("user.score", 88.5)?;
    ctx.setString("user.tier", "pro")?;
    ctx.setBool("user.active", true)?;

    let compiled = compile(alloc, "user.active && clamp(user.score, 0, 100) >= 80 && startsWith(user.tier, 'pr')")?;
    defer compiled.deinit();

    const res = compiled.eval(ctx)?;
    return res.asBool() ?? false;
}
```
