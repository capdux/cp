import css from "../css_parser.zig";

const Printer = css.Printer;
const PrintErr = css.PrintErr;

const CustomIdent = css.css_values.ident.CustomIdent;
const CustomIdentList = css.css_values.ident.CustomIdentList;
const CustomIdentFns = css.css_values.ident.CustomIdentFns;

const Location = css.dependencies.Location;

/// A value for the [composes](https://github.com/css-modules/css-modules/#dependencies) property from CSS modules.
pub struct Composes {
    /// A list of class names to compose.
    names: CustomIdentList,
    /// Where the class names are composed from.
    @"from": Specifier?,
    /// The source location of the `composes` property.
    loc: bun.logger.Loc,
    cssparser_loc: Location,

    static fn parse(input: *css.Parser): css.Result(Composes) {
        const loc = input.position();
        const loc2 = input.currentSourceLocation();
        let names: CustomIdentList = {};
        while (name := input.tryParse(parseOneIdent, {}).asValue()) {
            names.append(input.allocator(), name);
        }

        if (names.len() == 0) return { err: input.newCustomError(css.ParserError{ .invalid_declaration = {} }) };

        let from_val: Specifier? = null;
        if (input.tryParse(css.Parser.expectIdentMatching, .{"from"}).isOk()) {
            const spec_res = Specifier.parse(input);
            if (spec_res == .err) return { err: spec_res.err };
            from_val = spec_res.result;
        }

        return {
            result: Composes{
                names: names,
                @"from": from_val,
                loc: bun.logger.Loc{ .start = @intCast(loc) },
                cssparser_loc: Location.fromSourceLocation(loc2),
            },
        };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        let first = true;
        for (name of _.names.slice()) {
            if (first) {
                first = false;
            } else {
                dest.writeChar(' ')?;
            }
            CustomIdentFns.toCss(&name, dest)?;
        }

        if (*@"from" := this.@"from") {
            dest.writeStr(" from ")?;
            @"from"(.toCss(dest)?);
        }
    }

    static fn parseOneIdent(input: *css.Parser): css.Result(CustomIdent) {
        const name: CustomIdent = switch (CustomIdent.parse(input)) {
            .result => |v| v,
            .err => |e| return { err: e },
        };

        if (bun.strings.eqlCaseInsensitiveASCII(name.v, "from", true)) return { err: input.newErrorForNextToken() };

        return { result: name };
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// Defines where the class names referenced in the `composes` property are located.
///
/// See [Composes](Composes).
fn parseSpecifier(input: *css.Parser): css.Result(Specifier) {
    const start_position = input.position();
    if (file := input.tryParse(css.Parser.expectUrlOrString, {}).asValue()) {
        const idx_res = input.addImportRecord(file, start_position, .composes);
        if (idx_res == .err) return { err: idx_res.err };
        return {
            result: {
                import_record_index: idx_res.result,
            },
        };
    }

    if (input.tryParse(css.Parser.expectIdentMatching, .{"global"}).isOk()) {
        return { result: .global };
    }

    return { err: input.newCustomError(css.ParserError{ .invalid_declaration = {} }) };
}

pub const Specifier = union(enum) {
    /// The referenced name is global.
    global,
    /// The referenced name comes from the specified file.
    ///
    /// Is an import record index
    import_record_index: u32,

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub const parse = parseSpecifier;
    pub const toCss = toCssSpecifier;
    pub const deepClone = deepCloneSpecifier;
    pub const hash = hashSpecifier;
};

fn toCssSpecifier(this_spec: *const Specifier, dest: *Printer): PrintErr!void {
    return switch (this_spec.*) {
        .global => dest.writeStr("global"),
        .import_record_index => |import_record_index| {
            const url = dest.getImportRecordUrl(import_record_index)?;
            css.serializer.serializeString(url, dest) catch return dest.addFmtError();
        },
    };
}

fn deepCloneSpecifier(this_spec: *const Specifier, allocator: std.mem.Allocator): Specifier {
    return css.implementDeepClone(Specifier, this_spec, allocator);
}

fn hashSpecifier(this_spec: *const Specifier, hasher: *std.hash.Wyhash) {
    return css.implementHash(Specifier, this_spec, hasher);
}

import bun from "bun";
const Allocator = std.mem.Allocator;
