// Presets: keys, recall, save the current station, add, change, rename,
// delete, move by dragging, download, upload and the station scan.
import { expect, test } from '@playwright/test';
import { countReads, liveRadio } from './radio.js';

const CSV = 'slot,band,frequency,bandwidth,name,pi\n1,FM,91100,0,ONE,3712\n2,FM,92700,0,,\n5,FM,98300,168,FIVE,26FF\n';

// The radio on the live socket, on preset 5, and its presets. Gives the
// socket's end, to push a changed state.
async function radio(page, { csv = CSV, khz = 106400, f = '106.40', ps = ' MAGIC  ' } = {}) {
  const live = await liveRadio(page, (s) => {
    Object.assign(s.tun, { bnd: 'FM', khz, f, unt: 'MHz', bws: 0, pst: 5 });
    s.tun.rds = { syn: true, ps };
  });
  await page.route('**/api/presets.csv', (route) => route.fulfill({ body: csv }));
  return live;
}

// The next POST to a route: its body, address and type.
function nextPost(page, path) {
  return page
    .waitForRequest((r) => r.method() === 'POST' && new URL(r.url()).pathname === path)
    .then((r) => ({ body: r.postData(), url: r.url(), type: r.headers()['content-type'] }));
}

const keys = (page) => page.locator('main [data-slot]');

test('a key shows its slot, band, frequency and name; the one on the dial is marked', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await expect(keys(page)).toHaveCount(3);
  await expect(keys(page).nth(0)).toContainText('P01');
  await expect(keys(page).nth(0)).toContainText('FM');
  await expect(keys(page).nth(0)).toContainText('91.10');
  await expect(keys(page).nth(0)).toContainText('ONE');
  await expect(keys(page).nth(2)).toHaveAttribute('aria-pressed', 'true');
});

test('a tap recalls', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  const sent = nextPost(page, '/api/presets');
  await keys(page).nth(1).click();
  expect((await sent).body).toBe('do=recall&slot=2');
});

// The radio's answers to a preset write, by what was asked.
function answer(page, reply) {
  return page.route('**/api/presets', (route) => (route.request().method() === 'POST' ? route.fulfill(reply) : route.fallback()));
}

test('save current station: the radio picks the slot, and its answer is said', async ({ page }) => {
  await radio(page, { khz: 101900, f: '101.90', ps: 'NEW FM  ' });
  await answer(page, { body: 'Preset 3 is FM 101.90 MHz\n' });
  await page.goto('/#presets');
  const sent = nextPost(page, '/api/presets');
  await page.getByRole('button', { name: 'Save current station' }).click();
  expect((await sent).body).toBe('do=add&khz=101900&bnd=FM&bw=0&name=NEW+FM');
  await expect(page.getByRole('status').filter({ hasText: 'Preset 3 is FM 101.90 MHz' })).toBeVisible();
});

test('save current station: the radio\'s own words where they fit, Save current on a phone', async ({ page }, testInfo) => {
  await radio(page);
  await page.goto('/#presets');
  const said = testInfo.project.name === 'phone' ? 'Save current' : 'Save current station';
  await expect(page.getByRole('button', { name: 'Save current station' })).toHaveText(said, { useInnerText: true });
});

test('save current station: a station already stored is refused by the radio, and said', async ({ page }) => {
  await radio(page, { khz: 98300, f: '98.30' });
  await answer(page, { status: 409, body: '98.30 MHz is already preset 5.' });
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Save current station' }).click();
  await expect(page.getByRole('status').filter({ hasText: '98.30 MHz is already preset 5.' })).toBeVisible();
});

test('save current station: works from a list that could not be read, since the radio picks the slot', async ({ page }) => {
  await radio(page, { khz: 101900, f: '101.90' });
  await page.route('**/api/presets.csv', (route) => route.fulfill({ status: 500, body: 'no' }));
  await answer(page, { body: 'Preset 6 is FM 101.90 MHz' });
  await page.goto('/#presets');
  await expect(page.getByText('The presets could not be read.')).toBeVisible();
  const sent = nextPost(page, '/api/presets');
  await page.getByRole('button', { name: 'Save current station' }).click();
  expect((await sent).body).toContain('do=add&khz=101900');
});

