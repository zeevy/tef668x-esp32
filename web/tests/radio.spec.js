// The Radio screen: the reading, the scale, tuning, sound, the signal and RDS.
import fs from 'node:fs';
import { expect, test } from '@playwright/test';
import { holdAnswers, liveRadio, nextPost } from './radio.js';
import { typedKhz } from '../src/data/format.js';

const FM = (s) => {
  Object.assign(s.tun, { bnd: 'FM', khz: 106400, f: '106.40', unt: 'MHz', stp: 100, bws: 168, tmd: 'Auto', sql: 'Off', sqo: true, vol: -20, mut: false });
  Object.assign(s.tun.rds, { syn: true, ps: ' MAGIC  ', pi: '1064', pty: 12, ptn: 'Easy Listening', tp: false, ta: false, rt: 'THE RADIO TEXT', ms: 'speech' });
};

const reading = (page) => page.locator('main section').filter({ hasText: 'MHz' }).first();

// The box that scrolls the scale sideways, found by what it does, and marked
// so the test can come back to it.
async function scaleRow(page) {
  await page.getByRole('slider', { name: 'Band scale' }).evaluate((e) => {
    while (getComputedStyle(e).overflowX !== 'auto') e = e.parentElement;
    e.dataset.scaleRow = '';
  });
  return page.locator('[data-scale-row]');
}

test('the reading: frequency, name, programme, radio text', async ({ page }) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  await expect(reading(page)).toContainText('106.40');
  await expect(reading(page)).toContainText('MAGIC');
  await expect(reading(page)).toContainText('Easy Listening · PI 1064');
  await expect(reading(page)).toContainText('THE RADIO TEXT');
});

test('nothing moves when RDS and its radio text come or go', async ({ page }) => {
  const radio = await liveRadio(page, FM);
  await page.goto('/#radio');
  await expect(reading(page)).toContainText('THE RADIO TEXT');
  const before = await reading(page).boundingBox();
  // No preset either, so the name falls back to ---.
  radio.push((s) => Object.assign(s.tun, { rds: {}, pst: 0 }));
  await expect(reading(page)).not.toContainText('THE RADIO TEXT');
  await expect(reading(page)).toContainText('---');
  expect((await reading(page).boundingBox()).height).toBe(before.height);
});

test('a typed frequency is read as the radio would', () => {
  const kHz = ['98.3', '98', '98,3', '98300', '738', '153', '9.65', '9650', '65.81', '108', '144', '', 'abc', '-5'].map(typedKhz);
  expect(kHz).toEqual([98300, 98000, 98300, 98300, 738, 153, 9650, 9650, 65810, 108000, 144, null, null, null]);
});

test('a tap on the frequency opens it for typing, as on Home; Enter tunes', async ({ page }) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  await expect(page.getByLabel('Frequency', { exact: true })).toHaveCount(0);
  await reading(page).getByTitle('Tap to type a frequency').click();
  const box = page.getByLabel('Frequency to tune');
  await expect(box).toBeFocused();
  await box.fill('98.3');
  const sent = nextPost(page, '/api/tune');
  await box.press('Enter');
  expect(await sent).toBe('khz=98300');
  await expect(box).toHaveCount(0);
});

test('the step and seek buttons send what the radio expects', async ({ page }) => {
  await page.goto('/#radio');
  for (const [label, path, body] of [
    ['Seek down', '/api/seek', 'dir=down'],
    ['Step down', '/api/step', 'stp=-1'],
    ['Step up', '/api/step', 'stp=1'],
    ['Seek up', '/api/seek', 'dir=up'],
  ]) {
    const sent = nextPost(page, path);
    await page.getByRole('button', { name: label, exact: true }).click();
    expect(await sent).toBe(body);
  }
});

