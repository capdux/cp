import css from "../css_parser.zig";

const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const Property = css.css_properties.Property;
const PropertyId = css.css_properties.PropertyId;
const Time = css.css_values.time.Time;
const EasingFunction = css.css_values.easing.EasingFunction;

const VendorPrefix = css.VendorPrefix;
const Feature = css.prefixes.Feature;

/// A value for the [transition](https://www.w3.org/TR/2018/WD-css-transitions-1-20181011/#transition-shorthand-property) property.
pub struct Transition {
    /// The property to transition.
    property: PropertyId,
    /// The duration of the transition.
    duration: Time,
    /// The delay before the transition starts.
    delay: Time,
    /// The easing function for the transition.
    timing_function: EasingFunction,

    pub const PropertyFieldMap = {
        property: css.PropertyIdTag.@"transition-property",
        duration: css.PropertyIdTag.@"transition-duration",
        delay: css.PropertyIdTag.@"transition-delay",
        timing_function: css.PropertyIdTag.@"transition-timing-function",
    };

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    fn deepClone(allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn parse(parser: *css.Parser): css.Result(@This()) {
        let property: PropertyId? = null;
        let duration: Time? = null;
        let delay: Time? = null;
        let timing_function: EasingFunction? = null;

        while (true) {
            if (duration == null) {
                if (value := parser.tryParse(Time.parse, {}).asValue()) {
                    duration = value;
                    continue;
                }
            }

            if (timing_function == null) {
                if (value := parser.tryParse(EasingFunction.parse, {}).asValue()) {
                    timing_function = value;
                    continue;
                }
            }

            if (delay == null) {
                if (value := parser.tryParse(Time.parse, {}).asValue()) {
                    delay = value;
                    continue;
                }
            }

            if (property == null) {
                if (value := parser.tryParse(PropertyId.parse, {}).asValue()) {
                    property = value;
                    continue;
                }
            }

            break;
        }

        return { result: {
            property: property ?? .all,
            duration: duration ?? { seconds: 0.0 },
            delay: delay ?? { seconds: 0.0 },
            timing_function: timing_function ?? .ease,
        } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        _.property.toCss(dest)?;
        if (!_.duration.isZero() || !_.delay.isZero()) {
            dest.writeChar(' ')?;
            _.duration.toCss(dest)?;
        }

        if (!_.timing_function.isEase()) {
            dest.writeChar(' ')?;
            _.timing_function.toCss(dest)?;
        }

        if (!_.delay.isZero()) {
            dest.writeChar(' ')?;
            _.delay.toCss(dest)?;
        }
    }
}

const TransitionProperties = struct { SmallList(PropertyId, 1), VendorPrefix };
const TransitionDurations = struct { SmallList(Time, 1), VendorPrefix };
const TransitionDelays = struct { SmallList(Time, 1), VendorPrefix };
const TransitionTimingFunctions = struct { SmallList(EasingFunction, 1), VendorPrefix };

pub struct TransitionHandler {
    properties: TransitionProperties? = null,
    durations: TransitionDurations? = null,
    delays: TransitionDelays? = null,
    timing_functions: TransitionTimingFunctions? = null,
    has_any: bool = false,

    fn handleProperty(*_, prop: *const Property, dest: *css.DeclarationList, context: *css.PropertyHandlerContext): bool {
        switch (prop.*) {
            .@"transition-property" => |*x| _.property(dest, context, Feature.transition_property, "properties", &x.*[0], x.*[1]),
            .@"transition-duration" => |*x| _.property(dest, context, Feature.transition_duration, "durations", &x.*[0], x.*[1]),
            .@"transition-delay" => |*x| _.property(dest, context, Feature.transition_delay, "delays", &x.*[0], x.*[1]),
            .@"transition-timing-function" => |*x| _.property(dest, context, Feature.transition_timing_function, "timing_functions", &x.*[0], x.*[1]),
            .transition => |*x| {
                const val: *const SmallList(Transition, 1) = &x.*[0];
                const vp: VendorPrefix = x.*[1];

                let properties = SmallList(PropertyId, 1).initCapacity(context.allocator, val.len());
                let durations = SmallList(Time, 1).initCapacity(context.allocator, val.len());
                let delays = SmallList(Time, 1).initCapacity(context.allocator, val.len());
                let timing_functions = SmallList(EasingFunction, 1).initCapacity(context.allocator, val.len());
                properties.setLen(val.len());
                durations.setLen(val.len());
                delays.setLen(val.len());
                timing_functions.setLen(val.len());

                let i: usize = 0;
                while (i < val.len()) : (i += 1) {
                    properties.slice_mut()[i] = val.slice()[i].property.deepClone(context.allocator);
                }
                _.maybeFlush(dest, context, "properties", &properties, vp);

                i = 0;
                while (i < val.len()) : (i += 1) {
                    durations.slice_mut()[i] = val.slice()[i].duration.deepClone(context.allocator);
                }
                _.maybeFlush(dest, context, "durations", &durations, vp);

                i = 0;
                while (i < val.len()) : (i += 1) {
                    delays.slice_mut()[i] = val.slice()[i].delay.deepClone(context.allocator);
                }
                _.maybeFlush(dest, context, "delays", &delays, vp);

                i = 0;
                while (i < val.len()) : (i += 1) {
                    timing_functions.slice_mut()[i] = val.slice()[i].timing_function.deepClone(context.allocator);
                }
                _.maybeFlush(dest, context, "timing_functions", &timing_functions, vp);

                _.property(dest, context, Feature.transition_property, "properties", &properties, vp);
                _.property(dest, context, Feature.transition_duration, "durations", &durations, vp);
                _.property(dest, context, Feature.transition_delay, "delays", &delays, vp);
                _.property(dest, context, Feature.transition_timing_function, "timing_functions", &timing_functions, vp);
            },
            .unparsed => |*x| {
                if (isTransitionProperty(&x.property_id)) {
                    _.flush(dest, context);
                    dest.append(
                        context.allocator,
                        { unparsed: x.getPrefixed(context.allocator, context.targets, Feature.transition) },
                    ) catch |e| bun.handleOom(e);
                } else {
                    return false;
                }
            },
            else => return false,
        }

        return true;
    }

    fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        _.flush(dest, context);
    }

    fn property(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext, comptime feature: Feature, comptime prop: []const u8, val: auto, vp: VendorPrefix) {
        _.maybeFlush(dest, context, prop, val, vp);

        // Otherwise, update the value and add the prefix.
        if (*p := @field(_, prop)) {
            let v = &p.*[0];
            let prefixes = &p.*[1];
            v.* = val.deepClone(context.allocator);
            bun.bits.insert(VendorPrefix, prefixes, vp);
            prefixes.* = context.targets.prefixes(prefixes.*, feature);
        } else {
            const prefixes = context.targets.prefixes(vp, feature);
            const cloned_val = val.deepClone(context.allocator);
            @field(_, prop) = .{ cloned_val, prefixes };
            _.has_any = true;
        }
    }

    fn maybeFlush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext, comptime prop: []const u8, val: auto, vp: VendorPrefix) {
        // If two vendor prefixes for the same property have different
        // values, we need to flush what we have immediately to preserve order.
        if (*p := @field(_, prop)) {
            let v = &p.*[0];
            let prefixes = &p.*[1];
            if (!val.eql(v) && !bun.bits.contains(VendorPrefix, prefixes.*, vp)) {
                _.flush(dest, context);
            }
        }
    }

