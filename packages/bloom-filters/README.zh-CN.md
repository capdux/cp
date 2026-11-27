[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bloom-filters

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 bloom-filters

## 安装与导入

```bash
cap add bloom-filters
```

```cp
import { fnv1a64 } from "bloom-filters";
```

## 核心导出 API

- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn djb2Mix64(data: &[]u8): u64;`
- `export fn popcount(bits: &[]u8): u64;`
- `export fn fillRatio(bits: &[]u8): f64;`
- `export fn optimalNumHashes(bit_count: usize, expected_items: usize): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`bloom-filters@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`bloomfilter@1.0.0`)，许可证：MIT
