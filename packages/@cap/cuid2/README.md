# @cap/cuid2

Zero-allocation deterministic, collision-resistant, URL-safe lowercase Base36 Cuid2 generator and validator with built-in FIPS 180-4 SHA-256 for CP — clean-room MIT parity with npm `@paralleldrive/cuid2` and Rust `cuid2`.

纯 CP 零堆分配 Cuid2 分布式唯一标识符生成器与校验器，内置标准 SHA-256 熵混合与单调计数器，零系统调用 100% 确定性可测。

## Install / 安装

```bash
cap add @cap/cuid2
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Cuid2Generator` | `init(fingerprint, seed, id_len)`, `next(*_, timestamp_ms, dst) -> usize`, `counterValue() -> u64` | Deterministic Cuid2 generator (`id_len` clamped `8..32`, default 24) |
| `isCuid2` | `(s: []const u8) -> bool` | Validate Cuid2 format (`2..32` chars, leading `'a'..'z'`, lowercase Base36) |
| `isCuid2WithLen` | `(s: []const u8, expected_len: usize) -> bool` | Validate Cuid2 format with exact length |

## Usage / 使用示例

```cp
import { Cuid2Generator, isCuid2 } from "@cap/cuid2";

let gen = Cuid2Generator.init(0x1234 as u64, 0x5678 as u64, 24 as usize);
let out: [32]u8 = undefined;
const n = gen.next(1700000000000 as u64, out[0..32]);
const ok = isCuid2(out[0..n]); // true
```
