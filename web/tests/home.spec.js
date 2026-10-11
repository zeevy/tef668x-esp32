// Home: the reading, the dial's buttons, the presets strip, the last
// stations heard, and the radio's own state.
import fs from 'node:fs';
import { expect, test } from '@playwright/test';
import { holdAnswers, liveRadio, nextPost, recorded } from './radio.js';
import { dateLine } from '../src/data/format.js';

// The radio on the live socket, on Home's station, its state changed by
// `change`. Gives the socket's end, to push a changed state.
function withState(page, change) {
  return liveRadio(page, (s) => {
    station(s);
    change(s);
  });
}

// Home's own data, so a new recording of the radio cannot change what these
// tests see: sixteen presets, 92.70 in slot 2 and 106.40 in slot 9; the three
// newest log entries; and the radio on 106.40, MAGIC, in stereo, on preset 9.
const PRESETS =
  'slot,band,frequency,bandwidth,name,pi\n' +
  [91100, 92700, 93500, 94300, 95000, 98300, 101900, 102800, 106400, 104000, 90000, 102800, 106400, 106400, 106400, 106400]
    .map((khz, i) => `${i + 1},FM,${khz},0,,`)
    .join('\n') +
  '\n';
const LOG = [
  { n: 77 },
  { id: 74, time: 1791508426, real: true, band: 'FM', khz: 102200, level_dbuv: 11, usn: 144, multipath: 120, cochannel: 0, snr: 10, stereo: false, bw_khz: 114, name: null, pi: null, rt: null, note: null, renamed: false },
  { id: 75, time: 1791508798, real: true, band: 'MW', khz: 738, level_dbuv: 527, usn: 50, multipath: 0, cochannel: 0, snr: 34, stereo: false, bw_khz: 4, name: null, pi: null, rt: null, note: null, renamed: false },
  { id: 76, time: 1791559355, real: true, band: 'SW', khz: 2500, level_dbuv: 104, usn: 166, multipath: 0, cochannel: 0, snr: 12, stereo: false, bw_khz: 4, name: null, pi: null, rt: null, note: null, renamed: false },
];
function station(s) {
  Object.assign(s.tun, { bnd: 'FM', khz: 106400, f: '106.40', unt: 'MHz', st: true, plt: true, mno: false, pst: 9 });
  Object.assign(s.tun.rds ?? (s.tun.rds = {}), { syn: true, ps: ' MAGIC  ', pi: '1064', pty: 12, ptn: 'Easy Listening' });
}
test.beforeEach(async ({ page }) => {
  await page.route('**/api/presets.csv', (route) => route.fulfill({ body: PRESETS }));
  await page.route('**/api/log?last=3', (route) => route.fulfill({ body: LOG.map((x) => JSON.stringify(x)).join('\n') + '\n' }));
  await page.route('**/api/state', (route) => {
    const s = structuredClone(recorded);
    station(s);
    return route.fulfill({ json: s });
  });
});

// The card of presets, its keys, and the strip that holds them; the key that
// saves the station on the dial has no pressed state.
const presetCard = (page) => page.locator('section').filter({ hasText: 'Presets' });
const presetKeys = (page) => presetCard(page).locator('button[aria-pressed]');
const keyStrip = (page) => presetKeys(page).first().locator('..');

test('shows what the radio is receiving', async ({ page }) => {
  await withState(page, (s) => {
    s.clk.now = '15:03';
    Object.assign(s.tun, { sig: 296, snr: 22, mut: false });
    Object.assign(s.tun.rds, { tp: false, pty: 12, ptn: 'Easy Listening', pi: '1064', ps: ' MAGIC  ' });
  });
  await page.goto('/');
  const card = page.locator('section').filter({ hasText: 'Signal to noise' });
  await expect(card).toContainText('106.40');
  await expect(card).toContainText('MAGIC');
  await expect(card).toContainText('Easy Listening · PI 1064');
  await expect(card).toContainText('15:03');
  await expect(card).toContainText('29.6 dBµV');
  await expect(card).toContainText('22 dB');
  for (const chip of ['FM', 'Stereo', 'RDS']) await expect(card.getByText(chip, { exact: true })).toBeVisible();
  for (const chip of ['TP', 'Muted']) await expect(card.getByText(chip, { exact: true })).toHaveCount(0);
});

test('the dial buttons send what the radio expects, then read the state again', async ({ page }) => {
  await page.goto('/');
  const calls = [
    ['Seek down', '/api/seek', 'dir=down'],
    ['Step down', '/api/step', 'stp=-1'],
    ['Mute', '/api/mute', 'on=1'],
    ['Step up', '/api/step', 'stp=1'],
    ['Seek up', '/api/seek', 'dir=up'],
  ];
  for (const [label, path, body] of calls) {
    const sent = nextPost(page, path);
    const reread = page.waitForRequest('**/api/state');
    await page.locator('main').getByRole('button', { name: label, exact: true }).click();
    expect(await sent).toBe(body);
    await reread;
  }
});

test('muted: says so, and the button turns the sound on', async ({ page }) => {
  await withState(page, (s) => (s.tun.mut = true));
  await page.goto('/');
  await expect(page.getByText('Muted', { exact: true })).toBeVisible();
  const sent = nextPost(page, '/api/mute');
  await page.locator('main').getByRole('button', { name: 'Sound on' }).click();
  expect(await sent).toBe('on=0');
});

