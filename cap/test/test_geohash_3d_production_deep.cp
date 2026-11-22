// Source: cp/cap/test/test_geohash_3d_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { MortonError, Point3D, mortonEncode3D, mortonDecode3D, quantizeEncode3D, haversineMeters } from "cap:geo";

test "geohash_3d: Morton 3D encode/decode exact bit interleaving & roundtrip" {
    try expect_eq(mortonEncode3D(0, 0, 0), 0);
    try expect_eq(mortonEncode3D(1, 0, 0), 1);
    try expect_eq(mortonEncode3D(0, 1, 0), 2);
    try expect_eq(mortonEncode3D(0, 0, 1), 4);
    try expect_eq(mortonEncode3D(1, 1, 1), 7);

    const pt = mortonDecode3D(mortonEncode3D(123456, 654321, 1048575));
    try expect_eq(pt.x, 123456);
    try expect_eq(pt.y, 654321);
    try expect_eq(pt.z, 1048575);
}

test "geohash_3d: max 21-bit boundary (2097151, 2097151, 2097151)" {
    const max_u21: u21 = 2097151;
    const code = mortonEncode3D(max_u21, max_u21, max_u21);
    try expect_eq(code, 0x7FFF_FFFF_FFFF_FFFF);
    const pt = mortonDecode3D(code);
    try expect_eq(pt.x, max_u21);
    try expect_eq(pt.y, max_u21);
    try expect_eq(pt.z, max_u21);
}

test "geohash_3d: quantizeEncode3D locality preservation & range guard" {
    const c1 = try quantizeEncode3D(0.5, 0.5, 0.5);
    const c2 = try quantizeEncode3D(0.5001, 0.5001, 0.5001);
    const c3 = try quantizeEncode3D(0.99, 0.01, 0.01);
    const diff_near = if (c1 > c2) c1 - c2 else c2 - c1;
    const diff_far = if (c1 > c3) c1 - c3 else c3 - c1;
    try expect_eq(diff_near < diff_far, true);

    try expect_eq(quantizeEncode3D(-0.1, 0.5, 0.5) == MortonError.CoordinateOutOfRange, true);
    try expect_eq(quantizeEncode3D(0.5, 1.1, 0.5) == MortonError.CoordinateOutOfRange, true);
}

test "geohash_3d: Haversine great-circle distance NYC to London" {
    // NYC (40.7128, -74.0060) to London (51.5074, -0.1278) ~= 5570 km
    const dist = haversineMeters(40.7128, -74.0060, 51.5074, -0.1278);
    try expect_eq(dist > 5550000.0 and dist < 5600000.0, true);
    try expect_eq(haversineMeters(0.0, 0.0, 0.0, 0.0), 0.0);
}

test "geohash_3d: cap.geohash_3d dynamic mount" {
    const code = cap.geo.mortonEncode3D(7, 8, 9);
    const pt = cap.geo.mortonDecode3D(code);
    try expect_eq(pt.x, 7);
    try expect_eq(pt.y, 8);
    try expect_eq(pt.z, 9);
}
