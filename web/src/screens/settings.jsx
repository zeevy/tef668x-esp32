// Settings: every setting of the radio's menu that the web can change, in
// the menu's groups. A change goes to the radio at once and is drawn at
// once; a refusal puts back what the radio has. The radio keeps the
// reception settings by itself 10 s after the last change. One group at a
// time: a side menu on a laptop, a drop-down on a phone or a tablet.
import { useEffect, useRef, useState } from 'preact/hooks';
import { state } from '../data/radio.js';
import { useStoredSettings } from '../data/reads.js';
import { act, say } from '../data/act.js';
import { withMotion } from '../data/motion.js';
import { bandOf } from '../data/bands.js';
import { BANDS, isFm } from '../data/format.js';
import { PRESET_SLOTS } from '../data/limits.js';
import { Button, Card, Chip, Icon, Loading, PinNeeded, Segmented, Select, SettingRow, StatusHead, Toggle, Unread, useQuiet } from '../ui/kit.jsx';
import { Gears } from '../ui/backs.jsx';
import { SCAN_RANGE } from './dx.jsx';

// Values from `from` to `to` in `step`s, with `off` first when it has one.
const range = (from, to, step = 1, off) => [...(off !== undefined ? [off] : []), ...Array.from({ length: Math.floor((to - from) / step) + 1 }, (_, i) => from + i * step)];
const THEMES = ['Nightwatch', 'Daylight', 'Red Night', 'Phosphor', 'Clear', 'Custom', 'Slate', 'Paper', 'LCD', 'Ember', 'Clear Day', 'High Contrast', 'Mono', 'Hi-Fi', 'Violet', 'Blossom'];
const two = (n) => String(n).padStart(2, '0');
// Every UTC offset the clock takes, -12:00 to +14:00 in quarters of an hour.
const ZONES = range(-12 * 60, 14 * 60, 15).map((m) => `${m < 0 ? '-' : '+'}${two(Math.floor(Math.abs(m) / 60))}:${two(Math.abs(m) % 60)}`);
const offOr = (unit, off = 'Off') => (v) => (v === 0 ? off : `${v} ${unit}`);
const AM_BANDS = BANDS.filter((b) => !isFm(b));

