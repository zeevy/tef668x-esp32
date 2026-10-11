// Light or dark: the device's own setting, until the sun or moon in the
// header is pressed; that choice is then kept in this browser.
import { effect, signal } from '@preact/signals';

import { keep, load } from '../data/store.js';

const deviceDark = matchMedia('(prefers-color-scheme: dark)');
const kept = load('theme');
const theme = signal(kept === 'Light' || kept === 'Dark' ? kept : null);
const deviceIsDark = signal(deviceDark.matches);
deviceDark.addEventListener('change', (e) => (deviceIsDark.value = e.matches));
export const dark = () => (theme.value ?? (deviceIsDark.value ? 'Dark' : 'Light')) === 'Dark';
effect(() => {
  document.documentElement.dataset.theme = dark() ? 'dark' : 'light';
});

// The other theme, kept in this browser.
export function flipTheme() {
  theme.value = dark() ? 'Light' : 'Dark';
  keep('theme', theme.value);
}
