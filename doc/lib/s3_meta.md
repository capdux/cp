# cap:s3_meta

> 源码吸收自 `/data/dev/bun/src/s3_signing/{acl,storage_class,credentials}.cp`，提供纯 CP、零外部依赖、零堆分配的 AWS S3 / R2 / MinIO 对象存储元数据与 URI 工具库。

## 导入方式

```cp
import { S3Acl, S3StorageClass, isValidBucketName, S3Uri } from "cap:s3_meta";
```

## 核心类型与 API

### `S3Acl`（8 种标准策略）

枚举成员：`private_acl`（`"private"`）、`public_read`（`"public-read"`）、`public_read_write`（`"public-read-write"`）、`aws_exec_read`（`"aws-exec-read"`）、`authenticated_read`（`"authenticated-read"`）、`bucket_owner_read`（`"bucket-owner-read"`）、`bucket_owner_full_control`（`"bucket-owner-full-control"`）、`log_delivery_write`（`"log-delivery-write"`）。

- `S3Acl.parse(s: []const u8): S3Acl?`
- `asStr(): []const u8`

### `S3StorageClass`（11 种存储类别）

枚举成员：`STANDARD`、`STANDARD_IA`、`INTELLIGENT_TIERING`、`EXPRESS_ONEZONE`、`ONEZONE_IA`、`GLACIER`、`GLACIER_IR`、`REDUCED_REDUNDANCY`、`OUTPOSTS`、`DEEP_ARCHIVE`、`SNOW`。

- `S3StorageClass.parse(s: []const u8): S3StorageClass?`
- `asStr(): []const u8`
- `isArchiveTier(): bool`（`GLACIER` / `GLACIER_IR` / `DEEP_ARCHIVE` 返回 `true`）

### `isValidBucketName(name: []const u8): bool`

严格遵循 AWS S3 Bucket 命名规则：长度 `3..63`、仅小写字母/数字/连字符/点、首尾必须为小写字母或数字、禁止相邻双点 `..` 与 IPv4 点分格式。

### `S3Uri`

- 字段：`bucket: []const u8`、`key: []const u8`、`region: ([]const u8)?`
- `S3Uri.parse(input: []const u8): S3Uri?`：支持 `s3://bucket/key`、Virtual-Hosted-Style `https://<bucket>.s3.<region>.amazonaws.com/key` 与 Path-Style `https://s3.<region>.amazonaws.com/<bucket>/key`。
- `toCanonicalS3(out: []u8): ([]const u8)?`：零堆分配规范化输出 `s3://<bucket>/<key>`。

## 使用示例

```cp
import { S3Acl, S3StorageClass, S3Uri } from "cap:s3_meta";

fn example() {
    * = S3Acl.parse("bucket-owner-full-control");
    if (sc := S3StorageClass.parse("DEEP_ARCHIVE")) {
        * = sc.isArchiveTier(); // true
    }
    if (uri := S3Uri.parse("https://my-assets.s3.us-east-1.amazonaws.com/img/logo.png")) {
        let buf: [128]u8 = undefined;
        * = uri.toCanonicalS3(&buf); // "s3://my-assets/img/logo.png"
    }
}
```
