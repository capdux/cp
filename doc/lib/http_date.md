# `cap:http_date`

纯 CP 零堆分配 RFC 9110 §5.6.7 HTTP-date（IMF-fixdate `Sun, 06 Nov 1994 08:49:37 GMT`）解析与格式化库，内置 Howard Hinnant 历法算法实现与 Unix 时间戳精确互转，吸收自 `/data/dev/bun/src/http/Headers.cp`。

## 导出符号

- `HttpDateTime`：`fromUnixSeconds(epoch)`、`toUnixSeconds()`、`formatImf(out)`、`parseImf(input)`（含闰年天数校验）
- `formatHttpDate(epoch_sec, out)` / `parseHttpDate(input)`
