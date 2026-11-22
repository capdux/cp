// Source: Absorbed from /data/dev/bun/src/sys/sys.zig | Synced: 2026-11-22T15:05:00Z
// ============================================================================
// lib/sys/index.cp — Cross-platform System Calls, Files, Stat & Process Controls
// ============================================================================

const std = @import("std");

export {
    SyscallTag,
    SysError,
    Maybe,
    EPERM,
    ENOENT,
    ESRCH,
    EINTR,
    EIO,
    ENXIO,
    E2BIG,
    ENOEXEC,
    EBADF,
    ECHILD,
    EAGAIN,
    ENOMEM,
    EACCES,
    EFAULT,
    EBUSY,
    EEXIST,
    EXDEV,
    ENODEV,
    ENOTDIR,
    EISDIR,
    EINVAL,
    ENFILE,
    EMFILE,
    ENOTTY,
    ETXTBSY,
    EFBIG,
    ENOSPC,
    ESPIPE,
    EROFS,
    EMLINK,
    EPIPE,
    ERANGE,
    ENAMETOOLONG,
    ENOSYS,
    ENOTEMPTY,
    ELOOP,
    EWOULDBLOCK,
    EOPNOTSUPP,
    ECONNRESET,
    ECONNREFUSED,
    ETIMEDOUT,
    ECANCELED,
    linuxSyscallErrno,
    cErrno,
    errnoToError,
} from "./error";

export {
    FD,
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT,
    O_EXCL,
    O_NOCTTY,
    O_TRUNC,
    O_APPEND,
    O_NONBLOCK,
    O_DSYNC,
    O_DIRECT,
    O_DIRECTORY,
    O_NOFOLLOW,
    O_CLOEXEC,
    O_SYNC,
    O_PATH,
    O_TMPFILE,
    F_OK,
    X_OK,
    W_OK,
    R_OK,
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
    AT_FDCWD,
    AT_SYMLINK_NOFOLLOW,
    AT_REMOVEDIR,
    AT_SYMLINK_FOLLOW,
    AT_EMPTY_PATH,
    LOCK_SH,
    LOCK_EX,
    LOCK_NB,
    LOCK_UN,
    F_DUPFD,
    F_GETFD,
    F_SETFD,
    F_GETFL,
    F_SETFL,
    F_DUPFD_CLOEXEC,
    FD_CLOEXEC,
} from "./fd";

export {
    FileKind,
    StatxField,
    kindFromMode,
    Stat,
    PosixStat,
    fstat,
    fstatMaybe,
} from "./fstat";

export {
    stat,
    lstat,
    fstatat,
    lstatat,
    statMaybe,
    lstatMaybe,
    fstatatMaybe,
    lstatatMaybe,
    statx,
} from "./stat";

export {
    File,
} from "./file";

export {
    DirEntryKind,
    DirEntry,
    DirEntryOpt,
    DirIterator,
    Dir,
} from "./dir";

export {
    CopyFileState,
    copyFile,
    copyFileByFd,
    copyFileWithState,
} from "./copyFile";

export {
    POLLIN,
    POLLPRI,
    POLLOUT,
    POLLERR,
    POLLHUP,
    POLLNVAL,
    PollFd,
    openat,
    openatMaybe,
    poll,
    copyFileRange,
    sendfile,
} from "./ioOps";

export {
    Signal,
    fromNumber,
    fromName,
} from "./signal";

export {
    getTempDir,
    makeTempFile,
    makeTempDir,
    removeFile,
    removeDir,
} from "./tmp";

export {
    RENAME_NOREPLACE,
    RENAME_EXCHANGE,
    EpollEvent,
    EPOLL_CTL_ADD,
    EPOLL_CTL_DEL,
    EPOLL_CTL_MOD,
    EPOLLIN,
    EPOLLOUT,
    EPOLLERR,
    EPOLLHUP,
    EPOLLRDHUP,
    EPOLLET,
    open,
    openMaybe,
    close,
    read,
    write,
    pread,
    pwrite,
    lseek,
    ftruncate,
    dupFd,
    dup2,
    mkdir,
    mkdirat,
    mkdiratMaybe,
    mkdirAll,
    mkdirAllAt,
    rmdir,
    rmdirat,
    rmdirRecursive,
    rmdirRecursiveAt,
    unlink,
    unlinkat,
    unlinkatMaybe,
    rename,
    renameat,
    renameat2,
    renameat2Maybe,
    symlink,
    symlinkat,
    readlink,
    readlinkat,
    link,
    linkat,
    chmod,
    fchmod,
    chown,
    fchown,
    fsync,
    fdatasync,
    flock,
    fcntl,
    ioctl,
    createPipe,
    pipe2,
    epollCreate,
    epollCtl,
    epollWait,
    access,
    exists,
    getcwd,
    chdir,
    readFile,
    writeFile,
} from "./fsOps";

export {
    WNOHANG,
    WUNTRACED,
    WCONTINUED,
    UnameInfo,
    WaitPidResult,
    SpawnOptions,
    PosixSpawn,
    getpid,
    getppid,
    getuid,
    getgid,
    uname,
    pageSize,
    cpuCount,
    monotonicNs,
    realtimeMs,
    waitpid,
    waitpidMaybe,
    kill,
    killMaybe,
} from "./process";

