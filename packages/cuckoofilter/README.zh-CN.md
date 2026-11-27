[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cuckoofilter

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CoNEXT'14 Cuckoo Filter (256x4 16-bit fingerprint slots) with O(1) insert, contains, and safe delete for CP

## 安装与导入

```bash
cap add cuckoofilter
```

```cp
import { mulWrap64 } from "cuckoofilter";
```

## 核心导出 API

- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn avalanche64(x: u64): u64;`
- `export fn hashBytes(data: &[]u8): u64;`
- `export fn fingerprintFromHash(h: u64): u16;`
- `export fn altBucket(bucket: usize, fp: u16): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cuckoofilter@1.0.0`)，许可证：MIT
