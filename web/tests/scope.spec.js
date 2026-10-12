// Band scope: the sweep read when there is a new one, whole band or span,
// dimmed while one runs, repeat, the marks, the waterfall, the downloads.
import fs from 'node:fs';
import { expect, test } from '@playwright/test';
import { holdAnswers, liveRadio, nextPost } from './radio.js';

// Ten channels from 87.5 MHz; 87.70 strong.
const sweepOf = (rev, extra = {}) => ({
  rev, running: false, abandoned: false, time: 1791619196, real: true, took_ms: 3300, width: 114, low: 87500, step: 100, count: 10, whole: true, floor: 10, band: 'FM',
  level: [100, 120, 500, 110, null, 90, 100, 130, 95, 105], ...extra,
});

// The radio: its state says which sweep is the latest, and /api/scope gives
// it. r.next(sweep) is a new sweep on the radio, which its next state says.
async function radio(page, { tun = {}, rev = 1, sweep = sweepOf(1) } = {}) {
  const r = { sweep, reads: 0 };
  r.live = await liveRadio(page, (s) => {
    Object.assign(s.tun, { bnd: 'FM', khz: 87700, f: '87.70', unt: 'MHz', stp: 100 }, tun);
    s.scp = { rev, run: false };
    s.pnl.cur = null;
  });
  r.next = (next) => {
    r.sweep = next;
    r.live.push((s) => (s.scp.rev = next.rev));
  };
  await page.route('**/api/scope', (route) => {
    if (route.request().method() !== 'GET') return route.fallback();
    r.reads++;
    return route.fulfill({ json: r.sweep ?? { rev: 0, running: false, count: 0 } });
  });
  await page.route('**/api/presets.csv', (route) =>
    route.fulfill({ body: 'slot,band,frequency,bandwidth,name,pi\n9,FM,87700,0,MAGIC,1064\n2,MW,738,0,,\n' }),
  );
  await page.route('**/api/dx', (route) =>
    route.request().method() === 'GET'
      ? route.fulfill({ body: '{"on":false}\n{"i":0,"id":3,"khz":88200,"band":"FM","pi":"26FF","name":"MIRCHI  ","level_dbuv":300}\n' })
      : route.fallback(),
  );
  return r;
}

const panel = (page) => page.locator('main section').filter({ has: page.locator('h2', { hasText: 'scope' }) });
const chart = (page) => page.getByRole('img', { name: 'Level of every channel' });

test('no sweep yet says so', async ({ page }) => {
  await radio(page, { rev: 0, sweep: null });
  await page.goto('/#scope');
  await expect(panel(page)).toContainText('No sweep yet.');
});

test('the last sweep, what it is, and a tap tunes there', async ({ page }) => {
  await radio(page);
  await page.goto('/#scope');
  await expect(panel(page).locator('h2')).toContainText('FM scope');
  await expect(panel(page)).toContainText('Whole band · 10 channels · width 114 kHz · floor 1.0 dBµV · took 3.3 s');
  const box = await chart(page).boundingBox();
  // The radio holds its answer: the tune is drawn before it answers.
  const answer = await holdAnswers(page, '**/api/tune');
  const sent = nextPost(page, '/api/tune');
  await page.mouse.click(box.x + (5.5 / 10) * box.width, box.y + box.height / 2);
  expect(await sent).toBe('khz=88000');
  await expect(page.getByRole('link', { name: /88\.00/ })).toBeVisible();
  answer();
});

test('the whole band or the span round the dial', async ({ page }) => {
  await radio(page);
  await page.goto('/#scope');
  let sent = nextPost(page, '/api/scope');
  await page.getByRole('button', { name: 'Sweep', exact: true }).click();
  expect(await sent).toBe('sweep=1&span=0');
  await page.getByRole('radio', { name: 'Span' }).click();
  sent = nextPost(page, '/api/scope');
  await page.getByRole('button', { name: /Sweep/ }).click();
  expect(await sent).toBe('sweep=1&span=3600');
});

