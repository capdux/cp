# `@cap/destroy`

Zero-heap safe idempotent stream and file-descriptor destroyer with lifecycle completion guard — clean-room CP equivalent of npm `destroy`.

Reuses `@cap/on-finished` as an independent package.

## Features
- `DestroyableStream.generic()`, `DestroyableStream.readFile(fd, open)`, `DestroyableStream.zlib()`
- `destroyStream(s)` — idempotent teardown, handles `ReadStream` deferred close when `fd` opens late (`onOpen`)
- `destroyWithError(s, err_code)` — propagates error code to `@cap/on-finished` listeners while closing open descriptors
