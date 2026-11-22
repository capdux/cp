// Origin: Pure-CP Stdlib (Zero-Alloc OpenEXR HDR Image Header & Attribute Parser) | Created: 2026-11-22

export {
    EXR_MAGIC,
    EXR_FLAG_TILED,
    EXR_FLAG_LONG_NAMES,
    EXR_FLAG_NON_IMAGE_PARTS,
    EXR_FLAG_MULTIPART,
    ExrCompression,
    ExrBox2i,
    ExrAttr,
    ExrHeader,
    compressionFromByte,
    isExr,
    parseExrBox2i,
    ExrAttrIter,
    parseExrHeader,
} from "./exr.cp";
