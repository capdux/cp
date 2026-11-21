# `cap:css_layout_ctrl`

纯 CP 零堆分配 CSS 布局控制属性库（`overflow`、`text-overflow`、`position`、`container-type`、`contain`、`z-index`），吸收自 `/data/dev/bun/src/css/properties/{overflow,position,contain}.cp`。

## 导出类型

- `OverflowKeyword` / `Overflow`：`visible | hidden | clip | scroll | auto` 及 1~2 轴简写压缩
- `TextOverflow`：`clip | ellipsis`
- `Position`：`static | relative | absolute | fixed | sticky | -webkit-sticky`
- `ContainerType`：`normal | size | inline-size`
- `ContainFlags`：`none | strict | content | [size || inline-size || layout || style || paint]`（自动合并规范化为 `strict` / `content`）
- `ZIndex`：`auto | <integer>`