// A group a menu entry, its `icon` beside its name in the side menu. One
// row a setting. `key` is the API's name for it; `route` where a change
// goes: `s` POST /api/settings, `fm` POST /api/fm. `kind`: `switch`,
// `pair` (two to six choices side by side), `list` (a drop-down) or `steps`
// (less and more). `values` and `say` give the choices and their words.
// `restart`: taken at the next start. `fm`: the radio takes it only on FM.
// `flip`: the API's 1 means off. `reading`: the state's value under this key
// is the chip's live reading, not the setting, so the stored one is shown.
// `route` `step` or `width` is a band's own step or filter width, `band`
// naming it, its choices from the radio's band plan; `values` can be a
// function, for lists that come from the radio. `extra` is a part drawn
// under the group's rows.
const GROUPS = [
  {
    name: 'Sound',
    icon: 'volume-2',
    rows: [
      { label: 'Squelch floor', key: 'sqf', kind: 'steps', values: range(1, 40, 1, 0), say: offOr('dBµV') },
      { label: 'Volume AGC', key: 'agt', kind: 'steps', values: range(30, 80, 2, 0), say: offOr('%') },
      { label: 'AGC boost', key: 'agb', kind: 'steps', values: range(1, 8, 1, 0), say: offOr('dB', 'Cut only') },
      { label: 'Mute ramp', key: 'smu', kind: 'steps', values: range(0, 500, 10), say: (v) => `${v} ms` },
    ],
  },
  {
    name: 'FM reception',
    icon: 'radio',
    rows: [
      { label: 'Band plan', key: 'rgn', kind: 'list', values: [0, 1, 2, 3, 4], say: (v) => ['Full', 'Japan', 'Wide', '87 to 108 MHz', 'Worldwide'][v], restart: true },
      { label: 'Tuning step', key: 'step-FM', route: 'step', band: 'FM', kind: 'pair', values: () => bandOf('FM')?.steps ?? [], say: (v) => `${v} kHz` },
      { label: 'Seek sensitivity', key: 'fsn', kind: 'pair', values: range(1, 6) },
      { label: 'De-emphasis', key: 'dem', route: 'fm', kind: 'pair', values: [0, 50, 75], say: offOr('µs') },
      { label: 'Force mono', key: 'mno', route: 'fm', kind: 'switch', fm: true },
      { label: 'Stereo blend', key: 'bld', route: 'fm', kind: 'steps', values: range(20, 60, 1, 0), say: offOr('dBµV'), fm: true, reading: true },
      { label: 'High blend', key: 'hbl', route: 'fm', kind: 'steps', values: range(20, 60, 1, 0), say: offOr('dBµV'), fm: true, reading: true },
      { label: 'High cut', key: 'cut', route: 'fm', kind: 'steps', values: range(20, 60, 1, 0), say: offOr('dBµV'), fm: true, reading: true },
      { label: 'Noise blanker', key: 'fnb', route: 'fm', kind: 'steps', values: range(50, 150, 5, 0), say: offOr('%') },
      { label: 'iMS', key: 'ims', route: 'fm', kind: 'switch', fm: true },
      { label: 'Equaliser', key: 'eq', route: 'fm', kind: 'switch', fm: true },
      { label: 'RDS decoder', key: 'rds', kind: 'switch' },
      { label: 'RDS region', key: 'rrg', kind: 'pair', values: [0, 1], say: (v) => ['Europe', 'North America'][v] },
    ],
  },
  {
    name: 'AM reception',
    icon: 'radio',
    rows: [
      { label: 'MW channel spacing', key: 'spc', kind: 'pair', values: [0, 1], say: (v) => ['9 kHz', '10 kHz'][v], restart: true },
      ...AM_BANDS.map((b) => ({ label: `Tuning step, ${b}`, key: `step-${b}`, route: 'step', band: b, kind: 'pair', values: () => bandOf(b)?.steps ?? [], say: (v) => `${v} kHz` })),
      ...AM_BANDS.map((b) => ({ label: `Filter width, ${b}`, key: `width-${b}`, route: 'width', band: b, kind: 'pair', values: () => bandOf(b)?.widths ?? [], say: (v) => `${v} kHz` })),
      { label: 'Seek sensitivity', key: 'asn', kind: 'pair', values: range(1, 6) },
      { label: 'Noise blanker', key: 'anb', route: 'fm', kind: 'steps', values: range(50, 150, 5, 0), say: offOr('%') },
      { label: 'High cut, MW and SW', key: 'ahc', route: 'fm', kind: 'steps', values: range(20, 60, 1, 0), say: offOr('dBµV') },
      { label: 'High cut, LW', key: 'lhc', route: 'fm', kind: 'steps', values: range(20, 60, 1, 0), say: offOr('dBµV') },
      { label: 'Soft mute, MW and SW', key: 'asm', route: 'fm', kind: 'steps', values: range(0, 50), say: offOr('dBµV') },
      { label: 'Soft mute, LW', key: 'lsm', route: 'fm', kind: 'steps', values: range(0, 50), say: offOr('dBµV') },
    ],
  },
  {
    name: 'Display',
    icon: 'sun',
    rows: [
      { label: 'Day theme', key: 'thm', kind: 'list', values: range(0, 15), say: (v) => THEMES[v] },
      { label: 'Night theme', key: 'thn', kind: 'list', values: range(0, 15), say: (v) => THEMES[v] },
      { label: 'Brightness', key: 'blt', kind: 'steps', values: range(5, 100, 5), say: (v) => `${v} %` },
      { label: 'Dim level', key: 'bdm', kind: 'steps', values: range(0, 100, 5), say: (v) => `${v} %` },
      { label: 'Dim after', key: 'bds', kind: 'steps', values: range(5, 240, 5, 0), say: offOr('s', 'Never') },
      { label: 'Rotation', key: 'rot', kind: 'pair', values: [0, 180], say: (v) => `${v}°` },
      { label: 'Battery', key: 'bat', kind: 'pair', values: [0, 1, 2], say: (v) => ['Off', 'Per cent', 'Volts'][v] },
      { label: 'Battery check', key: 'bck', kind: 'switch', note: 'Every 5 min while nobody uses the radio, Wi-Fi stops for a moment to read the battery' },
    ],
    extra: Colours,
  },
  {
    name: 'Controls',
    icon: 'settings',
    rows: [
      { label: 'Key beeps', key: 'bpk', kind: 'list', values: [0, 1, 2, 3], say: (v) => ['Off', 'Keypad only', 'Short and long', 'Every press'][v] },
      { label: 'Keypad timeout', key: 'kpt', kind: 'steps', values: range(5, 60, 5), say: (v) => `${v} s` },
      { label: 'Band edge beep', key: 'bpe', kind: 'switch' },
      { label: 'Start chime', key: 'bps', kind: 'switch', restart: true },
      { label: 'Encoder type', key: 'enc', kind: 'pair', values: [0, 1], say: (v) => ['Standard', 'Optical'][v], restart: true },
      { label: 'Encoder direction', key: 'edr', kind: 'pair', values: [0, 1], say: (v) => ['Normal', 'Reversed'][v], restart: true },
      { label: 'Touch', key: 'tof', kind: 'switch', flip: true },
    ],
    extra: Knob,
  },
  {
    name: 'Clock',
    icon: 'refresh-cw',
    rows: [
      { label: 'Network time', key: 'ntp', kind: 'switch' },
      { label: 'Time zone, from UTC', key: 'tzo', kind: 'list', values: ZONES },
    ],
  },
  {
    name: 'DX scanner',
    icon: 'satellite-dish',
    rows: [
      { label: 'Dwell on each channel', key: 'ddw', kind: 'steps', values: range(5, 300, 5), say: (v) => `${(v / 10).toFixed(1)} s` },
      { label: 'Stop on', key: 'dst', kind: 'list', values: [0, 1, 2], say: (v) => ['New stations only', 'Any PI', 'Never'][v] },
      { label: 'Scan range', key: 'dsc', kind: 'list', values: [0, 1, 2], say: (v) => SCAN_RANGE[v] },
      { label: 'Preset start', key: 'dmf', kind: 'steps', values: range(1, PRESET_SLOTS) },
      { label: 'Preset end', key: 'dml', kind: 'steps', values: range(1, PRESET_SLOTS) },
      { label: 'Scan bandwidth', key: 'dbw', kind: 'list', values: () => (bandOf('FM')?.widths ?? []).filter((w) => w > 0), say: (v) => `${v} kHz` },
      { label: 'Loop band', key: 'dlp', kind: 'switch' },
      { label: 'Mute during scan', key: 'dmu', kind: 'switch' },
      { label: 'Log new stations', key: 'dal', kind: 'switch' },
      { label: 'Log radio text', key: 'drt', kind: 'switch' },
      { label: 'Watch presets', key: 'dwt', kind: 'switch' },
    ],
  },
  {
    name: 'Auto off',
    icon: 'power',
    rows: [{ label: 'Switch off after', key: 'slp', kind: 'steps', values: range(5, 600, 5, 0), say: offOr('min') }],
  },
];