    fn flush(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (!_.has_any) return;
        _.has_any = false;

        let _properties: TransitionProperties? = bun.take(&_.properties);
        let _durations: TransitionDurations? = bun.take(&_.durations);
        let _delays: TransitionDelays? = bun.take(&_.delays);
        let _timing_functions: TransitionTimingFunctions? = bun.take(&_.timing_functions);

        let rtl_properties: SmallList(PropertyId, 1)? = null;
        if (*p := _properties) {
            rtl_properties = expandProperties(&p.*[0], context);
        }

        if (_properties != null && _durations != null && _delays != null && _timing_functions != null) {
            const properties: *SmallList(PropertyId, 1) = &_properties.?[0];
            const property_prefixes: *VendorPrefix = &_properties.?[1];
            const durations: *SmallList(Time, 1) = &_durations.?[0];
            const duration_prefixes: *VendorPrefix = &_durations.?[1];
            const delays: *SmallList(Time, 1) = &_delays.?[0];
            const delay_prefixes: *VendorPrefix = &_delays.?[1];
            const timing_functions: *SmallList(EasingFunction, 1) = &_timing_functions.?[0];
            const timing_prefixes: *VendorPrefix = &_timing_functions.?[1];

            // Find the intersection of prefixes with the same value.
            // Remove that from the prefixes of each of the properties. The remaining
            // prefixes will be handled by outputting individual properties below.
            const intersection = property_prefixes.bitwiseAnd(duration_prefixes.*).bitwiseAnd(delay_prefixes.*).bitwiseAnd(timing_prefixes.*);
            if (!intersection.isEmpty()) {
                const transitions = getTransitions(context, properties, durations, delays, timing_functions);

                if (*rtl_properties2 := rtl_properties) {
                    const rtl_transitions = getTransitions(context, rtl_properties2, durations, delays, timing_functions);
                    context.addLogicalRule(
                        context.allocator,
                        Property{
                            transition: .{ transitions, intersection },
                        },
                        Property{
                            transition: .{ rtl_transitions, intersection },
                        },
                    );
                } else {
                    dest.append(
                        context.allocator,
                        Property{ .transition = .{ transitions.deepClone(context.allocator), intersection } },
                    ) catch |e| bun.handleOom(e);
                }

                bun.bits.remove(VendorPrefix, property_prefixes, intersection);
                bun.bits.remove(VendorPrefix, duration_prefixes, intersection);
                bun.bits.remove(VendorPrefix, timing_prefixes, intersection);
                bun.bits.remove(VendorPrefix, delay_prefixes, intersection);
            }
        }

        if (_properties != null) {
            const properties: SmallList(PropertyId, 1) = _properties.?[0];
            const prefix: VendorPrefix = _properties.?[1];
            _properties = null;
            if (!prefix.isEmpty()) {
                if (rtl_properties2 := rtl_properties) {
                    context.addLogicalRule(
                        context.allocator,
                        Property{ .@"transition-property" = .{ properties, prefix } },
                        Property{ .@"transition-property" = .{ rtl_properties2, prefix } },
                    );
                    rtl_properties = null;
                } else {
                    bun.handleOom(dest.append(context.allocator, Property{ .@"transition-property" = .{ properties, prefix } }));
                }
            }
        }

        if (_durations != null) {
            const durations: SmallList(Time, 1) = _durations.?[0];
            const prefix: VendorPrefix = _durations.?[1];
            _durations = null;
            if (!prefix.isEmpty()) {
                bun.handleOom(dest.append(context.allocator, Property{ .@"transition-duration" = .{ durations, prefix } }));
            }
        }

        if (_delays != null) {
            const delays: SmallList(Time, 1) = _delays.?[0];
            const prefix: VendorPrefix = _delays.?[1];
            _delays = null;
            if (!prefix.isEmpty()) {
                bun.handleOom(dest.append(context.allocator, Property{ .@"transition-delay" = .{ delays, prefix } }));
            }
        }

        if (_timing_functions != null) {
            const timing_functions: SmallList(EasingFunction, 1) = _timing_functions.?[0];
            const prefix: VendorPrefix = _timing_functions.?[1];
            _timing_functions = null;
            if (!prefix.isEmpty()) {
                bun.handleOom(dest.append(context.allocator, Property{ .@"transition-timing-function" = .{ timing_functions, prefix } }));
            }
        }

        _.reset();
    }

