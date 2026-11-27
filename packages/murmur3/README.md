[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/murmur3

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:murmur3` ([https://github.com/stusmall/murmur3](https://github.com/stusmall/murmur3))

Zero-heap Austin Appleby MurmurHash3 (32-bit x86 & 128-bit x64) implementation in pure CP (replaces Rust `murmur3` & npm `murmurhash3js`).

Bit-exact reference implementation suitable for Kafka/Cassandra/ClickHouse partitioning, consistent hashing, and bloom filters.

## Install

```bash
cap add @cap/murmur3
```

## Usage

```cp
import { murmur3_32, murmur3_128 } from "@cap/murmur3";

const h32  = murmur3_32("hello", 0);  // 0x248bfa47
const h128 = murmur3_128("hello", 0); // Hash128{ lo, hi }
```
