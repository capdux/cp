# @cap/potential-utf

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:potential_utf` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

Zero-allocation potentially-ill-formed UTF-8, UTF-16, and Unicode scalar value wrappers for CP — clean-room parity with ICU4X `potential_utf` (627M+ downloads on crates.io, Unicode-3.0).

纯 CP 零堆分配潜在非法 UTF-8 / UTF-16 / Unicode 码点安全包装库，完整支持 RFC 3629 严格校验、最长合法前缀提取、Unicode Maximal Subpart 容错替换（`U+FFFD`）与 UTF-16 代理对解码。

## Install / 安装

```bash
cap add @cap/potential-utf
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PotentialUtf8` | `fromBytes`, `fromStr`, `asBytes`, `len`, `isEmpty`, `isValidUtf8`, `toValidPrefix`, `decodeLossy(out)` | Potentially-ill-formed UTF-8 byte slice wrapper |
| `PotentialUtf16` | `fromSlice`, `asSlice`, `len`, `isEmpty`, `isValidUtf16`, `toUtf8Lossy(out)` | Potentially-ill-formed UTF-16 code unit slice wrapper |
| `PotentialCodePoint` | `fromU32`, `toU32`, `isValidScalar`, `toCharOrReplacement`, `encodeUtf8Lossy(out)` | Potentially-invalid 32-bit Unicode scalar value wrapper |

## Usage / 使用示例

```cp
import { PotentialUtf8, PotentialUtf16, PotentialCodePoint } from "@cap/potential-utf";

const s = PotentialUtf8.fromStr("hello");
const ok = s.isValidUtf8();
const cp = PotentialCodePoint.fromU32(0xD800 as u32);
const safe = cp.toCharOrReplacement(); // 0xFFFD
```
