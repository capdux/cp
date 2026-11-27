[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ngeohash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ngeohash` ([https://github.com/sunng87/node-geohash](https://github.com/sunng87/node-geohash))（原库名 `ngeohash` → CP 包名 `ngeohash`）

Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP.

## Features

- **Zero heap allocation**: encodes directly into caller-supplied `[]u8` buffers.
- **Standard Geohash Base32**: `encode`, `decodeBounds`, `decode`, and `isValidGeohash` using `"0123456789bcdefghjkmnpqrstuvwxyz"`.

## Usage

```cp
import { encode, decode } from "ngeohash";

let buf: [12]u8 = undefined;
const n = encode(42.6 as f64, -5.6 as f64, 5, buf[0..12]); // "ezs42"
const pt = decode(buf[0..n]);
```