export {
    GroupEntry,
    GroupMemberIter,
    EtcGroupIter,
    findGroupByName,
    findGroupByGid,
    isUserInGroup,
} from "./group";

export {
    ProtocolEntry,
    EtcProtocolsIter,
    findProtocolNumber,
    findProtocolByNumber,
} from "./protocols";

export {
    ServiceEntry,
    EtcServicesIter,
    findServicePort,
    findServiceByPort,
} from "./services";

export {
    parseShellLine,
    EtcShellsIter,
    isAllowedShell,
    findShellByBasename,
    countShells,
    isNologinShell,
} from "./shells";

export {
    NulArgIter,
    countArgs,
    argAt,
    ProcEnvEntry,
    ProcEnvIter,
    findProcEnv,
    formatCmdlineDisplay,
} from "./cmdline";

export {
    CpuBlock,
    CpuSummary,
    ProcCpuInfoIter,
    summarizeCpuInfo,
    hasCpuFlag,
} from "./cpuinfo";

export {
    ProcIoStats,
    parseProcIo,
} from "./procio";

export {
    LimitValue,
    ProcLimitEntry,
    ProcLimitsIter,
    parseLimitValue,
    findProcLimit,
    maxOpenFiles,
} from "./limits";

export {
    LoadAvg,
    SystemUptime,
} from "./loadavg";

export {
    MemInfoField,
    MemInfoIter,
    MemInfo,
} from "./meminfo";

export {
    MountEntry,
    ProcMountsIter,
    unescapeMountPath,
    findMountForPath,
} from "./mounts";

export {
    NetDevStats,
    ProcNetDevIter,
    findNetDev,
    sumNonLoopbackBytes,
} from "./netdev";

export {
    RTF_UP,
    RTF_GATEWAY,
    RTF_HOST,
    parseHexIpv4Le,
    RouteEntry,
    ProcRouteIter,
    findDefaultGateway,
} from "./route";

export {
    ProcStatus,
    parseProcStatus,
    findProcStatusField,
} from "./procstatus";

export {
    ProcState,
    parseProcState,
    ProcStat,
} from "./procstat";

export {
    ProcStatm,
    parseProcStatm,
} from "./statm";

export {
    ProcUptime,
    parseProcUptime,
} from "./uptime";

export {
    CgroupLimit,
    CpuMaxQuota,
    IoMaxEntry,
    parseCgroupMax,
    parseCpuMax,
    parseIoMaxLine,
    effectiveMemoryLimit,
} from "./cgroup";

export {
    SysctlEntry,
    SysctlIter,
    normalizeSysctlKey,
    sysctlKeysEqual,
    getSysctlValue,
} from "./sysctl";

export {
    FstabEntry,
    MountIter,
    unescapeFstabPath,
    findFstabForPath,
} from "./fstab";

export {
    OsReleaseField,
    OsReleaseIter,
    getOsReleaseField,
    isOsLike,
} from "./osrelease";

export {
    PasswdEntry,
    PasswdIter,
    lookupUserByName,
    lookupUserByUid,
    GroupEntry as PasswdGroupEntry,
    GroupIter,
    lookupGroupByName,
    lookupGroupByGid,
} from "./passwd";

export {
    NameserverIter,
    ResolvOptions,
    effectiveSearchRaw,
    firstNameserver,
} from "./resolv";

export {
    UnitEntry,
    UnitIter,
    getUnitValue,
    countSectionKey,
} from "./systemd";

export {
    DEFAULT_CONFIG_DIRS,
    DEFAULT_DATA_DIRS,
    XdgEnv,
    XdgDirIter,
    resolveConfigHome,
    resolveDataHome,
    resolveCacheHome,
    resolveStateHome,
    resolveRuntimeDir,
} from "./xdg";

import { pipe2 as sysPipe2, createPipe as sysCreatePipe } from "./fsOps";

pub struct Sys {
    static fn dup(fd: FD): !FD {
        return fd.dup();
    }

    static fn dup2(old_fd: FD, new_fd: FD): !FD {
        return old_fd.dup2(new_fd);
    }

    static fn pipe(): ![2]FD {
        return sysCreatePipe();
    }

    static fn pipe2(flags: i32): ![2]FD {
        return sysPipe2(flags);
    }

    /// Bind the current thread to a specific CPU core using Zig 0.17 std.os.linux.
    static fn setAffinity(core_id: usize): !void {
        zig {
            var cpu_set = std.mem.zeroes(std.os.linux.cpu_set_t);
            const word_idx = core_id / @bitSizeOf(usize);
            const bit_idx: u6 = @intCast(core_id % @bitSizeOf(usize));
            if (word_idx < cpu_set.len) {
                cpu_set[word_idx] |= (@as(usize, 1) << bit_idx);
            }
            const tid: std.os.linux.pid_t = @intCast(std.os.linux.syscall0(.gettid));
            try std.os.linux.sched_setaffinity(tid, &cpu_set);
        }
    }

    /// Load a dynamic library (e.g., .so/.dylib).
    pub struct Lib {
        handle: std.DynLib,