test('nothing goes grey or busy while a change is sent', async ({ page }) => {
  const answer = await holdAnswers(page, '**/api/seek', { body: 'seeking up' });
  await page.goto('/');
  await expect(page.locator('main').getByRole('button', { name: 'Mute' })).toBeVisible();
  const sent = nextPost(page, '/api/seek');
  await page.getByRole('button', { name: 'Seek up' }).click();
  await sent;
  // While the radio has not answered.
  for (const name of ['Seek down', 'Step down', 'Mute', 'Step up', 'Seek up']) {
    await expect(page.locator('main').getByRole('button', { name, exact: true })).toBeEnabled();
  }
  await expect(page.locator('main [disabled], main [aria-busy]')).toHaveCount(0);
  answer();
});

test('quick taps all reach the radio, one after another, in order', async ({ page }) => {
  const sent = [];
  let open = 0;
  await page.route('**/api/step', async (route) => {
    open++;
    sent.push([route.request().postData(), open]);
    await new Promise((r) => setTimeout(r, 300));
    open--;
    await route.fulfill({ body: 'ok' });
  });
  await page.goto('/');
  await page.getByRole('button', { name: 'Step up' }).click();
  await page.getByRole('button', { name: 'Step down' }).click();
  await page.getByRole('button', { name: 'Step up' }).click();
  await expect.poll(() => sent.length).toBe(3);
  expect(sent).toEqual([
    ['stp=1', 1],
    ['stp=-1', 1],
    ['stp=1', 1],
  ]);
});

