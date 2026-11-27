[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# toml_writer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:toml_writer` ([https://github.com/toml-rs/toml](https://github.com/toml-rs/toml))（原库名 `toml_writer` → CP 包名 `toml_writer`）

Zero-heap TOML 1.0 serializer and formatter in pure CP (inspired by Rust `toml_writer`, 321M+ downloads).

Automatically chooses bare vs quoted keys, escapes control characters and quotes per TOML 1.0 spec, and renders `[table]`, `[[array_of_tables]]`, scalars, comments, and string arrays directly into caller-provided buffers.

## Install

```bash
cap add toml_writer
```

## Usage

```cp
import { TomlWriter } from "toml_writer";

let buf: [512]u8 = undefined;
let w = TomlWriter.init(buf[0..512]);
w.writeTableHeader("package");
w.writeKvStr("name", "capdux");
w.writeKvInt("port", 8080);
w.writeKvBool("tls", true);
```
