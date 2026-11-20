// Source: cp/lib/quant/types.cp (AI Embedding vector quantization core types)

const std = @import("std");

pub const QuantError = error{
    DimensionMismatch,
    InvalidSubspaceCount,
    EmptyInput,
    InvalidCodebook,
    OutOfMemory,
};

pub struct ScalarQuantParams {
    pub scale: f32,
    pub zero_point: i32,
}

pub struct Int8Quantized {
    pub data: []i8,
    pub params: ScalarQuantParams,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.data.len > 0) {
            alloc.free(_.data);
        }
    }
}

pub struct AffineQuantized {
    pub data: []u8,
    pub params: ScalarQuantParams,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.data.len > 0) {
            alloc.free(_.data);
        }
    }
}

pub struct BinaryVector {
    pub bits: []u64,
    pub dim: usize,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.bits.len > 0) {
            alloc.free(_.bits);
        }
        _.dim = 0;
    }

    pub fn byteSize(): usize {
        return _.bits.len * @sizeOf(u64);
    }
}

pub struct PqCodebook {
    pub dim: usize,
    pub m: usize,
    pub k: usize,
    pub sub_dim: usize,
    pub centroids: []f32,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.centroids.len > 0) {
            alloc.free(_.centroids);
        }
        _.dim = 0;
        _.m = 0;
        _.k = 0;
        _.sub_dim = 0;
    }
}
