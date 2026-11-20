// A change on the radio from a screen, and the short notes the page shows
// over the screen.
import { signal } from '@preact/signals';
import { change } from './radio.js';

// Short notes over the screen. Each goes after four seconds; one that must
// be read, `kept`, stays until it is closed.
export const notes = signal([]);
export const unsay = (note) => (notes.value = notes.value.filter((n) => n !== note));
export function say(text, tone = 'good', kept = false) {
  const note = { text, tone, kept };
  notes.value = [...notes.value, note];
  if (!kept) setTimeout(() => unsay(note), 4000);
}

// A refusal, said in the radio's own words. One for the PIN opens the PIN
// prompt instead, so it is not said.
export function refused(e) {
  if (e.status !== 403) say(e.message, 'bad');
}

// Nothing waits on a change: the screen draws it at once when it can, and
// the radio's own words show when it refuses. Resolves true when the radio
// took it.
export function act(path, fields, expect) {
  return change(path, fields, expect).then(
    () => true,
    (e) => {
      refused(e);
      return false;
    },
  );
}
