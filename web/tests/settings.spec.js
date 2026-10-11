// Settings: the PIN, the groups, each kind of row sending its key to its
// route, a refusal putting the value back, the colours and the knob.
import fs from 'node:fs';
import { expect, test } from '@playwright/test';
import { countReads, liveRadio, nextPost } from './radio.js';

const stored = JSON.parse(fs.readFileSync(new URL('../mock/settings.json', import.meta.url)));

// The radio on the live socket, and its stored settings. Gives the socket's
// end, to push a changed state.
async function radio(page, { tun = {}, settings = {}, refuse = null, signed = true } = {}) {
  const live = await liveRadio(page, (s) => Object.assign(s.tun, { bnd: 'FM', ims: true, eq: false }, tun));
  await page.route('**/api/settings', (route) => {
    if (route.request().method() === 'GET') {
      return signed ? route.fulfill({ json: { ...stored, sqf: 12, agt: 0, fsn: 4, rds: 1, tof: 0, rgn: 3, cst: ['#101010', '#202020', '#303030'], ...settings } }) : route.fulfill({ status: 403, body: 'Enter the access PIN first.' });
    }
    return refuse ? route.fulfill({ status: 400, body: refuse }) : route.fulfill({ body: 'Saved and in use now.' });
  });
  return live;
}

// The group picker: a tile a group, on every screen size.
async function open(page, group) {
  const tile = page.getByRole('navigation', { name: 'Groups' }).getByRole('button', { name: group, exact: true });
  await tile.click();
  await expect(tile).toHaveAttribute('aria-current', 'page');
}

test('the settings need the PIN, and say so', async ({ page }) => {
  await radio(page, { signed: false });
  await page.goto('/#settings');
  await expect(page.getByText('The settings need the PIN.')).toBeVisible();
  await page.getByRole('button', { name: 'Enter the PIN' }).click();
  await expect(page.getByRole('dialog')).toBeVisible();
});

test('one group at a time, Sound first', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#settings');
  await expect(page.getByRole('navigation', { name: 'Groups' }).getByRole('button', { name: 'Sound', exact: true })).toHaveAttribute('aria-current', 'page');
  await expect(page.getByText('Squelch floor', { exact: true })).toBeVisible();
  await open(page, 'DX scanner');
  await expect(page.getByText('Dwell on each channel', { exact: true })).toBeVisible();
  await expect(page.getByText('Squelch floor', { exact: true })).toHaveCount(0);
});

test('Battery check is a switch in Display, off at first, and turns on', async ({ page }) => {
  await radio(page, { settings: { bck: 0 } });
  await page.goto('/#settings');
  await open(page, 'Display');
  const check = page.getByRole('switch', { name: 'Battery check' });
  await expect(check).toHaveAttribute('aria-checked', 'false');
  await expect(page.getByText('Every 5 min while nobody uses the radio', { exact: false })).toBeVisible();
  const sent = nextPost(page, '/api/settings');
  await check.click();
  expect(await sent).toBe('bck=1');
  await expect(check).toHaveAttribute('aria-checked', 'true');
});

test('less and more step through a setting\'s own values, Off first', async ({ page }) => {
  await radio(page);
  await page.goto('/#settings');
  let sent = nextPost(page, '/api/settings');
  await page.getByRole('button', { name: 'More Squelch floor' }).click();
  expect(await sent).toBe('sqf=13');
  await expect(page.getByLabel('Squelch floor', { exact: true })).toHaveText('13 dBµV');
  await expect(page.getByLabel('Volume AGC', { exact: true })).toHaveText('Off');
  sent = nextPost(page, '/api/settings');
  await page.getByRole('button', { name: 'More Volume AGC' }).click();
  expect(await sent).toBe('agt=30');
});

test('less and more from a value not in the list go to the nearest listed one that way', async ({ page }) => {
  // 33 % was set over the API; the row lists Off, then 30 to 80 % in twos.
  await radio(page, { settings: { agt: 33 } });
  await page.goto('/#settings');
  let sent = nextPost(page, '/api/settings');
  await page.getByRole('button', { name: 'More Volume AGC' }).click();
  expect(await sent).toBe('agt=34');
  await radio(page, { settings: { agt: 33 } });
  await page.reload();
  sent = nextPost(page, '/api/settings');
  await page.getByRole('button', { name: 'Less Volume AGC' }).click();
  expect(await sent).toBe('agt=32');
});

