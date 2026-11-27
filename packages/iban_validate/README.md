[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/iban

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:iban_validate` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `iban_validate` → CP 包名 `@cap/iban`）

Zero-heap ISO 13616 IBAN validator, ISO 7064 Mod-97-10 checksum engine, check-digit generator, and electronic/friendly formatter in pure CP.

## Features

- `countryLength(c0, c1)`: built-in ISO 13616 length table covering 88 countries/territories
- `electronicFormat(input, out)` & `friendlyFormat(input, out)`: strips spaces/hyphens, uppercases, or groups into 4-character blocks
- `mod97(electronic)`: overflow-free incremental ISO 7064 Mod 97-10 calculation
- `isValid(input)`: full length, country code, character set, and Mod-97 validation
- `computeCheckDigits(c0, c1, bban, &d0, &d1)`: computes `98 - mod97(CC00 + BBAN)` check digits

## Usage

```cp
import { isValid, computeCheckDigits } from "@cap/iban";

const ok = isValid("DE89 3704 0044 0532 0130 00"); // true
```