test('band, tuning mode and squelch send their values and show the new one at once', async ({ page }) => {
  await liveRadio(page, FM);
  const modeTaken = await holdAnswers(page, '**/api/mode', { body: 'mode Meter band' });
  const squelchTaken = await holdAnswers(page, '**/api/squelch', { body: 'squelch Manual' });
  await page.goto('/#radio');
  let sent = nextPost(page, '/api/band');
  await page.getByRole('radiogroup', { name: 'Band' }).getByRole('radio', { name: 'MW' }).click();
  expect(await sent).toBe('bnd=MW');
  sent = nextPost(page, '/api/mode');
  const mode = page.getByLabel('Tuning mode');
  await expect(mode.locator('option')).toHaveText(['Tuning manual', 'Tuning auto', 'Tuning presets', 'Tuning meter band']);
  await mode.selectOption('Meter band');
  expect(await sent).toBe('mod=MeterBand');
  // Before the radio answers.
  await expect(mode).toHaveValue('Meter band');
  modeTaken();
  sent = nextPost(page, '/api/squelch');
  const squelch = page.getByLabel('Squelch', { exact: true });
  await expect(squelch.locator('option')).toHaveText(['Squelch off', 'Squelch auto', 'Squelch manual']);
  await squelch.selectOption('Manual');
  expect(await sent).toBe('mod=manual');
  await expect(squelch).toHaveValue('Manual');
  squelchTaken();
});

test('step and bandwidth offer what the band has, and send the pick', async ({ page }) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  const step = page.getByLabel('Step', { exact: true });
  const width = page.getByLabel('Bandwidth');
  await expect(step.locator('option')).toHaveText(['Step 50 kHz', 'Step 100 kHz', 'Step 200 kHz']);
  await expect(step).toHaveValue('100');
  await expect(width.locator('option')).toHaveCount(17);
  await expect(width.locator('option').first()).toHaveText('Filter auto');
  await expect(width).toHaveValue('168');
  let sent = nextPost(page, '/api/step-size');
  await step.selectOption('200');
  expect(await sent).toBe('khz=200');
  sent = nextPost(page, '/api/bandwidth');
  await width.selectOption('0');
  expect(await sent).toBe('khz=0');
});

test('on MW the widths are the four AM ones', async ({ page }) => {
  await liveRadio(page, (s) => Object.assign(s.tun, { bnd: 'MW', khz: 738, f: '738', unt: 'kHz', stp: 9, bws: 4 }));
  await page.goto('/#radio');
  await expect(page.getByLabel('Bandwidth').locator('option')).toHaveText(['Filter 3 kHz', 'Filter 4 kHz', 'Filter 6 kHz', 'Filter 8 kHz']);
  await expect(page.getByLabel('Step', { exact: true }).locator('option')).toHaveText(['Step 1 kHz', 'Step 9 kHz']);
});

test('the scale: a mark every 100 kHz on FM, the dial in the middle; a tap tunes to the nearest step there', async ({ page }) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  const scale = page.getByRole('slider', { name: 'Band scale' });
  // Whole megahertz, 87 to 108, 80 px apart: ten marks of 8 px.
  await expect(scale.locator('span')).toHaveCount(22);
  const fm = JSON.parse(fs.readFileSync(new URL('../mock/bands.json', import.meta.url))).find((b) => b.band === 'FM');
  // 16 px at each end for the end numbers.
  expect((await scale.boundingBox()).width).toBe(((fm.high - fm.low) / 100) * 8 + 1 + 32);
  const row = await scaleRow(page);
  // The dial in the middle, or as near as the band's end allows.
  const middle = (khz) => row.evaluate((e, k) => Math.round(Math.min(e.scrollWidth - e.clientWidth, Math.max(0, 16 + ((k - 87000) / 100) * 8 - e.clientWidth / 2))), khz);
  const at = () => row.evaluate((e) => Math.round(e.scrollLeft));
  await expect.poll(at).toBe(await middle(106400));
  // 1 MHz right of the dial is 107.40.
  const dial = await scale.locator('[data-mark=dial]').boundingBox();
  const sent = nextPost(page, '/api/tune');
  await page.mouse.click(dial.x + dial.width / 2 + 80, dial.y + 4);
  const khz = Number(new URLSearchParams(await sent).get('khz'));
  expect(Math.abs(khz - 107400)).toBeLessThanOrEqual(100);
  expect(khz % 100).toBe(0);
  // At the band's top the scale stops with the last mark at the edge.
  expect(await at()).toBe(await row.evaluate((e) => e.scrollWidth - e.clientWidth));
});

