# `cap:exact_matcher` — 定长整型打包 $O(1)$ 字符串快速匹配器

> 参考来源：/data/dev/bun/src/string/immutable/exact_size_matcher.cp | 吸收日期：2026-11-22

## 概述

`cap:exact_matcher` 吸收自 Bun 不可变字符串热点优化器（`src/string/immutable/exact_size_matcher.cp`），将 `1 / 2 / 4 / 8 / 12 / 16` 字节以内的短字符串以小端序（Little-Endian）零填充打包为单个无符号整数，使运行期关键字、HTTP 方法、协议头或文件扩展名匹配能够直接降级为单条整数寄存器比较或 LLVM 跳转表 `switch`，实现零堆分配、$O(1)$ 无分支分发。

## 上游打包规则与 CP 缺陷修正

- **小端零填充整型编码**：
  - `str.len == 0`：返回 `0`；
  - `0 < str.len < max_bytes`：将字符串拷贝至零初始化的 `[max_bytes]u8` 缓冲区后按小端序读取为 `u8` / `u16` / `u32` / `u64` / `u96` / `u128`；
  - `str.len == max_bytes`：直接按小端序读取整块字节；
  - `str.len > max_bytes`：返回 `maxInt(T)` 作为不匹配哨兵值（确保不会与任何不含全 `0xFF` 的合法编译期 `case(...)` 撞车）。
- **编译期长度守卫**：`case(comptime str)` 在编译期校验 `str.len <= max_bytes`，超长直接触发 `@compileError`，杜绝静默截断。
- **CP 关键修正**：Bun 上游 `exact_size_matcher.cp` 中 `matchLower` 在 `str.len == max_bytes` 分支直接读取原始字节而遗漏了 `toLower` 转换；CP 标准库已修正该边界缺陷，保证 `matchLower` 与 `packU*Lower` 在 `len == max_bytes` 时同样执行完整 ASCII 小写折叠。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `ExactSizeMatcher(max_bytes)` | 泛型定长匹配器（`max_bytes` 仅允许 `1, 2, 4, 8, 12, 16`），提供 `match(str)`、`matchLower(str)`、`case(comptime str)` |
| `Matcher4` / `Matcher8` / `Matcher16` | 常用 4 / 8 / 16 字节特化别名（底层 `u32` / `u64` / `u128`） |
| `packU32` / `packU32Lower` | 4 字节大小写敏感 / 不敏感整型打包 |
| `packU64` / `packU64Lower` | 8 字节大小写敏感 / 不敏感整型打包 |
| `packU128` / `packU128Lower` | 16 字节大小写敏感 / 不敏感整型打包 |

## 边界注意事项

1. **含 NUL 字节输入**：由于短于 `max_bytes` 的字符串右侧补 `0x00`，包含尾部 `\x00` 字节的字符串会与去掉尾部 `\x00` 的字符串映射到同一整数；该匹配器仅适用于不含 NUL 的文本标识符/关键字。
2. **仅 ASCII 大小写折叠**：`matchLower` 与 `packU*Lower` 仅对 ASCII `A-Z` 执行小写转换，非 ASCII 字节保持原样。
3. **哨兵值冲突**：避免匹配全 `0xFF` 的原始二进制序列（会与超长哨兵 `maxInt(T)` 相同）。

## 示例

```cp
import { Matcher8 } from "cap:exact_matcher";

const method = "DELETE";
const is_delete = switch (Matcher8.match(method)) {
    Matcher8.case("DELETE") => true,
    * => false,
};

const hdr = "Content-";
const is_content = switch (Matcher8.matchLower(hdr)) {
    Matcher8.case("content-") => true, // len == 8 同样正确小写折叠
    * => false,
};
* = is_delete;
* = is_content;
```
