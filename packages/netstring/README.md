[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/netstring`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:netstring` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap D. J. Bernstein Netstring (`<len>:<payload>,`) framing encoder and streaming zero-copy decoder in pure CP. Binary-safe, rejects leading zeros per spec, and cleanly distinguishes incomplete frames (`NS_INCOMPLETE`) from malformed frames (`NS_INVALID`).

零堆分配纯 CP Netstring（`<len>:<payload>,`）自定界二进制帧编解码器，适用于 SCGI、RPC 与流式消息分帧。

## Installation

```bash
cap add @cap/netstring
```

## API

| Function | Description |
| --- | --- |
| `encodedLen(payload_len)` | Exact encoded byte length |
| `encode(payload, dst)` | Encode payload into `<len>:<payload>,` (`-1` if buffer too small) |
| `decode(buf)` | Decode one frame returning `NetstringFrame` (`NS_OK`, `NS_INCOMPLETE`, or `NS_INVALID`) |

## License

MIT