test('a reception setting stays as changed while the radio has not saved it yet', async ({ page }) => {
  // The state carries no blend reading, so the row shows the stored value,
  // which the radio only changes at its next save.
  const live = await radio(page, { tun: { bld: undefined }, settings: { bld: 0 } });
  await page.route('**/api/fm', (route) => route.fulfill({ body: 'Stereo blend 20 dBuV.' }));
  await page.goto('/#settings');
  await page.getByRole('navigation', { name: 'Groups' }).getByRole('button', { name: 'FM reception', exact: true }).click();
  const sent = nextPost(page, '/api/fm');
  await page.getByRole('button', { name: 'More Stereo blend' }).click();
  expect(await sent).toBe('bld=20');
  await expect(page.getByLabel('Stereo blend', { exact: true })).toHaveText('20 dBµV');
  // Two more states come, with no save between: the value stays. The page
  // takes a state as it comes, so by the next call into it, both are in.
  for (let i = 0; i < 2; i++) live.push();
  await page.evaluate(() => {});
  await expect(page.getByLabel('Stereo blend', { exact: true })).toHaveText('20 dBµV');
});

test('stereo blend, high blend and high cut show the stored levels, not the chip\'s live readings the state carries', async ({ page }) => {
  // In the state these three are what the chip does now, which moves with
  // the signal; the settings are the levels it starts at.
  const live = await radio(page, { tun: { bld: 7, hbl: 9, cut: 11 }, settings: { bld: 30, hbl: 40, cut: 0 } });
  await page.route('**/api/fm', (route) => route.fulfill({ body: 'High cut 20 dBuV.' }));
  await page.goto('/#settings');
  await open(page, 'FM reception');
  await expect(page.getByLabel('Stereo blend', { exact: true })).toHaveText('30 dBµV');
  await expect(page.getByLabel('High blend', { exact: true })).toHaveText('40 dBµV');
  await expect(page.getByLabel('High cut', { exact: true })).toHaveText('Off');
  const sent = nextPost(page, '/api/fm');
  await page.getByRole('button', { name: 'More High cut' }).click();
  expect(await sent).toBe('cut=20');
  // Drawn at once, and kept while the reading in the state moves.
  live.push((s) => (s.tun.cut = 13));
  await expect(page.getByLabel('High cut', { exact: true })).toHaveText('20 dBµV');
});

test('holding more steps on, and the radio gets the value once, after the release', async ({ page }) => {
  await page.clock.install();
  await radio(page);
  await page.goto('/#settings');
  const posts = [];
  page.on('request', (r) => r.method() === 'POST' && new URL(r.url()).pathname === '/api/settings' && posts.push(r.postData()));
  const more = await page.getByRole('button', { name: 'More Squelch floor' }).boundingBox();
  await page.mouse.move(more.x + more.width / 2, more.y + more.height / 2);
  await page.mouse.down();
  await page.clock.runFor(1300);
  await page.mouse.up();
  const shown = Number((await page.getByLabel('Squelch floor', { exact: true }).textContent()).split(' ')[0]);
  expect(shown).toBeGreaterThan(15);
  await expect.poll(() => posts.length).toBe(1);
  expect(posts[0]).toBe(`sqf=${shown}`);
});

test('a reception switch goes to /api/fm and shows what the radio uses now', async ({ page }, testInfo) => {
  await radio(page, { settings: { ims: 0 } });
  await page.goto('/#settings');
  await open(page, 'FM reception');
  const ims = page.getByRole('switch', { name: 'iMS' });
  await expect(ims).toHaveAttribute('aria-checked', 'true');
  const sent = nextPost(page, '/api/fm');
  await ims.click();
  expect(await sent).toBe('ims=0');
  await expect(ims).toHaveAttribute('aria-checked', 'false');
});

test('a stored switch goes to /api/settings; Touch is the other way round', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#settings');
  await open(page, 'FM reception');
  let sent = nextPost(page, '/api/settings');
  await page.getByRole('switch', { name: 'RDS decoder' }).click();
  expect(await sent).toBe('rds=0');
  await open(page, 'Controls');
  const touch = page.getByRole('switch', { name: 'Touch' });
  await expect(touch).toHaveAttribute('aria-checked', 'true');
  sent = nextPost(page, '/api/settings');
  await touch.click();
  expect(await sent).toBe('tof=1');
});

test('a choice of a few side by side, and a drop-down taken at the next start', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#settings');
  await open(page, 'FM reception');
  let sent = nextPost(page, '/api/settings');
  await page.getByRole('radiogroup', { name: 'Seek sensitivity' }).getByRole('radio', { name: '5' }).click();
  expect(await sent).toBe('fsn=5');
  await expect(page.getByText('At the next start').first()).toBeVisible();
  sent = nextPost(page, '/api/settings');
  await page.getByLabel('Band plan').selectOption('2');
  expect(await sent).toBe('rgn=2');
  await expect(page.getByText('Band plan is taken at the next start of the radio.')).toBeVisible();
});

