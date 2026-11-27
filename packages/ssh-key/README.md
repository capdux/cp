[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ssh-key

Zero-heap OpenSSH public key (ssh-ed25519, ssh-rsa, ecdsa-sha2-*, sk-ssh-*) authorized_keys parser, validator, and formatter for CP (compatible with npm sshpk & Rust ssh-key)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ssh-key` ([https://github.com/RustCrypto/SSH](https://github.com/RustCrypto/SSH)) / `npm:sshpk` ([https://github.com/TritonDataCenter/node-sshpk](https://github.com/TritonDataCenter/node-sshpk))

| Metadata | Value |
| --- | --- |
| Package | `ssh-key` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ssh-key
```

```cp
import {
    SshPublicKey,
    isHspace,
    identifyAlgorithm,
    isValidBase64,
    parsePublicKey,
    formatPublicKey,
    countAuthorizedKeys,
    isModernAlgorithm,
} from "ssh-key";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SshPublicKey` | `class` | Fields: `alg: u8, key_type: string, key_b64: string, comment: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isHspace` | `(c: u8): bool` | Checks predicate `isHspace` and returns `bool`. |
| `identifyAlgorithm` | `(key_type: string): u8` | Invokes `identifyAlgorithm` and returns `u8`. |
| `isValidBase64` | `(b64: string): bool` | Checks predicate `isValidBase64` and returns `bool`. |
| `parsePublicKey` | `(line: string): SshPublicKey` | Parses input via `parsePublicKey` and returns `SshPublicKey`. |
| `formatPublicKey` | `(key: SshPublicKey, out: []u8): usize` | Serializes/formats data via `formatPublicKey` returning `usize`. |
| `countAuthorizedKeys` | `(text: string): usize` | Invokes `countAuthorizedKeys` and returns `usize`. |
| `isModernAlgorithm` | `(alg: u8): bool` | Checks predicate `isModernAlgorithm` and returns `bool`. |

## Minimal Example

```cp
import { isValidBase64 } from "ssh-key";

pub fn main() {
    let result = isValidBase64("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
