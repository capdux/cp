[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# path-to-regexp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Express/Router URL path template matching, parameter extraction, and reverse route compilation (clean-room port of path-to-regexp v8)

## 安装与导入

```bash
cap add path-to-regexp
```

```cp
import { ptrStrEq } from "path-to-regexp";
```

## 核心导出 API

- `export fn ptrStrEq(a: string, b: string): bool;`
- `export fn ptrSegEq(a: string, b: string, case_sensitive: bool): bool;`
- `export fn ptrTrimSlashes(s: string): string;`
- `export fn ptrFindSegEnd(s: string, pos: usize): usize;`
- `export fn ptrSkipSlashes(s: string, pos: usize): usize;`
- `export fn ptrWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn ptrWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn ptrFindParam(params: &[]RouteParam, name: string): string?;`
- `export fn matchPath(pattern: string, path: string, case_sensitive: bool): RouteMatch;`
- `export fn compilePath(pattern: string, params: &[]RouteParam, out: []u8): usize;`
- `export fn countRouteParams(pattern: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`path-to-regexp@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`path-to-regexp@1.0.0`)，许可证：MIT
