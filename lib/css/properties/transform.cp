import css from "../css_parser.zig";

const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Result = css.Result;
const VendorPrefix = css.VendorPrefix;
const Property = css.css_properties.Property;

const LengthPercentage = css.css_values.length.LengthPercentage;
const Length = css.css_values.length.LengthValue;
const NumberOrPercentage = css.css_values.percentage.NumberOrPercentage;
const Angle = css.css_values.angle.Angle;

/// A value for the [transform](https://www.w3.org/TR/2019/CR-css-transforms-1-20190214/#propdef-transform) property.
pub struct TransformList {
    v: ArrayList(Transform),

    static fn parse(input: *css.Parser): Result(@This()) {
        if (input.tryParse(css.Parser.expectIdentMatching, .{"none"}).isOk()) {
            return { result: { v: {} } };
        }

        input.skipWhitespace();
        let results: ArrayList(Transform) = {};
        switch (Transform.parse(input)) {
            .result => |first| bun.handleOom(results.append(input.allocator(), first)),
            .err => |e| return { err: e },
        }

        while (true) {
            input.skipWhitespace();
            if (item := input.tryParse(Transform.parse, {}).asValue()) {
                bun.handleOom(results.append(input.allocator(), item));
            } else {
                return { result: { v: results } };
            }
        }
    }

    fn toCss(dest: *Printer): PrintErr!void {
        if (_.v.items.len == 0) {
            return dest.writeStr("none");
        }

        // TODO: Re-enable with a better solution
        //       See: https://github.com/parcel-bundler/lightningcss/issues/288
        if (dest.minify) {
            let base = std.Io.Writer.Allocating.init(dest.allocator);
            const base_writer = &base.writer;

            let scratchbuf = std.array_list.Managed(u8).init(dest.allocator);
            defer scratchbuf.deinit();
            let p = Printer.new(
                dest.allocator,
                scratchbuf,
                base_writer,
                css.PrinterOptions.defaultWithMinify(true),
                dest.import_info,
                dest.local_names,
                dest.symbols,
            );
            defer p.deinit();

            _.toCssBase(&p)?;

            return dest.writeStr(base.written());
        }

        return _.toCssBase(dest);
    }

