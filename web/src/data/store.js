// What this browser keeps for the page.

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
