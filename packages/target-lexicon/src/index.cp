// @cap/target-triple — Zero-heap LLVM / Rust / Zig Target Triple Parser
// Clean-room MIT implementation in pure CP (inspired by Rust `target-lexicon` [479M downloads]).

pub enum ArchKind {
    X86_64,
    Aarch64,
    X86,
    Arm,
    Riscv64,
    Riscv32,
    Wasm32,
    Wasm64,
    LoongArch64,
    PowerPc64,
    S390x,
    Unknown,
}

pub enum OsKind {
    Linux,
    Darwin,
    Windows,
    FreeBsd,
    OpenBsd,
    NetBsd,
    Android,
    Ios,
    Wasi,
    None,
    Unknown,
}

pub enum EnvKind {
    Gnu,
    Musl,
    Msvc,
    Eabi,
    Eabihf,
    None,
    Unknown,
}

pub struct TargetTriple {
    pub valid: bool,
    pub raw_arch: string,
    pub raw_vendor: string,
    pub raw_os: string,
    pub raw_env: string,
    pub arch: ArchKind,
    pub os: OsKind,
    pub env: EnvKind,
    pub pointer_width: u8,
    pub is_little_endian: bool,
}

pub fn parseArch(s: string) -> ArchKind {
    if (s == "x86_64" || s == "amd64" || s == "x86_64h") { return ArchKind.X86_64; }
    if (s == "aarch64" || s == "arm64") { return ArchKind.Aarch64; }
    if (s == "i386" || s == "i586" || s == "i686" || s == "x86") { return ArchKind.X86; }
    if (s == "arm" || s == "armv7" || s == "armv7a" || s == "thumbv7em") { return ArchKind.Arm; }
    if (s == "riscv64" || s == "riscv64gc") { return ArchKind.Riscv64; }
    if (s == "riscv32" || s == "riscv32imac") { return ArchKind.Riscv32; }
    if (s == "wasm32") { return ArchKind.Wasm32; }
    if (s == "wasm64") { return ArchKind.Wasm64; }
    if (s == "loongarch64") { return ArchKind.LoongArch64; }
    if (s == "powerpc64" || s == "powerpc64le") { return ArchKind.PowerPc64; }
    if (s == "s390x") { return ArchKind.S390x; }
    return ArchKind.Unknown;
}

pub fn parseOs(s: string) -> OsKind {
    if (s == "linux") { return OsKind.Linux; }
    if (s == "darwin" || s == "macos") { return OsKind.Darwin; }
    if (s == "windows" || s == "win32") { return OsKind.Windows; }
    if (s == "freebsd") { return OsKind.FreeBsd; }
    if (s == "openbsd") { return OsKind.OpenBsd; }
    if (s == "netbsd") { return OsKind.NetBsd; }
    if (s == "android") { return OsKind.Android; }
    if (s == "ios") { return OsKind.Ios; }
    if (s == "wasi" || s == "wasip1" || s == "wasip2") { return OsKind.Wasi; }
    if (s == "none" || s == "unknown") { return OsKind.None; }
    return OsKind.Unknown;
}

pub fn parseEnv(s: string) -> EnvKind {
    if (s == "gnu" || s == "gnueabi" || s == "gnueabihf") { return EnvKind.Gnu; }
    if (s == "musl" || s == "musleabi" || s == "musleabihf") { return EnvKind.Musl; }
    if (s == "msvc") { return EnvKind.Msvc; }
    if (s == "eabi") { return EnvKind.Eabi; }
    if (s == "eabihf") { return EnvKind.Eabihf; }
    if (s.len == 0 || s == "none") { return EnvKind.None; }
    return EnvKind.Unknown;
}

fn pointerWidthFor(a: ArchKind) -> u8 {
    switch (a) {
        ArchKind.X86_64 => { return 64; },
        ArchKind.Aarch64 => { return 64; },
        ArchKind.Riscv64 => { return 64; },
        ArchKind.Wasm64 => { return 64; },
        ArchKind.LoongArch64 => { return 64; },
        ArchKind.PowerPc64 => { return 64; },
        ArchKind.S390x => { return 64; },
        ArchKind.X86 => { return 32; },
        ArchKind.Arm => { return 32; },
        ArchKind.Riscv32 => { return 32; },
        ArchKind.Wasm32 => { return 32; },
        ArchKind.Unknown => { return 0; },
    }
}

