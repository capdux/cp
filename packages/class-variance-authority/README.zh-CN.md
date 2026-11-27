[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# class-variance-authority

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CSS variant class composer for CP (clean-room Apache-2.0 implementation of npm class-variance-authority / cva).

## 安装与导入

```bash
cap add class-variance-authority
```

```cp
import { cvaStrEq } from "class-variance-authority";
```

## 核心导出 API

- `export fn cvaStrEq(a: string, b: string): bool;`
- `export fn cvaIsSpace(ch: u8): bool;`
- `export fn cvaAppendClasses(out: []u8, pos: usize, src: string): usize;`
- `export fn cvaResolveOption(group: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): string?;`
- `export fn cvaMatchesCond(group: string, option: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): bool;`
- `export fn cvaCompose(base: string, rules: &[]VariantRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;`
- `export fn cvaComposeWithCompounds(base: string, rules: &[]VariantRule, compounds: &[]CompoundRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`class-variance-authority@1.0.0`)，许可证：Apache-2.0
