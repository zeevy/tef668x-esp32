// One spacing scale for the whole page: 16 px for page edges, inside cards,
// between cards and between blocks; 8 px within one group; 4 px from a label
// to its own bar and between menu entries.
import fs from 'node:fs';
import path from 'node:path';
import { expect, test } from '@playwright/test';

const SCALE = new Set(['0', '1', '2', '4']);
// Named on purpose: a control's own padding where the scale would not fit,
// room kept above the phone's foot tabs, and a drop-down list's room for its
// arrow, 16 px in from the edge.
const EXCEPTIONS = new Set(['px-3', 'pb-16', 'pr-10']);

function sources(dir) {
  return fs.readdirSync(dir, { withFileTypes: true }).flatMap((e) => {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) return e.name === 'design' ? [] : sources(p);
    return /\.(jsx?|css)$/.test(e.name) ? [p] : [];
  });
}

test('no spacing class outside the scale', async () => {
  const off = [];
  for (const file of sources(new URL('../src', import.meta.url).pathname)) {
    const text = fs.readFileSync(file, 'utf8');
    for (const m of text.matchAll(/(?<![\w-])-?(?:[mp][trblxy]?|gap(?:-[xy])?)-(\d+(?:\.\d+)?)(?![\w.])/g)) {
      const cls = m[0].replace(/^-/, '');
      if (!SCALE.has(m[1]) && !EXCEPTIONS.has(cls)) off.push(`${path.basename(file)}: ${m[0]}`);
    }
  }
  expect(off).toEqual([]);
});

test('on Home every card has 16 px inside and 16 px to the next', async ({ page }, testInfo) => {
  await page.goto('/');
  await expect(page.getByText('Last heard')).toBeVisible();
  const cards = await page.locator('main section').evaluateAll((els) =>
    els.map((e) => {
      const r = e.getBoundingClientRect();
      const s = getComputedStyle(e);
      return { top: r.top, bottom: r.bottom, left: r.left, right: r.right, pad: [s.paddingTop, s.paddingRight, s.paddingBottom, s.paddingLeft] };
    }),
  );
  for (const c of cards) expect(c.pad).toEqual(['16px', '16px', '16px', '16px']);
  // Cards one above the other in the same column are 16 px apart.
  for (const a of cards) {
    const below = cards.filter((b) => b.top >= a.bottom && b.left < a.right && b.right > a.left);
    if (below.length) expect(Math.min(...below.map((b) => b.top - a.bottom))).toBe(16);
  }
  // Cards side by side are 16 px apart too.
  for (const a of cards) {
    const beside = cards.filter((b) => b.left >= a.right && b.top === a.top);
    if (beside.length) expect(Math.min(...beside.map((b) => b.left - a.right))).toBe(16);
  }
  if (testInfo.project.name === 'phone') expect(cards[0].left).toBe(16);
});