test('a refusal shows what the radio said', async ({ page }) => {
  await page.route('**/api/step', (route) => route.fulfill({ status: 409, body: 'A seek is running.\n' }));
  await page.goto('/');
  await page.getByRole('button', { name: 'Step up' }).click();
  await expect(page.getByRole('status').filter({ hasText: 'A seek is running.' })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Step up' })).toBeEnabled();
});

test('a refusal for the PIN opens the prompt and no note', async ({ page }) => {
  await page.route('**/api/mute', (route) => route.fulfill({ status: 403, body: 'Sign in first.' }));
  await page.goto('/');
  await page.locator('main').getByRole('button', { name: 'Mute' }).click();
  await expect(page.getByRole('dialog').getByLabel('PIN')).toBeVisible();
  await expect(page.getByRole('status')).toHaveCount(0);
});

test.describe('the presets', () => {
  test('every one stored, the one on the dial marked, a tap recalls', async ({ page }) => {
    await withState(page, (s) => (s.tun.pst = 9));
    await page.goto('/');
    await expect(presetKeys(page)).toHaveCount(16);
    const on = presetCard(page).getByRole('button', { pressed: true });
    await expect(on).toHaveCount(1);
    await expect(on).toHaveAccessibleName('FM P09 106.40 MHz');
    const sent = nextPost(page, '/api/presets');
    await presetKeys(page).filter({ hasText: '92.70' }).click();
    expect(await sent).toBe('do=recall&slot=2');
  });

  test('none stored', async ({ page }) => {
    await page.route('**/api/presets.csv', (route) => route.fulfill({ body: 'slot,band,frequency,bandwidth,name,pi\n' }));
    await page.goto('/');
    await expect(page.getByText('No presets stored yet.')).toBeVisible();
  });

  test('a name with a comma and a quote, and an AM preset', async ({ page }) => {
    await page.route('**/api/presets.csv', (route) =>
      route.fulfill({ body: 'slot,band,frequency,bandwidth,name,pi\r\n1,FM,98300,0,"A,""B""",26FF\r\n2,MW,738,0,,\r\n' }),
    );
    await page.goto('/');
    await expect(presetKeys(page)).toHaveCount(2);
    await expect(presetKeys(page).nth(0)).toHaveAccessibleName('FM P01 98.30 A,"B"');
    await expect(presetKeys(page).nth(1)).toHaveAccessibleName('MW P02 738 kHz');
  });
});

test('last heard: the three newest first', async ({ page }) => {
  await page.goto('/');
  const rows = page.locator('section').filter({ hasText: 'Last heard' }).getByRole('button');
  await expect(rows).toHaveCount(3);
  await expect(rows).toContainText(['2500', '738', '102.20']);
  await expect(rows.nth(0)).toContainText('10.4 dBµV · mono');
});

test('nothing logged yet', async ({ page }) => {
  await page.route('**/api/log?last=3', (route) => route.fulfill({ body: '{"n":0}\n' }));
  await page.goto('/');
  await expect(page.getByText('Nothing logged yet.')).toBeVisible();
});

test('the state of the radio itself', async ({ page }) => {
  await withState(page, (s) => {
    Object.assign(s, { rssi: -61, up: 2513, ver: '0.2.0' });
    s.bat.boot = 4174;
    s.upd.ver = null;
  });
  await page.goto('/');
  for (const text of ['-61 dBm', '4.17 V', '41 min', '0.2.0', 'Up to date']) {
    await expect(page.getByText(text, { exact: true })).toBeVisible();
  }
});

test('on its hotspot, the update check in System\'s words, and a battery with no reading', async ({ page }) => {
  // Home itself, not the Wi-Fi setup the hotspot opens on.
  await page.addInitScript(() => sessionStorage.setItem('tef668x-setup-skipped', '1'));
  const live = await withState(page, (s) => {
    Object.assign(s, { net: 'ap', rssi: null });
    s.upd = { st: 'found', ver: '0.3.0', sz: null };
    s.bat = { fit: true, mv: null, boot: null };
  });
  await page.goto('/');
  for (const text of ['Hotspot', '0.3.0 is out', 'No reading']) {
    await expect(page.getByText(text, { exact: true })).toBeVisible();
  }
  // Joining a network, with no signal yet, is not the hotspot.
  live.push((s) => Object.assign(s, { net: 'joining', rssi: null }));
  await expect(page.getByText('Hotspot', { exact: true })).toHaveCount(0);
  for (const [st, words] of [
    ['off', 'Checks are off'],
    ['wait', 'Checks when the Wi-Fi is up'],
    ['checking', 'Checking now'],
    ['failed', 'The last check failed'],
  ]) {
    live.push((s) => (s.upd = { st, ver: null, sz: null }));
    await expect(page.getByText(words, { exact: true })).toBeVisible();
  }
  await expect(page.getByText('Up to date', { exact: true })).toHaveCount(0);
  live.push((s) => (s.bat = { fit: false, mv: null, boot: null }));
  await expect(page.getByText('No battery', { exact: true })).toBeVisible();
});

test('with no quality reading yet, every reading is -- with an empty bar, never NaN', async ({ page }) => {
  await withState(page, (s) => {
    for (const k of ['sig', 'snr', 'usn', 'wam', 'off', 'mod']) delete s.tun[k];
  });
  await page.goto('/');
  const player = page.locator('main section').first();
  const level = player.getByText('Level', { exact: true }).locator('../..');
  await expect(level).toContainText('-- dBµV');
  await expect(player.getByText('Signal to noise', { exact: true }).locator('..')).toContainText('-- dB');
  await expect(level.locator('[style]')).toHaveAttribute('style', /width: 0%/);
  for (const label of ['Noise', 'Multipath', 'Width', 'Offset']) {
    await expect(player.getByText(label, { exact: true }).locator('..')).toContainText('--');
  }
  await expect(player).not.toContainText('NaN');
  await expect(player).not.toContainText('undefined');
});

test('warns while the PIN is the first one, with a way to change it', async ({ page }) => {
  await page.goto('/');
  const warning = page.getByText('The radio still uses the default PIN.');
  await expect(warning).toBeVisible();
  await expect(page.getByRole('link', { name: 'Change' })).toHaveAttribute('href', '#network');
});

test('no PIN warning once the PIN is changed', async ({ page }) => {
  await withState(page, (s) => (s.dpn = false));
  await page.goto('/');
  await expect(page.getByText('106.40').first()).toBeVisible();
  await expect(page.getByText('The PIN is still')).toHaveCount(0);
});

test('the volume: shows what the radio has, and sends a new one when let go', async ({ page }) => {
  await withState(page, (s) => (s.tun.vol = -35));
  await page.goto('/');
  const slider = page.locator('main').getByRole('slider', { name: 'Volume' });
  await expect(slider).toHaveValue('-35');
  await expect(page.getByText('-35 dB', { exact: true })).toBeVisible();
  const sent = nextPost(page, '/api/volume');
  await slider.fill('-20');
  expect(await sent).toBe('db=-20');
});

test('the state of the radio sits just under the player', async ({ page }) => {
  await page.goto('/');
  const cards = page.locator('main section');
  await expect(cards.nth(1)).toContainText('Battery at start');
  await expect(cards.nth(2)).toContainText('Presets');
});

test.describe('a change is drawn at once from the radio state the page keeps', () => {
  // The state answers with mut and vol as the radio would after the change.
  // On the page's polling, as the dev server turns the live socket away:
  // these are of the read that follows the change.
  async function radioThatTakes(page) {
    const radio = { mut: false, vol: recorded.tun.vol };
    await page.route('**/api/mute', (route) => {
      radio.mut = new URLSearchParams(route.request().postData()).get('on') === '1';
      return route.fulfill({ body: radio.mut ? 'muted' : 'unmuted' });
    });
    await page.route('**/api/state', (route) => {
      const s = structuredClone(recorded);
      station(s);
      Object.assign(s.tun, radio);
      return route.fulfill({ json: s });
    });
    return radio;
  }

  test('before the radio answers', async ({ page }) => {
    const answer = await holdAnswers(page, '**/api/mute', { body: 'muted' });
    await page.goto('/');
    await page.locator('main').getByRole('button', { name: 'Mute' }).click();
    await expect(page.getByText('Muted', { exact: true })).toBeVisible();
    await expect(page.locator('main').getByRole('button', { name: 'Sound on' })).toBeVisible();
    answer();
  });

  test('and the read that confirms it changes nothing on the screen', async ({ page }) => {
    await page.clock.install();
    await radioThatTakes(page);
    await page.goto('/');
    await expect(page.locator('main').getByRole('button', { name: 'Mute' })).toBeEnabled();
    const reply = page.waitForResponse('**/api/mute');
    await page.locator('main').getByRole('button', { name: 'Mute' }).click();
    await expect(page.locator('main').getByRole('button', { name: 'Sound on' })).toBeEnabled();
    await reply;
    // Every change to the page from here on, while the confirming read lands.
    await page.evaluate(() => {
      window.changes = 0;
      // The level chart gains a bar with every state; that is what it is for.
      const chart = (n) => (n.nodeType === 1 ? n : n.parentElement)?.closest('[aria-label="Level, last 60 s"]');
      new MutationObserver((m) => (window.changes += m.filter((x) => !chart(x.target)).length)).observe(document.querySelector('main'), {
        subtree: true,
        childList: true,
        characterData: true,
        attributes: true,
      });
    });
    // The next read, 5 s on, is the one that confirms the mute.
    const landed = page.waitForEvent('requestfinished', (r) => r.url().endsWith('/api/state'));
    await page.clock.runFor(5000);
    await landed;
    await page.evaluate(() => {});
    expect(await page.evaluate(() => window.changes)).toBe(0);
    await expect(page.getByText('Muted', { exact: true })).toBeVisible();
  });

  test('and a read that started before the radio took it cannot undo it', async ({ page }) => {
    await page.clock.install();
    const radio = await radioThatTakes(page);
    let holdNext = false;
    let held = null;
    await page.route('**/api/state', (route) => {
      const s = structuredClone(recorded);
      Object.assign(s.tun, radio);
      if (!holdNext) return route.fulfill({ json: s });
      holdNext = false;
      held = () => route.fulfill({ json: s });
    });
    await page.goto('/');
    await expect(page.locator('main').getByRole('button', { name: 'Mute' })).toBeEnabled();
    // The next read, 5 s on, starts while unmuted and is held.
    holdNext = true;
    await page.clock.runFor(5000);
    await expect.poll(() => held).toBeTruthy();
    const reply = page.waitForResponse('**/api/mute');
    await page.locator('main').getByRole('button', { name: 'Mute' }).click();
    await reply;
    // The held read lands after the radio muted.
    const landed = page.waitForEvent('requestfinished', (r) => r.url().endsWith('/api/state'));
    await held();
    await landed;
    await page.evaluate(() => {});
    await expect(page.getByText('Muted', { exact: true })).toBeVisible();
  });

  test('and a refusal puts back what the radio has', async ({ page }) => {
    const answer = await holdAnswers(page, '**/api/mute', { status: 409, body: 'Not now.' });
    await page.goto('/');
    await page.locator('main').getByRole('button', { name: 'Mute' }).click();
    await expect(page.getByText('Muted', { exact: true })).toBeVisible();
    answer();
    await expect(page.getByRole('status').filter({ hasText: 'Not now.' })).toBeVisible();
    await expect(page.getByText('Muted', { exact: true })).toHaveCount(0);
  });

  test('a preset recall marks it and tunes the header at once', async ({ page }) => {
    const answer = await holdAnswers(page, '**/api/presets', { body: 'FM 92.70 MHz' });
    await page.goto('/');
    await presetKeys(page).filter({ hasText: '92.70' }).click();
    // Before the radio answers.
    await expect(presetCard(page).getByRole('button', { pressed: true })).toHaveAccessibleName('FM P02 92.70 MHz');
    await expect(page.getByRole('link', { name: /92\.70/ })).toBeVisible();
    answer();
  });
});

test('the volume bar is full at 0 dB, the top of the knob', async ({ page }) => {
  await withState(page, (s) => (s.tun.vol = 0));
  await page.goto('/');
  const slider = page.locator('main').getByRole('slider', { name: 'Volume' });
  await expect(slider).toHaveValue('0');
  await expect(slider).toHaveAttribute('style', /--fill: 100%/);
});

test('the five buttons sit together, under the frequency', async ({ page }) => {
  await page.goto('/');
  const boxes = [];
  for (const name of ['Seek down', 'Step down', 'Mute', 'Step up', 'Seek up']) {
    boxes.push(await page.locator('main').getByRole('button', { name, exact: true }).boundingBox());
  }
  for (let i = 1; i < boxes.length; i++) {
    expect(boxes[i].x - (boxes[i - 1].x + boxes[i - 1].width)).toBe(8);
    expect(boxes[i].y).toBe(boxes[0].y);
  }
  const number = await page.locator('main').getByTitle('Tap to type a frequency').first().getByText('106.40', { exact: true }).boundingBox();
  expect(boxes[0].x).toBe(number.x);
});

test('the state of the radio and the presets: side by side on a laptop, one above the other on a phone', async ({ page }, testInfo) => {
  await page.goto('/');
  const facts = await page.locator('main section').filter({ hasText: 'Battery at start' }).boundingBox();
  const presets = await page.locator('main section').filter({ hasText: 'Presets' }).boundingBox();
  if (testInfo.project.name === 'laptop') {
    expect(presets.y).toBe(facts.y);
    expect(presets.height).toBe(facts.height);
    expect(presets.x).toBeGreaterThan(facts.x + facts.width);
  } else {
    expect(presets.y).toBeGreaterThan(facts.y + facts.height);
  }
});

test('nothing on the page moves when RDS comes or goes', async ({ page }) => {
  const radio = await liveRadio(page, station);
  await page.goto('/');
  const player = page.locator('main section').filter({ hasText: 'Signal to noise' });
  const presets = page.locator('main section').filter({ hasText: 'Presets' });
  await expect(player).toContainText('MAGIC');
  const withRds = [await player.boundingBox(), await presets.boundingBox()];
  radio.push((s) => (s.tun.rds = {}));
  await expect(player).not.toContainText('MAGIC');
  const without = [await player.boundingBox(), await presets.boundingBox()];
  expect(without[0].height).toBe(withRds[0].height);
  expect(without[1].y).toBe(withRds[1].y);
});

test('the dial buttons, the volume and the presets show the hand on hover', async ({ page }) => {
  await page.goto('/');
  const cursor = (l) => l.evaluate((e) => getComputedStyle(e).cursor);
  for (const name of ['Seek down', 'Step down', 'Mute', 'Step up', 'Seek up']) {
    expect(await cursor(page.locator('main').getByRole('button', { name, exact: true }))).toBe('pointer');
  }
  expect(await cursor(page.locator('main').getByRole('slider', { name: 'Volume' }))).toBe('pointer');
  const preset = page.locator('section').filter({ hasText: 'Presets' }).getByRole('button').first();
  expect(await cursor(preset)).toBe('pointer');
});

test('with no RDS: --- for the name and the date under it, as on the radio', async ({ page }) => {
  await page.clock.setFixedTime(new Date(2026, 9, 10, 12, 0));
  await withState(page, (s) => (s.tun.rds = {}));
  await page.goto('/');
  const player = page.locator('main section').filter({ hasText: 'Signal to noise' });
  await expect(player.getByText('---', { exact: true })).toBeVisible();
  await expect(player.getByText('SATURDAY, 10th October 2026')).toBeVisible();
});

test('with no RDS on a named preset, its name', async ({ page }) => {
  await withState(page, (s) => {
    s.tun.rds = {};
    s.tun.pst = 1;
  });
  await page.route('**/api/presets.csv', (route) =>
    route.fulfill({ body: 'slot,band,frequency,bandwidth,name,pi\n1,FM,106400,0,MY FM,\n' }),
  );
  await page.goto('/');
  const player = page.locator('main section').filter({ hasText: 'Signal to noise' });
  await expect(player.getByText('MY FM', { exact: true })).toBeVisible();
});

test('a programme type of 0, none set, is not shown', async ({ page }) => {
  await withState(page, (s) => Object.assign(s.tun.rds, { pty: 0, ptn: 'None' }));
  await page.goto('/');
  const player = page.locator('main section').filter({ hasText: 'Signal to noise' });
  await expect(player).toContainText('PI 1064');
  await expect(player).not.toContainText('None');
});

test('the date line writes the day as the radio does', () => {
  const lines = [1, 2, 3, 4, 11, 12, 13, 21, 22, 23, 31].map((d) => dateLine(new Date(2026, 9, d)));
  expect(lines.map((l) => l.split(' ')[1])).toEqual(['1st', '2nd', '3rd', '4th', '11th', '12th', '13th', '21st', '22nd', '23rd', '31st']);
  expect(lines[0]).toBe('THURSDAY, 1st October 2026');
});

test('the preset keys are one width, 128 px, the first in view whole at the start and the one on the dial second', async ({ page }) => {
  await page.goto('/');
  await expect(presetKeys(page)).toHaveCount(16);
  const strip = keyStrip(page);
  const look = () =>
    strip.evaluate((el) => ({
      box: el.getBoundingClientRect().toJSON(),
      keys: [...el.children].map((k) => ({ ...k.getBoundingClientRect().toJSON(), on: k.getAttribute('aria-pressed') === 'true' })),
    }));
  const inView = ({ box, keys }) => keys.filter((k) => k.right > box.left + 1 && k.left < box.right - 1);
  // The strip moves to the one on the dial once the keys are drawn.
  await expect.poll(async () => inView(await look())[1]?.on).toBe(true);
  const { box, keys } = await look();
  for (const k of keys) expect(k.width).toBe(128);
  const seen = inView({ box, keys });
  expect(Math.abs(seen[0].left - box.left)).toBeLessThanOrEqual(0.5);
  expect(seen[1].on).toBe(true);
});

test('Last heard asks the radio for the newest three entries only', async ({ page }) => {
  const asked = page.waitForRequest((r) => new URL(r.url()).pathname === '/api/log');
  await page.goto('/');
  expect(new URL((await asked).url()).searchParams.get('last')).toBe('3');
});

test('arrows on both sides of the preset strip show when there are more keys that way, and move it', async ({ page }) => {
  await withState(page, (s) => (s.tun.pst = 1));
  await page.goto('/');
  const earlier = page.getByRole('button', { name: 'Earlier presets' });
  const later = page.getByRole('button', { name: 'Later presets' });
  // At the start: nothing before, more after.
  await expect(earlier).toBeHidden();
  await expect(later).toBeVisible();
  const strip = keyStrip(page);
  const before = await strip.evaluate((el) => el.scrollLeft);
  await later.click();
  await expect.poll(() => strip.evaluate((el) => el.scrollLeft)).toBeGreaterThan(before);
  await expect(earlier).toBeVisible();
});

test('the station on the dial can be saved from the strip, the radio picking the slot', async ({ page }) => {
  await withState(page, (s) => Object.assign(s.tun, { khz: 107900, f: '107.90', pst: 0, bws: 0 }));
  await page.goto('/');
  const save = page.getByRole('button', { name: 'Save this station as a preset' });
  const sent = nextPost(page, '/api/presets');
  await save.click();
  expect(await sent).toBe('do=add&khz=107900&bnd=FM&bw=0&name=MAGIC');
});

test('the radio text has two lines kept, with or without text', async ({ page }) => {
  await withState(page, (s) => (s.tun.rds.rt = 'NOW PLAYING A SONG'));
  await page.goto('/');
  await expect(page.getByText('NOW PLAYING A SONG')).toBeVisible();
});

test('a tap on the frequency types one; Enter tunes there', async ({ page }) => {
  await page.goto('/');
  await page.getByTitle('Tap to type a frequency').click();
  const box = page.getByLabel('Frequency to tune');
  await expect(box).toBeFocused();
  const sent = nextPost(page, '/api/tune');
  await box.fill('98.3');
  await box.press('Enter');
  expect(await sent).toBe('khz=98300');
});

test('log this station, then the last heard read again; a last heard row tunes', async ({ page }) => {
  await page.goto('/');
  let sent = nextPost(page, '/api/log');
  const reread = page.waitForRequest((r) => r.url().includes('/api/log?last=3'));
  await page.getByRole('button', { name: 'Log this station' }).click();
  await sent;
  await reread;
  sent = nextPost(page, '/api/tune');
  await page.locator('section').filter({ hasText: 'Last heard' }).getByRole('button', { name: /738/ }).click();
  expect(await sent).toBe('khz=738');
});

test('DX and scanning chips show while they run', async ({ page }) => {
  await withState(page, (s) => (s.dx = { ...(s.dx ?? {}), on: true, scn: 'running' }));
  await page.goto('/');
  const player = page.locator('section').filter({ hasText: 'Signal to noise' });
  await expect(player.getByRole('link', { name: 'DX', exact: true })).toHaveAttribute('href', '#dx');
  await expect(page.getByText('Scanning', { exact: true })).toBeVisible();
});

test('keys on a laptop: arrows step, Shift seeks, M mutes, a preset\'s number recalls it', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await page.goto('/');
  await expect(page.getByText('Keys:')).toBeVisible();
  for (const [press, path, body] of [
    ['ArrowRight', '/api/step', 'stp=1'],
    ['Shift+ArrowLeft', '/api/seek', 'dir=down'],
    ['m', '/api/mute', 'on=1'],
  ]) {
    const sent = nextPost(page, path);
    await page.keyboard.press(press);
    expect(await sent).toBe(body);
  }
  // One digit, then a pause: that preset.
  let sent = nextPost(page, '/api/presets');
  await page.keyboard.press('2');
  await expect(page.getByText('Preset 2', { exact: true })).toBeVisible();
  expect(await sent).toBe('do=recall&slot=2');
  // Two digits quickly: preset 12; Enter does not wait.
  sent = nextPost(page, '/api/presets');
  await page.keyboard.press('1');
  await page.keyboard.press('2');
  await page.keyboard.press('Enter');
  expect(await sent).toBe('do=recall&slot=12');
});

