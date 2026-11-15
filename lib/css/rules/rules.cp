import css from "../css_parser.zig";

const CustomMedia = css.CustomMedia;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Dependency = css.Dependency;
const dependencies = css.dependencies;

import import_rule from "./import.zig";
import layer from "./layer.zig";
import style from "./style.zig";
import keyframes from "./keyframes.zig";
import font_face from "./font_face.zig";
import font_palette_values from "./font_palette_values.zig";
import page from "./page.zig";
import supports from "./supports.zig";
import counter_style from "./counter_style.zig";
import custom_media from "./custom_media.zig";
import namespace from "./namespace.zig";
import unknown from "./unknown.zig";
import document from "./document.zig";
import nesting from "./nesting.zig";
import viewport from "./viewport.zig";
import property from "./property.zig";
import container from "./container.zig";
import scope from "./scope.zig";
import media from "./media.zig";
import starting_style from "./starting_style.zig";

import tailwind from "./tailwind.zig";

const debug = bun.Output.scoped(.CSS_MINIFY, .visible);

pub fn CssRule(comptime Rule: type): type {
    * = Rule;
    return union(enum) {
        /// A `@media` rule.
        media: media.MediaRule(Rule),
        /// An `@import` rule.
        @"import": import_rule.ImportRule,
        /// A style rule.
        style: style.StyleRule(Rule),
        /// A `@keyframes` rule.
        keyframes: keyframes.KeyframesRule,
        /// A `@font-face` rule.
        font_face: font_face.FontFaceRule,
        /// A `@font-palette-values` rule.
        font_palette_values: font_palette_values.FontPaletteValuesRule,
        /// A `@page` rule.
        page: page.PageRule,
        /// A `@supports` rule.
        supports: supports.SupportsRule(Rule),
        /// A `@counter-style` rule.
        counter_style: counter_style.CounterStyleRule,
        /// A `@namespace` rule.
        namespace: namespace.NamespaceRule,
        /// A `@-moz-document` rule.
        moz_document: document.MozDocumentRule(Rule),
        /// A `@nest` rule.
        nesting: nesting.NestingRule(Rule),
        /// A `@viewport` rule.
        viewport: viewport.ViewportRule,
        /// A `@custom-media` rule.
        custom_media: CustomMedia,
        /// A `@layer` statement rule.
        layer_statement: layer.LayerStatementRule,
        /// A `@layer` block rule.
        layer_block: layer.LayerBlockRule(Rule),
        /// A `@property` rule.
        property: property.PropertyRule,
        /// A `@container` rule.
        container: container.ContainerRule(Rule),
        /// A `@scope` rule.
        scope: scope.ScopeRule(Rule),
        /// A `@starting-style` rule.
        starting_style: starting_style.StartingStyleRule(Rule),
        /// A placeholder for a rule that was removed.
        ignored,
        /// An unknown at-rule.
        unknown: unknown.UnknownAtRule,
        /// A custom at-rule.
        custom: Rule,

        const This = @This();

        pub fn toCss(rule_self: *const @This(), dest: *Printer): PrintErr!void {
            return switch (rule_self.*) {
                .media => |x| x.toCss(dest),
                .@"import" => |x| x.toCss(dest),
                .style => |x| x.toCss(dest),
                .keyframes => |x| x.toCss(dest),
                .font_face => |x| x.toCss(dest),
                .font_palette_values => |x| x.toCss(dest),
                .page => |x| x.toCss(dest),
                .supports => |x| x.toCss(dest),
                .counter_style => |x| x.toCss(dest),
                .namespace => |x| x.toCss(dest),
                .moz_document => |x| x.toCss(dest),
                .nesting => |x| x.toCss(dest),
                .viewport => |x| x.toCss(dest),
                .custom_media => |x| x.toCss(dest),
                .layer_statement => |x| x.toCss(dest),
                .layer_block => |x| x.toCss(dest),
                .property => |x| x.toCss(dest),
                .starting_style => |x| x.toCss(dest),
                .container => |x| x.toCss(dest),
                .scope => |x| x.toCss(dest),
                .unknown => |x| x.toCss(dest),
                .custom => |x| x.toCss(dest) catch return dest.addFmtError(),
                .ignored => {},
            };
        }

        pub fn deepClone(rule_self: *const @This(), allocator: std.mem.Allocator): This {
            return css.implementDeepClone(@This(), rule_self, allocator);
        }
    };
}

