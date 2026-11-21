// Source: Absorbed from /data/dev/bun/src/string/ | Synced: 2026-11-20T08:44:59Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/string/ & /data/dev/bun/src/unicode/
// Upstream Sync: Refer to /data/dev/bun/src/string/ for future upstream updates.
// ============================================================================
// lib/string/index.cp
// Unified Cap String & Unicode SIMD Subsystem (cap:string)

export {
    SimdScanner,
} from "./simd_scan.cp";

export {
    REPLACEMENT_CHAR,
    DecodeResult,
    EncodeIntoResult,
    Utf8,
    Utf16,
    Wtf8,
    UnicodeWidth,
    utf8ByteSequenceLength,
    decodeUTF8,
    encodeUTF8,
    elementLengthLatin1IntoUTF8,
    copyLatin1IntoUTF8,
    elementLengthUTF16IntoUTF8,
    copyUTF16IntoUTF8,
    convertUTF8toUTF16,
    convertUTF8toUTF16InBuffer,
    convertUTF16toUTF8,
    convertUTF16toUTF8InBuffer,
    withoutUTF8BOM,
} from "./utf.cp";

export {
    StringBuilder,
} from "./builder.cp";

export {
    MutableString,
} from "./mutable_string.cp";

export {
    StringEscape,
    escapeHtml,
    escapeHtmlAlloc,
    escapeRegExp,
    escapeRegExpAlloc,
    escapeJs,
    escapeJsAlloc,
} from "./escape.cp";

export * from "./unicode.cp";
export * from "./grapheme.cp";
export * from "./case.cp";
export * from "./smol.cp";
export * from "./hashed.cp";
export * from "./path.cp";
export * from "./joiner.cp";
export * from "./wcwidth.cp";
export * from "./utils.cp";
