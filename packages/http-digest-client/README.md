[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `http-digest-client`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:http-digest-client` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:digest_auth` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `http-digest-client` → CP 包名 `http-digest-client`）

Zero-heap-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP.

## Features

- **`parseChallenge` / `formatChallenge`**: Handles `realm`, `nonce`, `opaque`, `qop`, `algorithm`, and `stale=true` with quoted comma/escape support.
- **`parseAuthorization` / `formatAuthorization`**: Handles `username`, `realm`, `nonce`, `uri`, `response`, `qop`, `nc`, `cnonce`, and `opaque` with required-field enforcement.
- **`supportsQop`**: Case-insensitive matching over comma-separated `qop` lists (`"auth, auth-int"`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    DigestChallenge,
    DigestResponse,
    parseChallenge,
    parseAuthorization,
    supportsQop,
    formatAuthorization,
    formatChallenge,
} from "http-digest-client";
```

## License

MIT
