[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/password-hash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:password-hash` ([https://github.com/RustCrypto/password-hashes](https://github.com/RustCrypto/password-hashes))

Zero-allocation pure CP implementation of the PHC string format (`$algorithm[$v=version][$k=v,...][$salt[$hash]]`) parser, serializer, parameter lookup, and constant-time comparison — clean-room parity with RustCrypto `password-hash` (165.0M+ downloads on crates.io, MIT).

纯 CP 零堆分配 PHC 密码哈希字符串格式解析与无损序列化库，支持完整 PHC 规范变体（含/不含版本、含/不含参数、仅盐值等）、严格字符集校验与常量时间哈希比较。

## Install / 安装

```bash
cap add @cap/password-hash
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PhcParam` | `struct { key: string; value: string; }` | Key-value parameter pair |
| `PhcHash` | `struct { algorithm, version, has_version, params, param_count, salt, hash, valid }` | Parsed PHC string representation |
| `parsePhc` | `(phc_str: string): PhcHash` | Parse a PHC string with strict character validation |
| `formatPhc` | `(h: PhcHash, out: []u8): usize` | Losslessly serialize a `PhcHash` into `out` |
| `getPhcParam` | `(h: PhcHash, key: string): string?` | Look up a parameter value by key |
| `ctEqual` | `(a: string, b: string): bool` | Constant-time byte comparison |

## Usage / 使用示例

```cp
import { parsePhc, formatPhc, getPhcParam, ctEqual } from "@cap/password-hash";

const s = "$argon2id$v=19$m=65536,t=3,p=4$c29tZXNhbHQ$RdescudvJCsgt3ub+b+dWRWJTmaaJObG";
const parsed = parsePhc(s);
const mem = getPhcParam(parsed, "m"); // "65536"
let buf: [128]u8 = undefined;
const n = formatPhc(parsed, buf[0..128]);
const ok = ctEqual(buf[0..n], s); // true
```
