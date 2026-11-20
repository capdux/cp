// Source: /data/dev/bun/src/boringssl/boringssl.cp
// lib/crypto/index.cp
// Unified Entry Point for Industrial Cryptography Subsystem (cap:crypto)

export {
    HashAlgorithm,
    Hash,
    sha256,
    sha256Raw,
    hash,
    sha512,
    sha512Raw,
    sha1,
    sha1Raw,
    md5,
    md5Raw,
    createHash,
} from "./hash.cp";

export {
    HmacAlgorithm,
    Hmac,
    hmacSha256,
    hmacSha256Raw,
    hmacSha512,
    hmacSha512Raw,
    createHmac,
} from "./hmac.cp";

export {
    randomBytes,
    fillRandom,
    randomUUID,
    randomUUIDInto,
    randomInt,
} from "./random.cp";

export {
    pbkdf2Into,
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
    TlsHostnameVerifier,
    checkX509ServerIdentity,
    checkServerIdentity,
} from "./tls.cp";

export {
    Ed25519KeyPair,
    X25519KeyPair,
    EcdsaP256KeyPair,
} from "./asymmetric.cp";

export {
    hkdfExtract,
    hkdfExpand,
    hkdfDerive,
    hkdfDeriveAlloc,
    hkdfSha512Extract,
    hkdfSha512Expand,
    hkdfSha512Derive,
    scryptDerive,
    scryptDeriveAlloc,
    argon2idDerive,
    argon2idHash,
    argon2idVerify,
    Blake3Stream,
    Blake3,
    blake3Raw,
    blake3,
    Sha384,
    Sha3_256,
    Sha3_512,
    sha384Raw,
    sha384,
    sha3_256Raw,
    sha3_256,
    sha3_512Raw,
    sha3_512,
    HmacSha384,
    HmacSha512,
    hmacSha384Raw,
    hmacSha384,
    XChaCha20Poly1305,
    Aes128Gcm,
} from "./kdf.cp";

export {
    constantTimeEql,
    base64UrlEncode,
    base64UrlDecode,
    JwtParts,
    Jwt,
    PemBlock,
    Pem,
} from "./jwt.cp";