// A row's choices: its list, or the list a function gets from the radio.
const valuesOf = (row) => (typeof row.values === 'function' ? row.values() : row.values);

export function Settings() {
  const s = state.value;
  const { stored, set, error, needPin } = useStoredSettings();
  const [group, setGroup] = useState(null);
  if (needPin) return <PinNeeded what="settings" />;
  if (!stored) return <Card>{error ? <Unread what="settings" /> : <Loading />}</Card>;
  // A band's step or width, changed here, is drawn under its row's key until
  // the radio's band plan is read again. A reception setting, one sent to
  // /api/fm, is the one the radio is using now, from the live state, which
  // is newer than the stored one until the radio saves itself; except one
  // whose key in the state is a reading.
  const valueOf = ({ key, route, band, reading }) => {
    if (band) return stored[key] ?? bandOf(band)?.[route];
    if (route === 'fm' && !reading && s.tun[key] !== undefined) return +s.tun[key];
    return stored[key];
  };
  // Each drawn at once over the stored settings, and a reception setting the
  // radio is using over its state too. A reception setting is stored only at
  // the radio's next save, so a read before that still has the old value;
  // the drawing stays until a read after the save.
  async function change(row, v) {
    const took = row.band
      ? await set({ khz: v, bnd: row.band }, row.route === 'step' ? '/api/step-size' : '/api/bandwidth', { [row.key]: v })
      : row.route === 'fm'
        ? await set({ [row.key]: v }, '/api/fm', { [row.key]: v }, !row.reading && ((n) => (n.tun[row.key] = v)))
        : await set({ [row.key]: v });
    if (took && row.restart) say(`${row.label} is taken at the next start of the radio.`);
  }
  const fmNow = isFm(s.tun.bnd);

  const here = GROUPS.find((g) => g.name === group) ?? GROUPS[0];
  const pick = (g) => withMotion(() => setGroup(g));
  const noteOf = (row) => [row.note, row.restart && 'At the next start', row.fm && !fmNow && 'Taken only on FM'].filter(Boolean).join(' · ');
  // The automatic save, as a chip: done, waiting, off or failed.
  const asv = s.asv ?? {};
  const saved = asv.bad ? ['bad', 'The last save failed'] : !asv.idl ? ['off', 'Saving by itself is off'] : asv.due > 0 ? ['accent', `Saving in ${Math.ceil(asv.due / 1000)} s`] : ['good', 'Saved'];
  // The open group across the page, gears behind, with every group as a
  // tile beside it on a laptop and under it on a phone, a name on two lines
  // where a tile is narrow; its settings under that.
  return (
    <div class="grid grid-cols-1 gap-4">
      <Card back={<Gears />} class="@container">
        <div class="grid grid-cols-1 gap-4 @3xl:grid-cols-2">
          <div class="grid content-between gap-4">
            <StatusHead icon={here.icon} title={here.name} sub={`${here.rows.length} settings · ${here.rows.slice(0, 3).map((r) => r.label).join(', ')}`} />
            <div class="flex flex-wrap items-center gap-2">
              <Chip tone={saved[0]}>{saved[1]}</Chip>
              <span class="text-sm text-muted">{asv.idl ? `Saved by itself ${Math.round(asv.idl / 1000)} s after a change` : 'Saved only from the menu'}</span>
            </div>
          </div>
          <nav class="grid grid-cols-4 gap-1 @sm:gap-2" aria-label="Groups">
            {GROUPS.map((g) => (
              <button
                type="button"
                aria-current={g === here ? 'page' : undefined}
                onClick={() => pick(g.name)}
                class={`grid min-h-16 place-items-center content-center gap-1 rounded-ctl px-1 py-2 text-center text-xs leading-4 ${g === here ? 'bg-accent-soft font-semibold text-accent-ink' : 'bg-raised hover:text-ink'}`}
              >
                <Icon name={g.icon} />
                <span>{g.name}</span>
              </button>
            ))}
          </nav>
        </div>
      </Card>
      <Card>
        <div class="grid">
          {here.rows.map((row) => (
            <SettingLine key={row.key} row={row} value={valueOf(row)} change={change} note={noteOf(row)} />
          ))}
          {here.extra && (
            <div class="pt-4">
              <here.extra s={s} stored={stored} set={set} />
            </div>
          )}
        </div>
      </Card>
    </div>
  );
}

