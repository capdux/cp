[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# cobs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cobs` ([https://github.com/jamesmunns/cobs.rs](https://github.com/jamesmunns/cobs.rs))

Zero-allocation Consistent Overhead Byte Stuffing (COBS), custom-sentinel COBS/XOR, framed codec, and streaming `CobsDecoder` for CP — clean-room MIT parity with Rust `cobs` and Cheshire & Baker (1999).

零堆分配的 CP COBS 无歧义字节填充与帧同步编码引擎，广泛用于串口通信、嵌入式协议、RPC 与二进制流分帧（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add cobs
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `max_encoded_len` | `(source_len: usize) -> usize` | 计算 COBS 编码最大字节数 |
| `encode` / `decode` | `(src, dst) -> usize / i64` | 标准 COBS 编解码（消除全部 `0x00`） |
| `encode_with_sentinel` / `decode_with_sentinel` | `(src, dst, sentinel: u8)` | 自定义哨兵字节 COBS/XOR 编解码 |
| `encode_framed` / `decode_framed` | `(src, dst)` | 带尾随 `0x00` 帧定界符的 COBS 编解码 |
| `CobsDecoder` | `init`, `push(*_, byte) -> i64`, `frame() -> []const u8`, `reset(*_)` | 内置 512 字节缓冲的流式增量帧解码器 |

## License

MIT
