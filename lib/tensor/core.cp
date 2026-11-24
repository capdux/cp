// Source: cp/lib/tensor/core.cp (Multi-dimensional Strided Tensor & Zero-Copy Views)
// Pure CP industrial multi-dimensional f32 tensor with strided views, slicing, transpose, reshape, and contiguous materialization.

const std = @import("std");

pub const MAX_TENSOR_DIMS: usize = 4;

pub err TensorError {
    DimensionMismatch,
    InvalidShape,
    NonContiguous,
    OutOfBounds,
    EmptyTensor,
    InvalidAxis,
}

fn computeElementCount(shape: []const usize): usize! {
    if (shape.len == 0 || shape.len > MAX_TENSOR_DIMS) {
        return TensorError.InvalidShape;
    }
    let total: usize = 1;
    let i: usize = 0;
    while (i < shape.len) {
        if (shape[i] == 0) {
            return TensorError.EmptyTensor;
        }
        total *= shape[i];
        i += 1;
    }
    return total;
}

fn fillShapeAndStrides(shape: []const usize, out_shape: *[4]usize, out_strides: *[4]usize) {
    let i: usize = 0;
    while (i < MAX_TENSOR_DIMS) {
        out_shape[i] = 0;
        out_strides[i] = 0;
        i += 1;
    }
    i = 0;
    while (i < shape.len) {
        out_shape[i] = shape[i];
        i += 1;
    }
    let stride: usize = 1;
    let d: usize = shape.len;
    while (d > 0) {
        d -= 1;
        out_strides[d] = stride;
        stride *= shape[d];
    }
}

pub struct Tensor {
    data: []f32 = [];
    shape: [4]usize = [0, 0, 0, 0];
    strides: [4]usize = [0, 0, 0, 0];
    ndim: usize = 0;
    offset: usize = 0;
    owns_data: bool = false;

    pub fn zeros(alloc: std.mem.Allocator, shape: []const usize): Tensor! {
        return Tensor.full(alloc, shape, 0.0);
    }

    pub fn ones(alloc: std.mem.Allocator, shape: []const usize): Tensor! {
        return Tensor.full(alloc, shape, 1.0);
    }

    pub static fn full(alloc: std.mem.Allocator, shape: []const usize, val: f32): Tensor! {
        const total = computeElementCount(shape)?;
        let buf = alloc.alloc(f32, total)?;
        let i: usize = 0;
        while (i < total) {
            buf[i] = val;
            i += 1;
        }
        let t = Tensor {
            data: buf,
            ndim: shape.len,
            offset: 0,
            owns_data: true,
        };
        fillShapeAndStrides(shape, &t.shape, &t.strides);
        return t;
    }

