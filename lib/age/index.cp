// Origin: Pure-CP Stdlib (Zero-Alloc age-encryption.org/v1 File Format Parser) | Created: 2026-11-22

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
