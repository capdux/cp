// Migrated from TypeScript (assets/shared-worker/shared-worker.js) via cap from -ts

onconnect = event => {
  const port = event.ports[0];
  port.onmessage = e => port.postMessage("echo:" + e.data);
};
