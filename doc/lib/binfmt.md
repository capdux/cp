# `cap:binfmt` — 二进制容器、可执行格式、科学数据与线协议序列化标准库

## 1. 概述

`cap:binfmt`（亦可通过 `import cap from "cap"` 访问 `cap.binfmt`）是 CP 语言纯原生、零 C 依赖的二进制容器与序列化格式聚合标准库，统一收敛四大类 16 个高频二进制解析与编解码引擎：

- **可执行文件与系统容器**：Apple Mach-O / Fat Binary（`macho.cp`）、PCAPng 抓包容器（`pcapng.cp`）、VMware VMDK 稀疏镜像（`vmdk.cp`）
- **分析列存与科学数据格式**：Unidata NetCDF CDF-1/2/5（`netcdf.cp`）、Apache Avro & OCF（`avro.cp`）、Apache Arrow IPC（`arrow.cp`）、Apache ORC（`orc.cp`）
- **通用与嵌入式线协议序列化**：MessagePack（`msgpack.cp`）、CBOR & Diagnostic Notation（`cborDiag.cp`）、MongoDB BSON 1.1（`bson.cp`）、UBJSON Draft 12（`ubjson.cp`）、Postcard + COBS（`postcard.cp`）
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
| `msgpack.cp` | MessagePack 全 36 种 Marker 与 Timestamp (`ext=-1`) 编解码器（对标 `3Hren/msgpack-rust` MIT 加固 + 16-lane SIMD 载荷拷贝 & `cap:simd` ASCII UTF-8 快路径） | `MSGPACK_DEFAULT_MAX_DEPTH`, `MSGPACK_EXT_TIMESTAMP`, `MSGPACK_MAX_NSEC`, `MsgPackError`, `MsgExt`, `MsgTimestamp`, `MsgValue`, `MsgPackWriter`, `MsgPackSliceWriter`, `MsgPackReader`, `msgpackCopyPayloadSimd`, `msgpackIsValidUtf8Simd`, `msgpackEncode`, `msgpackDecode`, `msgpackDecodeWithMaxDepth`, `validateMsgpack`, `validateMsgpackWithMaxDepth` |
| `cborDiag.cp` | RFC 8949 CBOR 编解码与规范诊断表示法 | `CBOR_DIAG_MAX_DEPTH`, `CborError`, `CborValue`, `CborEntry`, `CborTag`, `cborDiagEncode`, `cborDiagDecode`, `cborDiagEncodeCanonical`, `cborDiagToDiag` |
| `bson.cp` | MongoDB BSON 1.1 文档与 12 字节 ObjectId 编解码 | `BSON_MAX_DEPTH`, `ObjectId`, `BsonValue`, `BsonField`, `encodeDocument`, `decodeDocument`, `decodeDocumentWithMaxDepth`, `getField` |
| `ubjson.cp` | UBJSON Draft 12 零分配二进制编解码器 | `MAX_UBJ_DEPTH`, `UbjsonContainerHeader`, `UbjsonWriter`, `UbjsonReader` |
| `postcard.cp` | Postcard 嵌入式线协议、ZigZag 变长整数与 COBS 帧编解码 | `zigzagEncodeI32`/`I64`, `PostcardWriter`, `PostcardReader`, `cobsEncode`, `cobsEncodeFramed`, `cobsDecode` |
| `hprof.cp` | JVM HPROF 1.0.1/1.0.2 堆转储头与记录迭代 | `HprofBasicType`, `HprofHeader`, `HprofRecord`, `HprofStats`, `isHprof`, `parseHprofHeader`, `HprofRecordIter`, `summarizeHprof` |
| `jfr.cp` | JVM Java Flight Recorder v2.0 区块与事件帧解析 | `JFR_MAGIC`, `JfrChunkHeader`, `JfrEventFrame`, `JfrStats`, `isJfr`, `parseJfrChunkHeader`, `JfrChunkIter`, `JfrEventIter`, `summarizeJfr` |
| `lcov.cp` | LCOV `.info` 覆盖率轨迹文件解析与汇总 | `LcovRecordKind`, `LcovRecord`, `LcovFileSummary`, `LcovTotals`, `LcovRecordIter`, `LcovFileIter`, `summarizeLcov` |
| `aof.cp` | Redis AOF RESP 命令日志流解析与汇总 | `AofArgIter`, `AofCommand`, `AofIter`, `AofStats`, `isAofMixedRdb`, `isAofResp`, `summarizeAof` |
| `dex.cp` | Android DEX 可执行头、MapList 与 MUTF-8 字符串表解析 | `DexHeader`, `Uleb128Result`, `DexMapItem`, `DexMapIter`, `isDex`, `parseDexHeader`, `readUleb128`, `readDexString` |
| `pcap.cp` | libpcap `.pcap` 容器与 L2-L4（Ethernet/IPv4/TCP/UDP）解析与写入 | `PcapError`, `PcapGlobalHeader`, `PcapPacket`, `PcapFile`, `PcapWriter`, `parsePcap`, `parseEthernet`, `parseIpv4`, `parseTcp`, `parseUdp` |
| `har.cp` | W3C HTTP Archive `.har` 1.2 零分配解析与汇总 | `HarHeader`, `HarTimings`, `HarEntry`, `HarStats`, `HarEntryIter`, `isHar`, `parseHarHeader`, `summarizeHar` |
| `warc.cp` | ISO 28500 WARC 1.0/1.1 网页归档解析与汇总 | `WarcRecordType`, `WarcField`, `WarcFieldIter`, `WarcRecord`, `WarcIter`, `WarcStats`, `isWarc`, `summarizeWarc` |
| `fits.cp` | NASA FITS 2880B HDU 与 80B 卡片头零分配解析 | `FitsCard`, `FitsHeader`, `FitsCardIter`, `isFits`, `parseFitsHeader`, `fitsBytesPerPixel`, `findFitsCard`, `parseFitsInt`, `parseFitsBool` |
| `pickle.cp` | Python Pickle Protocol 2-5 操作码遍历与静态安全审计 | `PickleOp`, `PickleAudit`, `detectPickleProto`, `isPickle`, `isUnsafePickleOp`, `PickleIter`, `auditPickle` |
| `thrift.cp` | Apache Thrift IDL 声明与字段词法扫描 | `ThriftDeclKind`, `ThriftDecl`, `ThriftField`, `ThriftDeclIter`, `ThriftFieldIter`, `validateThriftBraces` |
| `ion.cp` | Amazon Ion 1.0 二进制值与结构流解析 | `ION_IVM_1_0`, `IonType`, `IonValue`, `IonReader`, `IonListIter`, `IonStructIter`, `isIonBinary` |
| `smile.cp` | Jackson Smile 1.0 二进制 JSON 词法读取器 | `SMILE_HEADER_MAGIC`, `SmileHeader`, `SmileTokenKind`, `SmileToken`, `isSmile`, `parseSmileHeader`, `SmileReader` |
| `rlp.cp` | Ethereum RLP 规范确定性编解码器 | `RlpKind`, `RlpItem`, `RlpListIter`, `RlpWriter`, `decodeRlpItem`, `decodeRlpU64` |
| `ssz.cp` | Ethereum SSZ 确定性编解码与 Bitlist/Bitvector 校验 | `SszWriter`, `SszReader`, `sszBitlistLen`, `sszBitlistGet`, `sszBitvectorValid`, `SszOffsetListIter`, `sszChunkCount` |
| `scale.cp` | Parity SCALE 确定性编解码器与 Compact 整数编码 | `ScaleOptionBool`, `compactU64Len`, `ScaleWriter`, `ScaleReader` |
| `xdr.cp` | RFC 4506 XDR 外部数据表示编解码器 | `XDR_UNIT`, `xdrPadLen`, `xdrAlignedLen`, `XdrReader`, `XdrWriter` |
| `hdf5.cp` | HDF5 Superblock v0-v3 与 OHDR v2 解析 | `HDF5_SIGNATURE`, `Hdf5Superblock`, `Hdf5ObjHeaderV2`, `findHdf5Signature`, `isHdf5`, `parseHdf5Superblock`, `parseHdf5ObjHeaderV2` |
| `rdb.cp` | Redis RDB 快照流式事件解析 | `RDB_MAGIC`, `RdbLengthKind`, `RdbEventKind`, `RdbHeader`, `RdbEvent`, `isRdb`, `parseRdbHeader`, `RdbReader` |

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

