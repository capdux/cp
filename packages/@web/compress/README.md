[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/compress

> Official HTTP response compression plugin for `cap:web` with pure-CP zero-heap RFC 1952 Gzip encoder.

- **License**: MIT
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { EncodingKind, negotiateEncoding } from "@web/compress";
  ```

## Overview

`@web/compress` handles `Accept-Encoding` quality-value negotiation (`gzip`, `deflate`, `identity`) and zero-heap RFC 1952 Gzip response compression for `cap:web`.

For full API signatures and examples, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
