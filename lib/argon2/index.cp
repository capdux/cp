// Source: cp/lib/argon2/index.cp (Public entry point for cap:argon2 & cap.argon2)

export {
    ARGON2_VERSION,
    DEFAULT_MAX_M_COST_KIB,
    Argon2Error,
    Argon2Variant,
    Argon2Params,
    PhcParsed,
} from "./types.cp";

export {
    hashRaw,
    hashPhc,
    hashPhcRandom,
    parsePhc,
    verifyPhc,
    verifyPhcWithLimit,
} from "./engine.cp";
