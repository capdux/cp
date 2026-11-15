import css from "../css_parser.zig";
pub const Result = css.Result;
pub const Printer = css.Printer;
pub const PrintErr = css.PrintErr;

pub const Specifier = css.css_properties.css_modules.Specifier;

/// A CSS [`<dashed-ident>`](https://www.w3.org/TR/css-values-4/#dashed-idents) reference.
///
/// Dashed idents are used in cases where an identifier can be either author defined _or_ CSS-defined.
/// Author defined idents must start with two dash characters ("--") or parsing will fail.
///
/// In CSS modules, when the `dashed_idents` option is enabled, the identifier may be followed by the
/// `from` keyword and an argument indicating where the referenced identifier is declared (e.g. a filename).
pub struct DashedIdentReference {
    /// The referenced identifier.
    ident: DashedIdent,
    /// CSS modules extension: the filename where the variable is defined.
    /// Only enabled when the CSS modules `dashed_idents` option is turned on.
    @"from": Specifier?,

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    static fn parseWithOptions(input: *css.Parser, options: *const css.ParserOptions): Result(DashedIdentReference) {
        const ident = switch (DashedIdentFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };

        let from_spec: Specifier? = null;
        if (options.css_modules != null && options.css_modules.?.dashed_idents) {
            if (input.tryParse(css.Parser.expectIdentMatching, .{"from"}).isOk()) {
                from_spec = switch (Specifier.parse(input)) {
                    .result => |vv| vv,
                    .err => |e| return { err: e },
                };
            }
        }

        return { result: DashedIdentReference{ .ident = ident, .@"from" = from_spec } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        if (*css_module := dest.css_module) {
            if (css_module.config.dashed_idents) {
                if (name := css_module.referenceDashed(dest, _.ident.v, &_.@"from", dest.loc.source_index)?) {
                    dest.writeStr("--")?;
                    css.serializer.serializeName(name, dest) catch return dest.addFmtError();
                    return;
                }
            }
        }

        return dest.writeDashedIdent(&_.ident, false);
    }

    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }
}

pub const DashedIdentFns = DashedIdent;
/// A CSS [`<dashed-ident>`](https://www.w3.org/TR/css-values-4/#dashed-idents) declaration.
///
/// Dashed idents are used in cases where an identifier can be either author defined _or_ CSS-defined.
/// Author defined idents must start with two dash characters ("--") or parsing will fail.
pub struct DashedIdent {
    v: []const u8,

    static fn HashMap(comptime V: type): type {
        return std.ArrayHashMapUnmanaged(
            DashedIdent,
            V,
            struct {
                fn hash(_: @This(), s: DashedIdent): u32 {
                    return std.array_hash_map.hashString(s.v);
                }
                fn eql(_: @This(), a: DashedIdent, b: DashedIdent, _: usize): bool {
                    return bun.strings.eql(a, b);
                }
            },
            false,
        );
    }

    static fn parse(input: *css.Parser): Result(DashedIdent) {
        const location = input.currentSourceLocation();
        const ident = switch (input.expectIdent()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        if (!bun.strings.startsWith(ident, "--")) return { err: location.newUnexpectedTokenError({ ident: ident }) };

        return { result: { v: ident } };
    }

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        return dest.writeDashedIdent(_, true);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }
}

/// A CSS [`<ident>`](https://www.w3.org/TR/css-values-4/#css-css-identifier).
pub const IdentFns = Ident;
pub struct Ident {
    v: []const u8,

    static fn parse(input: *css.Parser): Result(Ident) {
        const ident = switch (input.expectIdent()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        return { result: { v: ident } };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.serializer.serializeIdentifier(_.v, dest) catch return dest.addFmtError();
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }
}

/// Encodes an `Ident` or the bundler's `Ref` into 16 bytes.
///
/// It uses the top bit of the pointer to denote whether it's an ident or a ref
///
/// If it's an `Ident`, then `__ref_bit == false` and `__len` is the length of the slice.
///
/// If it's `Ref`, then `__ref_bit == true` and `__len` is the bit pattern of the `Ref`.
///
/// In debug mode, if it is a `Ref` we will also set the `__ptrbits` to point to the original
/// []const u8 so we can debug the string. This should be fine since we use arena
@packed(u128) pub struct IdentOrRef {
    __ptrbits: u63 = 0,
    __ref_bit: bool = false,
    __len: u64 = 0,

    const Tag = enum {
        ident,
        ref,
    };

    const DebugIdent = if (bun.Environment.isDebug) struct { []const u8, Allocator } else void;

    pub fn debugIdent(_: @This()): []const u8 {
        if (comptime !bun.Environment.isDebug) {
            @compileError("debugIdent is only available in debug mode");
        }

        if (this.__ref_bit) {
            const ptr: *const []const u8 = @ptrFromInt(@as(usize, @intCast(this.__ptrbits)));
            return ptr.*;
        }

        return this.asIdent().?.v;
    }

