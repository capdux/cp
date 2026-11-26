<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ajv-formats` ([https://github.com/ajv-validator/ajv-formats](https://github.com/ajv-validator/ajv-formats)) | 许可：`MIT`

# `@cap/ajv-formats`

零堆分配纯 CP JSON Schema Draft 2019-09 / 2020-12 字符串格式校验器，完整对标 npm **#205** `ajv-formats`（真实周下载 **2.12 亿/周**，AJV 生态核心校验依赖）。

## 核心 API

- 日期时间：`isDate`（RFC 3339 含闰年 2 月 29 日校验）、`isTime`（含闰秒与时区换算校验）、`isDateTime`
- 网络与标识：`isEmail`、`isHostname`（RFC 1034/1123）、`isIpv4`、`isIpv6`（支持 `::` 与混合 IPv4）、`isUuid`、`isUri`、`isJsonPointer`（RFC 6901）
- 统一分派：`validateFormat(format_name: string, value: string): bool`
