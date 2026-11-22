// Source: Absorbed from /data/dev/bun/src/css/selectors/builder.cp | Synced: 2026-11-22T10:15:27Z
// Pure-CP zero-allocation CSS Selector Builder (Servo/LightningCSS matching-order
// compound selector layout) with W3C Selectors Level 4 Specificity calculation.

pub enum Combinator: u8 {
    descendant = 0,     // ' '
    child = 1,          // '>'
    next_sibling = 2,   // '+'
    later_sibling = 3,  // '~'

    pub fn asStr(): []const u8 {
        return switch (_) {
            .descendant => " ",
            .child => " > ",
            .next_sibling => " + ",
            .later_sibling => " ~ ",
        };
    }
}

pub enum ComponentKind: u8 {
    universal = 0,       // *
    type_tag = 1,        // div
    id = 2,              // #app
    class_name = 3,      // .btn
    attribute = 4,       // [disabled]
    pseudo_class = 5,    // :hover
    pseudo_element = 6,  // ::before
    nesting = 7,         // &
    combinator = 8,
}

pub struct SelectorComponent {
    kind: ComponentKind,
    text: []const u8,
    combinator: Combinator,

    pub static fn universal(): SelectorComponent {
        return SelectorComponent{ kind: .universal, text: "*", combinator: .descendant };
    }

    pub static fn typeTag(name: []const u8): SelectorComponent {
        return SelectorComponent{ kind: .type_tag, text: name, combinator: .descendant };
    }

    pub static fn id(name: []const u8): SelectorComponent {
        return SelectorComponent{ kind: .id, text: name, combinator: .descendant };
    }

    pub static fn className(name: []const u8): SelectorComponent {
        return SelectorComponent{ kind: .class_name, text: name, combinator: .descendant };
    }

    pub static fn attribute(expr: []const u8): SelectorComponent {
        return SelectorComponent{ kind: .attribute, text: expr, combinator: .descendant };
    }

    pub static fn pseudoClass(name: []const u8): SelectorComponent {
        return SelectorComponent{ kind: .pseudo_class, text: name, combinator: .descendant };
    }

    pub static fn pseudoElement(name: []const u8): SelectorComponent {
        return SelectorComponent{ kind: .pseudo_element, text: name, combinator: .descendant };
    }

    pub static fn nesting(): SelectorComponent {
        return SelectorComponent{ kind: .nesting, text: "&", combinator: .descendant };
    }

    pub static fn fromCombinator(c: Combinator): SelectorComponent {
        return SelectorComponent{ kind: .combinator, text: "", combinator: c };
    }

    pub fn isCombinator(): bool {
        return _.kind == .combinator;
    }
}

/// W3C Selectors Level 4 specificity `(id, class, type)` packed into comparable u32.
pub struct Specificity {
    ids: u16,
    classes: u16,
    types: u16,

    pub static fn zero(): Specificity {
        return Specificity{ ids: 0, classes: 0, types: 0 };
    }

    pub fn toPacked(): u32 {
        const a: u32 = @min(_.ids, 255);
        const b: u32 = @min(_.classes, 255);
        const c: u32 = @min(_.types, 255);
        return (a << 16) | (b << 8) | c;
    }

    pub fn eql(other: Specificity): bool {
        return _.ids == other.ids && _.classes == other.classes && _.types == other.types;
    }

    pub fn gt(other: Specificity): bool {
        if (_.ids != other.ids) return _.ids > other.ids;
        if (_.classes != other.classes) return _.classes > other.classes;
        return _.types > other.types;
    }
}

pub struct CombinatorEntry {
    combinator: Combinator,
    left_compound_len: usize,
}

/// Built selector stored in Servo matching order (rightmost compound first,
/// simple selectors within each compound left-to-right).
pub struct BuiltSelector {
    components: [64]SelectorComponent,
    len: usize,
    specificity: Specificity,
    has_pseudo_element: bool,
    has_nesting: bool,

    pub fn slice(): []const SelectorComponent {
        return _.components[0.._.len];
    }
}

/// Fixed-capacity zero-allocation CSS SelectorBuilder.
pub struct SelectorBuilder {
    simple_selectors: [48]SelectorComponent,
    simple_len: usize,
    combinators: [16]CombinatorEntry,
    combo_len: usize,
    current_len: usize,

    pub static fn init(): SelectorBuilder {
        return SelectorBuilder{
            simple_selectors: [SelectorComponent.universal(); 48],
            simple_len: 0,
            combinators: [CombinatorEntry{ combinator: .descendant, left_compound_len: 0 }; 16],
            combo_len: 0,
            current_len: 0,
        };
    }

    pub fn hasCombinators(): bool {
        return _.combo_len > 0;
    }

    pub fn pushSimple(*_, comp: SelectorComponent): bool {
        if (comp.isCombinator() || _.simple_len >= 48) return false;
        _.simple_selectors[_.simple_len] = comp;
        _.simple_len += 1;
        _.current_len += 1;
        return true;
    }

