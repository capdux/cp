[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-parse` ([https://github.com/jbgutierrez/path-parse](https://github.com/jbgutierrez/path-parse)) | 许可：`MIT`

# `path-parse`

零堆分配、零拷贝纯 CP `path.parse()` 路径拆解库，对标 npm **#238** `path-parse`（真实周下载 **1.96 亿/周**），完整支持 POSIX 与 Win32 路径拆解为 `{ root, dir, base, ext, name }`。

## 核心 API

- `parsePosix(path: string): ParsedPath`
- `parseWin32(path: string): ParsedPath`
