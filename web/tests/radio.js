// The radio the page's tests run against: the state recorded from it, sent
// over the live socket as the radio sends it, and the writes the page makes.
import fs from 'node:fs';

// The radio's state as it was recorded, mock/state.json.
export const recorded = JSON.parse(fs.readFileSync(new URL('../mock/state.json', import.meta.url)));

// The body of the next POST to a route.
export function nextPost(page, path) {
  return page.waitForRequest((r) => r.method() === 'POST' && new URL(r.url()).pathname === path).then((r) => r.postData());
}

// The radio on the live socket, /api/live, as it is while the page is open:
// each socket the page opens gets the recorded state, changed by `change`,
// at once, and so does a read of /api/state made before the socket opens.
// push(more) changes that state again and sends it, as the radio does when
// something on it changes. `sockets` holds each socket the page opened.
export async function liveRadio(page, change) {
  let now = structuredClone(recorded);
  change?.(now);
  const sockets = [];
  await page.route('**/api/state', (route) => route.fulfill({ json: now }));
  await page.routeWebSocket('**/api/live', (ws) => {
    sockets.push(ws);
    ws.send(JSON.stringify(now));
  });
  return {
    sockets,
    push(more) {
      now = structuredClone(now);
      more?.(now);
      for (const ws of sockets) ws.send(JSON.stringify(now));
    },
  };
}

// Holds every answer to a route until the test lets them go, so it sees what
// the page draws before the radio answers, however slow the machine is.
// Gives the function that lets them go. `reply` is the answer each gets.
export async function holdAnswers(page, url, reply = { body: 'ok' }) {
  let go;
  const gone = new Promise((done) => (go = done));
  await page.route(url, async (route) => {
    await gone;
    // The page may be gone by then, at the end of a test.
    await route.fulfill(reply).catch(() => {});
  });
  return go;
}

// Counts the page's reads of a route in the page itself, as it makes them,
// so a count taken straight after the page's clock moves is exact. Gives a
// function that reads the count.
export async function countReads(page, path = '/api/state') {
  await page.addInitScript((path) => {
    const counts = (window.reads ??= {});
    counts[path] = 0;
    const get = window.fetch;
    window.fetch = (url, ...rest) => {
      if (url === path) counts[path]++;
      return get(url, ...rest);
    };
  }, path);
  return () => page.evaluate((path) => window.reads[path], path);
}

// Stops the page's clock before the page opens: its timers, such as the 5 s
// poll and the socket's waits, then run only as the test moves the clock on
// with page.clock.runFor, by exactly that much. A screen that waits for a
// frame to be drawn waits too, so this suits the tests of the timers alone.
export async function stopClock(page) {
  await page.clock.install({ time: new Date(2026, 9, 11, 9, 0) });
  await page.clock.pauseAt(new Date(2026, 9, 11, 9, 0, 1));
}
