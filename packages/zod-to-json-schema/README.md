[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/zod-to-json-schema

Zero-heap JSON Schema (Draft-07) builder and serializer in pure CP, ideal for AI Agent tool-calling parameter definitions and structured output contracts.

## API

- `SchemaType` (`String`, `Number`, `Integer`, `Boolean`, `Array`, `Object`, `Null`)
- `prop(name, kind, description, required)` / `stringProp(name, desc, format, required)` / `intProp(name, desc, required, min, max)` / `arrayProp(name, item_kind, desc, required)`
- `toJsonSchema(title, props, additional_properties, out): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`zod-to-json-schema@3.24.1`)
- **Upstream Repository**: https://github.com/StefanTerdell/zod-to-json-schema
- **License**: MIT (clean-room CP implementation)
