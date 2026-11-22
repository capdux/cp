# `cap:compile_target` — 跨平台构建与分发目标三元组解析库

> 参考来源：`/data/dev/bun/docs/bundler/executables.mdx`、`/data/dev/bun/src/options_types/CompileTarget.cp` | 吸收日期：2026-11-22

`cap:compile_target` 提供纯 CP、零外部依赖、零堆分配的跨平台编译目标规格解析、合法性矩阵校验、可执行文件后缀判定与 Zig 交叉编译三元组生成能力。

## 核心类型

### `TargetOs`

```cp
pub enum TargetOs: u8 {
    linux = 0,
    darwin = 1,
    windows = 2,
    freebsd = 3,
    wasm = 4,

    fn asStr(): string
    fn zigName(): string
    static fn parse(s: []const u8): TargetOs?
}
```

- 别名支持：`macos`/`osx`/`mac` → `.darwin`，`win32`/`win` → `.windows`，`wasi` → `.wasm`。

### `TargetArch`

```cp
pub enum TargetArch: u8 {
    x64 = 0,
    arm64 = 1,
    x86 = 2,
    wasm32 = 3,

    fn asStr(): string
    fn zigName(): string
    static fn parse(s: []const u8): TargetArch?
}
```

- 别名支持：`x86_64`/`amd64` → `.x64`，`aarch64` → `.arm64`，`i386`/`i686`/`ia32` → `.x86`。

### `TargetLibc`

```cp
pub enum TargetLibc: u8 {
    default = 0,
    glibc = 1,
    musl = 2,
    msvc = 3,
    android = 4,

    fn asStr(): string
    static fn parse(s: []const u8): TargetLibc?
}
```

- 别名支持：`gnu` → `.glibc`。

### `CompileTarget`

```cp
pub struct CompileTarget {
    os: TargetOs = .linux,
    arch: TargetArch = .x64,
    libc: TargetLibc = .default,
    baseline: bool = false,

    fn eql(other: CompileTarget): bool
    static fn parse(spec: []const u8): CompileTarget?
    fn isSupported(): bool
    fn exeSuffix(): []const u8
    fn toCanonical(out: []u8): ([]const u8)?
    fn toZigTriple(out: []u8): ([]const u8)?
}
```

- `CompileTarget.parse(spec)`：支持可选 `bun-` / `cap-` 前缀、连字符分隔及大小写不敏感解析（如 `cap-linux-x64-musl-baseline`、`bun-macos-aarch64`、`win32-amd64`）。
- `isSupported()`：校验 OS/Arch/Libc/baseline 合法组合，拒绝 `darwin-musl`、`windows-musl`、`arm64-baseline`、`linux-msvc` 等非法组合。
- `exeSuffix()`：Windows 返回 `".exe"`，其余平台返回 `""`。
- `toCanonical(out)`：规范化输出如 `linux-x64-musl-baseline`、`darwin-arm64`。
- `toZigTriple(out)`：输出标准 Zig 交叉编译三元组如 `x86_64-linux-musl`、`aarch64-macos`、`x86_64-windows-msvc`。

## 使用示例

```cp
import { CompileTarget } from "cap:compile_target";

const t = CompileTarget.parse("cap-linux-x86_64-musl-baseline").?;
// t.isSupported() == true
let buf: [64]u8 = [0; 64];
const canon = t.toCanonical(buf[0..]).?;  // "linux-x64-musl-baseline"
const zig_t = t.toZigTriple(buf[0..]).?;  // "x86_64-linux-musl"
```

## 上游行为契约与边界注意事项

1. **`baseline` CPU 特性语义（对齐 Bun `--target=...-baseline`）**：
   - `baseline = true` 仅适用于 `x64`（面向 2013 年以前不支持 AVX2 的 Nehalem 等老旧 CPU）；对 `arm64`/`wasm32` 设置 `baseline` 会被 `isSupported()` 明确拦截返回 `false`。
2. **OS / Arch / Libc 合法性矩阵**：
   - `darwin` 仅支持默认 Libc（拒绝 `darwin-musl`、`darwin-glibc`）；`windows` 拒绝 `musl`/`glibc`；`linux` 拒绝 `msvc`；`wasm` 必须搭配 `wasm32`。
3. **零堆分配格式化边界**：
   - `toCanonical(out)` 与 `toZigTriple(out)` 均写入调用方栈缓冲，空间不足时安全返回 `null`，无隐式堆分配。
