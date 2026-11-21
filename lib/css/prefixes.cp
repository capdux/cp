// Source: Absorbed from /data/dev/bun/src/css/prefixes.cp | Synced: 2026-11-20T13:41:13Z
// lib/css/prefixes.cp
// CSS Vendor Prefixing & Logical-to-Physical Property Expansion
// Absorbed from Bun src/css/prefixes.cp & src/css/logical.cp

const std = @import("std");

pub struct CssVendorPrefix {
    bits: u8 = 1;

    pub static fn none(): CssVendorPrefix {
        return CssVendorPrefix { bits: 1 };
    }

    pub static fn webkit(): CssVendorPrefix {
        return CssVendorPrefix { bits: 2 };
    }

    pub static fn moz(): CssVendorPrefix {
        return CssVendorPrefix { bits: 4 };
    }

    pub static fn ms(): CssVendorPrefix {
        return CssVendorPrefix { bits: 8 };
    }

    pub static fn o(): CssVendorPrefix {
        return CssVendorPrefix { bits: 16 };
    }

    pub static fn all(): CssVendorPrefix {
        return CssVendorPrefix { bits: 31 };
    }

    pub static fn fromBits(b: u8): CssVendorPrefix {
        return CssVendorPrefix { bits: b };
    }

    pub fn hasNone(): bool { return (_.bits & 1) != 0; }
    pub fn hasWebkit(): bool { return (_.bits & 2) != 0; }
    pub fn hasMoz(): bool { return (_.bits & 4) != 0; }
    pub fn hasMs(): bool { return (_.bits & 8) != 0; }
    pub fn hasO(): bool { return (_.bits & 16) != 0; }

    pub fn unionWith(other: CssVendorPrefix): CssVendorPrefix {
        return CssVendorPrefix { bits: _.bits | other.bits };
    }
}

pub struct CssExpandedDecl {
    name: []const u8 = "";
    value: []const u8 = "";
    important: bool = false;
}

pub struct CssPrefixer {
    /// Returns the common vendor prefixes historically required for a CSS property.
    pub static fn prefixesForProperty(prop_name: []const u8): CssVendorPrefix {
        if (std.ascii.eqlIgnoreCase(prop_name, "user-select")) {
            return CssVendorPrefix.fromBits(1 | 2 | 4 | 8); // standard + webkit + moz + ms
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "backdrop-filter") ||
            std.ascii.eqlIgnoreCase(prop_name, "mask-image") ||
            std.ascii.eqlIgnoreCase(prop_name, "mask-clip") ||
            std.ascii.eqlIgnoreCase(prop_name, "mask-composite") ||
            std.ascii.eqlIgnoreCase(prop_name, "text-size-adjust") ||
            std.ascii.eqlIgnoreCase(prop_name, "line-clamp") ||
            std.ascii.eqlIgnoreCase(prop_name, "clip-path"))
        {
            return CssVendorPrefix.fromBits(1 | 2); // standard + webkit
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "appearance")) {
            return CssVendorPrefix.fromBits(1 | 2 | 4); // standard + webkit + moz
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "hyphens")) {
            return CssVendorPrefix.fromBits(1 | 2 | 8); // standard + webkit + ms
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "transform") ||
            std.ascii.eqlIgnoreCase(prop_name, "transform-origin") ||
            std.ascii.eqlIgnoreCase(prop_name, "transition") ||
            std.ascii.eqlIgnoreCase(prop_name, "animation") ||
            std.ascii.eqlIgnoreCase(prop_name, "box-sizing"))
        {
            return CssVendorPrefix.fromBits(1 | 2 | 4);
        }
        return CssVendorPrefix.none();
    }

    /// Expands a CSS property into vendor-prefixed declarations followed by the unprefixed standard property.
    /// `scratch_buf` stores the generated `-webkit-*` / `-moz-*` / `-ms-*` / `-o-*` property name slices.
    pub static fn expand(
        prop_name: []const u8,
        value: []const u8,
        important: bool,
        prefixes: CssVendorPrefix,
        out_decls: []CssExpandedDecl,
        scratch_buf: []u8,
    ): usize {
        let count: usize = 0;
        let scratch_pos: usize = 0;

        if (prefixes.hasWebkit() && count < out_decls.len) {
            const s = std.fmt.bufPrint(scratch_buf[scratch_pos..], "-webkit-{s}", .{prop_name}) catch "";
            if (s.len > 0) {
                scratch_pos += s.len;
                out_decls[count] = CssExpandedDecl { name: s, value: value, important: important };
                count += 1;
            }
        }
        if (prefixes.hasMoz() && count < out_decls.len) {
            const s = std.fmt.bufPrint(scratch_buf[scratch_pos..], "-moz-{s}", .{prop_name}) catch "";
            if (s.len > 0) {
                scratch_pos += s.len;
                out_decls[count] = CssExpandedDecl { name: s, value: value, important: important };
                count += 1;
            }
        }
        if (prefixes.hasMs() && count < out_decls.len) {
            const s = std.fmt.bufPrint(scratch_buf[scratch_pos..], "-ms-{s}", .{prop_name}) catch "";
            if (s.len > 0) {
                scratch_pos += s.len;
                out_decls[count] = CssExpandedDecl { name: s, value: value, important: important };
                count += 1;
            }
        }
        if (prefixes.hasO() && count < out_decls.len) {
            const s = std.fmt.bufPrint(scratch_buf[scratch_pos..], "-o-{s}", .{prop_name}) catch "";
            if (s.len > 0) {
                scratch_pos += s.len;
                out_decls[count] = CssExpandedDecl { name: s, value: value, important: important };
                count += 1;
            }
        }
        if (prefixes.hasNone() && count < out_decls.len) {
            out_decls[count] = CssExpandedDecl { name: prop_name, value: value, important: important };
            count += 1;
        }
        return count;
    }
}

