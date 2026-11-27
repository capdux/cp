[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cobs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Consistent Overhead Byte Stuffing (COBS), custom-sentinel COBS/XOR, framed codec, and streaming CobsDecoder for CP

## 安装与导入

```bash
cap add cobs
```

```cp
import { max_encoded_len } from "cobs";
```

## 核心导出 API

- `export fn max_encoded_len(source_len: usize): usize;`
- `export fn encode(src: &[]u8, dst: []u8): usize;`
- `export fn decode(src: &[]u8, dst: []u8): i64;`
- `export fn encode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): usize;`
- `export fn decode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): i64;`
- `export fn encode_framed(src: &[]u8, dst: []u8): usize;`
- `export fn decode_framed(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cobs@1.0.0`)，许可证：MIT
