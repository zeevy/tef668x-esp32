// DX: the four tabs, DX mode and its width, the station in detail, the
// scope and its baseline, the scanner, the catches.
import { expect, test } from '@playwright/test';
import { holdAnswers, liveRadio, nextPost } from './radio.js';

// DX mode's head as the live frame carries it.
const HEAD = { on: true, pg: 0, n: 2, rev: 1, swr: 1, swp: false, scn: 'idle', sck: 0, scf: 0, scd: 0, sct: 0, dst: 0, dsc: 0, ddw: 25, wup: 0, wri: 0, wms: 0 };
const CATCHES = [
  { i: 0, id: 41, time: 1791619436, real: true, band: 'FM', khz: 98300, pi: '26FF', name: 'MIRCHI  ', country: 'IN', new: true, count: 3, level_dbuv: 488, logged: false, due: true },
  { i: 1, id: 40, time: 1791619326, real: true, band: 'FM', khz: 93500, pi: '0935', name: ' RED FM ', country: null, new: false, count: 5, level_dbuv: 432, logged: true, due: false },
];
// Ten channels from 87.5 MHz; the third one strong.
const SWEEP = {
  rev: 1, running: false, abandoned: false, time: 1791619196, real: true, took_ms: 300, width: 114, low: 87500, step: 100, count: 10, floor: 10,
  baseline: 'median', baseline_sweeps: 3,
  level: [100, 120, 500, 110, null, 90, 100, 130, 95, 105],
  baseline_level: [90, 110, 300, 100, null, 80, 95, 120, 90, 100],
  rise: [10, 10, 200, 10, null, 10, 5, 10, 5, 5],
  peak: null, kept: [1791619080, 1791619011], baseline_kept: 0,
};

// The radio on the live socket, in DX mode, with its catches and its last
// sweep. Gives the socket's end, to push a changed state.
async function radio(page, { head = {}, catches = CATCHES, sweep = SWEEP, tun = {}, rds = {} } = {}) {
  const live = await liveRadio(page, (s) => {
    s.dx = { ...HEAD, n: catches.length, ...head };
    Object.assign(s.tun, { bnd: 'FM', khz: 87700, f: '87.70', unt: 'MHz', stp: 100, bws: 114, pst: 0, sig: 452, usn: 120, wam: 80, off: -15, bw: 114, mod: 42, plt: true }, tun);
    s.tun.rds = { syn: true, blk: [0, 1, 3, 0], ...rds };
  });
  await page.route('**/api/dx', (route) =>
    route.request().method() === 'GET'
      ? route.fulfill({ body: [{ on: true }, ...catches].map((x) => JSON.stringify(x)).join('\n') + '\n' })
      : route.fallback(),
  );
  await page.route('**/api/dx/sweep', (route) => route.fulfill({ json: sweep ?? { rev: 0, count: 0, level: null } }));
  return live;
}

const card = (page, title) => page.locator('main section').filter({ has: page.locator('h2', { hasText: title }) });
const tab = (page, name) => page.getByRole('radio', { name });

const top = (page) => page.locator('main section').first();

test('four tabs at the foot of the station card, Signal first, each showing its own part', async ({ page }) => {
  await radio(page);
  await page.goto('/#dx');
  await expect(top(page).getByRole('radiogroup', { name: 'DX page' })).toBeVisible();
  await expect(tab(page, 'Signal')).toHaveAttribute('aria-checked', 'true');
  await expect(card(page, 'Signal')).toBeVisible();
  for (const [name, title] of [['Scope', 'Scope'], ['Scanner', 'Scanner'], ['Catches', 'Catches']]) {
    await tab(page, name).click();
    await expect(card(page, title)).toBeVisible();
    await expect(card(page, 'Signal')).toHaveCount(0);
  }
});

test('DX mode on and off, and its width', async ({ page }) => {
  await radio(page);
  await page.goto('/#dx');
  let sent = nextPost(page, '/api/dx');
  await expect(page.getByRole('switch', { name: 'DX mode' })).toHaveAttribute('aria-checked', 'true');
  await page.getByRole('switch', { name: 'DX mode' }).click();
  expect(await sent).toBe('on=0');
  sent = nextPost(page, '/api/dx');
  await page.getByLabel('DX width').selectOption('84');
  expect(await sent).toBe('khz=84');
});

test('DX mode says it is FM only on AM', async ({ page }) => {
  await radio(page, { head: { on: false }, tun: { bnd: 'MW', khz: 738, f: '738', unt: 'kHz' } });
  await page.goto('/#dx');
  await expect(page.getByText('DX mode works on FM and OIRT. Switch to FM first.')).toBeVisible();
});

