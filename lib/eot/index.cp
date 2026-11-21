// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Microsoft Embedded OpenType .eot Parser)
// Unified exports for cap:eot.

export {
    EOT_MAGIC,
    EOT_FIXED_HEADER_SIZE,
    EOT_VERSION_1,
    EOT_VERSION_2_1,
    EOT_VERSION_2_2,
    EOT_FLAG_COMPRESSED,
    EOT_FLAG_SUBSET,
    EOT_FLAG_XOR_ENCRYPTED,
    EotHeader,
    isEot,
    parseEotHeader,
    decodeUtf16LeName,
} from "./eot.cp";