test('a tune brings the dial back to the middle of the scale', async ({ page }) => {
  const radio = await liveRadio(page, FM);
  await page.goto('/#radio');
  await expect(reading(page)).toContainText('106.40');
  const row = await scaleRow(page);
  await row.evaluate((e) => (e.scrollLeft = 0));
  radio.push((s) => Object.assign(s.tun, { khz: 95000, f: '95.00' }));
  await expect(reading(page)).toContainText('95.00');
  const want = await row.evaluate((e) => Math.round(16 + ((95000 - 87000) / 100) * 8 - e.clientWidth / 2));
  await expect.poll(() => row.evaluate((e) => Math.round(e.scrollLeft))).toBe(want);
});

test('on MW a mark every 10 kHz, numbered every 100', async ({ page }) => {
  await liveRadio(page, (s) => Object.assign(s.tun, { bnd: 'MW', khz: 738, f: '738', unt: 'kHz', stp: 9, bws: 4 }));
  await page.goto('/#radio');
  const scale = page.getByRole('slider', { name: 'Band scale' });
  await expect(scale.locator('span')).toHaveText(['600', '700', '800', '900', '1000', '1100', '1200', '1300', '1400', '1500', '1600', '1700']);
  expect((await scale.boundingBox()).width).toBeCloseTo(((1791 - 522) / 10) * 8 + 1 + 32, 0);
});

test('volume and mute are in the header, before the frequency; on a phone the speaker opens them', async ({ page }, testInfo) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  if (testInfo.project.name === 'phone') {
    await expect(page.getByRole('slider', { name: 'Volume' })).toHaveCount(0);
    await page.getByRole('button', { name: 'Volume' }).click();
  } else {
    const [vol, dial] = [await page.locator('header').getByRole('slider', { name: 'Volume' }).boundingBox(), await page.getByRole('link', { name: /106\.40/ }).boundingBox()];
    expect(vol.x + vol.width).toBeLessThan(dial.x);
  }
  // The Radio screen has no volume of its own now.
  await expect(page.locator('main').getByRole('slider', { name: 'Volume' })).toHaveCount(0);
  const slider = page.getByRole('slider', { name: 'Volume' });
  await expect(slider).toHaveValue('-20');
  let sent = nextPost(page, '/api/volume');
  await slider.fill('-30');
  expect(await sent).toBe('db=-30');
  sent = nextPost(page, '/api/mute');
  await page.getByRole('button', { name: 'Mute' }).click();
  expect(await sent).toBe('on=1');
});

test('the signal and RDS figures, and --- for what RDS has not sent', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    Object.assign(s.tun, { sig: 348, snr: 25, usn: 12, wam: 50, off: 66, bw: 168, mod: 19, plt: true });
    Object.assign(s.tun.rds, { grp: 200, use: 150, cor: 10, bad: 4 });
  });
  await page.goto('/#radio');
  const signal = page.locator('main section').filter({ hasText: 'Signal to noise' });
  for (const text of ['34.8 dBµV', '25 dB', '6.6 kHz', '168 kHz', '19 %', 'Pilot']) await expect(signal).toContainText(text);
  const rds = page.locator('main section').filter({ hasText: 'Groups seen' });
  const row = (label) => rds.locator('dl > div').filter({ has: page.getByText(label, { exact: true }) }).locator('dd');
  await expect(row('PI')).toHaveText('1064');
  await expect(row('Programme type')).toHaveText('Easy Listening');
  await expect(row('Music or speech')).toHaveText('Speech');
  await expect(row('Traffic')).toHaveText('No traffic');
  await expect(row('Groups seen')).toHaveText('200');
  // What the station has not sent is ---, so the card keeps its size.
  for (const label of ['Country', 'Language', 'Station clock']) await expect(row(label)).toHaveText('---');
  await expect(rds).not.toContainText('Alternative frequencies');
  // Of the 800 blocks, 10 fixed and 4 lost; 150 of the 200 groups used.
  await expect(rds.getByRole('img', { name: '98 % clean, 1 % fixed, 1 % lost' })).toBeVisible();
  // A share's figure: its name, and the value under it.
  const share = (label) => rds.getByText(label, { exact: true }).locator('..');
  await expect(share('Clean')).toContainText('98 %');
  await expect(share('Used')).toContainText('75 %');
});

