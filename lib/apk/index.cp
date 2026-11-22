// Origin: Pure-CP Stdlib (Zero-Alloc Android APK ZIP EOCD & Signing Block v2/v3 Parser) | Created: 2026-11-22

export {
    ZIP_EOCD_SIG,
    ZIP_EOCD_MIN_SIZE,
    ZIP_MAX_COMMENT_LEN,
    APK_SIG_BLOCK_MAGIC,
    APK_SIG_V2_BLOCK_ID,
    APK_SIG_V3_BLOCK_ID,
    APK_SIG_V3_1_BLOCK_ID,
    APK_VERITY_PADDING_BLOCK_ID,
    APK_DEPENDENCY_INFO_BLOCK_ID,
    ApkZipEocd,
    ApkSigningBlock,
    ApkSigPair,
    ApkSigSummary,
    findZipEocd,
    findApkSigningBlock,
    ApkSigPairIter,
    findApkSigPair,
    summarizeApkSigning,
} from "./apk.cp";
