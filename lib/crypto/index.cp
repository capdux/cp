// Source: Custom implementation by Antigravity AI
// lib/crypto/index.cp
// Unified Entry Point for Industrial Cryptography Subsystem (cap:crypto)

export {
    HashAlgorithm,
    Hash,
    sha256,
    hash,
    sha512,
    sha1,
    md5,
    createHash,
} from "./hash.cp";

export {
    HmacAlgorithm,
    Hmac,
    hmacSha256,
    hmacSha256Raw,
    createHmac,
} from "./hmac.cp";

export {
    randomBytes,
    fillRandom,
    randomUUID,
    randomInt,
} from "./random.cp";

export {
    pbkdf2Sync,
    pbkdf2Raw,
    bytesToHex,
} from "./pbkdf2.cp";

export {
    Aes256Gcm,
    ChaCha20Poly1305,
} from "./cipher.cp";

export {
    load as loadBoringSSL,
    isSafeAltName,
    matchDnsName,
    checkX509ServerIdentity,
    checkServerIdentity,
} from "./tls.cp";

