[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `x509-cert`

CP 零堆分配 RFC 5280 X.509 v1/v2/v3 证书 DER / PEM 编解码库 —— 对齐 RustCrypto [`x509-cert`](https://github.com/RustCrypto/formats)（v0.2）的纯 CP MIT 净室实现，底层组合 `der`、`const-oid`、`spki` 与 `pem-rfc7468`。

## 安装与导入

```bash
cap add x509-cert
```

```cp
import {
    X509_PEM_LABEL, X509_MAX_SERIAL_LEN, X509_MAX_SIG_LEN,
    X509_OK, X509_ERR_BUFFER_TOO_SMALL, X509_ERR_INVALID_DER, X509_ERR_INVALID_VERSION,
    X509_ERR_INVALID_SERIAL, X509_ERR_INVALID_ALG, X509_ERR_ALG_MISMATCH,
    X509_ERR_INVALID_NAME, X509_ERR_INVALID_VALIDITY, X509_ERR_INVALID_SPKI,
    X509_ERR_INVALID_SIGNATURE, X509_ERR_INVALID_PEM, X509_ERR_INVALID_PEM_LABEL,
    X509_ERR_TRAILING_DATA,
    X509Validity, X509Name, X509Certificate, X509DecodeResult,
    encodeX509Der, decodeX509Der, encodeX509Pem, decodeX509Pem, isSelfSigned,
} from "x509-cert";
```

## 核心 API

### 常量与错误码

| 分类 | 常量 | 取值 |
| :--- | :--- | :--- |
| 格式与上限 | `X509_PEM_LABEL`, `X509_MAX_SERIAL_LEN`, `X509_MAX_SIG_LEN` | `"CERTIFICATE"`, `20`, `128` |
| 状态与错误码 (`i64`) | `X509_OK` (0), `X509_ERR_BUFFER_TOO_SMALL` (-1), `X509_ERR_INVALID_DER` (-2), `X509_ERR_INVALID_VERSION` (-3), `X509_ERR_INVALID_SERIAL` (-4), `X509_ERR_INVALID_ALG` (-5), `X509_ERR_ALG_MISMATCH` (-6), `X509_ERR_INVALID_NAME` (-7), `X509_ERR_INVALID_VALIDITY` (-8), `X509_ERR_INVALID_SPKI` (-9), `X509_ERR_INVALID_SIGNATURE` (-10), `X509_ERR_INVALID_PEM` (-11), `X509_ERR_INVALID_PEM_LABEL` (-12), `X509_ERR_TRAILING_DATA` (-13) | 编码失败返回负值；解码失败写入 `.err` |

### 类型与编解码函数

| 符号 | 签名 / 字段 | 说明 |
| :--- | :--- | :--- |
| `X509Validity` | `empty()`, `fromStrs(nb: string, na: string): X509Validity` | 有效期窗口（支持 `UTCTime` / `GeneralizedTime`，20 字节内联缓冲） |
| `X509Name` | `empty()`, `init(cn: string, org: string, country: string): X509Name`, `eq(other: X509Name): bool` | 证书主体/颁发者专有名称（`CN`、`O`、`C`） |
| `X509Certificate` | `version`, `serial`, `signature_alg: AlgorithmIdentifier`, `issuer`, `validity`, `subject`, `spki: SubjectPublicKeyInfo`, `signature`; `empty()` | 全栈式 RFC 5280 证书结构 |
| `X509DecodeResult` | `ok: bool`, `cert: X509Certificate`, `err: i64` | 解码结果结构体（成功时 `err == X509_OK`） |
| `encodeX509Der` | `(cert: X509Certificate, out: []u8): i64` | 编码为 DER 字节流；成功返回写入字节数（`> 0`），失败返回负错误码 |
| `decodeX509Der` | `(der: []const u8): X509DecodeResult` | 严格解析 DER 证书（校验内外签名算法一致与无尾随垃圾字节） |
| `encodeX509Pem` | `(cert: X509Certificate, crlf: bool, out: []u8): i64` | 编码为 RFC 7468 PEM（`-----BEGIN CERTIFICATE-----`） |
| `decodeX509Pem` | `(pem: string): X509DecodeResult` | 解析 PEM 证书并验证 `"CERTIFICATE"` 标签 |
| `isSelfSigned` | `(cert: X509Certificate): bool` | 判断 `cert.issuer.eq(cert.subject)` 是否自签名 |

## 代码示例

### 1. 解析 PEM 证书并读取主体信息

```cp
import { print } from "cap:io";
import { decodeX509Pem, isSelfSigned } from "x509-cert";

fn inspectCert(pem: string) {
    const res = decodeX509Pem(pem);
    if (!res.ok) {
        print("decode failed, err:", res.err);
        return;
    }
    const cert = res.cert;
    print("version:", cert.version, "self_signed:", isSelfSigned(cert));
    print("CN:", cert.subject.cn[0..cert.subject.cn_len]);
    print("Org:", cert.subject.org[0..cert.subject.org_len]);
}
```

### 2. 构造 X.509 证书并编码为 DER 与 PEM

```cp
import { print } from "cap:io";
import {
    X509Certificate, X509Name, X509Validity,
    encodeX509Der, encodeX509Pem,
} from "x509-cert";
import { AlgorithmIdentifier, SubjectPublicKeyInfo } from "spki";
import { ObjectIdentifier, OID_ED25519 } from "const-oid";

fn buildCert(pub_key: []const u8) {
    let cert = X509Certificate.empty();
    cert.version = 2; // v3
    cert.serial[0] = 0x01;
    cert.serial_len = 1;

    const ed_oid = ObjectIdentifier.fromStr(OID_ED25519);
    cert.signature_alg = AlgorithmIdentifier.fromOid(ed_oid.oid);
    cert.issuer = X509Name.init("root.cp-lang.org", "CP Foundation", "US");
    cert.subject = cert.issuer;
    cert.validity = X509Validity.fromStrs("260101000000Z", "360101000000Z");

    const spki_res = SubjectPublicKeyInfo.ed25519(pub_key);
    if (spki_res.ok) { cert.spki = spki_res.spki; }
    cert.signature_len = 64;

    let der_buf: [1024]u8 = undefined;
    const der_len = encodeX509Der(cert, der_buf[0..1024]);
    let pem_buf: [2048]u8 = undefined;
    const pem_len = encodeX509Pem(cert, false, pem_buf[0..2048]);
    print("der_len:", der_len, "pem_len:", pem_len);
}
```

## 与源包差异及性能亮点

- **零堆分配**：证书各字段均采用固定长度栈上数组，TLS 握手或 mTLS 证书校验无需堆分配与垃圾回收。
- **严格 RFC 5280 校验**：自动检测外层 `signatureAlgorithm` 与 `tbsCertificate.signature` 一致性（`X509_ERR_ALG_MISMATCH`），拒绝尾随多余字节（`X509_ERR_TRAILING_DATA`）。
- **无重复代码**：底层直接复用 `der`、`const-oid`、`spki` 与 `pem-rfc7468`。
