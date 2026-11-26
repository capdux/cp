# `@cap/faker` — Zero-Allocation Deterministic Fake Data Generator

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@faker-js/faker` ([https://github.com/faker-js/faker](https://github.com/faker-js/faker))（原库名 `@faker-js/faker` → CP 包名 `@cap/faker`）

> Clean-room pure-CP implementation modeled after npm `@faker-js/faker` & Rust `fake` | License: MIT

## Overview / 概述

`@cap/faker` is a zero-dependency, zero-heap-allocation deterministic fake data generator for CP, powered by **xoshiro256\*\*** seeded with **SplitMix64**. It produces reproducible test fixtures across person/identity, networking, RFC 4122 UUID v4, text/commerce, Luhn-valid test credit card numbers, and unbiased collection sampling.

`@cap/faker` 是纯 CP 零依赖、零堆分配的确定性仿真测试数据生成库，底层采用 **SplitMix64 + xoshiro256\*\*** 伪随机数引擎，支持同种子 100% 可复现的人物、网络、UUID v4、文本商业、Luhn 校验测试卡号及集合洗牌/无放回抽样。

## Installation / 安装

```bash
cap add @cap/faker
```

## API Reference / 导出 API

| Category | Signature | Description / 说明 |
| :--- | :--- | :--- |
| Lifecycle | `Faker.init(seed: u64): Faker` | Initialize deterministic PRNG / 按种子初始化 |
| Lifecycle | `f.reseed(seed: u64)` | Reset internal state to `seed` / 重置种子 |
| Primitives | `f.nextU64(): u64` | Next 64-bit unsigned random integer / 下一个 64 位无符号随机数 |
| Primitives | `f.nextInt(min: i64, max: i64): i64` | Inclusive signed range `[min, max]` / 闭区间有符号随机整数 |
| Primitives | `f.nextBool(prob_pct: u8): bool` | Weighted boolean (`0..100`%) / 按百分比概率返回布尔值 |
| Primitives | `f.pickStr(items: []const []const u8): []const u8` | Pick a random slice element / 随机选取字符串切片元素 |
| Person | `f.firstName()` / `f.lastName()` | Static first/last name / 随机名、姓 |
| Person | `f.fullName(out)` / `f.username(out)` / `f.email(out)` / `f.phone(out)` | Composed identity & E.164 phone / 姓名、用户名、邮箱、E.164 电话 |
| Network | `f.ipv4(out)` / `f.ipv6(out)` / `f.domain(out)` / `f.url(out)` | Network addresses & URLs / IPv4、IPv6、域名、HTTPS URL |
| Network | `f.uuidV4(out)` / `f.hexColor(out)` | RFC 4122 UUID v4 & `#rrggbb` / 合规 UUID v4 与十六进制色值 |
| Text & Biz | `f.word()` / `f.sentence(n, out)` / `f.paragraph(n, out)` | Words, sentences & paragraphs / 单词、句子与段落 |
| Text & Biz | `f.company(out)` / `f.city()` / `f.country()` / `f.isoDate(out)` | Company, geo & `YYYY-MM-DD` / 公司、城市、国家与 ISO 日期 |
| Finance | `f.creditCardNumber(out)` / `isValidLuhn(digits)` | 16-digit Luhn-valid test card & verifier / 通过 Luhn 校验的 16 位测试卡号 |
| Collections | `f.shuffleU64(items)` / `f.sampleIndices(pop_len, out)` | Fisher-Yates shuffle & Floyd's unique sampling / 原地洗牌与无放回抽样 |

## Quick Start / 使用示例

```cp
import { Faker, isValidLuhn } from "@cap/faker";

fn generateFixture() {
    let faker = Faker.init(2026);

    let name_buf: [64]u8 = undefined;
    let email_buf: [96]u8 = undefined;
    let uuid_buf: [40]u8 = undefined;
    let card_buf: [20]u8 = undefined;

    const full_name = faker.fullName(&name_buf);
    const email = faker.email(&email_buf);
    const id = faker.uuidV4(&uuid_buf);
    const card = faker.creditCardNumber(&card_buf);
    const ok = isValidLuhn(card); // always true

    // In-place Fisher-Yates shuffle
    let ids: [5]u64 = [10, 20, 30, 40, 50];
    faker.shuffleU64(&ids);

    // Unique index sampling without replacement
    let sampled: [3]usize = undefined;
    const count = faker.sampleIndices(100, &sampled);

    * = full_name;
    * = email;
    * = id;
    * = ok;
    * = count;
}
```
