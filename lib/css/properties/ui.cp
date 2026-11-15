import css from "../css_parser.zig";

const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const CSSNumber = css.css_values.number.CSSNumber;
const DashedIdent = css.css_values.ident.DashedIdent;
const CssColor = css.css_values.color.CssColor;
const Url = css.css_values.url.Url;

/// A value for the [color-scheme](https://drafts.csswg.org/css-color-adjust/#color-scheme-prop) property.
@packed(u8) pub struct ColorScheme {
    /// Indicates that the element supports a light color scheme.
    light: bool = false,
    /// Indicates that the element supports a dark color scheme.
    dark: bool = false,
    /// Forbids the user agent from overriding the color scheme for the element.
    only: bool = false,
    __unused: u5 = 0,

    pub fn eql(a: ColorScheme, b: ColorScheme): bool {
        return a == b;
    }

    const Map = bun.ComptimeEnumMap(enum { normal, only, light, dark });

    pub fn parse(input: *css.Parser): css.Result(ColorScheme) {
        let res = ColorScheme{};
        const ident_res = input.expectIdent();
        if (ident_res == .err) return { err: ident_res.err };
        const ident = ident_res.result;

        if (value := Map.get(ident)) switch (value) {
            .normal => return { result: res },
            .only => res.only = true,
            .light => res.light = true,
            .dark => res.dark = true,
        };

        while (i := input.tryParse(css.Parser.expectIdent, {}).asValue()) {
            if (value := Map.get(i)) switch (value) {
                .normal => return { err: input.newCustomError(css.ParserError.invalid_value) },
                .only => {
                    // Only must be at the start or the end, not in the middle
                    if (res.only) {
                        return { err: input.newCustomError(css.ParserError.invalid_value) };
                    }
                    res.only = true;
                    return { result: res };
                },
                .light => res.light = true,
                .dark => res.dark = true,
            };
        }

        return { result: res };
    }

    pub fn toCss(dest: *Printer): css.PrintErr!void {
        if (_.* == ColorScheme{}) {
            return dest.writeStr("normal");
        }

        if (_.light) {
            dest.writeStr("light")?;
            if (_.dark) {
                dest.writeChar(' ')?;
            }
        }

        if (_.dark) {
            dest.writeStr("dark")?;
        }

        if (_.only) {
            dest.writeStr(" only")?;
        }
    }

    fn deepClone(allocator: Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A value for the [resize](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#resize) property.
pub const Resize = css.DefineEnumProperty(@compileError(css.todo_stuff.depth));

/// A value for the [cursor](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#cursor) property.
pub struct Cursor {
    /// A list of cursor images.
    images: SmallList(CursorImage),
    /// A pre-defined cursor.
    keyword: CursorKeyword,
}

/// A [cursor image](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#cursor) value, used in the `cursor` property.
///
/// See [Cursor](Cursor).
pub struct CursorImage {
    /// A url to the cursor image.
    url: Url,
    /// The location in the image where the mouse pointer appears.
    hotspot: [2]CSSNumber?,
}

/// A pre-defined [cursor](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#cursor) value,
/// used in the `cursor` property.
///
/// See [Cursor](Cursor).
pub const CursorKeyword = css.DefineEnumProperty(@compileError(css.todo_stuff.depth));

/// A value for the [caret-color](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#caret-color) property.
pub const ColorOrAuto = union(enum) {
    /// The `currentColor`, adjusted by the UA to ensure contrast against the background.
    auto,
    /// A color.
    color: CssColor,
};

/// A value for the [caret-shape](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#caret-shape) property.
pub const CaretShape = css.DefineEnumProperty(@compileError(css.todo_stuff.depth));

/// A value for the [caret](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#caret) shorthand property.
pub const Caret = @compileError(css.todo_stuff.depth);

/// A value for the [user-select](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#content-selection) property.
pub const UserSelect = css.DefineEnumProperty(@compileError(css.todo_stuff.depth));

/// A value for the [appearance](https://www.w3.org/TR/2021/WD-css-ui-4-20210316/#appearance-switching) property.
pub const Appearance = union(enum) {
    none,
    auto,
    textfield,
    menulist_button,
    button,
    checkbox,
    listbox,
    menulist,
    meter,
    progress_bar,
    push_button,
    radio,
    searchfield,
    slider_horizontal,
    square_button,
    textarea,
    non_standard: []const u8,
};

pub struct ColorSchemeHandler {
    fn handleProperty(_: *@This(), property: *const css.Property, dest: *css.DeclarationList, context: *css.PropertyHandlerContext): bool {
        switch (property.*) {
            .@"color-scheme" => |*color_scheme_| {
                const color_scheme: *const ColorScheme = color_scheme_;
                if (!context.targets.isCompatible(css.compat.Feature.light_dark)) {
                    if (color_scheme.light) {
                        dest.append(
                            context.allocator,
                            defineVar(context.allocator, "--buncss-light", { ident: "initial" }),
                        ) catch |e| bun.handleOom(e);
                        dest.append(
                            context.allocator,
                            defineVar(context.allocator, "--buncss-dark", { whitespace: " " }),
                        ) catch |e| bun.handleOom(e);

                        if (color_scheme.dark) {
                            context.addDarkRule(
                                context.allocator,
                                defineVar(context.allocator, "--buncss-light", { whitespace: " " }),
                            );
                            context.addDarkRule(
                                context.allocator,
                                defineVar(context.allocator, "--buncss-dark", { ident: "initial" }),
                            );
                        }
                    } else if (color_scheme.dark) {
                        bun.handleOom(dest.append(context.allocator, defineVar(context.allocator, "--buncss-light", { whitespace: " " })));
                        bun.handleOom(dest.append(context.allocator, defineVar(context.allocator, "--buncss-dark", { ident: "initial" })));
                    }
                }
                bun.handleOom(dest.append(context.allocator, property.deepClone(context.allocator)));
                return true;
            },
            else => return false,
        }
    }

    fn finalize(_: *@This(), _: *css.DeclarationList, _: *css.PropertyHandlerContext)  {}
}

fn defineVar(allocator: Allocator, name: []const u8, value: css.Token): css.Property {
    return css.Property{
        custom: css.css_properties.custom.CustomProperty{
            name: css.css_properties.custom.CustomPropertyName{ .custom = css.DashedIdent{ .v = name } },
            value: css.TokenList{
                v: brk: {
                    let list: ArrayList(css.css_properties.custom.TokenOrValue) = {};
                    bun.handleOom(list.append(allocator, css.css_properties.custom.TokenOrValue{ .token = value }));
                    break :brk list;
                },
            },
        },
    };
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
