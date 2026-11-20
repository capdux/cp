// Source: cp/lib/bencode/index.cp (Unified Exports for cap:bencode)
// Pure CP zero-C-dependency BitTorrent BEP-3 Bencode & Torrent InfoHash library.

export {
    BENCODE_MAX_DEPTH,
    BencodeError,
    BencodeKind,
    BencodeDictEntry,
    BencodeValue,
} from "./types.cp";

export {
    decode,
    decodeWithLimits,
    encode,
} from "./codec.cp";

export {
    extractRawSection,
    computeInfoHashSha1,
    formatInfoHashHex,
} from "./torrent.cp";
