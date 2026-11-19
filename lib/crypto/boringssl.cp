// Source: /data/dev/bun/src/boringssl/boringssl.cp
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
