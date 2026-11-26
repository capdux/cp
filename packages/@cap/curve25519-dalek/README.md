# @cap/curve25519-dalek

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:curve25519-dalek` ([https://github.com/dalek-cryptography/curve25519-dalek](https://github.com/dalek-cryptography/curve25519-dalek))

Zero-allocation constant-time RFC 7748 X25519 Diffie-Hellman and $\mathbb{F}_{2^{255}-19}$ field arithmetic for CP — clean-room parity with `curve25519-dalek` (354M+ downloads on crates.io, BSD-3-Clause).

纯 CP 零堆分配恒定时间 Curve25519 有限域（16×`i64` 无溢出 limbs）与 RFC 7748 X25519 Montgomery Ladder 实现，完整通过 RFC 7748 §5.2 与 §6.1 官方测试向量验证。

## Install / 安装

```bash
cap add @cap/curve25519-dalek
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `FieldElement` | `ZERO`, `ONE`, `fromBytes`, `toBytes`, `add`, `sub`, `mul`, `square`, `invert` | Constant-time $\mathbb{F}_{2^{255}-19}$ field element |
| `clampScalar` | `(k: [32]u8): [32]u8` | RFC 7748 §5 scalar clamping |
| `x25519` | `(scalar: [32]u8, u_coord: [32]u8): [32]u8` | RFC 7748 X25519 Montgomery Ladder |
| `x25519Basepoint` | `(scalar: [32]u8): [32]u8` | Public key derivation (`u = 9`) |

## Usage / 使用示例

```cp
import { x25519, x25519Basepoint } from "@cap/curve25519-dalek";

const alicePub = x25519Basepoint(alicePriv);
const shared = x25519(alicePriv, bobPub);
```