test('a list that could not be read is said, not shown as none; Add waits for it; Try again reads it', async ({ page }) => {
  await radio(page);
  let reads = 0;
  await page.route('**/api/presets.csv', (route) => (++reads === 1 ? route.fulfill({ status: 500, body: 'no' }) : route.fulfill({ body: CSV })));
  await page.goto('/#presets');
  await expect(page.getByText('The presets could not be read.')).toBeVisible();
  await expect(page.getByText('No presets stored yet.')).toHaveCount(0);
  await expect(page.getByRole('button', { name: 'Add' })).toBeDisabled();
  await page.getByRole('button', { name: 'Try again' }).click();
  await expect(keys(page)).toHaveCount(3);
  await expect(page.getByRole('button', { name: 'Add' })).toBeEnabled();
});

test('a list that could not be read is read again by itself, 5 s on', async ({ page }) => {
  await page.clock.install();
  const live = await radio(page);
  let reads = 0;
  await page.route('**/api/presets.csv', (route) => (++reads === 1 ? route.fulfill({ status: 503, body: '' }) : route.fulfill({ body: CSV })));
  await page.goto('/#presets');
  await expect(page.getByText('The presets could not be read. Trying again.')).toBeVisible();
  // The first state that comes 5 s on.
  await page.clock.runFor(5000);
  live.push();
  await expect(keys(page)).toHaveCount(3);
  expect(reads).toBe(2);
});

test('the list is read again when the radio stores a preset, so Add offers the next free slot', async ({ page }) => {
  await radio(page);
  // Slot 3 is stored from the radio's menu after the screen read the list:
  // the state's count of presets moves.
  let n = 3;
  const live = await liveRadio(page, (s) => {
    Object.assign(s.tun, { bnd: 'FM', khz: 106400, f: '106.40', unt: 'MHz', bws: 0, pst: 5 });
    s.pst.n = n;
  });
  await page.route('**/api/presets.csv', (route) => route.fulfill({ body: n === 3 ? CSV : `${CSV}3,FM,94100,0,MENU,\n` }));
  await page.goto('/#presets');
  await expect(keys(page)).toHaveCount(3);
  n = 4;
  live.push((s) => (s.pst.n = n));
  await expect(keys(page)).toHaveCount(4);
  await page.getByRole('button', { name: 'Add' }).click();
  await expect(page.getByRole('dialog').getByLabel('Slot')).toHaveValue('4');
});

test('a rename on the radio shows too: the count stays, the list\'s rev moves', async ({ page }) => {
  await radio(page);
  let rev = 1;
  const live = await liveRadio(page, (s) => {
    Object.assign(s.tun, { bnd: 'FM', khz: 106400, f: '106.40', unt: 'MHz', bws: 0, pst: 5 });
    s.pst = { ...s.pst, n: 3, rev };
  });
  await page.route('**/api/presets.csv', (route) => route.fulfill({ body: rev === 1 ? CSV : CSV.replace('FIVE', 'RENAMED') }));
  await page.goto('/#presets');
  await expect(keys(page).nth(2)).toContainText('FIVE');
  rev = 2;
  live.push((s) => (s.pst.rev = rev));
  await expect(keys(page).nth(2)).toContainText('RENAMED');
});

test('add: a typed frequency, a name and a width, in the slot offered, which the radio picks; its answer is said', async ({ page }) => {
  await radio(page);
  await answer(page, { body: 'Preset 3 is FM 94.30 MHz\n' });
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Add' }).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByLabel('Slot')).toHaveValue('3');
  await dialog.getByLabel('Frequency').fill('94.3');
  await expect(dialog).toContainText('FM, 94.30 MHz');
  await dialog.getByLabel('Name').fill('Café Radio');
  await expect(dialog.getByLabel('Name')).toHaveValue('Caf Radio');
  await dialog.getByLabel('Bandwidth').selectOption('114');
  const sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=add&khz=94300&bnd=FM&bw=114&name=Caf+Radio');
  await expect(dialog).toBeHidden();
  await expect(page.getByRole('status').filter({ hasText: 'Preset 3 is FM 94.30 MHz' })).toBeVisible();
});

test('add: a slot the person types is written to, even the one offered', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Add' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel('Slot').fill('3');
  await dialog.getByLabel('Frequency').fill('94.3');
  const sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=set&slot=3&khz=94300&bnd=FM&bw=0&name=');
});

