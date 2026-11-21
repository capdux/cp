# cap:http_signature_input

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9421 HTTP Message Signatures `Signature-Input` 与 `Signature` 头部规范），提供纯 CP、零外部依赖、零堆分配的流式解析与时间窗校验库。

## 导入方式

```cp
import {
    SignatureInput,
    SignatureInputIter,
    ComponentIter,
    findSignatureInput,
    findSignatureValue,
} from "cap:http_signature_input";
```

## 核心类型与 API

- `SignatureInput`：
  - `label: []const u8`：签名标签（如 `"sig1"`）。
  - `components_raw: []const u8`：括号内覆盖组件原始列表（如 `"@method" "@authority" "content-digest"`）。
  - `created: i64?`：签名创建时间（Unix 秒）。
  - `expires: i64?`：签名过期时间（Unix 秒）。
  - `nonce: ([]const u8)?`：防重放随机串。
  - `alg: ([]const u8)?`：签名算法标识（如 `"rsa-pss-sha512"`、`"ed25519"`）。
  - `keyid: ([]const u8)?`：密钥标识。
  - `tag: ([]const u8)?`：应用层标签。
  - `isActiveAt(now_unix_sec: i64, leeway_sec: i64): bool`：校验 `created` / `expires` 时间窗（含时钟偏移容忍与倒置时间窗防护）。
- `SignatureInputIter`：`init(header: []const u8)` / `next(*_): SignatureInput?`，逗号分隔多签名流式迭代，支持括号与引号感知。
- `findSignatureInput(header: []const u8, label: []const u8): SignatureInput?`：按标签检索对应 `SignatureInput`。
- `ComponentIter`：`init(components_raw: []const u8)` / `next(*_): ([]const u8)?`，流式提取每个带引号的覆盖组件名，自动跳过组件后缀参数（如 `;req`、`;name="id"`）。
- `findSignatureValue(sig_header: []const u8, label: []const u8): ([]const u8)?`：从 `Signature: sig1=:base64:` 头中提取指定标签对应的 Base64 字节序列。

## 使用示例

```cp
import {
    SignatureInputIter,
    ComponentIter,
    findSignatureInput,
    findSignatureValue,
} from "cap:http_signature_input";

fn example() {
    const input_hdr =
        "sig1=(\"@method\" \"@authority\" \"content-digest\");created=1700000000;expires=1700000300;alg=\"ed25519\";keyid=\"k1\"";
    const sig_hdr = "sig1=:K2qGT5srn2OGbOIDzQ6kYT+ruWRW6yPZWppG5c1o=:";

    if (sig_in := findSignatureInput(input_hdr, "sig1")) {
        if (sig_in.isActiveAt(1700000100, 30)) {
            let comps = ComponentIter.init(sig_in.components_raw);
            while (comp := comps.next()) {
                * = comp;
            }
            if (b64 := findSignatureValue(sig_hdr, "sig1")) {
                * = b64;
            }
        }
    }
}
```
