# @cap/multipart

Clean-room MIT, zero-heap RFC 7578 `multipart/form-data` streaming reader, writer, and boundary extractor for CP — modeled on crates.io `multipart` / `multer`, built on [`@cap/mediatype`](../mediatype/README.md) and [`@cap/unicase`](../unicase/README.md).

## Features

- **Zero heap allocation & zero-copy slicing**: `MultipartReader` streams parts directly as slices over the input body without copying payloads.
- **RFC 7578 Writer**: `MultipartWriter` builds compliant `multipart/form-data` payloads (text fields, file uploads with Content-Type, closing boundary) into a caller-provided `[]u8` buffer.
- **Case-insensitive Headers & Boundary Extraction**: reuses `@cap/mediatype` for `extractBoundary` and `@cap/unicase` for case-insensitive header and parameter matching.

## Dependencies

- `@cap/mediatype` `1.0.0`
- `@cap/unicase` `1.0.0`

## API

### Types
- `MultipartPart { name: string, filename: string, content_type: string, data: string, is_file: bool }`
- `MultipartReader`:
  - `MultipartReader.new(boundary: string, body: string): MultipartReader`
  - `nextPart(*_): MultipartPart?`
  - `countParts(): usize`
- `MultipartWriter`:
  - `MultipartWriter.new(boundary: string): MultipartWriter`
  - `writeField(name: string, value: string, out: []u8, pos: usize): usize`
  - `writeFile(name: string, filename: string, content_type: string, data: string, out: []u8, pos: usize): usize`
  - `finish(out: []u8, pos: usize): usize`

### Functions
- `extractBoundary(content_type: string): string?`
- `formatContentTypeHeader(boundary: string, out: []u8): usize`

## License

MIT