fn isLittleEndianFor(a: ArchKind, raw_arch: string) -> bool {
    if (raw_arch == "powerpc64" || raw_arch == "s390x") {
        return false;
    }
    switch (a) {
        ArchKind.S390x => { return false; },
        _ => { return true; },
    }
}

/// Parse a 2-, 3-, or 4-part LLVM/Rust/Zig target triple:
/// - 2-part: `wasm32-wasi`, `x86_64-linux`
/// - 3-part: `aarch64-apple-darwin`, `x86_64-linux-musl`, `wasm32-unknown-unknown`
/// - 4-part: `x86_64-unknown-linux-gnu`, `x86_64-pc-windows-msvc`
pub fn parseTargetTriple(raw: string) -> TargetTriple {
    const invalid: TargetTriple = TargetTriple {
        valid: false,
        raw_arch: "",
        raw_vendor: "",
        raw_os: "",
        raw_env: "",
        arch: ArchKind.Unknown,
        os: OsKind.Unknown,
        env: EnvKind.None,
        pointer_width: 0,
        is_little_endian: true,
    };

    if (raw.len == 0) { return invalid; }

    let parts: [4]string = ["", "", "", ""];
    let pcount: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= raw.len) {
        if (i == raw.len || raw[i] == '-') {
            if (pcount >= 4) { return invalid; }
            if (i == start) { return invalid; }
            parts[pcount] = raw[start..i];
            pcount += 1;
            start = i + 1;
        }
        i += 1;
    }

    if (pcount < 2) { return invalid; }

    const rawArch: string = parts[0];
    const arch: ArchKind = parseArch(rawArch);

    let rawVendor: string = "unknown";
    let rawOs: string = "";
    let rawEnv: string = "";

    if (pcount == 2) {
        // e.g. wasm32-wasi, x86_64-linux
        rawOs = parts[1];
    } else if (pcount == 3) {
        // Could be arch-vendor-os (aarch64-apple-darwin) OR Zig style arch-os-abi (x86_64-linux-musl)
        const p1Os: OsKind = parseOs(parts[1]);
        let isP1Os: bool = false;
        switch (p1Os) {
            OsKind.Unknown => { isP1Os = false; },
            OsKind.None => { isP1Os = false; },
            _ => { isP1Os = true; },
        }
        if (isP1Os) {
            rawOs = parts[1];
            rawEnv = parts[2];
        } else {
            rawVendor = parts[1];
            rawOs = parts[2];
        }
    } else {
        // 4-part: arch-vendor-os-env
        rawVendor = parts[1];
        rawOs = parts[2];
        rawEnv = parts[3];
    }

    const os: OsKind = parseOs(rawOs);
    const env: EnvKind = parseEnv(rawEnv);

    return TargetTriple {
        valid: true,
        raw_arch: rawArch,
        raw_vendor: rawVendor,
        raw_os: rawOs,
        raw_env: rawEnv,
        arch: arch,
        os: os,
        env: env,
        pointer_width: pointerWidthFor(arch),
        is_little_endian: isLittleEndianFor(arch, rawArch),
    };
}

/// Return conventional executable suffix (".exe", ".wasm", or "").
pub fn exeSuffix(t: TargetTriple) -> string {
    switch (t.os) {
        OsKind.Windows => { return ".exe"; },
        OsKind.Wasi => { return ".wasm"; },
        _ => {},
    }
    switch (t.arch) {
        ArchKind.Wasm32 => { return ".wasm"; },
        ArchKind.Wasm64 => { return ".wasm"; },
        _ => { return ""; },
    }
}

/// Return conventional dynamic library suffix (".so", ".dylib", ".dll", or "").
pub fn dylibSuffix(t: TargetTriple) -> string {
    switch (t.os) {
        OsKind.Windows => { return ".dll"; },
        OsKind.Darwin => { return ".dylib"; },
        OsKind.Ios => { return ".dylib"; },
        OsKind.Linux => { return ".so"; },
        OsKind.Android => { return ".so"; },
        OsKind.FreeBsd => { return ".so"; },
        OsKind.OpenBsd => { return ".so"; },
        OsKind.NetBsd => { return ".so"; },
        _ => { return ""; },
    }
}
