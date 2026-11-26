# @cap/resp

Zero-heap Redis RESP2 wire protocol streaming frame parser and command encoder in pure CP.

## Features

- `parseFrame(buf)`: zero-copy streaming parser for Simple String (`+`), Error (`-`), Integer (`:`), binary-safe Bulk String (`$`), Null (`$-1\r\n`), and Array (`*`) headers, distinguishing `RESP_INCOMPLETE` from `RESP_ERROR_SYNTAX`
- `encodeCommand(args, dst)`: builds standard Redis RESP2 array commands (`*N\r\n$...\r\n`)
- `encodeSimpleString`, `encodeBulkString`, `encodeInteger`, `encodeNull`

## Usage

```cp
import { encodeCommand, parseFrame } from "@cap/resp";

let buf: [128]u8 = undefined;
const args: [3][]const u8 = ["SET", "k", "hello"];
const n = encodeCommand(args[0..3], buf[0..128]);
```