test('the keys do nothing while a frequency is typed', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await page.clock.install();
  await page.goto('/');
  await expect(page.getByText('Keys:')).toBeVisible();
  const posts = [];
  page.on('request', (r) => r.method() === 'POST' && posts.push(`${new URL(r.url()).pathname} ${r.postData()}`));
  await page.getByTitle('Tap to type a frequency').click();
  const box = page.getByLabel('Frequency to tune');
  // Real key presses into the box, as fill sends none.
  await box.pressSequentially('98.3');
  for (const key of ['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown']) await box.press(key);
  await expect(page.getByText(/^Preset \d+$/)).toHaveCount(0);
  // Past the waits for a step, the volume and a preset's digits. Then Enter
  // tunes, which goes to the radio after anything sent before it.
  await page.clock.runFor(1500);
  await box.press('Enter');
  await expect.poll(() => posts).toEqual(['/api/tune khz=98300']);
});

test('arrow keys up and down: the volume moves at once, the radio is sent it once', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await withState(page, (s) => (s.tun.vol = -20));
  await page.goto('/');
  await expect(page.getByText('-20 dB')).toBeVisible();
  const posts = [];
  page.on('request', (r) => r.method() === 'POST' && new URL(r.url()).pathname === '/api/volume' && posts.push(r.postData()));
  for (let i = 0; i < 3; i++) await page.keyboard.press('ArrowUp');
  await expect(page.getByText('-17 dB')).toBeVisible();
  await expect.poll(() => posts).toEqual(['db=-17']);
});

