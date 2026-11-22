// Origin: Pure-CP Stdlib (Zero-Alloc HDF5 Superblock v0-v3 & OHDR v2 Parser) | Created: 2026-11-22

export {
    HDF5_SIGNATURE,
    HDF5_UNDEF_ADDR_64,
    HDF5_OHDR_MAGIC,
    HDF5_OHDR_FLAG_PHASE_CHANGE,
    HDF5_OHDR_FLAG_TIMESTAMPS,
    Hdf5Superblock,
    Hdf5ObjHeaderV2,
    findHdf5Signature,
    isHdf5,
    parseHdf5Superblock,
    parseHdf5ObjHeaderV2,
} from "./hdf5.cp";
