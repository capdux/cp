// Origin: Pure-CP Stdlib (Zero-Alloc yEnc v1.3 Binary-to-Text Codec, Header/Trailer Parser & CRC32) | Created: 2026-11-22

export {
    YENC_OFFSET,
    YENC_ESCAPE_OFFSET,
    YENC_ESCAPE_CHAR,
    YENC_DEFAULT_LINE_LEN,
    yencCrc32,
    yencEncode,
    yencDecode,
    YencBeginHeader,
    YencPartHeader,
    YencEndTrailer,
    parseYencBegin,
    parseYencPart,
    parseYencEnd,
} from "./yenc.cp";
