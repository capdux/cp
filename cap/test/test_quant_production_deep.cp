// Source: cp/cap/test/test_quant_production_deep.cp (Deep production test suite for cap:quant)

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    QuantError,
    quantizeInt8Symmetric,
    quantizeInt8Affine,
    dequantizeInt8,
    dequantizeAffine,
    dotInt8,
    quantizeBinary,
    hammingDistance,
    binaryCosineApprox,
    trainPqKMeans,
    encodePq,
    buildAdcDistanceTable,
    adcDistance,
} from "cap:quant";

test("scalar int8 symmetric and affine quantization accuracy and dotInt8") {
    const alloc = std.testing.allocator;

    const a: [8]f32 = [ 1.0, -0.5, 0.25, -1.0, 0.75, -0.125, 0.5, 0.0 ];
    const b: [8]f32 = [ 0.9, -0.4, 0.3, -0.95, 0.7, -0.1, 0.45, 0.05 ];

    let qa = try quantizeInt8Symmetric(alloc, &a);
    defer qa.deinit(alloc);
    let qb = try quantizeInt8Symmetric(alloc, &b);
    defer qb.deinit(alloc);

    const deq_a = try dequantizeInt8(alloc, qa.data, qa.params);
    defer alloc.free(deq_a);

    let i: usize = 0;
    while (i < a.len) {
        const err = @abs(deq_a[i] - a[i]);
        try std.testing.expect(err <= qa.params.scale + 0.001);
        i += 1;
    }

    // Verify affine u8 quantization round-trip
    let af_a = try quantizeInt8Affine(alloc, &a);
    defer af_a.deinit(alloc);
    const deq_af = try dequantizeAffine(alloc, af_a.data, af_a.params);
    defer alloc.free(deq_af);

    i = 0;
    while (i < a.len) {
        const err = @abs(deq_af[i] - a[i]);
        try std.testing.expect(err <= af_a.params.scale + 0.001);
        i += 1;
    }

    // Verify dotInt8 approximates true f32 dot product
    let true_dot: f32 = 0.0;
    i = 0;
    while (i < a.len) {
        true_dot += a[i] * b[i];
        i += 1;
    }
    const approx_dot = try dotInt8(qa.data, qa.params.scale, qb.data, qb.params.scale);
    try std.testing.expect(@abs(approx_dot - true_dot) < 0.05);
}

test("binary 1-bit 32x compression and popCount hammingDistance") {
    const alloc = std.testing.allocator;

    // 128-dim f32 vector = 512 bytes; 1-bit binary = 2 u64 words = 16 bytes (32x compression)
    let v1: [128]f32 = undefined;
    let v2: [128]f32 = undefined;
    let i: usize = 0;
    while (i < 128) {
        v1[i] = if (i % 2 == 0) 1.0 else -1.0;
        // Flip first 10 dimensions in v2
        if (i < 10) {
            v2[i] = 0.0 - v1[i];
        } else {
            v2[i] = v1[i];
        }
        i += 1;
    }

    let b1 = try quantizeBinary(alloc, &v1);
    defer b1.deinit(alloc);
    let b2 = try quantizeBinary(alloc, &v2);
    defer b2.deinit(alloc);

    try expect_eq(b1.byteSize(), @as(usize, 16));
    try expect_eq(v1.len * @sizeOf(f32) / b1.byteSize(), @as(usize, 32));

    const dist = try hammingDistance(b1, b2);
    try expect_eq(dist, @as(u32, 10));

    const self_dist = try hammingDistance(b1, b1);
    try expect_eq(self_dist, @as(u32, 0));

    const cos_sim = try binaryCosineApprox(b1, b2);
    try std.testing.expect(cos_sim > 0.8 and cos_sim < 0.9);
}

