// Source: cp/lib/zlib/index.cp (Public Unified Export for cap:zlib & cap.zlib)

export {
    ZlibFormat,
    ZlibLevel,
    ZlibOptions,
    ZlibError,
} from "./types.cp";

export {
    crc32,
    adler32,
    detectFormat,
    compressRawDeflate,
    decompressRawDeflate,
    compressZlib,
    decompressZlib,
    compressGzip,
    decompressGzip,
    compress,
    decompress,
    gzipSync,
    gunzipSync,
    deflateSync,
    inflateSync,
    deflateRawSync,
    inflateRawSync,
} from "./codec.cp";
