// Source: cp/lib/sha_hmac/index.cp (Unified entry point for cap:sha_hmac & cap.sha_hmac)

export {
    ShaError,
    HashAlg,
    MAX_DIGEST_LEN,
    digestLen,
    blockLen,
    StreamingHasher,
    digestHex,
    hashOneshot,
    hashHex,
} from "./hash.cp";

export {
    StreamingHmac,
    hmacOneshot,
    hmacHex,
    verifyHmac,
} from "./hmac.cp";

export {
    hkdfExtract,
    hkdfExpand,
    hkdf,
    pbkdf2HmacSha256,
    pbkdf2Hmac,
} from "./kdf.cp";
