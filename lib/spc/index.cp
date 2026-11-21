// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc SNES SPC700 .spc Audio Parser)
// Unified exports for cap:spc.

export {
    SPC_MAGIC,
    SPC_HEADER_SIZE,
    SPC_RAM_OFFSET,
    SPC_RAM_SIZE,
    SPC_DSP_OFFSET,
    SPC_DSP_SIZE,
    SPC_EXTRA_RAM_OFFSET,
    SPC_EXTRA_RAM_SIZE,
    SPC_MIN_FILE_SIZE,
    SPC_XID6_MAGIC,
    SpcId666Format,
    SpcRegs,
    SpcHeader,
    SpcXid6Item,
    SpcXid6Iter,
    isSpc,
    parseSpcHeader,
} from "./spc.cp";
