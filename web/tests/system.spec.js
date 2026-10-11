// System: the firmware and the update check, installing, restarting and
// sleep each asked first, a file sent with its progress, the radio's reason
// read out of an HTML answer, and the health figures.
import { expect, test } from '@playwright/test';
import { liveRadio } from './radio.js';

// The radio on the live socket, up 4000 s. Gives the socket's end, to push a
// changed state.
function radio(page, change) {
  return liveRadio(page, (s) => {
    Object.assign(s, { ver: '0.2.0', bid: 'abc1234', brd: 'ats125', slt: 'app1', cnf: true, upd: { st: 'none', ver: null, sz: null }, up: 4000 });
    change?.(s);
  });
}

// The next POST to a route, as the request.
function nextPost(page, path) {
  return page.waitForRequest((q) => q.method() === 'POST' && new URL(q.url()).pathname === path);
}
const card = (page, title) => page.locator('main section').filter({ has: page.locator('h2', { hasText: title }) });

test('the firmware: version, build, up to date, and a chip only while on trial', async ({ page }) => {
  await radio(page);
  await page.goto('/#system');
  const fw = card(page, 'Firmware');
  await expect(fw).toContainText('Version 0.2.0');
  await expect(fw).toContainText('Build abc1234 · ATS125 · slot app1');
  await expect(fw).toContainText('Up to date');
  await expect(fw).not.toContainText('On trial');
  await page.unrouteAll();
  await radio(page, (s) => Object.assign(s, { cnf: false }));
  await page.reload();
  await expect(card(page, 'Firmware')).toContainText('On trial until it joins the Wi-Fi');
});

test('a newer version found: install it, asked first', async ({ page }) => {
  await radio(page, (s) => (s.upd = { st: 'found', ver: '0.2.1', sz: 1468006 }));
  await page.goto('/#system');
  await expect(card(page, 'Firmware')).toContainText('0.2.1 is out');
  await expect(page.getByText('Version 0.2.1 is out')).toBeVisible();
  await expect(page.getByText('1.4 MB, from GitHub')).toBeVisible();
  await page.getByRole('button', { name: 'Install 0.2.1' }).click();
  const sent = nextPost(page, '/update/install');
  await page.getByRole('dialog').getByRole('button', { name: 'Install' }).click();
  await sent;
  await expect(card(page, 'Firmware')).toContainText('Downloading and installing.');
});

test('the update check sits in the Firmware card, is the stored setting, and turns off and on', async ({ page }) => {
  const live = await radio(page);
  await page.route('**/api/settings', (route) => (route.request().method() === 'GET' ? route.fulfill({ json: { upc: 1 } }) : route.fulfill({ body: 'Saved and in use now.' })));
  await page.goto('/#system');
  const check = card(page, 'Firmware').getByRole('switch', { name: 'Check for updates' });
  await expect(check).toHaveAttribute('aria-checked', 'true');
  let sent = nextPost(page, '/api/settings');
  await check.click();
  expect((await sent).postData()).toBe('upc=0');
  // The check's own state still says up to date; the switch stays off. The
  // page takes a state as it comes, so by the next call into it, it is in.
  live.push();
  await page.evaluate(() => {});
  await expect(check).toHaveAttribute('aria-checked', 'false');
  sent = nextPost(page, '/api/settings');
  await check.click();
  expect((await sent).postData()).toBe('upc=1');
  await expect(check).toHaveAttribute('aria-checked', 'true');
});

test('restart asks first, then waits for the radio to come back', async ({ page }) => {
  const live = await radio(page);
  await page.goto('/#system');
  await page.getByRole('button', { name: 'Restart' }).click();
  const sent = nextPost(page, '/reboot');
  await page.getByRole('dialog').getByRole('button', { name: 'Restart' }).click();
  await sent;
  await expect(card(page, 'Firmware')).toContainText('Restarting. This page waits for it.');
  // Back, and up for 12 s.
  live.push((s) => (s.up = 12));
  await expect(page.getByText('The radio is back, version 0.2.0.')).toBeVisible();
});

