[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# convert-source-map

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation inline sourceMappingURL comment extractor, Base64 decoder/encoder, and comment stripper (clean-room MIT implementation of npm convert-source-map, #96 / 307M wk)

## 安装与导入

```bash
cap add convert-source-map
```

```cp
import { findSourceMapComment } from "convert-source-map";
```

## 核心导出 API

- `export enum SourceMapCommentKind`
- `export fn findSourceMapComment(src: string): SourceMapComment;`
- `export fn fromSource(src: string, out: []u8): i64;`
- `export fn toComment(json_map: string, multiline: bool, out: []u8): usize;`
- `export fn removeComments(src: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`convert-source-map@1.0.0`)，许可证：MIT
