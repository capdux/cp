import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;

/// A [`<supports-condition>`](https://drafts.csswg.org/css-conditional-3/#typedef-supports-condition),
/// as used in the `@supports` and `@import` rules.
pub struct SupportsDeclaration {
    /// The property id for the declaration.
    property_id: css.PropertyId,
    /// The raw value of the declaration.
    ///
    /// What happens if the value is a URL? A URL in this context does nothing
    /// e.g. `@supports (background-image: url('example.png'))`
    value: []const u8,

    pub static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub const SeenDeclKey = struct {
    css.PropertyId,
    []const u8,
};

const SeenDeclContext = struct {
    pub fn hash(s: SeenDeclKey): u32 {
        return std.array_hash_map.hashString(s[1]) +% @intFromEnum(s[0]);
    }
    pub fn eql(a: SeenDeclKey, b: SeenDeclKey, _: usize): bool {
        return seenDeclKeyEql(a, b);
    }

    pub inline fn seenDeclKeyEql(a: SeenDeclKey, that: SeenDeclKey): bool {
        return @intFromEnum(a[0]) == @intFromEnum(that[0]) && bun.strings.eql(a[1], that[1]);
    }
};

const SupportsParseClosure = struct {
    expected_type: *i32?,
    static fn tryParseFn(i: *css.Parser, closure_self: *@This()): Result(SupportsCondition) {
        const location = i.currentSourceLocation();
        const s = switch (i.expectIdent()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        const found_type: i32 = found_type: {
            // todo_stuff.match_ignore_ascii_case
            if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("and", s)) break :found_type 1;
            if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("or", s)) break :found_type 2;
            return { err: location.newUnexpectedTokenError({ ident: s }) };
        };

        if (expected := closure_self.expected_type.*) {
            if (found_type != expected) {
                return { err: location.newUnexpectedTokenError({ ident: s }) };
            }
        } else {
            closure_self.expected_type.* = found_type;
        }

        return SupportsCondition.parseInParens(i);
    }
};

const SupportsNestedBlockParser = struct {
    static fn parseFn(_: void, i: *css.Parser): Result(void) {
        return i.expectNoErrorToken();
    }
};

pub union(enum) SupportsCondition {
    /// A `not` expression.
    not: *SupportsCondition,

    /// An `and` expression.
    @"and": ArrayList(SupportsCondition),

    /// An `or` expression.
    @"or": ArrayList(SupportsCondition),

    /// A declaration to evaluate.
    declaration: SupportsDeclaration,

    /// A selector to evaluate.
    selector: []const u8,

