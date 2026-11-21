// Source: cp/cap/test/test_css_transform_production_deep.cp (Production deep tests for cap:css_transform)

import { test, expect_eq } from "cap:test";
import {
    Transform,
    CssTransformKind as TransformKind,
    CssTransformList as TransformList,
    Matrix2d,
    Matrix3d,
    TransformOrigin,
    TransformStyle,
    BackfaceVisibility,
} from "cap:css";

test("Matrix2d and Matrix3d identity, multiplication, determinant and invertibility") {
    const id2 = Matrix2d.identity();
    expect_eq(id2.isIdentity(), true);
    expect_eq(id2.determinant(), @as(f64, 1.0));
    expect_eq(id2.isInvertible(), true);

    // 奇异矩阵 det == 0 不可逆
    const singular2 = Matrix2d{ a: 2.0, b: 0.0, c: 0.0, d: 0.0, e: 10.0, f: 20.0 };
    expect_eq(singular2.determinant(), @as(f64, 0.0));
    expect_eq(singular2.isInvertible(), false);

    // 4x4 矩阵乘法：先 scale(2, 3, 1) 再 translate(10, 20, 0) -> T * S
    const t = Matrix3d.translation(10.0, 20.0, 0.0);
    const s = Matrix3d.scaling(2.0, 3.0, 1.0);
    const ts = t.multiply(s);
    expect_eq(ts.is2d(), true);
    expect_eq(ts.determinant(), @as(f64, 6.0));
    expect_eq(ts.isInvertible(), true);

    const m2 = ts.toMatrix2d().?;
    expect_eq(m2.approxEql(Matrix2d{ a: 2.0, b: 0.0, c: 0.0, d: 3.0, e: 10.0, f: 20.0 }, 0.0001), true);

    // 3D 旋转矩阵行列式恒为 1.0 且 is2d() == false
    const rx = Matrix3d.rotationX(3.141592653589793 / 2.0);
    expect_eq(rx.is2d(), false);
    expect_eq(rx.toMatrix2d() == null, true);
    expect_eq(@abs(rx.determinant() - 1.0) <= 0.0001, true);
}

test("2D CSS transform list parsing and matrix composition") {
    const alloc = std.testing.allocator;
    const list = TransformList.parse(
        alloc,
        "translate(10px, 20px) scale(200%, 3) rotate(0.25turn) skewX(0deg)",
    ) catch unreachable;

    expect_eq(list.len(), @as(usize, 4));
    expect_eq(list.isNone(), false);
    expect_eq(list.is2d(), true);

    const m = list.toMatrix3d();
    // translate(10, 20) * scale(2, 3) * rotate(90deg)
    // rotate(90deg): m11=0, m12=1, m21=-1, m22=0
    // scale(2,3) * rot90: m11=0, m12=3, m21=-2, m22=0
    // translate(10,20) * above: m41=10, m42=20
    expect_eq(@abs(m.m11 - 0.0) <= 0.0001, true);
    expect_eq(@abs(m.m12 - 3.0) <= 0.0001, true);
    expect_eq(@abs(m.m21 - (-2.0)) <= 0.0001, true);
    expect_eq(@abs(m.m22 - 0.0) <= 0.0001, true);
    expect_eq(@abs(m.m41 - 10.0) <= 0.0001, true);
    expect_eq(@abs(m.m42 - 20.0) <= 0.0001, true);

    // matrix() 直接解析
    const mlist = TransformList.parse(alloc, "matrix(1, 0, 0, 1, 50, -30)") catch unreachable;
    const mm = mlist.toMatrix3d();
    expect_eq(mm.approxEql(Matrix3d.translation(50.0, -30.0, 0.0), 0.0001), true);
}