test('add: the band from the radio\'s own edges, low band first', async ({ page }) => {
  await radio(page);
  // Medium wave on a 10 kHz spacing: 520 to 1710 kHz.
  await page.route('**/api/bands', (route) =>
    route.fulfill({
      json: [
        { band: 'LW', low: 144, high: 513, steps: [1, 9], step: 9, widths: [3, 4, 6, 8], width: 4 },
        { band: 'MW', low: 520, high: 1710, steps: [1, 10], step: 10, widths: [3, 4, 6, 8], width: 4 },
        { band: 'SW', low: 1700, high: 27000, steps: [1, 5], step: 5, widths: [3, 4, 6, 8], width: 4 },
        { band: 'OIRT', low: 65000, high: 74000, steps: [10, 30], step: 30, widths: [0], width: 0 },
        { band: 'FM', low: 65000, high: 108000, steps: [50, 100, 200], step: 100, widths: [0], width: 0 },
      ],
    }),
  );
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Add' }).click();
  const dialog = page.getByRole('dialog');
  for (const [typed, said] of [
    ['520', 'MW, 520 kHz'],
    ['1710', 'MW, 1710 kHz'],
    ['1711', 'SW, 1711 kHz'],
    ['68', 'OIRT, 68.00 MHz'],
    ['74.1', 'FM, 74.10 MHz'],
    ['30000', 'That frequency is in no band.'],
  ]) {
    await dialog.getByLabel('Frequency').fill(typed);
    await expect(dialog).toContainText(said);
  }
});

test('add: a frequency already a preset, and a taken slot, are said', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Add' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel('Slot').fill('5');
  await expect(dialog).toContainText('Slot 5 holds 98.30 MHz; saving replaces it.');
  await dialog.getByLabel('Frequency').fill('91.1');
  await dialog.getByRole('button', { name: 'Save' }).click();
  await expect(dialog).toContainText('91.10 MHz is already preset 1.');
});

test('change: rename, and delete', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  await page.getByRole('button', { name: 'Change preset 5' }).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByLabel('Frequency')).toHaveValue('98.30');
  await expect(dialog.getByLabel('Name')).toHaveValue('FIVE');
  await expect(dialog.getByLabel('Bandwidth')).toHaveValue('168');
  await dialog.getByLabel('Name').fill('RENAMED');
  let sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=set&slot=5&khz=98300&bnd=FM&bw=168&name=RENAMED');
  await page.getByRole('button', { name: 'Change preset 1' }).click();
  sent = nextPost(page, '/api/presets');
  await page.getByRole('dialog').getByRole('button', { name: 'Delete' }).click();
  expect((await sent).body).toBe('do=clear&slot=1');
});

test('on a phone a stored preset\'s dialog has Delete and Cancel side by side, and Save across the row under them', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'phone', 'a phone only');
  const csv = 'slot,band,frequency,bandwidth,name,pi,pty,ptn,ecc,country,saved\n5,FM,98300,168,FIVE,,,,,,\n';
  await radio(page, { csv });
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  await page.getByRole('button', { name: 'Change preset 5' }).click();
  const dialog = page.getByRole('dialog');
  const box = (name) => dialog.getByRole('button', { name, exact: true }).boundingBox();
  const [del, cancel, save] = [await box('Delete'), await box('Cancel'), await box('Save')];
  expect(Math.round(del.y)).toBe(Math.round(cancel.y));
  expect(Math.round(del.width)).toBe(Math.round(cancel.width));
  expect(save.y).toBeGreaterThan(del.y);
  expect(Math.round(save.width)).toBe(Math.round(cancel.x + cancel.width - del.x));
});

test('change: the dialog shows what the radio learnt about the station, and nothing for a preset with none', async ({ page }) => {
  const csv = 'slot,band,frequency,bandwidth,name,pi,pty,ptn,ecc,country,saved\n1,FM,91100,0,ONE,,,,,,\n5,FM,98300,168,FIVE,26FF,10,Pop Music,E2,IN,2026-10-10 08:03\n';
  await radio(page, { csv });
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  await page.getByRole('button', { name: 'Change preset 5' }).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog).toContainText('26FF');
  await expect(dialog).toContainText('Pop Music');
  await expect(dialog).toContainText('India');
  await expect(dialog).toContainText('Saved');
  await dialog.getByRole('button', { name: 'Cancel' }).click();
  await page.getByRole('button', { name: 'Change preset 1' }).click();
  await expect(dialog).not.toContainText('Programme type');
  await expect(dialog).not.toContainText('Saved');
});

