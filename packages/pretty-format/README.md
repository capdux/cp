[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# pretty-format

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-format` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest))

零堆分配的纯 CP 结构化值格式化打印库，Clean-Room MIT 实现对标 npm Top 45 包 `pretty-format`（周下载 1 亿+）。

## 特性

- **零堆写入器**：`PrettyWriter` 基于固定 2048 字节栈缓冲，零动态内存分配
- **丰富原语支持**：内置 `writeNull`、`writeBool`、`writeInt`、`writeFloat`、`writeString`（支持 `\n`/`\r`/`\t`/`\"`/`\\` 转义）、`writeSymbol`、`writeFunction`
- **多行与紧凑模式**：支持 `indent` 缩进层级、`print_basic_prototype` 原型前缀与 `min` 单行紧凑模式
- **深度保护**：超过 `max_depth` 时自动截断并输出 `"[Object]"` / `"[Array]"`，安全屏蔽深层写入

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`jestjs/jest` (`pretty-format`)](https://github.com/jestjs/jest/tree/main/packages/pretty-format)。
