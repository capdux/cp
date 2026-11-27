[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/css-select

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:css-select` ([https://github.com/fb55/css-select](https://github.com/fb55/css-select))

Clean-room MIT, zero-heap CSS selector parser, W3C Selectors Level 4 specificity calculator, and compound/list element matching engine for CP — modeled on `css-select` + `css-what`.

## Dependencies
- `@cap/unicase` (`1.0.0`) — reused for ASCII case-insensitive HTML tag and attribute matching (`eqAscii`).

## API

### Specificity
- `Specificity { ids: u32, classes: u32, tags: u32 }`
  - `Specificity.new(ids, classes, tags): Specificity`
  - `cmp(other: Specificity): i32` — lexicographical comparison (`-1`, `0`, `1`).
  - `toScore(): u64` — monotonic 64-bit packed score `(ids << 40) | (classes << 20) | tags`.
- `computeSpecificity(selector: string): Specificity`
  - Computes W3C Selectors Level 4 specificity for single selectors and comma-separated selector lists (returning the highest specificity in a list).
  - Counts `#id` in `ids`; `.class`, `[attr]`, and `:pseudo-class` in `classes`; `tag`, `::pseudo-element`, and legacy `:before`/`:after`/`:first-line`/`:first-letter` in `tags`.
  - Ignores universal `*`, combinators (`>`, `+`, `~`), namespace prefixes (`ns|tag`), and `:where(...)`; evaluates `:is(...)`, `:not(...)`, `:has(...)` using their maximum argument specificity.

### Element Matching
- `CssElement { tag: string, id: string, class_list: string, attr_name: string, attr_val: string }`
  - `CssElement.new(tag, id, class_list, attr_name, attr_val): CssElement`
- `matchesCompoundSelector(selector: string, el: CssElement): bool`
  - Supports `*`, case-insensitive `tag`, `#id`, `.class` (whitespace-separated token match), combined compound selectors (e.g. `div#app.card.active`), and attribute selectors (`[attr]`, `[attr=v]`, `[attr^=v]`, `[attr$=v]`, `[attr*=v]`, `[attr~=v]`, `[attr|=v]`, single/double quotes, and `i`/`s` case flags).
- `matchesSelectorList(selector_list: string, el: CssElement): bool`
  - Evaluates comma-separated selector lists (e.g. `h1, .title, #main`), respecting commas inside brackets and quotes.

### Combinator Chain Inspection
- `countSelectorParts(selector: string): usize`
  - Counts compound selector segments separated by whitespace or combinators (`>`, `+`, `~`), respecting `[...]` and `(...)` groups (e.g. `ul > li.item + li` returns `3`).
