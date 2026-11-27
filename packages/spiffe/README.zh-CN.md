[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# spiffe

Zero-allocation CNCF SPIFFE ID (spiffe://<trust-domain>/<path>) parser, strict security validator, and formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:spiffe` ([https://github.com/maxlambrecht/rust-spiffe](https://github.com/maxlambrecht/rust-spiffe))（原库名 `spiffe` → CP 包名 `spiffe`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `spiffe` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add spiffe
```

```cp
import {
    SpiffeId,
    sidEqBytes,
    sidIsTrustDomainChar,
    sidIsPathSegmentChar,
    isValidTrustDomain,
    isValidPath,
    parseSpiffeId,
    belongsToTrustDomain,
} from "spiffe";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SpiffeId` | `class` | 字段: `trust_domain: string, path: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sidEqBytes` | `(a: &[]u8, b: &[]u8): bool` | 调用 `sidEqBytes` 并返回 `bool`。 |
| `sidIsTrustDomainChar` | `(c: u8): bool` | 调用 `sidIsTrustDomainChar` 并返回 `bool`。 |
| `sidIsPathSegmentChar` | `(c: u8): bool` | 调用 `sidIsPathSegmentChar` 并返回 `bool`。 |
| `isValidTrustDomain` | `(td: string): bool` | 执行 `isValidTrustDomain` 条件判定，返回 `bool`。 |
| `isValidPath` | `(path: string): bool` | 执行 `isValidPath` 条件判定，返回 `bool`。 |
| `parseSpiffeId` | `(uri: string): SpiffeId` | 通过 `parseSpiffeId` 解析输入数据，返回 `SpiffeId`。 |
| `belongsToTrustDomain` | `(id: SpiffeId, trust_domain: string): bool` | 调用 `belongsToTrustDomain` 并返回 `bool`。 |
| `formatSpiffeId` | `(trust_domain: string, path: string, out: []u8): usize` | 通过 `formatSpiffeId` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidTrustDomain } from "spiffe";

pub fn main() {
    let result = isValidTrustDomain("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
