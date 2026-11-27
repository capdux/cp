[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# dedent

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dedent` ([https://github.com/dmnd/dedent](https://github.com/dmnd/dedent))（原库名 `dedent` → CP 包名 `dedent`）

Zero-heap common leading indentation detector, dedent stripper, and line indenter for CP.
Compatible with npm `strip-indent`, `dedent`, and Rust `indoc`.

## API

- `minIndent(input: string): usize`
- `stripIndent(input: string, out: []u8): usize`
- `indentLines(input: string, spaces: usize, out: []u8): usize`

## License

MIT