test('squelch shut and a traffic announcement show as chips', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    Object.assign(s.tun, { sql: 'Auto', sqo: false });
    s.tun.rds.ta = true;
  });
  await page.goto('/#radio');
  await expect(reading(page).getByText('Squelch shut', { exact: true })).toBeVisible();
  await expect(reading(page).getByText('TA', { exact: true })).toBeVisible();
});

test('a failed read of the bands is tried again with the next state, 5 s on', async ({ page }) => {
  await page.clock.install();
  const radio = await liveRadio(page, FM);
  let tries = 0;
  await page.route('**/api/bands', (route) => (++tries === 1 ? route.fulfill({ status: 503, body: 'busy' }) : route.fallback()));
  await page.goto('/#radio');
  await expect.poll(() => tries).toBe(1);
  // The first state that comes 5 s on.
  await page.clock.runFor(5000);
  radio.push();
  await expect(page.getByLabel('Step', { exact: true }).locator('option')).toHaveCount(3);
  expect(tries).toBe(2);
});

test('one card across the page: the reading, and its controls beside it on a laptop or under it on a phone', async ({ page }, testInfo) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  const card = reading(page);
  // The scale scrolls to the dial once the state has come, so its end is
  // inside the card; measure after that.
  await expect
    .poll(async () => {
      const [s, c] = [await page.getByRole('slider', { name: 'Band scale' }).boundingBox(), await card.boundingBox()];
      return s.x + s.width <= c.x + c.width;
    })
    .toBe(true);
  const scale = await page.getByRole('slider', { name: 'Band scale' }).boundingBox();
  const band = await card.getByRole('radiogroup', { name: 'Band' }).boundingBox();
  const lamps = await card.getByRole('switch', { name: 'EQ' }).boundingBox();
  const seek = await card.getByRole('button', { name: 'Seek up' }).boundingBox();
  const box = await card.boundingBox();
  if (testInfo.project.name === 'laptop') {
    expect(box.width).toBeGreaterThan(900);
    expect(band.x).toBeGreaterThan(scale.x + scale.width);
    // The rows spread to the reading's height: seek and step, the last,
    // end level with the scale, and nothing is left under either.
    const step = await card.getByLabel('Step', { exact: true }).boundingBox();
    expect(step.y).toBeGreaterThan(lamps.y + lamps.height);
    expect(seek.y).toBeGreaterThan(step.y + step.height);
    expect(Math.abs(seek.y + seek.height - (scale.y + scale.height))).toBeLessThanOrEqual(2);
    expect(box.y + box.height - (scale.y + scale.height)).toBeLessThanOrEqual(17);
    // Every row starts on the same line, with no labels and no clock.
    const xs = [await card.getByRole('radiogroup', { name: 'Band' }).boundingBox(), await card.getByLabel('Tuning mode').boundingBox()];
    expect(xs[1].x).toBe(xs[0].x);
    // Tuning mode and squelch share a row.
    expect((await card.getByLabel('Squelch', { exact: true }).boundingBox()).y).toBe(xs[1].y);
    expect((await card.getByRole('button', { name: 'Seek down' }).boundingBox()).x).toBe(xs[0].x);
    expect(lamps.x + lamps.width).toBe(seek.x + seek.width);
    await expect(card).not.toContainText('FM sound');
  } else {
    expect(band.y).toBeGreaterThan(scale.y + scale.height);
    // Nothing wider than the phone.
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBe(390);
  }
});

