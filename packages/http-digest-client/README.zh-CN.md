[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# http-digest-client

Zero-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:http-digest-client` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:digest_auth` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `http-digest-client` → CP 包名 `http-digest-client`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `http-digest-client` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add http-digest-client
```

```cp
import {
    DigestChallenge,
    DigestResponse,
    digIsSpace,
    trimSlice,
    stripDigestPrefix,
    unquoteVal,
    parseChallenge,
    parseAuthorization,
} from "http-digest-client";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DigestChallenge` | `class` | 字段: `realm: string, nonce: string, opaque_val: string, qop: string, algorithm: string, stale: bool, +1 项` |
| `DigestResponse` | `class` | 字段: `username: string, realm: string, nonce: string, uri: string, qop: string, nc: string, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `digIsSpace` | `(b: u8): bool` | 调用 `digIsSpace` 并返回 `bool`。 |
| `trimSlice` | `(s: string): string` | 调用 `trimSlice` 并返回 `string`。 |
| `stripDigestPrefix` | `(header: string): string` | 调用 `stripDigestPrefix` 并返回 `string`。 |
| `unquoteVal` | `(v: string): string` | 调用 `unquoteVal` 并返回 `string`。 |
| `parseChallenge` | `(header: string): DigestChallenge` | 通过 `parseChallenge` 解析输入数据，返回 `DigestChallenge`。 |
| `parseAuthorization` | `(header: string): DigestResponse` | 通过 `parseAuthorization` 解析输入数据，返回 `DigestResponse`。 |
| `supportsQop` | `(qop_list: string, target: string): bool` | 调用 `supportsQop` 并返回 `bool`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushQuotedField` | `(dst: []u8, pos: usize, first: bool, key: string, val: string): i64` | 写入或更新状态数据。 |
| `pushTokenField` | `(dst: []u8, pos: usize, first: bool, key: string, val: string): i64` | 写入或更新状态数据。 |
| `formatAuthorization` | `(resp: DigestResponse, out: []u8): usize` | 通过 `formatAuthorization` 编码或格式化输出，返回 `usize`。 |
| `formatChallenge` | `(ch: DigestChallenge, out: []u8): usize` | 通过 `formatChallenge` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseChallenge } from "http-digest-client";

pub fn main() {
    let result = parseChallenge("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
