# `cap:cel` — Common Expression Language 安全策略求值引擎

## 1. 概述

`cap:cel`（亦可通过 `cap.cel` 动态挂载）是纯 CP 实现的工业级 Common Expression Language (CEL) 安全策略表达式编译与沙盒求值引擎。专为 IAM/RBAC 鉴权、API 网关路由过滤、配置策略校验与运行时安全沙盒设计，具备：
- 完整递归下降 AST 编译器与可配置递归深度限制 (`compileCelWithMaxDepth`)；
- 确定性无 GC 内存管理与 `CelValue` 深度克隆/释放；
- 严格类型安全、除零与有符号整数溢出拦截、越界索引拦截及步数成本上限 (`max_steps`) 防护。

## 2. 核心类型

- `CelError`: 错误集合，包含 `SyntaxError`、`MaxDepthExceeded`、`CostLimitExceeded`、`TypeMismatch`、`DivisionByZero`、`IntegerOverflow`、`IndexOutOfBounds`、`KeyNotFound`、`UndeclaredIdentifier`、`UnknownFunction`、`InvalidArgument`、`OutOfMemory`。
- `CelValue`: 动态值联合体，支持 `null_val`、`bool_val`、`int_val`、`uint_val`、`double_val`、`string_val`、`list_val`、`map_val`；提供 `deinit(alloc)`、`clone(alloc)`、`equals(other)`、`mapGet(key)`、`typeName()`。
- `CelMapEntry`: `{ key: []const u8, value: CelValue }`。
- `CelEnv`: 求值环境变量表，提供 `init()`、`deinit(alloc)`、`bind(alloc, name, val)`、`bindOwned(alloc, name, val)`、`get(name)`。
- `CelExpr`: 编译后的 AST 节点，通过 `deinit(alloc)` 递归释放。
- `CelEvalOptions`: 求值沙盒配置（`max_steps: usize`，默认 `10000`）。

## 3. API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `compileCel` | `compileCel(alloc, source): CelError!*CelExpr` | 编译 CEL 表达式（默认最大深度 64） |
| `compileCelWithMaxDepth` | `compileCelWithMaxDepth(alloc, source, max_depth): CelError!*CelExpr` | 指定最大 AST 递归深度编译表达式 |
| `evalCel` | `evalCel(alloc, expr, env): CelError!CelValue` | 在给定环境中求值返回 `CelValue` |
| `evalCelWithOptions` | `evalCelWithOptions(alloc, expr, env, opts): CelError!CelValue` | 带成本步数上限的沙盒求值 |
| `evalBool` | `evalBool(alloc, expr, env): CelError!bool` | 求值并断言结果为 `bool` |
| `evalBoolWithOptions` | `evalBoolWithOptions(alloc, expr, env, opts): CelError!bool` | 带步数上限的布尔策略求值 |
| `celString` / `celInt` / `celUint` / `celDouble` / `celBool` / `celNull` | 构造辅助函数 | 快速构造 `CelValue` |

**支持语法与内置能力**：
- 运算符优先级：`? :` -> `||` -> `&&`（短路）-> `== != < <= > >= in` -> `+ -` -> `* / %` -> `! -` -> `.` / `[]` / `()`。
- 内置函数与方法：`size()`、`contains()`、`startsWith()`、`endsWith()`、`matches()`、`int()`、`uint()`、`double()`、`string()`、`bool()`、`has(x.y)`。
- 列表推导宏：`all(x, cond)`、`exists(x, cond)`、`exists_one(x, cond)`、`filter(x, cond)`、`map(x, expr)`。

## 4. 示例

```cp
import { CelEnv, compileCel, evalBool, celString } from "cap:cel";

let env = CelEnv.init();
defer env.deinit(alloc);
try env.bindOwned(alloc, "role", try celString(alloc, "admin"));

const expr = try compileCel(alloc, "role in ['admin', 'owner'] && role.startsWith('ad')");
defer expr.deinit(alloc);

const allowed = try evalBool(alloc, expr, &env);
```
