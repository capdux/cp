# CP 标准库：跨平台路径规范化、解析与安全边界校验手册 (`cap:paths`)

## 一、 架构总览与设计准则

`cap:paths` 是 CP 语言官方内置的纯 CP **跨平台路径解析、规范化与安全边界校验库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。核心解析函数（`parse`、`dirname`、`basename`、`extname`、`isAbsolute`、`containsNullByte`、`hasTrailingSlash`、`trimTrailingSlashes`、`matchGlobSimple`）均为**零堆分配切片视图**；路径构造与规范化函数（`normalize`、`join`、`resolve`、`relative`、`isSubpath`、`safeJoin`、`toPosixSlash`、`toWindowsSlash`）均显式接收 `std.mem.Allocator` 并由调用方负责释放，彻底杜绝隐藏全局状态与内存泄漏。同时支持 `import { ... } from "cap:paths"` 与 `import cap from "cap"; cap.paths.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/paths/types.cp` | `// Source: bun/src/paths/paths.cp -> cp/lib/paths/types.cp` | `PathError`、`PathStyle`、`ParsedPath`、根长度识别与零分配谓词 |
| `lib/paths/parse.cp` | `// Source: bun/src/paths/resolve_path_.zig -> cp/lib/paths/parse.cp` | 零堆分配 `parse`/`dirname`/`basename`/`extname` 与分隔符转换 |
| `lib/paths/ops.cp` | `// Source: bun/src/paths/resolve_path_.zig -> cp/lib/paths/ops.cp` | `normalize`、`join`、`resolve`、`relative` 路径折叠与根边界守卫 |
| `lib/paths/security.cp` | `// Source: cp/lib/paths/security.cp (...)` | `isSubpath`、`safeJoin` 目录穿越防护与 `matchGlobSimple` |
| `lib/paths/index.cp` | `// Source: cp/lib/paths/index.cp (...)` | `cap:paths` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集、风格枚举与数据结构

- **`PathError`**：
  - `InvalidPath`：非法路径格式。
  - `NullByteFound`：路径中检测到空字节 `\x00`（截断注入防护）。
  - `PathTraversal`：`safeJoin` 检测到 `../` 越界逃逸、绝对路径注入或盘符注入。
  - `InvalidDriveLetter`：Windows 盘符非 ASCII 字母（如 `1:\foo`）。
  - `OutOfMemory`：内存分配失败。
- **`PathStyle`**：`Posix` | `Windows`。
- **`ParsedPath`**：`{ root: string, dir: string, base: string, ext: string, name: string }`。

### 2. 零堆分配解析与分隔符谓词

- **`parse(path: string, style: PathStyle): ParsedPath`**：零堆分配分解路径；支持 POSIX `/`、Windows 盘符 `C:\` / `C:` 与 UNC `\\server\share\`；正确处理 `.gitignore` 等前导点隐藏文件（`ext == ""`）。
- **`dirname(path, style)` / `basename(path, style)` / `extname(path, style)`**：直接返回 `parse(path, style)` 对应切片视图。
- **`isAbsolute(path, style): bool`**：判定绝对路径（Windows 下 `C:\foo` 与 `\\srv\shr\a` 为 `true`，盘符相对路径 `C:foo` 为 `false`）。
- **`containsNullByte(path): bool`** / **`hasTrailingSlash(path, style): bool`** / **`trimTrailingSlashes(path, style): string`**（保留根前缀 `/`、`C:\`、`\\srv\shr\`）。

### 3. 路径规范化与组合（显式 `std.mem.Allocator`）

- **`normalize(alloc, path, style): string ! PathError`**：折叠 `.`、`..` 与多余分隔符并守卫根边界（`/../../a` -> `/a`、`C:\..\..\a` -> `C:\a`、`\\srv\shr\..\a` -> `\\srv\shr\a`）。
- **`join(alloc, parts, style): string ! PathError`**：拼接非空路径段并规范化；全空返回 `"."`。
- **`resolve(alloc, parts, style): string ! PathError`**：从右向左查找最右侧绝对路径并向后拼接规范化；若无绝对路径，则锚定至风格根（`/` 或 `\`），保证纯函数跨平台确定性。
- **`relative(alloc, from_path, to_path, style): string ! PathError`**：计算从 `from_path` 到 `to_path` 的相对路径；Windows 下盘符/UNC 不同时直接返回规范化后的 `to_path`。
- **`toPosixSlash(alloc, path)` / `toWindowsSlash(alloc, path)`**：分隔符全量互转并拦截 `\x00`。

### 4. 安全边界校验与简易通配符

- **`isSubpath(alloc, base_dir, target_path, style): bool ! PathError`**：按归一化分段校验归属关系，彻底免疫 `/var/www` 与 `/var/www2` 前缀陷阱，Windows 下分段与根大小写不敏感。
- **`safeJoin(alloc, base_dir, rel_parts, style): string ! PathError`**：拦截空字节（`NullByteFound`）、绝对路径/盘符注入及 `../` 越界逃逸（`PathTraversal`）。
- **`matchGlobSimple(path, pattern): bool`**：零堆分配 `*` / `?` 通配符匹配。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { PathStyle, parse, normalize, safeJoin, isSubpath } from "cap:paths";

pub fn demoPaths(): ! {
    const alloc = std.heap.page_allocator;

    const p = parse("C:\\Users\\demo\\file.tar.gz", PathStyle.Windows);
    * = p.name; // "file.tar"

    const norm = normalize(alloc, "/var/www/../lib//app/./index.cp", PathStyle.Posix)?;
    defer alloc.free(norm);

    const safe = safeJoin(alloc, "/srv/assets", &[_]string{ "images", "logo.png" }, PathStyle.Posix)?;
    defer alloc.free(safe);

    const inside = isSubpath(alloc, "/srv/assets", safe, PathStyle.Posix)?;
    * = inside; // true
}
```
