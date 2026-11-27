[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rend

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rend` ([https://github.com/rkyv/rend](https://github.com/rkyv/rend), `MIT`)

Zero-heap-allocation explicit-endianness primitive types (`U16Be`/`U16Le`, `U32Be`/`U32Le`, `I32Be`/`I32Le`, `U64Be`/`U64Le`, `I64Be`/`I64Le`) and bounds-checked byte-slice readers in pure CP (inspired by `rkyv/rend`).

## Features

- Endian-explicit fixed-byte wrapper structs (`fromNative`, `toNative`, `fromBytes`, `asBytes`):
  - 16-bit: `U16Be`, `U16Le`
  - 32-bit: `U32Be`, `U32Le`, `I32Be`, `I32Le`
  - 64-bit: `U64Be`, `U64Le`, `I64Be`, `I64Le`
- Bounds-checked slice readers returning optionals: `readU16Be`, `readU32Be`, `readU64Be`, `readU16Le`, `readU32Le`, `readU64Le`

## License

MIT