// One setting as a row, its control the kind the setting is: a switch, a
// choice of a few, a drop-down, or less and more.
function SettingLine({ row, value, change, note }) {
  const say = row.say ?? ((v) => String(v));
  const values = valuesOf(row);
  let control;
  if (row.kind === 'switch') {
    const on = row.flip ? !value : !!value;
    control = <Toggle label={row.label} on={on} onChange={(v) => change(row, row.flip ? +!v : +v)} />;
  } else if (row.kind === 'pair') {
    const words = values.map(say);
    control = <Segmented label={row.label} options={words} value={say(value)} onChange={(w) => change(row, values[words.indexOf(w)])} />;
  } else if (row.kind === 'list') {
    control = (
      <Select label={row.label} value={String(value)} onChange={(v) => change(row, typeof values[0] === 'number' ? +v : v)}>
        {!values.includes(value) && <option value={String(value)}>{value === undefined ? '--' : say(value)}</option>}
        {values.map((v) => (
          <option value={String(v)}>{say(v)}</option>
        ))}
      </Select>
    );
  } else {
    control = <Steps row={row} value={value} say={say} onChange={(v) => change(row, v)} />;
  }
  return (
    <SettingRow title={row.label} note={note} inline={row.kind === 'switch'}>
      {control}
    </SettingRow>
  );
}

// Holding less or more steps on: the first repeat after HOLD_MS, then one
// every REPEAT_MS. The radio is sent the value once it is still.
const HOLD_MS = 400;
const REPEAT_MS = 120;

