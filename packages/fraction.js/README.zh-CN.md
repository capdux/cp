[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fraction.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 fraction.js

## 安装与导入

```bash
cap add fraction.js
```

```cp
import { absToU64 } from "fraction.js";
```

## 核心导出 API

- `export fn absToU64(v: i64): u64;`
- `export fn divTrunc64(a: i64, b: i64): i64;`
- `export fn gcd64(a: i64, b: i64): i64;`
- `export fn zero(): Rational;`
- `export fn one(): Rational;`
- `export fn fromInt(n: i64): Rational;`
- `export fn new(n: i64, d: i64): Rational;`
- `export fn neg(a: Rational): Rational;`
- `export fn abs(a: Rational): Rational;`
- `export fn recip(a: Rational): Rational;`
- `export fn add(a: Rational, b: Rational): Rational;`
- `export fn sub(a: Rational, b: Rational): Rational;`
- `export fn mul(a: Rational, b: Rational): Rational;`
- `export fn div(a: Rational, b: Rational): Rational;`
- `export fn eq(a: Rational, b: Rational): bool;`
- `export fn cmp(a: Rational, b: Rational): i32;`
- `export fn toF64(a: Rational): f64;`
- `export fn floorInt(a: Rational): i64;`
- `export fn ceilInt(a: Rational): i64;`
- `export fn isInteger(a: Rational): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fraction.js@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`fraction@1.0.0`)，许可证：MIT
