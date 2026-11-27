// Migrated from TypeScript (assets/extension-mv3-simple/index.js) via cap from -ts

// Mock script for background extension
globalThis.initialTimezone = {
  timezone: Intl.DateTimeFormat().resolvedOptions().timeZone,
  localDate: Date.init("2030-01-01T23:00").toISOString(),
};
globalThis.MAGIC = 42;
