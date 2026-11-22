// Origin: Pure-CP Stdlib (Zero-Alloc Apple Mach-O & Fat Binary Parser) | Created: 2026-11-22

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