// Less and more through a setting's own values, with the value between.
function Steps({ row, value, say, onChange }) {
  const [shown, setShown] = useState(null);
  const hold = useRef(null);
  const quiet = useQuiet((v) => {
    setShown(null);
    if (v !== value) onChange(v);
  });
  const now = shown ?? value;
  // Nothing is sent while a button is held; the wait starts at the release.
  const go = (d) => {
    const base = quiet.value ?? value;
    const values = valuesOf(row);
    const at = values.indexOf(base);
    // A value set elsewhere that is not in the list steps to the nearest
    // listed one in the way pressed, not back to the first.
    quiet.hold(at < 0 ? ((d > 0 ? values.find((v) => v > base) : values.findLast((v) => v < base)) ?? base) : values[Math.min(values.length - 1, Math.max(0, at + d))]);
    setShown(quiet.value);
  };
  const clear = () => {
    clearTimeout(hold.current);
    clearInterval(hold.current);
  };
  const stop = () => {
    clear();
    if (quiet.value !== null) quiet.later(quiet.value);
  };
  const start = (d) => {
    clear();
    go(d);
    hold.current = setTimeout(() => (hold.current = setInterval(() => go(d), REPEAT_MS)), HOLD_MS);
  };
  useEffect(() => clear, []);
  const button = (d, icon, name) => (
    <button
      type="button"
      aria-label={`${name} ${row.label}`}
      onPointerDown={(e) => e.button === 0 && start(d)}
      onPointerUp={stop}
      onPointerLeave={stop}
      onPointerCancel={stop}
      onClick={(e) => e.detail === 0 && (go(d), stop())}
      onContextMenu={(e) => e.preventDefault()}
      class="grid size-11 touch-none place-items-center text-accent-ink select-none"
    >
      <Icon name={icon} />
    </button>
  );
  return (
    <div class="flex h-11 items-center rounded-ctl border border-line bg-surface">
      {button(-1, 'minus', 'Less')}
      <span class="num flex-1 text-center" aria-label={row.label}>
        {now === undefined ? '--' : say(now)}
      </span>
      {button(1, 'plus', 'More')}
    </div>
  );
}

// The Custom theme's colours. The header colour is stored but drawn
// nowhere, so it is left out.
const COLOURS = [
  [0, 'Background'],
  [2, 'Tile and row'],
  [3, 'Radio'],
  [4, 'Broadcast'],
  [5, 'Measurement'],
  [6, 'Good'],
  [7, 'Fault'],
  [8, 'Dead'],
];

// A colour just picked is drawn by its API name, tc and its number, until a
// read brings it in `cst`.
function Colours({ stored, set }) {
  return (
    <div class="grid gap-2">
      <span class="label">The Custom theme's colours</span>
      <div class="grid grid-cols-1 gap-2 min-[360px]:grid-cols-2 md:grid-cols-4">
        {COLOURS.map(([i, name]) => (
          <label class="flex h-11 cursor-pointer items-center gap-2 rounded-ctl bg-raised px-2">
            <input type="color" value={stored[`tc${i}`] ?? stored.cst?.[i] ?? '#000000'} onChange={(e) => set({ [`tc${i}`]: e.currentTarget.value })} class="size-7 cursor-pointer rounded-ctl border-0 bg-transparent" aria-label={name} />
            <span class="truncate text-sm">{name}</span>
          </label>
        ))}
      </div>
      <span class="text-xs text-muted">These change only what the Custom theme looks like.</span>
    </div>
  );
}

// The volume knob's ends, measured: turn it from one end to the other.
function Knob({ s }) {
  const pcl = s.inp?.pcl;
  const [busy, setBusy] = useState(false);
  const run = async (act_) => {
    if (await act('/api/pot', { act: act_ })) setBusy(act_ === 'start');
  };
  return (
    <div class="grid gap-2">
      <span class="label">Volume knob ends</span>
      {busy ? (
        <>
          <p class="text-sm">Turn the volume knob all the way down and all the way up.</p>
          <p class="num text-sm text-muted">{pcl ? `Lowest ${pcl.min}, highest ${pcl.max}` : 'Waiting for the knob to move.'}</p>
          <div class="grid grid-cols-2 gap-2">
            <Button onClick={() => run('finish')}>Done</Button>
            <Button kind="secondary" onClick={() => run('cancel')}>
              Cancel
            </Button>
          </div>
        </>
      ) : (
        <Button kind="secondary" icon="volume-2" onClick={() => run('start')}>
          Measure the knob
        </Button>
      )}
    </div>
  );
}