        static fn open(path: []const u8): !Lib {
            const lib = try std.DynLib.open(path);
            return Lib{ handle: lib };
        }

        fn deinit(*_) {
            _.handle.close();
        }

        fn lookup(*_, comptime T: type, symbol: []const u8): T {
            return _.handle.lookup(T, symbol) orelse @panic("Failed to find symbol");
        }
    }
}

pub const sys = Sys;

// === Consolidated Kernel/Firmware, Filesystems, Virtual Disks & RingBuffer ===

// --- bpf ---
export {
    BPF_INSN_SIZE,
    BPF_WIDE_INSN_SIZE,
    BPF_MAX_REG,
    BPF_LD,
    BPF_LDX,
    BPF_ST,
    BPF_STX,
    BPF_ALU,
    BPF_JMP,
    BPF_JMP32,
    BPF_ALU64,
    BPF_K,
    BPF_X,
    BPF_W,
    BPF_H,
    BPF_B,
    BPF_DW,
    BPF_IMM,
    BPF_MEM,
    BPF_ATOMIC,
    ALU_ADD,
    ALU_SUB,
    ALU_MUL,
    ALU_DIV,
    ALU_OR,
    ALU_AND,
    ALU_LSH,
    ALU_RSH,
    ALU_NEG,
    ALU_MOD,
    ALU_XOR,
    ALU_MOV,
    ALU_ARSH,
    ALU_END,
    JMP_JA,
    JMP_JEQ,
    JMP_JGT,
    JMP_JGE,
    JMP_JSET,
    JMP_JNE,
    JMP_JSGT,
    JMP_JSGE,
    JMP_CALL,
    JMP_EXIT,
    JMP_JLT,
    JMP_JLE,
    JMP_JSLT,
    JMP_JSLE,
    OP_LD_IMM64,
    OP_CALL,
    OP_EXIT,
    BpfInsn,
    decodeBpfInsn,
    encodeBpfInsn,
    encodeLdImm64,
    insnMov64Imm,
    insnMov64Reg,
    insnAlu64Reg,
    insnCall,
    insnExit,
    BpfProgIter,
    validateBpfProgram,
} from "./bpf.cp";

// --- acpi ---
export {
    RSDP_SIGNATURE,
    RSDP_V1_SIZE,
    RSDP_V2_SIZE,
    SDT_HEADER_SIZE,
    SIG_RSDT,
    SIG_XSDT,
    SIG_FACP,
    SIG_APIC,
    SIG_MCFG,
    SIG_HPET,
    SIG_DSDT,
    MADT_LOCAL_APIC,
    MADT_IO_APIC,
    MADT_INT_SRC_OVERRIDE,
    MADT_NMI_SOURCE,
    MADT_LOCAL_APIC_NMI,
    MADT_LOCAL_APIC_ADDR_OVERRIDE,
    MADT_LOCAL_X2APIC,
    acpiChecksum,
    verifyAcpiChecksum,
    AcpiRsdp,
    parseAcpiRsdp,
    AcpiSdtHeader,
    AcpiTable,
    parseAcpiTable,
    rsdtEntryCount,
    rsdtEntryAt,
    xsdtEntryCount,
    xsdtEntryAt,
    MadtEntry,
    MadtEntryIter,
    MadtTable,
    parseMadt,
    MadtLocalApic,
    parseMadtLocalApic,
    MadtIoApic,
    parseMadtIoApic,
    MadtIntOverride,
    parseMadtIntOverride,
    McfgEntry,
    McfgEntryIter,
    parseMcfgEntries,
} from "./acpi.cp";

// --- smbios ---
export {
    SMBIOS2_ANCHOR,
    SMBIOS2_DMI_ANCHOR,
    SMBIOS3_ANCHOR,
    SMBIOS2_EP_SIZE,
    SMBIOS3_EP_SIZE,
    SMBIOS_STRUCT_HEADER_SIZE,
    TYPE_BIOS,
    TYPE_SYSTEM,
    TYPE_BASEBOARD,
    TYPE_CHASSIS,
    TYPE_PROCESSOR,
    TYPE_MEMORY_DEVICE,
    TYPE_END_OF_TABLE,
    smbiosChecksum,
    Smbios2EntryPoint,
    parseSmbios2Entry,
    Smbios3EntryPoint,
    parseSmbios3Entry,
    SmbiosStruct,
    SmbiosTableIter,
    SmbiosBiosInfo,
    parseBiosInfo,
    SmbiosSystemInfo,
    parseSystemInfo,
    SmbiosMemoryDevice,
    parseMemoryDevice,
} from "./smbios.cp";

// --- fdt ---
export {
    FDT_MAGIC,
    FDT_HEADER_SIZE,
    FDT_VERSION,
    FDT_BEGIN_NODE,
    FDT_END_NODE,
    FDT_PROP,
    FDT_NOP,
    FDT_END,
    FdtHeader,
    FdtMemRsv,
    FdtMemRsvIter,
    FdtTokenKind,
    FdtToken,
    FdtTokenIter,
    FdtBlob,
    isFdt,
    parseFdt,
} from "./fdt.cp";