    fn toCssBase(dest: *Printer): PrintErr!void {
        for (item of _.v.items) {
            item.toCss(dest)?;
        }
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    static fn eql(lhs: *const @This(), other: *const @This()): bool {
        return css.implementEql(@This(), lhs, other);
    }
}

/// An individual transform function (https://www.w3.org/TR/2019/CR-css-transforms-1-20190214/#two-d-transform-functions).
fn parseTransform(input: *css.Parser): Result(Transform) {
    const fn_res = input.expectFunction();
    if (fn_res == .err) return .{ .err = fn_res.err };
    const function = fn_res.result;

    const Closure = struct { function: []const u8 };
    return input.parseNestedBlock(
        Transform,
        Closure{ .function = function },
        struct {
            fn parse(closure: Closure, i: *css.Parser) css.Result(Transform) {
                const location = i.currentSourceLocation();
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "matrix")) {
                    const a = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const b = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const c = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const d = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const e = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (ee := i.expectComma().asErr()) return { err: ee };
                    const f = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |ee| return { err: ee },
                    };
                    return { result: { matrix: { a: a, b: b, c: c, d: d, e: e, f: f } } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "matrix3d")) {
                    const m11 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m12 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m13 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m14 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m21 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m22 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m23 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m24 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m31 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m32 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m33 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m34 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m41 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m42 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m43 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const m44 = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { matrix_3d: {
                        m11: m11,
                        m12: m12,
                        m13: m13,
                        m14: m14,
                        m21: m21,
                        m22: m22,
                        m23: m23,
                        m24: m24,
                        m31: m31,
                        m32: m32,
                        m33: m33,
                        m34: m34,
                        m41: m41,
                        m42: m42,
                        m43: m43,
                        m44: m44,
                    } } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "translate")) {
                    const x = switch (LengthPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (i.tryParse(struct {
                        fn parse(p: *css.Parser): css.Result(void) {
                            return p.expectComma();
                        }
                    }.parse, {}).isOk()) {
                        const y = switch (LengthPercentage.parse(i)) {
                            .result => |v| v,
                            .err => |e| return { err: e },
                        };
                        return { result: { translate: { x: x, y: y } } };
                    } else {
                        return { result: { translate: { x: x, y: LengthPercentage.zero() } } };
                    }
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "translatex")) {
                    const x = switch (LengthPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { translate_x: x } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "translatey")) {
                    const y = switch (LengthPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { translate_y: y } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "translatez")) {
                    const z = switch (Length.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { translate_z: z } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "translate3d")) {
                    const x = switch (LengthPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const y = switch (LengthPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const z = switch (Length.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { translate_3d: { x: x, y: y, z: z } } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "scale")) {
                    const x = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (i.tryParse(struct {
                        fn parse(p: *css.Parser): css.Result(void) {
                            return p.expectComma();
                        }
                    }.parse, {}).isOk()) {
                        const y = switch (NumberOrPercentage.parse(i)) {
                            .result => |v| v,
                            .err => |e| return { err: e },
                        };
                        return { result: { scale: { x: x, y: y } } };
                    } else {
                        return { result: { scale: { x: x, y: x.deepClone(i.allocator()) } } };
                    }
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "scalex")) {
                    const x = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { scale_x: x } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "scaley")) {
                    const y = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { scale_y: y } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "scalez")) {
                    const z = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { scale_z: z } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "scale3d")) {
                    const x = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const y = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const z = switch (NumberOrPercentage.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { scale_3d: { x: x, y: y, z: z } } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "rotate")) {
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { rotate: angle } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "rotatex")) {
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { rotate_x: angle } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "rotatey")) {
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { rotate_y: angle } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "rotatez")) {
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { rotate_z: angle } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "rotate3d")) {
                    const x = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const y = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const z = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (e := i.expectComma().asErr()) return { err: e };
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { rotate_3d: { x: x, y: y, z: z, angle: angle } } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "skew")) {
                    const x = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    if (i.tryParse(struct {
                        fn parse(p: *css.Parser): css.Result(void) {
                            return p.expectComma();
                        }
                    }.parse, {}).isOk()) {
                        const y = switch (Angle.parseWithUnitlessZero(i)) {
                            .result => |v| v,
                            .err => |e| return { err: e },
                        };
                        return { result: { skew: { x: x, y: y } } };
                    } else {
                        return { result: { skew: { x: x, y: Angle{ .deg = 0.0 } } } };
                    }
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "skewx")) {
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { skew_x: angle } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "skewy")) {
                    const angle = switch (Angle.parseWithUnitlessZero(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { skew_y: angle } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(closure.function, "perspective")) {
                    const len = switch (Length.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { perspective: len } };
                } else {
                    return { err: location.newUnexpectedTokenError({ ident: closure.function }) };
                }
            }
        }.parse,
    );
}

