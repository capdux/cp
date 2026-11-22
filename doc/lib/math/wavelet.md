# `cap:wavelet` — 离散小波变换 DWT / IDWT（Haar、Daubechies-4、JPEG 2000 CDF 5/3）

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

纯 CP、零堆分配的 1D 离散小波变换标准库，提供正交归一化 Haar（单级与多级多分辨率）、4 抽头 Daubechies-4（`db2`，2 阶消失矩）、JPEG 2000 无损整数提升 Le Gall CDF 5/3 以及 Donoho-Johnstone 软/硬阈值去噪与 Parseval 能量计算。

## 导入

```cp
import {
    haarForward1d,
    haarInverse1d,
    haarForwardMulti1d,
    haarInverseMulti1d,
    db4Forward1d,
    db4Inverse1d,
    cdf53ForwardI32,
    cdf53InverseI32,
    softThreshold,
    hardThreshold,
    signalEnergy,
} from "cap:math";
```

## 核心 API

- **正交 Haar 小波**：
  - `haarForward1d(data: []f64, scratch: []f64): bool` / `haarInverse1d(data, scratch): bool`
  - `haarForwardMulti1d(data: []f64, levels: usize, scratch: []f64): bool` / `haarInverseMulti1d(...)`
- **Daubechies-4 (`db2`) 正交小波**：
  - `db4Forward1d(data: []f64, scratch: []f64): bool` / `db4Inverse1d(data, scratch): bool`（循环边界，`N >= 4` 偶数）
- **JPEG 2000 无损整数提升 CDF 5/3**：
  - `cdf53ForwardI32(data: []i32, scratch: []i32): bool` / `cdf53InverseI32(data, scratch): bool`（逐比特精确可逆）
- **去噪与能量**：
  - `softThreshold(data: []f64, threshold: f64)`、`hardThreshold(data: []f64, threshold: f64)`、`signalEnergy(data: []const f64): f64`

## 示例

```cp
import std from "std";
import { cdf53ForwardI32, cdf53InverseI32 } from "cap:math";

fn losslessRoundtrip(sig: []i32, scratch: []i32): bool {
    if (!cdf53ForwardI32(sig, scratch)) { return false; }
    return cdf53InverseI32(sig, scratch);
}
```