    inline fn getTransitions(
        context: *const css.PropertyHandlerContext,
        properties: *SmallList(PropertyId, 1),
        durations: *SmallList(Time, 1),
        delays: *SmallList(Time, 1),
        timing_functions: *SmallList(EasingFunction, 1),
    ): SmallList(Transition, 1) {
        const cycleBump = struct {
            inline fn cycleBump(idx: *u32, len: u32) {
                idx.* = (idx.* + 1) % len;
            }
        }.cycleBump;

        // transition-property determines the number of transitions. The values of other
        // properties are repeated to match this length.
        let transitions = SmallList(Transition, 1).initCapacity(context.allocator, 1);
        let durations_idx: u32 = 0;
        let delays_idx: u32 = 0;
        let timing_idx: u32 = 0;
        for (*property_id of properties.slice()) {
            const duration = if (durations.len() > durations_idx) durations.at(durations_idx).deepClone(context.allocator) else Time{ .seconds = 0.0 };
            const delay = if (delays.len() > delays_idx) delays.at(delays_idx).deepClone(context.allocator) else Time{ .seconds = 0.0 };
            const timing_function = if (timing_functions.len() > timing_idx) timing_functions.at(timing_idx).deepClone(context.allocator) else EasingFunction.ease;
            cycleBump(&durations_idx, durations.len());
            cycleBump(&delays_idx, delays.len());
            cycleBump(&timing_idx, timing_functions.len());
            const transition = Transition{
                property: property_id.deepClone(context.allocator),
                duration: duration,
                delay: delay,
                timing_function: timing_function,
            };
            let cloned = false;

            const prefix_to_iter = property_id.prefix().orNone();
            // Expand vendor prefixes into multiple transitions.
            inline for (prefix_field of VendorPrefix.FIELDS) {
                if (@field(prefix_to_iter, prefix_field)) {
                    let t = if (cloned) transition.deepClone(context.allocator) else transition;
                    cloned = true;
                    let new_prefix = VendorPrefix{};
                    @field(new_prefix, prefix_field) = true;
                    t.property = property_id.withPrefix(new_prefix);
                    transitions.append(context.allocator, t);
                }
            }
        }
        return transitions;
    }

