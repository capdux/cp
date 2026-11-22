# CP 标准库：DevOps、软件包、磁盘镜像、文件系统与发布运维套件手册 (`cap:devops`)

> 来源：Pure-CP 零 C 依赖 DevOps、系统包格式、虚拟化磁盘镜像、只读/闪存文件系统、引导头与发布运维标准库收敛（20 个子模块统一聚合） | 创建日期：2026-11-22

## 一、 架构总览

`cap:devops` 是 CP 语言官方内置的纯原生、零外部 C 依赖基础设施与 DevOps 标准库，统一覆盖 **系统软件包（DEB/RPM/APK）、光盘与虚拟化镜像（DMG/ISO 9660/UDF/VDI/VHD）、磁盘加密与 Linux/嵌入式文件系统（LUKS/EROFS/F2FS/CramFS/RomFS/MINIX/JFFS2/UBIFS）、Multiboot 引导镜像、CI 测试报告（TRX）、灰度开关（Feature Flags）以及确定性数据库 Schema 迁移**。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. 软件包与系统分发格式（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/devops/deb.cp` | Debian `.deb` & RFC 822 Control | `isDeb`, `parseDebPackage`, `parseDebControl`, `detectDebCompression`, `DebControlIter`, `DebPackage` |
| `lib/devops/rpm.cp` | Linux RPM v3/v4 包头与标签 | `isRpm`, `parseRpmLead`, `parseRpmHeaderSection`, `parseRpmPackage`, `RpmTagIter`, `RpmPackageInfo` |
| `lib/devops/apk.cp` | Android APK ZIP EOCD & Signing Block v2/v3 | `findZipEocd`, `findApkSigningBlock`, `ApkSigPairIter`, `summarizeApkSigning`, `ApkSigSummary` |

### 2. 光盘、磁盘镜像与虚拟化磁盘（5 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/devops/dmg.cp` | Apple UDIF `.dmg` koly & BLKX mish | `isDmg`, `parseDmgKoly`, `parseDmgMish`, `DmgStripeIter`, `DmgKolyHeader`, `DmgMishHeader` |
| `lib/devops/iso.cp` | ISO 9660 卷描述符与目录解析 | `isIso9660`, `parseIsoPvd`, `findIsoEntry`, `readIsoFileData`, `IsoDirIter`, `IsoPrimaryVolume` |
| `lib/devops/udf.cp` | ECMA-167 / ISO 13346 UDF & OSTA CS0 | `parseUdfTag`, `parseUdfVrsEntry`, `parseUdfAvdp`, `parseUdfPvd`, `decodeOstaCs0Ascii`, `udfCrc16` |
| `lib/devops/vdi.cp` | VirtualBox VDI 虚拟磁盘解析 | `isVdi`, `parseVdiHeader`, `decodeVdiBlockEntry`, `lookupVdiBlock`, `VdiHeader`, `VdiBlockMapping` |
| `lib/devops/vhd.cp` | Microsoft VHD 固定/动态/差分磁盘 | `isVhdFooter`, `parseVhdFooter`, `parseVhdDynamicHeader`, `lookupVhdBatEntry`, `vhdChecksum`, `VhdFooter` |

### 3. 磁盘加密、引导与文件系统解析（9 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/devops/luks.cp` | Linux LUKS1 & LUKS2 加密磁盘头 | `isLuks`, `parseLuks1Header`, `parseLuks2Header`, `Luks1Header`, `Luks2Header` |
| `lib/devops/erofs.cp` | Linux EROFS 只读文件系统 | `isErofs`, `parseErofsSuperblock`, `ErofsSuperblock` |
| `lib/devops/f2fs.cp` | Linux F2FS 闪存文件系统 | `isF2fs`, `parseF2fsSuperblock`, `F2fsSuperblock` |
| `lib/devops/cramfs.cp` | Linux CramFS 压缩只读文件系统 | `isCramfs`, `parseCramfsSuperblock`, `CramfsSuperblock` |
| `lib/devops/romfs.cp` | Linux Kernel romfs 只读文件系统 | `isRomfs`, `parseRomfs`, `romfsChecksum`, `RomfsDirIter`, `RomfsImage` |
| `lib/devops/minix.cp` | MINIX v1/v2/v3 文件系统 | `isMinixSuperblock`, `parseMinixSuperblock`, `parseMinixV1Inode`, `parseMinixV2Inode`, `iterMinixDir`, `S_IFMT`, `S_IFDIR`, `S_IFREG`, `S_IFLNK` |
| `lib/devops/jffs2.cp` | Linux JFFS2 闪存文件系统 | `isJffs2`, `parseJffs2NodeHeader`, `parseJffs2Inode`, `parseJffs2Dirent`, `iterJffs2Nodes`, `jffs2Crc32` |
| `lib/devops/ubifs.cp` | Linux UBIFS 闪存文件系统 | `parseUbiEcHeader`, `parseUbiVidHeader`, `isUbifsNode`, `parseUbifsSuperblock`, `parseUbifsMaster`, `UBIFS_COMPR_NONE`, `UBIFS_COMPR_LZO`, `UBIFS_COMPR_ZLIB`, `COMPR_ZSTD` |
| `lib/devops/multiboot.cp` | Multiboot v1/v2 引导镜像与内存映射 | `parseMb1Header`, `parseMb2Header`, `parseMb2BootInfo`, `iterMb2Mmap`, `parseMb2Framebuffer`, `parseMb2Module` |

### 4. CI 测试报告、灰度发布与 Schema 迁移（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/devops/trx.cp` | Visual Studio / .NET TRX CI 测试报告 | `isTrx`, `parseTrxHeader`, `parseTrxSummary`, `TrxResultIter`, `TrxOutcome`, `TrxSummary` |
| `lib/devops/feature.cp` | Feature Flags 灰度分桶与实验引擎 | `FeatureEngine`, `EvalContext`, `TargetingRule`, `VariantWeight`, `FeatureDecision`, `consistentBucket` |
| `lib/devops/migrate.cp` | 确定性数据库 Schema 迁移与防篡改校验 | `MigrationRegistry`, `SqliteMigrator`, `MigrationSpec`, `AppliedMigration`, `MigrationDirection`, `MigrateError` |

---

## 三、 使用示例

```typescript
import {
    isDeb,
    isRpm,
    isVhdFooter,
    isTrx,
    consistentBucket,
} from "cap:devops";

pub fn demoDevops(buf: []const u8) {
    * = isDeb(buf);
    * = isRpm(buf);
    * = isVhdFooter(buf);
    * = isTrx(buf);
    * = consistentBucket("new_checkout", "user_42");
}
```

---

## 收敛子模块深度文档

- [apk.md](devops/apk.md)
- [cramfs.md](devops/cramfs.md)
- [deb.md](devops/deb.md)
- [dmg.md](devops/dmg.md)
- [erofs.md](devops/erofs.md)
- [f2fs.md](devops/f2fs.md)
- [feature.md](devops/feature.md)
- [iso.md](devops/iso.md)
- [jffs2.md](devops/jffs2.md)
- [luks.md](devops/luks.md)
- [migrate.md](devops/migrate.md)
- [minix.md](devops/minix.md)
- [multiboot.md](devops/multiboot.md)
- [romfs.md](devops/romfs.md)
- [rpm.md](devops/rpm.md)
- [trx.md](devops/trx.md)
- [ubifs.md](devops/ubifs.md)
- [udf.md](devops/udf.md)
- [vdi.md](devops/vdi.md)
- [vhd.md](devops/vhd.md)
