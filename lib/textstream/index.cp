// Source: Absorbed from /data/dev/bun/src/runtime/webcore/TextEncoderStreamEncoder.cp | Synced: 2026-11-22T00:18:28Z

export {
    UTF8_REPLACEMENT,
    isLeadSurrogate,
    isTrailSurrogate,
    combineSurrogatePair,
    encodeCodePointUtf8,
    utf8ByteLengthFromLatin1,
    utf8ByteLengthFromUtf16,
    TextEncoderStream,
} from "./textstream.cp";
