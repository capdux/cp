[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ssri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ssri` ([https://github.com/npm/ssri](https://github.com/npm/ssri))

Zero-allocation W3C Subresource Integrity (SRI) multi-algorithm parser, validator, constant-time matcher, and formatter for CP — clean-room MIT parity with npm `ssri` (85M/wk).

纯 CP 零堆分配 W3C Subresource Integrity (SRI) 多算法完整性校验串解析、恒定时间比对与规范化输出引擎，适用于 `cap.lock`、包管理校验与 `<script integrity="...">`。

## Install / 安装

```bash
cap add @cap/ssri
```

## Features / 核心特性

- **Multi-Algorithm Priority**: Supports `sha512` > `sha384` > `blake3` > `sha256` (`algoStrength`), plus unknown algorithm passthrough.
- **Strict Validation**: Validates Base64 character set, padding alignment, and optional `?options` parameters with zero heap allocation.
- **Constant-Time Matching**: `matchesIntegrity` compares digests in constant time to resist timing side channels.

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `SriAlgo` | `enum { Sha256, Sha384, Sha512, Blake3, Unknown }` | Hash algorithm enum |
| `SriHash` | `struct { valid, algo, algo_name, digest, options }` | Single parsed SRI hash entry |
| `SriIntegrity` | `struct { hashes: [16]SriHash, count: usize }` | Parsed multi-hash integrity set |
| `algoStrength` | `(algo: SriAlgo) -> u8` | Cryptographic strength rank (`Sha512 > Sha384 > Blake3 > Sha256 > Unknown`) |
| `parseIntegrity` | `(raw: string) -> SriIntegrity` | Parse whitespace-separated SRI entries |
| `strongestHash` | `(sri: SriIntegrity) -> SriHash?` | Pick strongest known algorithm entry |
| `pickAlgorithm` | `(sri: SriIntegrity, algo: SriAlgo) -> SriHash?` | Pick first entry matching `algo` |
| `matchesIntegrity` | `(expected: SriIntegrity, actual: SriIntegrity) -> bool` | Constant-time match on any shared known algorithm |
| `formatHash` | `(algo: SriAlgo, base64_digest: string, out: []u8) -> string` | Format `<algo>-<digest>` |
| `formatIntegrity` | `(sri: SriIntegrity, out: []u8) -> string` | Canonical space-separated SRI string |

## Usage / 使用示例

```cp
import {
    SriAlgo,
    parseIntegrity,
    strongestHash,
    matchesIntegrity,
    formatHash,
} from "@cap/ssri";

const expected = parseIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU= sha512-z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg==?ct=application/js");
const best = strongestHash(expected); // Sha512

const actual = parseIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=");
const ok = matchesIntegrity(expected, actual); // true
```
