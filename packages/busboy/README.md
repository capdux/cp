# @cap/multipart-boundary

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:busboy` ([https://github.com/mscdex/busboy](https://github.com/mscdex/busboy)) / `crates.io:multer` ([https://github.com/rousan/multer-rs](https://github.com/rousan/multer-rs))（原库名 `busboy` → CP 包名 `@cap/multipart-boundary`）

Zero-heap RFC 7578 `multipart/form-data` boundary extractor, part iterator, and field/filename parser in pure CP.

## Features

- `isValidBoundary(boundary)`: RFC 2046 validation (1..70 chars, valid `bchars`, no trailing space)
- `extractBoundary(content_type)`: zero-copy extraction of unquoted or quoted `boundary=` parameter
- `nextPart(payload, boundary, start)` & `countParts(payload, boundary)`: zero-copy part iteration extracting `headers`, `body`, `name`, `filename`, and `content_type`

## Usage

```cp
import { extractBoundary, nextPart } from "@cap/multipart-boundary";

const b = extractBoundary("multipart/form-data; boundary=----CapForm123");
const part = nextPart(body, b, 0);
```