test('change mode: a drag onto another key, and onto an empty slot, moves', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  // Slots 1 to 6: three presets, the gaps 3 and 4, and 6 after the last.
  await expect(keys(page)).toHaveCount(6);
  const drag = async (from, to) => {
    const a = await keys(page).nth(from - 1).boundingBox();
    const b = await keys(page).nth(to - 1).boundingBox();
    await page.mouse.move(a.x + a.width / 2, a.y + a.height / 2);
    await page.mouse.down();
    await page.mouse.move(a.x + a.width / 2 + 20, a.y + a.height / 2, { steps: 3 });
    await page.mouse.move(b.x + b.width / 2, b.y + b.height / 2, { steps: 5 });
    await page.mouse.up();
  };
  let sent = nextPost(page, '/api/presets');
  // After a move the list is read again; the next drag waits for it.
  const reread = page.waitForResponse('**/api/presets.csv');
  await drag(2, 5);
  expect((await sent).body).toBe('do=move&slot=2&to=5');
  await reread;
  sent = nextPost(page, '/api/presets');
  await drag(1, 4);
  expect((await sent).body).toBe('do=move&slot=1&to=4');
});

test('after a drag in change mode and Done, a tap recalls again', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  const a = await keys(page).nth(1).boundingBox();
  const b = await keys(page).nth(4).boundingBox();
  const moved = nextPost(page, '/api/presets');
  await page.mouse.move(a.x + a.width / 2, a.y + a.height / 2);
  await page.mouse.down();
  await page.mouse.move(b.x + b.width / 2, b.y + b.height / 2, { steps: 5 });
  await page.mouse.up();
  expect((await moved).body).toBe('do=move&slot=2&to=5');
  await page.getByRole('button', { name: 'Done' }).click();
  const sent = nextPost(page, '/api/presets');
  await keys(page).first().click();
  expect((await sent).body).toBe('do=recall&slot=1');
});

test('change mode: a tap on an empty slot adds into it', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  await page.getByRole('button', { name: 'Add a preset in slot 4' }).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByLabel('Slot')).toHaveValue('4');
  await dialog.getByLabel('Frequency').fill('94.3');
  const sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=set&slot=4&khz=94300&bnd=FM&bw=0&name=');
});

// The band plan with the FM region Full: FM from 65 MHz, over all of OIRT.
const FULL = [
  { band: 'LW', low: 144, high: 513, steps: [1, 9], step: 9, widths: [3, 4, 6, 8], width: 4 },
  { band: 'MW', low: 522, high: 1791, steps: [1, 9], step: 9, widths: [3, 4, 6, 8], width: 4 },
  { band: 'SW', low: 1700, high: 27000, steps: [1, 5], step: 5, widths: [3, 4, 6, 8], width: 4 },
  { band: 'OIRT', low: 65000, high: 74000, steps: [10, 30], step: 30, widths: [0, 56, 114], width: 0 },
  { band: 'FM', low: 65000, high: 108000, steps: [50, 100, 200], step: 100, widths: [0, 56, 114], width: 0 },
];

test('a preset keeps its band: 68 MHz on FM is saved as FM; a frequency typed goes on the band the dialog shows', async ({ page }) => {
  await radio(page, { csv: `${CSV}7,FM,68000,0,LOW,\n` });
  await page.route('**/api/bands', (route) => route.fulfill({ json: FULL }));
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  await page.getByRole('button', { name: 'Change preset 7' }).click();
  let dialog = page.getByRole('dialog');
  await expect(dialog).toContainText('FM, 68.00 MHz');
  await dialog.getByLabel('Name').fill('LOW FM');
  let sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=set&slot=7&khz=68000&bnd=FM&bw=0&name=LOW+FM');
  await page.getByRole('button', { name: 'Done' }).click();
  await page.getByRole('button', { name: 'Add' }).click();
  dialog = page.getByRole('dialog');
  await dialog.getByLabel('Frequency').fill('68');
  await expect(dialog).toContainText('OIRT, 68.00 MHz');
  sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=add&khz=68000&bnd=OIRT&bw=0&name=');
});

