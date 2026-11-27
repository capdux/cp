[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# kebabcase-keys

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kebabcase-keys` ([https://github.com/sindresorhus/kebabcase-keys](https://github.com/sindresorhus/kebabcase-keys))

Clean-room MIT, zero-heap object key `kebab-case` converter for CP — modeled on npm's `kebabcase-keys` v1 and directly composing `@cap/decamelize-keys` and `@cap/map-obj`.

## API
- `kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult`
- `kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult`
- `kebabcaseKeyInto(key: string, out: []u8): usize`
- `kebabcasePathInto(dot_path: string, out: []u8): usize`
- `kebabcaseKeysBatch(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize`
