[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-potential-custom-element-name` ([https://github.com/mathiasbynens/is-potential-custom-element-name](https://github.com/mathiasbynens/is-potential-custom-element-name)) | 许可：`MIT`

# `is-potential-custom-element-name`

零堆分配纯 CP WHATWG HTML Custom Elements 规范合法自定义元素名校验器，对标 npm **#407** `is-potential-custom-element-name`（真实周下载 **1.32 亿/周**）。

## 核心 API

- `isPotentialCustomElementName(name: string): bool`
- `isReservedCustomElementName(name: string): bool`
- `isPcenChar(cp: u32): bool`