test('the strip moves to the preset the dial goes to', async ({ page }) => {
  await withState(page, (s) => (s.tun.pst = 1));
  await page.goto('/');
  const strip = keyStrip(page);
  await expect(presetCard(page).getByRole('button', { pressed: true })).toContainText('91.10');
  await presetKeys(page).filter({ hasText: '106.40' }).last().click();
  const inView = () =>
    strip.evaluate((el) => {
      const on = el.querySelector('button[aria-pressed="true"]');
      const a = el.getBoundingClientRect();
      const b = on.getBoundingClientRect();
      return b.left >= a.left - 1 && b.right <= a.right + 1;
    });
  await expect.poll(inView).toBe(true);
});

test('behind the player, with no sweep of this band, the station alone, its peak as tall as the signal', async ({ page }) => {
  await page.route('**/api/scope', (route) => route.fulfill({ json: { rev: 0, running: false } }));
  const radio = await liveRadio(page, (s) => (s.tun.sig = 100));
  await page.goto('/');
  const svg = page.locator('main section').first().locator('svg[preserveAspectRatio="none"]');
  // The top of the tallest bar.
  const top = () => svg.evaluate((e) => Math.min(...[...e.querySelectorAll('rect')].map((r) => +r.getAttribute('y'))));
  await expect.poll(top).toBeGreaterThan(0);
  const low = await top();
  radio.push((s) => (s.tun.sig = 600));
  await expect.poll(top).toBeLessThan(low - 50);
  // 201 bars, and no dial line.
  await expect(svg.locator('rect')).toHaveCount(201);
  await expect(svg.locator('line')).toHaveCount(0);
});

