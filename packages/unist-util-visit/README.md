[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# unist-util-visit

Zero-heap arena-based `unist` AST tree pre-order visitor, ancestor-chain collector, and node predicate matcher in pure CP (covers `unist-util-visit`, `unist-util-visit-parents`, and `unist-util-is`).

## API

- `newUnistTree(): UnistTree`
- `tree.addNode(kind, value, parent_idx): u32`
- `tree.isNode(idx, target_kind): bool`
- `tree.visitPreOrder(root_idx, target_kind, out): usize`
- `tree.getAncestors(node_idx, out): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`unist-util-visit@5.0.0`, `unist-util-visit-parents@6.0.1`, `unist-util-is@6.0.0`)
- **Upstream Repository**: https://github.com/syntax-tree/unist-util-visit
- **License**: MIT (clean-room CP implementation)
