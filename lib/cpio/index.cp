// Origin: Pure-CP Stdlib (Zero-Alloc Unix CPIO newc/crc/odc Archive Parser) | Created: 2026-11-22

export {
    CPIO_MAGIC_NEWC,
    CPIO_MAGIC_CRC,
    CPIO_MAGIC_ODC,
    CPIO_NEWC_HEADER_SIZE,
    CPIO_ODC_HEADER_SIZE,
    CPIO_TRAILER,
    CpioFormat,
    CpioEntry,
    detectCpioFormat,
    isCpio,
    CpioIter,
    findCpioEntry,
    countCpioEntries,
} from "./cpio.cp";
