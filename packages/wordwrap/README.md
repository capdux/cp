[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# wordwrap

Zero-heap text word wrapper with left indentation, column stop width, soft/hard wrapping modes, and explicit newline preservation in pure CP.

## API

- `wrapText(input, start_col, stop_col, hard_mode, out): usize` — wraps text to fit within `[start_col, stop_col)` with optional left indent; keeps overlong words intact when `hard_mode == false` or breaks them at `stop_col - start_col` when `hard_mode == true`, preserving explicit `\n`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`wordwrap@1.0.0`)
- **Upstream Repository**: https://github.com/substack/node-wordwrap
- **License**: MIT (clean-room CP implementation)
