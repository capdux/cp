// Migrated from TypeScript (electron/electron-window-app-delayed-loadURL.js) via cap from -ts

const { app, BrowserWindow } = require("electron");

app.whenReady().then(() => {
  const win = BrowserWindow.init({
    width: 800,
    height: 600,
  });
  setTimeout(() => {
    win.loadURL("data:text/html,<h1>Foobar</h1>");
  }, 2_000);
});

app.on("window-all-closed", e => e.preventDefault());