test('behind the player, the last whole sweep of this band, with the dial marked', async ({ page }) => {
  const sweep = JSON.parse(fs.readFileSync(new URL('../mock/scope.json', import.meta.url)));
  await page.route('**/api/scope', (route) => route.fulfill({ json: sweep }));
  await page.goto('/');
  const svg = page.locator('main section').first().locator('svg[preserveAspectRatio="none"]');
  // A bar a channel of the sweep.
  await expect(svg.locator('rect')).toHaveCount(sweep.count);
  await expect(svg.locator('line')).toHaveCount(1);
});

test('a long preset name is cut to one line, its key left aligned as the others', async ({ page }) => {
  await page.route('**/api/presets.csv', (route) =>
    route.fulfill({ body: 'slot,band,frequency,bandwidth,name,pi\n1,FM,102800,0,VIVIDH BHARATI LONG NAME,\n2,FM,104000,0,KOOL FM,\n' }),
  );
  await page.goto('/');
  const keys = page.locator('main button[aria-pressed]');
  await expect(keys).toHaveCount(2);
  const [a, b] = [await keys.nth(0).boundingBox(), await keys.nth(1).boundingBox()];
  expect(a.height).toBe(b.height);
  const name = keys.nth(0).locator('span').last();
  expect(await name.evaluate((e) => getComputedStyle(e).whiteSpace)).toBe('nowrap');
  expect(await keys.nth(0).evaluate((e) => getComputedStyle(e).textAlign)).toBe('left');
  await expect(keys.nth(0)).toHaveAttribute('title', 'VIVIDH BHARATI LONG NAME');
});

