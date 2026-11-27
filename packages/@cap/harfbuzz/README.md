[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`harfbuzz`](https://github.com/harfbuzz/harfbuzz)（`MIT`）（包名 `@cap/harfbuzz`）。

# @cap/harfbuzz

HarfBuzz (`15.2.0`) industrial-standard OpenType text shaping engine (Latin, CJK, Arabic, Indic, Emoji) for CP.

- **License**: MIT (Old MIT / HarfBuzz License)
- **Packaging**: Prebuilt stripped static archive (`libharfbuzz.a`, compiled from single amalgamation `harfbuzz.cc` with `-O3 -g0`) + thin C shim (`c/hb_ffi.c`) + RAII CP wrapper (`HbBuffer`).
