// Station log: the list, search, log the station on the dial, every reading
// of an entry, rename, note, tune, delete, download.
import { expect, test } from '@playwright/test';
import { countReads, holdAnswers, liveRadio, nextPost } from './radio.js';

const t0 = Math.floor(new Date(2026, 9, 10, 9, 30).getTime() / 1000);
const entries = [
  { id: 4, time: t0 - 86400 * 2, real: true, band: 'FM', khz: 98300, level_dbuv: 450, usn: 30, multipath: 120, cochannel: 0, snr: 30, stereo: true, bw_khz: 168, name: 'CITY    ', pi: '26FF', rt: 'Song - Artist', note: null },
  { id: 5, time: t0 - 3600, real: true, band: 'MW', khz: 738, level_dbuv: 527, usn: 0, multipath: 0, cochannel: 12, snr: 34, stereo: false, bw_khz: 4, name: null, pi: null, rt: null, note: 'Heard at night' },
  { id: 6, time: t0, real: true, band: 'FM', khz: 106400, level_dbuv: 296, usn: 88, multipath: 78, cochannel: 0, snr: 22, stereo: true, bw_khz: 184, name: ' MAGIC  ', pi: '1064', rt: null, note: null },
];

// The radio on the live socket, with its log, on a day of its own. Gives the
// socket's end, to push a changed state.
async function radio(page, list = entries) {
  await page.clock.setFixedTime(new Date(2026, 9, 10, 12, 0));
  const live = await liveRadio(page);
  await page.route('**/api/log', (route) =>
    route.request().method() === 'GET'
      ? route.fulfill({ body: [{ n: list.length }, ...list].map((x) => JSON.stringify(x)).join('\n') + '\n' })
      : route.fallback(),
  );
  return live;
}

// Every entry row, across the day cards; the first card is the toolbar.
const rows = (page) => page.locator('main section').filter({ hasNot: page.getByLabel('Search') }).locator('button');

test('a card for each day, newest first; a row gives the time, the day is the heading', async ({ page }) => {
  await radio(page, [entries[0], { ...entries[0], id: 3, time: t0 - 86400, note: 'Yesterday' }, ...entries.slice(1)]);
  await page.goto('/#log');
  const headings = page.locator('main section h2');
  await expect(headings).toHaveText(['Today · 2', 'Yesterday · 1', /^(October 8|8 October) · 1$/]);
  await expect(page.getByText('4 of 4')).toBeVisible();
  await expect(rows(page).nth(0)).toContainText('09:30');
  await expect(rows(page).nth(0)).not.toContainText('Oct');
});

test('newest first, with name or PI or ---, readings and a note', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await expect(rows(page)).toHaveCount(3);
  await expect(rows(page).nth(0)).toContainText('106.40');
  await expect(rows(page).nth(0)).toContainText('MAGIC');
  await expect(rows(page).nth(1)).toContainText('738');
  await expect(rows(page).nth(1)).toContainText('---');
  await expect(rows(page).nth(1)).toContainText('Heard at night');
  await expect(rows(page).nth(2)).toContainText('CITY');
  await expect(rows(page).nth(2)).toContainText('45.0 dBµV');
});

test('a row shows every reading; on AM co-channel in place of noise and multipath', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  for (const text of ['FM', '29.6 dBµV', '22 dB', 'Noise', '9 %', 'Multipath', '8 %', '184 kHz', 'Stereo', '1064']) await expect(rows(page).nth(0)).toContainText(text);
  await expect(rows(page).nth(1)).toContainText('Co-channel');
  await expect(rows(page).nth(1)).not.toContainText('Noise');
  await expect(rows(page).nth(1)).not.toContainText('Multipath');
});

test('a name typed by a person says so, in the row and the dialog', async ({ page }) => {
  await radio(page, entries.map((e) => (e.id === 6 ? { ...e, renamed: true } : { ...e, renamed: false })));
  await page.goto('/#log');
  await expect(rows(page).nth(0)).toContainText('renamed');
  await expect(rows(page).nth(2)).not.toContainText('renamed');
  await rows(page).nth(0).click();
  await expect(page.getByRole('dialog')).toContainText('Name, typed');
});

test('search looks in name, frequency, PI, note and radio text', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  const search = page.getByLabel('Search');
  for (const [q, n] of [['magic', 1], ['738', 1], ['26ff', 1], ['night', 1], ['artist', 1], ['fm', 2], ['nothing like it', 0]]) {
    await search.fill(q);
    await expect(rows(page)).toHaveCount(n);
  }
});

test('log this station', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  const sent = nextPost(page, '/api/log');
  await page.getByRole('button', { name: 'Log this station' }).click();
  expect((await sent) ?? '').toBe('');
});

test('an entry: every reading, tune to it', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await rows(page).nth(2).click();
  const dialog = page.getByRole('dialog');
  for (const text of ['98.30', 'Song - Artist', '45.0 dBµV', '30 dB', 'Multipath', '12 %', '168 kHz', 'Stereo', '26FF']) await expect(dialog).toContainText(text);
  const sent = nextPost(page, '/api/tune');
  await dialog.getByRole('button', { name: 'Tune' }).click();
  expect(await sent).toBe('khz=98300');
});

