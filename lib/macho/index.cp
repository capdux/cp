// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Apple Mach-O & Fat Binary Parser)

export {
    MachoKind,
    CPU_TYPE_X86_64,
    CPU_TYPE_ARM64,
    MH_EXECUTE,
    MH_DYLIB,
    MachoHeader,
    MachoLoadCmd,
    FatArchEntry,
    detectMachoKind,
    parseMachoHeader,
    MachoLoadCmdIter,
    FatArchIter,
} from "./macho.cp";
