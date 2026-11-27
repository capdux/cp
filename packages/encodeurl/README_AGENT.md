# encodeurl (AI Agent Quick Ref)

Zero-heap idempotent URL percent-encoder (preserves existing `%XX`), normalizer, decoder, and validator.

## Import
```typescript
import {
    needsUrlEncode, encodedUrlLen,
    encodeUrl, encodeUrlChecked, encodeUrlNormalized,
    decodeUrl, isEncodedUrl,
} from "encodeurl";
```

## API Signatures
- `needsUrlEncode(b: u8): bool`
- `encodedUrlLen(url: string): usize`
- `encodeUrl(url: string, out: []u8): usize`
- `encodeUrlChecked(url: string, out: []u8): usize?` (`null` if `out.len < encodedUrlLen(url)`)
- `encodeUrlNormalized(url: string, out: []u8): usize` (also uppercases `%xx` -> `%XX`)
- `decodeUrl(url: string, out: []u8): usize`
- `isEncodedUrl(url: string): bool`

## Minimal Example
```typescript
import { encodeUrlChecked, decodeUrl } from "encodeurl";

fn safeLocation(raw: string, out: []u8): usize {
    if (let n = encodeUrlChecked(raw, out)) {
        return n;
    }
    return 0;
}
```

## Pitfalls & Borrowing Rules
- `encodeUrl` and `encodeUrlNormalized` truncate safely if `out` is too small; prefer `encodeUrlChecked` (returns `usize?`) or pre-check with `encodedUrlLen(url)` when truncation is unacceptable.
- Idempotency rule: valid `%XX` sequences (two hex digits) are kept as-is; bare `%` not followed by two hex digits is encoded as `%25`.
- Worst-case encoded size is `3 * url.len` (when every byte needs encoding); decoded size is always `<= url.len`.
- `out` must be a mutable byte slice (`[]u8`) and must not alias `url`.
- `encodeUrl` preserves URL structural delimiters (`/`, `?`, `#`, `&`, `=`, `:`, `@`) — it is NOT `encodeURIComponent`.