fn toCssTransform(this_t: *const Transform, dest: *Printer): PrintErr!void {
    switch (this_t.*) {
        .translate => |t| {
            if (dest.minify && t.x.isZero() && !t.y.isZero()) {
                dest.writeStr("translateY(")?;
                t.y.toCss(dest)?;
            } else {
                dest.writeStr("translate(")?;
                t.x.toCss(dest)?;
                if (!t.y.isZero()) {
                    dest.delim(',', false)?;
                    t.y.toCss(dest)?;
                }
            }
            dest.writeChar(')')?;
        },
        .translate_x => |x| {
            dest.writeStr(if (dest.minify) "translate(" else "translateX(")?;
            x.toCss(dest)?;
            dest.writeChar(')')?;
        },
        .translate_y => |y| {
            dest.writeStr("translateY(")?;
            y.toCss(dest)?;
            dest.writeChar(')')?;
        },
        .translate_z => |z| {
            dest.writeStr("translateZ(")?;
            z.toCss(dest)?;
            dest.writeChar(')')?;
        },
        .translate_3d => |t| {
            if (dest.minify && !t.x.isZero() && t.y.isZero() && t.z.isZero()) {
                dest.writeStr("translate(")?;
                t.x.toCss(dest)?;
            } else if (dest.minify && t.x.isZero() && !t.y.isZero() && t.z.isZero()) {
                dest.writeStr("translateY(")?;
                t.y.toCss(dest)?;
            } else if (dest.minify && t.x.isZero() && t.y.isZero() && !t.z.isZero()) {
                dest.writeStr("translateZ(")?;
                t.z.toCss(dest)?;
            } else if (dest.minify && t.z.isZero()) {
                dest.writeStr("translate(")?;
                t.x.toCss(dest)?;
                dest.delim(',', false)?;
                t.y.toCss(dest)?;
            } else {
                dest.writeStr("translate3d(")?;
                t.x.toCss(dest)?;
                dest.delim(',', false)?;
                t.y.toCss(dest)?;
                dest.delim(',', false)?;
                t.z.toCss(dest)?;
            }
            dest.writeChar(')')?;
        },
        .scale => |s| {
            const x: f32 = s.x.intoF32();
            const y: f32 = s.y.intoF32();
            if (dest.minify && x == 1.0 && y != 1.0) {
                dest.writeStr("scaleY(")?;
                css.CSSNumberFns.toCss(&y, dest)?;
            } else if (dest.minify && x != 1.0 && y == 1.0) {
                dest.writeStr("scaleX(")?;
                css.CSSNumberFns.toCss(&x, dest)?;
            } else {
                dest.writeStr("scale(")?;
                css.CSSNumberFns.toCss(&x, dest)?;
                if (y != x) {
                    dest.delim(',', false)?;
                    css.CSSNumberFns.toCss(&y, dest)?;
                }
            }
            dest.writeChar(')')?;
        },
        .scale_x => |x| {
            dest.writeStr("scaleX(")?;
            css.CSSNumberFns.toCss(&x.intoF32(), dest)?;
            dest.writeChar(')')?;
        },
        .scale_y => |y| {
            dest.writeStr("scaleY(")?;
            css.CSSNumberFns.toCss(&y.intoF32(), dest)?;
            dest.writeChar(')')?;
        },
        .scale_z => |z| {
            dest.writeStr("scaleZ(")?;
            css.CSSNumberFns.toCss(&z.intoF32(), dest)?;
            dest.writeChar(')')?;
        },
        .scale_3d => |s| {
            const x: f32 = s.x.intoF32();
            const y: f32 = s.y.intoF32();
            const z: f32 = s.z.intoF32();
            if (dest.minify && z == 1.0 && x == y) {
                dest.writeStr("scale(")?;
                css.CSSNumberFns.toCss(&x, dest)?;
            } else if (dest.minify && x != 1.0 && y == 1.0 && z == 1.0) {
                dest.writeStr("scaleX(")?;
                css.CSSNumberFns.toCss(&x, dest)?;
            } else if (dest.minify && x == 1.0 && y != 1.0 && z == 1.0) {
                dest.writeStr("scaleY(")?;
                css.CSSNumberFns.toCss(&y, dest)?;
            } else if (dest.minify && x == 1.0 && y == 1.0 && z != 1.0) {
                dest.writeStr("scaleZ(")?;
                css.CSSNumberFns.toCss(&z, dest)?;
            } else if (dest.minify && z == 1.0) {
                dest.writeStr("scale(")?;
                css.CSSNumberFns.toCss(&x, dest)?;
                dest.delim(',', false)?;
                css.CSSNumberFns.toCss(&y, dest)?;
            } else {
                dest.writeStr("scale3d(")?;
                css.CSSNumberFns.toCss(&x, dest)?;
                dest.delim(',', false)?;
                css.CSSNumberFns.toCss(&y, dest)?;
                dest.delim(',', false)?;
                css.CSSNumberFns.toCss(&z, dest)?;
            }
            dest.writeChar(')')?;
        },
        .rotate => |angle| {
            dest.writeStr("rotate(")?;
            angle.toCssWithUnitlessZero(dest)?;
            dest.writeChar(')')?;
        },
        .rotate_x => |angle| {
            dest.writeStr("rotateX(")?;
            angle.toCssWithUnitlessZero(dest)?;
            dest.writeChar(')')?;
        },
        .rotate_y => |angle| {
            dest.writeStr("rotateY(")?;
            angle.toCssWithUnitlessZero(dest)?;
            dest.writeChar(')')?;
        },
        .rotate_z => |angle| {
            dest.writeStr(if (dest.minify) "rotate(" else "rotateZ(")?;
            angle.toCssWithUnitlessZero(dest)?;
            dest.writeChar(')')?;
        },
        .rotate_3d => |r| {
            if (dest.minify && r.x == 1.0 && r.y == 0.0 && r.z == 0.0) {
                dest.writeStr("rotateX(")?;
                r.angle.toCssWithUnitlessZero(dest)?;
            } else if (dest.minify && r.x == 0.0 && r.y == 1.0 && r.z == 0.0) {
                dest.writeStr("rotateY(")?;
                r.angle.toCssWithUnitlessZero(dest)?;
            } else if (dest.minify && r.x == 0.0 && r.y == 0.0 && r.z == 1.0) {
                dest.writeStr("rotate(")?;
                r.angle.toCssWithUnitlessZero(dest)?;
            } else {
                dest.writeStr("rotate3d(")?;
                css.CSSNumberFns.toCss(&r.x, dest)?;
                dest.delim(',', false)?;
                css.CSSNumberFns.toCss(&r.y, dest)?;
                dest.delim(',', false)?;
                css.CSSNumberFns.toCss(&r.z, dest)?;
                dest.delim(',', false)?;
                r.angle.toCssWithUnitlessZero(dest)?;
            }
            dest.writeChar(')')?;
        },
        .skew => |s| {
            if (dest.minify && s.x.isZero() && !s.y.isZero()) {
                dest.writeStr("skewY(")?;
                s.y.toCssWithUnitlessZero(dest)?;
            } else {
                dest.writeStr("skew(")?;
                s.x.toCss(dest)?;
                if (!s.y.isZero()) {
                    dest.delim(',', false)?;
                    s.y.toCssWithUnitlessZero(dest)?;
                }
            }
            dest.writeChar(')')?;
        },
        .skew_x => |angle| {
            dest.writeStr(if (dest.minify) "skew(" else "skewX(")?;
            angle.toCssWithUnitlessZero(dest)?;
            dest.writeChar(')')?;
        },
        .skew_y => |angle| {
            dest.writeStr("skewY(")?;
            angle.toCssWithUnitlessZero(dest)?;
            dest.writeChar(')')?;
        },
        .perspective => |len| {
            dest.writeStr("perspective(")?;
            len.toCss(dest)?;
            dest.writeChar(')')?;
        },
        .matrix => |m| {
            dest.writeStr("matrix(")?;
            css.CSSNumberFns.toCss(&m.a, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.b, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.c, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.d, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.e, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.f, dest)?;
            dest.writeChar(')')?;
        },
        .matrix_3d => |m| {
            dest.writeStr("matrix3d(")?;
            css.CSSNumberFns.toCss(&m.m11, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m12, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m13, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m14, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m21, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m22, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m23, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m24, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m31, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m32, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m33, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m34, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m41, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m42, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m43, dest)?;
            dest.delim(',', false)?;
            css.CSSNumberFns.toCss(&m.m44, dest)?;
            dest.writeChar(')')?;
        },
    }
}

