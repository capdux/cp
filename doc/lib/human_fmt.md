# `cap:human_fmt`

纯 CP 零堆分配人类可读格式化工具库（字节大小、纳秒耗时、URL 敏感凭据/UUID/令牌脱敏与 CLI 表格边框渲染），吸收自 `/data/dev/bun/src/bun_core/fmt.cp`。

## 导出符号

- `formatBytes(bytes, out)`：自适应输出 `B` / `KB` / `MB` / `GB` / `TB`
- `formatDurationNs(ns, out)`：自适应输出 `ns` / `us` / `ms` / `s` / `mXs`
- `redactUrl(input, out)`：自动脱敏 URL 密码、`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` UUID 与 `npm_*` 令牌
- `TableStyle` / `formatTableBorder` / `formatTableRow`：Unicode 与 ASCII 终端表格渲染
