# `cap:jks` — Java KeyStore (`.jks`) v1/v2 零分配二进制解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

`cap:jks` 提供纯 CP、零堆分配的 Sun/Oracle Java KeyStore (`.jks`) v1/v2 二进制解析能力（大端序），支持 `0xFEEDFEED` 魔数头校验、`PrivateKeyEntry`（含加密 PKCS#8 DER 与多张 X.509 证书链）、`TrustedCertEntry`、别名检索以及末尾 20 字节 SHA-1 完整性摘要切片提取。

## 导入方式

```cp
import {
    JKS_MAGIC,
    JKS_VERSION_1,
    JKS_VERSION_2,
    JKS_TAG_PRIVATE_KEY,
    JKS_TAG_TRUSTED_CERT,
    JKS_DIGEST_SIZE,
    JksHeader,
    JksCert,
    JksEntry,
    JksStats,
    JksCertIter,
    JksEntryIter,
    isJks,
    parseJksHeader,
    findJksEntry,
    summarizeJks,
} from "cap:binfmt";
```

## 常量

- `JKS_MAGIC = 0xFEEDFEED`
- `JKS_VERSION_1 = 1`、`JKS_VERSION_2 = 2`
- `JKS_TAG_PRIVATE_KEY = 1`、`JKS_TAG_TRUSTED_CERT = 2`
- `JKS_DIGEST_SIZE = 20`（尾部 20B SHA-1 完整性摘要）

## 数据结构

### `JksHeader`
- `version: u32`（`1` 或 `2`）、`entry_count: u32`、`entries_offset: usize`（`12`）
- `digest: []const u8`（末尾 20B SHA-1 切片）
- `signed_body: []const u8`（`data[0..data.len - 20]`，供外部 SHA-1 验签）

### `JksCert` & `JksCertIter`
- `JksCert`：`cert_type: []const u8`（v2 显式 UTF-8 如 `"X.509"`，v1 默认 `"X.509"`）、`der: []const u8`
- `JksCertIter.init(certs_slice, cert_count, version)` / `.next()`

### `JksEntry` & `JksEntryIter`
- `tag: u32`, `alias: []const u8`, `timestamp_ms: i64`
- `PrivateKeyEntry`：`protected_key_der: []const u8`, `cert_count: u32`, `certs_slice: []const u8`，附 `certs(version): JksCertIter`
- `TrustedCertEntry`：`cert_type: []const u8`, `cert_der: []const u8`
- 辅助：`isPrivateKey(): bool`、`isTrustedCert(): bool`

### `JksStats`
- `version`, `entry_count`, `private_key_count`, `trusted_cert_count`, `total_certs: u32`

## 核心函数

- `isJks(data: []const u8): bool` — 校验 `len >= 32`、魔数 `0xFEEDFEED` 及 `version` 1/2
- `parseJksHeader(data: []const u8): JksHeader?` — 解析 12B 头与 20B 尾摘要
- `findJksEntry(data: []const u8, alias: []const u8): JksEntry?` — 按别名零分配检索
- `summarizeJks(data: []const u8): JksStats?` — 单遍校验并统计（条目截断或尾部非精确 20B 摘要返回 `null`）

## 快速示例

```cp
import { parseJksHeader, JksEntryIter, findJksEntry, summarizeJks } from "cap:binfmt";

fn inspectKeystore(jks_bytes: []const u8) {
    if (hdr := parseJksHeader(jks_bytes)) {
        let it = JksEntryIter.init(jks_bytes);
        while (entry := it.next()) {
            if (entry.isPrivateKey()) {
                let cit = entry.certs(hdr.version);
                while (cert := cit.next()) {
                    * = cert.der;
                }
            }
        }
    }
    * = findJksEntry(jks_bytes, "server-tls");
    * = summarizeJks(jks_bytes);
}
```
