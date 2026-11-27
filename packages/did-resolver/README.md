[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# did-resolver

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:did-resolver` ([https://github.com/decentralized-identity/did-resolver](https://github.com/decentralized-identity/did-resolver)) / `crates.io:ssi-dids` ([https://github.com/spruceid/ssi](https://github.com/spruceid/ssi))（原库名 `did-resolver` → CP 包名 `did-resolver`）

Zero-heap W3C DID Core 1.0 Decentralized Identifier & DID URL parser, strict character validator, and formatter in pure CP.

## Features

- `isValidMethod(method)`: enforces lowercase `[a-z0-9]+`
- `isValidMethodId(id)`: enforces `[A-Za-z0-9._:-]` + valid `%XX` escapes and no trailing `:`
- `parseDid(input)`: zero-copy parsing of `did:<method>:<id>[/path][?query][#fragment]`
- `formatDid(method, id, fragment, out)`: validated serialization into caller buffer

## Usage

```cp
import { parseDid, formatDid } from "did-resolver";

const d = parseDid("did:web:example.com:user:alice#key-1");
// d.did => "did:web:example.com:user:alice", d.fragment => "key-1"
```
