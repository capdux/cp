[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# powerfmt

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 powerfmt

## 安装与导入

```bash
cap add powerfmt
```

```cp
import { digitByte } from "powerfmt";
```

## 核心导出 API

- `export fn digitByte(d: u64, uppercase: bool): u8;`
- `export fn writeByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn writeRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;`
- `export fn formatMagnitude(mag: u64, sign_byte: u8, spec: IntFmtSpec, out: []u8): usize;`
- `export fn formatU64(val: u64, spec: IntFmtSpec, out: []u8): usize;`
- `export fn formatI64(val: i64, spec: IntFmtSpec, out: []u8): usize;`
- `export fn formatDecPadded(val: i64, width: usize, out: []u8): usize;`
- `export fn formatHex(val: u64, uppercase: bool, out: []u8): usize;`
- `export fn formatWithCommas(val: i64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`powerfmt@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`format-number@1.0.0`)，许可证：MIT
