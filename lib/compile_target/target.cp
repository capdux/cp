// Source: Absorbed from /data/dev/bun/src/options_types/CompileTarget.cp

/// 目标操作系统枚举
pub enum TargetOs: u8 {
    linux = 0,
    darwin = 1,
    windows = 2,
    freebsd = 3,
    wasm = 4,

    fn asStr(): string {
        return switch (_) {
            .linux => "linux",
            .darwin => "darwin",
            .windows => "windows",
            .freebsd => "freebsd",
            .wasm => "wasm",
        };
    }

    fn zigName(): string {
        return switch (_) {
            .linux => "linux",
            .darwin => "macos",
            .windows => "windows",
            .freebsd => "freebsd",
            .wasm => "wasi",
        };
    }

    static fn parse(s: []const u8): TargetOs? {
        if (eqlIgnoreCaseAscii(s, "linux")) { return .linux; }
        if (eqlIgnoreCaseAscii(s, "darwin") || eqlIgnoreCaseAscii(s, "macos") || eqlIgnoreCaseAscii(s, "osx") || eqlIgnoreCaseAscii(s, "mac")) {
            return .darwin;
        }
        if (eqlIgnoreCaseAscii(s, "windows") || eqlIgnoreCaseAscii(s, "win32") || eqlIgnoreCaseAscii(s, "win")) {
            return .windows;
        }
        if (eqlIgnoreCaseAscii(s, "freebsd")) { return .freebsd; }
        if (eqlIgnoreCaseAscii(s, "wasm") || eqlIgnoreCaseAscii(s, "wasi")) { return .wasm; }
        return null;
    }
}

/// 目标 CPU 架构枚举
pub enum TargetArch: u8 {
    x64 = 0,
    arm64 = 1,
    x86 = 2,
    wasm32 = 3,

    fn asStr(): string {
        return switch (_) {
            .x64 => "x64",
            .arm64 => "arm64",
            .x86 => "x86",
            .wasm32 => "wasm32",
        };
    }

    fn zigName(): string {
        return switch (_) {
            .x64 => "x86_64",
            .arm64 => "aarch64",
            .x86 => "x86",
            .wasm32 => "wasm32",
        };
    }

    static fn parse(s: []const u8): TargetArch? {
        if (eqlIgnoreCaseAscii(s, "x64") || eqlIgnoreCaseAscii(s, "x86_64") || eqlIgnoreCaseAscii(s, "amd64")) {
            return .x64;
        }
        if (eqlIgnoreCaseAscii(s, "arm64") || eqlIgnoreCaseAscii(s, "aarch64")) {
            return .arm64;
        }
        if (eqlIgnoreCaseAscii(s, "x86") || eqlIgnoreCaseAscii(s, "i386") || eqlIgnoreCaseAscii(s, "i686") || eqlIgnoreCaseAscii(s, "ia32")) {
            return .x86;
        }
        if (eqlIgnoreCaseAscii(s, "wasm32")) {
            return .wasm32;
        }
        return null;
    }
}

/// 目标 C 运行时 / ABI 枚举
pub enum TargetLibc: u8 {
    default = 0,
    glibc = 1,
    musl = 2,
    msvc = 3,
    android = 4,

    fn asStr(): string {
        return switch (_) {
            .default => "default",
            .glibc => "glibc",
            .musl => "musl",
            .msvc => "msvc",
            .android => "android",
        };
    }

