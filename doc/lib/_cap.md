# CP 标准库：`cap` 根命名空间惰性挂载容器与零依赖微核心手册 (`lib/_cap` / `"cap"`)

> 参考来源：`/data/dev/bun/src/bun.cp` | 吸收日期：2026-11-21

## 一、 架构总览与设计准则

`lib/_cap` 是 CP 语言统一根命名空间 `import cap from "cap"`（或 `import c from "cap"`、`const cap = @import("cap")`）的底层微核心与静态入口容器。`cpc` 编译器对其执行**编译期按需惰性挂载（Compile-Time Lazy Namespace Mounting）**：

- **零重依赖微核心（`lib/_cap/core.cp`）**：仅包含极薄的高频基础原语（内存与 OOM 处理、编译期平台常量、断言与全局进程控制、数值与切片原语、底层 FD/POSIX 标志位常量、轻量输出与编译期字符串查找表），保证导入 `"cap"` 的解析与转译开销接近于零。
- **按需惰性路由（`lib/_cap/index.cp`）**：仅将可达代码图中实际通过 `<alias>.<ns>` 引用的标准库子命名空间（如 `cap.strings`、`cap.sys`、`cap.http`、`cap.net`、`cap.crypto` 等 50+ 子库）纳入解析与转译；未被引用的标准库完全不读盘、不转译、不参与编译。

### 1. 模块文件拓扑

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/_cap/core.cp` | 零依赖微核心原语实现（内存/OOM、环境常量、断言、数值/切片、FD/POSIX 常量、轻量输出与 `ComptimeStringMap`） |
| `lib/_cap/index.cp` | 静态入口与 59 个惰性子命名空间路由说明，重导出 `./core.cp` |

---

## 二、 核心 API 速查

### 1. 内存与 OOM 处理
- `OOM`：`std.mem.Allocator.Error` 类型别名
- `default_allocator` / `page_allocator` / `DefaultAllocator.allocator()`：默认页分配器
- `outOfMemory(): noreturn`：冷路径 OOM 终止
- `handleOom(error_union_or_set)`：解包错误联合/结果，遇 `OutOfMemory` 直接 panic
- `create(allocator, T, value)` / `destroy(ptr)` / `destroyWith(allocator, ptr)`：单对象分配与销毁
- `take(ptr)`：原子语义取出 `*?T` 中的值并置 `null`
- `copy(T, dest, src)` / `zero(Type)`：重叠安全字节拷贝与全零初始化

### 2. 环境与平台编译期常量（`Environment`）
- 操作系统：`isLinux`、`isMac`、`isDarwin`、`isWindows`、`isFreeBSD`、`isWasm`、`isPosix`、`os_tag`
- 构建模式：`isDebug`、`isRelease`、`isTest`、`allow_assert`、`mode`
- 架构位宽：`isX64`、`isX86`、`isAarch64`、`is64Bit`、`arch`

### 3. 断言、进程与数值切片原语
- 断言与全局：`debugAssert(ok)`、`assert(ok)`、`assertWithLocation(ok, src)`、`Global.exit(code)`、`Global.panic(fmt, args)`
- 数值与切片：`clamp(val, min, max)`、`cast(To, val)`、`range(min, max)`、`asByteSlice(buf)`、`constStrToU8(s)`、`sliceTo`

### 4. 底层路径、FD 与 POSIX 常量
- 类型与常量：`FD`（`i32`）、`invalid_fd`（`-1`）、`Mode`（`u32`）、`MAX_PATH_BYTES`（`4096`）、`PathBuffer`（`[4096]u8`）
- `O`：`RDONLY`、`WRONLY`、`RDWR`、`CREAT`、`EXCL`、`TRUNC`、`APPEND`、`NONBLOCK`、`DIRECTORY`、`CLOEXEC`
- `S`：`IFMT`、`IFIFO`、`IFCHR`、`IFDIR`、`IFBLK`、`IFREG`、`IFLNK`、`IFSOCK` 及 `IRWXU`/`IRUSR`/`IWUSR`/`IXUSR` 等权限位

### 5. 轻量输出与编译期映射
- `Output`：`print`、`println`、`errorln`、`debug`、`flush`
- `ComptimeStringMap(V, kvs_list)`：按长度分桶的编译期静态字符串查找表（`get` / `has`）
- `fmtIntSize(bytes, buf)`：字节数格式化为可读单位（`B` / `KB` / `MB` / `GB` / `TB` / `PB`）

---

## 三、 使用示例

```cp
import cap from "cap";

pub fn main() {
    if (comptime cap.Environment.isLinux) {
        cap.Output.println("Running on Linux 64-bit: {}", .{cap.Environment.is64Bit});
    }
    const clamped = cap.clamp(@as(i32, 150), 0, 100);
    cap.assert(clamped == 100);
}
```

---

## 收敛子模块深度文档

- [cap_hub.md](_cap/cap_hub.md)