test('a phone draws no picture behind a card; a laptop does', async ({ page }, testInfo) => {
  await page.goto('/');
  const back = page.locator('main section .back').first();
  await expect(back).toBeAttached();
  expect(await back.evaluate((e) => getComputedStyle(e).display)).toBe(testInfo.project.name === 'phone' ? 'none' : 'block');
});

test('the band picture is drawn at 8 px a channel, wider than the card, and pans slowly', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name === 'phone', 'A phone draws no picture behind a card.');
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  const sweep = JSON.parse(fs.readFileSync(new URL('../mock/scope.json', import.meta.url)));
  await page.route('**/api/scope', (route) => route.fulfill({ json: sweep }));
  await page.goto('/');
  const card = page.locator('main section').first();
  const svg = card.locator('svg[preserveAspectRatio="none"]');
  // Rounded: a box moving by a part of a pixel gives a width a hair off.
  await expect.poll(async () => Math.round((await svg.boundingBox())?.width ?? 0)).toBe(Math.max(1600, sweep.count * 8));
  expect((await svg.boundingBox()).width).toBeGreaterThan((await card.boundingBox()).width);
  expect(await svg.evaluate((e) => getComputedStyle(e).animationName)).toBe('pan');
  const x0 = (await svg.boundingBox()).x;
  await page.waitForTimeout(1500);
  expect((await svg.boundingBox()).x).toBeLessThan(x0 - 5);
});

test('with the motion turned down, the band picture stays still', async ({ page }) => {
  await page.goto('/');
  const svg = page.locator('main section').first().locator('svg[preserveAspectRatio="none"]');
  await expect(svg).toHaveCount(1);
  expect(await svg.evaluate((e) => getComputedStyle(e).animationName)).toBe('none');
});

test('the preset arrows lie over the strip, which takes the whole width inside the card padding', async ({ page }) => {
  await withState(page, (s) => (s.tun.pst = 5));
  await page.goto('/');
  const card = await page.locator('section').filter({ hasText: 'Presets' }).boundingBox();
  const box = await keyStrip(page).boundingBox();
  expect(Math.round(box.x - card.x)).toBe(17);
  expect(Math.round(card.x + card.width - (box.x + box.width))).toBe(17);
  const later = await page.getByRole('button', { name: 'Later presets' }).boundingBox();
  expect(later.x + later.width).toBeLessThanOrEqual(box.x + box.width + 0.5);
  expect(later.x).toBeGreaterThan(box.x);
});

// The first read of a route answered 503, the rest as before.
async function failOnce(page, url) {
  let reads = 0;
  await page.route(url, (route) => (++reads === 1 ? route.fulfill({ status: 503, body: '' }) : route.fallback()));
  return () => reads;
}

// These run on the page's polling, as the dev server turns the live socket
// away, with the clock moved on to the next state 5 s on.
test('presets that could not be read are said, and read again by themselves, 5 s on', async ({ page }) => {
  await page.clock.install();
  await failOnce(page, '**/api/presets.csv');
  await page.goto('/');
  await expect(presetCard(page)).toContainText('The presets could not be read. Trying again.');
  await page.clock.runFor(5000);
  await expect(presetKeys(page)).toHaveCount(16);
});

