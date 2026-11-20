// Source: cp/lib/idna/index.cp (Public Unified Export for cap:idna & cap.idna)

export {
    IdnaError,
    PUNYCODE_BASE,
    PUNYCODE_TMIN,
    PUNYCODE_TMAX,
    PUNYCODE_SKEW,
    PUNYCODE_DAMP,
    PUNYCODE_INITIAL_BIAS,
    PUNYCODE_INITIAL_N,
    decodeUtf8Codepoints,
    punycodeEncode,
    punycodeDecode,
} from "./punycode.cp";

export {
    MAX_LABEL_LEN,
    MAX_DOMAIN_LEN,
    toAscii,
    toUnicode,
    isValidDomain,
    normalizeDomain,
} from "./idna.cp";
