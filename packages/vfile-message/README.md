[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# vfile-message

Zero-allocation structured lint/AST diagnostic message and `unist` `Point`/`Position` formatter in pure CP (covers `vfile-message` and `unist-util-position`).

## API

- `makePoint(line, column, offset)` / `makePosition(start, end)` / `pointAtOffset(text, offset)`
- `createMessage(reason, file_path, source, rule_id, severity, at): VFileMessage`
- `stringifyPosition(pos, out): usize` — formats `L:C` or `L1:C1-L2:C2`.
- `formatMessage(msg, out): usize` — formats `path:L:C: severity: reason [source:rule_id]`.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`vfile-message@4.0.2`, `unist-util-position@5.0.0`)
- **Upstream Repository**: https://github.com/vfile/vfile-message
- **License**: MIT (clean-room CP implementation)
