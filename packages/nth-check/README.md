# @cap/nth-check

Zero-allocation CSS `:nth-child(An+B)` microsyntax parser and 0-based index matcher in pure CP.

## API

- `parseNth(formula: string): NthRule` — parses `even`, `odd`, `2n+1`, `-n+3`, `3n - 2`, `5`, etc. into `{ a, b, valid }`.
- `rule.matches(index: usize): bool` — tests whether 0-based `index` (1st child = 0) satisfies the parsed rule.
- `nthCheck(formula: string, index: usize): bool` — one-shot parse & match helper.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`nth-check@2.1.1`)
- **Upstream Repository**: https://github.com/fb55/nth-check
- **License**: BSD-2-Clause (clean-room CP implementation)
