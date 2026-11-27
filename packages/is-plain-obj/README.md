[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/is-plain-obj

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-plain-obj` ([https://github.com/sindresorhus/is-plain-obj](https://github.com/sindresorhus/is-plain-obj))

Clean-room MIT, zero-heap plain object metadata checker, JSON object literal validator, and prototype-pollution key guard for CP — modeled on npm's `is-plain-obj` v4.

## API
- Constants: `PROTO_NULL`, `PROTO_OBJECT`, `PROTO_CUSTOM_CLASS`, `PROTO_ARRAY`, `PROTO_PRIMITIVE`, `PROTO_FUNCTION`
- `ValueMeta` (`proto_kind`, `has_to_string_tag`, `is_iterator`, `is_null`, `plain`, `nullProto`)
- `isPlainObjectMeta(m: ValueMeta): bool`
- `isPlainJsonObject(json_text: string): bool`
- `isPlainObjectKey(key: string): bool`
- `hasDangerousProtoKey(json_text: string): bool`
