[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clean-stack` ([https://github.com/sindresorhus/clean-stack](https://github.com/sindresorhus/clean-stack)) | 许可：`MIT`

# `clean-stack`

零堆分配纯 CP 错误堆栈清理器，过滤 `node:internal/`、`internal/`、`(native)` 等运行时内部帧并支持 `pretty` 家目录缩写与 `base_path` 相对路径归一化，对标 npm Top-700 包 `clean-stack` v5。

## 核心 API

- `cleanStack(stack: string, out: []u8): usize` — 默认清理内部帧并剥离 `file://`
- `cleanStackWithOptions(stack: string, opts: CleanStackOptions, out: []u8): usize` — 支持 `pretty`（替换 `home_dir` 为 `~`）与 `base_path` 剥离
- `isInternalStackFrame(line: string): bool` — 判定单行是否为内部运行时帧
