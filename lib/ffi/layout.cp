// Source: Absorbed from /data/dev/bun/src/runtime/ffi/ffi.cp | Synced: 2026-11-22T04:04:00Z
// lib/ffi/layout.cp
// C-ABI Struct Memory Alignment, Padding, #packed & Nested Sub-Struct Layout Engine

const std = @import("std");
import { FFIType } from "./dlopen.cp";

pub struct CStructField {
    pub name: []const u8 = "";
    pub ty: FFIType = FFIType.Void;
    pub offset: usize = 0;
    pub size: usize = 0;
    pub align_bytes: usize = 1;
    pub padding_before: usize = 0;
    pub is_nested_struct: bool = false;
}

pub struct CStructLayout {
    pub fields: [64]CStructField = undefined;
    pub field_count: usize = 0;
    pub current_offset: usize = 0;
    pub max_align: usize = 1;
    pub total_size: usize = 0;
    pub total_padding: usize = 0;
    pub trailing_padding: usize = 0;
    pub is_packed: bool = false;
    pub finalized: bool = false;

    /// Initializes a natural C-ABI aligned struct layout calculator.
    pub static fn init(): CStructLayout {
        return CStructLayout {
            fields: undefined,
            field_count: 0,
            current_offset: 0,
            max_align: 1,
            total_size: 0,
            total_padding: 0,
            trailing_padding: 0,
            is_packed: false,
            finalized: false,
        };
    }

    /// Initializes a `#packed` (1-byte aligned, zero padding) struct layout calculator.
    pub static fn initPacked(): CStructLayout {
        return CStructLayout {
            fields: undefined,
            field_count: 0,
            current_offset: 0,
            max_align: 1,
            total_size: 0,
            total_padding: 0,
            trailing_padding: 0,
            is_packed: true,
            finalized: false,
        };
    }

    pub static fn typeAlign(ty: FFIType): usize {
        return FFIType.alignOf(ty);
    }

    pub static fn alignUp(offset: usize, alignment: usize): usize {
        if (alignment <= 1) {
            return offset;
        }
        const rem = offset % alignment;
        if (rem == 0) {
            return offset;
        }
        return offset + (alignment - rem);
    }

    fn appendInternal(
        *_,
        name: []const u8,
        ty: FFIType,
        sz: usize,
        raw_align: usize,
        is_nested: bool
    ): usize {
        const eff_align: usize = if (_.is_packed || raw_align == 0) (1 as usize) else raw_align;
        if (!_.is_packed && eff_align > _.max_align) {
            _.max_align = eff_align;
        }
        const aligned_offset = if (_.is_packed) _.current_offset else CStructLayout.alignUp(_.current_offset, eff_align);
        const pad = aligned_offset - _.current_offset;
        _.total_padding += pad;

        if (_.field_count < 64) {
            _.fields[_.field_count] = CStructField {
                name: name,
                ty: ty,
                offset: aligned_offset,
                size: sz,
                align_bytes: eff_align,
                padding_before: pad,
                is_nested_struct: is_nested,
            };
        }
        _.field_count += 1;
        _.current_offset = aligned_offset + sz;
        _.finalized = false;
        return aligned_offset;
    }

    /// Appends a field of `ty` to the struct layout and returns its byte offset.
    /// Supports arbitrarily many fields for size/offset/padding calculation; first 64 field descriptors are cached.
    pub fn addField(*_, ty: FFIType): usize {
        const sz = FFIType.sizeOf(ty);
        const al = FFIType.alignOf(ty);
        return _.appendInternal("", ty, sz, al, false);
    }

    /// Appends a named field of `ty` and returns its byte offset.
    pub fn addNamedField(*_, name: []const u8, ty: FFIType): usize {
        const sz = FFIType.sizeOf(ty);
        const al = FFIType.alignOf(ty);
        return _.appendInternal(name, ty, sz, al, false);
    }

    /// Appends a custom byte-sized field (e.g. fixed-size C array) with explicit alignment.
    pub fn addBytes(*_, size: usize, align_bytes: usize, name: []const u8 = ""): usize {
        return _.appendInternal(name, FFIType.Buffer, size, align_bytes, false);
    }

    /// Embeds a nested sub-struct layout, respecting its alignment and total size.
    pub fn addStruct(*_, sub: *const CStructLayout): usize {
        return _.addNamedStruct("", sub);
    }

    /// Embeds a named nested sub-struct layout, respecting its alignment and total size.
    pub fn addNamedStruct(*_, name: []const u8, sub: *const CStructLayout): usize {
        const sub_sz = sub.alignedSize();
        const sub_al = if (sub.is_packed) (1 as usize) else sub.max_align;
        return _.appendInternal(name, FFIType.Ptr, sub_sz, sub_al, true);
    }

    /// Computes the aligned total size of the struct without mutating it.
    pub fn alignedSize(): usize {
        if (_.finalized) {
            return _.total_size;
        }
        if (_.is_packed) {
            return _.current_offset;
        }
        return CStructLayout.alignUp(_.current_offset, _.max_align);
    }

    /// Finalizes the struct layout by computing trailing padding and array-stride `total_size`.
    pub fn finalize(*_): usize {
        if (_.finalized) {
            return _.total_size;
        }
        if (_.is_packed) {
            _.total_size = _.current_offset;
            _.trailing_padding = 0;
        } else {
            _.total_size = CStructLayout.alignUp(_.current_offset, _.max_align);
            _.trailing_padding = _.total_size - _.current_offset;
            _.total_padding += _.trailing_padding;
        }
        _.finalized = true;
        return _.total_size;
    }

    /// Returns the byte offset of the field at `index`, or `null` if out of cached bounds.
    pub fn fieldOffset(*_, index: usize): usize? {
        if (index >= _.field_count || index >= 64) {
            return null;
        }
        return _.fields[index].offset;
    }

    /// Looks up a cached field descriptor by name.
    pub fn fieldByName(name: []const u8): CStructField? {
        const limit = if (_.field_count < 64) _.field_count else (64 as usize);
        let i: usize = 0;
        while (i < limit) {
            if (std.mem.eql(u8, _.fields[i].name, name)) {
                return _.fields[i];
            }
            i += 1;
        }
        return null;
    }

    /// Looks up the byte offset of a named field.
    pub fn offsetOf(name: []const u8): usize? {
        if (f := _.fieldByName(name)) {
            return f.offset;
        }
        return null;
    }

    /// Returns total interior + trailing padding bytes accumulated in this layout.
    pub fn paddingBytes(): usize {
        return _.total_padding;
    }

    /// Returns trailing padding bytes added during `finalize()`.
    pub fn trailingPadding(): usize {
        return _.trailing_padding;
    }
}
