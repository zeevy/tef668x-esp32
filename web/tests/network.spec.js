// Network: the PIN, how the radio is connected, joining a Wi-Fi, the
// hotspot, the PC Link, a new PIN, and turning things off only when asked.
import fs from 'node:fs';
import { expect, test } from '@playwright/test';
import { liveRadio, nextPost } from './radio.js';

const stored = JSON.parse(fs.readFileSync(new URL('../mock/settings.json', import.meta.url)));

// The radio on the live socket, and its stored settings.
async function radio(page, { net = 'station', pcl = { on: false, cli: 0, ips: [], ref: 0 }, settings = {}, refuse = null, signed = true, dpn = true } = {}) {
  await liveRadio(page, (s) => Object.assign(s, { net, apn: net === 'ap' ? 'tef668x-setup-A1B2' : null, rssi: net === 'station' ? -58 : null, bars: net === 'station' ? 2 : null, ip: '192.168.1.40', pcl, dpn }));
  await page.route('**/api/settings', (route) => {
    if (route.request().method() === 'GET') {
      return signed ? route.fulfill({ json: { ...stored, sid: 'HomeNet', pss: true, hsp: 0, pcl: 0, ...settings } }) : route.fulfill({ status: 403, body: 'Enter the access PIN first.' });
    }
    return refuse ? route.fulfill({ status: 400, body: refuse }) : route.fulfill({ body: 'Saved and in use now.' });
  });
}

const card = (page, title) => page.locator('main section').filter({ has: page.locator('h2', { hasText: title }) });

test('the network settings need the PIN', async ({ page }) => {
  await radio(page, { signed: false });
  await page.goto('/#network');
  await expect(page.getByText('The network settings need the PIN.')).toBeVisible();
});

test('how the radio is connected', async ({ page }) => {
  // On the hotspot the page would open on setting up the Wi-Fi.
  await page.addInitScript(() => sessionStorage.setItem('tef668x-setup-skipped', '1'));
  for (const [net, words] of [['station', 'On HomeNet'], ['ap', 'On its own hotspot, tef668x-setup-A1B2'], ['joining', 'Joining HomeNet'], ['offline', 'Off the network']]) {
    await page.unrouteAll();
    await radio(page, { net });
    await page.goto(`/?${net}#network`);
    await expect(card(page, 'Connection')).toContainText(words);
  }
  await expect(card(page, 'Connection')).toContainText('address 192.168.1.40');
  await page.unrouteAll();
  await radio(page);
  await page.goto('/?bars#network');
  // -58 dBm lights the dot and three of the four arcs (above -80, -70 and
  // -60), in the tile left of the network's name; the outer arc stays faint.
  const bars = page.getByRole('img', { name: '4 of 5 bars, -58 dBm' });
  await expect(bars).toBeVisible();
  expect(await bars.locator('path').evaluateAll((ps) => ps.map((p) => p.getAttribute('opacity')))).toEqual([null, '1', '1', '1', '0.3']);
  const name = await card(page, 'Connection').getByText('On HomeNet').boundingBox();
  expect((await bars.boundingBox()).x + (await bars.boundingBox()).width).toBeLessThan(name.x);
  await expect(card(page, 'Connection')).toContainText('Default PIN');
  await expect(page.locator('main')).not.toContainText('000000');
});