---

## Batch 35 扩展：二进制对象、序列化器、科学数据容器与比特流（14 个子模块）

| 子模块文件 | 领域分类 | 核心导出说明 |
| :--- | :--- | :--- |
| `lib/binfmt/coff.cp` | COFF / PE Object File | `isCoff`, `parseCoffHeader`, `CoffSectionIter`, `CoffSymbolIter` |
| `lib/binfmt/bincode.cp` | Rust Bincode v1/v2 二进制编解码 | `BincodeReader`, `BincodeWriter`, `BincodeConfig` |
| `lib/binfmt/borsh.cp` | Borsh 确定性二进制编解码 | `BorshReader`, `BorshWriter` |
| `lib/binfmt/marshal.cp` | Ruby Marshal 4.8 二进制解析 | `isMarshal`, `parseMarshalHeader`, `MarshalReader` |
| `lib/binfmt/nbt.cp` | Named Binary Tag (Java BE & Bedrock LE) | `NbtTagType`, `NbtReader`, `parseNbtRoot` |
| `lib/binfmt/jks.cp` | Java KeyStore `.jks` v1/v2 二进制解析 | `isJks`, `parseJksHeader`, `JksEntryIter` |
| `lib/binfmt/tpm.cp` | TCG TPM 2.0 二进制头/Quote/Attestation | `parseTpmHeader`, `parseTpmAttest`, `parseTpmPublic` |
| `lib/binfmt/lzh.cp` | LZH / LHA 二进制归档与 CRC-16/ARC | `isLzh`, `LzhEntryIter`, `lzhCrc16` |
| `lib/binfmt/mseed.cp` | IRIS MiniSEED v2.4 地震学记录 | `isMiniSeed`, `parseMiniSeedHeader`, `MiniSeedIter` |
| `lib/binfmt/segy.cp` | SEG-Y Rev 1/2 地震勘探二进制格式 | `parseSegyBinaryHeader`, `parseSegyTraceHeader`, `SegyTraceIter` |
| `lib/binfmt/shp.cp` | ESRI Shapefile `.shp` 二进制矢量 | `isShapefile`, `parseShpHeader`, `ShpRecordIter` |
| `lib/binfmt/ts.cp` | ISO/IEC 13818-1 MPEG-TS 传输流 | `isMpegTs`, `MpegTsPacketIter`, `parseTsPacket` |
| `lib/binfmt/varintLeb128.cp` | LEB128 & ZigZag 变长整数编解码 | `encodeUleb128`, `decodeUleb128`, `encodeSleb128`, `decodeSleb128`, `zigzagEncode64`, `zigzagDecode64` |
| `lib/binfmt/bitstream.cp` | LSB/MSB 比特流读写器与 Leb128 | `BitReader`, `BitWriter`, `BitOrder`, `Leb128`, `BitstreamUleb128Result` |

