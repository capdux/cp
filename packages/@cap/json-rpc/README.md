# @cap/json-rpc

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:jsonrpc-core` ([https://github.com/paritytech/jsonrpc](https://github.com/paritytech/jsonrpc)) / `npm:json-rpc-2.0` ([https://github.com/shawninder/json-rpc-2.0](https://github.com/shawninder/json-rpc-2.0)) / `crates.io:jsonrpsee` ([https://github.com/paritytech/jsonrpsee](https://github.com/paritytech/jsonrpsee))（原库名 `jsonrpc-core` → CP 包名 `@cap/json-rpc`）

Zero-heap JSON-RPC 2.0 protocol encoder, envelope validator, batch detector, and standard error codes in pure CP.

## Features

- Standard JSON-RPC 2.0 error codes (`ERR_PARSE`, `ERR_INVALID_REQUEST`, `ERR_METHOD_NOT_FOUND`, `ERR_INVALID_PARAMS`, `ERR_INTERNAL`) + `errorMessage` & `isServerError`
- `encodeRequest(id_json, method, params_json, out)`: supports numeric/string IDs, notifications (empty `id_json`), and optional `params`
- `encodeResult(id_json, result_json, out)` & `encodeError(id_json, code, message, data_json, out)` with JSON string escaping
- `isValidEnvelope(json)` & `isBatch(json)` zero-heap inspectors

## Usage

```cp
import { encodeRequest, isValidEnvelope } from "@cap/json-rpc";

let buf: [256]u8 = undefined;
const n = encodeRequest("1", "tools/list", "{}", buf[0..256]);
const ok = isValidEnvelope(buf[0..n]); // true
```
