[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# idempotency-key

Zero-allocation IETF Idempotency-Key HTTP header parser, validator, RFC 8941 formatter, and request fingerprint state evaluator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:idempotency-key` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `idempotency-key` → CP 包名 `idempotency-key`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `idempotency-key` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add idempotency-key
```

```cp
import {
    idempIsOws,
    idempTrimOws,
    idempIsVisibleAscii,
    idempIsHex,
    isValidKey,
    parseKeyHeader,
    formatKeyHeader,
    isUuidV4Like,
} from "idempotency-key";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `idempIsOws` | `(c: u8): bool` | 调用 `idempIsOws` 并返回 `bool`。 |
| `idempTrimOws` | `(s: &[]u8): &[]u8` | 调用 `idempTrimOws` 并返回 `&[]u8`。 |
| `idempIsVisibleAscii` | `(c: u8): bool` | 调用 `idempIsVisibleAscii` 并返回 `bool`。 |
| `idempIsHex` | `(c: u8): bool` | 调用 `idempIsHex` 并返回 `bool`。 |
| `isValidKey` | `(key: string): bool` | 执行 `isValidKey` 条件判定，返回 `bool`。 |
| `parseKeyHeader` | `(header: string, out: []u8): usize` | 通过 `parseKeyHeader` 解析输入数据，返回 `usize`。 |
| `formatKeyHeader` | `(key: string, out: []u8): usize` | 通过 `formatKeyHeader` 编码或格式化输出，返回 `usize`。 |
| `isUuidV4Like` | `(key: string): bool` | 执行 `isUuidV4Like` 条件判定，返回 `bool`。 |
| `idempMulWrap64` | `(a: u64, b: u64): u64` | 调用 `idempMulWrap64` 并返回 `u64`。 |
| `idempFnvUpdate` | `(h: u64, data: &[]u8): u64` | 调用 `idempFnvUpdate` 并返回 `u64`。 |
| `idempFnvByte` | `(h: u64, b: u8): u64` | 调用 `idempFnvByte` 并返回 `u64`。 |
| `fingerprintRequest` | `(method: string, path: string, body: &[]u8): u64` | 调用 `fingerprintRequest` 并返回 `u64`。 |
| `evaluateState` | `(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32` | 调用 `evaluateState` 并返回 `i32`。 |

## 最小调用示例

```cp
import { isValidKey } from "idempotency-key";

pub fn main() {
    let result = isValidKey("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
