# @cap/fixed-point

零堆分配的 CP 64 位有符号 Q32.32 确定性定点数算术引擎 — 洁净室 MIT 实现，对标 Rust `fixed` crate，跨平台比特级完全一致、零浮点漂移，适用于游戏帧同步锁步、嵌入式无 FPU 信号处理与确定性金融计算。

Zero-allocation signed 64-bit Q32.32 deterministic fixed-point arithmetic engine for CP — clean-room MIT parity with Rust `fixed`.

## 安装 / Install

```bash
cap add @cap/fixed-point
```

## API 参考 / API Reference

- **`Fixed32`**（`raw: i64`, `FRAC_BITS = 32`）：
  - 构造与转换：`fromInt`, `fromRaw`, `fromRatio`, `fromF64`, `toInt`, `floor`, `ceil`, `round`, `toF64`, `rawValue`
  - 代数运算：`add`, `sub`, `neg`, `abs`, `mul`（32 位拆半无溢出）, `div`（128 位长除法无溢出，除零返回 0）
  - 实用函数：`lerp`, `clamp`, `sqrt`（确定性牛顿迭代）, `cmp`

## 使用示例 / Usage

```cp
import { Fixed32 } from "@cap/fixed-point";

const a = Fixed32.fromRatio(3, 2);     // 1.5
const b = Fixed32.fromInt(4);          // 4.0
const prod = a.mul(b);                 // 6.0
const root2 = Fixed32.fromInt(2).sqrt(); // ~1.414213562
```

## License

MIT
