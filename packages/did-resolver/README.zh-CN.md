[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# did-resolver

Zero-heap W3C DID Core 1.0 Decentralized Identifier & DID URL parser, strict character validator, and formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:did-resolver` ([https://github.com/decentralized-identity/did-resolver](https://github.com/decentralized-identity/did-resolver)) / `crates.io:ssi-dids` ([https://github.com/spruceid/ssi](https://github.com/spruceid/ssi))（原库名 `did-resolver` → CP 包名 `did-resolver`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `did-resolver` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add did-resolver
```

```cp
import {
    DidUrl,
    isValidMethod,
    isValidMethodId,
    parseDid,
    appendSlice,
    formatDid,
} from "did-resolver";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DidUrl` | `class` | 字段: `did: string, method: string, id: string, path: string, query: string, fragment: string, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isValidMethod` | `(method: string): bool` | 执行 `isValidMethod` 条件判定，返回 `bool`。 |
| `isValidMethodId` | `(id: string): bool` | 执行 `isValidMethodId` 条件判定，返回 `bool`。 |
| `parseDid` | `(input: string): DidUrl` | 通过 `parseDid` 解析输入数据，返回 `DidUrl`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `formatDid` | `(method: string, id: string, fragment: string, out: []u8): usize` | 通过 `formatDid` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidMethod } from "did-resolver";

pub fn main() {
    let result = isValidMethod("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
