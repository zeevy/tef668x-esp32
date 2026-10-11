// The frame: the way between screens, the header's status, the PIN prompt,
// the theme, and that nothing reloads the page.
import { expect, test } from '@playwright/test';
import { countReads, stopClock } from './radio.js';

const phone = (testInfo) => testInfo.project.name === 'phone';

test('shows the frequency, not the station, and a tap opens Radio', async ({ page }, testInfo) => {
  await page.goto('/');
  await expect(page.getByRole('heading', { level: 1 })).toHaveText('Home');
  const dial = page.getByRole('link', { name: /106\.40/ });
  await expect(dial).toContainText('MHz');
  await expect(dial).not.toContainText('MAGIC');
  await dial.click();
  await expect(page).toHaveURL(/#radio$/);
  await expect(page.getByRole('heading', { level: 1 })).toHaveText('Radio');
});

test('moves between screens without reloading the page', async ({ page }, testInfo) => {
  await page.goto('/');
  await page.evaluate(() => (window.notReloaded = true));
  const go = async (name) => {
    if (phone(testInfo) && !['Home', 'Radio', 'Presets & Log', 'DX'].includes(name)) {
      await page.getByRole('button', { name: 'More', exact: true }).click();
    }
    await page.getByRole('link', { name, exact: true }).click();
    await expect(page.getByRole('heading', { level: 1 })).toHaveText(name);
  };
  for (const name of ['Radio', 'Presets & Log', 'DX', 'Band scope', 'Settings', 'Network', 'System', 'Home']) {
    await go(name);
  }
  expect(await page.evaluate(() => window.notReloaded)).toBe(true);
});

test('a phone has four tabs and More; a laptop a menu across the top', async ({ page }, testInfo) => {
  await page.goto('/#settings');
  if (phone(testInfo)) {
    for (const name of ['Home', 'Radio', 'Presets & Log', 'DX']) {
      await expect(page.getByRole('link', { name, exact: true })).toBeVisible();
    }
    await expect(page.getByRole('link', { name: 'Settings' })).toHaveCount(0);
    await page.getByRole('button', { name: 'More', exact: true }).click();
    const sheet = page.getByRole('dialog');
    await expect(sheet.getByRole('link')).toHaveText(['Band scope', 'Settings', 'Network', 'System']);
    await page.keyboard.press('Escape');
    await expect(sheet).toBeHidden();
  } else {
    await expect(page.locator('nav').getByRole('link')).toHaveCount(8);
    await expect(page.getByRole('button', { name: 'More', exact: true })).toBeHidden();
  }
});

test('the battery is the last button in the header, in per cent or, at a tap, in volts; none on a radio without one', async ({ page }) => {
  let bat = { fit: true, mv: null, boot: 3724 };
  await page.route('**/api/state', async (route) => {
    const s = await (await route.fetch()).json();
    return route.fulfill({ json: { ...s, bat } });
  });
  await page.goto('/');
  const battery = page.getByRole('banner').getByRole('button').last();
  await expect(battery).toHaveAccessibleName('Battery 60 per cent, 3.72 volts, read when the radio started');
  await expect(battery).toHaveText('60%');
  await battery.click();
  await expect(battery).toHaveText('3.7');
  bat = { fit: false, mv: null, boot: 0 };
  await page.reload();
  await expect(page.getByRole('banner').getByRole('button', { name: /Sign(ed)? in/ })).toBeVisible();
  await expect(page.getByRole('button', { name: /^Battery/ })).toHaveCount(0);
});

test('an unknown address shows Home', async ({ page }) => {
  await page.goto('/#nothing-here');
  await expect(page.getByRole('heading', { level: 1 })).toHaveText('Home');
});

// These run on the page's polling, as the dev server turns the live socket
// away, and on a stopped clock, so each 5 s poll comes only as the test
// moves the clock on. The header shows the dial once the first state is in,
// which is when the next poll is set for 5 s on.
const firstState = (page) => expect(page.getByRole('link', { name: /106\.40/ })).toBeVisible();

test('says when the radio stops answering, and when it is back', async ({ page }) => {
  await stopClock(page);
  let down = false;
  await page.route('**/api/state', (route) => (down ? route.abort() : route.fallback()));
  await page.goto('/');
  // Nothing is shown while the link is well.
  await firstState(page);
  await expect(page.getByTitle('Live')).toHaveCount(0);
  await expect(page.getByTitle('No answer')).toHaveCount(0);
  down = true;
  await page.clock.runFor(5000);
  await expect(page.getByText('The radio is not answering')).toBeVisible();
  await expect(page.getByTitle('No answer')).toBeVisible();
  down = false;
  await page.clock.runFor(5000);
  await expect(page.getByText('The radio is not answering')).toBeHidden();
  await expect(page.getByTitle('No answer')).toHaveCount(0);
});

test('says Slow when an answer takes over 3 seconds', async ({ page }) => {
  await stopClock(page);
  let hold = false;
  let answer;
  await page.route('**/api/state', (route) => (hold ? (answer = () => route.fallback()) : route.fallback()));
  await page.goto('/');
  await firstState(page);
  await expect(page.getByTitle('Slow')).toHaveCount(0);
  hold = true;
  await page.clock.runFor(5000);
  await expect.poll(() => answer).toBeTruthy();
  // The read is answered 3.5 s after it was asked.
  await page.clock.runFor(3500);
  await answer();
  await expect(page.getByTitle('Slow')).toBeVisible();
});

test("reads the state every 5 seconds, so the radio's screen keeps up", async ({ page }) => {
  await stopClock(page);
  const reads = await countReads(page);
  await page.goto('/');
  await firstState(page);
  expect(await reads()).toBe(1);
  await page.clock.runFor(4999);
  expect(await reads()).toBe(1);
  await page.clock.runFor(1);
  expect(await reads()).toBe(2);
});

test('asks nothing of the radio while the tab is hidden', async ({ page }) => {
  await stopClock(page);
  const reads = await countReads(page);
  await page.goto('/');
  await firstState(page);
  await page.evaluate(() => {
    Object.defineProperty(document, 'hidden', { configurable: true, get: () => true });
    document.dispatchEvent(new Event('visibilitychange'));
  });
  await page.clock.runFor(6500);
  expect(await reads()).toBe(1);
  await page.evaluate(() => {
    Object.defineProperty(document, 'hidden', { configurable: true, get: () => false });
    document.dispatchEvent(new Event('visibilitychange'));
  });
  expect(await reads()).toBe(2);
});

test.describe('the PIN', () => {
  test.beforeEach(async ({ page }) => {
    await page.route('**/api/settings', (route) => route.fulfill({ status: 403, body: 'Sign in first' }));
  });

  test('signs in with six digits', async ({ page }) => {
    await page.route('**/auth', (route) => route.fulfill({ status: 200, body: '' }));
    await page.goto('/');
    await page.getByRole('button', { name: 'Sign in' }).click();
    const dialog = page.getByRole('dialog');
    const signIn = dialog.getByRole('button', { name: 'Sign in' });
    await expect(signIn).toBeDisabled();
    await dialog.getByLabel('PIN').fill('12a345');
    await expect(dialog.getByLabel('PIN')).toHaveValue('12345');
    await expect(signIn).toBeDisabled();
    await dialog.getByLabel('PIN').fill('123456');
    await signIn.click();
    await expect(dialog).toBeHidden();
    await expect(page.getByRole('button', { name: 'Signed in' })).toBeVisible();
  });

  test('says a wrong PIN, and too many tries with the wait', async ({ page }) => {
    let status = 403;
    await page.route('**/auth', (route) =>
      route.fulfill({ status, headers: status === 429 ? { 'Retry-After': '30' } : {}, body: '<html>' }),
    );
    await page.goto('/');
    await page.getByRole('button', { name: 'Sign in' }).click();
    const dialog = page.getByRole('dialog');
    await dialog.getByLabel('PIN').fill('111111');
    await dialog.getByRole('button', { name: 'Sign in' }).click();
    await expect(dialog.getByText('That PIN is not right.')).toBeVisible();
    status = 429;
    await dialog.getByRole('button', { name: 'Sign in' }).click();
    await expect(dialog.getByText('Too many tries. Wait 30 seconds.')).toBeVisible();
    await dialog.getByRole('button', { name: 'Not now' }).click();
    await expect(dialog).toBeHidden();
    await expect(page.getByRole('button', { name: 'Sign in' })).toBeVisible();
  });

  test('the PIN shows as dots, and the digits typed go when the box closes', async ({ page }) => {
    await page.goto('/');
    const dialog = page.getByRole('dialog');
    const pin = dialog.getByLabel('PIN');
    const open = () => page.getByRole('button', { name: 'Sign in' }).click();
    await open();
    await expect(pin).toHaveAttribute('type', 'password');
    await expect(pin).toHaveAttribute('inputmode', 'numeric');
    const ways = [
      ['Not now', () => dialog.getByRole('button', { name: 'Not now' }).click()],
      ['Escape', () => page.keyboard.press('Escape')],
      ['a tap outside', () => page.mouse.click(4, 4)],
    ];
    for (const [how, shut] of ways) {
      await pin.fill('123');
      await shut();
      await expect(dialog, how).toBeHidden();
      await open();
      await expect(pin, how).toHaveValue('');
    }
  });

  test('a change the radio refuses for the PIN opens the prompt', async ({ page }) => {
    await page.route('**/api/mute', (route) => route.fulfill({ status: 403, body: 'Sign in first' }));
    await page.goto('/');
    const refused = await page.evaluate(() =>
      import('/src/data/radio.js').then((m) => m.post('/api/mute', { on: 1 }).then(() => '', (e) => e.message)),
    );
    expect(refused).toBe('Sign in first');
    await expect(page.getByRole('dialog').getByLabel('PIN')).toBeVisible();
  });
});

for (const device of ['light', 'dark']) {
  test(`the theme follows a ${device} device until the sun or moon is pressed, and the pick is kept`, async ({ browser }) => {
    const context = await browser.newContext({ colorScheme: device });
    const page = await context.newPage();
    await page.goto('/');
    const root = page.locator('html');
    const other = device === 'dark' ? 'light' : 'dark';
    await expect(root).toHaveAttribute('data-theme', device);
    await page.getByRole('button', { name: device === 'dark' ? 'Light theme' : 'Dark theme' }).click();
    await expect(root).toHaveAttribute('data-theme', other);
    await page.reload();
    await expect(root).toHaveAttribute('data-theme', other);
    await context.close();
  });
}

test('with nothing picked, the theme changes when the device does', async ({ page }) => {
  await page.emulateMedia({ colorScheme: 'light' });
  await page.goto('/');
  const root = page.locator('html');
  await expect(root).toHaveAttribute('data-theme', 'light');
  await page.emulateMedia({ colorScheme: 'dark' });
  await expect(root).toHaveAttribute('data-theme', 'dark');
});

test('no screen scrolls sideways', async ({ page }) => {
  for (const id of ['', 'radio', 'presets', 'dx', 'scope', 'log', 'settings', 'network', 'system']) {
    await page.goto(`/#${id}`);
    await page.waitForLoadState('networkidle');
    const [scroll, width] = await page.evaluate(() => [document.documentElement.scrollWidth, innerWidth]);
    expect(scroll, `#${id}`).toBeLessThanOrEqual(width);
  }
});

test('on a laptop the menu runs across the top and the page keeps one width, centred', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'a laptop only');
  await page.setViewportSize({ width: 1600, height: 900 });
  await page.goto('/#radio');
  await expect(page.locator('aside')).toHaveCount(0);
  const menu = page.locator('header + nav');
  await expect(menu.getByRole('link')).toHaveCount(8);
  // The open screen is marked, and drawn so: its underline has a colour,
  // the others' none.
  const line = (name) => menu.getByRole('link', { name, exact: true }).evaluate((e) => getComputedStyle(e).borderBottomColor);
  await expect(menu.getByRole('link', { name: 'Radio' })).toHaveAttribute('aria-current', 'page');
  expect(await line('Home')).toBe('rgba(0, 0, 0, 0)');
  expect(await line('Radio')).not.toBe('rgba(0, 0, 0, 0)');
  const main = await page.locator('main').boundingBox();
  expect(main.width).toBe(1024);
  expect(main.x).toBe((1600 - 1024) / 2);
});

