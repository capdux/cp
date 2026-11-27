[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`object-hash`](https://github.com/puleos/object-hash)（`MIT`）（包名 `object-hash`）。

# `object-hash`

Zero-allocation deterministic, key-order-independent structured object hasher in pure CP (clean-room MIT implementation inspired by npm `object-hash`).

## Features

- **`HashField`** — Typed key-value pair (`kind`: `0=null`, `1=bool`, `2=number`, `3=string`).
- **`hashFieldsFnv64(fields)`** — Sorts fields lexicographically by key and computes canonical FNV-1a 64-bit hash over `kind:key=value`.
- **`hashFieldsHex(fields, out)`** — Writes 16-byte lowercase hex digest into `out`.
- **`hashStringFnv64(s)` / `hashBytesFnv64(data)`** — Standard FNV-1a 64-bit primitives.

## Usage

```cp
import { HashField, hashFieldsFnv64, hashFieldsHex } from "object-hash";
```

## License

MIT
