// The bands of the radio's band plan, from GET /api/bands: each band's
// edges, the steps and filter widths it offers, and the step and width it
// has now. Read at start, and again whenever the radio stores its settings
// (`asv.n`), which is when a band's step or width is kept.
import { getJson } from './radio.js';
import { kept } from './reads.js';

const plan = kept((s) => s.asv?.n, () => getJson('/api/bands'), true);

// One band by its name, or undefined until the list has come. A screen that
// asks is drawn again when the list comes. A failed read again keeps the
// last plan read: it changes only when a step or a width is stored, and a
// screen without it cannot tune.
let last = null;
export function bandOf(name) {
  last = plan.value.data ?? last;
  return last?.find((b) => b.band === name);
}

// The band a frequency in kHz is on, as the radio decides it from these
// edges: the lowest band first, and OIRT before FM, whose widest region
// starts at 65 MHz. Undefined until the list has come, or on no band.
const LOW_FIRST = ['LW', 'MW', 'SW', 'OIRT', 'FM'];
export const bandAt = (khz) =>
  LOW_FIRST.find((name) => {
    const b = bandOf(name);
    return b && khz >= b.low && khz <= b.high;
  });
