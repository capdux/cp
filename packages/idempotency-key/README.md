# @cap/idempotency

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:idempotency-key` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `idempotency-key` → CP 包名 `@cap/idempotency`）

Zero-allocation IETF `Idempotency-Key` HTTP header parser, validator, RFC 8941 formatter, and request fingerprint state evaluator for CP.

## Features

- **Zero heap allocation**: parses and formats directly over caller-supplied `[]u8` buffers.
- **RFC 8941 & bare key support**: `parseKeyHeader` handles quoted strings (`"key-123"`, unescaping `\"` / `\\`) and bare tokens, enforcing length `1..255` and visible ASCII `0x21..0x7E`.
- **Validation & formatting**: `isValidKey`, `isUuidV4Like`, and `formatKeyHeader`.
- **Conflict detection**: deterministic 64-bit FNV-1a `fingerprintRequest` and 4-state `evaluateState` (`IDEMP_NEW`, `IDEMP_REPLAY`, `IDEMP_IN_FLIGHT`, `IDEMP_CONFLICT`).

## Usage

```cp
import { parseKeyHeader, fingerprintRequest, evaluateState, IDEMP_REPLAY } from "@cap/idempotency";

let keyBuf: [255]u8 = undefined;
const klen = parseKeyHeader("\"550e8400-e29b-41d4-a716-446655440000\"", keyBuf[0..255]);
const fp = fingerprintRequest("POST", "/v1/charges", "{\"amount\":500}");
const state = evaluateState(true, false, fp, fp); // IDEMP_REPLAY
```