---

## 收敛子模块深度文档

- [aof.md](binfmt/aof.md)
- [arrow.md](binfmt/arrow.md)
- [avro.md](binfmt/avro.md)
- [bincode.md](binfmt/bincode.md)
- [bitstream.md](binfmt/bitstream.md)
- [borsh.md](binfmt/borsh.md)
- [bson.md](binfmt/bson.md)
- [cbor_diag.md](binfmt/cbor_diag.md)
- [coff.md](binfmt/coff.md)
- [dex.md](binfmt/dex.md)
- [fits.md](binfmt/fits.md)
- [har.md](binfmt/har.md)
- [hdf5.md](binfmt/hdf5.md)
- [hprof.md](binfmt/hprof.md)
- [ion.md](binfmt/ion.md)
- [jfr.md](binfmt/jfr.md)
- [jks.md](binfmt/jks.md)
- [lcov.md](binfmt/lcov.md)
- [lzh.md](binfmt/lzh.md)
- [macho.md](binfmt/macho.md)
- [marshal.md](binfmt/marshal.md)
- [mseed.md](binfmt/mseed.md)
- [msgpack.md](binfmt/msgpack.md)
- [nbt.md](binfmt/nbt.md)
- [netcdf.md](binfmt/netcdf.md)
- [orc.md](binfmt/orc.md)
- [pcap.md](binfmt/pcap.md)
- [pcapng.md](binfmt/pcapng.md)
- [pickle.md](binfmt/pickle.md)
- [postcard.md](binfmt/postcard.md)
- [rdb.md](binfmt/rdb.md)
- [rlp.md](binfmt/rlp.md)
- [scale.md](binfmt/scale.md)
- [segy.md](binfmt/segy.md)
- [shp.md](binfmt/shp.md)
- [smile.md](binfmt/smile.md)
- [ssz.md](binfmt/ssz.md)
- [thrift.md](binfmt/thrift.md)
- [tpm.md](binfmt/tpm.md)
- [ts.md](binfmt/ts.md)
- [ubjson.md](binfmt/ubjson.md)
- [varint_leb128.md](binfmt/varint_leb128.md)
- [vmdk.md](binfmt/vmdk.md)
- [warc.md](binfmt/warc.md)
- [xdr.md](binfmt/xdr.md)

