# @cap/regex-lite

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:regex-lite` ([https://github.com/rust-lang/regex](https://github.com/rust-lang/regex))

Pure CP, zero-allocation linear-time Thompson NFA regex engine (clean-room MIT parity with Rust `regex-lite` v0.1).

## Design & Reuse

- **Dual-Package Composition**:
  - Uses `@cap/regex-syntax` (`validateRegexSyntax`, `extractLiteralPrefix`, `isWordByte`) for full syntax validation, word-boundary checks, and mandatory literal-prefix extraction.
  - Uses `@cap/memchr` (`memmemFind`) to jump directly to literal-prefix candidates before invoking the NFA.
- **ReDoS-Proof Linear Time**: Compiles patterns into up to 128 Thompson NFA bytecode instructions executed by a PikeVM-style dual-list simulator with priority cutoff (supporting both greedy and lazy `*?`/`+?`/`??` quantifiers in strict `O(N * M)` time).
- **Zero Heap Allocation**: All NFA instructions, 256-bit character class bitsets, and simulation state arrays live on the stack.

## API

- `RegexMatch { matched: bool, start: usize, end: usize }` with `len(): usize`
- `RegexLite.compile(pattern: string): RegexLite`
- `isOk(): bool`, `errCode(): i32`
- `isMatch(text: string): bool`
- `find(text: string): RegexMatch`
- `findAt(text: string, start_pos: usize): RegexMatch`
- `findAll(text: string, out_matches: []RegexMatch): usize`
- `replaceAll(text: string, replacement: string, out: []u8): usize`
- `splitCount(text: string): usize`

## License

MIT
