# `cap:valkey` (`cap.valkey`) — RESP2/RESP3 Valkey & Redis Protocol Codec

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

Pure CP, zero-C-dependency implementation of the RESP2 and RESP3 wire protocols for Valkey and Redis, featuring full type modeling, resource-exhaustion bounds, incremental streaming parsing, and command/value encoding.

## Import

```cp
import {
    RespValue,
    RespStreamParser,
    RespLimits,
    ProtocolError,
    parseOne,
    parseOneWithLimits,
    parseInlineCommand,
    encodeValue,
    encodeCommand,
    encodeInlineCommand,
} from "cap:db";

// Or via dynamic namespace mounting:
import cap from "cap";
```

## Core Types

- **`RespType`**: `SimpleString` (`+`), `SimpleError` (`-`), `Integer` (`:`), `BulkString` (`$`), `Array` (`*`), `Null` (`_`), `Double` (`,`), `Boolean` (`#`), `BulkError` (`!`), `VerbatimString` (`=`), `Map` (`%`), `Set` (`~`), `Push` (`>`), `BigNumber` (`(`), `Attribute` (`|`).
- **`RespValue`**: Tagged union covering all RESP2 & RESP3 values with `.deinit(alloc)`, `.eql(other)`, `.isNull()`, `.asStr()`, `.asInt()`, `.asBool()`, and `.asDouble()`.
- **`VerbatimString`**: `{ format: []const u8, content: []const u8 }` (3-byte format tag such as `"txt"` or `"mkd"`).
- **`MapEntry`**: `{ key: RespValue, value: RespValue }`.
- **`RespLimits`**: Resource-exhaustion guard configuration:
  - `max_depth: usize = 32` (`DEFAULT_MAX_DEPTH`)
  - `max_bulk_len: usize = 64 * 1024 * 1024` (`DEFAULT_MAX_BULK_LEN`)
  - `max_collection_len: usize = 1024 * 1024` (`DEFAULT_MAX_COLLECTION_LEN`)
- **`ProtocolError`**: `Incomplete`, `DepthLimitExceeded`, `BulkTooLarge`, `CollectionTooLarge`, `InvalidTypeByte`, `InvalidInteger`, `InvalidDouble`, `InvalidBoolean`, `InvalidBulkLength`, `InvalidCollectionLength`, `InvalidVerbatimString`, `InvalidPush`, `InvalidInlineCommand`, `MissingCrLf`, `OutOfMemory`.

## Streaming Parser (`RespStreamParser`)

- `RespStreamParser.init(alloc)` / `RespStreamParser.initWithLimits(alloc, limits)`
- `parser.feed(chunk)` — Appends arbitrary byte slices (1..N bytes, half-packets, sticky packets).
- `parser.next()` — Returns `RespValue? ! ProtocolError`:
  - Returns `null` when more bytes are needed (`Incomplete`), preserving buffered bytes.
  - Returns an owned `RespValue` when a complete frame is decoded (caller must `val.deinit(alloc)`).
  - Returns `ProtocolError` immediately on malformed wire data or limit violations.
- `parser.setAllowInline(true)` — Enables inline command fallback (e.g. `PING\r\n`).
- `parser.bufferedLen()`, `parser.reset()`, `parser.deinit()`.

## Encoders & One-Shot Parsers

- `encodeCommand(alloc, args)` — Encodes `[]const []const u8` into a RESP2 Array command (`*N\r\n$len\r\narg\r\n`).
- `encodeInlineCommand(alloc, args)` — Encodes `[]const []const u8` into an inline command line (`arg0 arg1 ...\r\n`).
- `encodeValue(alloc, val, resp3)` — Encodes any `RespValue` into RESP3 (`resp3 == true`) or RESP2 (`resp3 == false`) wire bytes.
- `parseOne(alloc, data)` / `parseOneWithLimits(alloc, data, limits)` — Decodes a single frame, returning `ParseResult{ value, consumed }` or `ProtocolError.Incomplete`.
- `parseInlineCommand(alloc, data)` — Decodes an inline command line into a `RespValue.Array` of `.BulkString`.
