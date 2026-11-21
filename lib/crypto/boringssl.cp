// Source: Absorbed from /data/dev/bun/src/boringssl/boringssl.cp | Synced: 2026-11-20T17:39:25Z
// lib/crypto/boringssl.cp
// BoringSSL Initialization & Server Identity Wrapper for CP (cap:crypto)

export {
    INET6_ADDRSTRLEN,
    INET6_ADDRSTRLEN_BUF,
    NID_subject_alt_name,
    NID_commonName,
    load,
    isSafeAltName,
    matchDnsName,
    TlsHostnameVerifier,
    checkX509ServerIdentity,
    checkServerIdentity,
} from "./tls.cp";
