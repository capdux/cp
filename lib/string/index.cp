// Source: /data/dev/bun/src/string/
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/string/
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
    Utf8,
    Utf16,
    UnicodeWidth,
} from "./utf.cp";

export {
    StringBuilder,
} from "./builder.cp";

export {
    StringEscape,
} from "./escape.cp";

export * from "./grapheme.cp";
export * from "./case.cp";
export * from "./smol.cp";
export * from "./hashed.cp";
export * from "./path.cp";
export * from "./joiner.cp";
export * from "./wcwidth.cp";
