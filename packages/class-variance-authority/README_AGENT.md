# class-variance-authority@1.0.0 (Agent Quickref)

> Zero-heap CSS variant class composer for CP (clean-room Apache-2.0 implementation of npm class-variance-authority / cva). | License: Apache-2.0
> Import: `import { ... } from "class-variance-authority";` | Full types: `src/index.d.cp`

## Functions
- `export fn cvaStrEq(a: string, b: string): bool;`
- `export fn cvaIsSpace(ch: u8): bool;`
- `export fn cvaAppendClasses(out: []u8, pos: usize, src: string): usize;`
- `export fn cvaResolveOption(group: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): string?;`
- `export fn cvaMatchesCond(group: string, option: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): bool;`
- `export fn cvaCompose(base: string, rules: &[]VariantRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;`
- `export fn cvaComposeWithCompounds(base: string, rules: &[]VariantRule, compounds: &[]CompoundRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;`
