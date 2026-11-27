[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mysql2

Zero-heap MySQL / MariaDB / TiDB connection string parser (mysql:// URI & Go-style user:pass@tcp(host:port)/db) with password redaction for CP (compatible with npm mysql2 & Rust mysql)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mysql2` ([https://github.com/sidorares/node-mysql2](https://github.com/sidorares/node-mysql2)) / `crates.io:mysql` ([https://github.com/blackbeam/rust-mysql-simple](https://github.com/blackbeam/rust-mysql-simple))（原库名 `mysql2` → CP 包名 `mysql2`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mysql2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mysql2
```

```cp
import {
    MysqlConfig,
    eqBytes,
    startsWith,
    parsePort,
    indexOf,
    lastIndexOf,
    applyQueryParams,
    splitHostPort,
} from "mysql2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MysqlConfig` | `class` | 字段: `tls: bool, host: string, port: u16, user: string, password: string, database: string, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `eqBytes` | `(a: string, b: string): bool` | 调用 `eqBytes` 并返回 `bool`。 |
| `startsWith` | `(s: string, prefix: string): bool` | 调用 `startsWith` 并返回 `bool`。 |
| `parsePort` | `(s: string): i32` | 通过 `parsePort` 解析输入数据，返回 `i32`。 |
| `indexOf` | `(s: string, ch: u8): i64` | 调用 `indexOf` 并返回 `i64`。 |
| `lastIndexOf` | `(s: string, ch: u8): i64` | 调用 `lastIndexOf` 并返回 `i64`。 |
| `applyQueryParams` | `(query: string, cfg: &mut MysqlConfig)` | 就地执行 `applyQueryParams` 操作。 |
| `splitHostPort` | `(hp: string, out_host: &mut string, out_port: &mut u16): bool` | 调用 `splitHostPort` 并返回 `bool`。 |
| `parseUri` | `(uri: string): MysqlConfig` | 通过 `parseUri` 解析输入数据，返回 `MysqlConfig`。 |
| `parseGoDsn` | `(dsn: string): MysqlConfig` | 通过 `parseGoDsn` 解析输入数据，返回 `MysqlConfig`。 |
| `parse` | `(input: string): MysqlConfig` | 通过 `parse` 解析输入数据，返回 `MysqlConfig`。 |
| `appendStr` | `(dst: []u8, pos: usize, src: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `appendU16` | `(dst: []u8, pos: usize, val: u16): usize` | 调用 `appendU16` 并返回 `usize`。 |
| `formatUri` | `(cfg: MysqlConfig, redact_password: bool, out: []u8): usize` | 通过 `formatUri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parsePort } from "mysql2";

pub fn main() {
    let result = parsePort("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
