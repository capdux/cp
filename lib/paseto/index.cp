// Source: cp/lib/paseto/index.cp (PASETO v4 Standard Library Entrypoint)
// Unified public exports for `cap:paseto` and `cap.paseto`

export {
    V4_LOCAL_HEADER,
    V4_PUBLIC_HEADER,
    LOCAL_NONCE_LEN,
    LOCAL_TAG_LEN,
    PUBLIC_SIG_LEN,
    PasetoError,
    PasetoPurpose,
    PasetoParsed,
    VerifyClaimsOptions,
    paeEncode,
    PasetoV4,
} from "./paseto.cp";
