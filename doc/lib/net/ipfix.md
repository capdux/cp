# `cap:ipfix` — RFC 7011 IPFIX v10 零分配线协议解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配的 IP Flow Information Export（IPFIX v10）16 字节消息头、Set（Template / Options Template / Data）、标准与 Enterprise Field Specifier 及 RFC 7011 §7 变长字段解码器。

## 导入

```cp
import {
    IPFIX_PORT,
    IPFIX_TLS_PORT,
    IPFIX_VERSION,
    IPFIX_HEADER_SIZE,
    IPFIX_SET_HEADER_SIZE,
    SET_ID_TEMPLATE,
    SET_ID_OPTIONS_TEMPLATE,
    SET_ID_DATA_MIN,
    IE_ENTERPRISE_BIT,
    IE_VARLEN_INDICATOR,
    IpfixHeader,
    IpfixFieldSpec,
    IpfixFieldSpecIter,
    IpfixTemplateRecord,
    IpfixTemplateIter,
    IpfixSet,
    IpfixSetIter,
    IpfixMessage,
    isIpfix,
    parseIpfix,
    IpfixVarLenValue,
    decodeIpfixVarLen,
} from "cap:net";
```

## 核心 API

- **消息解析**：`isIpfix(data)`、`parseIpfix(data): IpfixMessage?`（校验 `version == 10`、`16 <= length <= data.len` 及全部 Set 边界）。
- **Set 迭代**：`IpfixMessage.sets(): IpfixSetIter`、`IpfixSet`（`isTemplate()`, `isOptionsTemplate()`, `isData()`, `templates()`）。
- **模板与字段规范**：
  - `IpfixTemplateIter` — 迭代 Template Set 内多条模板并自动识别尾部零填充。
  - `IpfixFieldSpecIter` — 支持 4B 标准 IE 与 8B Enterprise IE（`isEnterprise()`, `isVariableLength()`）。
- **RFC 7011 §7 变长字段**：`decodeIpfixVarLen(data): IpfixVarLenValue?`（首字节 `< 255` 为 1B 长度，`== 255` 后接 2B BE 长度）。

## 示例

```cp
import { parseIpfix } from "cap:net";

fn inspect(wire: []const u8) {
    if (msg := parseIpfix(wire)) {
        let sit = msg.sets();
        while (true) {
            if (set := sit.next()) {
                if (set.isTemplate()) {
                    let tit = set.templates();
                    if (tpl := tit.next()) {
                        * = tpl.template_id;
                    }
                }
            } else {
                break;
            }
        }
    }
}
```
