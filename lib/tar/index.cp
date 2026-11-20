// Source: cp/lib/tar/index.cp (Unified Exports for cap:tar)
// Pure CP zero-OS-dependency POSIX ustar Archive Writer & Safe Reader.

export {
    TAR_BLOCK_SIZE,
    TarError,
    TarTypeFlag,
    TarEntry,
    TarBuilder,
    TarReader,
    parseArchive,
    freeEntries,
} from "./tar.cp";