test('on MW the span is 360 kHz, and SW between two metre bands has the span only', async ({ page }) => {
  await radio(page, { tun: { bnd: 'MW', khz: 738, f: '738', unt: 'kHz', stp: 9 }, sweep: null, rev: 0 });
  await page.goto('/#scope');
  await page.getByRole('radio', { name: 'Span' }).click();
  let sent = nextPost(page, '/api/scope');
  await page.getByRole('button', { name: 'Sweep', exact: true }).click();
  expect(await sent).toBe('sweep=1&span=360');
  await page.unrouteAll();
  await radio(page, { tun: { bnd: 'SW', khz: 8000, f: '8000', unt: 'kHz', stp: 5 }, sweep: null, rev: 0 });
  await page.reload();
  await expect(page.getByRole('radiogroup', { name: 'What to sweep' })).toHaveCount(0);
  sent = nextPost(page, '/api/scope');
  await page.getByRole('button', { name: 'Sweep', exact: true }).click();
  expect(await sent).toBe('sweep=1&span=360');
});

test('on SW in a metre band the band chip names it, and the whole of it is swept first', async ({ page }) => {
  await radio(page, { tun: { bnd: 'SW', khz: 13845, f: '13845', unt: 'kHz', stp: 5, mtb: '22 m' }, sweep: null, rev: 0 });
  await page.goto('/#scope');
  const top = page.locator('main section').first();
  await expect(top.getByText('SW 22 m', { exact: true })).toBeVisible();
  await expect(page.getByRole('radio', { name: '22 m band' })).toHaveAttribute('aria-checked', 'true');
  let sent = nextPost(page, '/api/scope');
  await page.getByRole('button', { name: 'Sweep', exact: true }).click();
  expect(await sent).toBe('sweep=1&span=0');
  await page.getByRole('radio', { name: 'Span' }).click();
  sent = nextPost(page, '/api/scope');
  await page.getByRole('button', { name: 'Sweep', exact: true }).click();
  expect(await sent).toBe('sweep=1&span=360');
});

test('Log logs the station on the dial', async ({ page }) => {
  await radio(page);
  await page.goto('/#scope');
  const sent = nextPost(page, '/api/log');
  await page.getByRole('button', { name: 'Log', exact: true }).click();
  // The log takes the station on the dial: the post carries nothing.
  expect(await sent).toBeNull();
  await expect(page.getByText(/^Logged /)).toBeVisible();
});

test('a sweep is read only when the radio has a new one; the chart dims until it comes', async ({ page }) => {
  const r = await radio(page);
  await page.goto('/#scope');
  await expect(chart(page)).toBeVisible();
  // States that bring no new sweep, as the radio sends them twice a second.
  for (let i = 0; i < 4; i++) r.live.push();
  await page.evaluate(() => {});
  expect(r.reads).toBe(1);
  await page.getByRole('button', { name: 'Sweep', exact: true }).click();
  const area = panel(page).locator('[aria-busy]');
  await expect(area).toHaveAttribute('aria-busy', 'true');
  await expect(panel(page).locator('h2')).toContainText('sweeping');
  r.next(sweepOf(2, { level: [100, 120, 520, 110, 80, 90, 100, 130, 95, 105] }));
  await expect(area).toHaveAttribute('aria-busy', 'false');
  expect(r.reads).toBe(2);
  await expect(page.getByRole('img', { name: 'Waterfall of 2 sweeps' })).toBeVisible();
});