test('a refusal puts the value back and says why', async ({ page }) => {
  await radio(page, { refuse: 'That value is out of range.' });
  await page.goto('/#settings');
  await page.getByRole('button', { name: 'More Squelch floor' }).click();
  await expect(page.getByText('That value is out of range.')).toBeVisible();
  await expect(page.getByLabel('Squelch floor', { exact: true })).toHaveText('12 dBµV');
});

test('on AM the FM-only rows say so', async ({ page }, testInfo) => {
  await radio(page, { tun: { bnd: 'MW' } });
  await page.goto('/#settings');
  await open(page, 'FM reception');
  await expect(page.getByText('Taken only on FM')).toHaveCount(6);
});

test('the Custom theme\'s colours', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#settings');
  await open(page, 'Display');
  await expect(page.getByLabel('Tile and row')).toHaveValue('#303030');
  const sent = nextPost(page, '/api/settings');
  await page.getByLabel('Background').fill('#123456');
  expect(await sent).toBe('tc0=%23123456');
});

test('the volume knob\'s ends: measure, then done', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#settings');
  await open(page, 'Controls');
  let sent = nextPost(page, '/api/pot');
  await page.getByRole('button', { name: 'Measure the knob' }).click();
  expect(await sent).toBe('act=start');
  await expect(page.getByText('Turn the volume knob all the way down and all the way up.')).toBeVisible();
  sent = nextPost(page, '/api/pot');
  await page.getByRole('button', { name: 'Done' }).click();
  expect(await sent).toBe('act=finish');
  await expect(page.getByRole('button', { name: 'Measure the knob' })).toBeVisible();
});

test('on a laptop the groups are tiles beside the open group, and each setting a row with its control on the right', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'a laptop only');
  await radio(page);
  await page.goto('/#settings');
  const menu = page.getByRole('navigation', { name: 'Groups' });
  await expect(menu.getByRole('button')).toHaveText(['Sound', 'FM reception', 'AM reception', 'Display', 'Controls', 'Clock', 'DX scanner', 'Auto off']);
  await expect(menu.getByRole('button', { name: 'Sound' })).toHaveAttribute('aria-current', 'page');
  await open(page, 'FM reception');
  await expect(menu.getByRole('button', { name: 'FM reception' })).toHaveAttribute('aria-current', 'page');
  // Every control ends at the same right edge, and starts right of its name.
  const right = async (l) => {
    const b = await l.boundingBox();
    return Math.round(b.x + b.width);
  };
  const edge = await right(page.getByRole('radiogroup', { name: 'Tuning step' }));
  expect(await right(page.getByRole('switch', { name: 'Force mono' }))).toBe(edge);
  expect(await right(page.getByLabel('Band plan'))).toBe(edge);
  const name = await page.getByText('Force mono', { exact: true }).boundingBox();
  expect(name.x).toBeLessThan((await page.getByRole('radiogroup', { name: 'Tuning step' }).boundingBox()).x);
});

test('on a phone the group tiles sit under the open group with every name whole, and a control sits under its name', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'phone', 'a phone only');
  await radio(page);
  await page.goto('/#settings');
  const menu = page.getByRole('navigation', { name: 'Groups' });
  await expect(menu).toBeVisible();
  // No name is cut: each fits its tile, on two lines where it must.
  const cut = await menu.locator('button span').evaluateAll((names) => names.filter((n) => n.scrollWidth > n.clientWidth || n.getBoundingClientRect().right > n.parentElement.getBoundingClientRect().right).map((n) => n.textContent));
  expect(cut).toEqual([]);
  expect((await menu.boundingBox()).y).toBeGreaterThan((await page.getByText('Sound', { exact: true }).first().boundingBox()).y);
  await open(page, 'FM reception');
  const name = await page.getByText('Tuning step', { exact: true }).boundingBox();
  const control = await page.getByRole('radiogroup', { name: 'Tuning step' }).boundingBox();
  expect(control.y).toBeGreaterThan(name.y);
});

test('a change on the radio\'s menu shows without a reload, when the radio stores it', async ({ page }, testInfo) => {
  const radioNow = { saves: 3, blt: 90, cst: ['#101010'] };
  const live = await liveRadio(page, (s) => (s.asv = { ...s.asv, n: radioNow.saves }));
  await page.route('**/api/settings', (route) => route.fulfill({ json: { ...stored, blt: radioNow.blt, cst: radioNow.cst } }));
  await page.goto('/#settings');
  await open(page, 'Display');
  await expect(page.getByLabel('Brightness', { exact: true })).toHaveText('90 %');
  await expect(page.getByLabel('Background')).toHaveValue('#101010');
  // Someone turns the brightness down and picks another colour on the
  // radio's menu; it stores them.
  Object.assign(radioNow, { blt: 40, cst: ['#abcdef'], saves: 4 });
  live.push((s) => (s.asv.n = radioNow.saves));
  await expect(page.getByLabel('Brightness', { exact: true })).toHaveText('40 %');
  await expect(page.getByLabel('Background')).toHaveValue('#abcdef');
});

