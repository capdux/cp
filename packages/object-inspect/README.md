[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# object-inspect

Zero-heap debug value inspector and pretty-formatter in pure CP supporting quoted strings with escape sequences and truncation, integers, booleans, `<Buffer ...>` hex previews, and string arrays.

## API

- `defaultInspectOptions(): InspectOptions`
- `inspectString(s, opts, out): usize`
- `inspectBool(v, out): usize`
- `inspectInt(val, out): usize`
- `inspectBytes(data, max_bytes, out): usize`
- `inspectStringArray(items, opts, out): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`object-inspect@1.13.4`)
- **Upstream Repository**: https://github.com/inspect-js/object-inspect
- **License**: MIT (clean-room CP implementation)