test('change: a rename while the band plan is not known keeps the preset\'s own width', async ({ page }) => {
  await radio(page);
  await page.route('**/api/bands', (route) => route.fulfill({ status: 503, body: '' }));
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  await page.getByRole('button', { name: 'Change preset 5' }).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByLabel('Bandwidth')).toHaveValue('168');
  await dialog.getByLabel('Name').fill('RENAMED');
  const sent = nextPost(page, '/api/presets');
  await dialog.getByRole('button', { name: 'Save' }).click();
  expect((await sent).body).toBe('do=set&slot=5&khz=98300&bnd=FM&bw=168&name=RENAMED');
});

test('the radio\'s own word on its list: a save to flash that failed, and presets left out at start up', async ({ page }) => {
  const live = await radio(page);
  await page.goto('/#presets');
  await expect(keys(page)).toHaveCount(3);
  await expect(page.getByText('could not')).toHaveCount(0);
  live.push((s) => Object.assign(s.pst, { bad: true, lost: 2 }));
  await expect(page.getByText('The radio could not save the preset list. A change may be lost when it is turned off.')).toBeVisible();
  await expect(page.getByText('2 presets could not be read at start up and were left out.')).toBeVisible();
  live.push((s) => Object.assign(s.pst, { bad: false, lost: 1 }));
  await expect(page.getByText('1 preset could not be read at start up and was left out.')).toBeVisible();
  await expect(page.getByText('could not save')).toHaveCount(0);
});

test('after the page\'s own change the list is read once, not again for the state that follows it', async ({ page }) => {
  const reads = await countReads(page, '/api/presets.csv');
  const live = await radio(page);
  await answer(page, { body: 'Preset 3 is FM 106.40 MHz\n' });
  await page.goto('/#presets');
  await expect(keys(page)).toHaveCount(3);
  expect(await reads()).toBe(1);
  // Read once the radio took it.
  await page.getByRole('button', { name: 'Save current station' }).click();
  await expect.poll(reads).toBe(2);
  // The radio's next state carries the list's new count: that read holds it.
  // The new frequency shows that the page has taken the state.
  live.push((s) => {
    s.pst.n += 1;
    Object.assign(s.tun, { khz: 101900, f: '101.90' });
  });
  await expect(top(page)).toContainText('101.90');
  expect(await reads()).toBe(2);
  // A change that moves nothing, as one the radio refused: the next state
  // keeps the count, and a later move, as from the radio's own menu, is read.
  await page.getByRole('button', { name: 'Save current station' }).click();
  await expect.poll(reads).toBe(3);
  live.push((s) => Object.assign(s.tun, { khz: 102000, f: '102.00' }));
  await expect(top(page)).toContainText('102.00');
  live.push((s) => (s.pst.n += 1));
  await expect.poll(reads).toBe(4);
});

test('download is the CSV file', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  const link = page.getByRole('link', { name: 'Download' });
  await expect(link).toHaveAttribute('href', '/api/presets.csv');
  await expect(link).toHaveAttribute('download', 'presets.csv');
});

test('upload: a file over 16 KB is refused in plain words, and nothing is sent', async ({ page }) => {
  await radio(page);
  let posts = 0;
  page.on('request', (r) => r.method() === 'POST' && posts++);
  await page.goto('/#presets');
  await page.locator('input[type=file]').setInputFiles({ name: 'big.csv', mimeType: 'text/csv', buffer: Buffer.alloc(16385, 'a') });
  await expect(page.getByRole('status').filter({ hasText: 'big.csv is 17 KB; the radio takes a preset file of up to 16 KB.' })).toBeVisible();
  await expect(page.getByRole('dialog')).toHaveCount(0);
  expect(posts).toBe(0);
});

test('upload: an empty file is refused in plain words, and nothing is sent', async ({ page }) => {
  await radio(page);
  let posts = 0;
  page.on('request', (r) => r.method() === 'POST' && posts++);
  await page.goto('/#presets');
  await page.locator('input[type=file]').setInputFiles({ name: 'empty.csv', mimeType: 'text/csv', buffer: Buffer.alloc(0) });
  await expect(page.getByRole('status').filter({ hasText: 'That file is empty.' })).toBeVisible();
  await expect(page.getByRole('dialog')).toHaveCount(0);
  expect(posts).toBe(0);
});