test('a change on the radio\'s menu with saving by itself off is read when the radio says its settings differ from the stored ones', async ({ page }) => {
  const radioNow = { blt: 90 };
  const live = await liveRadio(page, (s) => (s.asv = { ...s.asv, n: 3, dif: false, idl: 0 }));
  await page.route('**/api/settings', (route) => route.fulfill({ json: { ...stored, blt: radioNow.blt } }));
  await page.goto('/#settings');
  await open(page, 'Display');
  await expect(page.getByLabel('Brightness', { exact: true })).toHaveText('90 %');
  // Someone turns the brightness down on the radio's menu. Nothing is
  // stored, so the count of saves stays.
  radioNow.blt = 40;
  live.push((s) => (s.asv.dif = true));
  await expect(page.getByLabel('Brightness', { exact: true })).toHaveText('40 %');
});

test('a stepper whose row goes away sends its change then, not never', async ({ page }) => {
  await radio(page);
  await page.goto('/#settings');
  const sent = nextPost(page, '/api/settings');
  await page.getByRole('button', { name: 'More Squelch floor' }).click();
  await open(page, 'Display');
  expect(await sent).toBe('sqf=13');
  await open(page, 'Sound');
  await expect(page.getByLabel('Squelch floor', { exact: true })).toHaveText('13 dBµV');
});

test('a band\'s own step and filter width, from the radio\'s band plan', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#settings');
  await open(page, 'AM reception');
  const width = page.getByRole('radiogroup', { name: 'Filter width, MW' });
  await expect(width.getByRole('radio')).toHaveText(['3 kHz', '4 kHz', '6 kHz', '8 kHz']);
  await expect(width.getByRole('radio', { name: '4 kHz' })).toHaveAttribute('aria-checked', 'true');
  let sent = page.waitForRequest((r) => r.method() === 'POST' && new URL(r.url()).pathname === '/api/bandwidth');
  await width.getByRole('radio', { name: '6 kHz' }).click();
  expect((await sent).postData()).toBe('khz=6&bnd=MW');
  sent = page.waitForRequest((r) => r.method() === 'POST' && new URL(r.url()).pathname === '/api/step-size');
  await page.getByRole('radiogroup', { name: 'Tuning step, SW' }).getByRole('radio', { name: '1 kHz' }).click();
  expect((await sent).postData()).toBe('khz=1&bnd=SW');
});

test('opening Settings reads the settings once, beside the sign-in check', async ({ page }) => {
  const reads = await countReads(page, '/api/settings');
  const live = await radio(page);
  await page.goto('/#settings');
  await expect(page.getByText('Squelch floor', { exact: true })).toBeVisible();
  // States that store nothing, as the radio sends them twice a second.
  for (let i = 0; i < 4; i++) live.push();
  await page.evaluate(() => {});
  expect(await reads()).toBe(2);
});

test('a refused read signs the page out, so the PIN can be given again', async ({ page }) => {
  let signed = true;
  const live = await radio(page);
  await page.route('**/api/settings', (route) => (signed ? route.fallback() : route.fulfill({ status: 403, body: 'Enter the access PIN first.' })));
  await page.goto('/#settings');
  const banner = page.getByRole('banner');
  await expect(banner.getByRole('button', { name: 'Signed in', exact: true })).toBeVisible();
  await expect(page.getByText('Squelch floor', { exact: true })).toBeVisible();
  // The session ends on the radio. The radio stores its settings, so the
  // page reads them again, and is refused.
  signed = false;
  live.push((s) => (s.asv.n += 1));
  await expect(banner.getByRole('button', { name: 'Sign in', exact: true })).toBeVisible();
  await page.getByRole('button', { name: 'Enter the PIN' }).click();
  await expect(page.getByRole('dialog').getByLabel('PIN', { exact: true })).toBeVisible();
});

test('settings that could not be read are said, and read again by themselves, 5 s on', async ({ page }) => {
  await page.clock.install();
  const live = await radio(page);
  let reads = 0;
  await page.route('**/api/settings', (route) => (route.request().method() === 'GET' && ++reads === 2 ? route.fulfill({ status: 503, body: '' }) : route.fallback()));
  await page.goto('/#settings');
  await expect(page.getByText('The settings could not be read. Trying again.')).toBeVisible();
  // The first state that comes 5 s on.
  await page.clock.runFor(5000);
  live.push();
  await expect(page.getByText('Squelch floor', { exact: true })).toBeVisible();
});