test('an entry: its programme type and country when the radio kept them, else neither', async ({ page }) => {
  await radio(page, entries.map((e) => (e.id === 4 ? { ...e, pty: 10, ptn: 'Pop Music', cty: 'DE' } : { ...e, pty: null, ptn: null, cty: null })));
  await page.goto('/#log');
  await rows(page).nth(0).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog).toContainText('106.40');
  await expect(dialog).not.toContainText('Programme type');
  await expect(dialog).not.toContainText('Country');
  await dialog.getByRole('button', { name: 'Cancel' }).click();
  await rows(page).nth(2).click();
  for (const text of ['Programme type', 'Pop Music', 'Country', 'Germany']) await expect(dialog).toContainText(text);
});

test('an entry with programme type 0, none set, shows no programme type', async ({ page }) => {
  await radio(page, entries.map((e) => ({ ...e, pty: 0, ptn: 'None', cty: null })));
  await page.goto('/#log');
  await rows(page).nth(0).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog).toContainText('106.40');
  await expect(dialog).not.toContainText('Programme type');
  await expect(dialog).not.toContainText('None');
});

test('on AM the co-channel reading, no noise', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await rows(page).nth(1).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog).toContainText('Co-channel');
  await expect(dialog).not.toContainText('Noise');
});

test('rename and note: sent with the id and time the list gave', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await rows(page).nth(0).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByLabel('Name')).toHaveValue('MAGIC');
  await dialog.getByLabel('Name').fill('MAGIC FM long');
  await expect(dialog.getByLabel('Name')).toHaveValue('MAGIC FM');
  await dialog.getByLabel('Note').fill('Clear all evening — fine');
  await expect(dialog.getByLabel('Note')).toHaveValue('Clear all evening  fine');
  const sent = nextPost(page, '/api/log');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect(await sent).toBe(`do=edit&id=6&time=${t0}&khz=106400&note=Clear+all+evening++fine&name=MAGIC+FM`);
});

test('a note alone does not send the name', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await rows(page).nth(0).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel('Note').fill('Just a note');
  const sent = nextPost(page, '/api/log');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect(await sent).toBe(`do=edit&id=6&time=${t0}&khz=106400&note=Just+a+note`);
});

test('delete', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await rows(page).nth(1).click();
  const sent = nextPost(page, '/api/log');
  await page.getByRole('dialog').getByRole('button', { name: 'Delete' }).click();
  expect(await sent).toBe(`do=delete&id=5&time=${t0 - 3600}&khz=738`);
});

test('an entry from firmware with no ids can be read and tuned, not changed', async ({ page }) => {
  await radio(page, entries.map(({ id, ...e }) => e));
  await page.goto('/#log');
  await rows(page).nth(0).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByLabel('Name')).toBeDisabled();
  await expect(dialog.getByRole('button', { name: 'Delete' })).toHaveCount(0);
  await expect(dialog.getByRole('button', { name: 'Tune' })).toBeVisible();
});

test('download is the CSV file', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  await expect(page.getByRole('link', { name: 'Download' })).toHaveAttribute('href', '/api/log.csv');
});

test('the toolbar fits a phone of 320 px', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'phone', 'a phone only');
  await page.setViewportSize({ width: 320, height: 700 });
  await radio(page);
  await page.goto('/#log');
  const spill = await page
    .locator('main section')
    .first()
    // The tools; the frequency is a button too, and tall on purpose.
    .locator('button:not([title="Tap to type a frequency"]), a')
    .evaluateAll((els) => els.filter((e) => e.scrollWidth > e.clientWidth || e.getBoundingClientRect().height > 45).map((e) => e.textContent));
  expect(spill).toEqual([]);
});

