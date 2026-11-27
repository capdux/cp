[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/stack-utils

Zero-heap V8 stack trace line parser in pure CP supporting named frames, anonymous frames, `file://` URIs, and native frames.

## API

- `StackFrame { function_name, file, line, column, is_native, valid }`
- `parseStackLine(line): StackFrame`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`stack-utils@2.0.6`)
- **Upstream Repository**: https://github.com/tapjs/stack-utils
- **License**: MIT (clean-room CP implementation)
