[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# memchr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation SWAR-accelerated byte and substring search engine (Rust memchr v2 parity)

## 安装与导入

```bash
cap add memchr
```

```cp
import { memchr } from "memchr";
```

## 核心导出 API

- `export fn memchr(needle: u8, haystack: &[]u8): i64;`
- `export fn memchr2(n1: u8, n2: u8, haystack: &[]u8): i64;`
- `export fn memchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;`
- `export fn memrchr(needle: u8, haystack: &[]u8): i64;`
- `export fn memrchr2(n1: u8, n2: u8, haystack: &[]u8): i64;`
- `export fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;`
- `export fn memchrCount(needle: u8, haystack: &[]u8): usize;`
- `export fn memchrFindAll(needle: u8, haystack: &[]u8, out_offsets: []usize): usize;`
- `export fn memmemFind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn memmemRfind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn countByte(needle: u8, haystack: &[]u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`memchr@1.0.0`)，许可证：MIT OR Unlicense
