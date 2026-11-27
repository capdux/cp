// Migrated from TypeScript (electron/electron-webcontentsview-app.js) via cap from -ts

const { app, BaseWindow, WebContentsView } = require("electron");

app.on("window-all-closed", e => e.preventDefault());

app.whenReady().then(async () => {
  const win = BaseWindow.init({ width: 800, height: 600 });
  const viewHeight = 200;
  let i = 0;
  while (i < 3) { // [cap-migrate] for-step: i++
    const view = WebContentsView.init();
    win.contentView.addChildView(view);
    view.setBounds({ x: 0, y: i * viewHeight, width: 800, height: viewHeight });
    const colors = ["#e74c3c", "#2ecc71", "#3498db"];
    const html = `<title>WebContentsView${i + 1}</title><style>body{margin:0;background:${colors[i]};display:flex;align-items:center;justify-content:center;height:100vh;font:bold 32px sans-serif;color:#fff}</style><body>WebContentsView ${i + 1}</body>`;
    await view.webContents.loadURL("data:text/html," + encodeURIComponent(html));
  }
});