pub const Transform = union(enum) {
    /// A 2D translation.
    translate: struct {
        x: LengthPercentage,
        y: LengthPercentage,

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },
    /// A translation in the X direction.
    translate_x: LengthPercentage,
    /// A translation in the Y direction.
    translate_y: LengthPercentage,
    /// A translation in the Z direction.
    translate_z: Length,
    /// A 3D translation.
    translate_3d: struct {
        x: LengthPercentage,
        y: LengthPercentage,
        z: Length,

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },
    /// A 2D scale.
    scale: struct {
        x: NumberOrPercentage,
        y: NumberOrPercentage,

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },
    /// A scale in the X direction.
    scale_x: NumberOrPercentage,
    /// A scale in the Y direction.
    scale_y: NumberOrPercentage,
    /// A scale in the Z direction.
    scale_z: NumberOrPercentage,
    /// A 3D scale.
    scale_3d: struct {
        x: NumberOrPercentage,
        y: NumberOrPercentage,
        z: NumberOrPercentage,

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },
    /// A 2D rotation.
    rotate: Angle,
    /// A rotation around the X axis.
    rotate_x: Angle,
    /// A rotation around the Y axis.
    rotate_y: Angle,
    /// A rotation around the Z axis.
    rotate_z: Angle,
    /// A 3D rotation.
    rotate_3d: struct {
        x: f32,
        y: f32,
        z: f32,
        angle: Angle,

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }
    },
    /// A 2D skew.
    skew: struct {
        x: Angle,
        y: Angle,

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },
    /// A skew along the X axis.
    skew_x: Angle,
    /// A skew along the Y axis.
    skew_y: Angle,
    /// A perspective transform.
    perspective: Length,
    /// A 2D matrix transform.
    matrix: Matrix(f32),
    /// A 3D matrix transform.
    matrix_3d: Matrix3d(f32),

    pub const parse = parseTransform;

    pub const toCss = toCssTransform;

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }

    pub fn eql(this: *const @This(), other: *const @This()) bool {
        return css.implementEql(@This(), this, other);
    }
};

