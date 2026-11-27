[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mongodb-connection-string-url

Zero-allocation MongoDB connection string (mongodb:// & mongodb+srv://) parser, option extractor, and password-redacting formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mongodb-connection-string-url` ([https://github.com/mongodb-js/devtools-shared](https://github.com/mongodb-js/devtools-shared)) / `crates.io:mongodb` ([https://github.com/mongodb/mongo-rust-driver](https://github.com/mongodb/mongo-rust-driver))（原库名 `mongodb-connection-string-url` → CP 包名 `mongodb-connection-string-url`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mongodb-connection-string-url` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mongodb-connection-string-url
```

```cp
import {
    MongoHost,
    MongoUri,
    muStartsWithIgnoreCase,
    muParsePort,
    muWrite,
    muWriteU16,
    muEmptyResult,
    parseMongoUri,
} from "mongodb-connection-string-url";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MongoHost` | `class` | 字段: `host: string, port: u16` |
| `MongoUri` | `class` | 字段: `is_srv: bool, username: string, password: string, hosts: [8]MongoHost, host_count: usize, database: string, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `muStartsWithIgnoreCase` | `(s: &[]u8, prefix: &[]u8): bool` | 调用 `muStartsWithIgnoreCase` 并返回 `bool`。 |
| `muParsePort` | `(s: &[]u8, out: &mut u16): bool` | 调用 `muParsePort` 并返回 `bool`。 |
| `muWrite` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `muWrite` 并返回 `bool`。 |
| `muWriteU16` | `(out: []u8, pos: &mut usize, val: u16): bool` | 调用 `muWriteU16` 并返回 `bool`。 |
| `muEmptyResult` | `(): MongoUri` | 调用 `muEmptyResult` 并返回 `MongoUri`。 |
| `parseMongoUri` | `(uri: string): MongoUri` | 通过 `parseMongoUri` 解析输入数据，返回 `MongoUri`。 |
| `getQueryOption` | `(uri: string, key: string): string` | 查询或读取目标数据，返回 `string`。 |
| `formatMongoUri` | `(cfg: MongoUri, redact_password: bool, out: []u8): usize` | 通过 `formatMongoUri` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseMongoUri } from "mongodb-connection-string-url";

pub fn main() {
    let result = parseMongoUri("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
