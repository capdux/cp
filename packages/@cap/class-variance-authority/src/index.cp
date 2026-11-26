// @cap/class-variance-authority — Zero-heap CSS variant class composer (cva)
// Clean-room Apache-2.0 implementation inspired by npm:class-variance-authority (joe-bell/cva).

pub struct VariantRule {
    group: string,
    option: string,
    classes: string,
}

pub struct VariantSelection {
    group: string,
    option: string,
}

pub struct CompoundRule {
    group1: string,
    option1: string,
    group2: string,
    option2: string,
    group3: string,
    option3: string,
    classes: string,

    static fn pair(g1: string, o1: string, g2: string, o2: string, classes: string): CompoundRule {
        return CompoundRule{
            group1: g1,
            option1: o1,
            group2: g2,
            option2: o2,
            group3: "",
            option3: "",
            classes: classes,
        };
    }

    static fn triple(g1: string, o1: string, g2: string, o2: string, g3: string, o3: string, classes: string): CompoundRule {
        return CompoundRule{
            group1: g1,
            option1: o1,
            group2: g2,
            option2: o2,
            group3: g3,
            option3: o3,
            classes: classes,
        };
    }
}

fn cvaStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn cvaIsSpace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

/// Append whitespace-normalized tokens from `src` into `out` starting at `pos`,
/// inserting a single `' '` separator when `pos > 0`.
fn cvaAppendClasses(out: []u8, pos: usize, src: string): usize {
    let o = pos;
    let i: usize = 0;
    while (i < src.len) {
        while (i < src.len && cvaIsSpace(src[i])) : (i += 1) {}
        if (i >= src.len) {
            break;
        }
        const tok_start = i;
        while (i < src.len && !cvaIsSpace(src[i])) : (i += 1) {}
        const tok_end = i;

        if (o > 0) {
            if (o >= out.len) {
                return o;
            }
            out[o] = ' ';
            o += 1;
        }
        let k = tok_start;
        while (k < tok_end && o < out.len) : (k += 1) {
            out[o] = src[k];
            o += 1;
        }
    }
    return o;
}

/// Resolve the active option for `group`:
/// `selected` (last matching entry) overrides `defaults` (last matching entry).
fn cvaResolveOption(group: string, defaults: []const VariantSelection, selected: []const VariantSelection): string? {
    let found: string? = null;
    let i: usize = 0;
    while (i < selected.len) : (i += 1) {
        if (cvaStrEq(selected[i].group, group)) {
            found = selected[i].option;
        }
    }
    if (v := found) {
        if (v.len > 0) {
            return v;
        }
    }
    i = 0;
    while (i < defaults.len) : (i += 1) {
        if (cvaStrEq(defaults[i].group, group)) {
            found = defaults[i].option;
        }
    }
    return found;
}

fn cvaMatchesCond(group: string, option: string, defaults: []const VariantSelection, selected: []const VariantSelection): bool {
    if (group.len == 0) {
        return true;
    }
    if (active := cvaResolveOption(group, defaults, selected)) {
        return cvaStrEq(active, option);
    }
    return false;
}

/// Compose CSS classes from `base`, variant `rules` (with `selected` overriding `defaults`),
/// and `extra` classes into `out`, normalizing all whitespace.
pub fn cvaCompose(
    base: string,
    rules: []const VariantRule,
    defaults: []const VariantSelection,
    selected: []const VariantSelection,
    extra: string,
    out: []u8
): usize {
    if (out.len == 0) {
        return 0;
    }
    let o: usize = 0;
    o = cvaAppendClasses(out, o, base);

    let i: usize = 0;
    while (i < rules.len) : (i += 1) {
        if (active := cvaResolveOption(rules[i].group, defaults, selected)) {
            if (cvaStrEq(active, rules[i].option)) {
                o = cvaAppendClasses(out, o, rules[i].classes);
            }
        }
    }

    o = cvaAppendClasses(out, o, extra);
    return o;
}

/// Compose CSS classes including `compounds` (multi-condition compound variant rules).
pub fn cvaComposeWithCompounds(
    base: string,
    rules: []const VariantRule,
    compounds: []const CompoundRule,
    defaults: []const VariantSelection,
    selected: []const VariantSelection,
    extra: string,
    out: []u8
): usize {
    if (out.len == 0) {
        return 0;
    }
    let o: usize = 0;
    o = cvaAppendClasses(out, o, base);

    let i: usize = 0;
    while (i < rules.len) : (i += 1) {
        if (active := cvaResolveOption(rules[i].group, defaults, selected)) {
            if (cvaStrEq(active, rules[i].option)) {
                o = cvaAppendClasses(out, o, rules[i].classes);
            }
        }
    }

    i = 0;
    while (i < compounds.len) : (i += 1) {
        const c = compounds[i];
        const m1 = cvaMatchesCond(c.group1, c.option1, defaults, selected);
        const m2 = cvaMatchesCond(c.group2, c.option2, defaults, selected);
        const m3 = cvaMatchesCond(c.group3, c.option3, defaults, selected);
        if (m1 && m2 && m3) {
            o = cvaAppendClasses(out, o, c.classes);
        }
    }

    o = cvaAppendClasses(out, o, extra);
    return o;
}
