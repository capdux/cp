# `cap:etag`

> 参考来源：/data/dev/bun/src/http_types/ETag.cp | 吸收日期：2026-11-22

纯 CP 零堆分配 RFC 9110 / RFC 7232 HTTP ETag 解析、强/弱比较、`If-None-Match` / `If-Match` 条件请求求值与 FNV-1a 64-bit 内容 ETag 生成库。

## 导出符号

- `ParsedETag`：`parse(input)`、`weakMatch(other)`、`strongMatch(other)`、`toHeader(out)`
- `ifNoneMatch(etag, header_val)`：支持 `*`、多值列表（含引号内逗号保护）与 RFC 9110 弱比较
- `ifMatch(etag, header_val)`：支持 `*`、多值列表与 RFC 9110 强比较
- `generateStrong(content, out)` / `generateWeak(content, out)`：生成 64-bit 十六进制强/弱 ETag

## 上游行为契约与条件请求边界

1. **RFC 9110 强比较 vs 弱比较语义**：
   - **弱比较 (`weakMatch` / `ifNoneMatch`)**：忽略 `W/` 前缀仅比较不透明标签内容，用于 `GET`/`HEAD` 缓存验证（命中时返回 `304 Not Modified`）；
   - **强比较 (`strongMatch` / `ifMatch`)**：要求双方均非弱 ETag 且内容完全一致，用于 `PUT`/`PATCH` 乐观锁并发更新（不命中时返回 `412 Precondition Failed`）与 `If-Range` 断点续传校验。
2. **多值列表引号内逗号保护**：
   - `ifNoneMatch` 与 `ifMatch` 词法扫描逗号分隔列表时正确跳过双引号内的逗号（如 `"a,b", W/"c"`），不会误切分实体标签；通配符 `*` 匹配任意已存在资源。
3. **零堆分配格式化缓冲**：
   - `toHeader`、`generateStrong`、`generateWeak` 均写入调用方提供缓冲，空间不足安全返回 `null`。
