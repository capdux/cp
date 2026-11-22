// Origin: Pure-CP Stdlib (Zero-Alloc VMware VMDK SparseExtentHeader & Text Descriptor Parser) | Created: 2026-11-22

export {
    VMDK_MAGIC,
    VMDK_SECTOR_SIZE,
    VMDK_HEADER_SIZE,
    VMDK_DEFAULT_GTES_PER_GT,
    FLAG_VALID_NEWLINE_DETECTOR,
    FLAG_REDUNDANT_GRAIN_TABLE,
    FLAG_ZEROED_GRAIN_GTE,
    FLAG_COMPRESSED_GRAINS,
    FLAG_MARKERS_IN_STREAM,
    COMPRESS_NONE,
    COMPRESS_DEFLATE,
    CID_NONE,
    VmdkSparseHeader,
    isVmdkSparse,
    parseVmdkSparseHeader,
    VmdkExtent,
    parseVmdkExtentLine,
    VmdkExtentIter,
    VmdkDescriptor,
    parseVmdkDescriptor,
} from "./vmdk.cp";
