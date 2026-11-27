[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# rustls-pki-types

> **源包溯源**：吸收自 **crates.io** [`rustls-pki-types`](https://github.com/rustls/pki-types)（v1 / `MIT`），CP 官方洁净室实现 `rustls-pki-types@1.0.0`。

纯 CP 实现的零堆分配 TLS/PKI 基础类型库：提供 RFC 1035/1123 `DnsName`、支持 DNS/IPv4/IPv6 的 `ServerName`、证书有效期 `UnixTime`，以及复用 `pem-rfc7468` 的 PEM 识别与 DER 解码。

## 核心亮点

- **零堆分配内联存储**：`DnsName` 内联存储最长 253 字节域名，`ServerName` 内联存储 DNS 或 IPv4/IPv6 字节，`decodePemDer` 直接写入调用方缓冲区。
- **零栈清零优化**：固定栈数组（`[253]u8`、`[4]u8`、`[16]u8` 与 IPv6 解析缓冲）全部采用 `undefined` 初始化，消除热路径栈清零开销。
- **严格标准校验**：完整校验 RFC 1035/1123 域名规则（单标签 ≤ 63 字节、总长 ≤ 253 字节、首尾不可为 `-`）及 IPv4/IPv6（含 `::` 零压缩与 IPv4 映射地址）。
- **统一 PEM/DER 桥接**：复用 `pem-rfc7468`，一行完成 PEM 类型探测与 DER 解码。

## 零成本迁移（Rust → CP）

直接沿用原包名 `rustls-pki-types`，无需改名：

**Rust：**
```rust
use rustls_pki_types::{DnsName, ServerName, UnixTime, CertificateDer};
```

**CP：**
```typescript
import {
    DnsName, ServerName, UnixTime, PkiPemResult,
    parseDnsName, isValidDnsName, parseServerName,
    detectPemKind, decodePemDer, pemKindLabel, isPrivateKeyKind
} from "rustls-pki-types";
```

## 实战示例

### 1. TLS SNI 主机名校验与证书有效期检查

```typescript
import { parseServerName, parseDnsName, UnixTime } from "rustls-pki-types";

export fn checkTlsTarget(host: string, now_secs: u64): bool {
    let sn = parseServerName(host);
    if (!sn.isValid()) {
        return false;
    }
    if (sn.isDns()) {
        let parent = parseDnsName("example.com");
        if (!sn.dns.isSubdomainOf(parent) && !sn.dns.eqIgnoreCase(parent)) {
            return false;
        }
    }
    let now = UnixTime.fromSeconds(now_secs);
    let not_before = UnixTime.fromSeconds(1700000000);
    let not_after = UnixTime.fromSeconds(1900000000);
    return now.isWithin(not_before, not_after);
}
```

### 2. 探测并解码 PEM 为 DER

```typescript
import { decodePemDer, pemKindLabel } from "rustls-pki-types";

export fn decodeCertOrKey(pem: string): usize {
    let mut der_buf: [4096]u8 = undefined;
    let res = decodePemDer(pem, der_buf[0..4096]);
    if (!res.isOk()) {
        return 0;
    }
    if (res.isCertificate() || res.isPrivateKey()) {
        let label = pemKindLabel(res.kind);
        _ = label;
        return res.der_len;
    }
    return 0;
}
```

## 核心 API 速览

- **`DnsName`**：`parse(input)`、`isValid()`、`eqIgnoreCase(other)`、`eqIgnoreCaseStr(other)`、`labelCount()`、`isSubdomainOf(parent)`、`toLowercase(out)`；快捷函数 `isValidDnsName`、`parseDnsName`。
- **`ServerName`**：`parse(input)`、`fromDns`、`fromIpv4`、`fromIpv6`、`isValid()`、`isDns()`、`isIpv4()`、`isIpv6()`、`isIp()`；快捷函数 `parseServerName`。
- **`UnixTime`**：`epoch()`、`fromSeconds(secs)`、`asSeconds()`、`addSeconds`、`subSeconds`、`isBefore`、`isAfter`、`equals`、`isWithin(not_before, not_after)`。
- **PEM 工具**：`detectPemKind(pem)`、`decodePemDer(pem, der_out)`、`pemKindLabel(kind)`、`isPrivateKeyKind(kind)`。
- **常量**：`SERVER_NAME_INVALID (0)` / `DNS (1)` / `IPV4 (2)` / `IPV6 (3)`；`PEM_UNKNOWN (0)` ~ `PEM_CSR (7)`；`PKI_OK (0)` / `PKI_ERR_* (-101..-104)`。

## 许可证

MIT
