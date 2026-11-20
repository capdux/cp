// Source: cp/lib/exe_format/index.cp (Unified exports for cap:exe_format)
export {
    ExeKind,
    ExeArch,
    detectExeKind,
    archFromElfMachine,
    archFromPeMachine,
    archFromMachOCpu,
    readU16Le,
    readU32Le,
    readU64Le,
    writeU64Le,
} from "./types.cp";

export {
    ELF64_EHDR_SIZE,
    ELF64_SHDR_SIZE,
    Elf64Header,
    Elf64Section,
    parseElf64Header,
    listElf64Sections,
    findElf64Section,
} from "./elf.cp";

export {
    PE_SIGNATURE,
    DOS_SIGNATURE,
    PE32_MAGIC,
    PE32_PLUS_MAGIC,
    PE_SECTION_HEADER_SIZE,
    PeHeaderInfo,
    parsePeHeader,
} from "./pe.cp";

export {
    MH_MAGIC_64,
    MACH_HEADER_64_SIZE,
    MachO64HeaderInfo,
    parseMachO64Header,
} from "./macho.cp";

export {
    TRAILER_FOOTER_SIZE,
    StandaloneTrailer,
    computeChecksum64,
    appendStandaloneTrailer,
    extractStandaloneTrailer,
} from "./trailer.cp";