test("with no station name, the reading shows the preset's", async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    s.tun.pst = 9;
    delete s.tun.rds.ps;
  });
  await page.goto('/#radio');
  await expect(reading(page)).toContainText('MAGIC FM');
});

test('a dot on the scale for each preset on this band; a tap recalls it', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    s.tun.pst = 9;
  });
  const answer = await holdAnswers(page, '**/api/presets', { body: 'Recalled preset 5.' });
  await page.goto('/#radio');
  const dots = reading(page).getByRole('button', { name: /^Preset / });
  await expect(dots).toHaveCount(9);
  await expect(reading(page).getByRole('button', { name: 'Preset 9, MAGIC FM, 106.40 MHz' })).toHaveAttribute('aria-pressed', 'true');
  const sent = nextPost(page, '/api/presets');
  await reading(page).getByRole('button', { name: 'Preset 5, MIRCHI, 98.30 MHz' }).click();
  expect(await sent).toBe('do=recall&slot=5');
  // Before the radio answers.
  await expect(reading(page)).toContainText('98.30');
  answer();
});

test('no preset dots on a band with no presets', async ({ page }) => {
  await liveRadio(page, (s) => Object.assign(s.tun, { bnd: 'MW', khz: 738, f: '738', unt: 'kHz', stp: 9, bws: 4 }));
  await page.goto('/#radio');
  await expect(page.getByRole('slider', { name: 'Band scale' })).toBeVisible();
  await expect(reading(page).getByRole('button', { name: /^Preset / })).toHaveCount(0);
});

test('the keys work here as on Home', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await liveRadio(page, FM);
  await page.goto('/#radio');
  await expect(reading(page)).toContainText('106.40');
  let sent = nextPost(page, '/api/step');
  await page.keyboard.press('ArrowRight');
  expect(await sent).toBe('stp=1');
  sent = nextPost(page, '/api/volume');
  await page.keyboard.press('ArrowUp');
  await expect(page.getByRole('slider', { name: 'Volume' })).toHaveValue('-19');
  expect(await sent).toBe('db=-19');
  sent = nextPost(page, '/api/presets');
  await page.keyboard.press('5');
  await expect(page.getByText('Preset 5', { exact: true })).toBeVisible();
  await page.keyboard.press('Enter');
  expect(await sent).toBe('do=recall&slot=5');
  await expect(page.getByText('Keys: ← and → step')).toBeVisible();
});

test('the FM sound switches show the radio and send a flip at once', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    Object.assign(s.tun, { mno: false, ims: true, eq: false });
  });
  const answer = await holdAnswers(page, '**/api/fm');
  await page.goto('/#radio');
  await expect(page.getByRole('switch', { name: 'Mono' })).toHaveAttribute('aria-checked', 'false');
  await expect(page.getByRole('switch', { name: 'iMS' })).toHaveAttribute('aria-checked', 'true');
  await expect(page.getByRole('switch', { name: 'EQ' })).toHaveAttribute('aria-checked', 'false');
  let sent = nextPost(page, '/api/fm');
  await page.getByRole('switch', { name: 'Mono' }).click();
  expect(await sent).toBe('mno=1');
  // Before the radio answers.
  await expect(page.getByRole('switch', { name: 'Mono' })).toHaveAttribute('aria-checked', 'true');
  answer();
  sent = nextPost(page, '/api/fm');
  await page.getByRole('switch', { name: 'iMS' }).click();
  expect(await sent).toBe('ims=0');
});

test('the FM sound switches are not there on MW', async ({ page }) => {
  await liveRadio(page, (s) => Object.assign(s.tun, { bnd: 'MW', khz: 738, f: '738', unt: 'kHz', stp: 9, bws: 4 }));
  await page.goto('/#radio');
  await expect(page.locator('main section').first()).toContainText('738');
  await expect(page.getByRole('switch')).toHaveCount(0);
});

