[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/destroy`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:destroy` ([https://github.com/stream-utils/destroy](https://github.com/stream-utils/destroy))

Zero-heap safe idempotent stream and file-descriptor destroyer with lifecycle completion guard — clean-room CP equivalent of npm `destroy`.

Reuses `@cap/on-finished` as an independent package.

## Features
- `DestroyableStream.generic()`, `DestroyableStream.readFile(fd, open)`, `DestroyableStream.zlib()`
- `destroyStream(s)` — idempotent teardown, handles `ReadStream` deferred close when `fd` opens late (`onOpen`)
- `destroyWithError(s, err_code)` — propagates error code to `@cap/on-finished` listeners while closing open descriptors
