[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# sfv

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sfv` ([https://github.com/undef1nd/sfv](https://github.com/undef1nd/sfv)) / `npm:ua-client-hints` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `sfv` → CP 包名 `sfv`）

Zero-allocation W3C User-Agent Client Hints (`Sec-CH-UA`, `Sec-CH-UA-Mobile`, `Sec-CH-UA-Platform`, `Sec-CH-UA-Full-Version-List`) RFC 8941 structured header parser and builder for CP.

## Features

- **Zero heap allocation**: zero-copy `BrandVersion` slicing and caller-supplied output buffers.
- **RFC 8941 structured list parsing**: `parseBrands` handles escaped quotes and embedded commas/semicolons.
- **GREASE filtering & lookup**: `isGreaseBrand` and case-insensitive `findBrandVersion`.
- **Scalar hints & formatting**: `parseMobile`, `parsePlatform`, and `formatBrands` with overflow-safe truncation.

## Usage

```cp
import { BrandVersion, parseBrands, isGreaseBrand, findBrandVersion, parseMobile, parsePlatform } from "sfv";

const ua = "\"Chromium\";v=\"124\", \"Google Chrome\";v=\"124\", \"Not-A.Brand\";v=\"99\"";
const ver = findBrandVersion(ua, "google chrome"); // "124"
const mob = parseMobile("?1"); // 1
const plat = parsePlatform("\"macOS\""); // "macOS"
```
