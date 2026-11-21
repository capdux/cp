# cap:http_max_forwards

> 源码吸收自 `/data/dev/bun/src/http/`（RFC 9110 §7.6.2 HTTP `Max-Forwards` 代理跳数限制规范），提供纯 CP、零外部依赖、零堆分配的 `Max-Forwards` 解析、代理转发裁定与十进制格式化库。

## 导入方式

```cp
import {
    MaxForwardsAction,
    MaxForwardsResult,
    parseMaxForwards,
    evaluateMaxForwards,
    formatMaxForwards,
} from "cap:http_max_forwards";
```

## 核心类型与 API

- `MaxForwardsAction`：枚举类型，包含 `forward`、`respond_directly`、`bad_request`。
- `MaxForwardsResult`：结构体，包含 `action: MaxForwardsAction` 与 `next_value: u32`。
- `parseMaxForwards(header: []const u8): u32?`：严格按 RFC 9110 §7.6.2 解析 `1*DIGIT`（仅允许首尾可选 OWS `' '` / `'\t'`），拒绝空值、负数、`+` 号、夹杂空白、CRLF、非数字及 `u32` 溢出。
- `evaluateMaxForwards(method: []const u8, header: ([]const u8)?): MaxForwardsResult`：按 RFC 9110 §7.6.2 仅对 `TRACE` 和 `OPTIONS` 生效；其他方法忽略并返回 `forward`；无头返回 `forward`；畸形头返回 `bad_request`；值为 `0` 返回 `respond_directly`；值 `> 0` 返回 `forward` 且 `next_value = val - 1`。
- `formatMaxForwards(val: u32, out: []u8): ([]const u8)?`：无前导零十进制输出，目标缓冲区空间不足时返回 `null`。

## 使用示例

```cp
import {
    MaxForwardsAction,
    evaluateMaxForwards,
    formatMaxForwards,
} from "cap:http_max_forwards";

fn example() {
    const res = evaluateMaxForwards("OPTIONS", "5");
    if (res.action == MaxForwardsAction.forward) {
        let buf: [16]u8 = undefined;
        * = formatMaxForwards(res.next_value, &buf); // "4"
    }
}
```
