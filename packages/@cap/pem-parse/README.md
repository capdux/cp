# @cap/pem-parse

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pem` ([https://github.com/jcreekmore/pem-rs](https://github.com/jcreekmore/pem-rs))（原库名 `pem` → CP 包名 `@cap/pem-parse`）

Zero-heap RFC 7468 PEM encapsulation parser, chain iterator, Base64 body decoder, and 64-column encoder in pure CP.

## Features

- `nextBlock(input, start)` & `countBlocks(input)`: zero-copy parsing and iteration over multi-block PEM chains with strict BEGIN/END label matching
- `decodeBase64Body(body_b64, out_der)`: strips CR/LF/whitespace and decodes RFC 4648 Base64 into DER bytes
- `encodePem(label, der, out)`: canonical 64-column wrapped RFC 7468 PEM encoder
- `isValidLabel(label)`: validates PEM encapsulation label characters

## Usage

```cp
import { encodePem, nextBlock, decodeBase64Body } from "@cap/pem-parse";

let pem: [256]u8 = undefined;
const payload: [4]u8 = [0x30, 0x02, 0x01, 0x00];
const n = encodePem("TEST BLOCK", payload[0..4], pem[0..256]);
const blk = nextBlock(pem[0..n], 0);
```
