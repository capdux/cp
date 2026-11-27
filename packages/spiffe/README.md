[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# spiffe

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:spiffe` ([https://github.com/maxlambrecht/rust-spiffe](https://github.com/maxlambrecht/rust-spiffe))（原库名 `spiffe` → CP 包名 `spiffe`）

Zero-allocation CNCF SPIFFE ID (`spiffe://<trust-domain>/<workload-path>`) parser, strict security validator, and formatter for CP.

## Features

- **Zero heap allocation**: zero-copy `SpiffeId` slicing and caller-supplied output buffers.
- **Strict CNCF SPIFFE compliance**: enforces lowercase `spiffe://` scheme, max 2048-byte total length, lowercase trust domain (`1..255`), and rejects `.` / `..` path traversal, empty `//` segments, trailing slashes, ports, userinfo, queries, and fragments.
- **Membership & formatting**: `belongsToTrustDomain`, `isValidTrustDomain`, `isValidPath`, and `formatSpiffeId`.

## Usage

```cp
import { parseSpiffeId, belongsToTrustDomain, formatSpiffeId } from "spiffe";

const id = parseSpiffeId("spiffe://prod.example.org/ns/default/sa/api-server");
const ok = belongsToTrustDomain(id, "prod.example.org"); // true
```