test("product quantization kmeans training encoding and adc ranking") {
    const alloc = std.testing.allocator;

    const v0: [8]f32 = [ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 ];
    const v1: [8]f32 = [ 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1 ];
    const v2: [8]f32 = [ 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0 ];
    const v3: [8]f32 = [ 5.1, 4.9, 5.0, 5.1, 4.9, 5.0, 5.1, 4.9 ];
    const v4: [8]f32 = [ 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0 ];
    const v5: [8]f32 = [ 10.1, 9.9, 10.0, 10.1, 9.9, 10.0, 10.1, 9.9 ];

    const train_vecs: [6][]const f32 = [ &v0, &v1, &v2, &v3, &v4, &v5 ];

    // D=8, M=4 subspaces (sub_dim=2), K=3 centroids
    let cb = try trainPqKMeans(alloc, &train_vecs, 4, 3, 20);
    defer cb.deinit(alloc);

    const code0 = try encodePq(alloc, cb, &v0);
    defer alloc.free(code0);
    const code2 = try encodePq(alloc, cb, &v2);
    defer alloc.free(code2);
    const code4 = try encodePq(alloc, cb, &v4);
    defer alloc.free(code4);

    try expect_eq(code0.len, @as(usize, 4));

    const query: [8]f32 = [ 4.95, 5.05, 5.0, 4.95, 5.05, 5.0, 4.95, 5.05 ];
    const table = try buildAdcDistanceTable(alloc, cb, &query);
    defer alloc.free(table);

    const d0 = try adcDistance(table, cb.m, cb.k, code0);
    const d2 = try adcDistance(table, cb.m, cb.k, code2);
    const d4 = try adcDistance(table, cb.m, cb.k, code4);

    // Query near v2 must rank code2 as closest neighbor
    try std.testing.expect(d2 < d0);
    try std.testing.expect(d2 < d4);
}

test("quantization boundary and error handling") {
    const alloc = std.testing.allocator;

    const empty_f32: [0]f32 = [];
    try std.testing.expectError(QuantError.EmptyInput, quantizeInt8Symmetric(alloc, &empty_f32));
    try std.testing.expectError(QuantError.EmptyInput, quantizeInt8Affine(alloc, &empty_f32));
    try std.testing.expectError(QuantError.EmptyInput, quantizeBinary(alloc, &empty_f32));

    // All-zero vector must succeed without division by zero
    const zeros: [4]f32 = [ 0.0, 0.0, 0.0, 0.0 ];
    let qz = try quantizeInt8Symmetric(alloc, &zeros);
    defer qz.deinit(alloc);
    try expect_eq(qz.data[0], @as(i8, 0));

    let afz = try quantizeInt8Affine(alloc, &zeros);
    defer afz.deinit(alloc);
    try expect_eq(afz.data[0], @as(u8, 0));

    // DimensionMismatch checks
    const a3: [3]i8 = [ 1, 2, 3 ];
    const b4: [4]i8 = [ 1, 2, 3, 4 ];
    try std.testing.expectError(QuantError.DimensionMismatch, dotInt8(&a3, 1.0, &b4, 1.0));

    const fv3: [3]f32 = [ 1.0, -1.0, 1.0 ];
    const fv4: [4]f32 = [ 1.0, -1.0, 1.0, -1.0 ];
    let bv3 = try quantizeBinary(alloc, &fv3);
    defer bv3.deinit(alloc);
    let bv4 = try quantizeBinary(alloc, &fv4);
    defer bv4.deinit(alloc);
    try std.testing.expectError(QuantError.DimensionMismatch, hammingDistance(bv3, bv4));

    // D % M != 0 check
    const train_bad: [1][]const f32 = [ &fv3 ];
    try std.testing.expectError(QuantError.InvalidSubspaceCount, trainPqKMeans(alloc, &train_bad, 2, 2, 5));
}

test("cap.quant dynamic mount and std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;

    const vec: [4]f32 = [ 0.5, -0.5, 1.0, -1.0 ];
    let q = try cap.quant.quantizeInt8Symmetric(alloc, &vec);
    defer q.deinit(alloc);

    let b = try cap.quant.quantizeBinary(alloc, &vec);
    defer b.deinit(alloc);

    const h = try cap.quant.hammingDistance(b, b);
    try expect_eq(h, @as(u32, 0));
}
