# @cap/socks5-frame

Zero-allocation RFC 1928 SOCKS5 and RFC 1929 Username/Password subnegotiation frame codec for CP.

## Features

- **Zero heap allocation**: encodes directly into caller-supplied `[]u8` buffers and decodes zero-copy `Socks5Request` frames.
- **Handshake & Auth**: `encodeGreeting`, `selectMethod`, and RFC 1929 `encodeUserPassAuth`.
- **Requests & Replies**: `encodeConnectDomain`, `encodeConnectIpv4`, `decodeRequest` (IPv4 / DOMAIN / IPv6), and `encodeReplyIpv4`.
- **Streaming-safe**: distinguishes partial frames (`S5_INCOMPLETE`) from malformed frames (`S5_INVALID`).

## Usage

```cp
import { encodeConnectDomain, decodeRequest, Socks5Request, S5_OK } from "@cap/socks5-frame";

let frame: [64]u8 = undefined;
const n = encodeConnectDomain("example.com", 443 as u16, frame[0..64]);
```
