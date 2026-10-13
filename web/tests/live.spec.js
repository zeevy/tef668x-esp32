// The live state: the radio sends its state over a WebSocket, and the page
// polls only while the socket is not open.
import { expect, test } from '@playwright/test';
import { countReads, holdAnswers, liveRadio, recorded, stopClock } from './radio.js';

const at9830 = (s) => Object.assign(s.tun, { f: '98.30', khz: 98300, rds: {} });

// Live sockets the test opens, closes and speaks on by hand, in place of the
// browser's own, so each step is exact: window.sockets holds each one the
// page made, and window.shut counts the times the page closed one. The dev
// server's own socket stays the browser's.
const handSockets = (page) =>
  page.addInitScript(() => {
    window.sockets = [];
    window.shut = 0;
    const Real = window.WebSocket;
    window.WebSocket = class {
      static OPEN = 1;
      readyState = 0;
      constructor(url, protocols) {
        if (!url.endsWith('/api/live')) return new Real(url, protocols);
        window.sockets.push(this);
      }
      close() {
        window.shut++;
      }
    };
  });
const made = (page) => page.evaluate(() => window.sockets.length);
// The radio turns the newest socket away: it closes, never having opened.
const turnAway = (page) => page.evaluate(() => window.sockets.at(-1).onclose());
// The tab goes out of view, or comes back.
const hide = (page, hidden) =>
  page.evaluate((hidden) => {
    Object.defineProperty(document, 'hidden', { configurable: true, get: () => hidden });
    document.dispatchEvent(new Event('visibilitychange'));
  }, hidden);
// Each request the page makes to a route, as the radio would see them.
const requests = (page, path) => {
  const seen = [];
  page.on('request', (r) => new URL(r.url()).pathname === path && seen.push(r));
  return seen;
};

test('while the radio sends the state, nothing is polled', async ({ page }) => {
  await page.clock.install();
  const reads = await countReads(page);
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  const before = await reads();
  // Past the 5 s poll.
  await page.clock.runFor(6000);
  expect(await reads()).toBe(before);
  await expect(page.getByTitle('Slow')).toHaveCount(0);
  await expect(page.getByTitle('No answer')).toHaveCount(0);
});

test('a state the radio sends shows at once', async ({ page }) => {
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  // Nothing is polled while the socket is open, so only the socket can bring it.
  radio.push(at9830);
  await expect(page.getByRole('link', { name: /98\.30/ })).toBeVisible();
});

test('the same state sent again changes nothing on the page', async ({ page }) => {
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  await expect(page.locator('main').getByRole('button', { name: 'Mute' })).toBeVisible();
  await page.evaluate(() => {
    window.changes = 0;
    new MutationObserver((m) => (window.changes += m.length)).observe(document.querySelector('main'), {
      subtree: true,
      childList: true,
      characterData: true,
      attributes: true,
    });
  });
  radio.push();
  // The page takes a state as it comes, so by the next call into it, it has
  // drawn whatever the state changed.
  await page.evaluate(() => {});
  expect(await page.evaluate(() => window.changes)).toBe(0);
});

test('when the socket closes, polling takes over and a new socket is tried 5 s on', async ({ page }) => {
  await page.clock.install();
  const reads = await countReads(page);
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  const before = await reads();
  await radio.sockets[0].close();
  await expect.poll(reads).toBe(before + 1);
  await page.clock.runFor(4500);
  expect(radio.sockets.length).toBe(1);
  await page.clock.runFor(500);
  await expect.poll(() => radio.sockets.length).toBe(2);
});

test('a hidden tab closes the socket and opens a new one when shown', async ({ page }) => {
  await page.clock.install();
  const reads = await countReads(page);
  const radio = await liveRadio(page);
  let closed = false;
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  radio.sockets[0].onClose(() => (closed = true));
  await hide(page, true);
  await expect.poll(() => closed).toBe(true);
  const before = await reads();
  // Past the 5 s poll and the 5 s wait for a new socket.
  await page.clock.runFor(6000);
  expect(await reads()).toBe(before);
  expect(radio.sockets.length).toBe(1);
  await hide(page, false);
  await expect.poll(() => radio.sockets.length).toBe(2);
});

test('a page that opens with the socket working reads no state of its own', async ({ page }) => {
  await stopClock(page);
  const states = requests(page, '/api/state');
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  await expect(page.getByRole('link', { name: /106\.40/ })).toBeVisible();
  // Past the 1 s wait for the socket and the 5 s poll.
  await page.clock.runFor(6000);
  expect(states).toHaveLength(0);
});

test('a socket that has not opened 1 s on leaves the page to read the state once', async ({ page }) => {
  await stopClock(page);
  await handSockets(page);
  const states = requests(page, '/api/state');
  await page.goto('/');
  await page.clock.runFor(999);
  expect(states).toHaveLength(0);
  await page.clock.runFor(1);
  await expect(page.getByRole('link', { name: /106\.40/ })).toBeVisible();
  expect(states).toHaveLength(1);
});