test('Presets & Log: #log opens the Log tab, and the tabs move between the two', async ({ page }) => {
  await radio(page);
  await page.goto('/#log');
  const tabs = page.getByRole('radiogroup', { name: 'Presets or log' });
  await expect(tabs.getByRole('radio', { name: 'Log' })).toHaveAttribute('aria-checked', 'true');
  await expect(page.getByRole('button', { name: 'Log this station' })).toBeVisible();
  await tabs.getByRole('radio', { name: 'Presets' }).click();
  await expect(page).toHaveURL(/#presets$/);
  await expect(page.getByRole('button', { name: 'Save current station' })).toBeVisible();
  await page.goBack();
  await expect(page.getByRole('button', { name: 'Log this station' })).toBeVisible();
});

test('a new state draws the station card again, not the page or the list; Log this names the station on the dial', async ({ page }) => {
  const live = await radio(page);
  await page.goto('/#log');
  await expect(rows(page)).toHaveCount(3);
  // Counts each part drawn, by name, through Preact's hook that runs before
  // a part is drawn. Its short name is Preact's own, pinned with Preact. The
  // page's own copy of Preact is the one main.jsx imports, as the dev server
  // writes its address.
  await page.evaluate(async () => {
    const main = await (await fetch('/src/main.jsx')).text();
    const { options } = await import(main.match(/from\s*["']([^"']*\/deps\/preact\.js[^"']*)["']/)[1]);
    const before = options.__r;
    window.drawn = {};
    options.__r = (v) => {
      const name = v.type?.name;
      if (name) window.drawn[name] = (window.drawn[name] ?? 0) + 1;
      before?.(v);
    };
  });
  const tune = (khz, f) => live.push((s) => Object.assign(s.tun, { bnd: 'FM', khz, f, unt: 'MHz' }));
  tune(99900, '99.90');
  tune(100100, '100.10');
  const top = page.locator('main section').first();
  await expect(top).toContainText('100.10');
  const drawn = await page.evaluate(() => window.drawn);
  expect(drawn.StationNow).toBeGreaterThan(0);
  expect(drawn.App).toBeUndefined();
  expect(drawn.StationLog).toBeUndefined();
  expect(drawn.EntryRow).toBeUndefined();
  await page.getByRole('button', { name: 'Log this station' }).click();
  await expect(page.getByText('Logged 100.10 MHz.')).toBeVisible();
});

test('a log that could not be read says so, and is read again by itself, 5 s on', async ({ page }) => {
  await page.clock.install();
  const live = await radio(page);
  let reads = 0;
  await page.route('**/api/log', (route) => (route.request().method() === 'GET' && ++reads === 1 ? route.fulfill({ status: 503, body: '' }) : route.fallback()));
  await page.goto('/#log');
  await expect(page.getByText('The log could not be read. Trying again.')).toBeVisible();
  // The first state that comes 5 s on.
  await page.clock.runFor(5000);
  live.push();
  await expect(rows(page)).toHaveCount(3);
  await expect(page.getByText('could not be read')).toHaveCount(0);
});

test('the list is read again when the radio logs a station, as by its own ENTER', async ({ page }) => {
  const now = { list: entries.slice(0, 2) };
  await page.clock.setFixedTime(new Date(2026, 9, 10, 12, 0));
  await page.route('**/api/log', (route) => route.fulfill({ body: [{ n: now.list.length }, ...now.list].map((x) => JSON.stringify(x)).join('\n') + '\n' }));
  const live = await liveRadio(page, (s) => (s.log.n = now.list.length));
  await page.goto('/#log');
  await expect(rows(page)).toHaveCount(2);
  now.list = entries;
  live.push((s) => (s.log.n = now.list.length));
  await expect(rows(page)).toHaveCount(3);
});

test('a log whose storage did not start says so, here and in Home\'s Last heard, not that nothing is logged', async ({ page }) => {
  await liveRadio(page, (s) => (s.log = { fit: false, n: 0 }));
  await page.route('**/api/log**', (route) => route.fulfill({ body: '{"n":0}\n' }));
  const words = 'The station log is not available: its storage did not start.';
  await page.goto('/#log');
  await expect(page.getByText(words)).toBeVisible();
  await expect(page.getByText('Nothing logged yet')).toHaveCount(0);
  await page.goto('/');
  await expect(page.locator('section').filter({ hasText: 'Last heard' })).toContainText(words);
  await expect(page.getByText('Nothing logged yet')).toHaveCount(0);
});

test('a read that brings the count but no entries is a failed read, not an empty log', async ({ page }) => {
  await liveRadio(page);
  await page.route('**/api/log', (route) => route.fulfill({ body: '{"n":3}\n' }));
  await page.goto('/#log');
  await expect(page.getByText('The log could not be read. Trying again.')).toBeVisible();
  await expect(page.getByText('Nothing logged yet')).toHaveCount(0);
});

test('the log is not read while a DX scan runs, which logs as it goes, and is read once when the scan ends', async ({ page }) => {
  const reads = await countReads(page, '/api/log');
  const live = await radio(page);
  await page.goto('/#log');
  await expect(rows(page)).toHaveCount(3);
  // The frequency in the top card shows that the page has taken each state.
  const dial = page.getByTitle('Tap to type a frequency');
  live.push((s) => {
    s.dx.scn = 'running';
    s.log.n = 4;
    Object.assign(s.tun, { khz: 99900, f: '99.90' });
  });
  await expect(dial).toContainText('99.90');
  live.push((s) => {
    s.dx.scn = 'stopped';
    s.log.n = 5;
    Object.assign(s.tun, { khz: 100000, f: '100.00' });
  });
  await expect(dial).toContainText('100.00');
  expect(await reads()).toBe(1);
  live.push((s) => (s.dx.scn = 'idle'));
  await expect.poll(reads).toBe(2);
});

test('tune to an entry is drawn at once, on its band, before the radio answers', async ({ page }) => {
  await radio(page);
  const answer = await holdAnswers(page, '**/api/tune');
  await page.goto('/#log');
  // The MW entry, newest but one.
  await rows(page).nth(1).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Tune' }).click();
  // Before the radio answers.
  await expect(page.getByRole('link', { name: /738/ })).toContainText('kHz');
  answer();
});
