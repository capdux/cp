[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# clf-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Apache/Nginx Common Log Format (CLF) and Combined Log Format parser and formatter in pure CP

## 安装与导入

```bash
cap add clf-parser
```

```cp
import { invalidClf } from "clf-parser";
```

## 核心导出 API

- `export fn invalidClf(): ClfRecord;`
- `export fn isSpace(b: u8): bool;`
- `export fn isLineEnd(b: u8): bool;`
- `export fn parseClf(line: &[]u8): ClfRecord;`
- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushU64(dst: []u8, pos: usize, val: u64): i64;`
- `export fn formatClf(host: &[]u8, user: &[]u8, time: &[]u8, method: &[]u8, path: &[]u8, proto: &[]u8, status: u16, bytes: i64, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`clf-parser@1.0.0`)，许可证：MIT
