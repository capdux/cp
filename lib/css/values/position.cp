import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const LengthPercentage = css.css_values.length.LengthPercentage;
const Percentage = css.css_values.percentage.Percentage;

pub struct LpAndXPos {
    y_lp: LengthPercentage?,
    x_pos: HorizontalPosition,
}

fn parseLpAndXPos(i: *css.Parser): Result(LpAndXPos) {
    const y_lp = i.tryParse(LengthPercentage.parse, {}).asValue();
    if (x_keyword := i.tryParse(HorizontalPositionKeyword.parse, {}).asValue()) {
        const x_lp = i.tryParse(LengthPercentage.parse, {}).asValue();
        const x_pos = HorizontalPosition{ .side = {
            side: x_keyword,
            offset: x_lp,
        } };
        return { result: LpAndXPos{ y_lp: y_lp, x_pos: x_pos } };
    }
    if (e := i.expectIdentMatching("center").asErr()) return { err: e };
    const x_pos = HorizontalPosition.center;
    return { result: LpAndXPos{ y_lp: y_lp, x_pos: x_pos } };
}

/// A CSS `<position>` value,
/// as used in the `background-position` property, gradients, masks, etc.
pub struct Position {
    /// The x-position.
    x: HorizontalPosition,
    /// The y-position.
    y: VerticalPosition,

    static fn parse(input: *css.Parser): Result(Position) {
        // Try parsing a horizontal position first
        if (horizontal_pos := input.tryParse(HorizontalPosition.parse, {}).asValue()) {
            switch (horizontal_pos) {
                .center => {
                    // Try parsing a vertical position next
                    if (y := input.tryParse(VerticalPosition.parse, {}).asValue()) {
                        return { result: Position{
                            x: .center,
                            y: y,
                        } };
                    }

                    // If it didn't work, assume the first actually represents a y position,
                    // and the next is an x position. e.g. `center left` rather than `left center`.
                    const x = input.tryParse(HorizontalPosition.parse, {}).unwrapOr(HorizontalPosition.center);
                    const y = VerticalPosition.center;
                    return { result: Position{ .x = x, y: y } };
                },
                .length => |*x| {
                    // If we got a length as the first component, then the second must
                    // be a keyword or length (not a side offset).
                    if (y_keyword := input.tryParse(VerticalPositionKeyword.parse, {}).asValue()) {
                        const y = VerticalPosition{ .side = {
                            side: y_keyword,
                            offset: null,
                        } };
                        return { result: Position{ .x = { length: x.* }, y: y } };
                    }
                    if (y_lp := input.tryParse(LengthPercentage.parse, {}).asValue()) {
                        const y = VerticalPosition{ .length = y_lp };
                        return { result: Position{ .x = { length: x.* }, y: y } };
                    }
                    const y = VerticalPosition.center;
                    * = input.tryParse(css.Parser.expectIdentMatching, .{"center"});
                    return { result: Position{ .x = { length: x.* }, y: y } };
                },
                .side => |*side| {
                    const x_keyword = side.side;
                    const lp = side.offset;

                    // If we got a horizontal side keyword (and optional offset), expect another for the vertical side.
                    // e.g. `left center` or `left 20px center`
                    if (input.tryParse(css.Parser.expectIdentMatching, .{"center"}).isOk()) {
                        const x = HorizontalPosition{ .side = {
                            side: x_keyword,
                            offset: lp,
                        } };
                        const y = VerticalPosition.center;
                        return { result: Position{ .x = x, y: y } };
                    }

                    // e.g. `left top`, `left top 20px`, `left 20px top`, or `left 20px top 20px`
                    if (y_keyword := input.tryParse(VerticalPositionKeyword.parse, {}).asValue()) {
                        const y_lp = switch (input.tryParse(LengthPercentage.parse, {})) {
                            .result => |vv| vv,
                            .err => null,
                        };
                        const x = HorizontalPosition{ .side = {
                            side: x_keyword,
                            offset: lp,
                        } };
                        const y = VerticalPosition{ .side = {
                            side: y_keyword,
                            offset: y_lp,
                        } };
                        return { result: Position{ .x = x, y: y } };
                    }

                    // If we didn't get a vertical side keyword (e.g. `left 20px`), then apply the offset to the vertical side.
                    const x = HorizontalPosition{ .side = {
                        side: x_keyword,
                        offset: null,
                    } };
                    let y: VerticalPosition = VerticalPosition.center;
                    if (lp_val := lp) {
                        y = VerticalPosition{ .length = lp_val };
                    }
                    return { result: Position{ .x = x, y: y } };
                },
            }
        }

        // If the horizontal position didn't parse, then it must be out of order. Try vertical position keyword.
        const y_keyword = switch (VerticalPositionKeyword.parse(input)) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        const lp_and_x_pos = input.tryParse(parseLpAndXPos, {});

        if (tuple := lp_and_x_pos.asValue()) {
            const y_lp = tuple.y_lp;
            const x = tuple.x_pos;
            const y = VerticalPosition{ .side = {
                side: y_keyword,
                offset: y_lp,
            } };
            return { result: Position{ .x = x, y: y } };
        }

        const x = HorizontalPosition.center;
        const y = VerticalPosition{ .side = {
            side: y_keyword,
            offset: null,
        } };
        return { result: Position{ .x = x, y: y } };
    }

