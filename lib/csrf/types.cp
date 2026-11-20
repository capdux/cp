// Source: bun/src/csrf/csrf.cp -> cp/lib/csrf/types.cp (core CSRF types & constants)
// Pure CP CSRF protection types, constants, and error definitions.

pub const DEFAULT_EXPIRATION_MS: u64 = 24 * 60 * 60 * 1000;
pub const SECRET_LEN: usize = 32;
pub const NONCE_LEN: usize = 16;
pub const TIMESTAMP_LEN: usize = 8;
pub const MAC_LEN: usize = 32;
pub const RAW_TOKEN_LEN: usize = TIMESTAMP_LEN + NONCE_LEN + MAC_LEN;
pub const MASK_PAD_LEN: usize = 32;

pub error CsrfError {
    InvalidToken,
    ExpiredToken,
    InvalidSignature,
    DecodingFailed,
    InvalidOrigin,
    CrossSiteRequest,
    MissingOrigin,
    InvalidSecret,
    OutOfMemory,
}

pub const CsrfConfig = struct {
    max_age_ms: u64 = DEFAULT_EXPIRATION_MS,
    require_origin_on_unsafe: bool = true,
    allow_referer_fallback: bool = true,
};
