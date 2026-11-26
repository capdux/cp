# @cap/rot13

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rot13-cipher` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:rot13` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `rot13-cipher` → CP 包名 `@cap/rot13`）

Zero-heap ROT13, ROT47, arbitrary Caesar shift, and Atbash alphabet substitution utilities in pure CP.

## Features

- `rot13(input, out)` & `rot13InPlace(buf)`: symmetric 13-letter ASCII rotation preserving case and punctuation
- `rot47(input, out)`: symmetric 47-position rotation over printable ASCII `33..126`
- `caesar(input, shift, out)`: arbitrary positive or negative Caesar shift
- `atbash(input, out)`: mirror substitution (`A <-> Z`, `a <-> z`)

## Usage

```cp
import { rot13, rot47, caesar, atbash } from "@cap/rot13";

let buf: [32]u8 = undefined;
const n = rot13("Hello, World!", buf[0..32]); // "Uryyb, Jbeyq!"
```