pub struct CssLogical {
    pub static fn isLogicalProperty(prop_name: []const u8): bool {
        return std.ascii.eqlIgnoreCase(prop_name, "inline-size") ||
            std.ascii.eqlIgnoreCase(prop_name, "block-size") ||
            std.ascii.eqlIgnoreCase(prop_name, "min-inline-size") ||
            std.ascii.eqlIgnoreCase(prop_name, "max-inline-size") ||
            std.ascii.eqlIgnoreCase(prop_name, "min-block-size") ||
            std.ascii.eqlIgnoreCase(prop_name, "max-block-size") ||
            std.ascii.eqlIgnoreCase(prop_name, "margin-inline") ||
            std.ascii.eqlIgnoreCase(prop_name, "margin-inline-start") ||
            std.ascii.eqlIgnoreCase(prop_name, "margin-inline-end") ||
            std.ascii.eqlIgnoreCase(prop_name, "margin-block") ||
            std.ascii.eqlIgnoreCase(prop_name, "margin-block-start") ||
            std.ascii.eqlIgnoreCase(prop_name, "margin-block-end") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding-inline") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding-inline-start") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding-inline-end") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding-block") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding-block-start") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding-block-end") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset-inline") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset-inline-start") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset-inline-end") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset-block") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset-block-start") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset-block-end");
    }

    /// Expands a CSS logical property (in horizontal-tb LTR mode) into 1 or 2 physical CSS properties.
    pub static fn expand(
        prop_name: []const u8,
        value: []const u8,
        important: bool,
        out_decls: []CssExpandedDecl,
    ): usize {
        if (out_decls.len == 0) return 0;

        // Single-property direct mappings
        if (std.ascii.eqlIgnoreCase(prop_name, "inline-size")) {
            out_decls[0] = CssExpandedDecl { name: "width", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "block-size")) {
            out_decls[0] = CssExpandedDecl { name: "height", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "min-inline-size")) {
            out_decls[0] = CssExpandedDecl { name: "min-width", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "max-inline-size")) {
            out_decls[0] = CssExpandedDecl { name: "max-width", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "min-block-size")) {
            out_decls[0] = CssExpandedDecl { name: "min-height", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "max-block-size")) {
            out_decls[0] = CssExpandedDecl { name: "max-height", value: value, important: important };
            return 1;
        }

        if (std.ascii.eqlIgnoreCase(prop_name, "margin-inline-start")) {
            out_decls[0] = CssExpandedDecl { name: "margin-left", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "margin-inline-end")) {
            out_decls[0] = CssExpandedDecl { name: "margin-right", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "margin-block-start")) {
            out_decls[0] = CssExpandedDecl { name: "margin-top", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "margin-block-end")) {
            out_decls[0] = CssExpandedDecl { name: "margin-bottom", value: value, important: important };
            return 1;
        }

        if (std.ascii.eqlIgnoreCase(prop_name, "padding-inline-start")) {
            out_decls[0] = CssExpandedDecl { name: "padding-left", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "padding-inline-end")) {
            out_decls[0] = CssExpandedDecl { name: "padding-right", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "padding-block-start")) {
            out_decls[0] = CssExpandedDecl { name: "padding-top", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "padding-block-end")) {
            out_decls[0] = CssExpandedDecl { name: "padding-bottom", value: value, important: important };
            return 1;
        }

        if (std.ascii.eqlIgnoreCase(prop_name, "inset-inline-start")) {
            out_decls[0] = CssExpandedDecl { name: "left", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "inset-inline-end")) {
            out_decls[0] = CssExpandedDecl { name: "right", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "inset-block-start")) {
            out_decls[0] = CssExpandedDecl { name: "top", value: value, important: important };
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "inset-block-end")) {
            out_decls[0] = CssExpandedDecl { name: "bottom", value: value, important: important };
            return 1;
        }

        // Two-value shorthand logical properties (e.g. `margin-inline: 1rem 2rem` or `padding-block: 8px`)
        let v1 = std.mem.trim(u8, value, " \t\r\n");
        let v2 = v1;
        let it = std.mem.tokenizeAny(u8, v1, " \t\r\n");
        if (first := it.next()) {
            v1 = first;
            if (second := it.next()) {
                v2 = second;
            } else {
                v2 = first;
            }
        }

        if (std.ascii.eqlIgnoreCase(prop_name, "margin-inline")) {
            out_decls[0] = CssExpandedDecl { name: "margin-left", value: v1, important: important };
            if (out_decls.len > 1) {
                out_decls[1] = CssExpandedDecl { name: "margin-right", value: v2, important: important };
                return 2;
            }
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "margin-block")) {
            out_decls[0] = CssExpandedDecl { name: "margin-top", value: v1, important: important };
            if (out_decls.len > 1) {
                out_decls[1] = CssExpandedDecl { name: "margin-bottom", value: v2, important: important };
                return 2;
            }
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "padding-inline")) {
            out_decls[0] = CssExpandedDecl { name: "padding-left", value: v1, important: important };
            if (out_decls.len > 1) {
                out_decls[1] = CssExpandedDecl { name: "padding-right", value: v2, important: important };
                return 2;
            }
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "padding-block")) {
            out_decls[0] = CssExpandedDecl { name: "padding-top", value: v1, important: important };
            if (out_decls.len > 1) {
                out_decls[1] = CssExpandedDecl { name: "padding-bottom", value: v2, important: important };
                return 2;
            }
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "inset-inline")) {
            out_decls[0] = CssExpandedDecl { name: "left", value: v1, important: important };
            if (out_decls.len > 1) {
                out_decls[1] = CssExpandedDecl { name: "right", value: v2, important: important };
                return 2;
            }
            return 1;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "inset-block")) {
            out_decls[0] = CssExpandedDecl { name: "top", value: v1, important: important };
            if (out_decls.len > 1) {
                out_decls[1] = CssExpandedDecl { name: "bottom", value: v2, important: important };
                return 2;
            }
            return 1;
        }

        out_decls[0] = CssExpandedDecl { name: prop_name, value: value, important: important };
        return 1;
    }
}