test('last heard that could not be read says so, and is read again by itself, 5 s on', async ({ page }) => {
  await page.clock.install();
  await failOnce(page, '**/api/log?last=3');
  await page.goto('/');
  const card = page.locator('section').filter({ hasText: 'Last heard' });
  await expect(card).toContainText('The log could not be read. Trying again.');
  await page.clock.runFor(5000);
  await expect(card.getByRole('button')).toHaveCount(3);
  await expect(card.getByRole('button')).toContainText(['2500', '738', '102.20']);
});

test('a sweep behind the player that could not be read is read again by itself, 5 s on', async ({ page }) => {
  await page.clock.install();
  const sweep = JSON.parse(fs.readFileSync(new URL('../mock/scope.json', import.meta.url)));
  await page.route('**/api/scope', (route) => route.fulfill({ json: sweep }));
  const reads = await failOnce(page, '**/api/scope');
  await page.goto('/');
  const svg = page.locator('main section').first().locator('svg[preserveAspectRatio="none"]');
  // The station alone first, then the sweep.
  await expect(svg.locator('rect')).toHaveCount(201);
  await page.clock.runFor(5000);
  await expect(svg.locator('rect')).toHaveCount(sweep.count);
  expect(reads()).toBe(2);
});

test('a tune from the frequency box or from Last heard is drawn at once, on the band it lands on', async ({ page }) => {
  // The radio answers neither until the end, so each is seen before it does.
  const answer = await holdAnswers(page, '**/api/tune');
  await page.goto('/');
  await page.locator('main').getByTitle('Tap to type a frequency').click();
  await page.getByLabel('Frequency to tune').fill('98.3');
  await page.getByLabel('Frequency to tune').press('Enter');
  // Preset 6 holds 98.30, so the radio will say so too.
  const head = page.getByRole('link', { name: /98\.30/ });
  await expect(head).toBeVisible();
  await expect(head).toContainText('P06');
  await page.locator('section').filter({ hasText: 'Last heard' }).getByRole('button', { name: /738/ }).click();
  await expect(page.getByRole('link', { name: /738/ })).toContainText('kHz');
  await expect(page.locator('main section').first().getByText('MW', { exact: true })).toBeVisible();
  answer();
});

test('a held arrow adds its steps up and sends them once; a held seek seeks once', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await page.clock.install();
  await page.goto('/');
  await expect(page.getByText('Keys:')).toBeVisible();
  const posts = [];
  page.on('request', (r) => r.method() === 'POST' && posts.push(`${new URL(r.url()).pathname} ${r.postData()}`));
  // A key held down: every keydown after the first is a repeat.
  for (let i = 0; i < 5; i++) await page.keyboard.down('ArrowRight');
  await page.keyboard.up('ArrowRight');
  await expect.poll(() => posts).toEqual(['/api/step stp=5']);
  await page.keyboard.down('Shift');
  for (let i = 0; i < 4; i++) await page.keyboard.down('ArrowLeft');
  await page.keyboard.up('ArrowLeft');
  await page.keyboard.up('Shift');
  // Past the 400 ms a step waits to be sent; then M, whose mute goes to the
  // radio after anything sent before it.
  await page.clock.runFor(1000);
  await page.keyboard.press('m');
  await expect.poll(() => posts).toEqual(['/api/step stp=5', '/api/seek dir=down', '/api/mute on=1']);
});

test('the volume keys: leaving the screen sends the volume at once, and the radio\'s own shows again after', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await page.clock.install();
  const radio = await liveRadio(page, (s) => {
    station(s);
    s.tun.vol = -20;
  });
  await page.goto('/');
  await expect(page.getByText('-20 dB')).toBeVisible();
  const sent = nextPost(page, '/api/volume');
  const landed = page.waitForEvent('requestfinished', (r) => r.url().endsWith('/api/volume'));
  await page.keyboard.press('ArrowUp');
  await page.evaluate(() => (location.hash = 'radio'));
  expect(await sent).toBe('db=-19');
  const slider = page.locator('header').getByRole('slider', { name: 'Volume' });
  await expect(slider).toHaveValue('-19');
  // The radio took it; a state soon after may still be from before, so the
  // page keeps -19 drawn for 300 ms.
  await landed;
  await page.evaluate(() => {});
  await page.clock.runFor(400);
  // Turned on the radio's own knob: the header follows it.
  radio.push((s) => (s.tun.vol = -30));
  await expect(slider).toHaveValue('-30');
});

test('an older volume send does not let go of the volume still being moved', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await withState(page, (s) => (s.tun.vol = -20));
  let answer;
  await page.route('**/api/volume', (route) => (answer = () => route.fulfill({ body: 'ok' })));
  await page.goto('/');
  await expect(page.getByText('-20 dB')).toBeVisible();
  const slider = page.locator('header').getByRole('slider', { name: 'Volume' });
  const asked = page.waitForRequest((r) => new URL(r.url()).pathname === '/api/volume');
  await page.keyboard.press('ArrowUp');
  await asked;
  // The radio has -19 to take, and the volume moves on before it answers.
  await page.keyboard.press('ArrowUp');
  await expect(slider).toHaveValue('-18');
  const answered = page.waitForEvent('requestfinished', (r) => new URL(r.url()).pathname === '/api/volume');
  await answer();
  await answered;
  await page.evaluate(() => {});
  expect(await slider.inputValue()).toBe('-18');
});
