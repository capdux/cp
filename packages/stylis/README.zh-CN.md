[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# stylis

> 版本：`4.3.6` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CSS minifier and nested selector preprocessor in pure CP

## 安装与导入

```bash
cap add stylis
```

```cp
import { stylIsWs } from "stylis";
```

## 核心导出 API

- `export fn stylIsWs(c: u8): bool;`
- `export fn stylPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn stylIsStripAfter(c: u8): bool;`
- `export fn stylIsStripBefore(c: u8): bool;`
- `export fn minifyCss(css: string, out: []u8): usize;`
- `export fn stylExpandSingle(parent: string, child: string, out: []u8, start_pos: usize): usize;`
- `export fn expandNestedSelector(parent: string, child: string, out: []u8): usize;`
- `export fn stylSkipCommentOrQuote(s: string, idx: usize): usize;`
- `export fn compileNestedCss(parent_sel: string, body_css: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`stylis@4.3.6`)（https://github.com/thysultan/stylis），许可证：MIT
