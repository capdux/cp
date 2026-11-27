[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ctr

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation NIST SP 800-38A AES-CTR stream cipher modes (Ctr128BE / Ctr128LE) with O(1) seek in pure CP

## 安装与导入

```bash
cap add ctr
```

```cp
import { ctrZeroBlock } from "ctr";
```

## 核心导出 API

- `export fn ctrZeroBlock(): [16]u8;`
- `export fn ctrAddBe128(iv: [16]u8, delta: u64): [16]u8;`
- `export fn ctrAddLe128(iv: [16]u8, delta: u64): [16]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ctr@1.0.0`)，许可证：MIT OR Apache-2.0
