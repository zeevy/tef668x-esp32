// First start: on its own hotspot the radio's page opens on setting up the
// Wi-Fi, sends it to /wifi with no PIN, says what to do next, and can be
// skipped for this tab.
import { expect, test } from '@playwright/test';
import { liveRadio } from './radio.js';

async function radio(page, net) {
  await liveRadio(page, (s) => Object.assign(s, { net, rssi: null, bars: null, ip: '192.168.4.1' }));
  await page.route('**/wifi', (route) => route.fulfill({ contentType: 'text/html', body: '<p>Saved. The radio is trying that network now.</p>' }));
}

test('on the hotspot the page opens on setting up the Wi-Fi, on any screen', async ({ page }) => {
  await radio(page, 'ap');
  await page.goto('/#presets');
  await expect(page.locator('main section h2')).toHaveText('Set up Wi-Fi');
  await page.getByLabel('Network name').fill('HomeNet');
  await page.getByLabel('Passphrase', { exact: true }).fill('a long passphrase');
  const sent = page.waitForRequest((r) => r.method() === 'POST' && new URL(r.url()).pathname === '/wifi');
  await page.getByRole('button', { name: 'Save and join' }).click();
  expect((await sent).postData()).toBe('sid=HomeNet&pwd=a+long+passphrase');
  await expect(page.getByText('The radio is joining HomeNet.')).toBeVisible();
  await expect(page.getByText('http://tef668x.local:8080')).toBeVisible();
});

test('it can be skipped, for this tab', async ({ page }) => {
  await radio(page, 'ap');
  await page.goto('/#presets');
  await page.getByRole('button', { name: 'Use the radio here, on its hotspot' }).click();
  await expect(page.getByText('Set up Wi-Fi')).toHaveCount(0);
  await page.reload();
  await expect(page.locator('main')).not.toContainText('Set up Wi-Fi');
});

test('on the home Wi-Fi it does not show', async ({ page }) => {
  await radio(page, 'station');
  await page.goto('/#presets');
  await expect(page.locator('main')).not.toContainText('Set up Wi-Fi');
});
