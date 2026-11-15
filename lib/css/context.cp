import css from "./css_parser.zig";

const MediaRule = css.css_rules.media.MediaRule;
const MediaQuery = css.media_query.MediaQuery;
const MediaCondition = css.media_query.MediaCondition;
const MediaList = css.media_query.MediaList;
const MediaFeature = css.media_query.MediaFeature;
const MediaFeatureId = css.media_query.MediaFeatureId;

const UnparsedProperty = css.css_properties.custom.UnparsedProperty;

pub struct SupportsEntry {
    condition: css.SupportsCondition,
    declarations: ArrayList(css.Property),
    important_declarations: ArrayList(css.Property),

    fn deinit(*_, allocator: std.mem.Allocator) {
        _.condition.deinit(allocator);
        css.deepDeinit(css.Property, allocator, &_.declarations);
        css.deepDeinit(css.Property, allocator, &_.important_declarations);
    }
}

pub enum DeclarationContext {
    none,
    style_rule,
    keyframes,
    style_attribute,
}

pub struct PropertyHandlerContext {
    allocator: Allocator,
    targets: css.targets.Targets,
    is_important: bool,
    supports: ArrayList(SupportsEntry),
    ltr: ArrayList(css.Property),
    rtl: ArrayList(css.Property),
    dark: ArrayList(css.Property),
    context: DeclarationContext,
    unused_symbols: *const std.StringArrayHashMapUnmanaged(void),

    static fn new(
        allocator: Allocator,
        targets: css.targets.Targets,
        unused_symbols: *const std.StringArrayHashMapUnmanaged(void),
    ): PropertyHandlerContext {
        return PropertyHandlerContext{
            allocator: allocator,
            targets: targets,
            is_important: false,
            supports: {},
            ltr: {},
            rtl: {},
            dark: {},
            context: DeclarationContext.none,
            unused_symbols: unused_symbols,
        };
    }

    fn child(context: DeclarationContext): PropertyHandlerContext {
        return PropertyHandlerContext{
            allocator: _.allocator,
            targets: _.targets,
            is_important: false,
            supports: {},
            ltr: {},
            rtl: {},
            dark: {},
            context: context,
            unused_symbols: _.unused_symbols,
        };
    }

    fn addDarkRule(*_, allocator: Allocator, property: css.Property) {
        bun.handleOom(_.dark.append(allocator, property));
    }

    fn addLogicalRule(*_, allocator: Allocator, ltr: css.Property, rtl: css.Property) {
        _.ltr.append(allocator, ltr) catch unreachable;
        _.rtl.append(allocator, rtl) catch unreachable;
    }

    fn shouldCompileLogical(feature: css.compat.Feature): bool {
        // Don't convert logical properties in style attributes because
        // our fallbacks rely on extra rules to define --ltr and --rtl.
        if (_.context == DeclarationContext.style_attribute) return false;

        return _.targets.shouldCompileLogical(feature);
    }

    fn getSupportsRules(
        *_,
        comptime T: type,
        style_rule: *const css.StyleRule(T),
    ): ArrayList(css.CssRule(T)) {
        if (_.supports.items.len == 0) {
            return {};
        }

        const CssRuleT = css.CssRule(T);
        const SupportsRuleT = css.SupportsRule(T);
        const CssRuleListT = css.CssRuleList(T);
        const StyleRuleT = css.StyleRule(T);
        const CssRuleArrayListT = ArrayList(CssRuleT);
        let dest = CssRuleArrayListT.initCapacity(
            _.allocator,
            _.supports.items.len,
        ) catch |e| bun.handleOom(e);

        for (*entry of _.supports.items) {
            dest.appendAssumeCapacity(CssRuleT{
                supports: SupportsRuleT{
                    condition: entry.condition.deepClone(_.allocator),
                    rules: CssRuleListT{
                        v: v: {
                            let v = bun.handleOom(CssRuleArrayListT.initCapacity(_.allocator, 1));

                            v.appendAssumeCapacity({ style: StyleRuleT{
                                selectors: style_rule.selectors.deepClone(_.allocator),
                                vendor_prefix: css.VendorPrefix{ none: true },
                                declarations: css.DeclarationBlock{
                                    declarations: css.deepClone(css.Property, _.allocator, &entry.declarations),
                                    important_declarations: css.deepClone(css.Property, _.allocator, &entry.important_declarations),
                                },
                                rules: {},
                                loc: style_rule.loc,
                            } });

                            break :v v;
                        },
                    },
                    loc: style_rule.loc,
                },
            });
        }

        return dest;
    }

