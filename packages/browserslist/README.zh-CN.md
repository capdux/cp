[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# browserslist

> 版本：`4.24.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Browserslist query clause parser and browser version matcher

## 安装与导入

```bash
cap add browserslist
```

```cp
import { blqStartsWithWord } from "browserslist";
```

## 核心导出 API

- `export enum BrowserOp`
- `export fn blqStartsWithWord(s: string, word: string): bool;`
- `export fn blqParseU32(s: string): u32;`
- `export fn blqIsDeadBrowser(browser: string): bool;`
- `export fn blqCmpVersion(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32;`
- `export fn parseBrowserClause(clause: string): BrowserClause;`
- `export fn matchBrowserVersion(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`browserslist@4.24.4`)（https://github.com/browserslist/browserslist），许可证：MIT
