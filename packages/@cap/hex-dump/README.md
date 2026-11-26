# `@cap/hex-dump`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hexdump-nodejs` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:pretty-hex` ([https://github.com/wolandr/pretty-hex](https://github.com/wolandr/pretty-hex))（原库名 `hexdump-nodejs` → CP 包名 `@cap/hex-dump`）

Zero-heap-allocation canonical `hexdump -C` binary debugger, spaced hex encoder, and first-diff locator in pure CP.

## Features

- **`formatLine`**: Formats up to 16 bytes into a canonical `hexdump -C` line with 8-digit hex offset, split 8+8 hex columns, and `|ASCII|` preview.
- **`formatDump`**: Multi-line `hexdump -C` dump with `\n` line separators.
- **`encodeHexSpaced`**: Lowercase or uppercase space-separated hex (`"de ad be ef"` / `"DE AD BE EF"`).
- **`firstDiffOffset`**: Locates the first differing byte offset between two slices (`-1` when equal).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { formatLine, formatDump, encodeHexSpaced, firstDiffOffset } from "@cap/hex-dump";
```

## License

MIT
