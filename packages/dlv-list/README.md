[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# dlv-list

Zero-heap fixed-capacity index-backed doubly linked list (`DlvListI64`) with stable `u32` handles and O(1) free-slot reuse in pure CP.

## API

- `DLV_NONE: u32` (`0xFFFFFFFF`), `DLV_CAP: usize` (`128`)
- `DlvListI64.new(): DlvListI64`
- `pushFront(val: i64): u32` / `pushBack(val: i64): u32`
- `popFront(): i64?` / `popBack(): i64?`
- `get(idx: u32): i64?` / `remove(idx: u32): i64?`
- `front(): i64?` / `back(): i64?`
- `len(): usize` / `isEmpty(): bool`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`dlv-list@0.5.2`)
- **Upstream Repository**: https://github.com/sgodwinson/dlv-list
- **License**: MIT (clean-room CP implementation)