test('a tap on a waterfall row shows that sweep', async ({ page }) => {
  const r = await radio(page);
  await page.goto('/#scope');
  await expect(chart(page)).toBeVisible();
  r.next(sweepOf(2, { level: [100, 120, 520, 110, 80, 90, 100, 130, 95, 105] }));
  const fall = page.getByRole('img', { name: 'Waterfall of 2 sweeps' });
  await expect(fall).toBeVisible();
  // A tap goes where the screen shows it, so the waterfall must be on it.
  await fall.scrollIntoViewIfNeeded();
  // A row a sweep from the top, of 24.
  const box = await fall.boundingBox();
  const row = (k) => box.y + ((k + 0.5) / 24) * box.height;
  await page.mouse.click(box.x + box.width / 2, row(1));
  await expect(panel(page)).toContainText('an older sweep, 1 back');
  await page.mouse.click(box.x + box.width / 2, row(0));
  await expect(panel(page)).not.toContainText('an older sweep');
});

test('repeat asks for the next sweep when one ends, and stops when one is stopped on the radio', async ({ page }) => {
  const r = await radio(page);
  await page.goto('/#scope');
  await expect(chart(page)).toBeVisible();
  let sent = nextPost(page, '/api/scope');
  await page.getByRole('switch', { name: 'Repeat' }).click();
  expect(await sent).toBe('sweep=1&span=0');
  await expect(page.getByRole('switch', { name: 'Repeat' })).toHaveAttribute('aria-checked', 'true');
  sent = nextPost(page, '/api/scope');
  r.next(sweepOf(2));
  expect(await sent).toBe('sweep=1&span=0');
  r.next(sweepOf(3, { abandoned: true }));
  await expect(page.getByText('Repeat stopped: the sweep was stopped on the radio.')).toBeVisible();
  await expect(page.getByRole('switch', { name: 'Repeat' })).toHaveAttribute('aria-checked', 'false');
});

test('presets of this band and DX catches are marked and named', async ({ page }) => {
  await radio(page);
  await page.goto('/#scope');
  await expect(panel(page)).toContainText('87.70 MHz · 50.0 dBµV · P09 MAGIC');
  await expect(chart(page).locator('[data-mark=catch]')).toHaveCount(1);
  const box = await chart(page).boundingBox();
  await page.mouse.move(box.x + (7.5 / 10) * box.width, box.y + box.height / 2);
  await expect(panel(page)).toContainText('88.20 MHz · 13.0 dBµV · caught MIRCHI');
});

test('CSV and PNG save the sweep', async ({ page }) => {
  await radio(page);
  await page.goto('/#scope');
  await expect(chart(page)).toBeVisible();
  let download = page.waitForEvent('download');
  await page.getByRole('button', { name: 'CSV' }).click();
  let file = await download;
  expect(file.suggestedFilename()).toMatch(/^scope-FM-\d{8}-\d{4}\.csv$/);
  const text = fs.readFileSync(await file.path(), 'utf8');
  expect(text.split('\n').slice(0, 3)).toEqual(['khz,dbuv', '87500,10.0', '87600,12.0']);
  expect(text).toContain('87900,\n');
  download = page.waitForEvent('download');
  await page.getByRole('button', { name: 'PNG' }).click();
  file = await download;
  expect(file.suggestedFilename()).toMatch(/^scope-FM-\d{8}-\d{4}\.png$/);
  expect(fs.readFileSync(await file.path()).subarray(1, 4).toString()).toBe('PNG');
});

test('the station card on top holds the tools; the chart is in a card under it', async ({ page }) => {
  await page.goto('/#scope');
  const top = page.locator('main section').first();
  await expect(top.getByTitle('Tap to type a frequency')).toBeVisible();
  for (const name of ['Sweep', 'PNG', 'CSV']) await expect(top.getByRole('button', { name })).toBeVisible();
  await expect(top.getByRole('switch', { name: 'Repeat' })).toBeVisible();
  await expect(top.getByRole('radiogroup', { name: 'What to sweep' })).toBeVisible();
  await expect(page.locator('main section').nth(1)).toContainText('scope');
});

