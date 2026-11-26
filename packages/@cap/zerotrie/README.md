# @cap/zerotrie

Zero-heap-allocation compact left-child right-sibling prefix trie (`ZeroTrie`) for mapping byte-string keys to `u32` values using caller-provided node pools in pure CP (inspired by ICU4X / Rust `zerotrie`).

## Features

- Compact `TrieNode { ch, has_value, value, first_child, next_sibling }` (saves 90%+ memory vs 256-way fanout arrays)
- Atomic `insert`: verifies remaining pool capacity before allocating nodes
- `init`, `insert`, `get`, `containsKey`, `hasPrefix`, `longestPrefixMatch` (returns `PrefixMatch { matched_len, value, found }`)

## License

MIT
