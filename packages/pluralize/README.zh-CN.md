[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pluralize

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation English pluralization, singularization, irregular/uncountable dictionary, count formatting & ordinal engine for CP

## 安装与导入

```bash
cap add pluralize
```

```cp
import { isVowelLower } from "pluralize";
```

## 核心导出 API

- `export fn isVowelLower(c: u8): bool;`
- `export fn detectCaseStyle(word: &[]u8): u8;`
- `export fn copySlice(out: []u8, src: &[]u8): &[]u8;`
- `export fn writeWithCase(out: []u8, src_lower: &[]u8, style: u8): &[]u8;`
- `export fn writeStemAndSuffix(out: []u8, stem: &[]u8, suffix_lower: &[]u8, all_upper: bool): &[]u8;`
- `export fn inList(lower: &[]u8, list: &[]&[]u8): bool;`
- `export fn isUncountable(word: &[]u8): bool;`
- `export fn plural(word: &[]u8, out: []u8): &[]u8;`
- `export fn singular(word: &[]u8, out: []u8): &[]u8;`
- `export fn isPlural(word: &[]u8): bool;`
- `export fn isSingular(word: &[]u8): bool;`
- `export fn pluralizeCount(word: &[]u8, count: i64, out: []u8): &[]u8;`
- `export fn ordinal(n: i64, out: []u8): &[]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pluralize@1.0.0`)，许可证：MIT