    pub fn pushCombinator(*_, c: Combinator): bool {
        if (_.current_len == 0 || _.combo_len >= 16) return false;
        _.combinators[_.combo_len] = CombinatorEntry{
            combinator: c,
            left_compound_len: _.current_len,
        };
        _.combo_len += 1;
        _.current_len = 0;
        return true;
    }

    /// Prepend `& ` (nesting prefix) to the selector, matching Servo/LightningCSS.
    pub fn addNestingPrefix(*_): bool {
        if (_.simple_len >= 48 || _.combo_len >= 16) return false;
        // Shift simple_selectors right by 1
        let i = _.simple_len;
        while (i > 0) : (i -= 1) {
            _.simple_selectors[i] = _.simple_selectors[i - 1];
        }
        _.simple_selectors[0] = SelectorComponent.nesting();
        _.simple_len += 1;

        // Shift combinators right by 1
        let j = _.combo_len;
        while (j > 0) : (j -= 1) {
            _.combinators[j] = _.combinators[j - 1];
        }
        _.combinators[0] = CombinatorEntry{ combinator: .descendant, left_compound_len: 1 };
        _.combo_len += 1;
        return true;
    }

    /// Compute W3C Selectors Level 4 specificity across all pushed simple selectors.
    pub fn computeSpecificity(): Specificity {
        let spec = Specificity.zero();
        let i: usize = 0;
        while (i < _.simple_len) : (i += 1) {
            switch (_.simple_selectors[i].kind) {
                .id => spec.ids += 1,
                .class_name, .attribute, .pseudo_class => spec.classes += 1,
                .type_tag, .pseudo_element => spec.types += 1,
                .universal, .nesting, .combinator => {},
            }
        }
        return spec;
    }

    /// Build selector in Servo right-to-left compound matching order.
    pub fn buildMatchingOrder(): BuiltSelector {
        let out = BuiltSelector{
            components: [SelectorComponent.universal(); 64],
            len: 0,
            specificity: _.computeSpecificity(),
            has_pseudo_element: false,
            has_nesting: false,
        };

        let k: usize = 0;
        while (k < _.simple_len) : (k += 1) {
            if (_.simple_selectors[k].kind == .pseudo_element) out.has_pseudo_element = true;
            if (_.simple_selectors[k].kind == .nesting) out.has_nesting = true;
        }

        let remaining_end = _.simple_len;
        let cur_len = _.current_len;
        let cur_start = remaining_end - cur_len;
        let combo_idx = _.combo_len;

        while (true) {
            let idx = cur_start;
            while (idx < remaining_end && out.len < 64) : (idx += 1) {
                out.components[out.len] = _.simple_selectors[idx];
                out.len += 1;
            }
            if (combo_idx == 0) break;
            combo_idx -= 1;
            const entry = _.combinators[combo_idx];
            if (out.len < 64) {
                out.components[out.len] = SelectorComponent.fromCombinator(entry.combinator);
                out.len += 1;
            }
            remaining_end = cur_start;
            cur_len = entry.left_compound_len;
            cur_start = remaining_end - cur_len;
        }

        return out;
    }

    /// Serialize the selector in source (left-to-right) CSS order into `out`.
    pub fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        let simple_idx: usize = 0;
        let c_idx: usize = 0;
        while (c_idx < _.combo_len) : (c_idx += 1) {
            const entry = _.combinators[c_idx];
            let j: usize = 0;
            while (j < entry.left_compound_len && simple_idx < _.simple_len) : (j += 1) {
                pos = writeComponent(out, pos, _.simple_selectors[simple_idx]);
                simple_idx += 1;
            }
            pos = appendStr(out, pos, entry.combinator.asStr());
        }
        while (simple_idx < _.simple_len) : (simple_idx += 1) {
            pos = writeComponent(out, pos, _.simple_selectors[simple_idx]);
        }
        return out[0..pos];
    }
}

fn writeComponent(out: []u8, pos: usize, c: SelectorComponent): usize {
    switch (c.kind) {
        .universal => return appendStr(out, pos, "*"),
        .type_tag => return appendStr(out, pos, c.text),
        .id => {
            const p = appendStr(out, pos, "#");
            return appendStr(out, p, c.text);
        },
        .class_name => {
            const p = appendStr(out, pos, ".");
            return appendStr(out, p, c.text);
        },
        .attribute => {
            let p = appendStr(out, pos, "[");
            p = appendStr(out, p, c.text);
            return appendStr(out, p, "]");
        },
        .pseudo_class => {
            const p = appendStr(out, pos, ":");
            return appendStr(out, p, c.text);
        },
        .pseudo_element => {
            const p = appendStr(out, pos, "::");
            return appendStr(out, p, c.text);
        },
        .nesting => return appendStr(out, pos, "&"),
        .combinator => return appendStr(out, pos, c.combinator.asStr()),
    }
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}