/// A 2D matrix.
pub fn Matrix(comptime T: type): type {
    return struct {
        a: T,
        b: T,
        c: T,
        d: T,
        e: T,
        f: T,

        pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
            return css.implementDeepClone(@This(), this, allocator);
        }

        pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
            return css.implementEql(@This(), lhs, rhs);
        }
    };
}

/// A 3D matrix.
pub fn Matrix3d(comptime T: type): type {
    return struct {
        m11: T,
        m12: T,
        m13: T,
        m14: T,
        m21: T,
        m22: T,
        m23: T,
        m24: T,
        m31: T,
        m32: T,
        m33: T,
        m34: T,
        m41: T,
        m42: T,
        m43: T,
        m44: T,

        pub fn eql(lhs: *const @This(), rhs: *const @This()) bool {
            return css.implementEql(@This(), lhs, rhs);
        }
    };
}

/// A value for the [transform-style](https://drafts.csswg.org/css-transforms-2/#transform-style-property) property.
pub enum TransformStyle {
    flat,
    @"preserve-3d",
    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [transform-box](https://drafts.csswg.org/css-transforms-1/#transform-box) property.
pub enum TransformBox {
    /// Uses the content box as reference box.
    @"content-box",
    /// Uses the border box as reference box.
    @"border-box",
    /// Uses the object bounding box as reference box.
    @"fill-box",
    /// Uses the stroke bounding box as reference box.
    @"stroke-box",
    /// Uses the nearest SVG viewport as reference box.
    @"view-box",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [backface-visibility](https://drafts.csswg.org/css-transforms-2/#backface-visibility-property) property.
pub enum BackfaceVisibility {
    visible,
    hidden,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the perspective property.
pub const Perspective = union(enum) {
    /// No perspective transform is applied.
    none,
    /// Distance to the center of projection.
    length: Length,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn eql(this: *const @This(), other: *const @This()) bool {
        return css.implementEql(@This(), this, other);
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

/// A value for the [translate](https://drafts.csswg.org/css-transforms-2/#propdef-translate) property.
fn parseTranslate(input: *css.Parser): css.Result(Translate) {
    if (input.tryParse(css.Parser.expectIdentMatching, .{"none"}).isOk()) {
        return { result: .none };
    }

    const x = switch (LengthPercentage.parse(input)) {
        .result => |v| v,
        .err => |e| return { err: e },
    };
    const y = input.tryParse(LengthPercentage.parse, {});
    const z = if (y.isOk()) input.tryParse(Length.parse, {}).asValue() else null;

    return {
        result: Translate{
            xyz: {
                x: x,
                y: y.unwrapOr(comptime LengthPercentage.zero()),
                z: z ?? Length.zero(),
            },
        },
    };
}

fn toCssTranslate(this_u: *const Translate, dest: *css.Printer): PrintErr!void {
    switch (this_u.*) {
        .none => try dest.writeStr("none"),
        .xyz => |xyz| {
            xyz.x.toCss(dest)?;
            if (!xyz.y.isZero() || !xyz.z.isZero()) {
                dest.writeChar(' ')?;
                xyz.y.toCss(dest)?;
                if (!xyz.z.isZero()) {
                    dest.writeChar(' ')?;
                    xyz.z.toCss(dest)?;
                }
            }
        },
    }
    return;
}


fn toTransformTranslate(this_u: *const Translate, allocator: std.mem.Allocator): Transform {
    return switch (this_u.*) {
        .none => Transform{ translate_3d: .{
            .x = LengthPercentage.zero(),
            .y = LengthPercentage.zero(),
            .z = Length.zero(),
        } },
        .xyz => |xyz| Transform{ translate_3d: .{
            .x = xyz.x.deepClone(allocator),
            .y = xyz.y.deepClone(allocator),
            .z = xyz.z.deepClone(allocator),
        } },
    };
}

pub const Translate = union(enum) {
    /// The "none" keyword.
    none,

    /// The x, y, and z translations.
    xyz: struct {
        /// The x translation.
        x: LengthPercentage,
        /// The y translation.
        y: LengthPercentage,
        /// The z translation.
        z: Length,

        pub fn __generateDeepClone() void {}

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },

    pub const parse = parseTranslate;
    pub const toCss = toCssTranslate;
    pub const toTransform = toTransformTranslate;

    pub fn eql(this: *const @This(), other: *const @This()) bool {
        return css.implementEql(@This(), this, other);
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

/// A value for the [rotate](https://drafts.csswg.org/css-transforms-2/#propdef-rotate) property.
pub struct Rotate {
    /// Rotation around the x axis.
    x: f32,
    /// Rotation around the y axis.
    y: f32,
    /// Rotation around the z axis.
    z: f32,
    /// The angle of rotation.
    angle: Angle,

    static fn parse(input: *css.Parser): css.Result(@This()) {
        if (input.tryParse(css.Parser.expectIdentMatching, .{"none"}).isOk()) {
            return { result: {
                x: 0.0,
                y: 0.0,
                z: 1.0,
                angle: { deg: 0.0 },
            } };
        }

        const angle = input.tryParse(Angle.parse, {});

        const XYZ = struct { x: f32, y: f32, z: f32 };
        const xyz = switch (input.tryParse(struct {
            static fn parse(i: *css.Parser): css.Result(XYZ) {
                const location = i.currentSourceLocation();
                const ident = switch (i.expectIdent()) {
                    .result => |v| v,
                    .err => |e| return { err: e },
                };
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "x")) {
                    return { result: { x: 1.0, y: 0.0, z: 0.0 } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "y")) {
                    return { result: { x: 0.0, y: 1.0, z: 0.0 } };
                } else if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(ident, "z")) {
                    return { result: { x: 0.0, y: 0.0, z: 1.0 } };
                }
                return { err: location.newUnexpectedTokenError({ ident: ident }) };
            }
        }.parse, {})) {
            .result => |v| v,
            .err => input.tryParse(struct {
                static fn parse(i: *css.Parser): css.Result(XYZ) {
                    const x = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    const y = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    const z = switch (css.CSSNumberFns.parse(i)) {
                        .result => |v| v,
                        .err => |e| return { err: e },
                    };
                    return { result: { x: x, y: y, z: z } };
                }
            }.parse, {}).unwrapOr({ x: 0.0, y: 0.0, z: 1.0 }),
        };

        const final_angle = switch (angle) {
            .result => |v| v,
            .err => switch (Angle.parse(input)) {
                .result => |v| v,
                .err => |e| return { err: e },
            },
        };

        return { result: {
            x: xyz.x,
            y: xyz.y,
            z: xyz.z,
            angle: final_angle,
        } };
    }

    fn toCss(dest: *css.Printer): PrintErr!void {
        if (_.x == 0.0 && _.y == 0.0 && _.z == 1.0 && _.angle.isZero()) {
            dest.writeStr("none")?;
            return;
        }

        if (_.x == 1.0 && _.y == 0.0 && _.z == 0.0) {
            dest.writeStr("x ")?;
        } else if (_.x == 0.0 && _.y == 1.0 && _.z == 0.0) {
            dest.writeStr("y ")?;
        } else if (!(_.x == 0.0 && _.y == 0.0 && _.z == 1.0)) {
            css.CSSNumberFns.toCss(&_.x, dest)?;
            dest.writeChar(' ')?;
            css.CSSNumberFns.toCss(&_.y, dest)?;
            dest.writeChar(' ')?;
            css.CSSNumberFns.toCss(&_.z, dest)?;
            dest.writeChar(' ')?;
        }

        _.angle.toCss(dest)?;
    }

    /// Converts the rotation to a transform function.
    fn toTransform(allocator: std.mem.Allocator): Transform {
        return { rotate_3d: {
            x: _.x,
            y: _.y,
            z: _.z,
            angle: _.angle.deepClone(allocator),
        } };
    }

    static fn eql(lhs: *const @This(), other: *const @This()): bool {
        return css.implementEql(@This(), lhs, other);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

/// A value for the [scale](https://drafts.csswg.org/css-transforms-2/#propdef-scale) property.
fn parseScale(input: *css.Parser): css.Result(Scale) {
    if (input.tryParse(css.Parser.expectIdentMatching, .{"none"}).isOk()) {
        return { result: .none };
    }

    const x = switch (NumberOrPercentage.parse(input)) {
        .result => |v| v,
        .err => |e| return { err: e },
    };

    const y = input.tryParse(NumberOrPercentage.parse, {});
    const z = if (y.isOk()) input.tryParse(NumberOrPercentage.parse, {}).asValue() else null;

    return { result: { xyz: {
        x: x,
        y: y.asValue() ?? x,
        z: z ?? { number: 1.0 },
    } } };
}

fn toCssScale(this_u: *const Scale, dest: *css.Printer): PrintErr!void {
    switch (this_u.*) {
        .none => try dest.writeStr("none"),
        .xyz => |xyz| {
            xyz.x.toCss(dest)?;
            const z_val = xyz.z.intoF32();
            if (!xyz.y.eql(&xyz.x) || z_val != 1.0) {
                dest.writeChar(' ')?;
                xyz.y.toCss(dest)?;
                if (z_val != 1.0) {
                    dest.writeChar(' ')?;
                    xyz.z.toCss(dest)?;
                }
            }
        },
    }
}


fn toTransformScale(this_u: *const Scale, allocator: std.mem.Allocator): Transform {
    return switch (this_u.*) {
        .none => Transform{ scale_3d: .{
            .x = .{ .number = 1.0 },
            .y = .{ .number = 1.0 },
            .z = .{ .number = 1.0 },
        } },
        .xyz => |xyz| Transform{ scale_3d: .{
            .x = xyz.x.deepClone(allocator),
            .y = xyz.y.deepClone(allocator),
            .z = xyz.z.deepClone(allocator),
        } },
    };
}

pub const Scale = union(enum) {
    /// The "none" keyword.
    none,

    /// Scale on the x, y, and z axis.
    xyz: struct {
        /// Scale on the x axis.
        x: NumberOrPercentage,
        /// Scale on the y axis.
        y: NumberOrPercentage,
        /// Scale on the z axis.
        z: NumberOrPercentage,

        pub fn __generateDeepClone() void {}

        pub fn eql(this: *const @This(), other: *const @This()) bool {
            return css.implementEql(@This(), this, other);
        }
    },

    pub const parse = parseScale;
    pub const toCss = toCssScale;
    pub const toTransform = toTransformScale;

    pub fn eql(this: *const @This(), other: *const @This()) bool {
        return css.implementEql(@This(), this, other);
    }

    pub fn deepClone(this: *const @This(), allocator: std.mem.Allocator) @This() {
        return css.implementDeepClone(@This(), this, allocator);
    }
};

const TransformAndPrefix = struct { TransformList, VendorPrefix };

pub struct TransformHandler {
    transform: TransformAndPrefix? = null,
    translate: Translate? = null,
    rotate: Rotate? = null,
    scale: Scale? = null,
    has_any: bool = false,

    fn handleProperty(
        *_,
        property: *const css.Property,
        dest: *css.DeclarationList,
        context: *css.PropertyHandlerContext,
    ): bool {
        const individualProperty = struct {
            fn individualProperty(handler: *TransformHandler, allocator: std.mem.Allocator, comptime field: []const u8, val: auto) {
                if (*transform := handler.transform) {
                    bun.handleOom(transform.*[0].v.append(allocator, val.toTransform(allocator)));
                } else {
                    @field(handler, field) = val.deepClone(allocator);
                    handler.has_any = true;
                }
            }
        }.individualProperty;
        const allocator = context.allocator;

        switch (property.*) {
            .transform => |val| {
                const transform_val = val[0];
                const vp = val[1];

                // If two vendor prefixes for the same property have different
                // values, we need to flush what we have immediately to preserve order.
                if (current := _.transform) {
                    if (!current[0].eql(&transform_val) && !bun.bits.contains(css.VendorPrefix, current[1], vp)) {
                        _.flush(allocator, dest, context);
                    }
                }

                // Otherwise, update the value and add the prefix.
                if (*transform := _.transform) {
                    transform.* = .{ transform_val.deepClone(allocator), bun.bits.@"or"(css.VendorPrefix, transform.*[1], vp) };
                } else {
                    _.transform = .{ transform_val.deepClone(allocator), vp };
                    _.has_any = true;
                }

                _.translate = null;
                _.rotate = null;
                _.scale = null;
            },
            .translate => |val| individualProperty(_, allocator, "translate", val),
            .rotate => |val| individualProperty(_, allocator, "rotate", val),
            .scale => |val| individualProperty(_, allocator, "scale", val),
            .unparsed => |unparsed| {
                if (unparsed.property_id == .transform ||
                    unparsed.property_id == .translate ||
                    unparsed.property_id == .rotate ||
                    unparsed.property_id == .scale)
                {
                    _.flush(allocator, dest, context);
                    const prop = if (unparsed.property_id == .transform)
                        Property{ .unparsed = unparsed.getPrefixed(allocator, context.targets, css.prefixes.Feature.transform) }
                    else
                        property.deepClone(allocator);
                    bun.handleOom(dest.append(allocator, prop));
                } else return false;
            },
            else => return false,
        }

        return true;
    }

    fn finalize(*_, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        _.flush(context.allocator, dest, context);
    }

    fn flush(*_, allocator: std.mem.Allocator, dest: *css.DeclarationList, context: *css.PropertyHandlerContext) {
        if (!_.has_any) return;

        _.has_any = false;

        const transform = bun.take(&_.transform);
        const translate = bun.take(&_.translate);
        const rotate = bun.take(&_.rotate);
        const scale = bun.take(&_.scale);

        if (t := transform) {
            const prefix = context.targets.prefixes(t[1], css.prefixes.Feature.transform);
            bun.handleOom(dest.append(allocator, Property{ .transform = .{ t[0], prefix } }));
        }

        if (t := translate) {
            bun.handleOom(dest.append(allocator, Property{ .translate = t }));
        }

        if (r := rotate) {
            bun.handleOom(dest.append(allocator, Property{ .rotate = r }));
        }

        if (s := scale) {
            bun.handleOom(dest.append(allocator, Property{ .scale = s }));
        }
    }
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
