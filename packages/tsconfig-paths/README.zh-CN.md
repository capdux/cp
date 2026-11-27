[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tsconfig-paths

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation TypeScript tsconfig.json baseUrl + paths alias wildcard matcher and path resolver for CP

## 安装与导入

```bash
cap add tsconfig-paths
```

```cp
import { eqStr } from "tsconfig-paths";
```

## 核心导出 API

- `export fn eqStr(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn endsWith(s: string, suffix: string): bool;`
- `export fn findStar(s: string): usize?;`
- `export fn matchesPattern(pattern: string, specifier: string): PatternMatch;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn writeNormalizedPath(raw: string, out: []u8, start_pos: usize): usize;`
- `export fn expandTargetInto(base_url: string, target: string, captured: string, out: []u8, start_pos: usize): usize;`
- `export fn findBestRule(matcher: PathsMatcher, specifier: string): BestRuleHit;`
- `export fn resolvePath(matcher: PathsMatcher, specifier: string, out: []u8): string?;`
- `export fn resolveAllCandidates(matcher: PathsMatcher, specifier: string, out_buf: []u8): CandidateList;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`tsconfig-paths@1.0.0`)，许可证：MIT
