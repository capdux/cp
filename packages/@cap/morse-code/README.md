# @cap/morse-code

Zero-heap ITU-R M.1677 International Morse Code encoder and decoder in pure CP.

## Features

- `encodeChar(c)` & `decodeSymbol(sym)`: supports `A-Z`/`a-z`, `0-9`, and `. , ? / - ( ) @`
- `encodeText(input, out)`: single-space between letters, `" / "` between words, collapsing consecutive whitespace
- `decodeText(morse, out)`: decodes Morse sequences and `/` word separators into uppercase ASCII text

## Usage

```cp
import { encodeText, decodeText } from "@cap/morse-code";

let buf: [64]u8 = undefined;
const n = encodeText("SOS", buf[0..64]); // "... --- ..."
```
