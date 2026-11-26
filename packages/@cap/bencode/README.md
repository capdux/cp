# @cap/bencode

Zero-heap BitTorrent BEP-3 deterministic Bencode streaming encoder, zero-copy decoder, validator, and top-level dictionary lookup in pure CP.

## Features

- Streaming `BencodeWriter` into caller-provided `[]u8` buffer with overflow protection
- Strict `decodeInt` (rejects `i-0e`, `i03e`, `ie`, overflow) and `decodeBytes` (rejects leading-zero lengths & truncation)
- Full `validate(src)` enforcing canonical BEP-3 key ordering and zero trailing bytes
- Zero-copy `dictLookup(src, key)` returning the exact encoded value span `[start, start + len)`

## Usage

```cp
import { BencodeWriter, validate, dictLookup, decodeInt } from "@cap/bencode";

let buf: [128]u8 = undefined;
let w = BencodeWriter.init(buf[0..128]);
* = w.beginDict();
* = w.writeBytes("age");
* = w.writeInt(42);
* = w.end();

const encoded = w.written();
const valid = validate(encoded); // true
const found = dictLookup(encoded, "age");
const age = decodeInt(encoded, found.start); // value = 42
```
