[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/pem

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-pemfile` ([https://github.com/rustls/pemfile](https://github.com/rustls/pemfile)) / `crates.io:pem-rfc7468` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))（原库名 `rustls-pemfile` → CP 包名 `@cap/pem`）

Zero-heap RFC 7468 PEM armor parser, multi-block certificate bundle iterator, Base64 extractor, and 64-column line-wrapping encoder in pure CP (replaces Rust `pem` [55M] & npm `pem-jwk`/`sshpk`).

## Install

```bash
cap add @cap/pem
```

## Usage

```cp
import { parsePem, parsePemBundle, extractBase64, encodePem } from "@cap/pem";

let out: [512]u8 = undefined;
const pem = encodePem("PUBLIC KEY", "MCowBQYDK2VwAyEA...", out[0..512]);
const blk = parsePem(pem);
// blk.label == "PUBLIC KEY"
```

## API

| Function | Description |
| --- | --- |
| `parsePem(input)` | Parse the first RFC 7468 PEM block |
| `parsePemFrom(input, offset)` | Parse next PEM block starting at byte offset |
| `parsePemBundle(input)` | Parse up to 16 PEM blocks from a cert chain / bundle |
| `extractBase64(blk, out)` | Strip whitespace & RFC 1421 headers, returning clean Base64 |
| `encodePem(label, b64, out)` | Format label + Base64 with 64-column RFC 7468 wrapping |
