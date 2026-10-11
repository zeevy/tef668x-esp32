// The parts every screen is built from. They take their colours, corners and
// type from the tokens in styles.css, so a look changes no part.
import { useEffect, useLayoutEffect, useRef, useState } from 'preact/hooks';
import { ICONS } from './icons.js';
import { pinAsked } from '../data/radio.js';

export function Icon({ name, size = 20, class: cls = '' }) {
  return (
    <svg
      viewBox="0 0 24 24"
      width={size}
      height={size}
      fill="none"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
      class={`shrink-0 ${cls}`}
      dangerouslySetInnerHTML={{ __html: ICONS[name] }}
    />
  );
}

function Spinner({ size = 18 }) {
  return (
    <svg viewBox="0 0 24 24" width={size} height={size} class="shrink-0 animate-spin" aria-hidden="true">
      <circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" stroke-width="3" opacity="0.25" />
      <path d="M21 12a9 9 0 0 0-9-9" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" />
    </svg>
  );
}

const BUTTON = {
  primary: 'bg-accent text-on-accent',
  secondary: 'bg-raised text-ink',
  quiet: 'text-accent-ink',
  danger: 'border border-bad text-bad',
};

// kind: primary, secondary, quiet or danger. busy shows a spinner and stops
// a second tap while the radio answers. `href` makes it a link with the same
// look, such as a download. small: 36 px, for a card's head, which a full
// size one would make taller.
export function Button({ kind = 'primary', icon, busy, href, small, children, class: cls = '', ...rest }) {
  const Tag = href ? 'a' : 'button';
  return (
    <Tag
      type={href ? undefined : 'button'}
      href={href}
      {...rest}
      disabled={busy || rest.disabled}
      aria-busy={busy || undefined}
      class={`inline-flex ${small ? 'hit relative h-9 px-3 text-sm' : 'h-11 px-4'} items-center justify-center gap-2 rounded-ctl font-medium transition-opacity active:opacity-70 disabled:opacity-40 ${BUTTON[kind]} ${cls}`}
    >
      {busy ? <Spinner /> : icon && <Icon name={icon} size={small ? 16 : 20} />}
      {children}
    </Tag>
  );
}

// small: 36 px, for the header, where three sit together.
export function IconButton({ icon, label, kind = 'secondary', small, class: cls = '', ...rest }) {
  return (
    <button
      type="button"
      aria-label={label}
      title={label}
      {...rest}
      class={`grid ${small ? 'hit relative size-9' : 'size-11'} place-items-center rounded-ctl transition-opacity active:opacity-70 disabled:opacity-40 ${BUTTON[kind]} ${cls}`}
    >
      <Icon name={icon} />
    </button>
  );
}

// An on and off setting in a box the size of the controls beside it: its
// words on the left, a round switch on the right, the box tinted while on.
// The whole box toggles.
export function Switch({ on, onChange, label }) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={on}
      aria-label={label}
      onClick={() => onChange(!on)}
      class={`flex h-11 w-full items-center justify-between gap-2 rounded-ctl px-4 font-medium transition-colors ${on ? 'bg-accent-soft text-accent-ink' : 'bg-raised'}`}
    >
      <span class="flex items-center gap-2">{label}</span>
      <Track on={on} />
    </button>
  );
}

// The round switch itself: the knob in the card's colour on an accent track
// while on, on a grey one while off.
function Track({ on }) {
  return (
    <span class={`relative h-6 w-10 shrink-0 rounded-full transition-colors ${on ? 'bg-accent' : 'bg-muted/85'}`}>
      <span class={`absolute top-1 size-4 rounded-full bg-surface shadow transition-all ${on ? 'left-5' : 'left-1'}`} />
    </span>
  );
}

// A control moved in quick steps, a slider or a key held down, sends its
// value once, QUIET_MS after the last step, rather than once a step: each
// request holds the radio 85 to 280 ms. later(v) waits to send v; hold(v)
// keeps v waiting with no time set, as while a button is held down; now()
// sends what waits at once. `value` is what waits, null when nothing does.
// What waits when the control goes away is sent then, not dropped.
const QUIET_MS = 400;

export function useQuiet(send) {
  const [q] = useState(() => {
    let timer;
    const q = {
      value: null,
      hold(v) {
        q.value = v;
        clearTimeout(timer);
      },
      later(v) {
        q.hold(v);
        timer = setTimeout(q.now, QUIET_MS);
      },
      now() {
        clearTimeout(timer);
        const v = q.value;
        q.value = null;
        if (v !== null) q.send(v);
      },
    };
    return q;
  });
  q.send = send;
  useEffect(() => q.now, []);
  return q;
}