test('a refusal in an HTML page shows the radio\'s own words', async ({ page }) => {
  await radio(page);
  await page.route('**/reboot', (route) => route.fulfill({ status: 409, contentType: 'text/html', body: '<!doctype html><html><head><style>p{}</style></head><body><h3>Not now</h3><p>A new firmware is on trial.</p></body></html>' }));
  await page.goto('/#system');
  await page.getByRole('button', { name: 'Restart' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Restart' }).click();
  await expect(page.getByText('Not now A new firmware is on trial.')).toBeVisible();
});

test('sleep asks first', async ({ page }) => {
  await radio(page);
  await page.goto('/#system');
  await page.getByRole('button', { name: 'Sleep' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Not now' }).click();
  await expect(page.getByRole('dialog')).toHaveCount(0);
  await page.getByRole('button', { name: 'Sleep' }).click();
  const sent = nextPost(page, '/api/sleep');
  await page.getByRole('dialog').getByRole('button', { name: 'Sleep' }).click();
  await sent;
});

test('a firmware file is sent as the form field firmware', async ({ page }) => {
  await radio(page);
  await page.route('**/update', (route) => route.fulfill({ contentType: 'text/html', body: '<p>Written. Restarting.</p>' }));
  await page.goto('/#system');
  await page.getByLabel('Firmware file').setInputFiles({ name: 'firmware.bin', mimeType: 'application/octet-stream', buffer: Buffer.alloc(4096, 7) });
  await expect(page.getByText('firmware.bin')).toBeVisible();
  const sent = nextPost(page, '/update');
  await page.getByRole('button', { name: 'Install this file' }).click();
  const req = await sent;
  expect(req.headers()['content-type']).toContain('multipart/form-data');
  expect(req.postDataBuffer().toString('latin1')).toContain('name="firmware"; filename="firmware.bin"');
  await expect(card(page, 'Firmware')).toContainText('Written. The radio restarts into it.');
});

const firmware = { name: 'firmware.bin', mimeType: 'application/octet-stream', buffer: Buffer.alloc(4096, 7) };
async function install(page) {
  await page.getByLabel('Firmware file').setInputFiles(firmware);
  await page.getByRole('button', { name: 'Install this file' }).click();
}

test('a firmware file refused for the PIN opens the sign-in box with its PIN field', async ({ page }) => {
  await radio(page);
  await page.route('**/update', (route) => route.fulfill({ status: 403, body: 'Sign in first' }));
  await page.goto('/#system');
  // Signed in when the page opened; the session has ended since.
  await expect(page.getByRole('button', { name: 'Signed in' })).toBeVisible();
  await install(page);
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByRole('heading', { name: 'Sign in' })).toBeVisible();
  await expect(dialog.getByLabel('PIN')).toBeVisible();
  await expect(page.getByText('Sign in first')).toHaveCount(0);
});

test('a firmware file the radio refuses shows its words, and stays chosen', async ({ page }) => {
  await radio(page);
  await page.route('**/update', (route) => route.fulfill({ status: 400, contentType: 'text/html', body: '<!doctype html><html><body><h3>Not installed</h3><p>This file is not for this radio.</p></body></html>' }));
  await page.goto('/#system');
  await install(page);
  await expect(page.getByText('Not installed This file is not for this radio.')).toBeVisible();
  await expect(page.getByText('firmware.bin')).toBeVisible();
  await expect(page.getByRole('dialog')).toHaveCount(0);
});

test('a firmware file that does not reach the radio says so', async ({ page }) => {
  await radio(page);
  await page.route('**/update', (route) => route.abort());
  await page.goto('/#system');
  await install(page);
  await expect(page.getByText('The file did not reach the radio.')).toBeVisible();
  await expect(page.getByRole('button', { name: 'Install this file' })).toBeVisible();
});

test('the bar shows how much of the file has gone, 0 to 100 %', async ({ page }) => {
  await radio(page);
  await page.goto('/#system');
  // Every value the bar takes. The file goes to the dev server itself, as
  // the browser shows no progress for an answer made up in the test.
  await page.evaluate(() => {
    window.seen = [];
    new MutationObserver(() => {
      const v = document.querySelector('[role=progressbar]')?.getAttribute('aria-valuenow');
      if (v != null && window.seen.at(-1) !== +v) window.seen.push(+v);
    }).observe(document.body, { subtree: true, childList: true, attributes: true, attributeFilter: ['aria-valuenow'] });
  });
  await install(page);
  await expect(card(page, 'Firmware')).toContainText('Written. The radio restarts into it.');
  await expect(page.getByRole('progressbar')).toHaveCount(0);
  const seen = await page.evaluate(() => window.seen);
  expect(seen[0]).toBe(0);
  expect(seen.at(-1)).toBe(100);
});

test('the health figures', async ({ page }) => {
  await radio(page, (s) => Object.assign(s, { hep: 122880, hmn: 69632, log: { fit: true, n: 78 }, pst: { n: 16, bad: false, lost: 0 } }));
  await page.goto('/#system');
  // Two cards of eight: how it is running, then its stores and links.
  const running = card(page, 'Running');
  const stores = card(page, 'Stores and links');
  for (const text of ['120 KB', '68 KB']) await expect(running).toContainText(text);
  for (const text of ['78 of 250', '16 of 99', 'TEF6686']) await expect(stores).toContainText(text);
  await expect(running.locator('dt')).toHaveCount(8);
  await expect(stores.locator('dt')).toHaveCount(8);
});

test('Restart and Sleep sit in the firmware card, the updates come last; on a laptop the two health cards side by side, the same height', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#system');
  const fw = card(page, 'Firmware');
  for (const name of ['Restart', 'Sleep']) await expect(fw.getByRole('button', { name })).toBeVisible();
  // The updates come last, under the health cards.
  const [health, updates] = [await card(page, 'Stores and links').boundingBox(), await card(page, 'Updates').boundingBox()];
  expect(updates.y).toBeGreaterThan(health.y + health.height);
  if (testInfo.project.name !== 'laptop') return;
  const [a, b] = [await card(page, 'Running').boundingBox(), await card(page, 'Stores and links').boundingBox()];
  expect(b.y).toBe(a.y);
  expect(b.height).toBe(a.height);
});
