// Source: cp/lib/uuid/index.cp (Unified Exports for cap:uuid)
// Reference-hardened against uuid-rs/uuid (MIT/Apache-2.0)
// lib/uuid/index.cp
// Unified Public Exports for Distributed Unique Identifier Engine (cap:uuid & cap.uuid)

export {
    Uuid,
    NIL,
    MAX,
    NAMESPACE_DNS,
    NAMESPACE_URL,
    NAMESPACE_OID,
    NAMESPACE_X500,
    VARIANT_NCS,
    VARIANT_RFC4122,
    VARIANT_RFC9562,
    VARIANT_MICROSOFT,
    VARIANT_FUTURE,
    VERSION_NIL,
    VERSION_MAC,
    VERSION_DCE,
    VERSION_MD5,
    VERSION_RANDOM,
    VERSION_SHA1,
    VERSION_SORT_MAC,
    VERSION_SORT_RAND,
    VERSION_CUSTOM,
    VERSION_MAX,
    simdEql16,
    simdIsHex16,
    simdIsHex32,
    simdAsciiToLower16,
    simdNormalizeHexLower32,
    simdDecodeHex32,
    simdFormatHex32,
} from "./uuid.cp";

export {
    Ulid,
} from "./ulid.cp";

export {
    NanoId,
    NANOID_DEFAULT_SIZE,
    NANOID_URL_ALPHABET,
} from "./nanoid.cp";