// --- pci ---
export {
    PCI_CONFIG_SIZE,
    PCIE_EXT_CONFIG_SIZE,
    PCI_HEADER_TYPE_ENDPOINT,
    PCI_HEADER_TYPE_BRIDGE,
    PCI_HEADER_MULTIFUNC_BIT,
    PCI_STATUS_CAP_LIST,
    CAP_PM,
    CAP_MSI,
    CAP_VENDOR,
    CAP_PCIE,
    CAP_MSIX,
    EXT_CAP_AER,
    EXT_CAP_ACS,
    EXT_CAP_SRIOV,
    EXT_CAP_RESIZABLE_BAR,
    CLASS_MASS_STORAGE,
    CLASS_NETWORK,
    CLASS_DISPLAY,
    CLASS_BRIDGE,
    CLASS_SERIAL_BUS,
    PciHeader,
    parsePciHeader,
    PciBar,
    decodePciBar,
    PciCap,
    PciCapIter,
    iterPciCaps,
    findPciCap,
    PcieExtCap,
    PcieExtCapIter,
    iterPcieExtCaps,
    findPcieExtCap,
} from "./pci.cp";

// --- hid ---
export {
    ITEM_TYPE_MAIN,
    ITEM_TYPE_GLOBAL,
    ITEM_TYPE_LOCAL,
    ITEM_LONG_PREFIX,
    MAIN_INPUT,
    MAIN_OUTPUT,
    MAIN_COLLECTION,
    MAIN_FEATURE,
    MAIN_END_COLLECTION,
    GLOBAL_USAGE_PAGE,
    GLOBAL_LOGICAL_MIN,
    GLOBAL_LOGICAL_MAX,
    GLOBAL_PHYSICAL_MIN,
    GLOBAL_PHYSICAL_MAX,
    GLOBAL_UNIT_EXPONENT,
    GLOBAL_UNIT,
    GLOBAL_REPORT_SIZE,
    GLOBAL_REPORT_ID,
    GLOBAL_REPORT_COUNT,
    GLOBAL_PUSH,
    GLOBAL_POP,
    LOCAL_USAGE,
    LOCAL_USAGE_MIN,
    LOCAL_USAGE_MAX,
    COLL_PHYSICAL,
    COLL_APPLICATION,
    COLL_LOGICAL,
    USAGE_PAGE_GENERIC_DESKTOP,
    USAGE_PAGE_KEYBOARD,
    USAGE_PAGE_LED,
    USAGE_PAGE_BUTTON,
    USAGE_PAGE_CONSUMER,
    FLAG_CONSTANT,
    FLAG_VARIABLE,
    FLAG_RELATIVE,
    HidItem,
    HidItemIter,
    HidReportSummary,
    summarizeHidDescriptor,
} from "./hid.cp";

// --- edid ---
export {
    EDID_BLOCK_SIZE,
    EDID_HEADER_MAGIC,
    DESC_SERIAL,
    DESC_UNSPECIFIED_TEXT,
    DESC_RANGE_LIMITS,
    DESC_MONITOR_NAME,
    DESC_DUMMY,
    CEA_EXT_TAG,
    edidChecksum,
    verifyEdidChecksum,
    decodePnpId,
    encodePnpId,
    EdidDetailedTiming,
    EdidDescriptor,
    EdidStandardTiming,
    EdidInfo,
    isEdid,
    parseEdid,
} from "./edid.cp";

// --- gpt ---
export {
    GPT_SIGNATURE,
    GPT_REVISION_1_0,
    GPT_MIN_HEADER_SIZE,
    GPT_MIN_ENTRY_SIZE,
    MBR_SECTOR_SIZE,
    MBR_BOOT_SIGNATURE,
    MBR_TYPE_GPT_PROTECTIVE,
    GUID_UNUSED,
    GUID_EFI_SYSTEM,
    GUID_LINUX_FS,
    GUID_LINUX_SWAP,
    GUID_MS_BASIC_DATA,
    gptCrc32,
    formatGuid,
    MbrPartition,
    MbrPartitionTable,
    parseMbrPartitions,
    isProtectiveMbr,
    GptHeader,
    isGptHeader,
    parseGptHeader,
    verifyGptEntriesCrc,
    GptEntry,
    GptEntryIter,
} from "./gpt.cp";

// --- ext4 ---
export {
    EXT4_SUPER_OFFSET,
    EXT4_SUPER_SIZE,
    EXT4_SUPER_MAGIC,
    EXT4_EXTENT_MAGIC,
    EXT4_ROOT_INO,
    COMPAT_HAS_JOURNAL,
    INCOMPAT_FILETYPE,
    INCOMPAT_EXTENTS,
    INCOMPAT_64BIT,
    RO_COMPAT_SPARSE_SUPER,
    RO_COMPAT_LARGE_FILE,
    RO_COMPAT_METADATA_CSUM,
    EXT4_EXTENTS_FL,
    S_IFMT,
    S_IFREG,
    S_IFDIR,
    S_IFLNK,
    FT_UNKNOWN,
    FT_REG_FILE,
    FT_DIR,
    FT_CHRDEV,
    FT_BLKDEV,
    FT_FIFO,
    FT_SOCK,
    FT_SYMLINK,
    Ext4Superblock,
    isExt4Superblock,
    parseExt4Superblock,
    Ext4Inode,
    parseExt4Inode,
    Ext4ExtentHeader,
    parseExt4ExtentHeader,
    Ext4Extent,
    Ext4ExtentIter,
    iterExt4Extents,
    Ext4DirEntry,
    Ext4DirIter,
} from "./ext4.cp";

