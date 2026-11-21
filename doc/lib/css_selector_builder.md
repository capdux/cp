# `cap:css_selector_builder`

纯 CP 零堆分配 CSS 选择器构建器（Servo / LightningCSS 右向左复合匹配序布局）与 W3C Selectors Level 4 特异度（Specificity）计算库，吸收自 `/data/dev/bun/src/css/selectors/builder.cp`。

## 导出类型

- `Combinator`：`descendant | child | next_sibling | later_sibling`
- `ComponentKind` / `SelectorComponent`：`universal`, `type_tag`, `id`, `class_name`, `attribute`, `pseudo_class`, `pseudo_element`, `nesting`, `combinator`
- `Specificity`：`(ids, classes, types)` 特异度计算、`toPacked()` 与 `gt()` 比较
- `BuiltSelector`：右向左匹配序组件序列 + 特异度 + `has_pseudo_element`/`has_nesting` 标记
- `SelectorBuilder`：`pushSimple`、`pushCombinator`、`addNestingPrefix`、`computeSpecificity`、`buildMatchingOrder`、`toCss`
