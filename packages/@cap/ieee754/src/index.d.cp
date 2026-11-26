// Type definitions for CP

export fn readF32Le(buf: &[]u8, offset: usize): f32;
export fn readF32Be(buf: &[]u8, offset: usize): f32;
export fn writeF32Le(buf: []u8, offset: usize, value: f32): bool;
export fn writeF32Be(buf: []u8, offset: usize, value: f32): bool;
export fn readF64Le(buf: &[]u8, offset: usize): f64;
export fn readF64Be(buf: &[]u8, offset: usize): f64;
export fn writeF64Le(buf: []u8, offset: usize, value: f64): bool;
export fn writeF64Be(buf: []u8, offset: usize, value: f64): bool;
export fn readIeee754(buf: &[]u8, offset: usize, is_le: bool, n_bytes: usize): f64;
export fn writeIeee754(buf: []u8, value: f64, offset: usize, is_le: bool, n_bytes: usize): bool;
