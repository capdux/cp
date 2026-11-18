// cap/test/test_builtin_json_unify.cp
// 验证内置全局 JSON.parse 统一零引入（动态 SIMD 零拷贝 + 强类型反射）
import { test, expect_eq } from "cap:test";
const std = @import("std");

pub struct Point {
    x: i32 = 0,
    y: i32 = 0,
}

fn test_builtin_dynamic(): !void {
    const json_str = "{\"name\":\"cp-lang\",\"speed\":100,\"flag\":true}";
    const v = try JSON.parse(json_str);
    expect_eq(v.isObject(), true);
    const name = v.get("name").?;
    expect_eq(std.mem.eql(u8, name.asString().?, "cp-lang"), true);
    const speed = v.get("speed").?;
    expect_eq(speed.asNumber().? == 100.0, true);
    const flag = v.get("flag").?;
    expect_eq(flag.asBool().?, true);
}

fn test_builtin_typed(): !void {
    const pt_str = "{\"x\":42,\"y\":84}";
    const pt = try JSON.parse(Point, pt_str);
    expect_eq(pt.x == 42, true);
    expect_eq(pt.y == 84, true);
}

fn main() {
    test("Builtin Dynamic JSON.parse without imports", () => {
        try test_builtin_dynamic();
    });

    test("Builtin Typed JSON.parse without imports", () => {
        try test_builtin_typed();
    });
}