// --- btrfs ---
export {
    BTRFS_SUPER_OFFSET,
    BTRFS_SUPER_SIZE,
    BTRFS_MAGIC,
    BTRFS_TREE_HEADER_SIZE,
    BTRFS_KEY_SIZE,
    BTRFS_SYS_CHUNK_ARRAY_OFFSET,
    BTRFS_SYS_CHUNK_ARRAY_MAX,
    BTRFS_CHUNK_FIXED_SIZE,
    BTRFS_STRIPE_SIZE,
    CSUM_CRC32C,
    CSUM_XXHASH64,
    CSUM_SHA256,
    CSUM_BLAKE2B,
    KEY_INODE_ITEM,
    KEY_DIR_ITEM,
    KEY_EXTENT_DATA,
    KEY_ROOT_ITEM,
    KEY_CHUNK_ITEM,
    BLOCK_GROUP_DATA,
    BLOCK_GROUP_SYSTEM,
    BLOCK_GROUP_METADATA,
    BLOCK_GROUP_RAID0,
    BLOCK_GROUP_RAID1,
    BLOCK_GROUP_DUP,
    BLOCK_GROUP_RAID10,
    BLOCK_GROUP_RAID5,
    BLOCK_GROUP_RAID6,
    btrfsCrc32c,
    BtrfsKey,
    BtrfsSysChunk,
    BtrfsSysChunkIter,
    BtrfsSuperblock,
    BtrfsTreeHeader,
    isBtrfsSuperblock,
    parseBtrfsSuperblock,
    parseBtrfsKey,
    parseBtrfsTreeHeader,
} from "./btrfs.cp";

// --- xfs ---
pub const _xfs = @import("./xfs.cp");
pub const XFS_S_IFMT = _xfs.S_IFMT;
pub const XFS_S_IFREG = _xfs.S_IFREG;
pub const XFS_S_IFDIR = _xfs.S_IFDIR;
export {
    XFS_SB_MAGIC,
    XFS_SB_MIN_SIZE,
    XFS_SB_SECTOR_SIZE,
    XFS_INODE_MAGIC,
    XFS_INODE_CORE_SIZE,
    XFS_AGF_MAGIC,
    XFS_AGI_MAGIC,
    XFS_SB_CRC_OFFSET,
    XFS_BMBT_REC_SIZE,
    XFS_SB_VERSION_4,
    XFS_SB_VERSION_5,
    FMT_DEV,
    FMT_LOCAL,
    FMT_EXTENTS,
    FMT_BTREE,
    RO_COMPAT_FINOBT,
    RO_COMPAT_RMAPBT,
    RO_COMPAT_REFLINK,
    INCOMPAT_FTYPE,
    INCOMPAT_BIGTIME,

    xfsCrc32c,
    xfsCrc32cWithZeroField,
    XfsSuperblock,
    XfsInodeCore,
    XfsBmbtRec,
    isXfsSuperblock,
    parseXfsSuperblock,
    parseXfsInodeCore,
    decodeXfsBmbtRec,
    encodeXfsBmbtRec,
} from "./xfs.cp";

// --- zfs ---
export {
    ZFS_UBERBLOCK_MAGIC,
    ZFS_UBERBLOCK_MAGIC_SWAPPED,
    ZFS_UBERBLOCK_SIZE,
    ZFS_BLKPTR_SIZE,
    ZFS_SECTOR_SHIFT,
    ZFS_VDEV_LABEL_SIZE,
    ZFS_VDEV_BOOT_OFFSET,
    COMP_INHERIT,
    COMP_ON,
    COMP_OFF,
    COMP_LZJB,
    COMP_EMPTY,
    COMP_GZIP_1,
    COMP_LZ4,
    COMP_ZSTD,
    CKSUM_INHERIT,
    CKSUM_ON,
    CKSUM_OFF,
    CKSUM_FLETCHER_2,
    CKSUM_FLETCHER_4,
    CKSUM_SHA256,
    CKSUM_SHA512,
    CKSUM_SKEIN,
    CKSUM_EDONR,
    CKSUM_BLAKE3,
    ZfsChecksum,
    zfsFletcher4Le,
    zfsFletcher4Be,
    ZfsDva,
    ZfsBlkPtr,
    ZfsUberblock,
    parseZfsBlkPtr,
    isZfsUberblock,
    parseZfsUberblock,
    selectActiveUberblock,
} from "./zfs.cp";

// --- fat ---
export {
    FAT_BOOT_SECTOR_SIZE,
    FAT_BOOT_SIGNATURE,
    FAT_DIR_ENTRY_SIZE,
    ATTR_READ_ONLY,
    ATTR_HIDDEN,
    ATTR_SYSTEM,
    ATTR_VOLUME_ID,
    ATTR_DIRECTORY,
    ATTR_ARCHIVE,
    ATTR_LFN,
    FatKind,
    FatDateTime,
    decodeDosDateTime,
    fatShortNameChecksum,
    FatBpb,
    isFatBootSector,
    parseFatBpb,
    FatDirEntry,
    FatDirIter,
} from "./fat.cp";

