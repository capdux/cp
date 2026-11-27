[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `stomp`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:stomp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap-allocation STOMP 1.2 messaging protocol frame encoder, decoder, and header escaper in pure CP.

## Features

- **Zero-Copy Decoding (`decodeFrame`)**: Parses commands, up to 16 headers, binary NUL-containing bodies via `content-length`, leading heartbeat EOLs (`\n`/`\r\n`), and streaming partial frames (`STOMP_INCOMPLETE`).
- **Frame Encoding (`encodeFrame`)**: Encodes `COMMAND\nk:v\n...\n\nBODY\0` into caller buffers with upfront overflow checking.
- **STOMP 1.2 Header Escaping (`escapeHeader` / `unescapeHeader`)**: Handles `\r`, `\n`, `\c` (`:`), and `\\`.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    STOMP_OK,
    STOMP_INCOMPLETE,
    STOMP_INVALID,
    MAX_HEADERS,
    StompHeader,
    StompFrame,
    escapeHeader,
    unescapeHeader,
    encodeFrame,
    decodeFrame,
    getHeader,
} from "stomp";
```

## License

MIT
