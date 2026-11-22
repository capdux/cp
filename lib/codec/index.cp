// Source: cp/lib/codec/index.cp (Unified Exports for cap:codec)
// Pure CP zero-C-dependency multi-radix & checksum encoding suite
// (Base58, Base58Check, Adobe ASCII85, RFC 1924 Base85, Bech32/Bech32m, Crockford Base32).

export {
    CodecError,
    Base58,
    Base85,
} from "./base58_85.cp";

export {
    Bech32Variant,
    Bech32Decoded,
    Bech32,
    Crockford32,
} from "./bech32_crockford.cp";

// === Consolidated Radix/Text Encodings, Distributed IDs, Check-Digits & 1D Barcodes ===

// --- base32 ---
export * from "./base32";

// --- base64url ---
export * from "./base64url";

// --- punycode ---
export {
    isAsciiOnly,
    encodePunycode,
    decodePunycode,
    encodeIdnLabel,
    decodeIdnLabel,
} from "./punycode.cp";

// --- vlq ---
export * from "./vlq";

// --- uuencode ---
export {
    UU_MAX_LINE_BYTES,
    UU_DEFAULT_MODE,
    XX_ALPHABET,
    UuHeader,
    UuDecoded,
    parseUuHeader,
    uuEncodeRaw,
    uuDecodeRaw,
    uuEncode,
    uuDecode,
    xxEncode,
    xxDecode,
} from "./uuencode.cp";

// --- morse ---
export {
    DIT_UNITS,
    DAH_UNITS,
    INTRA_CHAR_GAP,
    INTER_CHAR_GAP,
    INTER_WORD_GAP,
    PARIS_WORD_UNITS,
    MAX_MORSE_CODE_LEN,
    MorsePulse,
    wpmToDitMs,
    charToMorse,
    morseToChar,
    morseEncode,
    morseDecode,
    morseToPulses,
    morseDurationUnits,
} from "./morse.cp";

// --- braille ---
export {
    BRAILLE_BASE_CP,
    DOT1,
    DOT2,
    DOT3,
    DOT4,
    DOT5,
    DOT6,
    DOT7,
    DOT8,
    BRAILLE_NUMBER_SIGN,
    BRAILLE_CAPITAL_SIGN,
    dotBitForPixel,
    brailleToUtf8,
    utf8ToBraille,
    encodeBrailleMasks,
    decodeBrailleUtf8,
    asciiToBrailleMask,
    textToBrailleUtf8,
    BrailleCanvas,
} from "./braille.cp";

// --- roman ---
export {
    ROMAN_MIN,
    ROMAN_MAX,
    ROMAN_MAX_STR_LEN,
    MAX_COL_LETTERS_LEN,
    romanEncode,
    romanEncodeLower,
    romanDecode,
    romanDecodeLenient,
    isValidRoman,
    colIndexToLetters,
    lettersToColIndex,
} from "./roman.cp";

// --- nanoid ---
export {
    IdError,
    IdRng,
    NanoId,
    Ulid,
    MonotonicUlidGenerator,
    UuidV7,
} from "./nanoid.cp";

// --- ksuid ---
export {
    KSUID_EPOCH,
    KSUID_BYTE_LEN,
    KSUID_ENCODED_LEN,
    KSUID_PAYLOAD_LEN,
    KsuidError,
    Ksuid,
} from "./ksuid.cp";

// --- snowflake ---
export {
    DEFAULT_EPOCH_MS,
    MAX_DATACENTER_ID,
    MAX_WORKER_ID,
    MAX_SEQUENCE,
    SnowflakeError,
    SnowflakeParts,
    SnowflakeGenerator,
    decomposeId,
    encodeBase62,
    decodeBase62,
} from "./snowflake.cp";

// --- cuid ---
export {
    CuidVersion,
    isValidCuid1,
    isValidCuid2,
    detectCuidVersion,
    encodeBase36,
    decodeBase36,
    formatCuid1,
    parseCuid1Timestamp,
} from "./cuid.cp";

// --- cuid2 ---
export * from "./cuid2";

// --- isbn ---
export {
    ISBN10_LEN,
    ISBN13_LEN,
    ISSN_LEN,
    ISSN_FORMATTED_LEN,
    EAN13_LEN,
    EAN8_LEN,
    UPCA_LEN,
    isbn10CheckDigit,
    isValidIsbn10,
    isbn13CheckDigit,
    isValidIsbn13,
    isValidIsbn,
    isbn10To13,
    isbn13To10,
    issnCheckDigit,
    isValidIssn,
    formatIssn,
    eanCheckDigit as isbnEanCheckDigit,
    isValidEan13,
    isValidEan8,
    isValidUpcA,
} from "./isbn.cp";