// onInput follows the finger.
export function Slider({ value, min = 0, max = 100, onInput, label }) {
  return (
    <input
      type="range"
      aria-label={label}
      min={min}
      max={max}
      value={value}
      onInput={(e) => onInput(+e.currentTarget.value)}
      style={{ '--fill': `${Math.min(100, Math.max(0, ((value - min) / (max - min)) * 100))}%` }}
      class="w-full"
    />
  );
}

// One choice of a few, across the whole width it is given, each the same size.
// The highlight under the choice slides to a new one. It is placed by
// measuring the chosen button, so buttons of any width work, and placed again
// when the row changes size.
export function Segmented({ options, value, onChange, label }) {
  const box = useRef();
  const [thumb, setThumb] = useState(null);
  useLayoutEffect(() => {
    const place = () => {
      const b = box.current?.querySelector('[aria-checked="true"]');
      setThumb(b ? { left: b.offsetLeft, width: b.offsetWidth } : null);
    };
    place();
    const watch = new ResizeObserver(place);
    watch.observe(box.current);
    return () => watch.disconnect();
  }, [value, options.join()]);
  return (
    <div ref={box} role="radiogroup" aria-label={label} class="relative flex w-full gap-1 rounded-ctl bg-raised p-1">
      {thumb && (
        <span
          aria-hidden="true"
          class="absolute inset-y-1 rounded-ctl bg-surface shadow-sm ring-1 ring-thumb-edge transition-[left,width] duration-200 ease-out motion-reduce:transition-none"
          style={{ left: `${thumb.left}px`, width: `${thumb.width}px` }}
        />
      )}
      {options.map((o) => (
        <button
          type="button"
          role="radio"
          aria-checked={o === value}
          onClick={() => onChange(o)}
          class={`hit relative h-9 flex-1 rounded-ctl px-1 text-sm font-medium whitespace-nowrap transition-colors ${o === value ? 'text-ink' : 'text-muted'}`}
        >
          {o}
        </button>
      ))}
    </div>
  );
}

// A drop-down list with its own arrow, 16 px in from the edge as the text
// is; the browser's own sits against the edge.
export function Select({ label, value, onChange, children }) {
  return (
    <div class="relative">
      <select
        aria-label={label}
        value={value}
        onChange={(e) => onChange(e.currentTarget.value)}
        class="h-11 w-full appearance-none rounded-ctl border border-line bg-surface pr-10 pl-4 outline-none focus:border-accent"
      >
        {children}
      </select>
      <Icon name="chevron-down" class="pointer-events-none absolute top-1/2 right-4 -translate-y-1/2 text-muted" />
    </div>
  );
}

// A setting as a row, as Settings, Network and System have them: its name
// and a note on the left, its control on the right, a line under it; on a
// narrow card the control goes under the name, except a small one, such as
// a switch, which `inline` keeps beside it. `icon` puts its tile at the
// row's start. `warn` colours the note.
export function SettingRow({ title, note, warn, inline, icon, children }) {
  return (
    <div class="@container grid content-center border-b border-line py-2 last:border-b-0">
      <div class="flex items-start gap-4">
        {icon && <IconTile icon={icon} />}
        <div class={`grid min-w-0 flex-1 gap-2 ${inline ? 'grid-cols-[1fr_auto] items-center gap-4' : '@2xl:grid-cols-[1fr_24rem] @2xl:items-center @2xl:gap-4'}`}>
          <div class="grid">
            <span class="font-medium">{title}</span>
            {note && <span class={`text-xs ${warn ? 'text-warn' : 'text-muted'}`}>{note}</span>}
          </div>
          <div class="grid">{children}</div>
        </div>
      </div>
    </div>
  );
}

// An on or off setting in a row: the round switch alone, at the row's right
// end; the row says what it is.
export function Toggle({ label, on, onChange }) {
  return (
    <button type="button" role="switch" aria-checked={on} aria-label={label} onClick={() => onChange(!on)} class="flex h-11 w-11 items-center justify-end justify-self-end">
      <Track on={on} />
    </button>
  );
}

// A text box's look, for the inputs each form has.
export const INPUT = 'h-11 w-full rounded-ctl border border-line bg-surface px-4 outline-none focus:border-accent';

// A reading with its label over it, such as Offset 4.2 kHz; `tone` colours
// the number.
export function Figure({ label, value, unit, tone = '' }) {
  return (
    <div class="grid min-w-0 gap-1">
      <span class="label truncate">{label}</span>
      <span class={`num truncate text-sm ${tone}`} title={typeof value === 'string' ? value : undefined}>
        {value}
        {unit && <span class="text-muted"> {unit}</span>}
      </span>
    </div>
  );
}

