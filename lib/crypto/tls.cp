// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/crypto/tls.cp
// BoringSSL TLS & X.509 Server Identity Verification for CP (cap:crypto)

pub const INET6_ADDRSTRLEN: usize = 46;
pub const INET6_ADDRSTRLEN_BUF: usize = 47;
pub const NID_subject_alt_name: i32 = 85;
pub const NID_commonName: i32 = 13;

pub @c {
    @opaque struct SSL_CTX {}
    @opaque struct SSL {}
    @opaque struct X509 {}
    @opaque struct X509_EXTENSION {}
    @opaque struct X509_NAME {}
    @opaque struct X509_NAME_ENTRY {}
    @opaque struct stack_st_X509 {}
    @opaque struct stack_st_GENERAL_NAME {}

    struct ASN1_STRING {
        length: i32;
        type_id: i32;
        data: [*]u8!;
        flags: i64;
    }

    fn CRYPTO_library_init();
    fn SSL_library_init(): i32;
    fn SSL_load_error_strings();
    fn ERR_load_BIO_strings();
    fn OpenSSL_add_all_algorithms();
    fn SSL_CTX_up_ref(ctx: *SSL_CTX!): i32;
    fn X509_get_ext_by_NID(x: *X509!, nid: i32, lastpos: i32): i32;
    fn X509_get_ext(x: *X509!, loc: i32): *X509_EXTENSION!?;
    fn X509_get_subject_name(a: *X509!): *X509_NAME!?;
    fn X509_NAME_get_index_by_NID(name: *X509_NAME!, nid: i32, lastpos: i32): i32;
    fn X509_NAME_get_entry(name: *X509_NAME!, loc: i32): *X509_NAME_ENTRY!?;
    fn X509_NAME_ENTRY_get_data(ne: *X509_NAME_ENTRY!): *ASN1_STRING!?;
    fn ASN1_STRING_get0_data(x: *const ASN1_STRING!): [*]const u8!?;
    fn ASN1_STRING_length(x: *const ASN1_STRING!): i32;
    fn SSL_get_peer_cert_chain(ssl: *const SSL!): *stack_st_X509!?;
    fn sk_X509_value(sk: *const stack_st_X509!, i: usize): *X509!?;
}

pub type OutIPBuf = *[INET6_ADDRSTRLEN_BUF]u8!;
pub type IPTextBuf = [INET6_ADDRSTRLEN_BUF]u8;
pub type CStrPtr = [*:0]u8!;
pub type SSL_CTX_Ptr = *SSL_CTX!?;
pub type SSL_Ptr = *SSL!;
pub type X509_Ptr = *X509!;
pub type ASN1_OCTET_STRING_Ptr = *ASN1_STRING!;

let loaded: bool = false;

pub fn load() {
    if (loaded) {
        return;
    }
    loaded = true;
    unsafe {
        CRYPTO_library_init();
        * = SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
    }
}

pub fn isSafeAltName(name: string, allow_ip: bool): bool {
    if (name.len == 0 || name.len > 253) {
        return false;
    }
    for (ch of name) {
        if (ch == 0 || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            return false;
        }
        if (!allow_ip && ch == ':') {
            return false;
        }
    }
    return true;
}

pub fn matchDnsName(pattern: string, hostname: string): bool {
    if (pattern.len == 0) {
        return false;
    }
    if (!isSafeAltName(pattern, false)) {
        return false;
    }

    if (pattern[0] == '*') {
        if (pattern.len >= 2 && pattern[1] == '.') {
            const suffix = pattern[2..];
            if (std.mem.indexOfScalar(u8, suffix, '.') != null) {
                if (hostname.len > suffix.len + 1) {
                    const dot_index = hostname.len - suffix.len - 1;
                    if (hostname[dot_index] == '.' && std.mem.indexOfScalar(u8, hostname[0..dot_index], '.') == null) {
                        const host_suffix = hostname[dot_index + 1..];
                        if (std.ascii.eqlIgnoreCase(suffix, host_suffix)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    return std.ascii.eqlIgnoreCase(pattern, hostname);
}

pub fn checkX509ServerIdentity(x509: X509_Ptr, hostname: string): bool {
    unsafe {
        if (subject := X509_get_subject_name(x509)) {
            let last: i32 = -1;
            while (true) {
                const entry_idx = X509_NAME_get_index_by_NID(subject, NID_commonName, last);
                if (entry_idx < 0) {
                    break;
                }
                last = entry_idx;
                const entry_opt = X509_NAME_get_entry(subject, entry_idx);
                if (entry_opt == null) {
                    continue;
                }
                const entry = entry_opt.?;
                const data_opt = X509_NAME_ENTRY_get_data(entry);
                if (data_opt == null) {
                    continue;
                }
                const data = data_opt.?;
                const cn_ptr = ASN1_STRING_get0_data(data);
                const cn_len = ASN1_STRING_length(data);
                if (cn_ptr == null || cn_len <= 0) {
                    continue;
                }
                const ptr_nonnull = cn_ptr.?;
                const cn = ptr_nonnull[0..usize(cn_len)];
                if (matchDnsName(cn, hostname)) {
                    return true;
                }
            }
        }
    }
    return false;
}

pub fn checkServerIdentity(ssl_ptr: SSL_Ptr, hostname: string): bool {
    unsafe {
        if (cert_chain := SSL_get_peer_cert_chain(ssl_ptr)) {
            if (x509 := sk_X509_value(cert_chain, 0)) {
                return checkX509ServerIdentity(x509, hostname);
            }
        }
    }
    return false;
}
