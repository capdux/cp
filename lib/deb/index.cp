// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Debian .deb & RFC 822 Control Parser)

export {
    DEB_AR_MAGIC,
    DEB_BINARY_MEMBER,
    DebCompression,
    DebPackage,
    DebControlField,
    DebControlIter,
    DebControlMeta,
    detectDebCompression,
    isDeb,
    parseDebPackage,
    parseDebControl,
} from "./deb.cp";
