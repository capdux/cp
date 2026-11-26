// Type definitions for CP

export fn isSpace(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn trimSlice(s: &[]u8): &[]u8;
export fn unquote(s_in: &[]u8): &[]u8;
export fn eqIgnoreCase(a: &[]u8, b: string): bool;
export fn parseNonNegI64(s_in: &[]u8): i64;
export fn writeStrChecked(dst: []u8, pos: usize, s: string): i64;
export fn writeU64Checked(dst: []u8, pos: usize, v_in: u64): i64;
export fn appendDirectiveFlag(dst: []u8, pos_in: usize, written: &mut usize, name: string): i64;
export fn appendDirectiveKv(dst: []u8, pos_in: usize, written: &mut usize, name: string, val: i64): i64;
export class CacheControl {
    max_age: i64,
    s_maxage: i64,
    stale_while_revalidate: i64,
    stale_if_error: i64,
    no_cache: bool,
    no_store: bool,
    is_public: bool,
    is_private: bool,
    must_revalidate: bool,
    immutable: bool,
    no_transform: bool,
    fn parse(header: &[]u8): CacheControl;
    fn isCacheableShared(): bool;
    fn effectiveTtl(is_shared: bool): i64;
    fn format(dst: []u8): i64;
}

