// Type definitions for CP

export class VariantRule {
    group: string,
    option: string,
    classes: string,
}

export class VariantSelection {
    group: string,
    option: string,
}

export class CompoundRule {
    group1: string,
    option1: string,
    group2: string,
    option2: string,
    group3: string,
    option3: string,
    classes: string,
    fn pair(g1: string, o1: string, g2: string, o2: string, classes: string): CompoundRule;
    fn triple(g1: string, o1: string, g2: string, o2: string, g3: string, o3: string, classes: string): CompoundRule;
}

export fn cvaStrEq(a: string, b: string): bool;
export fn cvaIsSpace(ch: u8): bool;
export fn cvaAppendClasses(out: []u8, pos: usize, src: string): usize;
export fn cvaResolveOption(group: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): string?;
export fn cvaMatchesCond(group: string, option: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): bool;
export fn cvaCompose(base: string, rules: &[]VariantRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;
export fn cvaComposeWithCompounds(base: string, rules: &[]VariantRule, compounds: &[]CompoundRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;