pub fn CssRuleList(comptime AtRule: type): type {
    * = AtRule;
    return struct {
        v: ArrayList(CssRule(AtRule)) = {},

        const This = @This();

        pub fn minify(*_, context: *MinifyContext, parent_is_unused: bool): css.MinifyErr!void {
            // var keyframe_rules: keyframes.KeyframesName.HashMap(usize) = .{};
            // _ = keyframe_rules; // autofix
            // const layer_rules: layer.LayerName.HashMap(usize) = .{};
            // _ = layer_rules; // autofix
            // const property_rules: css.css_values.ident.DashedIdent.HashMap(usize) = .{};
            // _ = property_rules; // autofix
            let style_rules: StyleRuleKey(AtRule).HashMap(usize) = .{};
            let rules: ArrayList(CssRule(AtRule)) = .{};

            for (*rule of _.v.items) {
                // NOTE Anytime you append to `rules` with this `rule`, you must set `moved_rule` to true.
                let moved_rule = false;
                defer if (moved_rule) {
                    // PERF calling deinit here might allow mimalloc to reuse the freed memory
                    rule.* = .ignored;
                };

                switch (rule.*) {
                    .keyframes => {
                        debug("TODO: KeyframesRule", {});
                    },
                    .custom_media => {
                        if (context.custom_media != null) {
                            continue;
                        }
                    },
                    .media => |*med| {
                        moved_rule = false;
                        if (rules.items.len > 0 && rules.items[rules.items.len - 1] == .media) {
                            let last_rule = &rules.items[rules.items.len - 1].media;
                            if (last_rule.query.eql(&med.query)) {
                                bun.handleOom(last_rule.rules.v.appendSlice(context.allocator, med.rules.v.items));
                                * = try last_rule.minify(context, parent_is_unused);
                                continue;
                            }
                        }
                        if (try med.minify(context, parent_is_unused)) {
                            continue;
                        }
                    },
                    .supports => |*supp| {
                        if (rules.items.len > 0 && rules.items[rules.items.len - 1] == .supports) {
                            let last_rule = &rules.items[rules.items.len - 1].supports;
                            if (last_rule.condition.eql(&supp.condition)) {
                                continue;
                            }
                        }

                        supp.minify(context, parent_is_unused)?;
                        if (supp.rules.v.items.len == 0) continue;
                    },
                    .container => {
                        debug("TODO: ContainerRule", {});
                    },
                    .layer_block => |*lay| {
                        lay.rules.minify(context, parent_is_unused)?;
                        if (lay.rules.v.items.len == 0) continue;
                    },
                    .layer_statement => {
                        debug("TODO: LayerStatementRule", {});
                    },
                    .moz_document => {
                        debug("TODO: MozDocumentRule", {});
                    },
                    .style => |*sty| {
                        const Selector = css.selector.Selector;
                        const SelectorList = css.selector.SelectorList;
                        const Component = css.selector.Component;
                        debug("Input style:\n  Selectors: {f}\n  Decls: {f}\n", .{ sty.selectors.debug(), sty.declarations.debug() });
                        if (parent_is_unused || try sty.minify(context, parent_is_unused)) {
                            continue;
                        }

                        // If some of the selectors in this rule are not compatible with the targets,
                        // we need to either wrap in :is() or split them into multiple rules.
                        let incompatible: css.SmallList(css.selector.parser.Selector, 1) = if (sty.selectors.v.len() > 1 &&
                            context.targets.shouldCompileSelectors() &&
                            !sty.isCompatible(context.targets.*))
                        incompatible: {
                            debug("Making incompatible!\n", {});
                            // The :is() selector accepts a forgiving selector list, so use that if possible.
                            // Note that :is() does not allow pseudo elements, so we need to check for that.
                            // In addition, :is() takes the highest specificity of its arguments, so if the selectors
                            // have different weights, we need to split them into separate rules as well.
                            if (context.targets.isCompatible(css.compat.Feature.is_selector) && !sty.selectors.anyHasPseudoElement() && sty.selectors.specifitiesAllEqual()) {
                                const component = Component{ .is = sty.selectors.v.toOwnedSlice(context.allocator) };
                                let list: css.SmallList(css.selector.parser.Selector, 1) = .{};
                                list.append(context.allocator, Selector.fromComponent(context.allocator, component));
                                sty.selectors = SelectorList{
                                    v: list,
                                };
                                break :incompatible .{};
                            } else {
                                // Otherwise, partition the selectors and keep the compatible ones in this rule.
                                // We will generate additional rules for incompatible selectors later.
                                let incompatible: css.SmallList(Selector, 1) = .{};
                                let i: u32 = 0;
                                while (i < sty.selectors.v.len()) {
                                    if (css.selector.isCompatible(sty.selectors.v.slice()[i .. i + 1], context.targets.*)) {
                                        i += 1;
                                    } else {
                                        // Move the selector to the incompatible list.
                                        incompatible.append(
                                            context.allocator,
                                            sty.selectors.v.orderedRemove(i),
                                        );
                                    }
                                }
                                break :incompatible incompatible;
                            }
                        } else .{};

                        sty.updatePrefix(context);

                        // Attempt to merge the new rule with the last rule we added.
                        let merged = false;
                        if (rules.items.len > 0 && rules.items[rules.items.len - 1] == .style) {
                            const last_style_rule = &rules.items[rules.items.len - 1].style;
                            if (mergeStyleRules(AtRule, sty, last_style_rule, context)) {
                                // If that was successful, then the last rule has been updated to include the
                                // selectors/declarations of the new rule. This might mean that we can merge it
                                // with the previous rule, so continue trying while we have style rules available.
                                while (rules.items.len >= 2) {
                                    const len = rules.items.len;
                                    let [a, b] = bun.splitAtMut(CssRule(AtRule), rules.items, len - 1);
                                    if (b[0] == .style && a[len - 2] == .style) {
                                        if (mergeStyleRules(AtRule, &b[0].style, &a[len - 2].style, context)) {
                                            // If we were able to merge the last rule into the previous one, remove the last.
                                            * = rules.pop();
                                            // TODO: deinit?
                                            // popped.deinit(contet.allocator);
                                            continue;
                                        }
                                    }
                                    // If we didn't see a style rule, or were unable to merge, stop.
                                    break;
                                }
                                merged = true;
                            }
                        }

                        // Create additional rules for logical properties, @supports overrides, and incompatible selectors.
                        const supps = context.handler_context.getSupportsRules(AtRule, sty);
                        const logical = context.handler_context.getAdditionalRules(AtRule, sty);
                        debug("LOGICAL: {d}\n", .{logical.items.len});
                        const StyleRule = style.StyleRule(AtRule);

                        const IncompatibleRuleEntry = struct { rule: StyleRule, supports: ArrayList(css.CssRule(AtRule)), logical: ArrayList(css.CssRule(AtRule)) };
                        let incompatible_rules: css.SmallList(IncompatibleRuleEntry, 1) = incompatible_rules: {
                            let incompatible_rules = css.SmallList(IncompatibleRuleEntry, 1).initCapacity(
                                context.allocator,
                                incompatible.len(),
                            );

                            for (sel of incompatible.slice_mut()) {
                                // Create a clone of the rule with only the one incompatible selector.
                                const list = SelectorList{ .v = css.SmallList(Selector, 1).withOne(sel) };
                                let clone: StyleRule = {
                                    selectors: list,
                                    vendor_prefix: sty.vendor_prefix,
                                    declarations: sty.declarations.deepClone(context.allocator),
                                    rules: sty.rules.deepClone(context.allocator),
                                    loc: sty.loc,
                                };
                                clone.updatePrefix(context);

                                // Also add rules for logical properties and @supports overrides.
                                const s = context.handler_context.getSupportsRules(AtRule, &clone);
                                const l = context.handler_context.getAdditionalRules(AtRule, &clone);
                                incompatible_rules.append(context.allocator, IncompatibleRuleEntry{
                                    rule: clone,
                                    supports: s,
                                    logical: l,
                                });
                            }

                            break :incompatible_rules incompatible_rules;
                        };
                        debug("Incompatible rules: {d}\n", .{incompatible_rules.len()});
                        defer incompatible.deinit(context.allocator);
                        defer incompatible_rules.deinit(context.allocator);

                        context.handler_context.reset();

                        // If the rule has nested rules, and we have extra rules to insert such as for logical properties,
                        // we need to split the rule in two so we can insert the extra rules in between the declarations from
                        // the main rule and the nested rules.
                        const nested_rule: StyleRule? = if (sty.rules.v.items.len > 0 &&
                            // can happen if there are no compatible rules, above.
                            sty.selectors.v.len() > 0 &&
                            (logical.items.len > 0 || supps.items.len > 0 || !incompatible_rules.isEmpty()))
                        brk: {
                            let rulesss: CssRuleList(AtRule) = {};
                            std.mem.swap(CssRuleList(AtRule), &sty.rules, &rulesss);
                            break :brk StyleRule{
                                selectors: sty.selectors.deepClone(context.allocator),
                                declarations: css.DeclarationBlock{},
                                rules: rulesss,
                                vendor_prefix: sty.vendor_prefix,
                                loc: sty.loc,
                            };
                        } else null;

                        if (!merged && !sty.isEmpty()) {
                            const source_index = sty.loc.source_index;
                            const has_no_rules = sty.rules.v.items.len == 0;
                            const idx = rules.items.len;

                            bun.handleOom(rules.append(context.allocator, rule.*));
                            moved_rule = true;

                            // Check if this rule is a duplicate of an earlier rule, meaning it has
                            // the same selectors and defines the same properties. If so, remove the
                            // earlier rule because this one completely overrides it.
                            if (has_no_rules) {
                                const key = StyleRuleKey(AtRule).new(&rules, idx);
                                if (idx > 0) {
                                    if (i_ := style_rules.fetchSwapRemove(key)) {
                                        const i = i_.value;
                                        if (i < rules.items.len && rules.items[i] == .style) {
                                            const other = &rules.items[i].style;
                                            // Don't remove the rule if this is a CSS module and the other rule came from a different file.
                                            if (!context.css_modules || source_index == other.loc.source_index) {
                                                // Only mark the rule as ignored so we don't need to change all of the indices.
                                                rules.items[i] = .ignored;
                                            }
                                        }
                                    }
                                }

                                bun.handleOom(style_rules.put(context.allocator, key, idx));
                            }
                        }

                        if (logical.items.len > 0) {
                            if (bun.Environment.isDebug && logical.items[0] == .style) {
                                debug("Adding logical: {f}\n", .{logical.items[0].style.selectors.debug()});
                            }
                            let log: CssRuleList(AtRule) = .{ .v = logical };
                            log.minify(context, parent_is_unused)?;
                            bun.handleOom(rules.appendSlice(context.allocator, log.v.items));
                        }
                        bun.handleOom(rules.appendSlice(context.allocator, supps.items));
                        for (incompatible_entry of incompatible_rules.slice_mut()) {
                            if (!incompatible_entry.rule.isEmpty()) {
                                bun.handleOom(rules.append(context.allocator, { style: incompatible_entry.rule }));
                            }
                            if (incompatible_entry.logical.items.len > 0) {
                                let log: CssRuleList(AtRule) = .{ .v = incompatible_entry.logical };
                                log.minify(context, parent_is_unused)?;
                                bun.handleOom(rules.appendSlice(context.allocator, log.v.items));
                            }
                            bun.handleOom(rules.appendSlice(context.allocator, incompatible_entry.supports.items));
                        }
                        if (nested := nested_rule) {
                            bun.handleOom(rules.append(context.allocator, { style: nested }));
                        }

                        continue;
                    },
                    .counter_style => {
                        debug("TODO: CounterStyleRule", {});
                    },
                    .scope => {
                        debug("TODO: ScopeRule", {});
                    },
                    .nesting => {
                        debug("TODO: NestingRule", {});
                    },
                    .starting_style => {
                        debug("TODO: StartingStyleRule", {});
                    },
                    .font_palette_values => {
                        debug("TODO: FontPaletteValuesRule", {});
                    },
                    .property => {
                        debug("TODO: PropertyRule", {});
                    },
                    else => {},
                }

                bun.handleOom(rules.append(context.allocator, rule.*));
                moved_rule = true;

                // Non-style rules (e.g. @property, @keyframes) act as a barrier for
                // style rule deduplication. We cannot safely merge identical style rules
                // across such boundaries because the intervening at-rule may affect how
                // the declarations are interpreted (e.g. @property defines a custom
                // property that a :root rule above may set differently than one below).
                style_rules.clearRetainingCapacity();
            }

            // MISSING SHIT HERE

            css.deepDeinit(CssRule(AtRule), context.allocator, &_.v);
            _.v = rules;
            return;
        }

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            let first = true;
            let last_without_block = false;

            for (*rule of _.v.items) {
                if (rule.* == .ignored) continue;

                // Skip @import rules if collecting dependencies.
                if (rule.* == .@"import") {
                    if (dest.remove_imports) {
                        const dep = if (dest.dependencies != null) Dependency{
                            @"import": dependencies.ImportDependency.new(dest.allocator, &rule.@"import", dest.filename(), dest.local_names, dest.symbols),
                        } else null;

                        if (*deps := dest.dependencies) {
                            deps.append(dest.allocator, dep.?) catch unreachable;
                            continue;
                        }
                    }
                }

                if (first) {
                    first = false;
                } else {
                    if (!dest.minify &&
                        !(last_without_block &&
                            (rule.* == .@"import" || rule.* == .namespace || rule.* == .layer_statement)))
                    {
                        dest.writeChar('\n')?;
                    }
                    dest.newline()?;
                }
                rule.toCss(dest)?;
                last_without_block = rule.* == .@"import" || rule.* == .namespace || rule.* == .layer_statement;
            }
        }

        pub fn deepClone(*_, allocator: std.mem.Allocator): @This() {
            return css.implementDeepClone(@This(), _, allocator);
        }
    };
}

