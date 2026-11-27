# @cap/num-complex

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-complex` ([https://github.com/rust-num/num-complex](https://github.com/rust-num/num-complex))

零堆分配的 CP 复数数学引擎（`Complex64`） — 洁净室 MIT 实现，对标 Rust crates.io #317 `num-complex`（4.96 亿+ 总下载），服务 FFT、DSP、信号处理与科学计算。

Zero-allocation `Complex64` arithmetic, Smith overflow-safe division, polar coordinates, Euler `exp`, principal `sqrt`, and formatting for CP — clean-room MIT parity with Rust `num-complex` (496M+ downloads).

## 安装 / Install

```bash
cap add @cap/num-complex
```

## API 参考 / API Reference

- **构造器 / Constructors**：`Complex64.new(re, im)`、`zero()`、`one()`、`i()`、`fromPolar(r, theta)`
- **代数运算 / Algebra**：`add`、`sub`、`mul`、`div`（Smith 防溢出缩放）、`neg`、`conj`、`scale`、`inv`、`powi`（支持负整数快速幂）
- **几何与复变函数 / Geometry & Functions**：`normSqr`、`norm`（`hypot`）、`arg`（`atan2`）、`exp`（欧拉公式）、`sqrt`（主平方根）、`approxEq`
- **格式化 / Formatting**：`formatComplex(z, out) -> string`

## 使用示例 / Usage

```cp
import { Complex64, formatComplex } from "@cap/num-complex";

const z1 = Complex64.new(3.0, 4.0);
const mag = z1.norm();       // 5.0
const root = z1.sqrt();      // 2 + 1i

let buf: [64]u8 = undefined;
const text = formatComplex(z1, buf[0..64]); // "3+4i"
```

## License

MIT
