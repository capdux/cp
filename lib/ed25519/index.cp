// Source: native-cp-stdlib (RFC 8032 Ed25519 Signatures)

export {
    PUBLIC_KEY_LEN,
    SECRET_KEY_LEN,
    SEED_LEN,
    SIGNATURE_LEN,
    Ed25519Error,
    PublicKey,
    SecretKey,
    Signature,
    KeyPair,
    verify,
    verifyBatch,
    toHex,
    fromHex,
} from "./ed25519.cp";
