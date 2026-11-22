// Source: Absorbed from /data/dev/bun/src/install/lockfile/Package/Meta.cp | Synced: 2026-11-22T16:55:15Z
// Pure-CP zero-allocation Lockfile Package Meta (`PkgMeta`):
// OS/CPU bitmask platform matching (including `!` negation lists), 3-state `HasInstallScript`
// migration guard (`Old` / `False` / `True`), origin tagging, man-dir & SRI integrity storage.

pub const PKG_META_INVALID_ID: u32 = 0xFFFF_FFFF;
pub const PKG_META_MAX_MAN_DIR: usize = 128;
pub const PKG_META_MAX_INTEGRITY_LEN: usize = 128;

// Operating System bitmask flags (npm `os` field)
pub const PKG_OS_NONE: u16 = 0;
pub const PKG_OS_ALL: u16 = 0xFFFF;
pub const PKG_OS_LINUX: u16 = 1 << 0;
pub const PKG_OS_DARWIN: u16 = 1 << 1;
pub const PKG_OS_WIN32: u16 = 1 << 2;
pub const PKG_OS_FREEBSD: u16 = 1 << 3;
pub const PKG_OS_OPENBSD: u16 = 1 << 4;
pub const PKG_OS_SUNOS: u16 = 1 << 5;
pub const PKG_OS_AIX: u16 = 1 << 6;
pub const PKG_OS_ANDROID: u16 = 1 << 7;

// CPU Architecture bitmask flags (npm `cpu` field)
pub const PKG_ARCH_NONE: u16 = 0;
pub const PKG_ARCH_ALL: u16 = 0xFFFF;
pub const PKG_ARCH_X64: u16 = 1 << 0;
pub const PKG_ARCH_ARM64: u16 = 1 << 1;
pub const PKG_ARCH_ARM: u16 = 1 << 2;
pub const PKG_ARCH_IA32: u16 = 1 << 3;
pub const PKG_ARCH_PPC64: u16 = 1 << 4;
pub const PKG_ARCH_S390X: u16 = 1 << 5;
pub const PKG_ARCH_RISCV64: u16 = 1 << 6;
pub const PKG_ARCH_MIPS: u16 = 1 << 7;
pub const PKG_ARCH_LOONG64: u16 = 1 << 8;

pub enum PkgMetaOrigin: u8 {
    Npm = 0,
    Local = 1,
    Git = 2,
    Tarball = 3,
    Workspace = 4,
    Folder = 5,
}

/// 3-state install-script marker mirroring Bun `Meta.HasInstallScript`:
/// - `Old`: legacy lockfile value before install-script tracking (triggers `needsUpdate()`).
/// - `False`: confirmed no lifecycle install script.
/// - `True`: package defines install/preinstall/postinstall script.
pub enum HasInstallScript: u8 {
    Old = 0,
    False = 1,
    True = 2,
}

pub err PkgMetaError {
    FieldTooLong,
    LegacyInstallScriptState,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlLowerAscii(a: []const u8, lower_lit: []const u8): bool {
    if (a.len != lower_lit.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != lower_lit[i]) return false;
        i += 1;
    }
    return true;
}

fn copyFixed(dst: []u8, src: []const u8): usize! {
    if (src.len > dst.len) return PkgMetaError.FieldTooLong;
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
    return src.len;
}

pub fn parseNpmOsBit(name: []const u8): u16 {
    if (eqlLowerAscii(name, "linux")) return PKG_OS_LINUX;
    if (eqlLowerAscii(name, "darwin")) return PKG_OS_DARWIN;
    if (eqlLowerAscii(name, "win32")) return PKG_OS_WIN32;
    if (eqlLowerAscii(name, "freebsd")) return PKG_OS_FREEBSD;
    if (eqlLowerAscii(name, "openbsd")) return PKG_OS_OPENBSD;
    if (eqlLowerAscii(name, "sunos")) return PKG_OS_SUNOS;
    if (eqlLowerAscii(name, "aix")) return PKG_OS_AIX;
    if (eqlLowerAscii(name, "android")) return PKG_OS_ANDROID;
    return PKG_OS_NONE;
}

pub fn parseNpmArchBit(name: []const u8): u16 {
    if (eqlLowerAscii(name, "x64")) return PKG_ARCH_X64;
    if (eqlLowerAscii(name, "arm64")) return PKG_ARCH_ARM64;
    if (eqlLowerAscii(name, "arm")) return PKG_ARCH_ARM;
    if (eqlLowerAscii(name, "ia32")) return PKG_ARCH_IA32;
    if (eqlLowerAscii(name, "ppc64")) return PKG_ARCH_PPC64;
    if (eqlLowerAscii(name, "s390x")) return PKG_ARCH_S390X;
    if (eqlLowerAscii(name, "riscv64")) return PKG_ARCH_RISCV64;
    if (eqlLowerAscii(name, "mips")) return PKG_ARCH_MIPS;
    if (eqlLowerAscii(name, "loong64")) return PKG_ARCH_LOONG64;
    return PKG_ARCH_NONE;
}