test('back in view, the page reads neither the state nor the settings, and still shows it is signed in', async ({ page }) => {
  await stopClock(page);
  const states = requests(page, '/api/state');
  const settings = requests(page, '/api/settings');
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  // The recorded settings answer, so the one check as the page opens says
  // this browser is signed in.
  await expect(page.getByRole('button', { name: 'Signed in' })).toBeVisible();
  await hide(page, true);
  await hide(page, false);
  await expect.poll(() => radio.sockets.length).toBe(2);
  await page.clock.runFor(6000);
  expect(states).toHaveLength(0);
  expect(settings).toHaveLength(1);
  await expect(page.getByRole('button', { name: 'Signed in' })).toBeVisible();
});

test('a change is drawn at once, and a state from before the radio took it cannot undo it', async ({ page }) => {
  // Stopped, so the page's 300 ms after the reply, while a state may still
  // be from before the change, is exact.
  await stopClock(page);
  await page.route('**/api/mute', (route) => route.fulfill({ body: 'muted' }));
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  const muted = page.getByText('Muted', { exact: true });
  const reply = page.waitForResponse('**/api/mute');
  await page.locator('main').getByRole('button', { name: 'Mute' }).click();
  await expect(muted).toBeVisible();
  await reply;
  // Made before the radio task took the mute, sent just after the reply.
  radio.push((s) => (s.tun.mut = false));
  await page.evaluate(() => {});
  await expect(muted).toBeVisible();
  // Later, the radio really muted: still shown, nothing moved.
  await page.clock.runFor(400);
  radio.push((s) => (s.tun.mut = true));
  await expect(muted).toBeVisible();
  // And what the radio says after that wins: someone unmuted on the radio.
  await page.clock.runFor(400);
  radio.push((s) => (s.tun.mut = false));
  await expect(muted).toHaveCount(0);
});

test('a seek needs no read of its own while the socket is open', async ({ page }) => {
  const reads = await countReads(page);
  await page.route('**/api/seek', (route) => route.fulfill({ body: 'seeking up' }));
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  const before = await reads();
  await page.getByRole('button', { name: 'Seek up' }).click();
  // Changes go to the radio one after another, so the second seek is sent
  // only after the page has done all it does with the answer to the first.
  const second = page.waitForRequest((r) => r.method() === 'POST' && r.url().endsWith('/api/seek'));
  await page.getByRole('button', { name: 'Seek up' }).click();
  await second;
  expect(await reads()).toBe(before);
});

test('a read that started before a state the socket brought cannot put the older one back', async ({ page }) => {
  // The socket is slow to open, so the page reads the state. The socket then
  // brings 98.30 while that read is still on its way with the recorded 106.40.
  await stopClock(page);
  await handSockets(page);
  let answer;
  await page.route('**/api/state', (route) => (answer = () => route.fulfill({ json: recorded })));
  await page.goto('/');
  await page.clock.runFor(1000);
  await expect.poll(() => answer).toBeTruthy();
  await page.clock.runFor(100);
  const later = structuredClone(recorded);
  at9830(later);
  await page.evaluate((s) => {
    const ws = window.sockets.at(-1);
    ws.readyState = 1;
    ws.onopen();
    ws.onmessage({ data: JSON.stringify(s) });
  }, later);
  await expect(page.getByRole('link', { name: /98\.30/ })).toBeVisible();
  const landed = page.waitForEvent('requestfinished', (r) => r.url().endsWith('/api/state'));
  await answer();
  await landed;
  await page.evaluate(() => {});
  await expect(page.getByRole('link', { name: /98\.30/ })).toBeVisible();
  await expect(page.getByRole('link', { name: /106\.40/ })).toHaveCount(0);
});

test('a socket quiet for 10 s has lost the radio: polling takes over', async ({ page }) => {
  await stopClock(page);
  const reads = await countReads(page);
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  await expect(page.getByRole('link', { name: /MHz|kHz/ })).toBeVisible();
  const before = await reads();
  // The radio restarts: no close, no more states.
  await page.clock.runFor(9999);
  expect(await reads()).toBe(before);
  await page.clock.runFor(1);
  expect(await reads()).toBe(before + 1);
});

test('a refused change is taken back off at once while the socket is open', async ({ page }) => {
  const answer = await holdAnswers(page, '**/api/mute', { status: 409, body: 'Not now.' });
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  await page.locator('main').getByRole('button', { name: 'Mute' }).click();
  // Drawn before the radio answers.
  await expect(page.getByText('Muted', { exact: true })).toBeVisible();
  answer();
  // No state comes after the refusal, so only the refusal can take it off.
  await expect(page.getByText('Muted', { exact: true })).toHaveCount(0);
  await expect(page.locator('main').getByRole('button', { name: 'Mute' })).toBeVisible();
});

test('a socket the radio turns away is tried again 10, 20, 40, 60 and 60 s on', async ({ page }) => {
  await stopClock(page);
  await handSockets(page);
  await page.goto('/');
  await expect.poll(() => made(page)).toBe(1);
  for (const [i, s] of [10, 20, 40, 60, 60].entries()) {
    await turnAway(page);
    await page.clock.runFor(s * 1000 - 1);
    expect(await made(page), `before ${s} s`).toBe(i + 1);
    await page.clock.runFor(1);
    expect(await made(page), `at ${s} s`).toBe(i + 2);
  }
});

