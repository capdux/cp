[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# split2

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation streaming line splitter across arbitrary chunk boundaries (LF & CRLF) in pure CP

## 安装与导入

```bash
cap add split2
```

```cp
import { sp2Min } from "split2";
```

## 核心导出 API

- `export fn sp2Min(a: usize, b: usize): usize;`
- `export fn sp2CopyBytes(dst: []u8, dst_off: usize, src: &[]u8, n: usize);`
- `export fn splitLines(input: string, out_slices: []string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`split2@1.0.0`)，许可证：ISC
