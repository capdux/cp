# `cap:deb` — 纯 CP Debian `.deb` 软件包与 `DEBIAN/control` 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

## 概述

`cap:deb` 提供纯 CP、零外部依赖、零堆分配的 Debian `.deb`（基于 Unix `ar` 归档）容器解析与 RFC 822 `DEBIAN/control` 元数据提取库：
- **`.deb` 容器解析**：严格校验 `"!<arch>\n"` 魔数、60 字节成员头与 `` `\n `` 结尾、2 字节偶对齐，零拷贝提取 `debian-binary`（`2.0`）、`control.tar*` 与 `data.tar*` 成员并自动识别压缩算法（`.tar`/`.gz`/`.xz`/`.zst`/`.bz2`/`.lzma`）；
- **`DEBIAN/control` 解析**：`DebControlIter` 流式解析 `Key: Value` 与多行续行字段，`parseDebControl` 大小写不敏感一键提取 `package`、`version`、`architecture`、`maintainer`、`depends`、`section`、`priority`、`installed_size` 与 `description`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `DEB_AR_MAGIC`, `DEB_BINARY_MEMBER` | `ar` 归档魔数（`"!<arch>\n"`）与首成员名常量（`"debian-binary"`） |
| `DebCompression` | 压缩格式枚举：`none`, `gzip`, `xz`, `zstd`, `bzip2`, `lzma`, `unknown` |
| `isDeb(data)` | 探测合法 `.deb` 软件包（拒绝普通 `.a` 静态库） |
| `DebPackage` / `parseDebPackage(data)` | 零拷贝提取 `format_version`、`control_*`、`data_*` 成员视图 |
| `DebControlField` / `DebControlIter` | RFC 822 字段零拷贝流式迭代器（自动合并多行续行，忽略空行与 `#` 注释） |
| `DebControlMeta` / `parseDebControl(control_text)` | 一键提取核心元数据（含 `isValid()` 必填项校验） |

## 示例

```cp
import { isDeb, parseDebPackage, parseDebControl } from "cap:devops";

fn inspectDeb(deb_bytes: []const u8, control_text: []const u8): bool {
    if (!isDeb(deb_bytes)) return false;
    if (pkg := parseDebPackage(deb_bytes)) {
        const meta = parseDebControl(control_text);
        return pkg.data_data.len > 0 && meta.isValid();
    }
    return false;
}
```
