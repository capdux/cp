[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ripemd

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP RIPEMD-160 160-bit cryptographic hash function (crates.io ripemd equivalent)

## 安装与导入

```bash
cap add ripemd
```

```cp
import { rmdAdd } from "ripemd";
```

## 核心导出 API

- `export fn rmdAdd(a: u32, b: u32): u32;`
- `export fn rmdAdd3(a: u32, b: u32, c: u32): u32;`
- `export fn rmdAdd4(a: u32, b: u32, c: u32, d: u32): u32;`
- `export fn rmdRol(x: u32, n: u32): u32;`
- `export fn rmdNot(x: u32): u32;`
- `export fn rmdFunc(round: usize, x: u32, y: u32, z: u32): u32;`
- `export fn rmdKl(round: usize): u32;`
- `export fn rmdKr(round: usize): u32;`
- `export fn rmdReadU32Le(buf: []u8, off: usize): u32;`
- `export fn rmdWriteU32Le(v: u32, out: []u8, off: usize);`
- `export fn rmdHexNibble(v: u8): u8;`
- `export fn rmdCompress(state: [5]u32, block: []u8): [5]u32;`
- `export fn ripemd160Bytes(data: []u8, out20: []u8);`
- `export fn ripemd160Hex(s: string, out40: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ripemd@1.0.0`)，许可证：MIT OR Apache-2.0
