[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bn.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 256-bit unsigned big integer arithmetic (4x u64 limbs) for CP

## 安装与导入

```bash
cap add bn.js
```

```cp
import { addWithCarry64 } from "bn.js";
```

## 核心导出 API

- `export fn addWithCarry64(a: u64, b: u64, carry_in: u64): Add64Res;`
- `export fn subWithBorrow64(a: u64, b: u64, borrow_in: u64): Sub64Res;`
- `export fn mulWide64(a: u64, b: u64): Mul64Res;`
- `export fn clz64(x: u64): u32;`
- `export fn hexVal(c: u8): i32;`
- `export fn hexDigit(v: u8): u8;`
- `export fn zero(): U256;`
- `export fn one(): U256;`
- `export fn fromU64(v: u64): U256;`
- `export fn fromU128(lo: u64, hi: u64): U256;`
- `export fn maxVal(): U256;`
- `export fn fromHex(s: string): U256;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`bn.js@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`primitive-types@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`ruint@1.0.0`)，许可证：MIT