// --- exfat ---
pub const _exfat = @import("./exfat.cp");
pub const EXFAT_ATTR_READ_ONLY = _exfat.ATTR_READ_ONLY;
pub const EXFAT_ATTR_HIDDEN = _exfat.ATTR_HIDDEN;
pub const EXFAT_ATTR_SYSTEM = _exfat.ATTR_SYSTEM;
pub const EXFAT_ATTR_DIRECTORY = _exfat.ATTR_DIRECTORY;
pub const EXFAT_ATTR_ARCHIVE = _exfat.ATTR_ARCHIVE;
export {
    EXFAT_OEM_ID,
    EXFAT_BOOT_SIGNATURE,
    EXFAT_MIN_BOOT_SIZE,
    EXFAT_DIR_ENTRY_SIZE,
    ENTRY_EOD,
    ENTRY_ALLOC_BITMAP,
    ENTRY_UPCASE_TABLE,
    ENTRY_VOLUME_LABEL,
    ENTRY_FILE,
    ENTRY_STREAM_EXT,
    ENTRY_FILE_NAME,

    STREAM_ALLOC_POSSIBLE,
    STREAM_NO_FAT_CHAIN,
    VOLUME_FLAG_ACTIVE_FAT,
    VOLUME_FLAG_DIRTY,
    VOLUME_FLAG_MEDIA_FAILURE,
    VOLUME_FLAG_CLEAR_TO_ZERO,
    ExfatBootSector,
    isExfatBootSector,
    parseExfatBootSector,
    exfatBootChecksum,
    exfatEntrySetChecksum,
    ExfatFileEntry,
    parseFileEntry,
    ExfatStreamExt,
    parseStreamExt,
    decodeFileNameAscii,
    ExfatRawEntry,
    ExfatDirIter,
} from "./exfat.cp";

// --- ntfs ---
export {
    NTFS_VBR_SIZE,
    NTFS_OEM_ID,
    NTFS_BOOT_SIGNATURE,
    NTFS_FILE_MAGIC,
    NTFS_ATTR_END,
    MFT_FLAG_IN_USE,
    MFT_FLAG_DIRECTORY,
    ATTR_STANDARD_INFO,
    ATTR_ATTR_LIST,
    ATTR_FILE_NAME,
    ATTR_VOLUME_NAME,
    ATTR_VOLUME_INFO,
    ATTR_DATA,
    ATTR_INDEX_ROOT,
    ATTR_INDEX_ALLOC,
    ATTR_BITMAP,
    ATTR_REPARSE_POINT,
    NtfsVbr,
    isNtfsVbr,
    parseNtfsVbr,
    NtfsDataRun,
    NtfsRunIter,
    NtfsAttr,
    NtfsAttrIter,
    NtfsMftRecord,
    parseNtfsMftRecord,
} from "./ntfs.cp";

// --- apfs ---
export {
    APFS_OBJ_HEADER_SIZE,
    APFS_NX_MAGIC,
    APFS_VOL_MAGIC,
    APFS_MIN_BLOCK_SIZE,
    APFS_MAX_FILE_SYSTEMS,
    OBJ_TYPE_NX_SUPERBLOCK,
    OBJ_TYPE_BTREE,
    OBJ_TYPE_BTREE_NODE,
    OBJ_TYPE_SPACEMAN,
    OBJ_TYPE_OMAP,
    OBJ_TYPE_CHECKPOINT_MAP,
    OBJ_TYPE_FS,
    OBJ_VIRTUAL,
    OBJ_PHYSICAL,
    OBJ_EPHEMERAL,
    APFS_FS_UNENCRYPTED,
    APFS_FS_ONEKEY,
    APFS_FS_CASE_INSENSITIVE,
    APFS_FS_CASE_SENSITIVE,
    apfsFletcher64,
    verifyApfsChecksum,
    ApfsObjHeader,
    ApfsContainerSuper,
    ApfsVolumeSuper,
    parseApfsObjHeader,
    isApfsContainer,
    parseApfsContainer,
    isApfsVolume,
    parseApfsVolume,
} from "./apfs.cp";

// --- qcow2 ---
export {
    QCOW2_MAGIC,
    QCOW2_V2_HEADER_SIZE,
    QCOW2_V3_MIN_HEADER_SIZE,
    QCOW2_MIN_CLUSTER_BITS,
    QCOW2_MAX_CLUSTER_BITS,
    CRYPT_NONE,
    CRYPT_AES,
    CRYPT_LUKS,
    INCOMPAT_DIRTY,
    INCOMPAT_CORRUPT,
    INCOMPAT_EXTERNAL_DATA,
    INCOMPAT_COMPRESSION_TYPE,
    INCOMPAT_EXTENDED_L2,
    EXT_END,
    EXT_BACKING_FORMAT,
    EXT_FEATURE_NAME_TABLE,
    EXT_BITMAPS,
    EXT_FULL_DISK_ENCRYPTION,
    Qcow2Header,
    isQcow2,
    parseQcow2Header,
    Qcow2Ext,
    Qcow2ExtIter,
    iterQcow2Exts,
    Qcow2L1Entry,
    decodeL1Entry,
    Qcow2L2Entry,
    decodeL2Entry,
} from "./qcow2.cp";