test('on a tablet every name in the top menu fits without scrolling', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'a laptop only');
  await page.setViewportSize({ width: 768, height: 1024 });
  await page.goto('/');
  const row = page.locator('header + nav > div');
  const [scroll, width] = await row.evaluate((e) => [e.scrollWidth, e.clientWidth]);
  expect(scroll).toBeLessThanOrEqual(width);
});

test('the footer links to the firmware\'s source on GitHub, in a new tab', async ({ page }) => {
  await page.goto('/');
  const link = page.locator('footer').getByRole('link', { name: 'Source on GitHub' });
  await expect(link).toHaveAttribute('href', 'https://github.com/zeevy/tef668x-esp32');
  await expect(link).toHaveAttribute('target', '_blank');
});

test.describe('motion, with the device not asking for less', () => {
  test.use({ reducedMotion: 'no-preference' });

  test('a change of screen plays as a view transition', async ({ page }, testInfo) => {
    test.skip(testInfo.project.name !== 'laptop', 'the menu across the top');
    await page.goto('/');
    await page.evaluate(() => {
      window.transitions = 0;
      const start = document.startViewTransition.bind(document);
      document.startViewTransition = (cb) => (window.transitions++, start(cb));
    });
    await page.locator('header + nav').getByRole('link', { name: 'Radio', exact: true }).click();
    await expect(page.getByRole('radiogroup', { name: 'Band' })).toBeVisible();
    expect(await page.evaluate(() => window.transitions)).toBe(1);
  });

  test('the highlight of a choice slides to the new one', async ({ page }) => {
    await page.goto('/#radio');
    const group = page.getByRole('radiogroup', { name: 'Band' });
    const thumb = group.locator('span[aria-hidden="true"]');
    await expect(thumb).toHaveCSS('transition-property', 'left, width');
    const fm = await group.getByRole('radio', { name: 'FM' }).boundingBox();
    expect(Math.abs((await thumb.boundingBox()).x - fm.x)).toBeLessThanOrEqual(1);
  });

  test('with the device asking for less motion, a change of screen is made at once', async ({ page }, testInfo) => {
    test.skip(testInfo.project.name !== 'laptop', 'the menu across the top');
    await page.emulateMedia({ reducedMotion: 'reduce' });
    await page.goto('/');
    await page.evaluate(() => {
      window.transitions = 0;
      const start = document.startViewTransition.bind(document);
      document.startViewTransition = (cb) => (window.transitions++, start(cb));
    });
    await page.locator('header + nav').getByRole('link', { name: 'Radio', exact: true }).click();
    await expect(page.getByRole('radiogroup', { name: 'Band' })).toBeVisible();
    expect(await page.evaluate(() => window.transitions)).toBe(0);
  });
});

test('the header volume is the slim slider; the one on Home keeps its size', async ({ page }, testInfo) => {
  test.skip(testInfo.project.name !== 'laptop', 'the volume sits in the header on a laptop');
  await page.goto('/');
  await expect(page.locator('header .range-slim').getByRole('slider', { name: 'Volume' })).toHaveCount(1);
  await expect(page.locator('main .range-slim')).toHaveCount(0);
});

test('the open screen is marked for a reader, and notes are spoken as they come', async ({ page }) => {
  await page.goto('/#radio');
  // The tabs at the foot on a phone, the menu across the top on a laptop:
  // whichever shows.
  const nav = page.getByRole('navigation').filter({ visible: true });
  await expect(nav).toHaveCount(1);
  await expect(nav.getByRole('link', { name: 'Radio' })).toHaveAttribute('aria-current', 'page');
  await expect(nav.getByRole('link', { name: 'Home' })).not.toHaveAttribute('aria-current', 'page');
  await expect(page.locator('[aria-live="polite"]')).toHaveCount(1);
});