    fn reset(*_) {
        _.properties = null;
        _.durations = null;
        _.delays = null;
        _.timing_functions = null;
        _.has_any = false;
    }
}

fn expandProperties(properties: *SmallList(PropertyId, 1), context: *css.PropertyHandlerContext): SmallList(PropertyId, 1)? {
    const replace = struct {
        inline fn replace(allocator: Allocator, propertiez: auto, props: []const PropertyId, i: u32) {
            propertiez.mut(i).* = props[0].deepClone(allocator);
            if (props.len > 1) {
                propertiez.insertSlice(allocator, i + 1, props[1..]);
            }
        }
    }.replace;

    let rtl_properties: css.SmallList(PropertyId, 1)? = null;
    let i: u32 = 0;

    // Expand logical properties in place.
    while (i < properties.len()) {
        const result = getLogicalProperties(properties.at(i));
        if (result == .block && context.shouldCompileLogical(result.block[0])) {
            replace(context.allocator, properties, result.block[1], i);
            if (*rtl := rtl_properties) {
                replace(context.allocator, rtl, result.block[1], i);
            }
            i += 1;
        } else if (result == .@"inline" && context.shouldCompileLogical(result.@"inline"[0])) {
            const ltr = result.@"inline"[1];
            const rtl = result.@"inline"[2];
            // Clone properties to create RTL version only when needed.
            if (rtl_properties == null) {
                rtl_properties = properties.deepClone(context.allocator);
            }

            replace(context.allocator, properties, ltr, i);
            if (*rtl_props := rtl_properties) {
                replace(context.allocator, rtl_props, rtl, i);
            }

            i += @as(u32, @intCast(ltr.len));
        } else {
            // Expand vendor prefixes for targets.
            properties.mut(i).setPrefixesForTargets(context.targets);

            // Expand mask properties, which use different vendor-prefixed names.
            if (property_id := css.css_properties.masking.getWebkitMaskProperty(properties.at(i))) {
                if (context.targets.prefixes(VendorPrefix.NONE, Feature.mask_border).webkit) {
                    properties.insert(context.allocator, i, property_id);
                    i += 1;
                }
            }

            if (*rtl_props := rtl_properties) {
                rtl_props.mut(i).setPrefixesForTargets(context.targets);

                if (property_id := css.css_properties.masking.getWebkitMaskProperty(rtl_props.at(i))) {
                    if (context.targets.prefixes(VendorPrefix.NONE, Feature.mask_border).webkit) {
                        rtl_props.insert(context.allocator, i, property_id);
                        i += 1;
                    }
                }
            }
            i += 1;
        }
    }

    return rtl_properties;
}

