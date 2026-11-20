// Source: cp/lib/csrf/index.cp (cap:csrf & cap.csrf public exports)
// Pure CP CSRF protection library: CSPRNG + HMAC-SHA256 signed tokens, BREACH masking, and Origin/Referer validation.

export {
    CsrfError,
    CsrfConfig,
    DEFAULT_EXPIRATION_MS,
    SECRET_LEN,
    NONCE_LEN,
    TIMESTAMP_LEN,
    MAC_LEN,
    RAW_TOKEN_LEN,
    MASK_PAD_LEN,
} from "./types.cp";

export {
    CsrfProtector,
    generateSecret,
    generateSecretAlloc,
    signToken,
    signTokenAt,
    verifySignedToken,
    verifySignedTokenWithTtl,
    maskToken,
    unmaskAndVerify,
    unmaskAndVerifyWithTtl,
} from "./token.cp";

export {
    ParsedOrigin,
    isSafeMethod,
    parseOrigin,
    originsMatch,
    verifyOrigin,
} from "./origin.cp";
