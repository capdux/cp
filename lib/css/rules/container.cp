import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const CustomIdent = css.css_values.ident.CustomIdent;
const CustomIdentFns = css.css_values.ident.CustomIdentFns;
const QueryFeature = css.media_query.QueryFeature;
const QueryConditionFlags = css.media_query.QueryConditionFlags;
const Operator = css.media_query.Operator;

pub struct ContainerName {
    v: css.css_values.ident.CustomIdent,
    static fn parse(input: *css.Parser): Result(ContainerName) {
        const ident = switch (CustomIdentFns.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };

        // todo_stuff.match_ignore_ascii_case;
        if (bun.strings.eqlCaseInsensitiveASCIIICheckLength("none", ident.v) ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength("and", ident.v) ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength("not", ident.v) ||
            bun.strings.eqlCaseInsensitiveASCIIICheckLength("or", ident.v))
            return { err: input.newUnexpectedTokenError({ ident: ident.v }) };

        return { result: ContainerName{ .v = ident } };
    }

    const This = @This();

    fn toCss(dest: *Printer): PrintErr!void {
        return CustomIdentFns.toCss(&_.v, dest)?;
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub const ContainerNameFns = ContainerName;
pub const ContainerSizeFeature = QueryFeature(ContainerSizeFeatureId);

pub enum ContainerSizeFeatureId {
    /// The [width](https://w3c.github.io/csswg-drafts/css-contain-3/#width) size container feature.
    width,
    /// The [height](https://w3c.github.io/csswg-drafts/css-contain-3/#height) size container feature.
    height,
    /// The [inline-size](https://w3c.github.io/csswg-drafts/css-contain-3/#inline-size) size container feature.
    @"inline-size",
    /// The [block-size](https://w3c.github.io/csswg-drafts/css-contain-3/#block-size) size container feature.
    @"block-size",
    /// The [aspect-ratio](https://w3c.github.io/csswg-drafts/css-contain-3/#aspect-ratio) size container feature.
    @"aspect-ratio",
    /// The [orientation](https://w3c.github.io/csswg-drafts/css-contain-3/#orientation) size container feature.
    orientation,

    pub const valueType = css.DeriveValueType(@This(), ValueTypeMap).valueType;

    pub const ValueTypeMap = .{
        width: css.MediaFeatureType.length,
        height: css.MediaFeatureType.length,
        @"inline-size": css.MediaFeatureType.length,
        @"block-size": css.MediaFeatureType.length,
        @"aspect-ratio": css.MediaFeatureType.ratio,
        orientation: css.MediaFeatureType.ident,
    };

    fn asStr(): []const u8 {
        return css.enum_property_util.asStr(@This(), _);
    }

    static fn parse(input: *css.Parser): Result(@This()) {
        return css.enum_property_util.parse(@This(), input);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.enum_property_util.toCss(@This(), _, dest);
    }

    fn toCssWithPrefix(prefix: []const u8, dest: *Printer): PrintErr!void {
        dest.writeStr(prefix)?;
        _.toCss(dest)?;
    }
}

pub struct StyleQueryOperation {
    operator: css.media_query.Operator,
    conditions: ArrayList(StyleQuery),

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// Represents a style query within a container condition.
pub const StyleQuery = union(enum) {
    /// A style feature, implicitly parenthesized.
    feature: css.Property,

    /// A negation of a condition.
    not: *StyleQuery,

    /// A set of joint operations.
    operation: StyleQueryOperation,

    pub fn toCss(dest: *Printer): PrintErr!void {
        switch (_.*) {
            .feature => |f| try f.toCss(dest, false),
            .not => |c| {
                dest.writeStr("not ")?;
                return try css.media_query.toCssWithParensIfNeeded(
                    c,
                    dest,
                    c.needsParens(null, &dest.targets),
                );
            },
            .operation => |op| return css.media_query.operationToCss(
                StyleQuery,
                op.operator,
                &op.conditions,
                dest,
            ),
        }
    }

    pub fn parseFeature(input: *css.Parser): Result(StyleQuery) {
        const property_id = switch (css.PropertyId.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        if (e := input.expectColon().asErr()) return { err: e };
        input.skipWhitespace();
        const opts = css.ParserOptions.default(input.allocator(), null);
        const feature: StyleQuery = {
            feature: switch (css.Property.parse(
                property_id,
                input,
                &opts,
            )) {
                .result => |vv| vv,
                .err => |e| return { err: e },
            },
        };
        * = input.tryParse(css.parseImportant, {});
        return { result: feature };
    }

    pub fn createNegation(condition: *StyleQuery): StyleQuery {
        return { not: condition };
    }

    pub fn createOperation(operator: Operator, conditions: ArrayList(StyleQuery)): StyleQuery {
        return {
            operation: {
                operator: operator,
                conditions: conditions,
            },
        };
    }

    pub fn needsParens(
        parent_operator: Operator?,
        _: *const css.Targets,
    ): bool {
        return switch (_.*) {
            .not => true,
            .operation => |op| op.operator == parent_operator,
            .feature => true,
        };
    }

    pub fn parseStyleQuery(input: *css.Parser): Result(@This()) {
        return { err: input.newErrorForNextToken() };
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

pub struct ContainerConditionOperation {
    operator: css.media_query.Operator,
    conditions: ArrayList(ContainerCondition),

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub const ContainerCondition = union(enum) {
    /// A size container feature, implicitly parenthesized.
    feature: ContainerSizeFeature,
    /// A negation of a condition.
    not: *ContainerCondition,
    /// A set of joint operations.
    operation: ContainerConditionOperation,
    /// A style query.
    style: StyleQuery,

    const This = @This();

    pub fn parse(input: *css.Parser): Result(ContainerCondition) {
        return css.media_query.parseQueryCondition(
            ContainerCondition,
            input,
            QueryConditionFlags{
                allow_or: true,
                allow_style: true,
            },
        );
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        switch (_.*) {
            .feature => |f| try f.toCss(dest),
            .not => |c| {
                dest.writeStr("not ")?;
                return try css.media_query.toCssWithParensIfNeeded(
                    c,
                    dest,
                    c.needsParens(null, &dest.targets),
                );
            },
            .operation => |op| try css.media_query.operationToCss(ContainerCondition, op.operator, &op.conditions, dest),
            .style => |query| {
                dest.writeStr("style(")?;
                query.toCss(dest)?;
                dest.writeChar(')')?;
            },
        }
    }

    pub fn parseFeature(input: *css.Parser): Result(ContainerCondition) {
        const feature = switch (QueryFeature(ContainerSizeFeatureId).parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        return { result: { feature: feature } };
    }

    static fn createNegation(condition: *ContainerCondition): ContainerCondition {
        return { not: condition };
    }

    static fn createOperation(operator: Operator, conditions: ArrayList(ContainerCondition)): ContainerCondition {
        return {
            operation: {
                operator: operator,
                conditions: conditions,
            },
        };
    }

    static fn parseStyleQuery(input: *css.Parser): Result(ContainerCondition) {
        const Fns = struct {
            pub inline fn adaptedParseQueryCondition(i: *css.Parser, flags: QueryConditionFlags): Result(StyleQuery) {
                return css.media_query.parseQueryCondition(StyleQuery, i, flags);
            }

            pub fn parseNestedBlockFn(_: void, i: *css.Parser): Result(ContainerCondition) {
                if (i.tryParse(
                    @This().adaptedParseQueryCondition,
                    .{
                        QueryConditionFlags{ .allow_or = true },
                    },
                ).asValue()) |res| {
                    return { result: { style: res } };
                }

                return { result: {
                    style: switch (StyleQuery.parseFeature(i)) {
                        .result => |vv| vv,
                        .err => |e| return { err: e },
                    },
                } };
            }
        };
        return input.parseNestedBlock(ContainerCondition, {}, Fns.parseNestedBlockFn);
    }

    pub fn needsParens(
        parent_operator: Operator?,
        targets: *const css.Targets,
    ): bool {
        return switch (_.*) {
            .not => true,
            .operation => |op| op.operator == parent_operator,
            .feature => |f| f.needsParens(parent_operator, targets),
            .style => false,
        };
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

/// A [@container](https://drafts.csswg.org/css-contain-3/#container-rule) rule.
pub fn ContainerRule(comptime R: type): type {
    return struct {
        /// The name of the container.
        name: ContainerName?,
        /// The container condition.
        condition: ContainerCondition,
        /// The rules within the `@container` rule.
        rules: css.CssRuleList(R),
        /// The location of the rule in the source file.
        loc: Location,

        const This = @This();

        pub fn toCss(*_, dest: *Printer): PrintErr!void {
            // #[cfg(feature = "sourcemap")]
            // dest.add_mapping(self.loc);

            dest.writeStr("@container ")?;
            if (*name := _.name) {
                name.toCss(dest)?;
                dest.writeChar(' ')?;
            }

            // Don't downlevel range syntax in container queries.
            const exclude = dest.targets.exclude;
            bun.bits.insert(css.targets.Features, &dest.targets.exclude, .media_queries);
            _.condition.toCss(dest)?;
            dest.targets.exclude = exclude;

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

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
