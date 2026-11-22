# `cap:pkcs12` — 零分配 RFC 7292 PKCS#12 (`.p12`/`.pfx`) ASN.1 DER 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

> **模块路径**：`lib/pkcs12/`（入口 `lib/pkcs12/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，严格遵循 RFC 7292 (PKCS#12 v1.1) 与 ASN.1 DER 编码规范：支持 `PFX` v3 容器校验与解包、`MacData`（`DigestInfo` + `macSalt` + 缺省为 `1` 的 `iterations`）解析、`AuthenticatedSafe`（`id-data` 明文与 `id-encryptedData` 密文）零拷贝迭代，以及 `SafeContents` / `SafeBag`（含 `localKeyId` 与 `friendlyName` UTF-16BE `BMPString` 属性提取）零拷贝迭代。

---

## 一、导入方式

```cp
import {
    OID_DATA,
    OID_ENCRYPTED_DATA,
    OID_KEY_BAG,
    OID_SHROUDED_KEY_BAG,
    OID_CERT_BAG,
    OID_CRL_BAG,
    OID_SECRET_BAG,
    OID_FRIENDLY_NAME,
    OID_LOCAL_KEY_ID,
    Pkcs12BagKind,
    Pkcs12MacData,
    Pkcs12Pfx,
    Pkcs12SafeContent,
    Pkcs12Bag,
    Pkcs12SafeIter,
    Pkcs12BagIter,
    parsePkcs12Der,
    isPkcs12Der,
} from "cap:crypto";
```

---

## 二、核心常量与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| ContentType OID | `OID_DATA`, `OID_ENCRYPTED_DATA` | `1.2.840.113549.1.7.{1,6}` 原始 DER OID 编码字节 |
| SafeBag OID | `OID_KEY_BAG`, `OID_SHROUDED_KEY_BAG`, `OID_CERT_BAG`, `OID_CRL_BAG`, `OID_SECRET_BAG` | `1.2.840.113549.1.12.10.1.{1..5}` 原始 DER OID 编码字节 |
| PKCS#9 Attribute OID | `OID_FRIENDLY_NAME`, `OID_LOCAL_KEY_ID` | `1.2.840.113549.1.9.{20,21}` 原始 DER OID 编码字节 |
| `Pkcs12BagKind: u8` | `key_bag`, `shrouded_key_bag`, `cert_bag`, `crl_bag`, `secret_bag`, `unknown` | `SafeBag.bagId` 分类枚举 |
| `Pkcs12MacData` | `digest_alg_oid`, `digest`, `mac_salt`, `iterations`, `present` | 可选 `MacData` 视图（省略 `iterations` 时按 RFC 7292 缺省为 `1`） |
| `Pkcs12Pfx` | `version`, `auth_safe_type_oid`, `auth_safe_data`, `mac_data`; `safeContents()` | 外层 `PFX` v3 视图（若 `authSafe` 为 `OID_DATA` 已自动剥离 `[0] EXPLICIT OCTET STRING`） |
| `Pkcs12SafeContent` | `content_type_oid`, `is_encrypted`, `content_der`; `bags()` | `AuthenticatedSafe` 中单个 `ContentInfo` 视图 |
| `Pkcs12Bag` | `bag_kind`, `bag_id_oid`, `bag_value_der`, `local_key_id`, `friendly_name_bmp` | 单个 `SafeBag` 视图（已剥离 `[0] EXPLICIT` 并提取 PKCS#9 属性） |

---

## 三、核心 API

- `parsePkcs12Der(der_data: []const u8): Pkcs12Pfx?` — 严格 DER 解析 `PFX` 容器，校验 `version == 3`、解包 `authSafe` 并解析可选 `macData`。
- `isPkcs12Der(der_data: []const u8): bool` — 快速判定输入是否为合法的 RFC 7292 v3 `PFX` DER 容器。
- `Pkcs12SafeIter` — 零分配迭代 `AuthenticatedSafe`（`SEQUENCE OF ContentInfo`），提供 `init(...)`、`next(*_)` 与 `hasError()`。
- `Pkcs12BagIter` — 零分配迭代明文 `SafeContents`（`SEQUENCE OF SafeBag`），提供 `init(...)`、`next(*_)` 与 `hasError()`。

---

## 四、使用示例

```cp
import { parsePkcs12Der, Pkcs12BagKind } from "cap:crypto";

fn inspectPfx(der: []const u8) {
    if (pfx := parsePkcs12Der(der)) {
        if (pfx.mac_data.present) {
            * = pfx.mac_data.digest_alg_oid;
            * = pfx.mac_data.iterations;
        }
        let safes = pfx.safeContents();
        while (sc := safes.next()) {
            if (!sc.is_encrypted) {
                let bags = sc.bags();
                while (bag := bags.next()) {
                    if (bag.bag_kind == Pkcs12BagKind.cert_bag) {
                        * = bag.bag_value_der;
                        * = bag.local_key_id;
                        * = bag.friendly_name_bmp;
                    }
                }
            }
        }
    }
}
```