test("3D CSS transform functions (translate3d, scale3d, rotate3d, perspective, matrix3d)") {
    const alloc = std.testing.allocator;
    const list = TransformList.parse(
        alloc,
        "perspective(500px) translate3d(10px, 20px, 30px) scale3d(2, 2, 2) rotate3d(0, 1, 0, 180deg)",
    ) catch unreachable;

    expect_eq(list.len(), @as(usize, 4));
    expect_eq(list.is2d(), false);
    expect_eq(list.items()[0].is3d(), true);
    expect_eq(list.items()[1].is3d(), true);
    expect_eq(list.items()[2].is3d(), true);
    expect_eq(list.items()[3].is3d(), true);

    // 验证 perspective(500px) 矩阵 m34 == -0.002
    const pm = list.items()[0].toMatrix3d();
    expect_eq(@abs(pm.m34 - (-0.002)) <= 0.00001, true);

    // matrix3d 16 分量解析
    const m3d_list = TransformList.parse(
        alloc,
        "matrix3d(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 5, 6, 7, 1)",
    ) catch unreachable;
    expect_eq(m3d_list.toMatrix3d().approxEql(Matrix3d.translation(5.0, 6.0, 7.0), 0.0001), true);
}

test("TransformOrigin, TransformStyle and BackfaceVisibility") {
    const c = TransformOrigin.center();
    let buf: [64]u8 = [0; 64];
    expect_eq(c.toCss(buf[0..64]) catch unreachable, "50% 50%");

    const o1 = TransformOrigin.parse("top") catch unreachable;
    expect_eq(o1.toCss(buf[0..64]) catch unreachable, "50% 0%");

    const o2 = TransformOrigin.parse("left bottom") catch unreachable;
    expect_eq(o2.toCss(buf[0..64]) catch unreachable, "0% 100%");

    const o3 = TransformOrigin.parse("10px 25px -40px") catch unreachable;
    expect_eq(o3.toCss(buf[0..64]) catch unreachable, "10px 25px -40px");

    expect_eq(TransformStyle.fromStr("flat").?.is3d(), false);
    expect_eq(TransformStyle.fromStr("preserve-3d").?.is3d(), true);
    expect_eq(TransformStyle.fromStr("preserve-3d").?.asStr(), "preserve-3d");
    expect_eq(TransformStyle.fromStr("3d") == null, true);

    expect_eq(BackfaceVisibility.fromStr("visible").?.isHidden(), false);
    expect_eq(BackfaceVisibility.fromStr("hidden").?.isHidden(), true);
    expect_eq(BackfaceVisibility.fromStr("hidden").?.asStr(), "hidden");
    expect_eq(BackfaceVisibility.fromStr("none") == null, true);
}

test("Canonical toCss serialization and shorthand normalization") {
    const alloc = std.testing.allocator;
    let buf: [128]u8 = [0; 128];

    const empty_list = TransformList.parse(alloc, "none") catch unreachable;
    expect_eq(empty_list.isNone(), true);
    expect_eq(empty_list.toCss(buf[0..128]) catch unreachable, "none");

    // translate(10px, 0) -> translateX(10px); scale(2, 2) -> scale(2); skew(15deg, 0) -> skewX(15deg)
    const list = TransformList.parse(alloc, "translate(10px, 0) scale(2, 2) skew(15deg, 0deg)") catch unreachable;
    expect_eq(list.toCss(buf[0..128]) catch unreachable, "translateX(10px) scale(2) skewX(15deg)");
}

test("Malformed transform syntax, invalid args, negative perspective and zero-leak RAII") {
    const alloc = std.testing.allocator;

    expect_eq(switch (TransformList.parse(alloc, "")) { err.EmptyInput => true, else => false }, true);
    expect_eq(switch (TransformList.parse(alloc, "unknownFn(10px)")) { err.UnknownFunction => true, else => false }, true);
    expect_eq(switch (TransformList.parse(alloc, "rotate(45deg")) { err.InvalidSyntax => true, else => false }, true);
    expect_eq(switch (TransformList.parse(alloc, "translate(10px, 20px, 30px)")) { err.InvalidArgumentCount => true, else => false }, true);
    expect_eq(switch (TransformList.parse(alloc, "perspective(-200px)")) { err.NegativePerspective => true, else => false }, true);
    expect_eq(switch (TransformList.parse(alloc, "rotate(45px)")) { err.InvalidUnit => true, else => false }, true);

    // 缓冲不足防护
    const ok_list = TransformList.parse(alloc, "translateX(10px)") catch unreachable;
    let tiny: [4]u8 = [0; 4];
    expect_eq(switch (ok_list.toCss(tiny[0..4])) { err.BufferTooSmall => true, else => false }, true);
}
