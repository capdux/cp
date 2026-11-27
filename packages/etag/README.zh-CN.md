[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# etag

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110 HTTP ETag generator, parser, and If-None-Match / If-Match evaluator in pure CP

## 安装与导入

```bash
cap add etag
```

```cp
import { etgIsSpace } from "etag";
```

## 核心导出 API

- `export fn etgIsSpace(b: u8): bool;`
- `export fn etgIsEtagChar(b: u8): bool;`
- `export fn etgStrEq(a: string, b: string): bool;`
- `export fn parseEtag(input: string, out: &mut EntityTag): bool;`
- `export fn formatEtag(weak: bool, tag: string, out: []u8): usize;`
- `export fn strongEqual(a: EntityTag, b: EntityTag): bool;`
- `export fn weakEqual(a: EntityTag, b: EntityTag): bool;`
- `export fn etgHexLower(nibble: u8): u8;`
- `export fn etgWriteHexU64(val: u64, out: []u8, start: usize): usize;`
- `export fn etgHexLenU64(val: u64): usize;`
- `export fn generateStatEtag(size: u64, mtime_ms: u64, out: []u8): usize;`
- `export fn etgFnv1a64(data: &[]u8): u64;`
- `export fn generateContentEtag(data: &[]u8, weak: bool, out: []u8): usize;`
- `export fn etgMatchHeaderList(header: string, current: EntityTag, require_strong: bool): bool;`
- `export fn matchIfNoneMatch(header: string, current: EntityTag): bool;`
- `export fn matchIfMatch(header: string, current: EntityTag): bool;`
- `export fn compareStrong(a: string, b: string): bool;`
- `export fn compareWeak(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`etag@1.0.0`)，许可证：MIT