test('the squelch chip on the reading: open or shut, and in Manual the level it opens at', async ({ page }) => {
  const radio = await liveRadio(page, (s) => {
    FM(s);
    Object.assign(s.tun, { sql: 'Manual', sqo: false, sqa: 250, sig: 180 });
  });
  await page.goto('/#radio');
  await expect(reading(page).getByText('Squelch shut · 25.0 dBµV', { exact: true })).toBeVisible();
  await expect(page.getByLabel('Squelch', { exact: true })).toHaveValue('Manual');
  radio.push(FM);
  await expect(page.getByLabel('Squelch', { exact: true })).toHaveValue('Off');
  await expect(reading(page).getByText(/^Squelch (open|shut)/)).toHaveCount(0);
});

test('RDS: blocks A to D, country, language, the clock, and the other frequencies, which tune', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    Object.assign(s.tun.rds, { blk: [0, 1, 3, 0], cty: 'IN', lgn: 'Telugu', lng: 41, ct: '2026-10-11T14:05+05:30', af: [106400, 104000, 91100] });
  });
  await page.goto('/#radio');
  const rds = page.locator('main section').filter({ hasText: 'Groups seen' });
  await expect(rds.getByRole('img', { name: 'RDS blocks: A 4 of 4, B 3 of 4, C 1 of 4, D 4 of 4' })).toBeVisible();
  for (const text of ['India', 'Telugu', '14:05, 11 Oct']) await expect(rds).toContainText(text);
  const afs = rds.getByRole('button');
  await expect(afs).toHaveText(['106.40', '104.00', '91.10']);
  await expect(afs.first()).toHaveAttribute('aria-pressed', 'true');
  const sent = nextPost(page, '/api/tune');
  await rds.getByRole('button', { name: '104.00' }).click();
  expect(await sent).toBe('khz=104000');
});

test('RDS and the signal sit side by side on a laptop, the same height', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'one column on a phone');
  await liveRadio(page, FM);
  await page.goto('/#radio');
  const rds = await page.locator('main section').filter({ hasText: 'Groups seen' }).boundingBox();
  const signal = await page.locator('main section').filter({ hasText: 'Signal to noise' }).boundingBox();
  expect(signal.y).toBe(rds.y);
  expect(signal.height).toBe(rds.height);
  expect(rds.x).toBeLessThan(signal.x);
});

test('no band picture behind the radio card here; Home has it', async ({ page }) => {
  await liveRadio(page, FM);
  await page.goto('/#radio');
  await expect(reading(page)).toContainText('106.40');
  await expect(reading(page).locator('svg[preserveAspectRatio="none"]')).toHaveCount(0);
});

test('the preset the dial is on shows over the unit, as the radio writes it', async ({ page }) => {
  const radio = await liveRadio(page, (s) => {
    FM(s);
    s.tun.pst = 9;
  });
  await page.goto('/#radio');
  const tag = reading(page).getByText('P09', { exact: true });
  await expect(tag).toBeVisible();
  // The unit holds the tag, which sits over it.
  await expect(tag.locator('..')).toContainText('MHz');
  const unit = await tag.locator('..').boundingBox();
  const box = await tag.boundingBox();
  expect(box.y + box.height).toBeLessThanOrEqual(unit.y);
  expect(Math.abs(box.x - unit.x)).toBeLessThanOrEqual(1);
  radio.push((s) => (s.tun.pst = 0));
  await expect(tag).toHaveCount(0);
});

test('the preset number shows over the unit everywhere the dial is shown: header, Home, Presets and DX', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    s.tun.pst = 9;
  });
  for (const hash of ['/', '/#presets', '/#dx']) {
    await page.goto(hash);
    const tag = page.locator('main').getByText('P09', { exact: true }).first();
    await expect(tag).toBeVisible();
    // Over the unit, not beside it.
    const unit = await tag.locator('..').boundingBox();
    expect((await tag.boundingBox()).y).toBeLessThan(unit.y);
  }
  // The header's box has the number over the unit too.
  const head = page.getByRole('link', { name: /106\.40/ });
  await expect(head.getByText('P09', { exact: true })).toBeVisible();
  const [p, mhz] = [await head.getByText('P09', { exact: true }).boundingBox(), await head.getByText('MHz', { exact: true }).boundingBox()];
  expect(p.y + p.height).toBeLessThanOrEqual(mhz.y + 1);
});