test('the PI: confirmed, heard with its unsure digits, a station sending 0000, nothing yet', async ({ page }) => {
  for (const [rds, value] of [
    [{ pi: '26FF' }, '26FF'],
    [{ pih: '26?F' }, '26?F'],
    [{ piz: true }, '0000'],
    [{}, '----'],
  ]) {
    await page.unrouteAll();
    await radio(page, { rds });
    await page.goto('/?' + value + '#dx');
    await expect(top(page)).toContainText(`PI ${value}`);
  }
});

test('the name fills in a character at a time', async ({ page }) => {
  await radio(page, { rds: { psh: 'MIR     ', psm: 0b111 } });
  await page.goto('/#dx');
  await expect(top(page)).toContainText('MIR-----');
});

test('every reading of the station, its country and the radio text', async ({ page }) => {
  await radio(page, { rds: { pi: '26FF', cty: 'IN', rt: 'A RADIO TEXT' } });
  await page.goto('/#dx');
  const signal = card(page, 'Signal');
  for (const text of ['45.2 dBµV', 'Noise', '12 %', 'Multipath', '8 %', '-1.5 kHz', '114 kHz', '42 %', 'A RADIO TEXT']) await expect(signal).toContainText(text);
  await expect(top(page)).toContainText('PI 26FF · IN');
});

test('RDS blocks A to D as the radio draws them: 4 segments clean, 3 or 2 fixed, 1 lost, none with no RDS', async ({ page }) => {
  await radio(page, { rds: { blk: [0, 1, 3, 2] } });
  await page.goto('/#dx');
  await expect(page.getByRole('img', { name: 'RDS blocks: A 4 of 4, B 3 of 4, C 1 of 4, D 2 of 4' })).toBeVisible();
  await page.unrouteAll();
  await radio(page, { rds: { syn: false } });
  await page.reload();
  await expect(page.getByRole('img', { name: 'RDS blocks: A 0 of 4, B 0 of 4, C 0 of 4, D 0 of 4' })).toBeVisible();
});

test('the scope: a tap tunes to that channel', async ({ page }) => {
  await radio(page);
  await page.goto('/#dx');
  await tab(page, 'Scope').click();
  const chart = page.getByRole('img', { name: 'Level of every channel' });
  // The tuned channel, 87.70 MHz, stands out from the rest, and is named under the chart.
  // The bars are one path, a bar a move to; the tuned one is drawn on its own.
  await expect.poll(async () => (await chart.locator('[data-mark=levels]').getAttribute('d')).match(/M/g).length).toBe(8);
  await expect(card(page, 'Scope')).toContainText('87.70 MHz · 50.0 dBµV · +20.0 dB over its baseline');
  const box = await chart.boundingBox();
  const sent = nextPost(page, '/api/tune');
  await page.mouse.click(box.x + (2.5 / 10) * box.width, box.y + box.height / 2);
  expect(await sent).toBe('khz=87700');
});

test('the scope: sweep, and the baseline from the median or a kept sweep', async ({ page }) => {
  await radio(page);
  await page.goto('/#dx');
  await tab(page, 'Scope').click();
  let sent = nextPost(page, '/api/dx');
  await card(page, 'Scope').getByRole('button', { name: 'Sweep' }).click();
  expect(await sent).toBe('sweep=1');
  const baseline = page.getByLabel('Baseline', { exact: true });
  await expect(baseline.locator('option')).toHaveCount(3);
  await expect(baseline).toHaveValue('0');
  sent = nextPost(page, '/api/dx');
  await baseline.selectOption('2');
  expect(await sent).toBe('baseline=2');
});

test('the scope shows the radio\'s cursor where its knob puts it, and names that channel', async ({ page }) => {
  const at = (cur) => (s) => {
    Object.assign(s.tun, { bnd: 'FM', khz: 87700, f: '87.70', unt: 'MHz', stp: 100, sig: 452 });
    s.tun.rds = {};
    s.pnl.cur = cur;
  };
  await radio(page);
  await liveRadio(page, at(88200));
  await page.goto('/#dx');
  await tab(page, 'Scope').click();
  const chart = page.getByRole('img', { name: 'Level of every channel' });
  await expect(chart.locator('[data-mark=cursor]')).toHaveAttribute('x1', '7.4');
  await expect(card(page, 'Scope')).toContainText("88.20 MHz · 13.0 dBµV · +1.0 dB over its baseline · the radio's cursor");
  await liveRadio(page, at(null));
  await page.reload();
  await tab(page, 'Scope').click();
  await expect(chart.locator('[data-mark=cursor]')).toHaveCount(0);
  await expect(card(page, 'Scope')).toContainText('87.70 MHz · 50.0 dBµV');
});

