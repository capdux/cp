# CP 标准库：沙盒安全表达式与规则策略求值引擎手册 (`cap:expr` / `cap.expr`)

## 一、 架构总览与安全模型

`cap:expr` 是 CP 语言官方内置的纯原生零副作用沙盒表达式求值与规则策略引擎，专为 **AI Agent 安全护栏（Guardrails）**、**特性开关（Feature Flags）**、**RBAC/ABAC 权限策略** 与 **动态数据过滤** 打造。

全模块 **零 C 依赖、零 `std.posix`、零 `@import("bun")`**，严格保证：
- **只读零副作用上下文 (`ExprEnv`)**：表达式内部无法修改外部状态，禁止赋值操作（`=` 触发 `SyntaxError`）；
- **安全算术边界**：整型/浮点除法与取模强制校验零除数（`DivisionByZero`），整数运算防溢出崩溃；
- **严格深度与计算步数防护**：括号/列表/调用/一元最大嵌套深度 `MAX_DEPTH = 64`（超限返回 `MaxDepthExceeded`），单次求值最大步数 `MAX_STEPS = 10000`（超限返回 `StepLimitExceeded`），彻底杜绝栈溢出与恶意算力耗尽；
- **严格短路求值**：`&&`、`||` 与三元 `cond ? a : b` 对未命中分支只做语法与深度步数校验、不执行运行时求值。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/expr/value.cp` | `// Source: cp/lib/expr/value.cp (Sandboxed Expression Value & Environment)` | `ExprError`、`ExprValue`（构造/`isTruthy`/`eql`）、`ExprEnv` |
| `lib/expr/eval.cp` | `// Source: cp/lib/expr/eval.cp (Precedence-Climbing Sandboxed Expression Evaluator)` | 词法扫描、爬山法求值器、9 个纯内置函数、深度与步数护栏 |
| `lib/expr/index.cp` | `// Source: cp/lib/expr/index.cp (Unified Exports for cap:expr)` | `cap:expr` 与 `cap.expr` 统一公开导出入口 |

---

## 二、 核心 API 与语法契约

### 1. 错误类型 (`ExprError`)

- `SyntaxError`：词法/语法错误、非法赋值 `=`、未闭合括号/字符串
- `TypeMismatch`：不支持的操作数类型组合或数值溢出
- `DivisionByZero`：整型/浮点 `/` 或 `%` 除数为 `0`
- `UnknownIdentifier`：引用未在 `ExprEnv` 中绑定的变量或点分路径
- `UnknownFunction`：调用非白名单内置函数
- `InvalidArity`：内置函数实参数量不匹配
- `MaxDepthExceeded`：嵌套深度超过 `MAX_DEPTH` (`64`)
- `StepLimitExceeded`：求值步数超过 `MAX_STEPS` (`10000`)
- `OutOfMemory`：内存分配失败

### 2. 动态值 (`ExprValue`) 与环境 (`ExprEnv`)

- **`ExprValue`**：`Null`、`Bool(bool)`、`Int(i64)`、`Float(f64)`、`Str(string)`、`List([]ExprValue)`
  - 工厂方法：`makeNull()`、`makeBool(v)`、`makeInt(v)`、`makeFloat(v)`、`makeStr(v)`、`makeList(items)`
  - 语义方法：`isTruthy(): bool`、`eql(other: ExprValue): bool`、`isNull()`/`asBool()`/`asInt()`/`asFloat()`/`asNumber()`/`asStr()`/`asList()`
- **`ExprEnv`**：
  - 生命周期：`init(alloc): ExprEnv`、`deinit(*_)`、`clear(*_)`
  - 绑定写入：`set(*_, key, value): !`、`setNull`、`setBool`、`setInt`、`setFloat`、`setStr`、`setList`（支持顶层变量名与点分路径如 `"user.role"`、`"request.tokens"`、`"env.region"`）
  - 只读查询：`get(key): ExprValue?`、`has(key): bool`、`len(): usize`

### 3. 表达式语法与内置纯函数

- **字面量**：`null`、`true`、`false`、十进制/十六进制(`0x`)/二进制(`0b`)/八进制(`0o`) 整数、浮点数、单/双引号字符串（支持 `\n`, `\r`, `\t`, `\\`, `\"`, `\'`, `\0`, `\xHH` 转义）、列表 `[1, "a", true]`
- **运算符优先级（由低到高）**：
  1. 三元条件：`cond ? a : b`（右结合，惰性分支）
  2. 短路逻辑或：`||`
  3. 短路逻辑与：`&&`
  4. 相等比较：`==`、`!=`
  5. 序比较与成员包含：`<`、`<=`、`>`、`>=`、`in`（支持元素在列表中及子串在字符串中）
  6. 加减：`+`（数值加法与字符串拼接）、`-`
  7. 乘除模：`*`、`/`、`%`（严格防除零）
  8. 一元前缀：`-`、`!`
  9. 括号 `(...)`、字面量、标识符/点分路径、函数调用 `fn(...)`
- **内置纯函数**：
  - `len(x)`：字符串字节长度或列表元素个数
  - `contains(s, sub)`：字符串子串包含或列表成员包含
  - `startsWith(s, pre)` / `endsWith(s, suf)`：前缀/后缀匹配
  - `lower(s)` / `upper(s)`：ASCII 大小写转换
  - `abs(n)`：整数或浮点数绝对值
  - `min(a, b)` / `max(a, b)`：数值或字符串极值

### 4. 求值入口 (`ExprEvaluator`)

- `ExprEvaluator.eval(alloc, source, env): ExprValue!`（建议传入 `ArenaAllocator` 以便统一释放结果字符串/列表）
- `ExprEvaluator.evalBool(alloc, source, env): bool!`（内部自动使用临时 Arena，零泄漏直接返回 `bool`）

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { ExprEnv, ExprEvaluator } from "cap:expr";

pub fn checkAgentGuardrail(): !bool {
    const alloc = std.heap.page_allocator;
    let env = ExprEnv.init(alloc);
    defer env.deinit();

    env.setStr("user.role", "analyst")?;
    env.setInt("request.tokens", 2048)?;
    env.setStr("env.region", "us-east")?;
    env.setStr("request.prompt", "Summarize quarterly metrics")?;

    const policy = "user.role in [\"admin\", \"analyst\"] && request.tokens <= 4096 && !contains(lower(request.prompt), \"ignore previous\")";
    return cap.expr.ExprEvaluator.evalBool(alloc, policy, env);
}
```
