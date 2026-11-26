# @cap/content-disposition

Zero-heap RFC 6266 / RFC 5987 HTTP `Content-Disposition` header formatter and parser in pure CP (npm `content-disposition` parity).

## Features

- Supports `inline` (`DISP_INLINE`), `attachment` (`DISP_ATTACHMENT`), and `form-data` (`DISP_FORM_DATA`)
- `format(kind, filename, dst)`: strips directory path prefixes (`/` and `\`), quotes ASCII filenames, and automatically emits both ASCII fallback `filename="..."` and RFC 5987 `filename*=UTF-8''...` for non-ASCII filenames
- `parse(header, filename_out)`: prioritizes `filename*=UTF-8''...` percent-decoding, falls back to quoted-string (with escape handling) or bare token `filename=...`

## Usage

```cp
import { format, parse, DISP_ATTACHMENT } from "@cap/content-disposition";

let hdr: [256]u8 = undefined;
const n = format(DISP_ATTACHMENT, "report.pdf", hdr[0..256]);
// hdr[0..n] => attachment; filename="report.pdf"
```