test('the scope reads the floor in tenths of a dBµV, as every level', async ({ page }) => {
  // As the radio sent it: a floor of 8.1 dBµV, levels 5.1 to 41.1.
  await radio(page, { tun: { khz: 98300, f: '98.30' }, sweep: { ...SWEEP, floor: 81, level: [51, 90, 411, 100, null, 85, 95, 120, 70, 60] } });
  await page.goto('/#dx');
  await tab(page, 'Scope').click();
  await expect(card(page, 'Scope')).toContainText('Floor 8.1 dBµV');
  await expect(card(page, 'Scope')).toContainText('The dial is off this sweep.');
  // The third channel's bar in the path: M x top h0.8 v height.
  const bars = (await page.getByRole('img', { name: 'Level of every channel' }).locator('[data-mark=levels]').getAttribute('d')).split('M');
  expect(Number(bars.find((b) => b.startsWith('2 ')).match(/v([\d.]+)/)[1])).toBeGreaterThan(50);
});

test('the scope dims from the Sweep press until the new sweep comes', async ({ page }) => {
  let rev = 1;
  await radio(page);
  const live = await liveRadio(page, (s) => (s.dx = { ...HEAD, swr: rev }));
  await page.route('**/api/dx/sweep', (route) => route.fulfill({ json: { ...SWEEP, rev } }));
  await page.goto('/#dx');
  await tab(page, 'Scope').click();
  const chart = card(page, 'Scope').locator('[aria-busy]');
  await expect(chart).toHaveAttribute('aria-busy', 'false');
  await card(page, 'Scope').getByRole('button', { name: 'Sweep' }).click();
  await expect(chart).toHaveAttribute('aria-busy', 'true');
  await expect(chart).toHaveCSS('opacity', '0.3');
  await expect(card(page, 'Scope').locator('h2')).toHaveText('Scope · sweeping');
  rev = 2;
  live.push((s) => (s.dx.swr = rev));
  await expect(chart).toHaveAttribute('aria-busy', 'false');
  await expect(chart).toHaveCSS('opacity', '1');
});

test('the scope says when there is no sweep yet', async ({ page }) => {
  await radio(page, { sweep: null });
  await page.goto('/#dx');
  await tab(page, 'Scope').click();
  await expect(card(page, 'Scope')).toContainText('No sweep yet.');
});

test('the sweep and the catches are read only when the live frame says they changed', async ({ page }) => {
  let sweeps = 0;
  let lists = 0;
  await radio(page);
  const live = await liveRadio(page, (s) => (s.dx = { ...HEAD, swr: 1, rev: 1 }));
  await page.route('**/api/dx', (route) => {
    lists++;
    return route.fulfill({ body: '{"on":true}\n' });
  });
  await page.route('**/api/dx/sweep', (route) => {
    sweeps++;
    return route.fulfill({ json: SWEEP });
  });
  await page.goto('/#dx');
  await expect.poll(() => sweeps).toBe(1);
  expect(lists).toBe(1);
  // Frames that change neither, as the radio sends them twice a second.
  for (let i = 0; i < 4; i++) live.push();
  live.push((s) => (s.dx.swr = 2));
  await expect.poll(() => sweeps).toBe(2);
  expect(lists).toBe(1);
  live.push((s) => (s.dx.rev = 2));
  await expect.poll(() => lists).toBe(2);
  expect(sweeps).toBe(2);
});

test('the scanner: ready, running with its progress, stopped; start, stop, go on, learn', async ({ page }) => {
  await radio(page);
  await page.goto('/#dx');
  await tab(page, 'Scanner').click();
  const scanner = card(page, 'Scanner');
  await expect(scanner).toContainText('Ready');
  await expect(scanner).toContainText('Band and presets · stops on new stations · 2.5 s a channel');
  let sent = nextPost(page, '/api/dx');
  await scanner.getByRole('button', { name: 'Start scan' }).click();
  expect(await sent).toBe('scan=1');
  sent = nextPost(page, '/api/dx');
  await scanner.getByRole('button', { name: 'Learn locals' }).click();
  expect(await sent).toBe('learn=1');

  await page.unrouteAll();
  await radio(page, { head: { scn: 'running', sck: 98300, scf: 2, scd: 50, sct: 200 } });
  await page.reload();
  await tab(page, 'Scanner').click();
  await expect(card(page, 'Scanner')).toContainText('Scanning 98.30 MHz');
  await expect(card(page, 'Scanner')).toContainText('2 found');
  await expect(page.getByRole('progressbar', { name: 'Scan progress' })).toHaveAttribute('aria-valuenow', '25');
  sent = nextPost(page, '/api/dx');
  await card(page, 'Scanner').getByRole('button', { name: 'Stop' }).click();
  expect(await sent).toBe('scan=0');

  await page.unrouteAll();
  await radio(page, { head: { scn: 'stopped', sck: 98300 } });
  await page.reload();
  await tab(page, 'Scanner').click();
  await expect(card(page, 'Scanner')).toContainText('Stopped on 98.30 MHz');
  sent = nextPost(page, '/api/dx');
  await card(page, 'Scanner').getByRole('button', { name: 'Go on' }).click();
  expect(await sent).toBe('scan=1');
});