test('Older and Newer pick a sweep from the waterfall, by keys or a finger', async ({ page }) => {
  const r = await radio(page);
  await page.goto('/#scope');
  await expect(chart(page)).toBeVisible();
  // One sweep: nothing to pick.
  await expect(page.getByRole('button', { name: 'Older sweep' })).toHaveCount(0);
  r.next(sweepOf(2, { level: [100, 120, 520, 110, 80, 90, 100, 130, 95, 105] }));
  await expect(page.getByRole('img', { name: 'Waterfall of 2 sweeps' })).toBeVisible();
  const older = page.getByRole('button', { name: 'Older sweep' });
  const newer = page.getByRole('button', { name: 'Newer sweep' });
  await expect(newer).toBeDisabled();
  expect((await older.boundingBox()).height).toBe(44);
  await older.focus();
  await page.keyboard.press('Enter');
  await expect(panel(page)).toContainText('an older sweep, 1 back');
  await expect(older).toBeDisabled();
  await newer.click();
  await expect(panel(page)).not.toContainText('an older sweep');
});

test('the chart by keys: Left and Right pick a channel and say it, Enter tunes there', async ({ page }) => {
  await radio(page);
  await page.goto('/#scope');
  const slider = page.getByRole('slider', { name: 'Pick a channel, Enter tunes to it' });
  await expect(slider).toHaveAttribute('aria-valuetext', '87.70 MHz · 50.0 dBµV · P09 MAGIC');
  await slider.focus();
  await page.keyboard.press('ArrowRight');
  await expect(panel(page)).toContainText('87.80 MHz · 11.0 dBµV');
  await expect(slider).toHaveAttribute('aria-valuetext', '87.80 MHz · 11.0 dBµV');
  await expect(slider).toHaveAttribute('aria-valuenow', '87800');
  await page.keyboard.press('ArrowLeft');
  await page.keyboard.press('ArrowLeft');
  await expect(panel(page)).toContainText('87.60 MHz · 12.0 dBµV');
  const sent = nextPost(page, '/api/tune');
  await page.keyboard.press('Enter');
  expect(await sent).toBe('khz=87600');
});

test('the waterfall: each level a colour of the ramp, its foot the page background for no reading and rows not swept yet, in each theme', async ({ page }) => {
  await page.emulateMedia({ colorScheme: 'dark' });
  await radio(page);
  await page.goto('/#scope');
  const fall = page.getByRole('img', { name: 'Waterfall of 1 sweeps' });
  await expect(fall).toBeVisible();
  const at = (x, y) => fall.evaluate((c, [x, y]) => [...c.getContext('2d').getImageData(x, y, 1, 1).data], [x, y]);
  // A floor of 1 dBµV puts the foot of the scale at -2 dBµV: 50 dBµV is
  // 81 % up it, 10 dBµV 19 %.
  await expect.poll(() => at(2, 0)).toEqual([246, 158, 61, 255]);
  expect(await at(0, 0)).toEqual([58, 26, 15, 255]);
  expect(await at(4, 0)).toEqual([14, 16, 19, 255]);
  expect(await at(0, 5)).toEqual([14, 16, 19, 255]);
  // The light theme's background at the foot, drawn again at the switch.
  await page.getByRole('button', { name: 'Light theme' }).click();
  await expect.poll(() => at(0, 5)).toEqual([242, 243, 245, 255]);
  expect(await at(4, 0)).toEqual([242, 243, 245, 255]);
});

test('a sweep that could not be read is said, and read again by itself, 5 s on', async ({ page }) => {
  await page.clock.install();
  const r = await radio(page);
  await page.route('**/api/scope', (route) => (r.reads === 0 && route.request().method() === 'GET' ? (r.reads++, route.fulfill({ status: 503, body: '' })) : route.fallback()));
  await page.goto('/#scope');
  await expect(panel(page)).toContainText('The sweep could not be read. Trying again.');
  // The first state that comes 5 s on.
  await page.clock.runFor(5000);
  r.live.push();
  await expect(chart(page)).toBeVisible();
  expect(r.reads).toBe(2);
});
