// Source: cp/lib/attention/kv_cache.cp (Autoregressive KV-Cache with Rollback)
const std = @import("std");
import { AttnError } from "./sdpa.cp";

pub struct PagedKvSlot {
    pub layer: usize,
    pub seq_offset: usize,
    pub num_tokens: usize,
}

pub struct KvCache {
    alloc: std.mem.Allocator,
    num_layers: usize,
    num_kv_heads: usize,
    head_dim: usize,
    max_seq_len: usize,
    layer_lens: []usize,
    k_buf: []f32,
    v_buf: []f32,

    pub fn init(
        alloc: std.mem.Allocator,
        num_layers: usize,
        num_kv_heads: usize,
        head_dim: usize,
        max_seq_len: usize,
    ): AttnError!KvCache {
        if (num_layers == 0 or num_kv_heads == 0 or head_dim == 0 or max_seq_len == 0) {
            return AttnError.InvalidDimension;
        }

        let layer_lens = alloc.alloc(usize, num_layers) catch return AttnError.OutOfMemory;
        errdefer alloc.free(layer_lens);
        @memset(layer_lens, 0);

        const total_elems = num_layers * max_seq_len * num_kv_heads * head_dim;
        let k_buf = alloc.alloc(f32, total_elems) catch return AttnError.OutOfMemory;
        errdefer alloc.free(k_buf);
        @memset(k_buf, 0.0);

        let v_buf = alloc.alloc(f32, total_elems) catch return AttnError.OutOfMemory;
        errdefer alloc.free(v_buf);
        @memset(v_buf, 0.0);

        return KvCache{
            .alloc = alloc,
            .num_layers = num_layers,
            .num_kv_heads = num_kv_heads,
            .head_dim = head_dim,
            .max_seq_len = max_seq_len,
            .layer_lens = layer_lens,
            .k_buf = k_buf,
            .v_buf = v_buf,
        };
    }

    pub fn deinit(*_) {
        _.alloc.free(_.layer_lens);
        _.alloc.free(_.k_buf);
        _.alloc.free(_.v_buf);
    }

    pub fn appendStep(
        *_,
        layer: usize,
        k_vec: []const f32,
        v_vec: []const f32,
    ): AttnError!PagedKvSlot {
        if (layer >= _.num_layers) {
            return AttnError.InvalidLayer;
        }
        const kv_stride = _.num_kv_heads * _.head_dim;
        if (k_vec.len != kv_stride or v_vec.len != kv_stride) {
            return AttnError.InvalidDimension;
        }
        const cur_len = _.layer_lens[layer];
        if (cur_len >= _.max_seq_len) {
            return AttnError.CacheFull;
        }

        const layer_stride = _.max_seq_len * kv_stride;
        const dst_off = layer * layer_stride + cur_len * kv_stride;
        @memcpy(_.k_buf[dst_off .. dst_off + kv_stride], k_vec);
        @memcpy(_.v_buf[dst_off .. dst_off + kv_stride], v_vec);
        _.layer_lens[layer] = cur_len + 1;

        return PagedKvSlot{
            .layer = layer,
            .seq_offset = cur_len,
            .num_tokens = 1,
        };
    }

    pub fn seqLen(layer: usize): usize {
        if (layer >= _.num_layers) {
            return 0;
        }
        return _.layer_lens[layer];
    }

    pub fn keysSlice(layer: usize): []const f32 {
        if (layer >= _.num_layers) {
            return _.k_buf[0..0];
        }
        const kv_stride = _.num_kv_heads * _.head_dim;
        const layer_stride = _.max_seq_len * kv_stride;
        const base = layer * layer_stride;
        const count = _.layer_lens[layer] * kv_stride;
        return _.k_buf[base .. base + count];
    }

    pub fn valuesSlice(layer: usize): []const f32 {
        if (layer >= _.num_layers) {
            return _.v_buf[0..0];
        }
        const kv_stride = _.num_kv_heads * _.head_dim;
        const layer_stride = _.max_seq_len * kv_stride;
        const base = layer * layer_stride;
        const count = _.layer_lens[layer] * kv_stride;
        return _.v_buf[base .. base + count];
    }

    pub fn truncateTo(*_, new_len: usize) {
        let i: usize = 0;
        while (i < _.num_layers) : (i += 1) {
            if (_.layer_lens[i] > new_len) {
                _.layer_lens[i] = new_len;
            }
        }
    }

    pub fn reset(*_) {
        @memset(_.layer_lens, 0);
    }
}
