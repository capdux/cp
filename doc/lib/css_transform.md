# `cap:css_transform` — CSS 2D/3D Transform 解析、4x4 矩阵数学与序列化

> 溯源：从 `/data/dev/bun/src/css/properties/transform.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_transform` 提供零外部依赖的 CSS Transforms Level 1/2 解析、4x4 列主序齐次矩阵合成与规范 CSS 序列化引擎：

- 完整支持全部 **21 种** 2D/3D Transform 函数（`translate*`、`scale*`、`rotate*`、`skew*`、`perspective`、`matrix`、`matrix3d`）；
- 内置 **`Matrix2d`** 与 **`Matrix3d`** 列主序矩阵数学模型（单位阵、乘法合成、2D/3D 判定与无损降维、4x4 行列式与可逆性检测）；
- 包含 **`TransformOrigin`**、**`TransformStyle`**、**`BackfaceVisibility`** 及带 `~init()` 确定性 RAII 释放的 **`TransformList`**。

---

## 快速导入

```cp
import {
    Transform,
    TransformKind,
    TransformList,
    Matrix2d,
    Matrix3d,
    TransformOrigin,
    TransformStyle,
    BackfaceVisibility,
    Error,
} from "cap:css_transform";
```

---

## API 参考

### 1. `Matrix2d` & `Matrix3d`

| 类型 | 核心方法 | 说明 |
| :--- | :--- | :--- |
| `Matrix2d` | `identity()`, `determinant()`, `isInvertible()`, `isIdentity()`, `multiply(rhs)`, `toMatrix3d()`, `approxEql(other, eps)` | 2D 仿射矩阵 `(a, b, c, d, e, f)` |
| `Matrix3d` | `identity()`, `translation(tx, ty, tz)`, `scaling(sx, sy, sz)`, `rotationX/Y/Z(rad)`, `rotation3d(x, y, z, rad)`, `skewing(ax, ay)`, `perspectiveDist(d)`, `multiply(b)`, `is2d()`, `isIdentity()`, `toMatrix2d()`, `determinant()`, `isInvertible()`, `approxEql(o, eps)` | 4x4 列主序齐次矩阵 `m11..m44` |

### 2. `Transform` & `TransformList`（含 `~init()` RAII 析构）

| 类型 / 方法 | 说明 |
| :--- | :--- |
| `Transform` | 提供全部 21 种静态工厂、`is3d()`、`toMatrix3d()`、`toCss(buf)`（含 `translate(x, 0) -> translateX(x)`、`scale(s, s) -> scale(s)`、`skew(a, 0) -> skewX(a)` 简写优化） |
| `TransformList.parse(alloc, input)` | 解析 `"none"` 或空格分隔的多个 2D/3D 变换函数 |
| `TransformList.toMatrix3d()` / `is2d()` / `toCss(buf)` | 按 CSS 规范从左到右合成 4x4 矩阵并支持规范序列化 |

### 3. `TransformOrigin` / `TransformStyle` / `BackfaceVisibility`

- `TransformOrigin`：`center()`, `parse(input)`（支持 `left`/`center`/`right`/`top`/`bottom` + `px`/`%` + Z 偏移）、`toCss(buf)`
- `TransformStyle`：`flat` / `preserve_3d`（`fromStr`, `asStr`, `is3d`）
- `BackfaceVisibility`：`visible` / `hidden`（`fromStr`, `asStr`, `isHidden`）

---

## 使用示例

```cp
import { TransformList } from "cap:css_transform";

fn composeTransform(alloc: std.mem.Allocator)! {
    const list = TransformList.parse(alloc, "translate(10px, 20px) scale(2) rotate(90deg)")?;
    const m = list.toMatrix3d();
    const is_2d = m.is2d(); // true
    * = is_2d;

    let buf: [128]u8 = [0; 128];
    const css = list.toCss(buf[0..128])?;
    * = css;
} // 离开作用域自动触发 list.~init() 释放内存
```
