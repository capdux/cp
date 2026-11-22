# `cap:binfmt` — 二进制容器、可执行格式、科学数据与线协议序列化标准库

## 1. 概述

`cap:binfmt`（亦可通过 `import cap from "cap"` 访问 `cap.binfmt`）是 CP 语言纯原生、零 C 依赖的二进制容器与序列化格式聚合标准库，统一收敛四大类 16 个高频二进制解析与编解码引擎：

- **可执行文件与系统容器**：Apple Mach-O / Fat Binary（`macho.cp`）、PCAPng 抓包容器（`pcapng.cp`）、VMware VMDK 稀疏镜像（`vmdk.cp`）
- **分析列存与科学数据格式**：Unidata NetCDF CDF-1/2/5（`netcdf.cp`）、Apache Avro & OCF（`avro.cp`）、Apache Arrow IPC（`arrow.cp`）、Apache ORC（`orc.cp`）
- **通用与嵌入式线协议序列化**：MessagePack（`msgpack.cp`）、CBOR & Diagnostic Notation（`cbor_diag.cp`）、MongoDB BSON 1.1（`bson.cp`）、UBJSON Draft 12（`ubjson.cp`）、Postcard + COBS（`postcard.cp`）
- **诊断、覆盖率与日志格式**：JVM HPROF（`hprof.cp`）、Java Flight Recorder v2.0（`jfr.cp`）、LCOV `.info`（`lcov.cp`）、Redis AOF RESP（`aof.cp`）

## 2. 模块清单与核心符号

| 子模块 | 核心职责 | 主要公开符号（含防冲突别名） |
| :--- | :--- | :--- |
| `macho.cp` | Apple Mach-O 32/64 与 Universal Fat Binary 解析 | `MachoKind`, `MachoHeader`, `MachoLoadCmd`, `FatArchEntry`, `detectMachoKind`, `parseMachoHeader`, `MachoLoadCmdIter`, `FatArchIter` |
| `pcapng.cp` | PCAPng 抓包区块（SHB/IDB/EPB/Options）零分配解析 | `PcapngBlock`, `ShbInfo`, `IdbInfo`, `EpbPacket`, `PcapngOption`, `isPcapng`, `PcapngBlockIter`, `parseShb`, `parseIdb`, `parseEpb` |
| `vmdk.cp` | VMware VMDK SparseExtentHeader 与文本描述符解析 | `VmdkSparseHeader`, `isVmdkSparse`, `parseVmdkSparseHeader`, `VmdkExtent`, `VmdkExtentIter`, `VmdkDescriptor`, `parseVmdkDescriptor` |
| `netcdf.cp` | Unidata NetCDF CDF-1/CDF-2/CDF-5 头与维度/变量/属性迭代 | `NetcdfFormat`, `NetcdfType`, `NetcdfHeader`, `detectNetcdfFormat`, `parseNetcdfHeader`, `NetcdfDimIter`, `NetcdfAttrIter`, `NetcdfVarIter` |
| `avro.cp` | Apache Avro Schema 驱动二进制编解码与 OCF 容器 | `AVRO_DEFAULT_MAX_DEPTH`, `OCF_MAGIC`, `AvroSchema`, `AvroValue`, `AvroContainer`, `encodeDatum`, `decodeDatum`, `writeContainer`, `readContainer` |
| `arrow.cp` | Apache Arrow IPC Stream / File 与 FlatBuffers 元数据解析 | `ARROW_MAGIC`, `ArrowMessageMeta`, `ArrowFooterInfo`, `ArrowMessageIter`, `isArrowFile`, `isArrowStream`, `parseArrowFileFooter` |
| `orc.cp` | Apache ORC 列存 PostScript 与压缩块迭代 | `ORC_MAGIC`, `OrcCompression`, `OrcPostScript`, `OrcLayout`, `parseOrcPostScript`, `parseOrcLayout`, `OrcBlockIter` |
| `msgpack.cp` | MessagePack 二进制编解码器 | `MSGPACK_DEFAULT_MAX_DEPTH`, `MsgPackError`, `MsgValue`, `MsgPackWriter`, `MsgPackReader`, `msgpackEncode`, `msgpackDecode`, `msgpackDecodeWithMaxDepth` |
| `cbor_diag.cp` | RFC 8949 CBOR 编解码与规范诊断表示法 | `CBOR_DIAG_MAX_DEPTH`, `CborError`, `CborValue`, `CborEntry`, `CborTag`, `cborDiagEncode`, `cborDiagDecode`, `cborDiagEncodeCanonical`, `cborDiagToDiag` |
| `bson.cp` | MongoDB BSON 1.1 文档与 12 字节 ObjectId 编解码 | `BSON_MAX_DEPTH`, `ObjectId`, `BsonValue`, `BsonField`, `encodeDocument`, `decodeDocument`, `decodeDocumentWithMaxDepth`, `getField` |
| `ubjson.cp` | UBJSON Draft 12 零分配二进制编解码器 | `MAX_UBJ_DEPTH`, `UbjsonContainerHeader`, `UbjsonWriter`, `UbjsonReader` |
| `postcard.cp` | Postcard 嵌入式线协议、ZigZag 变长整数与 COBS 帧编解码 | `zigzagEncodeI32`/`I64`, `PostcardWriter`, `PostcardReader`, `cobsEncode`, `cobsEncodeFramed`, `cobsDecode` |
| `hprof.cp` | JVM HPROF 1.0.1/1.0.2 堆转储头与记录迭代 | `HprofBasicType`, `HprofHeader`, `HprofRecord`, `HprofStats`, `isHprof`, `parseHprofHeader`, `HprofRecordIter`, `summarizeHprof` |
| `jfr.cp` | JVM Java Flight Recorder v2.0 区块与事件帧解析 | `JFR_MAGIC`, `JfrChunkHeader`, `JfrEventFrame`, `JfrStats`, `isJfr`, `parseJfrChunkHeader`, `JfrChunkIter`, `JfrEventIter`, `summarizeJfr` |
| `lcov.cp` | LCOV `.info` 覆盖率轨迹文件解析与汇总 | `LcovRecordKind`, `LcovRecord`, `LcovFileSummary`, `LcovTotals`, `LcovRecordIter`, `LcovFileIter`, `summarizeLcov` |
| `aof.cp` | Redis AOF RESP 命令日志流解析与汇总 | `AofArgIter`, `AofCommand`, `AofIter`, `AofStats`, `isAofMixedRdb`, `isAofResp`, `summarizeAof` |

## 3. 使用示例

```cp
import {
    PostcardWriter,
    PostcardReader,
    cobsEncodeFramed,
    cobsDecode,
    isPcapng,
    summarizeLcov,
} from "cap:binfmt";

pub fn demoBinfmt() {
    let raw: [64]u8 = [0; 64];
    let w = PostcardWriter.init(raw[0..]);
    * = w.writeVarintU32(42);
    * = w.writeStr("cap:binfmt");

    let framed: [80]u8 = [0; 80];
    if (n := cobsEncodeFramed(w.written(), framed[0..])) {
        let decoded: [64]u8 = [0; 64];
        * = cobsDecode(framed[0..n], decoded[0..]);
    }
    * = isPcapng(raw[0..]);
    * = summarizeLcov("TN:\nSF:main.cp\nDA:1,1\nend_of_record\n");
}
```