test('the catches: a tap tunes, Log writes one to the log, a logged one says so', async ({ page }) => {
  await radio(page);
  await page.goto('/#dx');
  await tab(page, 'Catches').click();
  const catches = card(page, 'Catches');
  await expect(catches.locator('h2')).toHaveText('Catches · 2');
  await expect(catches).toContainText('MIRCHI');
  await expect(catches).toContainText('New');
  await expect(catches).toContainText('PI 26FF · IN · 48.8 dBµV · ×3');
  await expect(catches.getByRole('button', { name: 'Log' })).toHaveCount(1);
  await expect(catches).toContainText('Logged');
  let sent = nextPost(page, '/api/tune');
  await catches.getByRole('button', { name: /RED FM/ }).click();
  expect(await sent).toBe('khz=93500');
  sent = nextPost(page, '/api/dx');
  await catches.getByRole('button', { name: 'Log' }).click();
  expect(await sent).toBe('logid=41');
  await expect(catches.getByRole('link', { name: 'FMLIST CSV' })).toHaveAttribute('href', '/api/dx.csv');
});

test('a preset coming up shows as a note once, as the radio says it; one from before the page opened does not', async ({ page }) => {
  await radio(page);
  const live = await liveRadio(page, (s) => (s.dx = { ...HEAD, wup: 98300, wri: 60, wms: 3000 }));
  await page.goto('/#dx');
  await expect(page.getByRole('switch', { name: 'DX mode' })).toBeVisible();
  // The same one again, as every frame carries it until the next.
  live.push();
  await page.evaluate(() => {});
  await expect(page.getByText('98.30 MHz up 6 dB')).toHaveCount(0);
  live.push((s) => Object.assign(s.dx, { wup: 98300, wri: 92, wms: 5000 }));
  await expect(page.getByText('98.30 MHz up 9 dB')).toBeVisible();
  await expect(page.getByText('98.30 MHz up 6 dB')).toHaveCount(0);
});

test('the DX width shows DX mode\'s own width, not the FM one', async ({ page }) => {
  await radio(page, { tun: { bws: 0, dxw: 84 } });
  await page.goto('/#dx');
  await expect(page.getByLabel('DX width')).toHaveValue('84');
});

test('catches and a sweep that could not be read are said, and read again by themselves, 5 s on', async ({ page }) => {
  await page.clock.install();
  const live = await radio(page);
  const reads = { list: 0, sweep: 0 };
  await page.route('**/api/dx', (route) => (route.request().method() === 'GET' && ++reads.list === 1 ? route.fulfill({ status: 503, body: '' }) : route.fallback()));
  await page.route('**/api/dx/sweep', (route) => (++reads.sweep === 1 ? route.fulfill({ status: 503, body: '' }) : route.fallback()));
  await page.goto('/#dx');
  await tab(page, 'Catches').click();
  await expect(card(page, 'Catches')).toContainText('The catches could not be read. Trying again.');
  await expect(card(page, 'Catches')).not.toContainText('No catches yet');
  await tab(page, 'Scope').click();
  await expect(card(page, 'Scope')).toContainText('The sweep could not be read. Trying again.');
  // The first state that comes 5 s on.
  await page.clock.runFor(5000);
  live.push();
  await expect(page.getByRole('img', { name: 'Level of every channel' })).toBeVisible();
  await tab(page, 'Catches').click();
  await expect(card(page, 'Catches').locator('h2')).toHaveText('Catches · 2');
});

test('a tune from a catch or from the scope is drawn at once, before the radio answers', async ({ page }) => {
  await radio(page);
  // The radio answers neither until the end.
  const answer = await holdAnswers(page, '**/api/tune');
  await page.goto('/#dx');
  await tab(page, 'Catches').click();
  await card(page, 'Catches').getByRole('button', { name: /RED FM/ }).click();
  await expect(top(page)).toContainText('93.50');
  await tab(page, 'Scope').click();
  const box = await page.getByRole('img', { name: 'Level of every channel' }).boundingBox();
  await page.mouse.click(box.x + (5.5 / 10) * box.width, box.y + box.height / 2);
  await expect(top(page)).toContainText('88.00');
  answer();
});