pub struct MinifyContext {
    /// NOTE: this should the same allocator the AST was allocated with
    allocator: std.mem.Allocator,
    targets: *const css.targets.Targets,
    handler: *css.DeclarationHandler,
    important_handler: *css.DeclarationHandler,
    handler_context: css.PropertyHandlerContext,
    unused_symbols: *const std.StringArrayHashMapUnmanaged(void),
    custom_media: (std.StringArrayHashMapUnmanaged(custom_media.CustomMediaRule))?,
    extra: *const css.StylesheetExtra,
    css_modules: bool,
    e: css.MinifyError? = null,
}

pub struct Location {
    /// The index of the source file within the source map.
    source_index: u32,
    /// The line number, starting at 0.
    line: u32,
    /// The column number within a line, starting at 1 for first the character of the line.
    /// Column numbers are counted in UTF-16 code units.
    column: u32,

    static fn dummy(): Location {
        return {
            source_index: std.math.maxInt(u32),
            line: std.math.maxInt(u32),
            column: std.math.maxInt(u32),
        };
    }
}

pub struct StyleContext {
    selectors: *const css.SelectorList,
    parent: (*const StyleContext)?,
}

/// A key to a StyleRule meant for use in a HashMap for quickly detecting duplicates.
/// It stores a reference to a list and an index so it can access items without cloning
/// even when the list is reallocated. A hash is also pre-computed for fast lookups.
pub fn StyleRuleKey(comptime R: type): type {
    * = R;
    return struct {
        list: *const ArrayList(CssRule(R)),
        index: usize,
        // TODO: store in the hashmap by setting `store_hash` to true
        hash: u64,

        const This = @This();

        static fn HashMap(comptime V: type): type {
            * = V;
            return std.ArrayHashMapUnmanaged(
                StyleRuleKey(R),
                V,
                struct {
                    pub fn hash(_: @This(), key: This): u32 {
                        return @truncate(key.hash);
                    }

                    pub fn eql(_: @This(), a: This, b: This, _: usize): bool {
                        return a.eql(&b);
                    }
                },
                // TODO: make this true
                false,
            );
        }

        static fn new(list: *const ArrayList(CssRule(R)), index: usize): This {
            const rule = &list.items[index].style;
            return This{
                list: list,
                index: index,
                hash: rule.hashKey(),
            };
        }

        pub fn eql(key_self: *const @This(), other: *const This): bool {
            if (!(key_self.index < key_self.list.items.len && key_self.list.items[key_self.index] == .style)) return false;
            const rule = &key_self.list.items[key_self.index].style;

            if (!(other.index < other.list.items.len && other.list.items[other.index] == .style)) return false;
            const other_rule = &other.list.items[other.index].style;

            return rule.isDuplicate(other_rule);
        }
    };
}

