// Source: Absorbed from /data/dev/bun/src/boringssl/boringssl.cp | Synced: 2026-11-22T11:07:59Z
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

// --- Signatures, OpenPGP, Age & PKI Containers ---

export {
    MINISIGN_UNTRUSTED_PREFIX,
    MINISIGN_TRUSTED_PREFIX,
    MINISIGN_ALG_LEGACY,
    MINISIGN_ALG_PREHASH,
    MINISIGN_PUBKEY_ALG,
    MINISIGN_PUBKEY_BIN_LEN,
    MINISIGN_SIG_BIN_LEN,
    MINISIGN_GLOBAL_SIG_BIN_LEN,
    MinisignPublicKey,
    MinisignSignature,
    parseMinisignPublicKeyLine,
    parseMinisignPublicKey,
    parseMinisignSignature,
    formatMinisignPublicKey,
    formatMinisignSignature,
} from "./minisign.cp";

export {
    PGP_CRC24_INIT,
    PGP_CRC24_POLY,
    TAG_PKESK,
    TAG_SIGNATURE,
    TAG_SKESK,
    TAG_ONE_PASS_SIG,
    TAG_SECRET_KEY,
    TAG_PUBLIC_KEY,
    TAG_SECRET_SUBKEY,
    TAG_COMPRESSED_DATA,
    TAG_SED,
    TAG_MARKER,
    TAG_LITERAL_DATA,
    TAG_TRUST,
    TAG_USER_ID,
    TAG_PUBLIC_SUBKEY,
    TAG_USER_ATTR,
    TAG_SEIPD,
    TAG_MDC,
    PgpArmorHeader,
    PgpPacket,
    PgpPacketIter,
    PgpPublicKeyInfo,
    PgpSignatureInfo,
    PgpLiteralData,
    pgpCrc24,
    parsePgpArmor,
    decodePgpArmor,
    parsePgpPublicKey,
    parsePgpSignature,
    parsePgpLiteralData,
} from "./pgp.cp";

export {
    AGE_VERSION_LINE,
    AGE_ARMOR_BEGIN,
    AGE_ARMOR_END,
    AGE_STREAM_CHUNK_SIZE,
    AGE_STREAM_TAG_SIZE,
    AGE_NONCE_SIZE,
    AgeStanza,
    AgeHeader,
    AgeStanzaIter,
    isAge,
    isAgeArmored,
    parseAgeHeader,
    isValidAgePublicKey,
    isValidAgeSecretKey,
} from "./age.cp";

export {
    SSHSIG_MAGIC,
    SSHSIG_VERSION,
    SSHSIG_ARMOR_BEGIN,
    SSHSIG_ARMOR_END,
    SshsigBlob,
    isSshsigBinary,
    isSshsigArmored,
    decodeSshsigArmor,
    parseSshsigBinary,
    buildSshsigSignedData,
} from "./sshsig.cp";

export {
    OID_DATA,
    OID_SIGNED_DATA,
    OID_ENVELOPED_DATA,
    OID_DIGESTED_DATA,
    OID_ENCRYPTED_DATA,
    Pkcs7ContentType,
    Pkcs7ContentInfo,
    Pkcs7SignedData,
    Pkcs7SignerInfo,
    Pkcs7CertIter,
    Pkcs7SignerIter,
    parseContentInfo,
    isPkcs7SignedData,
    parseSignedData,
} from "./pkcs7.cp";

export {
    OID_RSA_ENCRYPTION,
    OID_EC_PUBLIC_KEY,
    OID_P256,
    OID_P384,
    OID_P521,
    OID_X25519,
    OID_X448,
    OID_ED25519,
    OID_ED448,
    OID_PBES2,
    OID_PBKDF2,
    KeyAlgorithm,
    PrivateKeyInfo,
    EncryptedPrivateKeyInfo,
    SubjectPublicKeyInfo,
    parsePkcs8PrivateKey,
    parsePkcs8Encrypted,
    parseSpkiPublicKey,
} from "./pkcs8.cp";

export {
    OID_KEY_BAG,
    OID_SHROUDED_KEY_BAG,
    OID_CERT_BAG,
    OID_CRL_BAG,
    OID_SECRET_BAG,
    OID_FRIENDLY_NAME,
    OID_LOCAL_KEY_ID,
    Pkcs12BagKind,
    Pkcs12MacData,
    Pkcs12Pfx,
    Pkcs12SafeContent,
    Pkcs12Bag,
    Pkcs12SafeIter,
    Pkcs12BagIter,
    parsePkcs12Der,
    isPkcs12Der,
} from "./pkcs12.cp";

export {
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_BIT_STRING,
    TAG_OCTET_STRING,
    TAG_NULL,
    TAG_OID,
    TAG_UTF8_STRING,
    TAG_PRINTABLE_STRING,
    TAG_IA5_STRING,
    TAG_UTC_TIME,
    TAG_GENERALIZED_TIME,
    TAG_SEQUENCE,
    TAG_SET,
    CLASS_MASK,
    CONSTRUCTED_MASK,
    DerTlv,
    DerBitString,
    DerReader,
    DerWriter,
    parseDerTlv,
    formatDerOid,
} from "./der.cp";

// --- Secret Sharing, Merkle Trees, OTP & Bech32 ---

export {
    ShamirError,
    Share,
    freeShares,
    splitSecret,
    combineShares,
} from "./shamir.cp";

export {
    MerkleError,
    ProofSide,
    ProofStep,
    MerkleProof,
    MerkleTree,
    hashLeaf,
    hashNode,
    verifyProof,
    diffTrees,
} from "./merkle.cp";