test('join a Wi-Fi: the passphrase is always sent, and an empty one only for an open network', async ({ page }) => {
  await radio(page);
  await page.goto('/#network');
  await expect(page.getByText('Wi-Fi network')).toBeVisible();
  await page.getByRole('button', { name: 'Change' }).first().click();
  const wifi = page.getByRole('dialog');
  await expect(wifi.getByLabel('Network name')).toHaveValue('HomeNet');
  await wifi.getByLabel('Network name').fill('Cafe');
  await wifi.getByRole('button', { name: 'Save and join' }).click();
  await expect(page.getByText('Type the passphrase, or tick that the network has none.')).toBeVisible();
  let sent = nextPost(page, '/api/settings');
  await wifi.getByLabel('Passphrase', { exact: true }).fill('secret pass');
  await wifi.getByRole('button', { name: 'Save and join' }).click();
  expect(await sent).toBe('sid=Cafe&pwd=secret+pass');
  await expect(page.getByRole('dialog')).toHaveCount(0);
  await page.getByRole('button', { name: 'Change' }).first().click();
  await wifi.getByLabel('Network name').fill('Cafe');
  await wifi.getByRole('switch', { name: 'This network has no passphrase' }).click();
  await expect(wifi.getByLabel('Passphrase', { exact: true })).toHaveCount(0);
  sent = nextPost(page, '/api/settings');
  await wifi.getByRole('button', { name: 'Save and join' }).click();
  expect(await sent).toBe('sid=Cafe&pwd=');
});

test('the hotspot, and a refusal putting it back', async ({ page }) => {
  await radio(page);
  await page.goto('/#network');
  const sent = nextPost(page, '/api/settings');
  // On leaves the home network for the hotspot alone, so it asks first.
  await page.getByRole('radiogroup', { name: 'Hotspot' }).getByRole('radio', { name: 'On' }).click();
  const ask = page.getByRole('dialog');
  await expect(ask).toContainText('The radio leaves HomeNet and serves only its own hotspot');
  await expect(ask).toContainText('http://192.168.4.1:8080');
  await ask.getByRole('button', { name: 'Turn it on' }).click();
  expect(await sent).toBe('hsp=1');
  await page.unrouteAll();
  await radio(page, { refuse: 'Not now.' });
  await page.reload();
  await page.getByRole('radiogroup', { name: 'Hotspot' }).getByRole('radio', { name: 'Off' }).click();
  await expect(page.getByText('Not now.')).toBeVisible();
  await expect(page.getByRole('radiogroup', { name: 'Hotspot' }).getByRole('radio', { name: 'Auto' })).toHaveAttribute('aria-checked', 'true');
});

test('the PC Link, and the PCs connected', async ({ page }) => {
  await radio(page, { settings: { pcl: 1 }, pcl: { on: true, cli: 1, ips: ['192.168.1.20'], ref: 2 } });
  await page.goto('/#network');
  await expect(page.getByText('Connected: 192.168.1.20')).toBeVisible();
  await expect(card(page, 'Connection')).toContainText('PC Link on, 1 PC');
  const sent = nextPost(page, '/api/settings');
  await page.getByRole('switch', { name: 'PC Link' }).click();
  expect(await sent).toBe('pcl=0');
});

test('a new PIN: six digits, typed twice, then sign in with it', async ({ page }) => {
  await radio(page);
  await page.goto('/#network');
  await expect(page.getByText('Still the default PIN: anyone on your network can change the radio')).toBeVisible();
  await page.getByRole('button', { name: 'Change' }).nth(1).click();
  const pin = page.getByRole('dialog');
  await expect(pin).toContainText('The radio still uses the default PIN.');
  await pin.getByLabel('New PIN').fill('123456');
  await pin.getByLabel('Again').fill('123455');
  await pin.getByRole('button', { name: 'Change PIN' }).click();
  await expect(page.getByText('The two PINs are not the same.')).toBeVisible();
  await pin.getByLabel('Again').fill('12345a6');
  const sent = nextPost(page, '/api/settings');
  await pin.getByRole('button', { name: 'Change PIN' }).click();
  expect(await sent).toBe('pin=123456');
  // The new PIN signs this browser out too: the sign-in box opens, with its
  // PIN field, in place of the one for a new PIN.
  await expect(page.getByRole('dialog').getByLabel('PIN', { exact: true })).toBeVisible();
  await expect(page.getByLabel('New PIN')).toHaveCount(0);
});

