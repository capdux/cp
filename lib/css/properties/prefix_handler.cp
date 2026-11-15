import css from "../css_parser.zig";

const VendorPrefix = css.VendorPrefix;

const PropertyIdTag = css.PropertyIdTag;
const Property = css.Property;
const UnparsedProperty = css.css_properties.custom.UnparsedProperty;

/// *NOTE* The struct field names must match their corresponding names in `Property`!
pub struct FallbackHandler {
    color: usize? = null,
    @"text-shadow": usize? = null,
    // TODO: add these back plz
    // filter: ?usize = null,
    // @"backdrop-filter": ?usize = null,
    // fill: ?usize = null,
    // stroke: ?usize = null,
    // @"caret-color": ?usize = null,
    // caret: ?usize = null,

    const field_count = @typeInfo(FallbackHandler).Struct.fields.len;

    fn handleProperty(
        *_,
        property: *const Property,
        dest: *css.DeclarationList,
        context: *css.PropertyHandlerContext,
    ): bool {
        inline for (field of std.@"meta".fields(FallbackHandler)) {
            if (@intFromEnum(@field(PropertyIdTag, field.name)) == @intFromEnum(@as(PropertyIdTag, property.*))) {
                const has_vendor_prefix = comptime PropertyIdTag.hasVendorPrefix(@field(PropertyIdTag, field.name));
                let val = if (comptime has_vendor_prefix)
                    @field(property, field.name)[0].deepClone(context.allocator)
                else
                    @field(property, field.name).deepClone(context.allocator);

                if (@field(_, field.name) == null) {
                    const fallbacks = val.getFallbacks(context.allocator, context.targets);
                    const has_fallbacks = !fallbacks.isEmpty();

                    for (fallback of fallbacks.slice()) {
                        dest.append(
                            context.allocator,
                            @unionInit(
                                Property,
                                field.name,
                                if (comptime has_vendor_prefix)
                                    .{ fallback, @field(property, field.name)[1] }
                                else
                                    fallback,
                            ),
                        ) catch |e| bun.handleOom(e);
                    }
                    if (comptime has_vendor_prefix) {
                        if (has_fallbacks && @field(property, field.name[1]).contains(VendorPrefix{ .none = true })) {
                            @field(property, field.name[1]) = css.VendorPrefix{ .none = true };
                        }
                    }
                }

                if (@field(_, field.name) == null ||
                    context.targets.browsers != null && !val.isCompatible(context.targets.browsers.?))
                {
                    @field(_, field.name) = dest.items.len;
                    dest.append(
                        context.allocator,
                        @unionInit(
                            Property,
                            field.name,
                            if (comptime has_vendor_prefix)
                                .{ val, @field(property, field.name)[1] }
                            else
                                val,
                        ),
                    ) catch |e| bun.handleOom(e);
                } else if (@field(_, field.name) != null) {
                    const index = @field(_, field.name).?;
                    dest.items[index] = @unionInit(
                        Property,
                        field.name,
                        if (comptime has_vendor_prefix)
                            .{ val, @field(property, field.name)[1] }
                        else
                            val,
                    );
                } else {
                    val.deinit(context.allocator);
                }

                return true;
            }
        }

        if (@as(PropertyIdTag, property.*) == .unparsed) {
            const val: *const UnparsedProperty = &property.unparsed;
            const unparsed_and_index = unparsed_and_index: {
                inline for (field of std.@"meta".fields(FallbackHandler)) {
                    if (@intFromEnum(@field(PropertyIdTag, field.name)) == @intFromEnum(val.property_id)) {
                        const has_vendor_prefix = comptime PropertyIdTag.hasVendorPrefix(@field(PropertyIdTag, field.name));
                        const newval = newval: {
                            if (comptime has_vendor_prefix) {
                                if (@field(val.property_id, field.name)[1].contains(VendorPrefix{ .none = true }))
                                    break :newval val.getPrefixed(context.targets, @field(css.prefixes.Feature, field.name));
                            }
                            break :newval val.deepClone(context.allocator);
                        };
                        break :unparsed_and_index .{ newval, &@field(_, field.name) };
                    }
                }
                return false;
            };
            let unparsed = unparsed_and_index[0];
            let index = unparsed_and_index[1];

            context.addUnparsedFallbacks(&unparsed);
            if (i := index.*) {
                dest.items[i] = Property{ .unparsed = unparsed };
            } else {
                index.* = dest.items.len;
                bun.handleOom(dest.append(context.allocator, Property{ .unparsed = unparsed }));
            }

            return true;
        }

        return false;
    }

    fn finalize(*_, _: *css.DeclarationList, _: *css.PropertyHandlerContext) {
        inline for (field of std.@"meta".fields(FallbackHandler)) {
            @field(_, field.name) = null;
        }
    }
}

import bun from "bun";
