[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# github-slugger

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Markdown heading outline extractor, GitHub-compatible anchor slugger & automatic nested TOC generator (github-slugger / markdown-toc parity) for CP

## 安装与导入

```bash
cap add github-slugger
```

```cp
import { githubSlug } from "github-slugger";
```

## 核心导出 API

- `export fn githubSlug(heading: string, out: []u8): string;`
- `export fn extractToc(markdown: string): TocResult;`
- `export fn renderTocMarkdown(toc: TocResult, min_level: u8, max_level: u8, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`github-slugger@1.0.0`)，许可证：ISC