## 历史二进制格式域散装模块收敛（W29-Sync R7）

以下 5 个原独立顶层目录已统一收敛至 `cap:binfmt`（`lib/binfmt/`），可直接从 `"cap:binfmt"` 导入：

| 原顶层目录 | 收敛目标文件 | 主要导出符号 |
|---|---|---|
| `cap:arrow_ipc` | `lib/binfmt/arrow{Types,Flatbuf,Ipc}.cp` | `ArrowError`, `ArrowType`, `ArrowField`, `ArrowSchema`, `ArrowColumn`, `RecordBatch`, `ARROW_IPC_MAGIC`, `writeStream`, `readStream`, `writeFile`, `readFile` |
| `cap:binary_schema` | `lib/binfmt/binarySchema.cp` | `END_OF_MESSAGE`, `SchemaError`, `SchemaReader`, `SchemaWriter`, `FixedSchemaWriter`, `CompactUint64` |
| `cap:exe_format` | `lib/binfmt/exe{Types,Elf,Pe,Macho,Trailer}.cp` | `ExeKind`, `ExeArch`, `detectExeKind`, `parseElf64Header`, `parsePeHeader`, `parseMachO64Header`, `appendStandaloneTrailer`, `extractStandaloneTrailer` |
| `cap:pe_format` | `lib/binfmt/peFormat.cp` | `PeFormatError`, `PEFile`, `DOSHeader`, `PEHeader`, `OptionalHeader64`, `SectionHeader`, `StripMode`, `StripOpts`, `computePEChecksum`, `isPE` |
| `cap:wasm_fmt` | `lib/binfmt/wasm{Types,Leb128,Codec}.cp` | `WASM_MAGIC`, `WASM_VERSION`, `WasmError`, `WasmModule`, `wasmReadUleb128`, `writeUleb128`, `readSleb128`, `writeSleb128`, `parseWasm`, `encodeWasm`, `findExport`, `addCustomSection` |