export {
    OtpError,
    OtpAlgorithm,
    TotpVerifyResult,
    OtpAuthUri,
    base32Encode,
    base32Decode,
    hotp,
    hotpString,
    totp,
    totpString,
    verifyTotp,
    formatOtpAuthUri,
    parseOtpAuthUri,
} from "./otp.cp";

export {
    BECH32_CONST,
    BECH32M_CONST,
    Bech32Variant,
    Bech32Error,
    DecodedBech32,
    DecodedBytes,
    encode,
    decode,
    convertBits,
    encodeBytes,
    decodeBytes,
    encodeSegwitAddr,
} from "./bech32.cp";

// --- Non-Cryptographic / Checksum / Locality-Sensitive Hashes ---

export {
    SIPHASH_KEY_LEN,
    SipHashStream,
    siphash24,
    siphash13,
    siphash24_128,
    verifySipHash24,
} from "./siphash.cp";

export {
    FNV32_OFFSET_BASIS,
    FNV32_PRIME,
    FNV64_OFFSET_BASIS,
    FNV64_PRIME,
    FNV128_OFFSET_BASIS,
    FNV128_PRIME,
    Fnv1a32,
    Fnv1a64,
    Fnv1a128,
    Fnv1_32,
    Fnv1_64,
    Fnv1_128,
    fnv1a32,
    fnv1a64,
    fnv1a128,
    fnv1_32,
    fnv1_64,
    fnv1_128,
    fnvXorFold32,
    djb2,
    djb2Xor,
    sdbm,
    jenkinsOaat,
} from "./fnv.cp";

export {
    murmur3_32,
    murmur3_128,
    Murmur3_32Stream,
    Murmur3_128Stream,
} from "./murmur3.cp";

export {
    crc8Smbus,
    crc16CcittFalse,
    crc16Xmodem,
    crc16Modbus,
    Crc32Stream,
    crc32Ieee,
    crc32Castagnoli,
    crc32Combine,
    Crc64Stream,
    crc64Xz,
    crc64Ecma182,
} from "./crc.cp";

export {
    SIMHASH_BITS,
    SIMHASH_DEFAULT_THRESHOLD,
    SimHasher,
    simhashText,
    simhashShingles,
    hammingDistance,
    simhashSimilarity,
    isNearDuplicate,
    simhashBands4,
    sharesAnyBand4,
} from "./simhash.cp";

export {
    MINHASH_DEFAULT_K,
    MINHASH_EMPTY_VAL,
    MinHash,
    minhashJaccard,
    minhashBandHash,
    minhashCardinality,
} from "./minhash.cp";

// --- Multi-Version KeyRing Rotation & Envelope MAC Verifier ---

export {
    KEYRING_TAG_LEN,
    KEYRING_SECRET_LEN,
    KeyVersionState,
    KeyRingEntry,
    KeyRingStats,
    KeyRing,
} from "./keyring.cp";

// --- Zero-Alloc RFC 6962 Domain-Separated Merkle Tree & Inclusion Proofs ---

export {
    MERKLE_HASH_LEN,
    MERKLE_MAX_LEAVES,
    MERKLE_MAX_PROOF_DEPTH,
    MerkleHash,
    MerkleProofStep,
    MerkleInclusionProof,
    merkleHashEmpty,
    merkleHashLeaf,
    merkleHashNode,
    Rfc6962MerkleTree,
    verifyMerkleInclusionProof,
} from "./merkle_tree.cp";

// --- Zero-Alloc Forward-Secret Symmetric Key Ratchet & Out-of-Order Key Cache ---

export {
    RATCHET_KEY_LEN,
    RATCHET_MAX_SKIPPED_KEYS,
    RatchetDerivedKey,
    RatchetStats,
    SymmetricKeyRatchet,
} from "./key_ratchet.cp";

// --- Zero-Alloc Versioned Envelope Encryption & Key-Rotation Keyring ---

export {
    ENVELOPE_MAGIC,
    ENVELOPE_FORMAT_VERSION,
    ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256,
    ENVELOPE_FIXED_HEADER_LEN,
    ENVELOPE_TAG_LEN,
    ENVELOPE_KEK_LEN,
    ENVELOPE_MIN_NONCE_LEN,
    ENVELOPE_MAX_NONCE_LEN,
    ENVELOPE_MIN_SALT_LEN,
    ENVELOPE_MAX_SALT_LEN,
    ENVELOPE_KEYRING_MAX_KEYS,
    EnvelopeStatus,
    EnvelopeKeyState,
    EnvelopeHeader,
    EnvelopeDecodeResult,
    EnvelopeOpResult,
    EnvelopeKeyEntry,
    EnvelopeKeyringStats,
    EnvelopeKeyring,
    encodeEnvelopeHeader,
    decodeEnvelopeHeader,
    sealEnvelope,
    openEnvelope,
} from "./envelope_seal.cp";

// --- Zero-Alloc GF(256) Shamir (k, n) Threshold Secret Sharing ---

export {
    SHAMIR_MAX_SECRET_LEN,
    SHAMIR_MIN_THRESHOLD,
    SHAMIR_MAX_SHARES,
    SHAMIR_TAG_LEN,
    gf256Mul,
    gf256Inv,
    gf256Div,
    ShamirStatus,
    ShamirShare,
    ShamirSplitResult,
    ShamirCombineResult,
    shamirRequiredEntropyBytes,
    computeSecretTagSha256,
    verifyReconstructedSecret,
    splitSecretWithEntropy,
    splitSecretDeterministic,
    combineShamirShares,
} from "./shamir_secret.cp";
