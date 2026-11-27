[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/borsh

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:borsh` ([https://github.com/near/borsh-rs](https://github.com/near/borsh-rs))

Zero-heap-allocation canonical little-endian Borsh binary serializer and deserializer in pure CP (inspired by NEAR / Solana `borsh`).

## Features

- `BorshWriter`: `writeBool`, `writeU8`, `writeU16Le`, `writeU32Le`, `writeU64Le`, `writeI32Le`, `writeI64Le`, `writeBytes` (4-byte LE length prefix), `writeString`, `writeOptionTag`
- `BorshReader`: strict `readBool` / `readOptionTag` (rejects any byte other than `0`/`1`), `readU8`..`readI64Le`, zero-copy bounds-checked `readBytesSpan`, `remaining`

## License

MIT
