// Source: cp/cap/test/test_mime_type_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { Category, classifyMime, byExtension, byPath } from "cap:mime_type";

test "mime_type: classifyMime with charset parameters & case insensitivity" {
    try expect_eq(classifyMime("application/json; charset=utf-8") == .json, true);
    try expect_eq(classifyMime("TEXT/HTML; charset=UTF-8") == .html, true);
    try expect_eq(classifyMime("application/wasm") == .wasm, true);
    try expect_eq(classifyMime("application/octet-stream") == .other, true);
    try expect_eq(classifyMime("image/png") == .image, true);
    try expect_eq(classifyMime("invalid") == .none, true);
}

test "mime_type: Category predicates (isCode, isTextLike, autosetFilename)" {
    try expect_eq(Category.javascript.isCode(), true);
    try expect_eq(Category.wasm.isCode(), true);
    try expect_eq(Category.wasm.isTextLike(), false);
    try expect_eq(Category.css.isTextLike(), true);
    try expect_eq(Category.other.autosetFilename(), true);
    try expect_eq(Category.image.autosetFilename(), false);
}

test "mime_type: byExtension handles leading dot & uppercase" {
    try expect_eq(std.mem.eql(u8, byExtension(".JS").?, "text/javascript; charset=utf-8"), true);
    try expect_eq(std.mem.eql(u8, byExtension("woff2").?, "font/woff2"), true);
    try expect_eq(byExtension("unknown_ext_xyz") == null, true);
}

test "mime_type: byPath handles nested paths & dotless filenames" {
    try expect_eq(std.mem.eql(u8, byPath("/v1.2/static/logo.svg").?, "image/svg+xml"), true);
    try expect_eq(byPath("/v1.2/no_extension") == null, true);
}

test "mime_type: cap.mime_type dynamic mount" {
    const ct = cap.mime_type.byExtension("wasm").?;
    try expect_eq(cap.mime_type.classifyMime(ct) == Category.wasm, true);
}
