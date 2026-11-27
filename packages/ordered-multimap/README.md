[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ordered-multimap

Zero-heap insertion-ordered multimap (`OrderedMultiMap`) supporting multiple values per key in pure CP.

## API

- `OMM_CAP: usize` (`128`)
- `OrderedMultiMap.new(): OrderedMultiMap`
- `append(key: string, val: string): bool` — append value for key (preserves multiple values)
- `insert(key: string, val: string): bool` — replace all existing values for key with a single value
- `getFirst(key: string): string?` / `getAll(key: string, out: []string): usize`
- `removeAll(key: string): usize` / `containsKey(key: string): bool`
- `len(): usize` / `keysLen(): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`ordered-multimap@0.7.3`)
- **Upstream Repository**: https://github.com/sgodwinson/ordered-multimap
- **License**: MIT (clean-room CP implementation)
