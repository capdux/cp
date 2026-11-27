[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `sec1`

CP 零堆分配 SEC1 v2.0 椭圆曲线点编码与 RFC 5915 `ECPrivateKey` DER/PEM 编解码库 —— 对齐 RustCrypto [`sec1`](https://github.com/RustCrypto/formats)（v0.7）的纯 CP MIT 净室实现，底层组合 `const-oid`、`pem-rfc7468` 与 `subtle`。

## 安装与导入

```bash
cap add sec1
```

```cp
import {
    SEC1_IDENTITY, SEC1_COMPRESSED_EVEN, SEC1_COMPRESSED_ODD, SEC1_UNCOMPRESSED,
    SEC1_MAX_POINT_LEN, SEC1_MAX_PRIV_LEN,
    SEC1_P256_COORD_LEN, SEC1_P384_COORD_LEN, SEC1_P521_COORD_LEN,
    SEC1_PEM_LABEL,
    SEC1_OID_PRIME256V1, SEC1_OID_SECP256K1, SEC1_OID_SECP384R1, SEC1_OID_SECP521R1,
    SEC1_OK, SEC1_ERR_BUFFER_TOO_SMALL, SEC1_ERR_INVALID_POINT_TAG,
    SEC1_ERR_INVALID_POINT_LEN, SEC1_ERR_INVALID_DER, SEC1_ERR_INVALID_VERSION,
    SEC1_ERR_INVALID_PRIV_KEY, SEC1_ERR_INVALID_CURVE_OID, SEC1_ERR_INVALID_PUB_KEY,
    SEC1_ERR_INVALID_PEM, SEC1_ERR_INVALID_PEM_LABEL, SEC1_ERR_TRAILING_DATA,
    PointResult, EncodedPoint, EcPrivateKey, EcPrivateKeyResult,
    makeEcPrivateKey, encodeEcPrivateKeyDer, decodeEcPrivateKeyDer,
    encodeEcPrivateKeyPem, decodeEcPrivateKeyPem,
} from "sec1";
```

## 核心 API

### 常量与错误码

| 分类 | 常量 | 说明 |
| :--- | :--- | :--- |
| 点格式标签 (`u8`) | `SEC1_IDENTITY` (0x00), `SEC1_COMPRESSED_EVEN` (0x02), `SEC1_COMPRESSED_ODD` (0x03), `SEC1_UNCOMPRESSED` (0x04) | SEC1 v2.0 压缩/未压缩点前缀 |
| 曲线尺寸与标签 | `SEC1_MAX_POINT_LEN` (133), `SEC1_MAX_PRIV_LEN` (66), `SEC1_P256_COORD_LEN` (32), `SEC1_P384_COORD_LEN` (48), `SEC1_P521_COORD_LEN` (66), `SEC1_PEM_LABEL` (`"EC PRIVATE KEY"`) | 覆盖 P-256 / secp256k1 / P-384 / P-521 |
| 曲线 OID | `SEC1_OID_PRIME256V1`, `SEC1_OID_SECP256K1`, `SEC1_OID_SECP384R1`, `SEC1_OID_SECP521R1` | 点分 OID 字符串 |
| 错误码 (`i64`) | `SEC1_OK` (0), `SEC1_ERR_BUFFER_TOO_SMALL` (-1) .. `SEC1_ERR_TRAILING_DATA` (-11) | 编解码失败负错误码 |

### `EncodedPoint` 与 `EcPrivateKey`

| 符号 | 签名 / 方法 | 说明 |
| :--- | :--- | :--- |
| `EncodedPoint` 构造 | `identity()`, `fromUncompressed(x, y): PointResult`, `fromCompressed(x, y_is_odd): PointResult`, `fromBytes(bytes): PointResult`, `fromBytesForCoord(bytes, expected_coord_size): PointResult` | 构造或解析 SEC1 曲线点（`buf: [133]u8`, `len`, `coord_size`） |
| `EncodedPoint` 查询 | `tag(): u8`, `isIdentity()`, `isCompressed()`, `isUncompressed()`, `xBytes(out): usize`, `yBytes(out): usize`, `toBytes(out): usize`, `compress(): PointResult`, `eq(other): bool` | 读取坐标、序列化或将未压缩点转为压缩点 |
| `EcPrivateKey` | `empty()`, `zeroize(*_)` | RFC 5915 私钥结构（`priv_key: [66]u8`、`curve_oid`、`pub_key`）；`zeroize` 常量时间擦除私钥内存 |
| `makeEcPrivateKey` | `(priv_bytes: []const u8, has_curve: bool, curve_oid: ObjectIdentifier, has_pub_key: bool, pub_key: EncodedPoint): EcPrivateKeyResult` | 带校验的私钥构造器 |
| `encodeEcPrivateKeyDer` / `decodeEcPrivateKeyDer` | `(key, out: []u8): i64` / `(der: []const u8): EcPrivateKeyResult` | RFC 5915 DER 编解码 |
| `encodeEcPrivateKeyPem` / `decodeEcPrivateKeyPem` | `(key, crlf: bool, out: []u8): i64` / `(pem: string): EcPrivateKeyResult` | RFC 7468 PEM 编解码（`-----BEGIN EC PRIVATE KEY-----`） |

## 代码示例

### 1. 压缩未压缩的 P-256 SEC1 椭圆曲线点

```cp
import { print } from "cap:io";
import { EncodedPoint } from "sec1";

fn compressPoint(x: []const u8, y: []const u8) {
    const res = EncodedPoint.fromUncompressed(x, y);
    if (!res.ok) {
        print("invalid point, err:", res.err);
        return;
    }
    const comp = res.point.compress();
    if (comp.ok) {
        let out: [33]u8 = undefined;
        const n = comp.point.toBytes(out[0..33]);
        print("compressed tag:", comp.point.tag(), "len:", n);
    }
}
```

### 2. 解析 `EC PRIVATE KEY` PEM 并在退出时自动清零私钥

```cp
import { print } from "cap:io";
import { decodeEcPrivateKeyPem, encodeEcPrivateKeyDer } from "sec1";

fn reencodePrivateKey(pem: string) {
    const dec = decodeEcPrivateKeyPem(pem);
    if (!dec.ok) {
        print("decode failed, err:", dec.err);
        return;
    }
    // 必须用 `let` 声明以便 `defer key.zeroize()` 执行安全擦除
    let key = dec.key;
    defer key.zeroize();

    let der: [256]u8 = undefined;
    const n = encodeEcPrivateKeyDer(key, der[0..256]);
    print("DER length:", n, "has_curve:", key.has_curve);
}
```

## 与源包差异及性能亮点

- **零堆分配与抗优化内存擦除**：私钥标量与曲线点全部存放在栈上，调用 `key.zeroize()`（底层调用 `subtle.zeroizeBytes`）配合 `defer` 确保密钥不滞留内存。
- **严格 RFC 5915 校验**：校验版本号（必须为 `1`）、私钥长度、可选 `[0]` 曲线 OID 与 `[1]` 公钥位串，并拒绝尾随多余字节。
- **全主流曲线支持**：原生覆盖 P-256（32B）、secp256k1（32B）、P-384（48B）与 P-521（66B）。
