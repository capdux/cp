# @cap/subtle

零堆分配的 CP 恒定时间（Constant-Time）防时序侧信道比较/条件选择/交换与敏感内存安全清零引擎 — 洁净室 MIT 实现，对标 Rust crates.io #136 `zeroize`（9.35 亿总下载）+ #139 `subtle`（9.21 亿总下载）。

Zero-allocation constant-time cryptographic comparison, branchless selection/swap, and DSE-resistant memory zeroization primitives for CP — clean-room MIT parity with Rust `subtle` and `zeroize`.

## 安装 / Install

```bash
cap add @cap/subtle
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `Choice` | 恒定时间布尔类型（`val: 0 \| 1`）：`fromU8`, `fromBool`, `toBool`, `ctNot`, `ctAnd`, `ctOr`, `ctXor` |
| `ctEqU8` / `ctEqU32` / `ctEqU64` | 无分支整数相等比较 |
| `ctLtU64` / `ctGtU64` | 无分支、无溢出 64 位无符号大小比较 |
| `ctSelectU8` / `ctSelectU32` / `ctSelectU64` | 无分支条件选择 |
| `ctSwapU64(c, a, b)` | 无分支条件交换 |
| `ctEqBytes(a, b)` / `ctEqFixed32(a, b)` | 恒定时间字节串与 32 字节摘要（SHA-256 / HMAC）比对 |
| `ctCopyIf(c, dst, src)` | 无分支条件内存覆盖 |
| `zeroizeBytes(buf)` / `isAllZero(buf)` | 抗死存储消除（DSE）敏感内存安全清零与全零校验 |

## 使用示例 / Usage

```cp
import { Choice, ctEqBytes, ctSelectU64, zeroizeBytes, isAllZero } from "@cap/subtle";

const valid = ctEqBytes(computed_hmac, header_hmac).toBool();
const picked = ctSelectU64(Choice.fromBool(valid), 100 as u64, 0 as u64);

let secret: [32]u8 = undefined;
zeroizeBytes(secret[0..32]);
const wiped = isAllZero(secret[0..32]).toBool(); // true
```

## License

MIT
