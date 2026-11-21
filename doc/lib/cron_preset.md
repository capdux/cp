# `cap:cron_preset` — Vixie Cron `@macro` Preset Expander

Zero-allocation Vixie cron `@macro` (`@yearly`/`@annually`/`@monthly`/`@weekly`/`@daily`/`@midnight`/`@hourly`/`@reboot`) parser and 5-field normalizer.

## Exports

- `CronPreset`, `parseCronPreset(raw): CronPreset?`, `presetToCron5(preset): ([]const u8)?`, `normalizeCron5(raw): ([]const u8)?`.
