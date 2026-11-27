[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# toidentifier

纯 CP 实现的零堆分配标识符转换库，可将任意词组或 HTTP 状态短语转换为 PascalCase、camelCase、`CONSTANT_CASE`、`snake_case` 与安全标识符，并内置标识符校验函数。

- **包名**：`toidentifier`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `npm/toidentifier`（`MIT`，`dougwilson/toidentifier`），扩展了缓冲区检查、安全前导下划线补齐、多命名风格转换与合法性校验。

## 安装与导入

```bash
cap add toidentifier
```

```typescript
import {
    toIdentifierLen, toIdentifier, toIdentifierChecked,
    toSafeIdentifier, toCamelIdentifier, toConstIdentifier, toSnakeIdentifier,
    isValidIdentifier, isValidJsIdentifier,
} from "toidentifier";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `toIdentifierLen` | `(input: string): usize` | 预计算 `toIdentifier(input, out)` 的精确输出长度。 |
| `toIdentifier` | `(input: string, out: []u8): usize` | 将词组转为 PascalCase 标识符（如 `"Bad Request"` -> `"BadRequest"`），过滤非字母数字字符。 |
| `toIdentifierChecked` | `(input: string, out: []u8): usize?` | 同 `toIdentifier`，缓冲区长度不足时返回 `null`。 |
| `toSafeIdentifier` | `(input: string, out: []u8): usize` | 生成合法标识符：若以数字开头则自动前置 `_`，空输入输出 `"_"`。 |
| `toCamelIdentifier` | `(input: string, out: []u8): usize` | 转为 `camelCase` 标识符（如 `"Bad Request"` -> `"badRequest"`）。 |
| `toConstIdentifier` | `(input: string, out: []u8): usize` | 转为 `UPPER_SNAKE_CASE` 常量标识符（如 `"Bad Request"` -> `"BAD_REQUEST"`）。 |
| `toSnakeIdentifier` | `(input: string, out: []u8): usize` | 转为 `lower_snake_case` 标识符（如 `"Bad Request"` -> `"bad_request"`）。 |
| `isValidIdentifier` | `(s: string): bool` | 校验标准 ASCII 标识符语法（`[A-Za-z_][A-Za-z0-9_]*`）。 |
| `isValidJsIdentifier` | `(s: string): bool` | 校验 JavaScript ASCII 标识符语法（`[A-Za-z_$][A-Za-z0-9_$]*`）。 |

## 使用示例

### 1. HTTP 状态短语转 PascalCase 标识符

```typescript
import { toIdentifier, toIdentifierChecked } from "toidentifier";

fn main() {
    let buf: [64]u8 = undefined;
    let n = toIdentifier("I'm a Teapot", buf[0..]);
    // buf[0..n] == "ImATeapot"

    if (let m = toIdentifierChecked("Bad Gateway", buf[0..])) {
        // buf[0..m] == "BadGateway"
    }
}
```

### 2. 多风格标识符生成

```typescript
import { toCamelIdentifier, toConstIdentifier, toSnakeIdentifier } from "toidentifier";

fn main() {
    let camel: [64]u8 = undefined;
    let c_len = toCamelIdentifier("Not Found", camel[0..]); // "notFound"

    let cnst: [64]u8 = undefined;
    let k_len = toConstIdentifier("Not Found", cnst[0..]);  // "NOT_FOUND"

    let snake: [64]u8 = undefined;
    let s_len = toSnakeIdentifier("Not Found", snake[0..]); // "not_found"
}
```

### 3. 安全标识符与合法性校验

```typescript
import { toSafeIdentifier, isValidIdentifier, isValidJsIdentifier } from "toidentifier";

fn main() {
    let buf: [32]u8 = undefined;
    let n = toSafeIdentifier("404 Not Found", buf[0..]); // "_404NotFound"
    let ok = isValidIdentifier(buf[0..n]);               // true
    let js_ok = isValidJsIdentifier("$value_1");         // true
}
```

## 与源包差异及零拷贝安全亮点

- **全程零堆分配**：直接向调用方提供的 `out: []u8` 缓冲区单趟写入，无需正则引擎与中间字符串数组。
- **全风格扩展**：在上游 PascalCase 的基础上扩展 `toCamelIdentifier`、`toConstIdentifier`、`toSnakeIdentifier`、`toSafeIdentifier` 及合法性校验。
- **安全容量检查**：`toIdentifierChecked` 返回 `usize?`，缓冲区不足时返回 `null` 避免静默截断。