    fn toCss(dest: *css.Printer): css.PrintErr!void {
        if (_.x == .side && _.y == .length && _.x.side.side != .left) {
            _.x.toCss(dest)?;
            dest.writeStr(" top ")?;
            _.y.length.toCss(dest)?;
        } else if (_.x == .side && _.x.side.side != .left && _.y.isCenter()) {
            // If there is a side keyword with an offset, "center" must be a keyword not a percentage.
            _.x.toCss(dest)?;
            dest.writeStr(" center")?;
        } else if (_.x == .length && _.y == .side && _.y.side.side != .top) {
            dest.writeStr("left ")?;
            _.x.length.toCss(dest)?;
            dest.writeStr(" ")?;
            _.y.toCss(dest)?;
        } else if (_.x.isCenter() && _.y.isCenter()) {
            // `center center` => 50%
            _.x.toCss(dest)?;
        } else if (_.x == .length && _.y.isCenter()) {
            // `center` is assumed if omitted.
            _.x.length.toCss(dest)?;
        } else if (_.x == .side && _.x.side.offset == null && _.y.isCenter()) {
            const p: LengthPercentage = _.x.side.side.intoLengthPercentage();
            p.toCss(dest)?;
        } else if (_.y == .side && _.y.side.offset == null && _.x.isCenter()) {
            _.y.toCss(dest)?;
        } else if (_.x == .side && _.x.side.offset == null && _.y == .side && _.y.side.offset == null) {
            const x: LengthPercentage = _.x.side.side.intoLengthPercentage();
            const y: LengthPercentage = _.y.side.side.intoLengthPercentage();
            x.toCss(dest)?;
            dest.writeStr(" ")?;
            y.toCss(dest)?;
        } else {
            const zero = LengthPercentage.zero();
            const fifty = LengthPercentage{ .percentage = { v: 0.5 } };
            const x_len: (*const LengthPercentage)? = x_len: {
                switch (_.x) {
                    .side => |side| {
                        if (side.side == .left) {
                            if (*offset := side.offset) {
                                if (offset.isZero()) {
                                    break :x_len &zero;
                                } else {
                                    break :x_len offset;
                                }
                            } else {
                                break :x_len &zero;
                            }
                        }
                    },
                    .length => |len| {
                        if (len.isZero()) {
                            break :x_len &zero;
                        }
                    },
                    .center => break :x_len &fifty,
                }
                break :x_len null;
            };

            const y_len: (*const LengthPercentage)? = y_len: {
                switch (_.y) {
                    .side => |side| {
                        if (side.side == .top) {
                            if (*offset := side.offset) {
                                if (offset.isZero()) {
                                    break :y_len &zero;
                                } else {
                                    break :y_len offset;
                                }
                            } else {
                                break :y_len &zero;
                            }
                        }
                    },
                    .length => |len| {
                        if (len.isZero()) {
                            break :y_len &zero;
                        }
                    },
                    .center => break :y_len &fifty,
                }
                break :y_len null;
            };

            if (x_len != null && y_len != null) {
                x_len.?.toCss(dest)?;
                dest.writeStr(" ")?;
                y_len.?.toCss(dest)?;
            } else {
                _.x.toCss(dest)?;
                dest.writeStr(" ")?;
                _.y.toCss(dest)?;
            }
        }
    }

    static fn default(): @This() {
        return {
            x: HorizontalPosition{ .length = LengthPercentage{ .percentage = { v: 0.0 } } },
            y: VerticalPosition{ .length = LengthPercentage{ .percentage = { v: 0.0 } } },
        };
    }

    /// Returns whether both the x and y positions are centered.
    fn isCenter(): bool {
        return _.x.isCenter() && _.y.isCenter();
    }

    static fn center(): Position {
        return { x: .center, y: .center };
    }

