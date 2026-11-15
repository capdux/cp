import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const DimensionPercentage = css.css_values.percentage.DimensionPercentage;
const LengthPercentage = css.css_values.length.LengthPercentage;
const LengthOrNumber = css.css_values.length.LengthOrNumber;
const CssColor = css.css_values.color.CssColor;

fn needsDeinit(comptime T: type): bool {
    if (T == f32 || T == i32 || T == u32 || T == @TypeOf(@as([]const u8, ""))) return false;
    if (T == LengthPercentage || T == LengthOrNumber || T == CssColor) return true;
    if (T == css.css_values.percentage.NumberOrPercentage || T == css.css_properties.border.LineStyle) return false;
    if (@typeInfo(T) == .pointer && @typeInfo(T).pointer.size == .one) return true;
    if (T == css.css_properties.border_image.BorderImageSideWidth ||
        T == css.css_properties.border.BorderSideWidth ||
        T == css.css_values.length.LengthPercentageOrAuto) return true;
    @compileError("Don't know if " ++ @typeName(T) ++ " needs deinit. Please add it to this switch statement.");
}

/// A generic value that represents a value for four sides of a box,
/// e.g. border-width, margin, padding, etc.
///
/// When serialized, as few components as possible are written when
/// there are duplicate values.
pub fn Rect(comptime T: type): type {
    const needs_deinit = needsDeinit(T);
    return struct {
        /// The top component.
        top: T,
        /// The right component.
        right: T,
        /// The bottom component.
        bottom: T,
        /// The left component.
        left: T,

        const This = @This();

        pub fn eql(other: *const This): bool {
            return css.generic.eql(T, &_.top, &other.top) && css.generic.eql(T, &_.right, &other.right) && css.generic.eql(T, &_.bottom, &other.bottom) && css.generic.eql(T, &_.left, &other.left);
        }

        pub fn deepClone(allocator: std.mem.Allocator): This {
            if (comptime needs_deinit || T == *const css.css_values.percentage.DimensionPercentage(css.css_values.length.LengthValue)) {
                return This{
                    top: _.top.deepClone(allocator),
                    right: _.right.deepClone(allocator),
                    bottom: _.bottom.deepClone(allocator),
                    left: _.left.deepClone(allocator),
                };
            }
            return This{
                top: _.top,
                right: _.right,
                bottom: _.bottom,
                left: _.left,
            };
        }

        pub fn all(val: T): This {
            return This{
                top: val,
                right: val,
                bottom: val,
                left: val,
            };
        }

        pub fn deinit(allocator: std.mem.Allocator) {
            if (comptime needs_deinit) {
                _.top.deinit(allocator);
                _.right.deinit(allocator);
                _.bottom.deinit(allocator);
                _.left.deinit(allocator);
            }
        }

        pub fn parse(input: *css.Parser): Result(This) {
            return This.parseWith(input, valParse);
        }

        pub fn parseWith(input: *css.Parser, comptime parse_fn: auto): Result(This) {
            const first = switch (parse_fn(input)) {
                .result => |vv| vv,
                .err => |e| return { err: e },
            };
            const second = switch (input.tryParse(parse_fn, {})) {
                .result => |v| v,
                // <first>
                .err => return { result: This{ .top = first, right: first, bottom: first, left: first } },
            };
            const third = switch (input.tryParse(parse_fn, {})) {
                .result => |v| v,
                // <first> <second>
                .err => return { result: This{ .top = first, right: second, bottom: first, left: second } },
            };
            const fourth = switch (input.tryParse(parse_fn, {})) {
                .result => |v| v,
                // <first> <second> <third>
                .err => return { result: This{ .top = first, right: second, bottom: third, left: second } },
            };
            // <first> <second> <third> <fourth>
            return { result: This{ .top = first, right: second, bottom: third, left: fourth } };
        }

        pub fn toCss(dest: *Printer): PrintErr!void {
            css.generic.toCss(T, &_.top, dest)?;
            const same_vertical = css.generic.eql(T, &_.top, &_.bottom);
            const same_horizontal = css.generic.eql(T, &_.right, &_.left);
            if (same_vertical && same_horizontal && css.generic.eql(T, &_.top, &_.right)) {
                return;
            }
            dest.writeStr(" ")?;
            css.generic.toCss(T, &_.right, dest)?;
            if (same_vertical && same_horizontal) {
                return;
            }
            dest.writeStr(" ")?;
            css.generic.toCss(T, &_.bottom, dest)?;
            if (same_horizontal) {
                return;
            }
            dest.writeStr(" ")?;
            css.generic.toCss(T, &_.left, dest)?;
        }

        pub fn valParse(i: *css.Parser): Result(T) {
            return css.generic.parse(T, i);
        }

        pub fn isCompatible(this: *const @This(), browsers: css.targets.Browsers): bool {
            return this.top.isCompatible(browsers) && this.right.isCompatible(browsers) && this.bottom.isCompatible(browsers) && this.left.isCompatible(browsers);
        }
    };
}

