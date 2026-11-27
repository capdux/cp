# `@cap/content-disposition`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:content-disposition` ([https://github.com/jshttp/content-disposition](https://github.com/jshttp/content-disposition))

Zero-allocation RFC 6266 / RFC 5987 HTTP `Content-Disposition` response header parser, formatter, `filename*` decoder, and filename sanitizer for CP. Clean-room MIT implementation inspired by npm `content-disposition`.

## Features

- **Zero heap allocation**: Parses directly into zero-copy slices (`ContentDisposition`) and formats/decodes into caller-provided `[]u8` buffers.
- **RFC 6266 & RFC 5987 compliant**: Case-insensitive matching of `attachment`, `inline`, `filename`, and `filename*` via `@cap/unicase`.
- **Automatic UTF-8 `filename*` encoding & decoding**: Reuses `@cap/percent-encoding` (`componentSet`, `percentEncode`, `percentDecode`) with automatic ASCII fallback (`?`) and RFC 5987 `UTF-8''...` emission for non-ASCII filenames.
- **Path-traversal & header-injection protection**: Built-in `sanitizeFilename` strips `/` and `\` directory prefixes, control characters (`0x00..0x1F`, `0x7F`), and relative `.` / `..` names.

## API

### `ContentDisposition`

```cp
pub struct ContentDisposition {
    pub valid: bool,
    pub disp_type: string,
    pub filename: string,
    pub filename_star: string,

    pub static fn invalid(): ContentDisposition
    pub fn isAttachment(): bool
    pub fn isInline(): bool
    pub fn hasFilename(): bool
}
```

### Functions

- `parseContentDisposition(header: string): ContentDisposition` — Parses a `Content-Disposition` header value into disposition type, `filename`, and `filename*`.
- `formatContentDisposition(disp_type: string, filename: string, out: []u8): usize` — Formats `<disp_type>; filename="..."` for ASCII filenames, and automatically appends `; filename*=UTF-8''...` when `filename` contains non-ASCII UTF-8 bytes.
- `decodeFilenameStar(raw_star: string, out: []u8): usize` — Decodes RFC 5987 `charset'lang'encoded` values (`UTF-8`, `US-ASCII`, `ISO-8859-1`) into `out`.
- `sanitizeFilename(raw: string, out: []u8): usize` — Strips path traversal separators and control characters from a raw filename.

## Example

```cp
import {
    parseContentDisposition,
    formatContentDisposition,
    decodeFilenameStar,
    sanitizeFilename
} from "@cap/content-disposition";

let parsed = parseContentDisposition(
    "attachment; filename=\"report.pdf\"; filename*=UTF-8''%E6%8A%A5%E5%91%8A.pdf"
);
// parsed.isAttachment() == true
// parsed.filename == "report.pdf"

let name_buf: [128]u8 = undefined;
let name_len = decodeFilenameStar(parsed.filename_star, name_buf[0..128]);
// name_buf[0..name_len] == "报告.pdf"

let hdr_buf: [256]u8 = undefined;
let hdr_len = formatContentDisposition("attachment", "报告.pdf", hdr_buf[0..256]);
// hdr_buf[0..hdr_len] == "attachment; filename=\"??.pdf\"; filename*=UTF-8''%E6%8A%A5%E5%91%8A.pdf"
```

## License

MIT