test('the frequency is the same everywhere: a tap opens it for typing on Home, Radio, Presets and DX', async ({ page }) => {
  await liveRadio(page, FM);
  for (const hash of ['/', '/#radio', '/#presets', '/#dx']) {
    await page.goto(hash);
    await page.locator('main').getByTitle('Tap to type a frequency').click();
    const box = page.getByLabel('Frequency to tune');
    await expect(box).toBeFocused();
    await box.fill('98.3');
    const sent = nextPost(page, '/api/tune');
    await box.press('Enter');
    expect(await sent).toBe('khz=98300');
  }
});

test('the scale takes the keys: with focus there, Left and Right step the dial', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'keys are for a laptop');
  await liveRadio(page, FM);
  await page.goto('/#radio');
  const scale = page.getByRole('slider', { name: 'Band scale' });
  await expect(scale).toHaveAttribute('aria-valuetext', '106.40 MHz');
  await expect(scale).toHaveAttribute('aria-valuenow', '106400');
  const row = await scaleRow(page);
  const at = () => row.evaluate((e) => Math.round(e.scrollLeft));
  await expect.poll(at).toBeGreaterThan(0);
  const before = await at();
  await scale.focus();
  await expect(scale).toBeFocused();
  // The focus leaves the scale where it is, the dial in view.
  expect(await at()).toBe(before);
  let sent = nextPost(page, '/api/step');
  await page.keyboard.press('ArrowRight');
  expect(await sent).toBe('stp=1');
  sent = nextPost(page, '/api/step');
  await page.keyboard.press('ArrowLeft');
  expect(await sent).toBe('stp=-1');
});

// The radio holds its answer to a tune, so a test sees what is drawn before
// it answers.
const slowTune = (page) => holdAnswers(page, '**/api/tune');

for (const [name, tun] of [
  ['OIRT, 30 kHz apart', { bnd: 'OIRT', khz: 69440, f: '69.44', stp: 30 }],
  ['FM, 200 kHz apart', { khz: 106300, f: '106.30', stp: 200 }],
]) {
  test(`a tap on the scale tunes to a channel on the dial's own grid: ${name}`, async ({ page }) => {
    await liveRadio(page, (s) => {
      FM(s);
      Object.assign(s.tun, tun);
    });
    const answer = await slowTune(page);
    await page.goto('/#radio');
    const scale = page.getByRole('slider', { name: 'Band scale' });
    const row = await scaleRow(page);
    await expect.poll(() => row.evaluate((e) => e.scrollLeft)).toBeGreaterThan(0);
    // 1 MHz right of the dial is 80 px.
    const dial = await scale.locator('[data-mark=dial]').boundingBox();
    const sent = nextPost(page, '/api/tune');
    await page.mouse.click(dial.x + dial.width / 2 + 80, dial.y + 4);
    const khz = Number(new URLSearchParams(await sent).get('khz'));
    expect((khz - tun.khz) % tun.stp).toBe(0);
    expect(Math.abs(khz - tun.khz - 1000)).toBeLessThanOrEqual(tun.stp);
    // Drawn at once, before the radio answers.
    await expect(scale).toHaveAttribute('aria-valuenow', String(khz));
    answer();
  });
}

test('an alternative frequency is drawn at once, before the radio answers', async ({ page }) => {
  await liveRadio(page, (s) => {
    FM(s);
    s.tun.rds.af = [106400, 104000];
  });
  const answer = await slowTune(page);
  await page.goto('/#radio');
  await page.locator('main section').filter({ hasText: 'Groups seen' }).getByRole('button', { name: '104.00' }).click();
  // Before the radio answers.
  await expect(reading(page)).toContainText('104.00');
  // Preset 8 holds it; with no RDS yet the preset's name shows.
  await expect(reading(page).getByText('P08', { exact: true })).toBeVisible();
  await expect(reading(page).locator('p').filter({ hasText: 'KOOL FM' })).toBeVisible();
  answer();
});
