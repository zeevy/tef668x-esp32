// The keys a laptop gets on Home, Radio and DX, and a preset as a key and the
// key that adds one, which they share with the Presets screen.
import { useEffect, useRef, useState } from 'preact/hooks';
import { state, volHeld } from '../data/radio.js';
import { freq, presetTag, typedKhz, unit } from '../data/format.js';
import { mute, recall, seek, step, tune, volume } from '../data/commands.js';
import { VOL_MAX, VOL_MIN } from '../data/limits.js';
import { say } from '../data/act.js';
import { Icon, useQuiet } from './kit.jsx';

// A preset as a key, the same on Home and Presets: its band and number on
// top, the frequency large, the name under it cut to one line; lit when the
// dial is on it. Anything else given goes on the button, and `children`
// inside it, for Presets' Change mode.
export function PresetKey({ p, on, class: cls = '', children, ...rest }) {
  return (
    <button
      type="button"
      aria-pressed={on}
      title={p.name || undefined}
      {...rest}
      class={`relative flex h-20 min-w-0 flex-col items-start justify-center gap-1 rounded-ctl px-3 text-left ${on ? 'bg-accent text-on-accent' : 'bg-raised'} ${cls}`}
    >
      <span class={`flex w-full justify-between text-xs ${on ? '' : 'text-muted'}`}>
        <span>{p.band}</span>
        <span class="num font-semibold">{presetTag(p.slot)}</span>
      </span>
      <span class="num text-lg leading-6 font-semibold">{freq(p.khz, p.band)}</span>
      <span class={`w-full truncate text-xs ${on ? '' : 'text-muted'} ${children ? 'pr-4' : ''}`}>{p.name || unit(p.band)}</span>
      {children}
    </button>
  );
}

// An empty key, dashed, the size of a preset's, that adds one: a plus and
// `children`, such as the slot it adds to.
export function AddKey({ class: cls = '', children, ...rest }) {
  return (
    <button type="button" {...rest} class={`flex h-20 min-w-0 flex-col items-start justify-center gap-1 rounded-ctl border-2 border-dashed border-muted/50 px-3 text-xs text-muted ${cls}`}>
      <Icon name="plus" size={16} />
      {children}
    </button>
  );
}

// Keys on a laptop: ← and → step, with Shift they seek; ↑ and ↓ the volume,
// a dB a press; M mutes or turns the sound on; a preset's number recalls it.
// Not while typing or with a dialog open, and not with Ctrl, Alt or Cmd.
// A preset's digits are gathered until PRESET_TYPE_MS after the last, so 12
// is preset 12; Enter recalls at once, Escape lets it go. The volume is
// drawn at once and sent once it is still. A held arrow adds its steps up
// and sends them as one when it is let go, so the dial does not go on
// moving after; a held seek or M counts once. The keys are set up once, and
// recall from the presets as last given. Gives the preset number being
// typed, '' while none is.
//
// With `freqTyping`, as on DX, digits and a point type a frequency instead,
// as XDR-GTK and FM-DX Webserver do: Enter tunes there, Backspace takes the
// last one back, Escape lets it go. `more` gets the keys none of these take,
// and says true for one it took.
const PRESET_TYPE_MS = 1000;

export function useKeys(presets, { freqTyping, more } = {}) {
  const [typedSlot, setTypedSlot] = useState('');
  const known = useRef();
  known.current = presets;
  const extra = useRef();
  extra.current = more;
  const vol = useQuiet(volume);
  const steps = useQuiet((n) => n && step(n));
  useEffect(() => {
    let digits = '';
    let digitTimer;
    const typed = (d) => {
      digits = d;
      setTypedSlot(d);
    };
    function tuneTyped() {
      const khz = typedKhz(digits, state.value?.tun.bnd);
      typed('');
      if (khz === null) say('Type a frequency, such as 98.3 or 738.', 'bad');
      else tune(khz);
    }
    function recallTyped() {
      clearTimeout(digitTimer);
      const n = +digits;
      typed('');
      const p = known.current?.find((x) => x.slot === n);
      if (p) recall(p);
      else if (n) say(`There is no preset ${n}.`, 'bad');
    }
    function onKey(e) {
      // A key a focused part took for itself, such as an arrow on a chart.
      if (e.defaultPrevented) return;
      // [ and ] take Option or AltGr on many keyboards, such as German ones.
      const bracket = e.key === '[' || e.key === ']';
      if (e.metaKey || ((e.ctrlKey || e.altKey) && !bracket) || document.querySelector('dialog[open]')) return;
      if (e.target.closest?.('input, textarea, select, [contenteditable]')) return;
      const st = state.value;
      if (!st) return;
      if (freqTyping && /^[0-9.,]$/.test(e.key)) {
        typed((digits + e.key).slice(0, 7));
      } else if (freqTyping && digits && e.key === 'Backspace') {
        typed(digits.slice(0, -1));
      } else if (!freqTyping && /^[0-9]$/.test(e.key)) {
        typed((digits + e.key).slice(-2));
        clearTimeout(digitTimer);
        digitTimer = setTimeout(recallTyped, PRESET_TYPE_MS);
      } else if (digits && e.key === 'Enter') {
        e.preventDefault();
        if (freqTyping) tuneTyped();
        else recallTyped();
      } else if (digits && e.key === 'Escape') {
        clearTimeout(digitTimer);
        typed('');
      } else if (e.key === 'ArrowUp' || e.key === 'ArrowDown') {
        e.preventDefault();
        const db = Math.max(VOL_MIN, Math.min(VOL_MAX, (volHeld.value ?? st.tun.vol) + (e.key === 'ArrowUp' ? 1 : -1)));
        volHeld.value = db;
        vol.later(db);
      } else if (e.key === 'ArrowLeft' || e.key === 'ArrowRight') {
        e.preventDefault();
        const up = e.key === 'ArrowRight';
        if (!e.shiftKey) steps.later((steps.value ?? 0) + (up ? 1 : -1));
        else if (!e.repeat) seek(up);
      } else if ((e.key === 'm' || e.key === 'M') && !e.repeat) {
        mute(!st.tun.mut);
      } else if (!e.repeat && extra.current?.(e, st)) {
        e.preventDefault();
      }
    }
    const onKeyUp = (e) => (e.key === 'ArrowLeft' || e.key === 'ArrowRight') && steps.now();
    addEventListener('keydown', onKey);
    addEventListener('keyup', onKeyUp);
    return () => {
      removeEventListener('keydown', onKey);
      removeEventListener('keyup', onKeyUp);
      clearTimeout(digitTimer);
    };
  }, []);
  return typedSlot;
}

// The keys, in a line under the screen on a laptop, and the preset number
// or the frequency being typed, large over the page.
const HELP = "Keys: ← and → step, Shift with them seeks, ↑ and ↓ the volume, M mutes, a preset's number recalls it.";
export function KeysHelp({ typedSlot, help = HELP, typing = 'Preset' }) {
  return (
    <>
      <p class="hidden text-sm text-muted md:block">{help}</p>
      {typedSlot !== '' && (
        <div aria-live="polite" class="num fixed bottom-24 left-1/2 z-20 -translate-x-1/2 rounded-card bg-ink px-4 py-2 text-2xl text-surface shadow-card">
          {typing} {typedSlot}
        </div>
      )}
    </>
  );
}
