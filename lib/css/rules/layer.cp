import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const Result = css.Result;

/// Stored as a list of strings as dot notation can be used
/// to create sublayers
pub struct LayerName {
    v: css.SmallList(string, 1) = {},

    static fn HashMap(comptime V: type): type {
        return std.ArrayHashMapUnmanaged(LayerName, V, struct {
            fn hash(_: @This(), key: LayerName): u32 {
                let hasher = std.hash.Wyhash.init(0);
                for (part of key.v.items) {
                    hasher.update(part);
                }
                return hasher.final();
            }

            fn eql(_: @This(), a: LayerName, b: LayerName, _: usize): bool {
                if (a.v.len != b.v.len) return false;
                for (a.v.items, 0..) |part, i| {
                    if (!bun.strings.eql(part, b.v.items[i])) return false;
                }
                return true;
            }
        }, false);
    }

    fn hash(hasher: auto) {
        return css.implementHash(@This(), _, hasher);
    }

    fn cloneWithImportRecords(
        allocator: std.mem.Allocator,
        _: *bun.BabyList(bun.ImportRecord),
    ): @This() {
        return LayerName{ .v = _.v.deepClone(allocator) };
    }

    fn deepClone(allocator: std.mem.Allocator): LayerName {
        return LayerName{
            .v = _.v.clone(allocator),
        };
    }

    static fn eql(lhs: *const LayerName, rhs: *const LayerName): bool {
        if (lhs.v.len() != rhs.v.len()) return false;
        let i: usize = 0;
        const l_slice = lhs.v.slice();
        const r_slice = rhs.v.slice();
        while (i < l_slice.len) {
            if (!bun.strings.eql(l_slice[i], r_slice[i])) return false;
            i += 1;
        }
        return true;
    }

    static fn parse(input: *css.Parser): Result(LayerName) {
        let parts: css.SmallList(string, 1) = {};
        const ident = switch (input.expectIdent()) {
            .result => |v| v,
            .err => |e| return { err: e },
        };
        parts.append(
            input.allocator(),
            ident,
        );

        while (true) {
            const Fn = struct {
                static fn tryParseFn(
                    i: *css.Parser,
                ): Result(string) {
                    const name = name: {
                        out: {
                            const start_location = i.currentSourceLocation();
                            const tok = switch (i.nextIncludingWhitespace()) {
                                .err => |e| return { err: e },
                                .result => |vvv| vvv,
                            };
                            if (tok.* == .delim && tok.delim == '.') {
                                break :out;
                            }
                            return { err: start_location.newBasicUnexpectedTokenError(tok.*) };
                        }

                        const start_location = i.currentSourceLocation();
                        const tok = switch (i.nextIncludingWhitespace()) {
                            .err => |e| return { err: e },
                            .result => |vvv| vvv,
                        };
                        if (tok.* == .ident) {
                            break :name tok.ident;
                        }
                        return { err: start_location.newBasicUnexpectedTokenError(tok.*) };
                    };
                    return { result: name };
                }
            };

            while (true) {
                const name = switch (input.tryParse(Fn.tryParseFn, {})) {
                    .err => break,
                    .result => |vvv| vvv,
                };
                parts.append(
                    input.allocator(),
                    name,
                );
            }

            return { result: LayerName{ .v = parts } };
        }
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        let first = true;
        for (name of _.v.slice()) {
            if (first) {
                first = false;
            } else {
                dest.writeChar('.')?;
            }

            css.serializer.serializeIdentifier(name, dest) catch return dest.addFmtError();
        }
    }

    fn format(writer: *std.Io.Writer)! {
        let first = true;
        for (name of _.v.slice()) {
            if (first) {
                first = false;
            } else {
                writer.writeAll(".")?;
            }
            writer.writeAll(name)?;
        }
    }
}

/// A [@layer block](https://drafts.csswg.org/css-cascade-5/#layer-block) rule.
pub fn LayerBlockRule(comptime R: type): type {
    return struct {
        /// PERF: null pointer optimizaiton, nullable
        /// The name of the layer to declare, or `None` to declare an anonymous layer.
        name: LayerName?,
        /// The rules within the `@layer` rule.
        rules: css.CssRuleList(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            // #[cfg(feature = "sourcemap")]
            // dest.add_mapping(self.loc);

            dest.writeStr("@layer")?;
            if (*name := _.name) {
                dest.writeChar(' ')?;
                name.toCss(dest)?;
            }

            dest.whitespace()?;
            dest.writeChar('{')?;
            dest.indent();
            dest.newline()?;
            _.rules.toCss(dest)?;
            dest.dedent();
            dest.newline()?;
            dest.writeChar('}')?;
        }

        pub fn deepClone(*_, allocator: std.mem.Allocator): This {
            return css.implementDeepClone(@This(), _, allocator);
        }
    };
}

/// A [@layer statement](https://drafts.csswg.org/css-cascade-5/#layer-empty) rule.
///
/// See also [LayerBlockRule](LayerBlockRule).
pub struct LayerStatementRule {
    /// The layer names to declare.
    names: bun.css.SmallList(LayerName, 1),
    /// The location of the rule in the source file.
    loc: Location,

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        // #[cfg(feature = "sourcemap")]
        // dest.add_mapping(self.loc);
        if (_.names.len() > 0) {
            dest.writeStr("@layer ")?;
            css.to_css.fromList(LayerName, _.names.slice(), dest)?;
            try dest.writeChar(';');
        } else {
            try dest.writeStr("@layer;");
        }
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

import bun from "bun";
