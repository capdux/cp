[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sqlstring

Zero-allocation ANSI SQL / PostgreSQL / MySQL string literal, identifier, and LIKE pattern escaper with placeholder counter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sqlstring` ([https://github.com/mysqljs/sqlstring](https://github.com/mysqljs/sqlstring)) / `npm:pg-format` ([https://github.com/datalanche/node-pg-format](https://github.com/datalanche/node-pg-format))（原库名 `sqlstring` → CP 包名 `sqlstring`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `sqlstring` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sqlstring
```

```cp
import {
    escapeQuoted,
    escapeStringAnsi,
    escapeIdentifierAnsi,
    escapeIdentifierMysql,
    escapeLike,
    isSafeUnquotedIdent,
    countPlaceholders,
} from "sqlstring";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `escapeQuoted` | `(input: string, quote: u8, allow_empty: bool, out: []u8): usize` | 调用 `escapeQuoted` 并返回 `usize`。 |
| `escapeStringAnsi` | `(input: string, out: []u8): usize` | 调用 `escapeStringAnsi` 并返回 `usize`。 |
| `escapeIdentifierAnsi` | `(ident: string, out: []u8): usize` | 调用 `escapeIdentifierAnsi` 并返回 `usize`。 |
| `escapeIdentifierMysql` | `(ident: string, out: []u8): usize` | 调用 `escapeIdentifierMysql` 并返回 `usize`。 |
| `escapeLike` | `(pattern: string, escape_char: u8, out: []u8): usize` | 调用 `escapeLike` 并返回 `usize`。 |
| `isSafeUnquotedIdent` | `(ident: string): bool` | 执行 `isSafeUnquotedIdent` 条件判定，返回 `bool`。 |
| `countPlaceholders` | `(sql: string): usize` | 调用 `countPlaceholders` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isSafeUnquotedIdent } from "sqlstring";

pub fn main() {
    let result = isSafeUnquotedIdent("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
