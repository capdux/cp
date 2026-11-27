// Migrated from TypeScript (installation/fixture-scripts/electron-app.js) via cap from -ts

const { app } = require("electron");

app.on("window-all-closed", e => e.preventDefault());
