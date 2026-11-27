[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsesc` ([https://github.com/mathiasbynens/jsesc](https://github.com/mathiasbynens/jsesc)) | 许可：`MIT`

# `jsesc`

零堆分配纯 CP JavaScript / JSON 字符串转义库，完整对标 npm **#148** `jsesc` v3（真实周下载 **2.50 亿/周**，Babel / Mathias Bynens 核心依赖）。

## 核心 API

- `JsescOptions`：`quote` (`'`/`"`/`` ` ``)、`wrap`、`escape_everything`、`minimal`、`is_script_context`、`json`、`es6`、`lowercase_hex`
- `jsesc(input: string, out: []u8): usize`：默认单引号模式转义（含 `\b`/`\f`/`\n`/`\r`/`\t`/`\0`/`\\`、非 ASCII `\xHH`/`\uXXXX` 及 U+2028/U+2029 行分隔符）
- `jsescWithOptions(input: string, opts: JsescOptions, out: []u8): usize`：支持 `wrap`、`json` 严格 JSON 模式、`is_script_context`（转义 `</script` 与 `<!--`）、`es6`（`\u{XXXXX}` 码点转义）、`minimal`、`escape_everything` 与 `lowercase_hex`