// --- iban ---
export {
    IBAN_MIN_LEN,
    IBAN_MAX_LEN,
    IbanCheckDigits,
    IbanInfo,
    ibanCountryLength,
    ibanMod97,
    isValidIban,
    parseIban,
    computeIbanCheckDigits,
    formatIbanElectronic,
    formatIbanPrint,
} from "./iban.cp";

// --- verhoeff ---
export {
    verhoeffCheckDigit,
    isValidVerhoeff,
    verhoeffAppend,
    dammCheckDigit,
    isValidDamm,
    dammAppend,
    iso7064Mod11_2CheckDigit,
    isValidIso7064Mod11_2,
    iso7064Mod37_2CheckDigit,
    isValidIso7064Mod37_2,
} from "./verhoeff.cp";

// --- ean ---
export {
    EAN13_MODULES,
    EAN8_MODULES,
    UPCA_MODULES,
    EAN_DIGIT_MODULES,
    GUARD_OUTER,
    GUARD_CENTER,
    eanPatternL,
    eanPatternG,
    eanPatternR,
    ean13ParityMask,
    eanCheckDigit,
    encodeEan13Modules,
    decodeEan13Modules,
    encodeEan8Modules,
    decodeEan8Modules,
    encodeUpcAModules,
} from "./ean.cp";

// --- code128 ---
export {
    CODE128_START_A,
    CODE128_START_B,
    CODE128_START_C,
    CODE128_STOP,
    CODE128_FNC1,
    CODE128_CODE_C,
    CODE128_CODE_B,
    CODE128_CODE_A,
    CODE128_SHIFT,
    CODE128_MOD_PER_SYMBOL,
    CODE128_STOP_MODULES,
    code128Pattern,
    code128SymbolFromPattern,
    code128Checksum,
    encodeCode128B,
    encodeCode128C,
    encodeCode128Auto,
    renderCode128Modules,
    decodeCode128Symbols,
    decodeCode128Modules,
} from "./code128.cp";

// --- code39 ---
export {
    CODE39_ALPHABET_LEN,
    CODE39_START_STOP,
    CODE39_ELEMENTS_PER_CHAR,
    CODE39_WIDE_COUNT,
    code39Value,
    code39Char,
    code39Pattern9,
    code39CharFromPattern9,
    code39CheckDigit,
    verifyCode39Mod43,
    encodeCode39Modules,
    decodeCode39Modules,
} from "./code39.cp";

// ============================================================================
// Batch 38: ECC, Finite Fields, Universal Prefix Codes & DCT Transform Codecs
// ============================================================================
// --- elias ---
export {
    BitWriter,
    BitReader,
    writeEliasGamma,
    readEliasGamma,
    writeEliasDelta,
    readEliasDelta,
    writeEliasOmega,
    readEliasOmega,
    writeExpGolombU,
    readExpGolombU,
    writeExpGolombS,
    readExpGolombS,
    writeRice,
    readRice,
    writeFibonacci,
    readFibonacci,
} from "./elias.cp";

// --- gf256 ---
export {
    GF256_POLY_AES,
    GF256_POLY_RS,
    gf256MulConstTime,
    Gf256Field,
} from "./gf256.cp";

// --- golay ---
export {
    GOLAY24_DATA_BITS,
    GOLAY24_CODE_BITS,
    GOLAY24_MIN_DIST,
    GOLAY23_CODE_BITS,
    GolayStatus,
    GolayDecode,
    golay24Parity,
    golay24Encode,
    golay24Decode,
    golay23Encode,
    golay23Decode,
    golay24EncodeBytes,
    golay24DecodeBytes,
} from "./golay.cp";

// --- hamming ---
export {
    EccStatus,
    Hamming4Decode,
    Secded4Decode,
    Secded64Decode,
    hamming74Encode,
    hamming74Decode,
    secded84Encode,
    secded84Decode,
    secded84EncodeBytes,
    secded84DecodeBytes,
    secded7264Encode,
    secded7264Decode,
} from "./hamming.cp";

// --- reed_solomon ---
export {
    RsError,
    gfAdd,
    gfMul,
    gfPow,
    gfInv,
    gfDiv,
    ReedSolomon,
} from "./reed_solomon.cp";

// --- dct ---
export {
    DCT_BLOCK_DIM,
    DCT_BLOCK_SIZE,
    JPEG_ZIGZAG_8X8,
    JPEG_LUMA_QUANT_50,
    JPEG_CHROMA_QUANT_50,
    dct1d,
    idct1d,
    dct8x8,
    idct8x8,
    zigzagReorder8x8,
    unzigzagReorder8x8,
    quantize8x8,
    dequantize8x8,
    scaleQuantTable,
} from "./dct.cp";