test('upload: merge or replace, the file sent as it is', async ({ page }) => {
  await radio(page);
  await page.route('**/api/presets/import**', (route) => route.fulfill({ body: '3 of 3 lines imported, 0 refused, 0 already taken, 0 names cut short' }));
  await page.goto('/#presets');
  await page.locator('input[type=file]').setInputFiles({ name: 'mine.csv', mimeType: 'text/csv', buffer: Buffer.from(CSV) });
  const dialog = page.getByRole('dialog');
  await expect(dialog).toContainText('Upload mine.csv');
  const sent = nextPost(page, '/api/presets/import');
  await dialog.getByRole('button', { name: 'Merge' }).click();
  const req = await sent;
  expect(req.url).toContain('mode=merge');
  expect(req.type).toContain('text/csv');
  expect(req.body).toBe(CSV);
  await expect(page.getByRole('status').filter({ hasText: '3 of 3 lines imported' })).toBeVisible();
});

test('the four buttons fit on one line each, down to 320 px', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'phone', 'a phone only');
  await page.setViewportSize({ width: 320, height: 700 });
  await radio(page);
  await page.goto('/#presets');
  const spill = await page
    .locator('main section')
    .first()
    // The four tools; the frequency is a button too, and tall on purpose.
    .locator('button:not([title="Tap to type a frequency"]), a, label')
    .evaluateAll((els) => els.filter((e) => e.scrollWidth > e.clientWidth || e.getBoundingClientRect().height > 45).map((e) => e.textContent));
  expect(spill).toEqual([]);
});

const MIXED = `${CSV}12,MW,738,4,AIR,\n20,SW,9650,4,,\n`;
const top = (page) => page.locator('main section').first();

test('the top card: the station on now with its P number and name, and how many slots are used', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await expect(top(page)).toContainText('106.40');
  await expect(top(page).getByText('P05', { exact: true })).toBeVisible();
  // The station's own name first, as on Home and Radio; the preset's when it
  // sends none.
  await expect(top(page)).toContainText('MAGIC');
  await expect(top(page)).toContainText('3 of 99 slots used');
});

test('the top card says Not a preset, with the station name, off the presets', async ({ page }) => {
  await liveRadio(page, (s) => {
    Object.assign(s.tun, { bnd: 'FM', khz: 106400, f: '106.40', unt: 'MHz', pst: 0 });
    s.tun.rds = { syn: true, ps: ' MAGIC  ' };
  });
  await page.route('**/api/presets.csv', (route) => route.fulfill({ body: CSV }));
  await page.goto('/#presets');
  await expect(top(page).getByText('Not a preset', { exact: true })).toBeVisible();
  await expect(top(page)).toContainText('MAGIC');
});

test('the band filter: only the bands that hold presets, and it shows that band alone', async ({ page }) => {
  await radio(page, { csv: MIXED });
  await page.goto('/#presets');
  const filter = page.getByRole('radiogroup', { name: 'Band', exact: true });
  await expect(filter.getByRole('radio')).toHaveText(['All', 'FM', 'MW', 'SW']);
  await expect(keys(page)).toHaveCount(5);
  await filter.getByRole('radio', { name: 'MW' }).click();
  await expect(keys(page)).toHaveCount(1);
  await expect(keys(page).first()).toContainText('738');
  await expect(page.locator('main section').filter({ hasText: 'on MW' })).toBeVisible();
  // Change mode shows every slot, whatever the filter.
  await page.getByRole('button', { name: 'Change' }).click();
  await expect(keys(page).filter({ hasText: '98.30' })).toHaveCount(1);
});

test('no band filter when every preset is on one band', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await expect(keys(page)).toHaveCount(3);
  await expect(page.getByRole('radiogroup', { name: 'Band', exact: true })).toHaveCount(0);
});

test('the top card shows the preset name when the station sends none', async ({ page }) => {
  await radio(page, { ps: '' });
  await page.goto('/#presets');
  await expect(page.locator('main section').first()).toContainText('FIVE');
});

