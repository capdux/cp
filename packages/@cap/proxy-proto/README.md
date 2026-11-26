# @cap/proxy-proto

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-protocol-js` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:ppp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `proxy-protocol-js` → CP 包名 `@cap/proxy-proto`）

Zero-heap HAProxy PROXY Protocol v1 (ASCII) and v2 (binary 12-byte signature) parser and encoder in pure CP.

## Features

- `isV1Prefix` / `isV2Signature`: instant protocol version detection
- `parseV1` & `encodeV1`: TCP4, TCP6, and UNKNOWN support with strict 107-byte limit, port range `0..65535`, and `PP_INCOMPLETE` partial-frame detection
- `parseV2` & `encodeV2Ipv4`: 16-byte binary header, LOCAL/PROXY command handling, and 28-byte IPv4/TCP frame codec

## Usage

```cp
import { encodeV2Ipv4, parseV2, ProxyV2Header, PP_OK } from "@cap/proxy-proto";

let frame: [28]u8 = undefined;
const n = encodeV2Ipv4([192, 168, 1, 10], [10, 0, 0, 1], 54321, 443, frame[0..28]);
let hdr: ProxyV2Header = undefined;
const st = parseV2(frame[0..n], &hdr); // PP_OK
```
