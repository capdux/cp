# `cap:bin` — 二进制切片扫描与字节序读写基座

内置 L0 标准库通道（`lib/bin/index.cp`），为 CP 生态包提供零堆、展开加速的字节切片比较/查找、常量时间比较与 Big/Little-Endian 整数编解码。

## 核心导出

- 比较与查找：`bytesEq`、`ctEqual`、`startsWithBytes`、`endsWithBytes`、`findByteInBytes`、`countByte`、`indexOfBytes`
- 字节序读写：`readU16BE/LE`、`readU32BE/LE`、`readU64BE/LE`、`writeU16BE/LE`、`writeU32BE/LE`、`writeU64BE/LE`