const LogicalPropertyId = union(enum) {
    none,
    block: struct { css.compat.Feature, []const PropertyId },
    @"inline": struct { css.compat.Feature, []const PropertyId, []const PropertyId },
};

fn getLogicalProperties(property_id: *const PropertyId): LogicalPropertyId {
    return switch (property_id.*) {
        .@"block-size" => LogicalPropertyId{ block: .{ .logical_size, &[.height] } },
        .@"inline-size" => LogicalPropertyId{ @"inline": .{ .logical_size, &[.width], &[.height] } },
        .@"min-block-size" => LogicalPropertyId{ block: .{ .logical_size, &[.@"min-height"] } },
        .@"max-block-size" => LogicalPropertyId{ block: .{ .logical_size, &[.@"max-height"] } },
        .@"min-inline-size" => LogicalPropertyId{ @"inline": .{ .logical_size, &[.@"min-width"], &[.@"min-height"] } },
        .@"max-inline-size" => LogicalPropertyId{ @"inline": .{ .logical_size, &[.@"max-width"], &[.@"max-height"] } },

        .@"inset-block-start" => LogicalPropertyId{ block: .{ .logical_inset, &[.top] } },
        .@"inset-block-end" => LogicalPropertyId{ block: .{ .logical_inset, &[.bottom] } },
        .@"inset-inline-start" => LogicalPropertyId{ @"inline": .{ .logical_inset, &[.left], &[.right] } },
        .@"inset-inline-end" => LogicalPropertyId{ @"inline": .{ .logical_inset, &[.right], &[.left] } },
        .@"inset-block" => LogicalPropertyId{ block: .{ .logical_inset, &[ .top, .bottom ] } },
        .@"inset-inline" => LogicalPropertyId{ block: .{ .logical_inset, &[ .left, .right ] } },
        .inset => LogicalPropertyId{ block: .{ .logical_inset, &[ .top, .bottom, .left, .right ] } },

        .@"margin-block-start" => LogicalPropertyId{ block: .{ .logical_margin, &[.@"margin-top"] } },
        .@"margin-block-end" => LogicalPropertyId{ block: .{ .logical_margin, &[.@"margin-bottom"] } },
        .@"margin-inline-start" => LogicalPropertyId{ @"inline": .{ .logical_margin, &[.@"margin-left"], &[.@"margin-right"] } },
        .@"margin-inline-end" => LogicalPropertyId{ @"inline": .{ .logical_margin, &[.@"margin-right"], &[.@"margin-left"] } },
        .@"margin-block" => LogicalPropertyId{ block: .{ .logical_margin, &[ .@"margin-top", .@"margin-bottom" ] } },
        .@"margin-inline" => LogicalPropertyId{ block: .{ .logical_margin, &[ .@"margin-left", .@"margin-right" ] } },

        .@"padding-block-start" => LogicalPropertyId{ block: .{ .logical_padding, &[.@"padding-top"] } },
        .@"padding-block-end" => LogicalPropertyId{ block: .{ .logical_padding, &[.@"padding-bottom"] } },
        .@"padding-inline-start" => LogicalPropertyId{ @"inline": .{ .logical_padding, &[.@"padding-left"], &[.@"padding-right"] } },
        .@"padding-inline-end" => LogicalPropertyId{ @"inline": .{ .logical_padding, &[.@"padding-right"], &[.@"padding-left"] } },
        .@"padding-block" => LogicalPropertyId{ block: .{ .logical_padding, &[ .@"padding-top", .@"padding-bottom" ] } },
        .@"padding-inline" => LogicalPropertyId{ block: .{ .logical_padding, &[ .@"padding-left", .@"padding-right" ] } },

        .@"border-block-start" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-top"] } },
        .@"border-block-start-width" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-top-width"] } },
        .@"border-block-start-color" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-top-color"] } },
        .@"border-block-start-style" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-top-style"] } },

        .@"border-block-end" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-bottom"] } },
        .@"border-block-end-width" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-bottom-width"] } },
        .@"border-block-end-color" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-bottom-color"] } },
        .@"border-block-end-style" => LogicalPropertyId{ block: .{ .logical_borders, &[.@"border-bottom-style"] } },

        .@"border-inline-start" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-left"], &[.@"border-right"] } },
        .@"border-inline-start-width" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-left-width"], &[.@"border-right-width"] } },
        .@"border-inline-start-color" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-left-color"], &[.@"border-right-color"] } },
        .@"border-inline-start-style" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-left-style"], &[.@"border-right-style"] } },

        .@"border-inline-end" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-right"], &[.@"border-left"] } },
        .@"border-inline-end-width" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-right-width"], &[.@"border-left-width"] } },
        .@"border-inline-end-color" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-right-color"], &[.@"border-left-color"] } },
        .@"border-inline-end-style" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[.@"border-right-style"], &[.@"border-left-style"] } },

        .@"border-block" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-top", .@"border-bottom" ] } },
        .@"border-block-color" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-top-color", .@"border-bottom-color" ] } },
        .@"border-block-width" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-top-width", .@"border-bottom-width" ] } },
        .@"border-block-style" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-top-style", .@"border-bottom-style" ] } },

        .@"border-inline" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-left", .@"border-right" ] } },
        .@"border-inline-color" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-left-color", .@"border-right-color" ] } },
        .@"border-inline-width" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-left-width", .@"border-right-width" ] } },
        .@"border-inline-style" => LogicalPropertyId{ block: .{ .logical_borders, &[ .@"border-left-style", .@"border-right-style" ] } },

        .@"border-start-start-radius" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[PropertyId{ @"border-top-left-radius": VendorPrefix.NONE }], &[PropertyId{ @"border-top-right-radius": VendorPrefix.NONE }] } },
        .@"border-start-end-radius" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[PropertyId{ @"border-top-right-radius": VendorPrefix.NONE }], &[PropertyId{ @"border-top-left-radius": VendorPrefix.NONE }] } },
        .@"border-end-start-radius" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[PropertyId{ @"border-bottom-left-radius": VendorPrefix.NONE }], &[PropertyId{ @"border-bottom-right-radius": VendorPrefix.NONE }] } },
        .@"border-end-end-radius" => LogicalPropertyId{ @"inline": .{ .logical_borders, &[PropertyId{ @"border-bottom-right-radius": VendorPrefix.NONE }], &[PropertyId{ @"border-bottom-left-radius": VendorPrefix.NONE }] } },

        else => .none,
    };
}

fn isTransitionProperty(property_id: *const PropertyId): bool {
    return switch (property_id.*) {
        .@"transition-property",
        .@"transition-duration",
        .@"transition-delay",
        .@"transition-timing-function",
        .transition => true,
        else => false,
    };
}

import bun from "bun";
const Allocator = std.mem.Allocator;
