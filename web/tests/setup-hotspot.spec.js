// The Set up Wi-Fi page says the hotspot is open, so the passphrase typed on
// it goes to the radio unencrypted.
import { expect, test } from '@playwright/test';
import { liveRadio } from './radio.js';

test('on the hotspot the setup page says the passphrase goes over it unencrypted', async ({ page }) => {
  await liveRadio(page, (s) => Object.assign(s, { net: 'ap', rssi: null, bars: null, ip: '192.168.4.1' }));
  await page.goto('/');
  await expect(page.locator('main section h2')).toHaveText('Set up Wi-Fi');
  await expect(page.getByText('This hotspot is open, so the passphrase you type here goes to the radio unencrypted.')).toBeVisible();
});
