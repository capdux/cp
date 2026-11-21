# `cap:alt_svc`

纯 CP 零堆分配 RFC 7838 HTTP `Alt-Svc` 头部解析器与 TTL 缓存（用于 HTTP/3 升级发现），吸收自 `/data/dev/bun/src/http/h3_client/AltSvc.cp`。

## 导出符号

- `DEFAULT_MAX_AGE`（86400 秒）
- `AltSvcEntry`：`{ protocol, host, port, max_age, persist }` + `isH3()` / `isSameHost()`
- `AltSvcHeader`：`parse(field_value)`（支持 `clear`、多协议列表、引号权威、`; ma=` 与 `; persist=1`）与 `firstSameHostH3()`
- `AltSvcCache`：固定 64 源零分配 TTL 缓存，提供 `record(host, port, header, now_sec)`、`lookup(host, port, now_sec)` 与 `remove`
