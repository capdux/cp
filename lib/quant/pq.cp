// Source: cp/lib/quant/pq.cp (Product Quantization K-Means codebook and ADC lookup)

const std = @import("std");
import { QuantError, PqCodebook } from "./types.cp";

fn l2SquaredSlice(a: []const f32, b: []const f32): f32 {
    let sum: f32 = 0.0;
    let i: usize = 0;
    while (i < a.len) {
        const d = a[i] - b[i];
        sum += d * d;
        i += 1;
    }
    return sum;
}

pub fn trainPqKMeans(
    alloc: std.mem.Allocator,
    vectors: []const []const f32,
    m: usize,
    k: usize,
    max_iters: usize,
): QuantError!PqCodebook {
    if (vectors.len == 0) {
        return QuantError.EmptyInput;
    }
    const dim = vectors[0].len;
    if (dim == 0) {
        return QuantError.EmptyInput;
    }
    if (m == 0 or dim % m != 0) {
        return QuantError.InvalidSubspaceCount;
    }
    if (k == 0 or k > 256) {
        return QuantError.InvalidCodebook;
    }

    let v_idx: usize = 1;
    while (v_idx < vectors.len) {
        if (vectors[v_idx].len != dim) {
            return QuantError.DimensionMismatch;
        }
        v_idx += 1;
    }

    const sub_dim = dim / m;
    const total_centroids = m * k * sub_dim;
    let centroids = alloc.alloc(f32, total_centroids) catch return QuantError.OutOfMemory;
    errdefer alloc.free(centroids);

    let sums = alloc.alloc(f32, k * sub_dim) catch return QuantError.OutOfMemory;
    defer alloc.free(sums);
    let counts = alloc.alloc(usize, k) catch return QuantError.OutOfMemory;
    defer alloc.free(counts);
    let assignments = alloc.alloc(usize, vectors.len) catch return QuantError.OutOfMemory;
    defer alloc.free(assignments);

    let sub: usize = 0;
    while (sub < m) {
        const sub_offset = sub * sub_dim;
        const cb_sub_offset = sub * k * sub_dim;

        // Deterministic initialization across training samples
        let c: usize = 0;
        while (c < k) {
            const sample_idx = if (vectors.len >= k) (c * vectors.len) / k else c % vectors.len;
            const src = vectors[sample_idx][sub_offset .. sub_offset + sub_dim];
            let dst = centroids[cb_sub_offset + c * sub_dim .. cb_sub_offset + (c + 1) * sub_dim];
            let d: usize = 0;
            while (d < sub_dim) {
                dst[d] = src[d];
                d += 1;
            }
            c += 1;
        }

        v_idx = 0;
        while (v_idx < vectors.len) {
            assignments[v_idx] = k;
            v_idx += 1;
        }

        let iter: usize = 0;
        while (iter < max_iters) {
            let changed = false;

            let s_idx: usize = 0;
            while (s_idx < k * sub_dim) {
                sums[s_idx] = 0.0;
                s_idx += 1;
            }
            c = 0;
            while (c < k) {
                counts[c] = 0;
                c += 1;
            }

            v_idx = 0;
            while (v_idx < vectors.len) {
                const sub_vec = vectors[v_idx][sub_offset .. sub_offset + sub_dim];
                let best_c: usize = 0;
                let best_dist: f32 = std.math.inf(f32);

                c = 0;
                while (c < k) {
                    const cent = centroids[cb_sub_offset + c * sub_dim .. cb_sub_offset + (c + 1) * sub_dim];
                    const dist = l2SquaredSlice(sub_vec, cent);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_c = c;
                    }
                    c += 1;
                }

                if (assignments[v_idx] != best_c) {
                    assignments[v_idx] = best_c;
                    changed = true;
                }
                counts[best_c] += 1;
                let d: usize = 0;
                while (d < sub_dim) {
                    sums[best_c * sub_dim + d] += sub_vec[d];
                    d += 1;
                }
                v_idx += 1;
            }

            c = 0;
            while (c < k) {
                if (counts[c] > 0) {
                    const inv_cnt = 1.0 / @as(f32, @floatFromInt(counts[c]));
                    let dst = centroids[cb_sub_offset + c * sub_dim .. cb_sub_offset + (c + 1) * sub_dim];
                    let d: usize = 0;
                    while (d < sub_dim) {
                        dst[d] = sums[c * sub_dim + d] * inv_cnt;
                        d += 1;
                    }
                }
                c += 1;
            }

            if (!changed) {
                break;
            }
            iter += 1;
        }

        sub += 1;
    }

    return PqCodebook{
        .dim = dim,
        .m = m,
        .k = k,
        .sub_dim = sub_dim,
        .centroids = centroids,
    };
}

pub fn encodePq(alloc: std.mem.Allocator, codebook: PqCodebook, vec: []const f32): QuantError![]u8 {
    if (vec.len != codebook.dim) {
        return QuantError.DimensionMismatch;
    }
    if (codebook.m == 0 or codebook.k == 0 or codebook.k > 256) {
        return QuantError.InvalidCodebook;
    }

    let codes = alloc.alloc(u8, codebook.m) catch return QuantError.OutOfMemory;
    let sub: usize = 0;
    while (sub < codebook.m) {
        const sub_offset = sub * codebook.sub_dim;
        const cb_sub_offset = sub * codebook.k * codebook.sub_dim;
        const sub_vec = vec[sub_offset .. sub_offset + codebook.sub_dim];

        let best_c: usize = 0;
        let best_dist: f32 = std.math.inf(f32);
        let c: usize = 0;
        while (c < codebook.k) {
            const cent = codebook.centroids[cb_sub_offset + c * codebook.sub_dim .. cb_sub_offset + (c + 1) * codebook.sub_dim];
            const dist = l2SquaredSlice(sub_vec, cent);
            if (dist < best_dist) {
                best_dist = dist;
                best_c = c;
            }
            c += 1;
        }
        codes[sub] = @intCast(best_c);
        sub += 1;
    }

    return codes;
}

pub fn buildAdcDistanceTable(alloc: std.mem.Allocator, codebook: PqCodebook, query: []const f32): QuantError![]f32 {
    if (query.len != codebook.dim) {
        return QuantError.DimensionMismatch;
    }
    if (codebook.m == 0 or codebook.k == 0) {
        return QuantError.InvalidCodebook;
    }

    let table = alloc.alloc(f32, codebook.m * codebook.k) catch return QuantError.OutOfMemory;
    let sub: usize = 0;
    while (sub < codebook.m) {
        const sub_offset = sub * codebook.sub_dim;
        const cb_sub_offset = sub * codebook.k * codebook.sub_dim;
        const sub_query = query[sub_offset .. sub_offset + codebook.sub_dim];

        let c: usize = 0;
        while (c < codebook.k) {
            const cent = codebook.centroids[cb_sub_offset + c * codebook.sub_dim .. cb_sub_offset + (c + 1) * codebook.sub_dim];
            table[sub * codebook.k + c] = l2SquaredSlice(sub_query, cent);
            c += 1;
        }
        sub += 1;
    }
    return table;
}

pub fn adcDistance(table: []const f32, m: usize, k: usize, code: []const u8): QuantError!f32 {
    if (code.len != m or table.len != m * k) {
        return QuantError.DimensionMismatch;
    }

    let sum: f32 = 0.0;
    let sub: usize = 0;
    while (sub < m) {
        const c_idx = @as(usize, code[sub]);
        if (c_idx >= k) {
            return QuantError.InvalidCodebook;
        }
        sum += table[sub * k + c_idx];
        sub += 1;
    }
    return sum;
}