    fn eql(other: *const Position): bool {
        return _.x.eql(&other.x) && _.y.eql(&other.y);
    }

    fn isZero(): bool {
        return _.x.isZero() && _.y.isZero();
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

fn parseCenterIdent(i: *css.Parser): Result(void) {
    return i.expectIdentMatching("center");
}

pub fn PositionComponentSide(comptime S: type): type {
    return struct {
        /// A side keyword.
        side: S,
        /// Offset from the side.
        offset: LengthPercentage?,

        pub fn deepClone(allocator: std.mem.Allocator): @This() {
            return css.implementDeepClone(@This(), _, allocator);
        }

        pub fn eql(other: *const @This()): bool {
            return css.implementEql(@This(), _, other);
        }
    };
}

pub fn PositionComponent(comptime S: type): type {
    return union(enum) {
        /// The `center` keyword.
        center,
        /// A length or percentage from the top-left corner of the box.
        length: LengthPercentage,
        /// A side keyword with an optional offset.
        side: PositionComponentSide(S),

        const This = @This();

        pub fn isZero(this: *const This) bool {
            if (this.* == .length and this.length.isZero()) return true;
            return false;
        }

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(this: *const This, other: *const This) bool {
            return css.implementEql(@This(), this, other);
        }

        pub fn parse(input: *css.Parser) Result(This) {
            if (input.tryParse(parseCenterIdent, .{}).isOk()) {
                return .{ .result = .center };
            }

            if (input.tryParse(LengthPercentage.parse, .{}).asValue()) |lp| {
                return .{ .result = .{ .length = lp } };
            }

            const side = switch (S.parse(input)) {
                .result => |vv| vv,
                .err => |e| return .{ .err = e },
            };
            const offset = input.tryParse(LengthPercentage.parse, .{}).asValue();
            return .{ .result = .{ .side = .{ .side = side, .offset = offset } } };
        }

        pub fn toCss(this: *const @This(), dest: *css.Printer) css.PrintErr!void {
            switch (this.*) {
                .center => {
                    if (dest.minify) {
                        try dest.writeStr("50%");
                    } else {
                        try dest.writeStr("center");
                    }
                },
                .length => |*lp| try lp.toCss(dest),
                .side => |*s| {
                    try s.side.toCss(dest);
                    if (s.offset) |lp| {
                        try dest.writeStr(" ");
                        try lp.toCss(dest);
                    }
                },
            }
        }

        pub fn isCenter(this: *const This) bool {
            switch (this.*) {
                .center => return true,
                .length => |*l| {
                    if (l.* == .percentage) return l.percentage.v == 0.5;
                },
                else => {},
            }
            return false;
        }
    };
}

pub enum HorizontalPositionKeyword {
    /// The `left` keyword.
    left,
    /// The `right` keyword.
    right,

    fn deepClone(allocator: std.mem.Allocator): HorizontalPositionKeyword {
        return css.implementDeepClone(@This(), _, allocator);
    }

    fn eql(other: *const HorizontalPositionKeyword): bool {
        return _.* == other.*;
    }

    fn asStr(): []const u8 {
        return css.enum_property_util.asStr(@This(), _);
    }

    static fn parse(input: *css.Parser): Result(@This()) {
        return css.enum_property_util.parse(@This(), input);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.enum_property_util.toCss(@This(), _, dest);
    }

    fn intoLengthPercentage(): LengthPercentage {
        return switch (_.*) {
            .left => LengthPercentage.zero(),
            .right => LengthPercentage{ .percentage = css.css_values.percentage.Percentage{ .v = 1.0 } },
        };
    }
}

pub enum VerticalPositionKeyword {
    /// The `top` keyword.
    top,
    /// The `bottom` keyword.
    bottom,

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    fn eql(other: *const VerticalPositionKeyword): bool {
        return _.* == other.*;
    }

    fn asStr(): []const u8 {
        return css.enum_property_util.asStr(@This(), _);
    }

    static fn parse(input: *css.Parser): Result(@This()) {
        return css.enum_property_util.parse(@This(), input);
    }

    fn toCss(dest: *Printer): PrintErr!void {
        return css.enum_property_util.toCss(@This(), _, dest);
    }

    fn intoLengthPercentage(): LengthPercentage {
        return switch (_.*) {
            .top => LengthPercentage.zero(),
            .bottom => LengthPercentage{ .percentage = Percentage{ .v = 1.0 } },
        };
    }
}

pub const HorizontalPosition = PositionComponent(HorizontalPositionKeyword);
pub const VerticalPosition = PositionComponent(VerticalPositionKeyword);

