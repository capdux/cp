[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# prettier

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap JSON and CSS pretty-printer formatter (clean-room CP implementation inspired by npm prettier)

## 安装与导入

```bash
cap add prettier
```

```cp
import { pfIsSpace } from "prettier";
```

## 核心导出 API

- `export fn pfIsSpace(c: u8): bool;`
- `export fn pfPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pfPushIndent(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize;`
- `export fn pfPeekNextNonSpace(s: string, start: usize): u8;`
- `export fn formatJson(input: string, opts: FormatOptions, out: []u8): usize;`
- `export fn formatCss(input: string, opts: FormatOptions, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`prettier@1.0.0`)，许可证：MIT
