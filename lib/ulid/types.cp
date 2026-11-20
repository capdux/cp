// Source: cp/lib/ulid/types.cp (ULID Constants and Error Definitions)
// Core error taxonomy and layout constants for 128-bit ULID.

pub const ULID_BYTE_LEN: usize = 16;
pub const ULID_ENCODED_LEN: usize = 26;
pub const ULID_UUID_LEN: usize = 36;
pub const ULID_ENTROPY_LEN: usize = 10;
pub const ULID_MAX_TIMESTAMP: u64 = 0x0000_FFFF_FFFF_FFFF;

pub err UlidError {
    InvalidLength,
    InvalidChar,
    TimestampOverflow,
    RandomOverflow,
    InvalidUuid,
    MonotonicOverflow,
}
