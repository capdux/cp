// Type definitions for CP

export enum ArchKind {
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
    Unknown
}

export enum OsKind {
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
    Unknown
}

export enum EnvKind {
    Gnu,
    Musl,
    Msvc,
    Eabi,
    Eabihf,
    None,
    Unknown
}

export class TargetTriple {
    valid: bool,
    raw_arch: string,
    raw_vendor: string,
    raw_os: string,
    raw_env: string,
    arch: ArchKind,
    os: OsKind,
    env: EnvKind,
    pointer_width: u8,
    is_little_endian: bool,
}

export fn parseArch(s: string): ArchKind;
export fn parseOs(s: string): OsKind;
export fn parseEnv(s: string): EnvKind;
export fn pointerWidthFor(a: ArchKind): u8;
export fn isLittleEndianFor(a: ArchKind, raw_arch: string): bool;
export fn parseTargetTriple(raw: string): TargetTriple;
export fn exeSuffix(t: TargetTriple): string;
export fn dylibSuffix(t: TargetTriple): string;
