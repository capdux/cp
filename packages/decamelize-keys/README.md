[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/decamelize-keys

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:decamelize-keys` ([https://github.com/sindresorhus/decamelize-keys](https://github.com/sindresorhus/decamelize-keys))

Clean-room MIT, zero-heap object key decamelizer for CP — modeled on npm's `decamelize-keys` v2 and directly composing `@cap/map-obj` and `@cap/decamelize`.

## API
- `DecamelizeKeysResult` (`empty`, `len`, `get`, `pushRaw`, `pushDecamelized`)
- `decamelizeKeys(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult`
