# `cap:css_at_rules` — 纯 CP CSS `@keyframes` / `@layer` / `@counter-style` / `@property` 规则模型

## 概述

`cap:css_at_rules` 吸收自 Bun `src/css/rules/{keyframes,layer,counter_style,property}.cp`，纯 CP 零外部依赖、零堆分配，提供关键帧选择器（含滚动时间线范围）、点分级联层名、计数器样式系统与注册自定义属性描述符的完整解析、规范化校验与序列化。

## 核心类型与 API

### `@keyframes`
- `isValidKeyframesIdent(name: string): bool`（拒绝 CSS 全局关键字）
- `TimelineRangeName`：`cover` / `contain` / `entry` / `exit` / `entry_crossing` / `exit_crossing`
- `KeyframeSelectorKind` / `KeyframeSelector`：`parse(input)`、`normalizedPercent()`、`toCss(buf, minify)`

### `@layer`
- `LayerName`：`anonymous()`、`parse(input)`、`isAnonymous()`、`depth()`、`eql(other)`、`isPrefixOf(other)`、`toCss(buf)`

### `@counter-style`
- `CounterSystem`：`cyclic` / `numeric` / `alphabetic` / `symbolic` / `additive` / `fixed` / `extends_style`（对应 CSS `"extends"`）
  - `default()`、`parse(input)`、`toCss()`、`requiresSymbols()`、`requiresMinTwoSymbols()`

### `@property`
- `PropertySyntaxKind`：`universal (*)` / `<length>` / `<color>` / `<number>` / `<percentage>` 等 16 种语法类型（`parse()` / `toCss()` / `isUniversal()`）
- `isValidCustomPropertyName(name: string): bool`
- `PropertyDescriptor.create(name, syntax_str, inherits, initial_value)`（强制非 `*` 语法必须提供 `initial-value`）

## 示例

```cp
import { KeyframeSelector, LayerName, CounterSystem, PropertyDescriptor } from "cap:css_at_rules";

const kf = KeyframeSelector.parse("entry 50%")?;
const parent = LayerName.parse("framework")?;
const child = LayerName.parse("framework.theme")?; // parent.isPrefixOf(child) == true
const prop = PropertyDescriptor.create("--accent", "<color>", true, "#09f")?;
```
