[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pg-connection-string

Zero-heap PostgreSQL connection string parser and URI builder supporting both postgresql:// URIs and libpq key=value DSNs.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pg-connection-string` ([https://github.com/brianc/node-postgres](https://github.com/brianc/node-postgres)) / `crates.io:tokio-postgres` ([https://github.com/sfackler/rust-postgres](https://github.com/sfackler/rust-postgres))（原库名 `pg-connection-string` → CP 包名 `pg-connection-string`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `pg-connection-string` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pg-connection-string
```

```cp
import {
    PgConfig,
    makeInvalid,
    isWs,
    parsePortStrict,
    isValidSslMode,
    parseUri,
    parseKeyValue,
    parse,
} from "pg-connection-string";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PgConfig` | `class` | 字段: `host: string, port: u16, user: string, password: string, database: string, sslmode: string, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeInvalid` | `(): PgConfig` | 调用 `makeInvalid` 并返回 `PgConfig`。 |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `parsePortStrict` | `(s: string): i32` | 通过 `parsePortStrict` 解析输入数据，返回 `i32`。 |
| `isValidSslMode` | `(mode: string): bool` | 执行 `isValidSslMode` 条件判定，返回 `bool`。 |
| `parseUri` | `(uri: string): PgConfig` | 通过 `parseUri` 解析输入数据，返回 `PgConfig`。 |
| `parseKeyValue` | `(dsn: string): PgConfig` | 通过 `parseKeyValue` 解析输入数据，返回 `PgConfig`。 |
| `parse` | `(input: string): PgConfig` | 通过 `parse` 解析输入数据，返回 `PgConfig`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `appendU16` | `(out: []u8, pos: usize, v: u16): i64` | 调用 `appendU16` 并返回 `i64`。 |
| `formatUri` | `(cfg: PgConfig, include_password: bool, out: []u8): usize` | 通过 `formatUri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parsePortStrict } from "pg-connection-string";

pub fn main() {
    let result = parsePortStrict("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
