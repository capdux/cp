[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/nanoid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:nanoid` ([https://github.com/ai/nanoid](https://github.com/ai/nanoid))

Clean-room MIT, zero-heap, URL-safe & custom-alphabet ID generator for CP — modeled on npm's `nanoid` v5 with bitmask rejection sampling to eliminate modulo bias.

## API
- Constants: `URL_ALPHABET` (64 chars), `DEFAULT_SIZE` (`21`)
- `nanoidFromBytes(bytes: []u8, size: usize, out: []u8): usize`
- `customAlphabetFromBytes(alphabet: string, bytes: []u8, size: usize, out: []u8): usize`
- `nanoidSeeded(seed: u64, size: usize, out: []u8): usize`
- `NanoIdGenerator` (`new`, `nextInto`, `nextCustomInto`)
- `isValidNanoId(id: string, alphabet: string): bool`