test('change: another slot moves the preset there, by keys too; onto a taken slot the two swap, and the dialog says so', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Change' }).click();
  const posts = [];
  page.on('request', (r) => r.method() === 'POST' && new URL(r.url()).pathname === '/api/presets' && posts.push(r.postData()));
  await page.getByRole('button', { name: 'Change preset 5' }).click();
  let dialog = page.getByRole('dialog');
  await dialog.getByLabel('Slot').fill('7');
  await expect(dialog).not.toContainText('holds');
  await dialog.getByLabel('Slot').press('Enter');
  await expect.poll(() => posts).toEqual(['do=move&slot=5&to=7', 'do=set&slot=7&khz=98300&bnd=FM&bw=168&name=FIVE']);
  await expect(dialog).toHaveCount(0);
  posts.length = 0;
  await page.getByRole('button', { name: 'Change preset 1' }).click();
  dialog = page.getByRole('dialog');
  await dialog.getByLabel('Slot').fill('2');
  await expect(dialog).toContainText('Slot 2 holds 92.70 MHz; saving swaps the two, so that one goes to slot 1.');
  await dialog.getByRole('button', { name: 'Save' }).click();
  await expect.poll(() => posts).toEqual(['do=move&slot=1&to=2', 'do=set&slot=2&khz=91100&bnd=FM&bw=0&name=ONE']);
});

const scanCard = (page) => page.locator('main section').filter({ hasText: 'Station scan' });

test('station scan: Scan sends the band the radio is on, or the one picked', async ({ page }) => {
  await radio(page);
  await page.goto('/#presets');
  let sent = nextPost(page, '/api/scan');
  await page.getByRole('button', { name: 'Scan', exact: true }).click();
  expect((await sent).body).toBe('bnd=FM');
  await page.getByRole('radiogroup', { name: 'Band to scan' }).getByRole('radio', { name: 'MW' }).click();
  sent = nextPost(page, '/api/scan');
  await page.getByRole('button', { name: 'Scan', exact: true }).click();
  expect((await sent).body).toBe('bnd=MW');
});

test('station scan: a refusal is said in the radio\'s own words', async ({ page }) => {
  await radio(page);
  await page.route('**/api/scan', (route) => route.fulfill({ status: 409, body: 'The DX scanner is running. Stop it first.' }));
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Scan', exact: true }).click();
  await expect(page.getByRole('status').filter({ hasText: 'The DX scanner is running. Stop it first.' })).toBeVisible();
});

test('station scan: one started on the radio shows its progress, then what it found; the card keeps its height', async ({ page }) => {
  const live = await radio(page);
  await page.goto('/#presets');
  const card = scanCard(page);
  await expect(card).toContainText('Saves new stations in empty slots only.');
  const height = (await card.boundingBox()).height;
  live.push((s) => Object.assign(s.tun, { scn: true, scd: 103, sct: 206 }));
  await expect(card).toContainText('103 of 206 channels');
  await expect(card).toContainText('Scanning');
  await expect(card.getByRole('progressbar', { name: 'Scan progress' })).toHaveAttribute('aria-valuenow', '50');
  expect((await card.boundingBox()).height).toBe(height);
  live.push((s) => Object.assign(s.tun, { scn: false, scd: 0, sct: 0, scf: 23, sca: 5, scr: 2, scc: true }));
  await expect(card).toContainText('Found 23, saved 5. Full, 2 not saved.');
  await expect(card).toContainText('Complete');
  await expect(card).not.toContainText('Scanning');
  expect((await card.boundingBox()).height).toBe(height);
  // Stopped before the end, as by a tune.
  live.push((s) => Object.assign(s.tun, { scf: 4, sca: 1, scr: 0, scc: false }));
  await expect(card).toContainText('Found 4, saved 1.');
  await expect(card).toContainText('Stopped');
});

test('station scan: the presets it saves show as it saves them', async ({ page }) => {
  let n = 3;
  const live = await radio(page);
  await page.route('**/api/presets.csv', (route) => route.fulfill({ body: n === 3 ? CSV : `${CSV}3,FM,94100,0,,\n` }));
  await page.goto('/#presets');
  await expect(keys(page)).toHaveCount(3);
  n = 4;
  live.push((s) => {
    Object.assign(s.tun, { scn: true, scd: 40, sct: 206 });
    s.pst.n = n;
  });
  await expect(keys(page)).toHaveCount(4);
  await expect(page.locator('main [data-slot="3"]')).toContainText('94.10');
});