// --- vhdx ---
export {
    VHDX_FILE_ID_SIG,
    VHDX_HEADER_SIG,
    VHDX_REGION_SIG,
    VHDX_METADATA_SIG,
    VHDX_HEADER_SIZE,
    VHDX_REGION_SIZE,
    VHDX_HEADER1_OFFSET,
    VHDX_HEADER2_OFFSET,
    VHDX_REGION1_OFFSET,
    VHDX_REGION2_OFFSET,
    VHDX_FILE_ID_CREATOR_OFFSET,
    VHDX_FILE_ID_CREATOR_LEN,
    VHDX_MAX_REGION_ENTRIES,
    VHDX_REGION_ENTRY_SIZE,
    REGION_GUID_BAT,
    REGION_GUID_METADATA,
    BAT_NOT_PRESENT,
    BAT_UNDEFINED,
    BAT_ZERO,
    BAT_UNMAPPED,
    BAT_FULLY_PRESENT,
    BAT_PARTIALLY_PRESENT,
    vhdxCrc32c,
    vhdxCrc32cWithZeroField,
    VhdxFileId,
    VhdxHeader,
    VhdxRegionEntry,
    VhdxRegionIter,
    VhdxRegionTable,
    VhdxBatEntry,
    isVhdx,
    parseVhdxFileId,
    parseVhdxHeader,
    selectActiveHeader,
    parseVhdxRegionTable,
    decodeVhdxBatEntry,
} from "./vhdx.cp";

// --- scsi ---
export {
    OP_TEST_UNIT_READY,
    OP_REQUEST_SENSE,
    OP_INQUIRY,
    OP_READ_CAPACITY_10,
    OP_READ_10,
    OP_WRITE_10,
    OP_SYNCHRONIZE_CACHE_10,
    OP_READ_16,
    OP_WRITE_16,
    OP_SERVICE_ACTION_IN_16,
    STATUS_GOOD,
    STATUS_CHECK_CONDITION,
    STATUS_BUSY,
    STATUS_RESERVATION_CONFLICT,
    STATUS_TASK_SET_FULL,
    SK_NO_SENSE,
    SK_RECOVERED_ERROR,
    SK_NOT_READY,
    SK_MEDIUM_ERROR,
    SK_HARDWARE_ERROR,
    SK_ILLEGAL_REQUEST,
    SK_UNIT_ATTENTION,
    SK_DATA_PROTECT,
    SK_ABORTED_COMMAND,
    PDT_DIRECT_ACCESS,
    PDT_SEQUENTIAL_ACCESS,
    PDT_CD_DVD,
    PDT_ENCLOSURE,
    cdbLengthForOpcode,
    ScsiInquiryData,
    parseInquiry,
    ScsiCapacity,
    parseReadCapacity10,
    parseReadCapacity16,
    ScsiSense,
    parseSense,
    encodeInquiryCdb,
    encodeRead10Cdb,
    encodeWrite10Cdb,
    encodeRead16Cdb,
    encodeWrite16Cdb,
} from "./scsi.cp";

// --- ringbuf ---
export {
    RingError,
    OverflowPolicy,
    RingBuffer,
    ByteRing,
    MicroBatcher,
} from "./ringbuf.cp";

// ============================================================================
// Batch 40: Application Architecture & Service Orchestration (di, pipeline, pubsub)
// ============================================================================
export {
    MAX_RESOLUTION_DEPTH,
    DiError,
    ServiceLifetime,
    DestructorFn,
    FactoryFn,
    ServiceInstance,
    ServiceContainer,
    ServiceScope,
} from "./di.cp";

export {
    PipelineError,
    StageOutcome,
    StageTrace,
    PipelineContext,
    StageFn,
    ErrorHookFn,
    PredicateFn,
    StageSpec,
    Pipeline,
} from "./pipeline.cp";

export {
    TopicStyle,
    TopicMatcher,
} from "./pubsubTopic.cp";

export {
    PubSubError,
    DropPolicy,
    BrokerMessage,
    BrokerStats,
    PubSubBroker,
} from "./pubsubBroker.cp";

// --- byte_ring (Power-of-Two Bitmask Batch Ring Byte & Framed Queue) ---
export {
    ByteRingStats,
    ByteRingQueue,
} from "./byteRing.cp";

// --- gen_slab (ABA-Safe O(1) Generational Slab Object Handle Pool) ---
export {
    GEN_SLAB_MAX_SLOTS,
    GEN_SLAB_NO_FREE,
    GenHandle,
    GenSlabStats,
    GenSlabPool,
} from "./genSlab.cp";

