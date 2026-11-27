# `@cap/pluralize` — Zero-Allocation English Pluralization & Ordinal Engine

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pluralize` ([https://github.com/plurals/pluralize](https://github.com/plurals/pluralize))

> Clean-room pure-CP implementation modeled after npm `pluralize`, `inflection`, and Rust `pluralizer` | License: MIT

## Overview / 概述

`@cap/pluralize` is a zero-dependency, zero-heap-allocation English inflection library for CP. It includes a 56-pair irregular noun dictionary, a 36-word uncountable noun table, regular English plural/singular transformation rules, automatic case-style preservation (`child`/`Child`/`CHILD`), count formatting (`"1 item"`, `"3 items"`), and English ordinals (`"1st"`, `"11th"`, `"21st"`).

`@cap/pluralize` 是纯 CP 零依赖、零堆分配的英语单复数与序数词变形引擎，内置 56 组高频不规则名词词典、36 个不可数名词表、规则英语单复数互转、智能大小写还原、数量词格式化及序数词生成。

## Installation / 安装

```bash
cap add @cap/pluralize
```

## API Reference / 导出 API

| Signature | Description / 说明 |
| :--- | :--- |
| `plural(word: []const u8, out: []u8): []const u8` | Pluralize an English noun, preserving case / 转复数并保持大小写 |
| `singular(word: []const u8, out: []u8): []const u8` | Singularize an English noun, preserving case / 转单数并保持大小写 |
| `isPlural(word: []const u8): bool` | Check whether `word` is plural / 判断是否为复数 |
| `isSingular(word: []const u8): bool` | Check whether `word` is singular / 判断是否为单数 |
| `isUncountable(word: []const u8): bool` | Check whether `word` is uncountable / 判断是否为不可数名词 |
| `pluralizeCount(word: []const u8, count: i64, out: []u8): []const u8` | Format `"<count> <word>"` with proper inflection / 数量+单复数格式化 |
| `ordinal(n: i64, out: []u8): []const u8` | Format English ordinal (`1st`, `2nd`, `3rd`, `11th`, `21st`) / 序数词格式化 |

## Quick Start / 使用示例

```cp
import {
    plural,
    singular,
    isPlural,
    isSingular,
    isUncountable,
    pluralizeCount,
    ordinal,
} from "@cap/pluralize";

fn demoPluralize() {
    let buf: [64]u8 = undefined;

    const p1 = plural("child", &buf);           // "children"
    const p2 = plural("CITY", &buf);            // "CITIES"
    const s1 = singular("Analyses", &buf);      // "Analysis"
    const c1 = pluralizeCount("item", 1, &buf); // "1 item"
    const c2 = pluralizeCount("item", 3, &buf); // "3 items"
    const o1 = ordinal(21, &buf);               // "21st"
    const u1 = isUncountable("information");    // true

    * = p1; * = p2; * = s1; * = c1; * = c2; * = o1; * = u1;
}
```
