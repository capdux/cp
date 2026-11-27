[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hkdf

纯 CP 实现的零堆分配 RFC 5869 HKDF 密钥派生函数库，完整支持 SHA-256、SHA-384、SHA-512，以及多段 `info` 与 TLS 1.3 `HkdfLabel` 扩展。

- **包名**：`hkdf`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `crates.io/hkdf`（`MIT OR Apache-2.0`，`RustCrypto/KDFs`），底层组合 `hmac` 实现 HMAC-SHA2 原语。

## 安装与导入

```bash
cap add hkdf
```

```typescript
import {
    HKDF_HASH_LEN, HKDF_MAX_OKM_LEN,
    HKDF_SHA256_HASH_LEN, HKDF_SHA256_MAX_OKM_LEN,
    HKDF_SHA384_HASH_LEN, HKDF_SHA384_MAX_OKM_LEN,
    HKDF_SHA512_HASH_LEN, HKDF_SHA512_MAX_OKM_LEN,
    HkdfHashAlg, HkdfSha256, HkdfSha384, HkdfSha512,
    hkdfSha256, hkdfSha384, hkdfSha512,
    hkdfExtract, hkdfExpand, hkdfDerive,
} from "hkdf";
```

## 核心 API

### 常量与算法枚举

| 符号 | 值 / 签名 | 说明 |
| --- | --- | --- |
| `HKDF_HASH_LEN` / `HKDF_SHA256_HASH_LEN` | `32` | SHA-256 PRK 字节长度。 |
| `HKDF_MAX_OKM_LEN` / `HKDF_SHA256_MAX_OKM_LEN` | `8160`（`255 * 32`） | RFC 5869 SHA-256 最大 OKM 长度。 |
| `HKDF_SHA384_HASH_LEN` / `HKDF_SHA384_MAX_OKM_LEN` | `48` / `12240` | SHA-384 PRK 长度与最大 OKM 长度。 |
| `HKDF_SHA512_HASH_LEN` / `HKDF_SHA512_MAX_OKM_LEN` | `64` / `16320` | SHA-512 PRK 长度与最大 OKM 长度。 |
| `HkdfHashAlg` | `Sha256`, `Sha384`, `Sha512`；`hashLen(): usize`, `maxOkmLen(): usize` | 通用 HKDF 函数的运行时算法枚举。 |

### 算法类

| 类 | 方法 | 说明 |
| --- | --- | --- |
| `HkdfSha256` | `extract(salt: []u8, ikm: []u8): HkdfSha256`, `fromPrk(prk_in: []u8): HkdfSha256?`, `expand(info: []u8, okm: []u8): bool`, `expandMultiInfo(infos: &[]string, okm: []u8): bool`, `expandLabel(label: &[]u8, context: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool` | RFC 5869 HKDF-SHA256（`prk: [32]u8`），支持多段 info、TLS 1.3 标签扩展及常量时间 PRK 校验。 |
| `HkdfSha384` | `extract(salt: &[]u8, ikm: &[]u8): HkdfSha384`, `fromPrk(prk_in: &[]u8): HkdfSha384?`, `expand(info: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool` | RFC 5869 HKDF-SHA384（`prk: [48]u8`）。 |
| `HkdfSha512` | `extract(salt: &[]u8, ikm: &[]u8): HkdfSha512`, `fromPrk(prk_in: &[]u8): HkdfSha512?`, `expand(info: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool` | RFC 5869 HKDF-SHA512（`prk: [64]u8`）。 |

### 一次性与多算法分发函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `hkdfSha256` | `(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool` | 一次性 SHA-256 Extract + Expand。 |
| `hkdfSha384` | `(salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool` | 一次性 SHA-384 Extract + Expand。 |
| `hkdfSha512` | `(salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool` | 一次性 SHA-512 Extract + Expand。 |
| `hkdfExtract` | `(alg: HkdfHashAlg, salt: &[]u8, ikm: &[]u8, prk_out: []u8): usize` | 通用 Extract，写入 `prk_out` 并返回写入字节数（缓冲区不足返回 `0`）。 |
| `hkdfExpand` | `(alg: HkdfHashAlg, prk: &[]u8, info: &[]u8, okm: []u8): bool` | 通用 Expand，从 `prk` 派生到 `okm`。 |
| `hkdfDerive` | `(alg: HkdfHashAlg, salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool` | 通用一次性 Extract + Expand。 |

## 使用示例

### 1. 使用 `HkdfSha256` 分步 Extract 与 Expand

```typescript
import { HkdfSha256 } from "hkdf";

fn main() {
    let salt: [4]u8 = [0x00, 0x01, 0x02, 0x03];
    let ikm: [4]u8 = [0x0b, 0x0b, 0x0b, 0x0b];
    let info: [4]u8 = [0xf0, 0xf1, 0xf2, 0xf3];
    let okm: [42]u8 = undefined;

    let hk = HkdfSha256.extract(salt[0..], ikm[0..]);
    let ok = hk.expand(info[0..], okm[0..]); // true
}
```

### 2. TLS 1.3 标签派生（`expandLabel`）

```typescript
import { HkdfSha256 } from "hkdf";

fn deriveTrafficKey(prk_bytes: []u8, key_out: []u8): bool {
    if (let hk = HkdfSha256.fromPrk(prk_bytes)) {
        let label = "key";
        let ctx: [0]u8 = [];
        return hk.expandLabel(label, ctx[0..], key_out);
    }
    return false;
}
```

### 3. 运行时算法选择（SHA-384 / SHA-512）

```typescript
import { HkdfHashAlg, hkdfDerive } from "hkdf";

fn main() {
    let salt: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    let ikm: [16]u8 = [0; 16];
    let info: [4]u8 = [0x61, 0x70, 0x70, 0x31];
    let okm: [64]u8 = undefined;

    let ok = hkdfDerive(HkdfHashAlg.Sha512, salt[0..], ikm[0..], info[0..], okm[0..]);
}
```

## 与源包差异及零拷贝安全亮点

- **全栈零堆分配**：PRK 与 OKM 均存储在栈数组或调用方切片（`[32/48/64]u8` 与 `okm: []u8`）中，无堆内存泄漏风险。
- **内置 TLS 1.3 `expandLabel`**：直接按 RFC 8446 构造 `HkdfLabel`（长度 + `"tls13 "` + label + context）并流式扩展。
- **常量时间 PRK 校验**：`verifyPrk` 使用常量时间字节比对，防范时序侧信道。
