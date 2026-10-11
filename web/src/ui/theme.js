// Light or dark: the device's own setting, until the sun or moon in the
// header is pressed; that choice is then kept in this browser.
import { effect, signal } from '@preact/signals';

// A value this browser keeps, or null; and keeping one. `store` is
// sessionStorage for this tab only. In a private window, or with site data
// blocked, nothing is kept and the page goes on without it.
export function load(key, store = 'localStorage') {
  try {
    return window[store].getItem(key);
  } catch {
    return null;
  }
}
export function keep(key, value, store = 'localStorage') {
  try {
    window[store].setItem(key, value);
  } catch {
    // Not kept: the choice lasts until the page closes.
  }
}

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
