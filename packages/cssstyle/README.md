<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`cssstyle`](https://github.com/jsdom/cssstyle)（`MIT`）（包名 `@cap/cssstyle`）。

# `@cap/cssstyle`

Zero-allocation CSS inline style declaration parser, query, mutator, and serializer in pure CP (clean-room MIT implementation inspired by npm `cssstyle`).

## Features

- **`CssProp`** — Property entry (`name`, `value`, `important`).
- **`CssStyleDecl`** — Fixed 64-slot stack-allocated CSS declaration list with `parse`, `getPropertyValue`, `getPropertyPriority`, `setProperty`, `removeProperty`, `len`, and `toCssText`.
- **Full Inline CSS Semantics** — Ignores `/* ... */` comments, case-insensitive property matching, `!important` parsing & serialization, and in-place overwrite on duplicate property names.

## Usage

```cp
import { CssProp, CssStyleDecl } from "@cap/cssstyle";

let style: CssStyleDecl = CssStyleDecl.new();
style.parse("color: red; font-size: 14px !important;");
```

## License

MIT
