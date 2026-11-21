# `cap:brace_expand` — Zero-Allocation Shell Brace Expansion

Zero-allocation Bash-style brace expansion iterator supporting comma lists (`{a,b,c}`), integer ranges with optional step (`{0..10..2}`, `{5..1}`), zero-padded ranges (`{01..05}`), and ASCII letter ranges (`{a..e}`).

## Exports

- `BraceExpandIter` — `init(pattern)`, `next(*_, out): ([]const u8)?`.
