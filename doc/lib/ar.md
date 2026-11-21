# `cap:ar` — 纯 CP Unix `ar` / `.a` / `.deb` 归档容器零分配解析库

## 概述

`cap:ar` 提供纯 CP、零外部依赖、零堆分配的 Unix `ar` 归档解析器（适用于静态库 `.a` 与 Debian `.deb` 软件包），支持 8 字节 `"!<arch>\n"` 魔数校验、60 字节 ASCII 成员头解码、奇数成员 2 字节偶对齐填充跳过、GNU `"/"` 符号表与 `"//"` 长文件名表解引用以及 BSD `"#1/<len>"` 长文件名剥离。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `AR_MAGIC` / `AR_HEADER_SIZE` / `AR_END_MAGIC` | 常量：`"!<arch>\n"`、`60`、`` "`\n" `` |
| `ArMemberKind` | 成员类型枚举：`normal`, `gnu_symtab`, `gnu_strtab`, `bsd_symtab`, `unknown` |
| `ArMember` | 零拷贝成员：`raw_name`, `name`, `mtime`, `uid`, `gid`, `mode`, `size`, `kind`, `data` |
| `isAr(data)` | 校验 8 字节魔数 `"!<arch>\n"` |
| `ArIter` | 流式成员迭代器：`init(data)`、`next(*_): ArMember?`（自动支持 GNU/BSD 长名与 2B 对齐） |
| `findArMember(data, name)` | 按解析后的名称查找成员 |
| `countArMembers(data)` | 统计归档成员总数 |

## 示例

```cp
import { isAr, findArMember } from "cap:ar";

fn readDebVersion(deb_bytes: []const u8): ([]const u8)? {
    if (!isAr(deb_bytes)) return null;
    if (m := findArMember(deb_bytes, "debian-binary")) {
        return m.data;
    }
    return null;
}
```
