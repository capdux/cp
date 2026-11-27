// Type definitions for CP

export fn mcBytesEq(a: &[]u8, b: &[]u8): bool;
export fn memchr(needle: u8, haystack: &[]u8): i64;
export fn memchr2(n1: u8, n2: u8, haystack: &[]u8): i64;
export fn memchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;
export fn memrchr(needle: u8, haystack: &[]u8): i64;
export fn memrchr2(n1: u8, n2: u8, haystack: &[]u8): i64;
export fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;
export fn memchrCount(needle: u8, haystack: &[]u8): usize;
export fn memchrFindAll(needle: u8, haystack: &[]u8, out_offsets: []usize): usize;
export fn memmemFind(haystack: &[]u8, needle: &[]u8): i64;
export fn memmemRfind(haystack: &[]u8, needle: &[]u8): i64;
export fn countByte(needle: u8, haystack: &[]u8): usize;
