// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ICC Color Profile Parser)

export {
    ICC_HEADER_SIZE,
    ICC_TAG_TABLE_OFFSET,
    ICC_TAG_ENTRY_SIZE,
    IccHeader,
    IccTagEntry,
    iccTagEquals,
    isIccProfile,
    parseIccHeader,
    IccTagIter,
    findIccTag,
} from "./icc.cp";
