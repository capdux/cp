# `cap:iban` — ISO 13616 IBAN 与 ISO 7064 Mod-97-10 校验及格式化库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

纯 CP、零堆分配的国际银行账号（IBAN）金融标准库，内置 52 国 ISO 13616 长度注册表、流式 ISO 7064 Mod-97-10 校验（无需大整数）、校验位反算以及电子/打印双向格式化。

## 导入

```cp
import {
    IBAN_MIN_LEN,
    IBAN_MAX_LEN,
    IbanCheckDigits,
    IbanInfo,
    ibanCountryLength,
    ibanMod97,
    isValidIban,
    parseIban,
    computeIbanCheckDigits,
    formatIbanElectronic,
    formatIbanPrint,
} from "cap:codec";
```

## 核心 API

- **协议常量与国家表**：
  - `IBAN_MIN_LEN = 15`、`IBAN_MAX_LEN = 34`
  - `ibanCountryLength(cc0: u8, cc1: u8): u8?` — 大小写不敏感查询 52 国 IBAN 长度（`DE=22, GB=22, FR=27, NL=18, CH=21, ...`）。
- **数据模型**：
  - `IbanInfo` — `{ country: [2]u8, check_digits: [2]u8, bban: [30]u8, bban_len: usize }`，提供 `bbanSlice(): []const u8` 与 `totalLen(): usize`。
- **校验、解析与反算**：
  - `ibanMod97(iban: []const u8): u8?` — 忽略空格/连字符，移位并逐位取模 97（合法 IBAN 恒返回 `1`）。
  - `isValidIban(iban: []const u8): bool` — 全套国家表长度、格式与 Mod-97 校验。
  - `parseIban(iban: []const u8): IbanInfo?` — 严格校验并拆解国家码、校验位与 BBAN。
  - `computeIbanCheckDigits(country: [2]u8, bban: []const u8): IbanCheckDigits?` — 按 `98 - mod97(bban + country + "00")` 生成两位零填充校验位（`IbanCheckDigits = [2]u8`）。
- **电子/打印格式化**：
  - `formatIbanElectronic(iban: []const u8, out: []u8): usize?` — 输出大写无空格电子格式。
  - `formatIbanPrint(iban: []const u8, out: []u8): usize?` — 按每 4 字符空格分组输出打印格式。

## 示例

```cp
import std from "std";
import { isValidIban, formatIbanPrint } from "cap:codec";

fn normalizeForReceipt(raw: []const u8, out: []u8): usize? {
    if (!isValidIban(raw)) {
        return null;
    }
    return formatIbanPrint(raw, out);
}
```