test('turning Wi-Fi off asks first', async ({ page }) => {
  await radio(page);
  await page.goto('/#network');
  // The two turn offs are the last two rows: Wi-Fi, then the web server.
  const off = page.locator('main').getByRole('button', { name: 'Turn off' });
  let posts = 0;
  page.on('request', (r) => r.method() === 'POST' && posts++);
  await off.nth(0).click();
  await expect(page.getByRole('dialog')).toContainText('Turn off Wi-Fi?');
  await page.getByRole('dialog').getByRole('button', { name: 'Keep it on' }).click();
  await expect(page.getByRole('dialog')).toHaveCount(0);
  await off.nth(1).click();
  await expect(page.getByRole('dialog')).toContainText('Turn off the web server?');
  const sent = nextPost(page, '/api/settings');
  await page.getByRole('dialog').getByRole('button', { name: 'Turn off' }).click();
  expect(await sent).toBe('web=0');
  // Changes go to the radio one after another, so a Wi-Fi off sent at Keep
  // it on would have gone before this one.
  expect(posts).toBe(1);
});

test('on the hotspot, Auto or Off asks first, as the radio leaves it', async ({ page }) => {
  // On its hotspot the page opens on setting up Wi-Fi; skipped, as a person does.
  await page.addInitScript(() => sessionStorage.setItem('tef668x-setup-skipped', '1'));
  await radio(page, { net: 'ap', settings: { hsp: 1 } });
  await page.goto('/#network');
  const sent = nextPost(page, '/api/settings');
  await page.getByRole('radiogroup', { name: 'Hotspot' }).getByRole('radio', { name: 'Auto' }).click();
  const ask = page.getByRole('dialog');
  await expect(ask).toContainText('The radio leaves its hotspot and joins HomeNet');
  await ask.getByRole('button', { name: 'Set it to Auto' }).click();
  expect(await sent).toBe('hsp=0');
});

test('on the home network, Auto and Off change at once', async ({ page }) => {
  await radio(page, { settings: { hsp: 0 } });
  await page.goto('/#network');
  const sent = nextPost(page, '/api/settings');
  await page.getByRole('radiogroup', { name: 'Hotspot' }).getByRole('radio', { name: 'Off' }).click();
  expect(await sent).toBe('hsp=2');
  await expect(page.getByRole('dialog')).toHaveCount(0);
});

test('every item is a row with its control on the right', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'rows side by side from tablet width');
  await radio(page);
  await page.goto('/#network');
  const right = async (l) => {
    const b = await l.boundingBox();
    return Math.round(b.x + b.width);
  };
  const edge = await right(page.getByRole('radiogroup', { name: 'Hotspot' }));
  expect(await right(page.getByRole('switch', { name: 'PC Link' }))).toBe(edge);
  expect(await right(page.locator('main').getByRole('button', { name: 'Turn off' }).first())).toBe(edge);
});

test('a refused hotspot change puts back the hotspot alone, not a PC Link change made beside it', async ({ page }) => {
  await radio(page);
  // The hotspot change is refused, once the test has seen both drawn.
  let refuse;
  const refused = new Promise((done) => (refuse = done));
  await page.route('**/api/settings', async (route) => {
    if (route.request().method() === 'GET') return route.fallback();
    if (route.request().postData().startsWith('hsp')) {
      await refused;
      return route.fulfill({ status: 400, body: 'Not now.' });
    }
    return route.fulfill({ body: 'Saved and in use now.' });
  });
  await page.goto('/#network');
  const hotspot = page.getByRole('radiogroup', { name: 'Hotspot' });
  const pcl = page.getByRole('switch', { name: 'PC Link' });
  await hotspot.getByRole('radio', { name: 'Off' }).click();
  await pcl.click();
  // Both drawn at once, before the radio answers.
  await expect(hotspot.getByRole('radio', { name: 'Off' })).toHaveAttribute('aria-checked', 'true');
  await expect(pcl).toHaveAttribute('aria-checked', 'true');
  refuse();
  await expect(page.getByText('Not now.')).toBeVisible();
  await expect(hotspot.getByRole('radio', { name: 'Auto' })).toHaveAttribute('aria-checked', 'true');
  await expect(pcl).toHaveAttribute('aria-checked', 'true');
});
