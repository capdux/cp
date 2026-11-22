# `cap:iot` — 统一工业自动化、车联网、嵌入式固件、设备发现与链路遥测标准库

> 聚合路径：`lib/iot/index.cp`（`import { ... } from "cap:iot"` / `import cap from "cap"; cap.iot.*`）

`cap:iot` 是 CP 标准库的统一工业协议、车联网、嵌入式微控制器固件、局域网零配置发现与工业以太网遥测主包，100% 纯 CP 零外部 C 依赖、零堆分配设计。

---

## 模块架构总览

| 分类 | 文件路径 | 标准 / 协议 | 说明 |
| :--- | :--- | :--- | :--- |
| 工业与车联网 | `lib/iot/can.cp` | ISO 11898 CAN 2.0/FD / SocketCAN / SAE J1939 / DBC | 车载总线帧编解码与 DBC 信号提取 |
| 工业与车联网 | `lib/iot/modbus.cp` | Modbus TCP (MBAP) & Modbus RTU (CRC-16) | 工业 PLC/传感器寄存器读写编解码 |
| 工业与车联网 | `lib/iot/dnp3.cp` | IEEE 1815 DNP3 | 电力/水利 SCADA 线缆协议与 CRC-16 编解码 |
| 工业与车联网 | `lib/iot/s7.cp` | Siemens S7comm / TPKT (RFC 1006) / COTP (ISO 8073) | 西门子 PLC 通信协议编解码 |
| 工业与车联网 | `lib/iot/knx.cp` | ISO/IEC 14543-3-2 KNXnet/IP / cEMI / DPT9 | 智能楼宇自动化协议编解码 |
| 嵌入式固件与遥测 | `lib/iot/ihex.cp` | Intel HEX (I8HEX / I16HEX / I32HEX) | 嵌入式固件烧录镜像编解码 |
| 嵌入式固件与遥测 | `lib/iot/srec.cp` | Motorola S-Record (S0–S9) | 嵌入式固件 S-Record 编解码 |
| 嵌入式固件与遥测 | `lib/iot/uf2.cp` | Microsoft UF2 | 微控制器 USB Bootloader 固件块编解码 |
| 嵌入式固件与遥测 | `lib/iot/fit.cp` | Garmin / ANT+ FIT 1.0 / 2.0 | 可穿戴/运动传感器二进制遥测解析 |
| 零配置设备发现 | `lib/iot/mdns.cp` | RFC 6762 mDNS & RFC 6763 DNS-SD | 局域网多播服务发现线缆编解码 |
| 零配置设备发现 | `lib/iot/ssdp.cp` | UPnP / DLNA SSDP | 智能设备发现通知与搜索报文编解码 |
| 零配置设备发现 | `lib/iot/llmnr.cp` | RFC 4795 LLMNR | 链路本地多播名称解析（常量别名 `LLMNR_CLASS_IN`/`LLMNR_TYPE_*`/`LLMNR_FLAG_TC`） |
| 零配置设备发现 | `lib/iot/lldp.cp` | IEEE 802.1AB LLDP | 链路层邻居发现 TLV 编解码 |
| 零配置设备发现 | `lib/iot/cdp.cp` | Cisco Discovery Protocol v1/v2 | 交换机邻居发现（常量别名 `CDP_TLV_*`/`CDP_CAP_*`） |
| 工业以太网与遥测 | `lib/iot/stp.cp` | IEEE 802.1D / 802.1w STP & RSTP | 生成树协议 BPDU 编解码 |
| 工业以太网与遥测 | `lib/iot/lacp.cp` | IEEE 802.1AX / 802.3ad LACPDU | 链路聚合控制协议编解码 |
| 工业以太网与遥测 | `lib/iot/bfd.cp` | RFC 5880 BFD | 双向转发检测线缆解析器 |
| 工业以太网与遥测 | `lib/iot/pim.cp` | RFC 7761 PIM-SM v2 | 组播路由协议线缆编解码 |
| 工业以太网与遥测 | `lib/iot/sflow.cp` | sFlow v5 | 交换机/边缘流量采样遥测解析器 |
| 工业以太网与遥测 | `lib/iot/vxlan.cp` | RFC 7348 VXLAN & VXLAN-GPE | 虚拟扩展局域网封装编解码 |