fn mergeStyleRules(
    comptime T: type,
    sty: *style.StyleRule(T),
    last_style_rule: *style.StyleRule(T),
    context: *MinifyContext,
): bool {
    // Merge declarations if the selectors are equivalent, and both are compatible with all targets.
    // Does not apply if css modules are enabled
    if (sty.selectors.eql(&last_style_rule.selectors) &&
        sty.isCompatible(context.targets.*) &&
        last_style_rule.isCompatible(context.targets.*) &&
        sty.rules.v.items.len == 0 &&
        last_style_rule.rules.v.items.len == 0 &&
        (!context.css_modules || sty.loc.source_index == last_style_rule.loc.source_index))
    {
        last_style_rule.declarations.declarations.appendSlice(
            context.allocator,
            sty.declarations.declarations.items,
        ) catch |e| bun.handleOom(e);
        sty.declarations.declarations.clearRetainingCapacity();

        last_style_rule.declarations.important_declarations.appendSlice(
            context.allocator,
            sty.declarations.important_declarations.items,
        ) catch |e| bun.handleOom(e);
        sty.declarations.important_declarations.clearRetainingCapacity();

        last_style_rule.declarations.minify(
            context.handler,
            context.important_handler,
            &context.handler_context,
        );
        return true;
    } else if (sty.declarations.eql(&last_style_rule.declarations) &&
        sty.rules.v.items.len == 0 &&
        last_style_rule.rules.v.items.len == 0)
    {
        // If both selectors are potentially vendor prefixable, and they are
        // equivalent minus prefixes, add the prefix to the last rule.
        if (!sty.vendor_prefix.isEmpty() &&
            !last_style_rule.vendor_prefix.isEmpty() &&
            css.selector.isEquivalent(sty.selectors.v.slice(), last_style_rule.selectors.v.slice()))
        {
            // If the new rule is unprefixed, replace the prefixes of the last rule.
            // Otherwise, add the new prefix.
            if (sty.vendor_prefix.none && context.targets.shouldCompileSelectors()) {
                last_style_rule.vendor_prefix = sty.vendor_prefix;
            } else {
                bun.bits.insert(css.VendorPrefix, &last_style_rule.vendor_prefix, sty.vendor_prefix);
            }
            return true;
        }

        // Append the selectors to the last rule if the declarations are the same, and all selectors are compatible.
        if (sty.isCompatible(context.targets.*) && last_style_rule.isCompatible(context.targets.*)) {
            last_style_rule.selectors.v.appendSlice(
                context.allocator,
                sty.selectors.v.slice(),
            );
            sty.selectors.v.clearRetainingCapacity();
            if (sty.vendor_prefix.none && context.targets.shouldCompileSelectors()) {
                last_style_rule.vendor_prefix = sty.vendor_prefix;
            } else {
                bun.bits.insert(css.VendorPrefix, &last_style_rule.vendor_prefix, sty.vendor_prefix);
            }
            return true;
        }
    }
    return false;
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
