[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/mongo-uri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mongodb-connection-string-url` ([https://github.com/mongodb-js/devtools-shared](https://github.com/mongodb-js/devtools-shared)) / `crates.io:mongodb` ([https://github.com/mongodb/mongo-rust-driver](https://github.com/mongodb/mongo-rust-driver))（原库名 `mongodb-connection-string-url` → CP 包名 `@cap/mongo-uri`）

Zero-allocation MongoDB connection string (`mongodb://` and `mongodb+srv://`) parser, query option extractor, and password-redacting formatter for CP.

## Features

- **Zero heap allocation**: zero-copy `MongoUri` with fixed 8-host replica-set seedlist and caller-supplied `[]u8` output buffers.
- **Standard & SRV support**: parses `mongodb://` (default port `27017`, multi-host replica sets, IPv6 `[::1]:27017`) and `mongodb+srv://` (enforces single host without explicit port, defaults `tls = true`).
- **Options & redaction**: parses `replicaSet`, `authSource`, `tls`/`ssl`, provides zero-copy `getQueryOption`, and `formatMongoUri(cfg, true, out)` masks passwords as `***`.

## Usage

```cp
import { parseMongoUri, formatMongoUri, getQueryOption } from "@cap/mongo-uri";

const cfg = parseMongoUri("mongodb+srv://app:s3cr3t@cluster0.example.net/prod?retryWrites=true");
let buf: [128]u8 = undefined;
const n = formatMongoUri(cfg, true, buf[0..128]);
// "mongodb+srv://app:***@cluster0.example.net/prod"
```
