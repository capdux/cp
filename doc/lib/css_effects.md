# `cap:css_effects`

纯 CP 零堆分配 CSS Filter Effects（`filter` / `backdrop-filter`）滤镜函数链解析与序列化库，吸收自 `/data/dev/bun/src/css/properties/effects.cp`。

## 导出类型

- `FilterKind`：覆盖全部 11 种标准滤镜（`blur`, `brightness`, `contrast`, `grayscale`, `hue_rotate`, `invert`, `opacity`, `saturate`, `sepia`, `drop_shadow`, `url`）
- `DropShadow`：`drop-shadow(x y [blur] [color])`
- `Filter`：单滤镜函数解析与归一化（百分比范围裁剪、`0` 长度与默认参数支持）
- `FilterList`：`none` 或最多 16 项级联滤镜链解析与规范化 `toCss` 序列化
