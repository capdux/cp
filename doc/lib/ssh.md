# CP 标准库：OpenSSH 公钥与 authorized_keys 策略引擎手册 (`cap:ssh`)

> 参考来源：`/data/dev/bun/src/install/hosted_git_info.cp`, OpenSSH RFC 4251/4253 | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:ssh` 是 CP 语言官方内置的纯 CP **OpenSSH 公钥 Wire 格式解析、标准 `SHA256:` 指纹计算与 `authorized_keys` 策略解析引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，复用 `cap:crypto`（SHA-256）与 `cap:encoding`（Base64），为 CP Git 服务端、DevOps 自动化与零信任访问网关提供工业级安全实现。既可通过 `import { ... } from "cap:ssh"` 导入，也支持 `import cap from "cap"; cap.ssh.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ssh/keys.cp` | `// Source: cp/lib/ssh/keys.cp (...)` | `SshError`、`SshKeyType`、`SshPublicKey`、`AuthorizedKeyEntry`、`AuthorizedKeys` |
| `lib/ssh/index.cp` | `// Source: cp/lib/ssh/index.cp (...)` | `cap:ssh` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与算法枚举

- **`SshError`**：
  - `InvalidPublicKey`：公钥行格式错误、缺少字段或算法特定 Wire 负载长度/曲线/格式不合法（如 `ssh-ed25519` 公钥长度非 32 字节）。
  - `InvalidBase64`：Base64 编码包含非法字符、错误填充或长度不合法。
  - `WireTruncated`：RFC 4251 4 字节大端序长度前缀越界或二进制 Wire 负载截断。
  - `AlgorithmMismatch`：外层声明算法名（如 `ssh-rsa`）与 Wire 内部首字段算法名（如 `ssh-ed25519`）不一致。
  - `UnsupportedKeyType`：未受支持的公钥算法类型。
  - `InvalidAuthorizedKeys`：`authorized_keys` 选项存在未闭合引号、空选项或缺少公钥主体。
  - `OutOfMemory`：内存分配失败。
- **`SshKeyType`**：`Ed25519`（`ssh-ed25519`）、`Rsa`（`ssh-rsa`）、`EcdsaP256`（`ecdsa-sha2-nistp256`）、`SkEd25519`（`sk-ssh-ed25519@openssh.com`）、`Unknown`。提供 `SshKeyType.fromAlgoName(name)` 与 `toAlgoName()`。

### 2. `SshPublicKey`（公钥解析、Wire 校验与 SHA-256 指纹）

- `SshPublicKey.parse(alloc: std.mem.Allocator, line: string): SshPublicKey ! SshError`：解析 `<algo> <base64> [comment]`，执行严格标准 Base64 解码与 RFC 4251 长度前缀 Wire 结构校验（校验内外部算法一致性及各算法密钥体长度/曲线/点格式）。
- `deinit(*_)`：释放内部持有的算法名、Wire 字节与注释切片。
- `fingerprintSha256(alloc: std.mem.Allocator): []u8 ! SshError`：计算标准 `"SHA256:<base64_no_pad>"` 指纹（与 `ssh-keygen -lf` 完全一致）。
- `format(alloc: std.mem.Allocator): []u8 ! SshError`：规范格式化输出 `<algo> <base64>[ <comment>]`。
- `equals(other: *const SshPublicKey): bool`：逐字节比对 `wire_blob` 判断两把公钥是否等价。

### 3. `AuthorizedKeyEntry` & `AuthorizedKeys`（策略解析与密钥匹配）

- **`AuthorizedKeyEntry`**：包含 `options_raw: string`、`command: string?`、`from_pattern: string?`、`principals: string?`、`no_port_forwarding: bool`、`no_pty: bool`、`cert_authority: bool`、`public_key: SshPublicKey` 及 `deinit(*_)`。
- `AuthorizedKeys.parseFile(alloc: std.mem.Allocator, content: string): []AuthorizedKeyEntry ! SshError`：多行解析 `authorized_keys`，自动跳过 `#` 注释与空行，支持引号内含逗号与转义引号 `\"` 的复杂选项（如 `command="git-shell -c \"...\"",no-pty,from="10.0.0.0/8",principals="deploy"`）。
- `AuthorizedKeys.freeEntries(alloc: std.mem.Allocator, entries: []AuthorizedKeyEntry)`：释放解析结果数组及全部条目资源。
- `AuthorizedKeys.matchKey(entries: []const AuthorizedKeyEntry, candidate: *const SshPublicKey): usize?`：在已授权列表中查找匹配候选公钥的条目下标，未命中返回 `null`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SshPublicKey, AuthorizedKeys } from "cap:ssh";
import cap from "cap";

pub fn demoSsh(): ! {
    const alloc = std.heap.page_allocator;

    const auth_file =
        \\# Production deploy key
        \\command="git-shell -c \"git-receive-pack\"",no-pty,no-port-forwarding,from="10.0.0.0/8" ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8= deploy@ci
    ;

    const entries = AuthorizedKeys.parseFile(alloc, auth_file)?;
    defer AuthorizedKeys.freeEntries(alloc, entries);

    let candidate = cap.ssh.SshPublicKey.parse(
        alloc,
        "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=",
    )?;
    defer candidate.deinit();

    const fp = candidate.fingerprintSha256(alloc)?;
    defer alloc.free(fp);

    if (idx := AuthorizedKeys.matchKey(entries, &candidate)) {
        _ = entries[idx].command;
    }
}
```

---

## 附加零分配子模块

- **`lib/ssh/config.cp`**（OpenSSH `ssh_config` 解析与 Host 匹配）：`SshHostConfig`、`matchHostPatterns`、`resolveSshConfig`、`getSshOption`
- **`lib/ssh/pubkey.cp`**（OpenSSH 公钥与 `authorized_keys` 零分配迭代）：`PubKeyType`、`parseKeyType`、`SshPubKey`、`AuthorizedKeysIter`

---

## 上游行为契约与安全边界注意事项

1. **外层与 Wire 内层算法名双重一致校验（`AlgorithmMismatch`）**：
   - `SshPublicKey` 解析时同时比对文本前缀算法名与 RFC 4251 二进制 Wire 负载首字段算法名，拒绝伪装算法头的畸形公钥；对 `ssh-ed25519` 严格校验 32 字节原始公钥长度。
2. **OpenSSH `SHA256:` 指纹无填充约定**：
   - `fingerprintSha256` 按 OpenSSH 标准输出 `SHA256:<base64-no-pad>`（去除末尾 `=` 填充），与 `ssh-keygen -lf` 输出 100% 一致。
3. **`ssh_config` 首次匹配优先（First-Match-Wins）**：
   - `resolveSshConfig` 遵循 OpenSSH 官方规则：多个匹配 `Host` 块中对同一选项**首个出现的值生效**，否定模式（`!pattern`）优先排除。