/// Evaluates an npm `os` array (e.g. `["linux", "darwin"]` or `["!win32"]`) into a bitmask.
pub fn applyNpmOsList(items: []const []const u8): u16 {
    if (items.len == 0) return PKG_OS_ALL;
    let include_mask: u16 = PKG_OS_NONE;
    let exclude_mask: u16 = PKG_OS_NONE;
    let has_include = false;
    let i: usize = 0;
    while (i < items.len) {
        const item = items[i];
        if (item.len > 1 && item[0] == '!') {
            exclude_mask |= parseNpmOsBit(item[1..]);
        } else if (item.len > 0) {
            has_include = true;
            include_mask |= parseNpmOsBit(item);
        }
        i += 1;
    }
    const base: u16 = if (has_include) include_mask else PKG_OS_ALL;
    return base & (~exclude_mask);
}

/// Evaluates an npm `cpu` array (e.g. `["x64", "arm64"]` or `["!ia32"]`) into a bitmask.
pub fn applyNpmArchList(items: []const []const u8): u16 {
    if (items.len == 0) return PKG_ARCH_ALL;
    let include_mask: u16 = PKG_ARCH_NONE;
    let exclude_mask: u16 = PKG_ARCH_NONE;
    let has_include = false;
    let i: usize = 0;
    while (i < items.len) {
        const item = items[i];
        if (item.len > 1 && item[0] == '!') {
            exclude_mask |= parseNpmArchBit(item[1..]);
        } else if (item.len > 0) {
            has_include = true;
            include_mask |= parseNpmArchBit(item);
        }
        i += 1;
    }
    const base: u16 = if (has_include) include_mask else PKG_ARCH_ALL;
    return base & (~exclude_mask);
}

/// Zero-allocation Lockfile Package Meta aligned with Bun `Lockfile.Package.Meta`.
pub struct PkgMeta {
    origin: PkgMetaOrigin = PkgMetaOrigin.Npm,
    arch: u16 = PKG_ARCH_ALL,
    os: u16 = PKG_OS_ALL,
    id: u32 = PKG_META_INVALID_ID,
    has_install_script: HasInstallScript = HasInstallScript.False,
    man_dir_buf: [PKG_META_MAX_MAN_DIR]u8 = [0; PKG_META_MAX_MAN_DIR],
    man_dir_len: usize = 0,
    integrity_buf: [PKG_META_MAX_INTEGRITY_LEN]u8 = [0; PKG_META_MAX_INTEGRITY_LEN],
    integrity_len: usize = 0,

    static fn init(): PkgMeta {
        return PkgMeta {};
    }

    static fn initWithId(id: u32, origin: PkgMetaOrigin): PkgMeta {
        return PkgMeta {
            id: id,
            origin: origin,
        };
    }

    /// Returns true if the target `(cpu_bit, os_bit)` does NOT satisfy this package's `cpu`/`os` masks.
    fn isDisabled(cpu_bit: u16, os_bit: u16): bool {
        const arch_ok = (_.arch & cpu_bit) != 0;
        const os_ok = (_.os & os_bit) != 0;
        return !arch_ok || !os_ok;
    }

    fn matchesPlatform(cpu_bit: u16, os_bit: u16): bool {
        return !_.isDisabled(cpu_bit, os_bit);
    }

    fn hasInstallScript(): bool {
        return _.has_install_script == HasInstallScript.True;
    }

    fn setHasInstallScript(*_, has_script: bool) {
        _.has_install_script = if (has_script) HasInstallScript.True else HasInstallScript.False;
    }

    /// True if `has_install_script` is still `.Old` (legacy lockfile entry requiring refresh).
    fn needsUpdate(): bool {
        return _.has_install_script == HasInstallScript.Old;
    }

    /// Guard mirroring Bun's serializer assertion: `.Old` must never be written to a new lockfile.
    fn validateForSave(): ! {
        if (_.has_install_script == HasInstallScript.Old) {
            return PkgMetaError.LegacyInstallScriptState;
        }
    }

    fn setManDir(*_, dir: []const u8): ! {
        _.man_dir_len = copyFixed(_.man_dir_buf[0..], dir)?;
    }

    fn manDir(): []const u8 {
        return _.man_dir_buf[0.._.man_dir_len];
    }

    fn setIntegrity(*_, sri: []const u8): ! {
        _.integrity_len = copyFixed(_.integrity_buf[0..], sri)?;
    }

    fn integrity(): []const u8 {
        return _.integrity_buf[0.._.integrity_len];
    }

    fn hasIntegrity(): bool {
        return _.integrity_len > 0;
    }

    fn cloneWithId(new_id: u32): PkgMeta {
        return PkgMeta {
            origin: _.origin,
            arch: _.arch,
            os: _.os,
            id: new_id,
            has_install_script: _.has_install_script,
            man_dir_buf: _.man_dir_buf,
            man_dir_len: _.man_dir_len,
            integrity_buf: _.integrity_buf,
            integrity_len: _.integrity_len,
        };
    }
}
