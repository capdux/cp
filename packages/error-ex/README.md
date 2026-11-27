[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# error-ex

Zero-heap structured error and context formatter with `{key}` template interpolation in pure CP.

## API

- `ErrorField { key: string, value: string, line_mode: bool }`
- `StructuredError { name: string, code: i32, message: string }`
- `formatStructuredError(e: StructuredError, fields: []const ErrorField, out: []u8): usize`
- `formatErrorTemplate(tmpl: string, key: string, val: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`error-ex@1.3.2`)
- **Upstream Repository**: https://github.com/Qix-/node-error-ex
- **License**: MIT (clean-room CP implementation)
