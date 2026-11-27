[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/encodeurl

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:encodeurl` ([https://github.com/pillarjs/encodeurl](https://github.com/pillarjs/encodeurl))

Clean-room MIT, zero-heap idempotent URL encoder for CP — modeled on npm's `encodeurl` v2 (180M+/week).

## API
- `encodeUrl(url, out): usize` — percent-encodes unsafe characters and lone `%` without double-encoding valid `%XX`
- `isEncodedUrl(url): bool` — checks whether `url` is already safely encoded