test('a socket that opened starts the waits again: 5 s after it closes, then 10 s', async ({ page }) => {
  await stopClock(page);
  await handSockets(page);
  await page.goto('/');
  await expect.poll(() => made(page)).toBe(1);
  // Two turned away: the next wait would be 40 s.
  await turnAway(page);
  await page.clock.runFor(10000);
  await turnAway(page);
  await page.clock.runFor(20000);
  expect(await made(page)).toBe(3);
  // The third opens, then the radio closes it.
  await page.evaluate(() => {
    const ws = window.sockets.at(-1);
    ws.readyState = 1;
    ws.onopen();
    ws.readyState = 3;
    ws.onclose();
  });
  await page.clock.runFor(4999);
  expect(await made(page)).toBe(3);
  await page.clock.runFor(1);
  expect(await made(page)).toBe(4);
  // Turned away again: 10 s, not 40.
  await turnAway(page);
  await page.clock.runFor(9999);
  expect(await made(page)).toBe(4);
  await page.clock.runFor(1);
  expect(await made(page)).toBe(5);
});

test('a frame that is not a state is let go, and the socket stays open', async ({ page }) => {
  const errors = [];
  page.on('pageerror', (e) => errors.push(e.message));
  const reads = await countReads(page);
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  const before = await reads();
  radio.sockets[0].send('not a state');
  radio.push(at9830);
  await expect(page.getByRole('link', { name: /98\.30/ })).toBeVisible();
  expect(errors).toEqual([]);
  expect(await reads()).toBe(before);
  expect(radio.sockets.length).toBe(1);
});

test('a change the radio does not answer, or refuses with no words, is said in plain words', async ({ page }) => {
  await page.route('**/api/mute', (route) => route.abort());
  await page.goto('/');
  await page.locator('main').getByRole('button', { name: 'Mute' }).click();
  await expect(page.getByRole('status').filter({ hasText: 'The radio did not answer.' })).toBeVisible();
  await page.unroute('**/api/mute');
  await page.route('**/api/mute', (route) => route.fulfill({ status: 500, body: '' }));
  await page.locator('main').getByRole('button', { name: 'Mute' }).click();
  await expect(page.getByRole('status').filter({ hasText: 'The radio refused it (500).' })).toBeVisible();
});

test('a state with no tuner part is dropped, so the page waits for one it can draw', async ({ page }) => {
  const radio = await liveRadio(page, (s) => delete s.tun);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  radio.push();
  await page.evaluate(() => {});
  await expect(page.getByText('Waiting for the radio')).toBeVisible();
  await expect(page.getByText('This page could not draw what the radio sent.')).toHaveCount(0);
});

test('a fault while drawing shows a line and Reload, not a blank page', async ({ page }) => {
  await page.route('**/api/state', (route) => route.fulfill({ json: { ...recorded, clk: undefined } }));
  await page.goto('/');
  await expect(page.getByText('This page could not draw what the radio sent.')).toBeVisible();
  await page.unrouteAll();
  await page.getByRole('button', { name: 'Reload' }).click();
  await expect(page.locator('main').getByRole('button', { name: 'Mute' })).toBeVisible();
});

test('a socket that hangs while it connects is closed after 10 s, and polling goes on', async ({ page }) => {
  await stopClock(page);
  await handSockets(page);
  const reads = await countReads(page);
  await page.goto('/');
  // The socket has not opened 1 s on, so the page reads the state, and the
  // next poll is set for 5 s on.
  await page.clock.runFor(1000);
  await expect(page.getByRole('link', { name: /MHz|kHz/ })).toBeVisible();
  await page.clock.runFor(8999);
  expect(await page.evaluate(() => window.shut)).toBe(0);
  await page.clock.runFor(1);
  expect(await page.evaluate(() => window.shut)).toBe(1);
  expect(await reads()).toBeGreaterThan(1);
  // A socket that never opened: the next is tried 10 s on.
  await page.clock.runFor(9999);
  expect(await made(page)).toBe(1);
  await page.clock.runFor(1);
  expect(await made(page)).toBe(2);
});

test('a band plan that could not be read is asked again 5 s on, not with every state', async ({ page }) => {
  await page.clock.install();
  const reads = await countReads(page, '/api/bands');
  await page.route('**/api/bands', (route) => route.fulfill({ status: 503, body: '' }));
  const radio = await liveRadio(page);
  await page.goto('/');
  await expect.poll(() => radio.sockets.length).toBe(1);
  await expect.poll(reads).toBe(1);
  // Ten states in 2 s, as the radio sends them.
  for (let i = 0; i < 10; i++) {
    radio.push();
    await page.clock.runFor(200);
  }
  expect(await reads()).toBe(1);
  await page.clock.runFor(3500);
  radio.push();
  await expect.poll(reads).toBe(2);
});
