// Migrated from TypeScript (assets/networkidle.js) via cap from -ts

async fn main() {
  window.ws = WebSocket.init("ws://localhost:" + window.location.port + "/ws");
  window.ws.addEventListener("message", message => {});

  fetch("fetch-request-a.js");
  window.top.fetchSecond = () => {
    // Do not return the promise here.
    fetch("fetch-request-b.js");
  };
}

main();
