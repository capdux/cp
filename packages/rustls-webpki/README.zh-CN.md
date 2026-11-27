[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rustls-webpki

Pure CP zero-allocation WebPKI X.509 certificate verification engine (RFC 6125 DNS hostname matching, RFC 5280 validity & chain verification; reuses x509-cert)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-webpki` ([https://github.com/rustls/webpki](https://github.com/rustls/webpki))

| 元信息 | 值 |
| --- | --- |
| 包名 | `rustls-webpki` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rustls-webpki
```

```cp
import {
    X509TimeResult,
    wpkiAsciiLower,
    wpkiIsAsciiAlphaNum,
    wpkiIsValidDnsLabelBytes,
    wpkiValidateDnsNameBytes,
    wpkiCountDotsBytes,
    wpkiEqIgnoreCaseBytes,
    wpkiHasStarBytes,
} from "rustls-webpki";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `X509TimeResult` | `class` | 字段: `ok: bool, unix_secs: u64, year: u32, month: u8, day: u8, hour: u8, +2 项` · 方法: `fail` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wpkiAsciiLower` | `(b: u8): u8` | 调用 `wpkiAsciiLower` 并返回 `u8`。 |
| `wpkiIsAsciiAlphaNum` | `(b: u8): bool` | 调用 `wpkiIsAsciiAlphaNum` 并返回 `bool`。 |
| `wpkiIsValidDnsLabelBytes` | `(label: &[]u8): bool` | 调用 `wpkiIsValidDnsLabelBytes` 并返回 `bool`。 |
| `wpkiValidateDnsNameBytes` | `(name: &[]u8): bool` | 调用 `wpkiValidateDnsNameBytes` 并返回 `bool`。 |
| `wpkiCountDotsBytes` | `(s: &[]u8): usize` | 调用 `wpkiCountDotsBytes` 并返回 `usize`。 |
| `wpkiEqIgnoreCaseBytes` | `(a: &[]u8, b: &[]u8): bool` | 调用 `wpkiEqIgnoreCaseBytes` 并返回 `bool`。 |
| `wpkiHasStarBytes` | `(s: &[]u8): bool` | 调用 `wpkiHasStarBytes` 并返回 `bool`。 |
| `verifyDnsHostnameBytes` | `(pattern: &[]u8, hostname: &[]u8): bool` | 校验输入合法性，返回 `bool`。 |
| `verifyDnsHostname` | `(pattern: string, hostname: string): bool` | 校验输入合法性，返回 `bool`。 |
| `wpkiDigit` | `(b: u8): i32` | 调用 `wpkiDigit` 并返回 `i32`。 |
| `wpkiParse2Digits` | `(buf: &[]u8, off: usize): i32` | 调用 `wpkiParse2Digits` 并返回 `i32`。 |
| `wpkiParse4Digits` | `(buf: &[]u8, off: usize): i32` | 调用 `wpkiParse4Digits` 并返回 `i32`。 |
| `wpkiIsLeapYear` | `(year: u32): bool` | 调用 `wpkiIsLeapYear` 并返回 `bool`。 |
| `wpkiDaysInMonth` | `(year: u32, month: u8): u8` | 调用 `wpkiDaysInMonth` 并返回 `u8`。 |
| `wpkiDaysFromCivil` | `(year: u32, month: u8, day: u8): u64` | 调用 `wpkiDaysFromCivil` 并返回 `u64`。 |
| `parseX509TimeUtc` | `(time_bytes: &[]u8): X509TimeResult` | 通过 `parseX509TimeUtc` 解析输入数据，返回 `X509TimeResult`。 |
| `verifyValidityWindow` | `(validity: X509Validity, now_unix: u64): i32` | 校验输入合法性，返回 `i32`。 |
| `verifyValidityAt` | `(cert: X509Certificate, now_unix: u64): i32` | 校验输入合法性，返回 `i32`。 |
| `verifyCertForDnsName` | `(cert: X509Certificate, hostname: string, now_unix: u64): i32` | 校验输入合法性，返回 `i32`。 |
| `verifyDerCertForDnsName` | `(der: &[]u8, hostname: string, now_unix: u64): i32` | 校验输入合法性，返回 `i32`。 |
| `verifyChainLink` | `(leaf: X509Certificate, issuer: X509Certificate): bool` | 校验输入合法性，返回 `bool`。 |
| `verifyTrustAnchor` | `(anchor: X509Certificate, now_unix: u64): i32` | 校验输入合法性，返回 `i32`。 |
| `verifyEndEntityChain` | `(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32` | 校验输入合法性，返回 `i32`。 |

## 最小调用示例

```cp
import { verifyDnsHostname } from "rustls-webpki";

pub fn main() {
    let result = verifyDnsHostname("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
