# @cap/harfbuzz

HarfBuzz (`15.2.0`) industrial-standard OpenType text shaping engine (Latin, CJK, Arabic, Indic, Emoji) for CP.

- **License**: MIT (Old MIT / HarfBuzz License)
- **Packaging**: Prebuilt stripped static archive (`libharfbuzz.a`, compiled from single amalgamation `harfbuzz.cc` with `-O3 -g0`) + thin C shim (`c/hb_ffi.c`) + RAII CP wrapper (`HbBuffer`).
