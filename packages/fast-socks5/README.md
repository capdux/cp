[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# fast-socks5

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fast-socks5` ([https://github.com/dizda/fast-socks5](https://github.com/dizda/fast-socks5)) / `npm:socks` ([https://github.com/JoshGlazebrook/socks](https://github.com/JoshGlazebrook/socks))（原库名 `fast-socks5` → CP 包名 `fast-socks5`）

Zero-allocation RFC 1928 SOCKS5 and RFC 1929 Username/Password subnegotiation frame codec for CP.

## Features

- **Zero heap allocation**: encodes directly into caller-supplied `[]u8` buffers and decodes zero-copy `Socks5Request` frames.
- **Handshake & Auth**: `encodeGreeting`, `selectMethod`, and RFC 1929 `encodeUserPassAuth`.
- **Requests & Replies**: `encodeConnectDomain`, `encodeConnectIpv4`, `decodeRequest` (IPv4 / DOMAIN / IPv6), and `encodeReplyIpv4`.
- **Streaming-safe**: distinguishes partial frames (`S5_INCOMPLETE`) from malformed frames (`S5_INVALID`).

## Usage

```cp
import { encodeConnectDomain, decodeRequest, Socks5Request, S5_OK } from "fast-socks5";

let frame: [64]u8 = undefined;
const n = encodeConnectDomain("example.com", 443 as u16, frame[0..64]);
```