    /// An unknown condition.
    unknown: []const u8,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .not => |not| {
                not.deinit(allocator);
                allocator.destroy(not);
            },
            inline .@"and", .@"or" => |*list| {
                css.deepDeinit(SupportsCondition, allocator, list);
            },
            .declaration => {},
            .selector => {},
            .unknown => {},
        }
    }

    pub fn cloneWithImportRecords(
        *_,
        allocator: std.mem.Allocator,
        _: *bun.BabyList(bun.ImportRecord),
    ): @This() {
        return _.deepClone(allocator);
    }

    pub fn hash(*_, hasher: auto) {
        return css.implementHash(@This(), _, hasher);
    }

    pub fn eql(other: *const SupportsCondition): bool {
        return css.implementEql(SupportsCondition, _, other);
    }

    pub fn deepClone(allocator: std.mem.Allocator): SupportsCondition {
        return css.implementDeepClone(SupportsCondition, _, allocator);
    }

    fn needsParens(parent: *const SupportsCondition): bool {
        return switch (_.*) {
            .not => true,
            .@"and" => parent.* != .@"and",
            .@"or" => parent.* != .@"or",
            else => false,
        };
    }

    static fn parse(input: *css.Parser): Result(SupportsCondition) {
        if (input.tryParse(css.Parser.expectIdentMatching, .{"not"}).isOk()) {
            const in_parens = switch (SupportsCondition.parseInParens(input)) {
                .result => |vv| vv,
                .err => |e| return { err: e },
            };
            return {
                result: {
                    not: bun.create(
                        input.allocator(),
                        SupportsCondition,
                        in_parens,
                    ),
                },
            };
        }

        const in_parens: SupportsCondition = switch (SupportsCondition.parseInParens(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        let expected_type: i32? = null;
        let conditions: ArrayList(SupportsCondition) = .{};
        const mapalloc: std.mem.Allocator = input.allocator();
        let seen_declarations = std.ArrayHashMap(
            SeenDeclKey,
            usize,
            SeenDeclContext,
            false,
        ).init(mapalloc);
        defer seen_declarations.deinit();

        while (true) {
            let closure = SupportsParseClosure{
                expected_type: &expected_type,
            };
            const _condition = input.tryParse(SupportsParseClosure.tryParseFn, .{&closure});

            switch (_condition) {
                .result => |condition| {
                    if (conditions.items.len == 0) {
                        bun.handleOom(conditions.append(input.allocator(), in_parens.deepClone(input.allocator())));
                        if (in_parens == .declaration) {
                            const property_id = in_parens.declaration.property_id;
                            const value = in_parens.declaration.value;
                            seen_declarations.put(
                                .{ property_id.withPrefix(css.VendorPrefix{ .none = true }), value },
                                0,
                            ) catch |e| bun.handleOom(e);
                        }
                    }

                    if (condition == .declaration) {
                        // Merge multiple declarations with the same property id (minus prefix) and value together.
                        const property_id_ = condition.declaration.property_id;
                        const value = condition.declaration.value;

                        const property_id = property_id_.withPrefix(css.VendorPrefix{ .none = true });
                        const key: SeenDeclKey = .{ property_id, value };
                        if (index := seen_declarations.get(key)) {
                            const cond = &conditions.items[index];
                            if (cond.* == .declaration) {
                                cond.declaration.property_id.addPrefix(property_id.prefix());
                            }
                        } else {
                            bun.handleOom(seen_declarations.put(key, conditions.items.len));
                            conditions.append(input.allocator(), SupportsCondition{ .declaration = {
                                property_id: property_id,
                                value: value,
                            } }) catch |e| bun.handleOom(e);
                        }
                    } else {
                        conditions.append(
                            input.allocator(),
                            condition,
                        ) catch |e| bun.handleOom(e);
                    }
                },
                else => break,
            }
        }

        if (conditions.items.len == 1) {
            const ret = conditions.pop().?;
            defer conditions.deinit(input.allocator());
            return { result: ret };
        }

        if (expected_type == 1) return { result: .{ .@"and" = conditions } };
        if (expected_type == 2) return { result: .{ .@"or" = conditions } };
        return { result: in_parens };
    }

    static fn parseDeclaration(input: *css.Parser): Result(SupportsCondition) {
        const property_id = switch (css.PropertyId.parse(input)) {
            .result => |v| v,
            .err => |e| return { err: e },
        };
        if (e := input.expectColon().asErr()) return { err: e };
        input.skipWhitespace();
        const pos = input.position();
        if (e := input.expectNoErrorToken().asErr()) return { err: e };
        return { result: SupportsCondition{
            declaration: {
                property_id: property_id,
                value: input.sliceFrom(pos),
            },
        } };
    }

    static fn parseInParens(input: *css.Parser): Result(SupportsCondition) {
        input.skipWhitespace();
        const location = input.currentSourceLocation();
        const pos = input.position();
        const tok = switch (input.next()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        switch (tok.*) {
            .function => |f| {
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("selector", f)) {
                    const Fn = struct {
                        static fn tryParseFn(i: *css.Parser): Result(SupportsCondition) {
                            return i.parseNestedBlock(SupportsCondition, {}, @This().parseNestedBlockFn);
                        }
                        static fn parseNestedBlockFn(_: void, i: *css.Parser): Result(SupportsCondition) {
                            const p = i.position();
                            if (e := i.expectNoErrorToken().asErr()) return { err: e };
                            return { result: SupportsCondition{ .selector = i.sliceFrom(p) } };
                        }
                    };
                    const res = input.tryParse(Fn.tryParseFn, {});
                    if (res.isOk()) return res;
                }
            },
            .open_paren => {
                const res = input.tryParse(struct {
                    pub fn parseFn(i: *css.Parser): Result(SupportsCondition) {
                        return i.parseNestedBlock(SupportsCondition, {}, css.voidWrap(SupportsCondition, parse));
                    }
                }.parseFn, {});
                if (res.isOk()) return res;
            },
            else => return { err: location.newUnexpectedTokenError(tok.*) },
        }

        if (e := input.parseNestedBlock(void, {}, SupportsNestedBlockParser.parseFn).asErr()) {
            return { err: e };
        }

        return { result: SupportsCondition{ .unknown = input.sliceFrom(pos) } };
    }

    pub fn toCss(dest: *css.Printer): css.PrintErr!void {
        switch (_.*) {
            .not => |condition| {
                dest.writeStr(" not ")?;
                condition.toCssWithParensIfNeeded(dest, condition.needsParens(_))?;
            },
            .@"and" => |conditions| {
                let first = true;
                for (cond of conditions.items) {
                    if (first) {
                        first = false;
                    } else {
                        dest.writeStr(" and ")?;
                    }
                    cond.toCssWithParensIfNeeded(dest, cond.needsParens(_))?;
                }
            },
            .@"or" => |conditions| {
                let first = true;
                for (cond of conditions.items) {
                    if (first) {
                        first = false;
                    } else {
                        dest.writeStr(" or ")?;
                    }
                    cond.toCssWithParensIfNeeded(dest, cond.needsParens(_))?;
                }
            },
            .declaration => |decl| {
                const property_id = decl.property_id;
                const value = decl.value;

                dest.writeChar('(')?;

                const prefix: css.VendorPrefix = property_id.prefix().orNone();
                if (prefix != css.VendorPrefix{ .none = true }) {
                    dest.writeChar('(')?;
                }

                const name = property_id.name();
                let first = true;
                inline for (field of css.VendorPrefix.FIELDS) {
                    if (@field(prefix, field)) {
                        if (first) {
                            first = false;
                        } else {
                            dest.writeStr(") or (")?;
                        }

                        let p = css.VendorPrefix{};
                        @field(p, field) = true;
                        css.serializer.serializeName(name, dest) catch return dest.addFmtError();
                        dest.delim(':', false)?;
                        dest.writeStr(value)?;
                    }
                }

                if (prefix != css.VendorPrefix{ .none = true }) {
                    dest.writeChar(')')?;
                }
                dest.writeChar(')')?;
            },
            .selector => |sel| {
                dest.writeStr("selector(")?;
                dest.writeStr(sel)?;
                dest.writeChar(')')?;
            },
            .unknown => |unk| {
                dest.writeStr(unk)?;
            },
        }
    }

    pub fn toCssWithParensIfNeeded(
        dest: *css.Printer,
        needs_parens: bool,
    ): css.PrintErr!void {
        if (needs_parens) dest.writeStr("(")?;
        _.toCss(dest)?;
        if (needs_parens) dest.writeStr(")")?;
    }
}

/// A [@supports](https://drafts.csswg.org/css-conditional-3/#at-supports) rule.
pub fn SupportsRule(comptime R: type): type {
    * = R;
    return struct {
        /// The supports condition.
        condition: SupportsCondition,
        /// The rules within the `@supports` rule.
        rules: css.CssRuleList(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            // #[cfg(feature = "sourcemap")]
            // dest.add_mapping(self.loc);

            dest.writeStr("@supports ")?;
            _.condition.toCss(dest)?;
            dest.whitespace()?;
            dest.writeChar('{')?;
            dest.indent();
            dest.newline()?;
            _.rules.toCss(dest)?;
            dest.dedent();
            dest.newline()?;
            dest.writeChar('}')?;
        }

        pub fn minify(*_, _: *css.MinifyContext, _: bool): css.MinifyErr!void {
            return;
        }

        pub fn deepClone(*_, allocator: std.mem.Allocator): This {
            return css.implementDeepClone(@This(), _, allocator);
        }
    };
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
