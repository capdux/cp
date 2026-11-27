[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# uuencode

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap POSIX/Unix uuencode and uudecode codec supporting 45-byte lines, backtick/space zero encoding, and begin/end frames.

## 安装与导入

```bash
cap add uuencode
```

```cp
import { uuEncChar } from "uuencode";
```

## 核心导出 API

- `export fn uuEncChar(v: u8): u8;`
- `export fn uuDecVal(c: u8): i32;`
- `export fn bytesEq(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn encodeLine(data: &[]u8, out: []u8): usize;`
- `export fn decodeLine(line: string, out: []u8): i64;`
- `export fn encode(data: &[]u8, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`uuencode@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`uuencode@1.0.0`)，许可证：MIT
