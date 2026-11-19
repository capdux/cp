// Source: /data/dev/bun/src/runtime/ffi/
// lib/ffi/layout.cp
// C-ABI Struct Memory Alignment & Offset Calculator for FFI

import { FFIType } from "./dlopen.cp";

pub struct CStructField {
    pub ty: FFIType = FFIType.Void;
    pub offset: usize = 0;
    pub size: usize = 0;
    pub align_bytes: usize = 1;
}

pub struct CStructLayout {
    pub fields: [32]CStructField = undefined;
    pub field_count: usize = 0;
    pub current_offset: usize = 0;
    pub max_align: usize = 1;
    pub total_size: usize = 0;
    pub finalized: bool = false;

    pub static fn init(): CStructLayout {
        return CStructLayout {
            fields: undefined,
            field_count: 0,
            current_offset: 0,
            max_align: 1,
            total_size: 0,
            finalized: false,
        };
    }

    pub static fn typeAlign(ty: FFIType): usize {
        const sz = FFIType.byteSize(ty);
        if (sz == 0) {
            return 1;
        }
        return sz;
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

    /// Appends a field of `ty` to the C struct layout, applying natural C-ABI alignment padding and returning the field's byte offset.
    pub fn addField(*_, ty: FFIType): usize {
        if (_.field_count >= 32) {
            return _.current_offset;
        }
        const sz = FFIType.byteSize(ty);
        const al = CStructLayout.typeAlign(ty);
        if (al > _.max_align) {
            _.max_align = al;
        }
        const aligned_offset = CStructLayout.alignUp(_.current_offset, al);
        _.fields[_.field_count] = CStructField {
            ty: ty,
            offset: aligned_offset,
            size: sz,
            align_bytes: al,
        };
        _.field_count += 1;
        _.current_offset = aligned_offset + sz;
        return aligned_offset;
    }

    /// Finalizes the struct layout by rounding the total size up to `max_align` (for array stride compatibility).
    pub fn finalize(*_): usize {
        _.total_size = CStructLayout.alignUp(_.current_offset, _.max_align);
        _.finalized = true;
        return _.total_size;
    }

    /// Returns the byte offset of the field at `index`, or `null` if out of bounds.
    pub fn fieldOffset(*_, index: usize): usize? {
        if (index >= _.field_count) {
            return null;
        }
        return _.fields[index].offset;
    }
}
