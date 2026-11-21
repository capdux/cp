# `cap:bfd` — 零分配 RFC 5880 BFD 双向转发检测协议解析库

> **模块路径**：`lib/bfd/`（入口 `lib/bfd/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 5880 Bidirectional Forwarding Detection 线协议规范：支持 24 字节控制包头、全诊断码/状态/标志位解码、Simple Password / Keyed MD5 / Meticulous Keyed MD5 / Keyed SHA-1 / Meticulous Keyed SHA-1 认证段解析，以及 RFC 5880 Detection Time 微秒级计算。

---

## 一、导入方式

```cp
import {
    BFD_UDP_PORT_SINGLE_HOP,
    BFD_UDP_PORT_ECHO,
    BFD_UDP_PORT_MULTI_HOP,
    BFD_VERSION,
    BFD_MIN_LEN,
    BfdDiag,
    BfdState,
    BfdAuthType,
    BfdAuth,
    BfdPacket,
    parseBfd,
    isBfd,
    detectionTimeUs,
} from "cap:bfd";
```

---

## 二、核心常量与数据结构

| 符号 | 说明 |
| :--- | :--- |
| 协议常量 | `BFD_UDP_PORT_SINGLE_HOP (3784)`, `BFD_UDP_PORT_ECHO (3785)`, `BFD_UDP_PORT_MULTI_HOP (4784)`, `BFD_VERSION (1)`, `BFD_MIN_LEN (24)` |
| `BfdDiag: u8` | `none (0)` ~ `reverse_concatenated_path_down (8)`, `unknown (255)` |
| `BfdState: u8` | `admin_down (0)`, `down (1)`, `init (2)`, `up (3)` |
| `BfdAuthType: u8` | `simple_password (1)`, `keyed_md5 (2)`, `meticulous_keyed_md5 (3)`, `keyed_sha1 (4)`, `meticulous_keyed_sha1 (5)` |
| `BfdAuth` | `auth_type`, `auth_len`, `auth_key_id`, `sequence_number`, `data`（零拷贝密码或摘要切片） |
| `BfdPacket` | `version`, `diag`, `raw_diag`, `state`, `poll`, `final_flag`, `control_plane_independent`, `auth_present`, `demand`, `multipoint`, `detect_mult`, `length`, `my_discriminator`, `your_discriminator`, `desired_min_tx_us`, `required_min_rx_us`, `required_min_echo_rx_us`, `auth: BfdAuth?` |

---

## 三、核心 API

- `parseBfd(data: []const u8): BfdPacket?` — 严格校验 RFC 5880 约束（`version == 1`、`detect_mult != 0`、`multipoint == false`、`my_discriminator != 0`、长度及认证段最小长度）并零拷贝解码。
- `isBfd(data: []const u8): bool` — 快速判定输入是否为合法的 RFC 5880 BFD 控制包。
- `detectionTimeUs(remote_min_tx: u32, local_min_rx: u32, detect_mult: u8): u64` — 计算协商检测超时时间（微秒）。

---

## 四、使用示例

```cp
import { parseBfd, detectionTimeUs } from "cap:bfd";

fn handleBfd(pkt: []const u8) {
    if (b := parseBfd(pkt)) {
        const dt = detectionTimeUs(b.desired_min_tx_us, 50000, b.detect_mult);
        * = dt;
        if (auth := b.auth) {
            * = auth.auth_key_id;
            * = auth.data;
        }
    }
}
```
