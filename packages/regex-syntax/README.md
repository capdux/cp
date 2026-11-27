<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`regex-syntax`](https://github.com/rust-lang/regex)（`MIT OR Apache-2.0`）（包名 `@cap/regex-syntax`）。

# @cap/regex-syntax

Pure CP, zero-allocation regular expression syntax validator, metacharacter escaper, and literal prefix extractor (clean-room MIT parity with Rust `regex-syntax` v0.8).

## Design

- **Zero Heap Allocation**: Uses a fixed-size stack (up to 64 levels of nesting) to validate balanced groups, alternations, character classes, quantifiers, and escapes without dynamic memory allocation.
- **Exact Metacharacter Parity**: `isMetaCharacter` and `escapeRegex` match Rust `regex_syntax::is_meta_character` and `regex_syntax::escape`.
- **Literal Prefix Extraction**: `extractLiteralPrefix` extracts the mandatory static literal byte prefix (handling leading `^`/`\A`, escaped literals like `\.`, and rolling back optional atoms like `a?` / `a*`), ready to feed `@cap/memchr` prefiltering.

## API

### Types & Error Constants
- `RegexSyntaxInfo { valid, err_code, err_pos, capture_count, has_anchors, has_backref, min_len, literal_prefix_len }` with `isOk(): bool`
- Error codes: `RSX_OK`, `RSX_ERR_UNCLOSED_GROUP`, `RSX_ERR_UNOPENED_GROUP`, `RSX_ERR_UNCLOSED_CLASS`, `RSX_ERR_INVALID_CLASS_RANGE`, `RSX_ERR_EMPTY_CLASS`, `RSX_ERR_DANGLING_QUANTIFIER`, `RSX_ERR_DOUBLE_QUANTIFIER`, `RSX_ERR_INVALID_QUANTIFIER`, `RSX_ERR_TRAILING_BACKSLASH`, `RSX_ERR_INVALID_ESCAPE`, `RSX_ERR_INVALID_GROUP_HEADER`, `RSX_ERR_NESTING_LIMIT`

### Functions
- `isMetaCharacter(c: u8): bool`
- `escapeRegex(input: string, out: []u8): usize`
- `isWordByte(b: u8): bool`
- `validateRegexSyntax(pattern: string): RegexSyntaxInfo`
- `extractLiteralPrefix(pattern: string, out: []u8): usize`

## License

MIT