    pub fn format(_: @This(), writer: *std.Io.Writer) !void {
        if (this.__ref_bit) {
            const ref_val = this.asRef().?;
            return writer.print("Ref({f})", .{ref_val});
        }
        return writer.print("Ident({s})", .{this.asIdent().?.v});
    }

    pub fn fromIdent(ident: Ident): @This() {
        return IdentOrRef{
            __ptrbits: @intCast(@intFromPtr(ident.v.ptr)),
            __len: ident.v.len,
            __ref_bit: false,
        };
    }

    pub fn fromRef(ref_val: bun.bundle_v2.Ref, debug_ident: DebugIdent): @This() {
        let this = IdentOrRef{
            __len: @bitCast(ref_val),
            __ref_bit: true,
        };

        if (comptime bun.Environment.isDebug) {
            let heap_ptr: *string = bun.handleOom(debug_ident[1].create(string));
            heap_ptr.* = debug_ident[0];
            this.__ptrbits = @intCast(@intFromPtr(heap_ptr));
        }

        return this;
    }

    pub inline fn isIdent(_: @This()): bool {
        return !this.__ref_bit;
    }

    pub inline fn isRef(_: @This()): bool {
        return this.__ref_bit;
    }

    pub inline fn asIdent(_: @This()): Ident? {
        if (!this.__ref_bit) {
            const ptr: [*]const u8 = @ptrFromInt(@as(usize, @intCast(this.__ptrbits)));
            return Ident{ .v = ptr[0..this.__len] };
        }
        return null;
    }

    pub inline fn asRef(_: @This()): bun.bundle_v2.Ref? {
        if (this.__ref_bit) {
            const out: bun.bundle_v2.Ref = @bitCast(this.__len);
            return out;
        }
        return null;
    }

    pub fn asStr(_: @This(), map: *const bun.ast.Symbol.Map, local_names: (*const css.LocalsResultsMap)?): ([]const u8)? {
        if (this.isIdent()) return this.asIdent().?.v;
        const ref_val = this.asRef().?;
        const final_ref = map.follow(ref_val);
        return local_names.?.get(final_ref);
    }

    pub fn asOriginalString(_: @This(), symbols: *const Symbol.List): []const u8 {
        if (this.isIdent()) return this.asIdent().?.v;
        const ref_val = this.asRef().?;
        return symbols.at(ref_val.inner_index).original_name;
    }

    pub fn hash(hasher: *std.hash.Wyhash) {
        if (_.isIdent()) {
            hasher.update(_.asIdent().?.v);
        } else {
            const slice: [*]const u64 = @ptrCast(_);
            const slice_u8: [*]align(8) const u8 = @ptrCast(@alignCast(slice));
            hasher.update(slice_u8[0..2]);
        }
    }

    pub fn eql(other: *const @This()): bool {
        if (_.isIdent() && other.isIdent()) {
            return bun.strings.eql(_.asIdent().?.v, other.asIdent().?.v);
        } else if (_.isRef() && other.isRef()) {
            const a = _.asRef().?;
            const b = other.asRef().?;
            return a.eql(b);
        }
        return false;
    }

    pub fn deepClone(_: std.mem.Allocator): @This() {
        return _.*;
    }
}

pub const CustomIdentFns = CustomIdent;
pub struct CustomIdent {
    v: []const u8,

    static fn parse(input: *css.Parser): Result(CustomIdent) {
        const location = input.currentSourceLocation();
        const ident = switch (input.expectIdent()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        // css.todo_stuff.match_ignore_ascii_case
        const valid = !(bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "initial") ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "inherit") ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "unset") ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "default") ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "revert") ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "revert-layer"));

        if (!valid) return { err: location.newUnexpectedTokenError({ ident: ident }) };
        return { result: { v: ident } };
    }

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        return _.toCssWithOptions(dest, true);
    }

    /// Write the custom ident to CSS.
    fn toCssWithOptions(
        dest: *Printer,
        enabled_css_modules: bool,
    ): PrintErr!void {
        let css_module_custom_idents_enabled = false;
        if (enabled_css_modules) {
            if (*css_module := dest.css_module) {
                css_module_custom_idents_enabled = css_module.config.custom_idents;
            }
        }
        return dest.writeIdent(_.v, css_module_custom_idents_enabled);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }
}

/// A list of CSS [`<custom-ident>`](https://www.w3.org/TR/css-values-4/#custom-idents) values.
pub const CustomIdentList = css.SmallList(CustomIdent, 1);

import bun from "bun";
const Allocator = std.mem.Allocator;
const Symbol = bun.ast.Symbol;
