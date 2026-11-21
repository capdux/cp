# `cap:css_sizing`

纯 CP 实现的 CSS Sizing 与盒模型 Insets 属性库，吸收自 `/data/dev/bun/src/css/properties/size.cp` 与 `/data/dev/bun/src/css/properties/margin_padding.cp`。

## 导出类型

- `BoxSizing`：`content-box | border-box`
- `SizeVendorPrefix` / `SizeUnit` / `SizeLength`：厂商前缀与 `<length-percentage>` 值
- `Size`：`width` / `height` / `inline-size` / `block-size`（支持 `auto`、`contain`、`min-content`、`max-content`、`fit-content`、`fit-content(<len-pct>)`、`stretch`、`-webkit-fill-available`、`-moz-available` 等）
- `MaxSize`：`max-width` / `max-height`（支持 `none` 与所有固有尺寸关键字/长度）
- `AspectRatio`：`auto | <ratio> | auto && <ratio>`
- `InsetValue` / `BoxInsets` / `LogicalInsets`：`margin` / `padding` / `inset` 1~4 值与 `margin-inline` / `padding-block` 1~2 值简写解析与最短压缩序列化
