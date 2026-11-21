# `cap:http_link_header`

纯 CP 零堆分配 RFC 8288 HTTP `Link` 头部解析器与格式化库（支持 REST 分页、Early Hints/Preload 与引号参数保护），吸收自 `/data/dev/bun/src/http/HeaderValueIterator.cp`。

## 导出符号

- `WebLink`：`{ uri, rel, as_dest, media_type, title, crossorigin, nopush }`
- `LinkHeaderIterator`：`init(header)` / `next()`
- `findLinkByRel(header, rel)`：按关系类型（大小写无关）零分配查找首个链接
- `formatWebLink(link, out)`：格式化标准 RFC 8288 `Link` 头部
