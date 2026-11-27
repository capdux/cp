[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# hkdf

面向 CP 的零堆分配 RFC 5869 **HKDF-SHA256**（基于 HMAC 的提取-扩展密钥派生函数，对齐 RustCrypto `hkdf`）。

支持两阶段 `extract` + `expand`、基于已有 32 字节伪随机密钥（PRK）的 `fromPrk` 恢复以及一次性 `hkdfSha256` 派生，全程栈上执行、零堆分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `hkdf`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/RustCrypto/KDFs` |
| 开源协议 | MIT（上游：MIT OR Apache-2.0） |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: hkdf)
use hkdf::Hkdf;
let hk = Hkdf::<Sha256>::new(Some(salt), ikm);
hk.expand(info, &mut okm)?;
```

```cp
// CP (cap: hkdf) — 零堆分配 RFC 5869 HKDF-SHA256
import { HkdfSha256, hkdfSha256 } from "hkdf";
const hk = HkdfSha256.extract(salt, ikm);
const ok = hk.expand(info, okm);
```

## 安装

```bash
cap add hkdf
```

## 实战示例

### 基础用法：一次性派生与多子密钥提取-扩展

```cp
import { HkdfSha256, hkdfSha256 } from "hkdf";

let salt: [13]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C];
let ikm: [22]u8 = [0x0B; 22];
let info: [10]u8 = [0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9];

// 1. 一次性密钥派生（RFC 5869 测试向量 1 -> 42 字节 OKM）
let okm: [42]u8 = undefined;
const ok = hkdfSha256(salt[0..13], ikm[0..22], info[0..10], okm[0..42]);

// 2. 一次 Extract，多次按不同 info 域隔离扩展出加密密钥与认证密钥
const hk = HkdfSha256.extract(salt[0..13], ikm[0..22]);
let encInfo: [7]u8 = ['e', 'n', 'c', '-', 'k', 'e', 'y'];
let macInfo: [7]u8 = ['m', 'a', 'c', '-', 'k', 'e', 'y'];
let encKey: [32]u8 = undefined;
let macKey: [32]u8 = undefined;
const ok1 = hk.expand(encInfo[0..7], encKey[0..32]);
const ok2 = hk.expand(macInfo[0..7], macKey[0..32]);
```

### 安全与边界用法：空盐值、`fromPrk` 长度校验与 8160 字节上限

```cp
import { HkdfSha256 } from "hkdf";

// 1. 空盐值按 RFC 5869 §2.2 自动使用 32 字节全零盐
let emptySalt: [0]u8 = [];
let ikm: [16]u8 = [0x42; 16];
const hk = HkdfSha256.extract(emptySalt[0..0], ikm[0..16]);

// 2. 从已有 PRK 恢复（不足 32 字节安全返回 null）
let shortPrk: [16]u8 = [0; 16];
const invalid = HkdfSha256.fromPrk(shortPrk[0..16]); // null

if (resumed := HkdfSha256.fromPrk(hk.prk[0..32])) {
    let key: [32]u8 = undefined;
    const derived = resumed.expand(emptySalt[0..0], key[0..32]); // true
}

// 3. 严格执行 RFC 5869 上限（255 * 32 = 8160 字节），超长返回 false
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `HkdfSha256.extract` | `static fn(salt: []u8, ikm: []u8): HkdfSha256` | RFC 5869 Extract（空盐默认 32 字节全零）。 |
| `HkdfSha256.fromPrk` | `static fn(prk_in: []u8): HkdfSha256?` | 从 `>= 32` 字节 PRK 恢复；不足 32 字节返回 `null`。 |
| `HkdfSha256.expand` | `fn(info: []u8, okm: []u8): bool` | RFC 5869 Expand；`okm.len > 8160` 返回 `false`。 |
| `hkdfSha256` | `fn(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool` | 一次性完成 Extract + Expand 并写入 `okm`。 |

## 安全特性

- **零堆分配**：PRK（`[32]u8`）、计数器块与 HMAC 中间态均在栈上完成。
- **严格 RFC 5869 合规**：内置 `8160` 字节 OKM 上限保护与空盐规范处理。

## 协议

MIT — 对齐 RustCrypto `hkdf` 的纯 CP 净室实现。
