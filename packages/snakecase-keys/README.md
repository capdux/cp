[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# snakecase-keys

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:snakecase-keys` ([https://github.com/bendrucker/snakecase-keys](https://github.com/bendrucker/snakecase-keys))

Clean-room MIT, zero-heap object key `snake_case` converter for CP — modeled on npm's `snakecase-keys` v9 and directly composing `@cap/map-obj` and `@cap/heck-case`.

## API
- `SnakecaseKeysResult` (`empty`, `len`, `get`, `pushRaw`, `pushSnake`)
- `snakecaseKeys(src: MapObjResult, exclude: []string): SnakecaseKeysResult`
- `snakecaseKeyInto(key: string, out: []u8): usize`