    static fn parse(s: []const u8): TargetLibc? {
        if (eqlIgnoreCaseAscii(s, "default")) { return .default; }
        if (eqlIgnoreCaseAscii(s, "glibc") || eqlIgnoreCaseAscii(s, "gnu")) { return .glibc; }
        if (eqlIgnoreCaseAscii(s, "musl")) { return .musl; }
        if (eqlIgnoreCaseAscii(s, "msvc")) { return .msvc; }
        if (eqlIgnoreCaseAscii(s, "android")) { return .android; }
        return null;
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

struct BufWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            if (_.pos < _.buf.len) {
                _.buf[_.pos] = s[i];
                _.pos += 1;
            } else {
                _.overflow = true;
            }
            i += 1;
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 跨平台编译与分发目标三元组模型
pub struct CompileTarget {
    os: TargetOs = .linux,
    arch: TargetArch = .x64,
    libc: TargetLibc = .default,
    baseline: bool = false,

    fn eql(other: CompileTarget): bool {
        return _.os == other.os && _.arch == other.arch && _.libc == other.libc && _.baseline == other.baseline;
    }

    /// 解析目标规格字符串（支持可选 `bun-`/`cap-` 前缀及连字符分隔，如 `linux-x64-musl-baseline`、`macos-aarch64`）
    static fn parse(spec: []const u8): CompileTarget? {
        let s = trimSpaces(spec);
        if (s.len == 0) { return null; }

        if (startsWithIgnoreCaseAscii(s, "bun-")) {
            s = s[4..s.len];
        } else if (startsWithIgnoreCaseAscii(s, "cap-")) {
            s = s[4..s.len];
        }
        if (s.len == 0) { return null; }

        let result = CompileTarget{};
        let has_os = false;
        let has_arch = false;
        let has_libc = false;
        let has_mode = false;

        let i: usize = 0;
        while (i <= s.len) {
            const start = i;
            while (i < s.len && s[i] != '-') { i += 1; }
            const tok = s[start..i];
            if (tok.len == 0) { return null; }

            if (o := TargetOs.parse(tok)) {
                if (has_os) { return null; }
                result.os = o;
                has_os = true;
            } else if (a := TargetArch.parse(tok)) {
                if (has_arch) { return null; }
                result.arch = a;
                has_arch = true;
            } else if (l := TargetLibc.parse(tok)) {
                if (has_libc) { return null; }
                result.libc = l;
                has_libc = true;
            } else if (eqlIgnoreCaseAscii(tok, "baseline")) {
                if (has_mode) { return null; }
                result.baseline = true;
                has_mode = true;
            } else if (eqlIgnoreCaseAscii(tok, "modern")) {
                if (has_mode) { return null; }
                result.baseline = false;
                has_mode = true;
            } else {
                return null;
            }

            if (i == s.len) { break; }
            i += 1;
        }

        // wasm 便捷补全
        if (has_os && result.os == .wasm && !has_arch) {
            result.arch = .wasm32;
            has_arch = true;
        }
        if (has_arch && result.arch == .wasm32 && !has_os) {
            result.os = .wasm;
            has_os = true;
        }

        if (!has_os || !has_arch) {
            return null;
        }
        return result;
    }

    /// 校验 OS / Arch / Libc / baseline 组合是否合法受支持
    fn isSupported(): bool {
        // baseline（无 AVX2 兼容模式）仅适用于 x64
        if (_.baseline && _.arch != .x64) {
            return false;
        }

        return switch (_.os) {
            .linux => {
                const arch_ok = (_.arch == .x64 || _.arch == .arm64 || _.arch == .x86);
                const libc_ok = switch (_.libc) {
                    .default => true,
                    .glibc => true,
                    .musl => true,
                    .android => (_.arch == .arm64 || _.arch == .x64),
                    .msvc => false,
                };
                arch_ok && libc_ok;
            },
            .darwin => (_.arch == .x64 || _.arch == .arm64) && _.libc == .default,
            .windows => (_.arch == .x64 || _.arch == .arm64 || _.arch == .x86) && (_.libc == .default || _.libc == .msvc),
            .freebsd => (_.arch == .x64 || _.arch == .arm64) && _.libc == .default,
            .wasm => _.arch == .wasm32 && _.libc == .default,
        };
    }

    /// 返回平台可执行文件后缀（Windows 返回 `".exe"`，其余返回 `""`）
    fn exeSuffix(): []const u8 {
        if (_.os == .windows) {
            return ".exe";
        }
        return "";
    }

    /// 规范化输出目标名（如 `linux-x64-musl-baseline`、`darwin-arm64`）
    fn toCanonical(out: []u8): ([]const u8)? {
        let w = BufWriter{ buf: out };
        w.writeStr(_.os.asStr());
        w.writeStr("-");
        w.writeStr(_.arch.asStr());
        if (_.libc != .default) {
            w.writeStr("-");
            w.writeStr(_.libc.asStr());
        }
        if (_.baseline) {
            w.writeStr("-baseline");
        }
        return w.finish();
    }

    /// 输出标准 Zig 交叉编译三元组（如 `x86_64-linux-musl`、`aarch64-macos`、`x86_64-windows-msvc`）
    fn toZigTriple(out: []u8): ([]const u8)? {
        let w = BufWriter{ buf: out };
        w.writeStr(_.arch.zigName());
        w.writeStr("-");
        w.writeStr(_.os.zigName());
        switch (_.os) {
            .linux => switch (_.libc) {
                .default => w.writeStr("-gnu"),
                .glibc => w.writeStr("-gnu"),
                .musl => w.writeStr("-musl"),
                .android => w.writeStr("-android"),
                .msvc => w.writeStr("-msvc"),
            },
            .windows => switch (_.libc) {
                .default => w.writeStr("-msvc"),
                .msvc => w.writeStr("-msvc"),
                .glibc => w.writeStr("-gnu"),
                .musl => w.writeStr("-musl"),
                .android => w.writeStr("-android"),
            },
            .darwin => {},
            .freebsd => {},
            .wasm => {},
        }
        return w.finish();
    }
}
