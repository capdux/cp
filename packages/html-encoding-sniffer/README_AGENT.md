# html-encoding-sniffer — AI Agent Quick Ref

Zero-allocation WHATWG HTML 13.2.3.2 byte stream encoding sniffer and label normalizer (npm html-encoding-sniffer parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SniffOptions,
    hesToLower,
    hesEqIgnoreCase,
    hesIsAsciiWs,
    hesTrimAsciiWs,
    hesStartsWithIgnoreCase,
    normalizeEncodingLabel,
    hesMapMetaEncoding,
} from "html-encoding-sniffer";
```

## Core Types & Constants

```cp
class SniffOptions { transport_layer_encoding: string, default_encoding: string }
```

## API Signatures

```cp
fn hesToLower(ch: u8): u8;
fn hesEqIgnoreCase(a: string, b: string): bool;
fn hesIsAsciiWs(ch: u8): bool;
fn hesTrimAsciiWs(s: string): string;
fn hesStartsWithIgnoreCase(bytes: &[]u8, pos: usize, needle: string): bool;
fn normalizeEncodingLabel(label: string): string;
fn hesMapMetaEncoding(label: string): string;
fn hesExtractCharsetFromContent(content: string): string;
fn hesPrescanMeta(bytes: &[]u8): string;
fn sniffHtmlEncoding(bytes: &[]u8, opts: SniffOptions): string;
```

## Minimal Usage

```cp
import { hesEqIgnoreCase } from "html-encoding-sniffer";

pub fn main() {
    let result = hesEqIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"html-encoding-sniffer"`; install via `cap add html-encoding-sniffer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
