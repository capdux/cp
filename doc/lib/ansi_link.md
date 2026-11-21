# `cap:ansi_link` — OSC 8 Terminal Hyperlink Codec

Zero-allocation pure-CP encoder, parser, detector, and stripper for terminal hyperlinks conforming to the OSC 8 specification (`\x1b]8;<params>;<url><TERM><text>\x1b]8;;<TERM>`), supporting both ST (`\x1b\\`) and BEL (`\x07`) terminators.

## Types

### `Osc8Link`

```cp
pub struct Osc8Link {
    url: []const u8;
    text: []const u8;
    params: []const u8;
}
```

Zero-copy view over a parsed OSC 8 hyperlink sequence (`params` holds key-value pairs such as `"id=doc1"` or `""`).

## Exports

- `formatOsc8(url: []const u8, text: []const u8, params: []const u8, out: []u8): ([]const u8)?` — Formats an ST-terminated OSC 8 hyperlink (`\x1b]8;<params>;<url>\x1b\\<text>\x1b]8;;\x1b\\`) into `out`. Returns `null` if `out` is too small or if `url`/`params` contain C0/DEL control bytes.
- `formatOsc8Bel(url: []const u8, text: []const u8, out: []u8): ([]const u8)?` — Formats a BEL-terminated OSC 8 hyperlink (`\x1b]8;;<url>\x07<text>\x1b]8;;\x07`) into `out`. Returns `null` if `out` is too small or if `url` contains control bytes.
- `parseOsc8(input: []const u8): Osc8Link?` — Parses a single complete OSC 8 hyperlink sequence (accepts ST or BEL terminators). Rejects unterminated sequences, missing delimiters, nested openings, or trailing bytes.
- `stripOsc8(input: []const u8, out: []u8): ([]const u8)?` — Strips all OSC 8 control sequences from `input` into `out`, preserving visible text and non-OSC-8 sequences. Returns `null` if `out` is too small.
- `hasOsc8(input: []const u8): bool` — Fast scan for any OSC 8 introducer (`\x1b]8;`).
- `osc8EncodedLen(url: []const u8, text: []const u8, params: []const u8): usize` — Exact byte length of an ST-terminated OSC 8 sequence.
- `osc8BelEncodedLen(url: []const u8, text: []const u8): usize` — Exact byte length of a BEL-terminated OSC 8 sequence.

## Example

```cp
import { formatOsc8, parseOsc8, stripOsc8, hasOsc8 } from "cap:ansi_link";

let buf: [256]u8 = [0; 256];
if (encoded := formatOsc8("https://cp-lang.dev", "CP Docs", "id=doc1", buf[0..])) {
    if (link := parseOsc8(encoded)) {
        // link.url == "https://cp-lang.dev"
        // link.text == "CP Docs"
        // link.params == "id=doc1"
    }
    let plain_buf: [64]u8 = [0; 64];
    if (plain := stripOsc8(encoded, plain_buf[0..])) {
        // plain == "CP Docs"
    }
}
```
