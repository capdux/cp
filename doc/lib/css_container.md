# `cap:css_container` — 纯 CP CSS Container Queries Level 3 容器查询引擎

## 概述

`cap:css_container` 吸收自 Bun `src/css/rules/container.cp`，纯 CP 零外部依赖、零堆分配，提供 `container-type` 属性模型、`@container` 容器名称校验（禁用 `none`/`and`/`not`/`or`）、尺寸/宽高比/方向特征解析与求值，以及支持 `and`/`or`/`not` 逻辑组合的 `ContainerQuery` 解析、求值与规范化 `toCss` 序列化。

## 核心类型与 API

- `CssContainerError`：`InvalidContainerName` / `InvalidContainerType` / `InvalidFeatureName` / `InvalidFeatureValue` / `InvalidQuerySyntax` / `TooManyConditions` / `BufferTooSmall`
- `ContainerType`：`normal` / `size` / `inline_size`（`parse()`、`toCss()`、`supportsSizeQueries()`、`supportsBlockQueries()`）
- `ContainerName`：`isValidName(s)`、`parse(s)`、`toCss()`
- `ContainerSizeFeatureId`：`width` / `height` / `inline_size` / `block_size` / `aspect_ratio` / `orientation`
- `Orientation`：`portrait` / `landscape`；`CompareOp`：`eq` / `ge` / `le` / `gt` / `lt`；`LogicOp`：`and_op` / `or_op`
- `ContainerSizeFeature`：`parse(input)`、`evaluate(width, height)`、`toCss(buf)`
- `ContainerQuery`：`parse(input)`、`evaluate(width, height)`、`toCss(buf)`

## 示例

```cp
import { ContainerQuery, ContainerType } from "cap:css_container";

const ct = ContainerType.parse("inline-size").?; // ct.supportsSizeQueries() == true
const q = ContainerQuery.parse("sidebar (min-width: 300px) and (max-width: 800px)")?;
// q.evaluate(500.0, 400.0) == true
// q.evaluate(200.0, 400.0) == false
```