// Facts as rows, the name on the left and the value on the right, a line
// under each. A row is [name, value, num]: `num` puts the value in the
// figures' font. A value not known is ---. `class` lays the rows out.
export function FactRows({ rows, class: cls = 'grid' }) {
  return (
    <dl class={cls}>
      {rows.map(([name, value, num]) => (
        <div class="flex items-center justify-between gap-2 border-b border-line py-2">
          <dt class="text-sm text-muted">{name}</dt>
          <dd class={`truncate text-sm font-medium ${num ? 'num' : ''}`} title={value ?? ''}>
            {value ?? '---'}
          </dd>
        </div>
      ))}
    </dl>
  );
}

// A grey box that pulses where something is still being read.
export function Loading({ class: cls = 'h-14' }) {
  return <div class={`animate-pulse rounded-ctl bg-raised ${cls}`} aria-hidden="true" />;
}

// In place of something the radio did not give when asked. The page asks
// again by itself, so the line says so.
export function Unread({ what }) {
  return <p class="text-muted">The {what} could not be read. Trying again.</p>;
}

// In place of a screen whose reads the radio refused for want of the PIN.
export function PinNeeded({ what }) {
  return (
    <Card>
      <p>The {what} need the PIN.</p>
      <Button icon="lock" class="mt-4" onClick={() => (pinAsked.value = true)}>
        Enter the PIN
      </Button>
    </Card>
  );
}

// A button that picks a file, the browser's own picker behind it;
// `onFile` gets the file.
export function FileButton({ accept, label, onFile, class: cls = '', children }) {
  return (
    <label class={`inline-flex h-11 min-w-0 cursor-pointer items-center justify-center gap-2 rounded-ctl bg-raised px-3 font-medium ${cls}`}>
      {children}
      <input
        type="file"
        accept={accept}
        aria-label={label}
        class="sr-only"
        onChange={(e) => {
          const file = e.currentTarget.files?.[0];
          e.currentTarget.value = '';
          if (file) onFile(file);
        }}
      />
    </label>
  );
}

const TONE = {
  accent: 'bg-accent/15 text-accent-ink',
  good: 'bg-good/15 text-good',
  warn: 'bg-warn/15 text-warn',
  bad: 'bg-bad/15 text-bad',
  off: 'bg-raised text-muted',
};

// A short state, such as Stereo or RDS.
export function Chip({ tone = 'accent', icon, children }) {
  return (
    <span class={`inline-flex h-6 items-center gap-1 rounded-ctl px-2 text-xs font-semibold ${TONE[tone]}`}>
      {icon && <Icon name={icon} size={14} />}
      {children}
    </span>
  );
}

// A level as a bar, with its reading. A slider in place of the bar makes a
// setting look the same as the readings beside it.
export function Meter({ value, max, label, unit, children }) {
  return (
    <div class="grid gap-1">
      <div class="flex items-baseline justify-between">
        <span class="label">{label}</span>
        <span class="num text-sm">
          {value} <span class="text-muted">{unit}</span>
        </span>
      </div>
      {children ?? <Bar pct={Math.max(0, Math.min(100, (value / max) * 100)) || 0} />}
    </div>
  );
}

// A bar 8 px tall, filled to `pct` per cent; with no `pct`, made of
// `children`, such as RDS's clean, fixed and lost parts side by side.
// `label` names it for a screen reader: a filled bar is then a progress bar,
// one of parts a picture.
export function Bar({ pct, label, class: cls = '', children }) {
  const filled = pct != null;
  return (
    <div class={`flex h-2 overflow-hidden rounded-ctl bg-raised ${cls}`} role={label && (filled ? 'progressbar' : 'img')} aria-label={label} aria-valuenow={label && filled ? Math.round(pct) : undefined}>
      {filled ? <div class="rounded-ctl bg-accent transition-[width] duration-300 ease-out motion-reduce:transition-none" style={{ width: `${pct}%` }} /> : children}
    </div>
  );
}

// `warn` draws it in the warning colour, for a card whose actions can leave
// the radio in a state it does not come back from by itself. `back` is a
// faint picture behind its content (ui/backs.jsx).
export function Card({ title, end, warn, back, children, class: cls = '' }) {
  return (
    <section class={`rounded-card border p-4 shadow-card ${warn ? 'border-warn/50 bg-warn/5' : 'border-edge bg-surface'} ${back ? 'relative isolate overflow-hidden' : ''} ${cls}`}>
      {back && (
        <div aria-hidden="true" class="back pointer-events-none absolute inset-0 -z-10 overflow-hidden text-accent">
          {back}
        </div>
      )}
      {title && (
        <header class="mb-4 flex min-h-6 items-center justify-between gap-2">
          <h2 class={`label ${warn ? 'text-warn' : ''}`}>{title}</h2>
          {end}
        </header>
      )}
      {children}
    </section>
  );
}

// The accent tile is solid, as it may sit over a card's picture.
const TILE = {
  accent: 'bg-accent-soft text-accent-ink',
  off: 'bg-raised text-muted',
};

