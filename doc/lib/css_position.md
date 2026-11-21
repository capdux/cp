# `cap:css_position`

纯 CP 零堆分配 CSS `<position>` 1/2/3/4 值坐标解析与规范化压缩序列化库，吸收自 `/data/dev/bun/src/css/values/position.cp`。

## 导出类型

- `HorizontalSide`（`left | right`）/ `VerticalSide`（`top | bottom`）
- `PosUnit` / `PosLength`（`px | % | em | rem`）
- `AxisPosKind` / `HorizontalPosition` / `VerticalPosition`
- `CssPosition`：完整支持 1/2/3/4 值语法（含反向顺序如 `bottom 10px right 20px`、`top left`、`center`）与最短规范化 `toCss` 序列化