// --- proc_supervisor (Zero-Alloc Process / Sidecar Supervisor & Health-Probe FSM) ---
export {
    PROC_SUP_MAX_SLOTS,
    PROC_SUP_WINDOW_HISTORY,
    PROC_SUP_INVALID_SLOT,
    ProcSupState,
    ProcSupActionKind,
    ProcSupRestartPolicy,
    ProcSupStatus,
    ProcSupConfig,
    ProcSupSlot,
    ProcSupAction,
    ProcSupStats,
    ProcSupervisor,
} from "./supervisor.cp";

// --- flightLog (Zero-Alloc Black-Box Binary Flight-Recorder Event Ring Buffer) ---
export {
    FLIGHT_LOG_RING_BYTES,
    FLIGHT_LOG_MAX_PAYLOAD,
    FLIGHT_LOG_RECORD_HEADER_SIZE,
    FLIGHT_LOG_DUMP_MAGIC,
    FLIGHT_LOG_DUMP_HEADER_SIZE,
    FLIGHT_LOG_DUMP_TRAILER_SIZE,
    FlightSeverity,
    FlightRecordStatus,
    FlightDumpStatus,
    FlightEvent,
    FlightDumpMeta,
    FlightLogStats,
    FlightRecorder,
    verifyBinaryDump,
    parseBinaryDump,
} from "./flightLog.cp";

// --- workQueue (Zero-Alloc Multi-Priority Aging Work Queue & Backpressure Scheduler) ---
export {
    WORK_QUEUE_MAX_CAPACITY,
    WORK_QUEUE_MAX_PRIORITY,
    WorkBackpressurePolicy,
    WorkEnqueueStatus,
    WorkQueueConfig,
    WorkTask,
    WorkEnqueueOutcome,
    WorkDequeueItem,
    WorkQueueStats,
    WorkQueue,
} from "./workQueue.cp";

// --- statFs (Zero-Alloc POSIX / Node fs.statfs Filesystem Capacity & Inode Model) ---
export {
    FS_MAGIC_EXT4,
    FS_MAGIC_TMPFS,
    FS_MAGIC_BTRFS,
    FS_MAGIC_XFS,
    FS_MAGIC_OVERLAYFS,
    FS_MAGIC_NFS,
    FS_MAGIC_PROC,
    FS_MAGIC_SYSFS,
    FS_MAGIC_ZFS,
    StatFsError,
    statFsTypeName,
    StatFsSmall,
    StatFsBig,
} from "./statFs.cp";

// --- libuvErrorMap (Zero-Alloc libuv / Node.js System & DNS Error Code Map) ---
export {
    LIBUV_ERROR_COUNT,
    LIBUV_UNKNOWN_MESSAGE,
    LibuvErrorCategory,
    LibuvErrno,
    LibuvErrorEntry,
    LibuvErrorMap,
} from "./libuvErrorMap.cp";

// --- fsConstants (Zero-Alloc Node.js fs.constants, CopyfileFlags & POSIX FileMode) ---
export {
    COPYFILE_EXCL,
    COPYFILE_FICLONE,
    COPYFILE_FICLONE_FORCE,
    COPYFILE_VALID_MASK,
    UV_FS_O_FILEMAP,
    FS_IFMT,
    FS_IFIFO,
    FS_IFCHR,
    FS_IFDIR,
    FS_IFBLK,
    FS_IFREG,
    FS_IFLNK,
    FS_IFSOCK,
    S_ISUID,
    S_ISGID,
    S_ISVTX,
    S_IRWXU,
    S_IRUSR,
    S_IWUSR,
    S_IXUSR,
    S_IRWXG,
    S_IRGRP,
    S_IWGRP,
    S_IXGRP,
    S_IRWXO,
    S_IROTH,
    S_IWOTH,
    S_IXOTH,
    FS_PERM_MASK,
    CopyfileFlags,
    FileMode,
} from "./fsConstants.cp";

// --- bulkhead (Zero-Alloc Multi-Tenant Bulkhead Isolation & AIMD Adaptive Concurrency Guard) ---
export {
    BULKHEAD_MAX_COMPARTMENTS,
    BULKHEAD_MAX_QUEUE_PER_COMP,
    BulkheadAcquireStatus,
    BulkheadConfig,
    BulkheadAcquireResult,
    BulkheadReleaseResult,
    BulkheadCompartmentSnapshot,
    BulkheadGlobalStats,
    BulkheadGuard,
} from "./bulkhead.cp";

// --- walkerSkip (Zero-Alloc Hash-Accelerated Directory Walker Skip Filter & Path Stack) ---
export {
    WALKER_MAX_SKIP_RULES,
    WALKER_MAX_DEPTH,
    WALKER_PATH_BUF_CAP,
    WalkerEntryKind,
    WalkerDecision,
    WalkerSkipError,
    walkerHashName,
    WalkerSkipFilter,
    WalkerPathStack,
} from "./walkerSkip.cp";

// --- fairSched (Zero-Alloc Multi-Tenant Deficit Round Robin Weighted Fair Scheduler) ---
export {
    FAIR_SCHED_MAX_TENANTS,
    FAIR_SCHED_QUEUE_CAP,
    FAIR_SCHED_BPS_SCALE,
    FairSchedOpStatus,
    FairTask,
    FairTenantStats,
    FairSchedStats,
    DeficitRoundRobinScheduler,
} from "./fairSched.cp";