    pub fn fromSlice(alloc: std.mem.Allocator, shape: []const usize, src: []const f32): Tensor! {
        const total = computeElementCount(shape)?;
        if (src.len != total) {
            return TensorError.DimensionMismatch;
        }
        let buf = alloc.alloc(f32, total)?;
        @memcpy(buf, src);
        let t = Tensor {
            data: buf,
            ndim: shape.len,
            offset: 0,
            owns_data: true,
        };
        fillShapeAndStrides(shape, &t.shape, &t.strides);
        return t;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.owns_data && _.data.len > 0) {
            alloc.free(_.data);
        }
        _.data = [];
        _.ndim = 0;
        _.offset = 0;
        _.owns_data = false;
    }

    pub fn numel(): usize {
        if (_.ndim == 0) {
            return 0;
        }
        let total: usize = 1;
        let i: usize = 0;
        while (i < _.ndim) {
            total *= _.shape[i];
            i += 1;
        }
        return total;
    }

    pub fn shapeSlice(): []const usize {
        return _.shape[0.._.ndim];
    }

    pub fn isContiguous(): bool {
        if (_.ndim == 0) {
            return false;
        }
        let expected: usize = 1;
        let d: usize = _.ndim;
        while (d > 0) {
            d -= 1;
            if (_.shape[d] > 1 && _.strides[d] != expected) {
                return false;
            }
            expected *= _.shape[d];
        }
        return true;
    }

    pub fn linearOffset(indices: []const usize): usize! {
        if (indices.len != _.ndim) {
            return TensorError.DimensionMismatch;
        }
        let idx: usize = _.offset;
        let i: usize = 0;
        while (i < _.ndim) {
            if (indices[i] >= _.shape[i]) {
                return TensorError.OutOfBounds;
            }
            idx += indices[i] * _.strides[i];
            i += 1;
        }
        if (idx >= _.data.len) {
            return TensorError.OutOfBounds;
        }
        return idx;
    }

    pub fn at(indices: []const usize): f32! {
        const idx = _.linearOffset(indices)?;
        return _.data[idx];
    }

    pub fn set(*_, indices: []const usize, val: f32): void! {
        const idx = _.linearOffset(indices)?;
        _.data[idx] = val;
    }

    /// Zero-copy 2D transpose by swapping shape[0..2] and strides[0..2].
    pub fn transpose2d(): Tensor! {
        if (_.ndim != 2) {
            return TensorError.DimensionMismatch;
        }
        let view = Tensor {
            data: _.data,
            shape: _.shape,
            strides: _.strides,
            ndim: 2,
            offset: _.offset,
            owns_data: false,
        };
        view.shape[0] = _.shape[1];
        view.shape[1] = _.shape[0];
        view.strides[0] = _.strides[1];
        view.strides[1] = _.strides[0];
        return view;
    }

    /// Zero-copy slice along `axis` in range [start, end).
    pub fn sliceAxis(axis: usize, start: usize, end: usize): Tensor! {
        if (axis >= _.ndim) {
            return TensorError.InvalidAxis;
        }
        if (start >= end || end > _.shape[axis]) {
            return TensorError.OutOfBounds;
        }
        let view = Tensor {
            data: _.data,
            shape: _.shape,
            strides: _.strides,
            ndim: _.ndim,
            offset: _.offset + start * _.strides[axis],
            owns_data: false,
        };
        view.shape[axis] = end - start;
        return view;
    }

    /// Zero-copy reshape for contiguous tensors. Returns NonContiguous if strided.
    pub fn reshape(alloc: std.mem.Allocator, new_shape: []const usize): Tensor! {
        const _unused_alloc = alloc;
        if (!_.isContiguous()) {
            return TensorError.NonContiguous;
        }
        const new_total = computeElementCount(new_shape)?;
        if (new_total != _.numel()) {
            return TensorError.InvalidShape;
        }
        let view = Tensor {
            data: _.data,
            ndim: new_shape.len,
            offset: _.offset,
            owns_data: false,
        };
        fillShapeAndStrides(new_shape, &view.shape, &view.strides);
        return view;
    }

    /// Materializes a potentially strided/non-contiguous tensor into a new compact contiguous Tensor.
    pub fn contiguous(alloc: std.mem.Allocator): Tensor! {
        const total = _.numel();
        if (total == 0) {
            return TensorError.EmptyTensor;
        }
        let buf = alloc.alloc(f32, total)?;
        let out = Tensor {
            data: buf,
            ndim: _.ndim,
            offset: 0,
            owns_data: true,
        };
        fillShapeAndStrides(_.shape[0.._.ndim], &out.shape, &out.strides);

        if (_.isContiguous()) {
            @memcpy(buf, _.data[_.offset .. _.offset + total]);
            return out;
        }

        let coords: [4]usize = [0, 0, 0, 0];
        let out_idx: usize = 0;
        while (out_idx < total) {
            let src_idx: usize = _.offset;
            let d: usize = 0;
            while (d < _.ndim) {
                src_idx += coords[d] * _.strides[d];
                d += 1;
            }
            buf[out_idx] = _.data[src_idx];
            out_idx += 1;

            let carry = _.ndim;
            while (carry > 0) {
                carry -= 1;
                coords[carry] += 1;
                if (coords[carry] < _.shape[carry]) {
                    break;
                }
                coords[carry] = 0;
            }
        }
        return out;
    }

    pub fn clone(alloc: std.mem.Allocator): Tensor! {
        return _.contiguous(alloc);
    }
}
