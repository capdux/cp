[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`spdx`](https://github.com/EmbarkStudios/spdx)（`MIT OR Apache-2.0`）（包名 `@cap/spdx`）。

# @cap/spdx

Zero-allocation SPDX 2.3 open-source license expression parser, validator, and compatibility evaluator for CP — clean-room MIT parity with npm `spdx-expression-parse` (55M/wk), `spdx-satisfies` (30M/wk), and Rust `spdx` (40M).

纯 CP 零堆分配 SPDX 2.3 开源许可证表达式解析、校验与兼容性评估引擎，内置 40+ 高频 SPDX 许可证元数据与递归下降求值器。

## Install / 安装

```bash
cap add @cap/spdx
```

## Features / 核心特性

- **40+ Built-in SPDX Licenses**: Covers `MIT`, `Apache-2.0`, `BSD-2-Clause`, `BSD-3-Clause`, `ISC`, `Unlicense`, `CC0-1.0`, `Zlib`, `BSL-1.0`, `MPL-2.0`, `LGPL-*`, `GPL-*`, `AGPL-*`, `EUPL-1.2`, `EPL-2.0`, `CDDL-1.0`, `Artistic-2.0`, `BlueOak-1.0.0`, `0BSD`, `PostgreSQL`, `curl`, `FTL`, `WTFPL`, `CC-BY-4.0`, `CC-BY-SA-4.0`, `OpenSSL`, `NCSA`, `Python-2.0`, etc.
- **Full SPDX 2.3 Precedence**: Recursive-descent parser supporting `WITH` > `AND` > `OR`, nested parentheses `(...)`, `+` suffix (`GPL-2.0+`), `WITH <exception>` (`Apache-2.0 WITH LLVM-exception`), and `LicenseRef-` / `DocumentRef-` references.
- **Zero Heap Allocation**: Fixed-size stack buffers (`MAX_LICENSES = 32`, `MAX_DEPTH = 32`) with deterministic O(n) single-pass evaluation.

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `LicenseCategory` | `enum { Permissive, WeakCopyleft, StrongCopyleft, NetworkCopyleft, PublicDomain, Unknown }` | License copyleft / permissiveness classification |
| `LicenseInfo` | `struct { id: string, category: LicenseCategory, isOsiApproved: bool }` | Built-in SPDX license metadata |
| `SpdxResult` | `struct { valid: bool, allKnown: bool, errorOffset: usize, licenses: [32]string, licenseCount: usize }` | Validation & extracted license list result |
| `lookupLicense` | `(id: string) -> LicenseInfo?` | Look up category & OSI-approved status (supports `+` suffix) |
| `validateExpression` | `(expr: string) -> SpdxResult` | Validate SPDX syntax, check known licenses, and extract referenced IDs |
| `isPermissiveOnly` | `(expr: string) -> bool` | Evaluate whether a permissive/public-domain compliance path exists |
| `satisfies` | `(expr: string, allowed: []const string) -> bool` | Check if an expression is satisfied by an allowlist of licenses |

## Usage / 使用示例

```cp
import {
    LicenseCategory,
    lookupLicense,
    validateExpression,
    isPermissiveOnly,
    satisfies,
} from "@cap/spdx";

// 1. Metadata lookup
const mit = lookupLicense("MIT");

// 2. Expression validation & license extraction
const res = validateExpression("(Apache-2.0 WITH LLVM-exception OR MIT) AND ISC");
// res.valid == true, res.allKnown == true, res.licenseCount == 3

// 3. Permissive compliance check
const ok1 = isPermissiveOnly("MIT OR GPL-3.0-only");   // true (can choose MIT)
const ok2 = isPermissiveOnly("MIT AND GPL-3.0-only");  // false (GPL-3.0-only required)

// 4. Allowlist evaluation
const allowed: [2]string = ["MIT", "Apache-2.0"];
const ok3 = satisfies("(Apache-2.0 OR BSD-3-Clause) AND MIT", allowed[0..2]); // true
```
