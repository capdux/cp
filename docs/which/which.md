# `cap:which` — 跨平台可执行二进制查找与权限校验标准库

> **上游溯源**: `/data/dev/bun/src/which/which.cp` (`which.zig`)  
> **模块路径**: `lib/which/`  
> **导入协议**: `import { Which, WhichOptions, which, isExecutable, endsWithExtension } from "cap:which";`

---

## 一、 架构概述与设计亮点

`cap:which` 吸收自 Bun 的 `Bun.which` 原生可执行文件定位引擎（`bun/src/which/which.cp`），用于在跨平台文件系统中快速解析命令名称并校验其可执行权限：

1. **零堆分配输出缓冲区机制**：调用方传入栈上缓冲区 `out_buf: []u8`，查找命中时直接返回指向 `out_buf` 的切片视图 `([]const u8)?`，全程零动态内存分配。
2. **三级路径解析流水线**：
   - **绝对路径直通校验**：当输入为绝对路径（如 `"/bin/sh"`）时，不扫描 `PATH`，直接校验目标文件是否具备可执行权限。
   - **含路径分隔符的相对路径解析**：当输入包含 `/` 或 `\`（如 `"./sh"` 或 `"bin/cap"`）时，结合 `options.cwd`（或当前进程工作目录）规整前导 `./` 后直接定位并校验。
   - **`PATH` 环境变量分词探测**：按平台分隔符（POSIX `:` / Windows `;`）逐段扫描 `options.path`（缺省自动读取系统 `getenv("PATH")`），返回首个通过权限校验的完整路径。
3. **严格的目录排除与跨平台权限校验 (`isExecutable`)**：
   - **POSIX (Linux / macOS)**：调用 `std.c.access(c_path, X_OK)` 校验可执行位（`1`），并额外通过 `opendir` 排除具有 `+x` 权限的同名目录（防止将 `/bin` 或同名子目录误判为可执行二进制文件）。
   - **Windows**：自动识别并补全 `.exe`、`.cmd`、`.bat` 扩展名（大小写不敏感）。

---

## 二、 `cap:which` 与 `Bun.which` API 对照表

| 功能描述 | `cap:which` API | `Bun.which` (JS/TS) 对照 | 说明 |
| :--- | :--- | :--- | :--- |
| **定位可执行程序** | `which(bin, options, out_buf)` | `Bun.which(bin, { PATH, cwd })` | 找到返回 `([]const u8)?`，未找到返回 `null` |
| **自定义查找配置** | `WhichOptions { path, cwd }` | `{ PATH?: string, cwd?: string }` | `path` 覆盖系统 `PATH`；`cwd` 指定相对路径基准目录 |
| **校验文件可执行性** | `isExecutable(filepath)` | `bun.sys.isExecutableFilePath(p)` | 校验文件存在、非目录且具备可执行权限 |
| **Windows 扩展名判定** | `endsWithExtension(str)` | `bun.which.endsWithExtension(s)` | 大小写不敏感检测 `.exe` / `.cmd` / `.bat` 后缀 |

---

## 三、 代码示例

### 1. 系统 `PATH` 查找与自定义 `PATH` / `cwd` 查找

```typescript
const std = @import("std");
import { which, isExecutable, WhichOptions } from "cap:which";

fn demoWhichLookup(): void {
    let out_buf: [512]u8 = undefined;

    // 1. 在默认系统 PATH 中查找二进制文件
    if (sh_path := which("sh", .{}, &out_buf)) {
        // sh_path => "/bin/sh" 或 "/usr/bin/sh"
        let ok = isExecutable(sh_path); // true
    }

    // 2. 使用自定义 PATH 环境变量隔离查找
    let custom_opts = WhichOptions{
        path: "/usr/local/bin:/bin:/usr/bin",
        cwd: null,
    };
    if (found := which("sh", custom_opts, &out_buf)) {
        // found => "/bin/sh"
    }

    // 3. 解析带 "./" 的相对路径并指定基准目录 cwd
    if (rel_found := which("./sh", .{ .cwd = "/bin" }, &out_buf)) {
        // rel_found => "/bin/sh"
    }

    // 4. 查找不存在的命令返回 null
    let missing = which("nonexistent_cmd_xyz", .{}, &out_buf);
    // missing => null
}
```

### 2. 权限与目录安全校验 (`isExecutable` & `endsWithExtension`)

```typescript
import { isExecutable, endsWithExtension } from "cap:which";

fn demoSecurityChecks(): void {
    // 目录即使具有 +x 权限，isExecutable 也会安全返回 false
    let dir_check = isExecutable("/bin");     // false
    let bin_check = isExecutable("/bin/sh");  // true

    // Windows 可执行扩展名大小写不敏感识别
    let ext1 = endsWithExtension("bun.exe");    // true
    let ext2 = endsWithExtension("DEPLOY.CMD"); // true
    let ext3 = endsWithExtension("script.sh");  // false
}
```