// An icon on a small square tile, at the start of a row or a fact.
export function IconTile({ icon, tone = 'accent' }) {
  return (
    <div class={`grid size-10 shrink-0 place-items-center rounded-ctl ${TILE[tone]}`}>
      <Icon name={icon} />
    </div>
  );
}

// The head of a status card: a large icon tile, a large line and a small
// one. `icon` is an icon's name, or a drawing of its own for the tile, such
// as the Wi-Fi signal's bars.
export function StatusHead({ icon, title, sub }) {
  return (
    <div class="flex items-center gap-4">
      <div class="grid size-14 shrink-0 place-items-center rounded-card bg-accent-soft text-accent-ink">
        {typeof icon === 'string' ? <Icon name={icon} size={28} /> : icon}
      </div>
      <div class="min-w-0 flex-1">
        <p class="truncate text-2xl font-semibold">{title}</p>
        <p class="num text-sm text-muted">{sub}</p>
      </div>
    </div>
  );
}

// A question before an action that cannot be taken back: what happens, and
// two buttons, the safe one first.
export function Ask({ title, text, no, yes, kind = 'primary', onYes, onClose }) {
  const go = () => {
    onClose();
    onYes();
  };
  return (
    <Modal title={title} cancel={no} onClose={onClose} actions={<Button kind={kind} onClick={go}>{yes}</Button>}>
      <p class="text-sm">{text}</p>
    </Modal>
  );
}

// Two widths, each with 16 px kept on each side of a phone. The native
// dialog's own max-width is narrower than that on a phone, so it is lifted.
const MODAL = {
  sm: 'w-[min(28rem,calc(100%-2rem))]',
  lg: 'w-[min(32rem,calc(100%-2rem))]',
};

// Every dialog: a title, the content, and a row of buttons at the foot that
// wraps, and stays in view when the dialog scrolls, as on a phone held
// sideways. `danger`, such as Delete, on the left, and Cancel in focus at
// the start so a key pressed at once deletes nothing; on the right a quiet
// `cancel`, then `actions`, the main button last. With no `actions` the
// content has its own button, as a form: a Close key by the title takes
// Cancel's place, so that button stays in view with the phone's keyboard
// open. `onSubmit` makes the dialog a form.
export function Modal({ title, size = 'sm', onClose, onSubmit, danger, cancel = 'Cancel', actions, children }) {
  const Body = onSubmit ? 'form' : 'div';
  return (
    <Dialog onClose={onClose} class={`m-auto max-w-none rounded-card ${MODAL[size]}`}>
      <Body onSubmit={onSubmit} class="grid gap-4 p-4">
        <div class={`flex items-center justify-between gap-2 ${actions ? '' : '-my-2'}`}>
          <h2 class="min-w-0 text-lg font-semibold wrap-anywhere">{title}</h2>
          {!actions && <IconButton icon="x" label="Close" kind="quiet" onClick={onClose} />}
        </div>
        {children}
        {actions && (
          <div class="sticky bottom-0 -mb-4 flex flex-wrap items-center justify-end gap-2 bg-surface pb-4">
            {danger}
            <span class="flex-1" />
            <Button kind="quiet" autofocus={!!danger} onClick={onClose}>
              {cancel}
            </Button>
            {actions}
          </div>
        )}
      </Body>
    </Dialog>
  );
}

// A short note that the last action worked or did not.
export function Toast({ tone = 'good', children }) {
  return (
    <div role="status" class="flex items-center gap-2 rounded-ctl border border-line bg-surface px-4 py-2 shadow-lg motion-safe:animate-rise">
      <Icon name={tone === 'good' ? 'circle-check' : 'triangle-alert'} class={tone === 'good' ? 'text-good' : 'text-bad'} />
      <span class="flex-1 text-sm">{children}</span>
    </div>
  );
}

// A line across the top of a screen when something needs attention.
export function Banner({ tone = 'warn', icon = 'triangle-alert', children, action }) {
  return (
    <div class={`flex items-start gap-2 rounded-ctl px-4 py-2 text-sm leading-5 ${tone === 'off' ? 'border border-line bg-surface text-muted' : TONE[tone]}`}>
      <Icon name={icon} />
      <span class="flex-1">{children}</span>
      {action}
    </div>
  );
}

// A native modal dialog, open while it is drawn: Escape and a tap outside
// close it.
export function Dialog({ onClose, class: cls = '', children }) {
  const ref = useRef();
  useEffect(() => ref.current.showModal(), []);
  return (
    <dialog
      ref={ref}
      onClose={onClose}
      onClick={(e) => e.target === ref.current && onClose()}
      class={`bg-surface text-ink shadow-xl backdrop:bg-black/50 motion-safe:animate-rise ${cls}`}
    >
      {children}
    </dialog>
  );
}
