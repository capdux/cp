[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# webpki-roots

Zero-allocation Mozilla NSS / WebPKI root CA trust anchor catalog and dynamic trust store reusing x509-cert and spki (webpki-roots parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:webpki-roots` ([https://github.com/rustls/webpki-roots](https://github.com/rustls/webpki-roots))

| 元信息 | 值 |
| --- | --- |
| 包名 | `webpki-roots` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add webpki-roots
```

```cp
import {
    TrustAnchorInfo,
    TrustAnchorResult,
    TrustAnchorStore,
    wrStrEq,
    wrBytesEqStr,
    wrIsDigit,
    wrParseCertNotAfterYear,
    wrMakeRoot,
} from "webpki-roots";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TrustAnchorInfo` | `class` | 字段: `name: string, org: string, country: string, key_alg: u8, spki_fingerprint: u64, not_after_year: u16` · 方法: `empty, init, isEmpty, isValidAtYear, matchesIssuer` |
| `TrustAnchorResult` | `class` | 字段: `found: bool, anchor: TrustAnchorInfo` |
| `TrustAnchorStore` | `class` | 字段: `anchors: [32]TrustAnchorInfo, len: usize` · 方法: `empty, withMozillaDefaults, count, capacity, at, addAnchor, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wrStrEq` | `(a: string, b: string): bool` | 调用 `wrStrEq` 并返回 `bool`。 |
| `wrBytesEqStr` | `(b: &[]u8, s: string): bool` | 调用 `wrBytesEqStr` 并返回 `bool`。 |
| `wrIsDigit` | `(ch: u8): bool` | 调用 `wrIsDigit` 并返回 `bool`。 |
| `wrParseCertNotAfterYear` | `(cert: X509Certificate): u16` | 调用 `wrParseCertNotAfterYear` 并返回 `u16`。 |
| `wrMakeRoot` | `(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo` | 调用 `wrMakeRoot` 并返回 `TrustAnchorInfo`。 |
| `mozillaRootAt` | `(idx: usize): TrustAnchorInfo` | 调用 `mozillaRootAt` 并返回 `TrustAnchorInfo`。 |
| `lookupMozillaRootByCn` | `(cn: string): TrustAnchorResult` | 查询或读取目标数据，返回 `TrustAnchorResult`。 |
| `certSpkiFingerprint` | `(cert: X509Certificate): u64` | 调用 `certSpkiFingerprint` 并返回 `u64`。 |
| `wrEmptyAnchors` | `(): [32]TrustAnchorInfo` | 调用 `wrEmptyAnchors` 并返回 `[32]TrustAnchorInfo`。 |

## 最小调用示例

```cp
import { wrStrEq } from "webpki-roots";

pub fn main() {
    let result = wrStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