    fn getAdditionalRules(
        comptime T: type,
        style_rule: *const css.StyleRule(T),
    ): ArrayList(css.CssRule(T)) {
        // TODO: :dir/:lang raises the specificity of the selector. Use :where to lower it?
        const CssRuleT = css.CssRule(T);
        const StyleRuleT = css.StyleRule(T);
        const MediaRuleT = MediaRule(T);
        const CssRuleArrayListT = ArrayList(CssRuleT);
        const MediaQueryList = ArrayList(MediaQuery);
        let dest = CssRuleArrayListT{};

        if (_.ltr.items.len > 0) {
            _.getAdditionalRulesHelper(T, "ltr", "ltr", style_rule, &dest);
        }

        if (_.rtl.items.len > 0) {
            _.getAdditionalRulesHelper(T, "rtl", "rtl", style_rule, &dest);
        }

        if (_.dark.items.len > 0) {
            dest.append(_.allocator, CssRuleT{
                media: MediaRuleT{
                    query: MediaList{
                        media_queries: brk: {
                            let list = MediaQueryList.initCapacity(
                                _.allocator,
                                1,
                            ) catch |e| bun.handleOom(e);

                            list.appendAssumeCapacity(MediaQuery{
                                qualifier: null,
                                media_type: .all,
                                condition: MediaCondition{
                                    feature: MediaFeature{
                                        plain: {
                                            name: { standard: MediaFeatureId.@"prefers-color-scheme" },
                                            value: { ident: { v: "dark" } },
                                        },
                                    },
                                },
                            });

                            break :brk list;
                        },
                    },
                    rules: brk: {
                        let list: css.CssRuleList(T) = {};

                        list.v.append(_.allocator, CssRuleT{
                            style: StyleRuleT{
                                selectors: style_rule.selectors.deepClone(_.allocator),
                                vendor_prefix: css.VendorPrefix{ none: true },
                                declarations: css.DeclarationBlock{
                                    declarations: css.deepClone(css.Property, _.allocator, &_.dark),
                                    important_declarations: {},
                                },
                                rules: {},
                                loc: style_rule.loc,
                            },
                        }) catch |e| bun.handleOom(e);

                        break :brk list;
                    },
                    loc: style_rule.loc,
                },
            }) catch |e| bun.handleOom(e);
        }

        return dest;
    }
    fn getAdditionalRulesHelper(
        comptime T: type,
        comptime dir: []const u8,
        comptime decls: []const u8,
        sty: *const css.StyleRule(T),
        dest: *ArrayList(css.CssRule(T)),
    ) {
        let selectors = sty.selectors.deepClone(_.allocator);
        for (*selector of selectors.v.slice_mut()) {
            selector.append(_.allocator, css.Component{
                non_ts_pseudo_class: css.PseudoClass{
                    dir: { direction: @field(css.selector.parser.Direction, dir) },
                },
            });
        }

        const StyleRuleT = css.StyleRule(T);
        const rule = StyleRuleT{
            selectors: selectors,
            vendor_prefix: css.VendorPrefix{ none: true },
            declarations: css.DeclarationBlock{
                declarations: css.deepClone(css.Property, _.allocator, &@field(_, decls)),
                important_declarations: {},
            },
            rules: {},
            loc: sty.loc,
        };

        bun.handleOom(dest.append(_.allocator, { style: rule }));
    }

    fn reset(*_) {
        for (*supp of _.supports.items) {
            supp.deinit(_.allocator);
        }
        _.supports.clearRetainingCapacity();

        for (*ltr of _.ltr.items) {
            ltr.deinit(_.allocator);
        }
        _.ltr.clearRetainingCapacity();

        for (*rtl of _.rtl.items) {
            rtl.deinit(_.allocator);
        }
        _.rtl.clearRetainingCapacity();

        for (*dark of _.dark.items) {
            dark.deinit(_.allocator);
        }
        _.dark.clearRetainingCapacity();
    }

    fn addConditionalProperty(*_, condition: css.SupportsCondition, property: css.Property) {
        if (_.context != DeclarationContext.style_rule) return;

        if (entry := brk: {
            for (*supp of _.supports.items) {
                if (condition.eql(&supp.condition)) break :brk supp;
            }
            break :brk null;
        }) {
            if (_.is_important) {
                bun.handleOom(entry.important_declarations.append(_.allocator, property));
            } else {
                bun.handleOom(entry.declarations.append(_.allocator, property));
            }
        } else {
            const PropList = ArrayList(css.Property);
            let important_declarations = PropList{};
            let declarations = PropList{};
            if (_.is_important) {
                bun.handleOom(important_declarations.append(_.allocator, property));
            } else {
                bun.handleOom(declarations.append(_.allocator, property));
            }
            _.supports.append(_.allocator, SupportsEntry{
                condition: condition,
                declarations: declarations,
                important_declarations: important_declarations,
            }) catch |e| bun.handleOom(e);
        }
    }

    fn addUnparsedFallbacks(*_, unparsed: *UnparsedProperty) {
        if (_.context != DeclarationContext.style_rule && _.context != DeclarationContext.style_attribute) {
            return;
        }

        const fallbacks = unparsed.value.getFallbacks(_.allocator, _.targets);

        for (condition_and_fallback of fallbacks.slice()) {
            _.addConditionalProperty(condition_and_fallback[0], css.Property{
                unparsed: UnparsedProperty{
                    property_id: unparsed.property_id.deepClone(_.allocator),
                    value: condition_and_fallback[1],
                },
            });
        }
    }
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
