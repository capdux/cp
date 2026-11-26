# @cap/methods

零堆分配的 CP 原生 HTTP/WebDAV 方法表与 RFC 9110 谓词库，Clean-Room MIT 实现对标 npm 顶流包 `methods`（1.2 亿+/周下载，Express / Koa 核心依赖）。

## 特性

- 完整收录 34 个 RFC 9110 / WebDAV 标准 HTTP 方法，O(log n) 大小写不敏感二分查找
- `toLowerMethod` / `toUpperMethod` 零分配规范化
- RFC 9110 §9.2 `isSafeMethod` / `isIdempotentMethod` / `isCacheableMethod` / `mayHaveRequestBody` 语义谓词

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`jshttp/methods`](https://github.com/jshttp/methods)。
