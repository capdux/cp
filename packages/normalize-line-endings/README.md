# @cap/normalize-line-endings

Zero-heap line ending detector and normalizer (`CRLF`/`CR` ↔ `LF`) in pure CP.

## API

- `detectLineEnding(input: string): LineEnding` (`None`, `Lf`, `Crlf`, `Cr`, `Mixed`)
- `normalizeToLf(input: string, out: []u8): usize`
- `normalizeToCrlf(input: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`normalize-line-endings@0.3.0`)
- **Upstream Repository**: https://github.com/derekdreery/normalize-line-endings
- **License**: Apache-2.0 (clean-room CP implementation)
