// DX: the station on the dial with DX mode and its width across the top;
// under it, as four tabs, the signal in detail, the band swept against its
// usual levels, the scanner, and the stations it caught, the same four pages
// the radio's own DX mode has.
import { useEffect, useRef, useState } from 'preact/hooks';
import { state } from '../data/radio.js';
import { catches, dxSweep } from '../data/reads.js';
import { freq, isFm, level, readings, when } from '../data/format.js';
import { act, say } from '../data/act.js';
import { before, copy, stationLine, tune, tuneBack } from '../data/commands.js';
import { bandAt, bandOf } from '../data/bands.js';
import { Bar, Button, Card, Chip, Figure, Icon, IconButton, Loading, Segmented, Select, Switch, Unread } from '../ui/kit.jsx';
import { AfList, BlockTile, RadioText, StationChips, StationNow, StationRow } from '../ui/station.jsx';
import { withMotion } from '../data/motion.js';
import { SweepButton, SweepChart, age, useAsked } from '../ui/sweep.jsx';
import { Radar } from '../ui/backs.jsx';
import { DxMeter, useMeter } from '../ui/meter.jsx';
import { KeysHelp, useKeys } from '../ui/keys.jsx';

// Where a scan goes, by the radio's number for it.
export const SCAN_RANGE = ['Band and presets', 'Whole band', 'Presets only'];

// DX mode's head comes in the live frame (`dx` in the state); the catches
// and the sweep are read only when it says they changed.
export function Dx() {
  const s = state.value;
  const head = s.dx;
  const caught = catches.use();
  const sweep = dxSweep.use();
  const m = useMeter(s);
  const typed = useKeys(null, { freqTyping: true, more: (e, st) => dxKey(e, st, send) });
  const [part, setPart] = useState('Scope');
  useWatchNote(head);
  if (!head) return <Card><Loading /></Card>;
  async function send(fields, said) {
    const took = await act('/api/dx', fields);
    if (took && said) say(said);
    return took;
  }
  const t = s.tun;
  const r = t.rds ?? {};
  const parts = {
    Scope: () => <Scope s={s} head={head} sweep={sweep.data} error={sweep.error} send={send} />,
    Scanner: () => <Scanner head={head} send={send} />,
    Catches: () => <Catches list={caught.data} error={caught.error} send={send} />,
  };
  // As Presets & Log: the station and its readings on the left; the meter,
  // DX mode, its width and the tabs on the right; the open tab under it.
  return (
    <div class="grid grid-cols-1 gap-4">
      <StationNow
        s={s}
        back={<Radar />}
        name={<Name r={r} />}
        chips={
          <div class="flex items-start justify-between gap-2">
            <StationChips s={s} />
            <StationTools t={t} />
          </div>
        }
        sub={[`PI ${piText(r)}`, r.ecc && `ECC ${r.ecc}`, r.cty, r.pty > 0 && r.ptn].filter(Boolean).join(' · ')}
        below={
          <>
            <RadioText r={r} class="" />
            <Signal t={t} />
          </>
        }
      >
        <DxMeter m={m} />
        <DxControls s={s} head={head} send={send} />
        {!isFm(t.bnd) && <p class="text-sm text-muted">DX mode works on FM and OIRT. Switch to FM first.</p>}
        <Segmented label="DX page" options={Object.keys(parts)} value={part} onChange={(p) => withMotion(() => setPart(p))} />
      </StationNow>
      {parts[part]()}
      <KeysHelp typedSlot={typed} typing="Tune" help={DX_HELP} />
    </div>
  );
}

// A station on the fmdx.org map, in a new tab, for an FM station whose PI is
// sure. It needs no position: the map shows every station on the frequency
// and finds the PI among them. The page cannot read the map's data itself,
// which is FMLIST's, so this is a link out.
function MapLink({ khz, band, pi }) {
  if (!isFm(band) || !pi) return null;
  const label = 'Find on the fmdx.org map';
  return <Button small kind="secondary" icon="map-pin" href={`https://maps.fmdx.org/#freq=${freq(khz, band)}&findPi=${pi}`} target="_blank" rel="noopener noreferrer" aria-label={label} title={label} />;
}

// Back to the frequency before, a copy of the station line, and the map, as
// icons at the end of the chips.
function StationTools({ t }) {
  const back = before.value;
  return (
    <div class="flex gap-2">
      <IconButton small icon="undo-2" label={back != null ? `Back to ${freq(back, bandAt(back))}` : 'Back'} disabled={back == null} onClick={tuneBack} />
      <IconButton small icon="copy" label="Copy the station" onClick={() => copy(stationLine(t))} />
      <MapLink khz={t.khz} band={t.bnd} pi={t.rds?.pi} />
    </div>
  );
}

const DX_HELP = 'Keys: type a frequency and press Enter, B goes back, ← and → step, Shift with them seeks, [ and ] the DX width, C copies the station, ↑ and ↓ the volume, M mutes.';

// DX's own keys, beside the ones every screen with keys has: B tunes back,
// [ and ] step the DX width, C copies the station line. True for a key it
// took.
function dxKey(e, st, send) {
  const k = e.key.toLowerCase();
  if (k === 'b') {
    if (before.value == null) say('Nothing to go back to yet.', 'bad');
    else tuneBack();
  } else if (k === '[' || k === ']') {
    if (!st.dx?.on) {
      say('Turn DX mode on to change its width.', 'bad');
      return true;
    }
    const widths = (bandOf('FM')?.widths ?? []).filter((w) => w > 0);
    const at = widths.indexOf(st.tun.dxw);
    const next = widths[Math.max(0, Math.min(widths.length - 1, at + (k === ']' ? 1 : -1)))];
    if (next && next !== st.tun.dxw) send({ khz: next }, `DX width ${next} kHz`);
  } else if (k === 'c') {
    copy(stationLine(st.tun));
  } else return false;
  return true;
}

// A note when the preset watch flags a preset come up, as the radio's own
// header says it for a moment: each alert once, by when it was flagged.
function useWatchNote(head) {
  const seen = useRef();
  useEffect(() => {
    if (!head) return;
    // The alert there when the page opens is old news: kept, not shown.
    const first = seen.current === undefined;
    if (!first && head.wms === seen.current) return;
    seen.current = head.wms;
    if (!first && head.wup) say(`${freq(head.wup, 'FM')} MHz up ${Math.round(head.wri / 10)} dB`);
  }, [head?.wms]);
}

// DX mode on or off beside its filter width, at one size. DX mode is FM
// only. A width picked while DX mode is off opens it, as the width is DX
// mode's own.
function DxControls({ s, head, send }) {
  const t = s.tun;
  const set = head.on && t.dxw;
  return (
    <div class="grid grid-cols-1 gap-2 @sm:grid-cols-2">
      <Switch label="DX mode" on={head.on} onChange={(on) => send({ on: on ? 1 : 0 })} />
      <Select label="DX width" value={set ? t.dxw : ''} onChange={(khz) => send(head.on ? { khz } : { on: 1, khz })}>
        {!set && <option value="">Width</option>}
        {(bandOf('FM')?.widths ?? []).filter((w) => w > 0).map((w) => (
          <option value={w}>Width {w} kHz</option>
        ))}
      </Select>
    </div>
  );
}

// The PI as the radio holds it: confirmed, or heard with a ? for each digit
// not yet sure, or 0000 from a station that sends no ID, or nothing yet.
function piText(r) {
  if (r.piz) return '0000';
  return r.pi ?? r.pih ?? '----';
}

// The station name as it builds up, a character at a time: the ones not yet
// heard are dim.
function Name({ r }) {
  if (r.ps) return <span>{r.ps.trim() || '---'}</span>;
  if (!r.psh) return <span class="text-muted">--------</span>;
  return (
    <span>
      {[...r.psh].map((c, i) => (
        <span class={r.psm & (1 << i) ? '' : 'text-muted'}>{r.psm & (1 << i) ? c : '-'}</span>
      ))}
    </span>
  );
}

// The signal in detail, in the station card under the radio text: blocks A
// to D of the last group beside the readings, then the other frequencies.
// The level is on the meter.
function Signal({ t }) {
  const q = readings(t.bnd, t);
  return (
    <div class="grid gap-4">
      <div class="grid items-start gap-4 @sm:grid-cols-[7rem_1fr]">
        <BlockTile r={t.rds ?? {}} />
        <div class="grid grid-cols-3 gap-2">
          {[q.snr, q.noise, q.wam, q.offset, q.width, q.mod].filter(Boolean).map((f) => (
            <Figure {...f} />
          ))}
        </div>
      </div>
      <AfList t={t} />
    </div>
  );
}

// The last sweep with its baseline, peaks and rise, as the shared chart
// draws it; the baseline is the median or a kept sweep.
function Scope({ s, head, sweep, error, send }) {
  const t = s.tun;
  const [asked, markAsked] = useAsked(sweep?.rev);
  const sweeping = head.swp || asked;
  // A sweep is at DX mode's width, so DX mode opens first; the radio takes
  // the two as separate requests.
  async function sweepNow() {
    if (!head.on && !(await send({ on: 1 }))) return;
    if (await send({ sweep: 1 }, 'Sweeping the band, about 4 s.')) markAsked();
  }
  const button = <SweepButton sweeping={sweeping} onClick={sweepNow} />;
  if (!sweep?.level) {
    return (
      <Card title="Scope" end={button}>
        {!sweep && error ? <Unread what="sweep" /> : <p class="text-muted">No sweep yet. Sweep the band.</p>}
      </Card>
    );
  }
  const kept = sweep.kept ?? [];
  const labelOf = (when_) => (when_ ? new Date(when_ * 1000).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) : 'no time');
  return (
    <Card title={`Scope · ${sweeping ? 'sweeping' : age(sweep)}`} end={button}>
      <SweepChart sweep={sweep} band={t.bnd} dialKhz={t.khz} cursorKhz={s.pnl?.cur} dim={sweeping} onTune={tune} />
      <div class="mt-4 grid grid-cols-1 gap-2 sm:grid-cols-[1fr_auto] sm:items-center">
        <Select label="Baseline" value={String(sweep.baseline_kept ?? 0)} onChange={(v) => send({ baseline: v === '0' ? 'auto' : v }, 'Baseline changed.')}>
          <option value="0">Baseline: median of {sweep.baseline_sweeps ?? kept.length}</option>
          {kept.map((k, i) => (
            <option value={String(i + 1)}>Baseline: sweep of {labelOf(k)}</option>
          ))}
          {sweep.baseline_kept === -1 && <option value="-1">Baseline: a fixed sweep</option>}
        </Select>
        <span class="num text-sm text-muted">
          Floor {level(sweep.floor)} · width {sweep.width} kHz
        </span>
      </div>
    </Card>
  );
}

// The scanner: where it is, how far it has got, what it found; start, stop
// and Learn Local Stations.
function Scanner({ head, send }) {
  const running = head.scn === 'running';
  const pct = head.sct ? Math.round((head.scd / head.sct) * 100) : 0;
  const where = head.sck ? `${freq(head.sck, 'FM')} MHz` : '';
  const says = running ? `Scanning ${where}` : head.scn === 'stopped' ? `Stopped on ${where}` : 'Ready';
  // What a scan does, as the radio's own scanner page says it.
  const how = [SCAN_RANGE[head.dsc], ['stops on new stations', 'stops on any PI', 'never stops'][head.dst], head.ddw && `${(head.ddw / 10).toFixed(1)} s a channel`].filter(Boolean).join(' · ');
  return (
    <Card title="Scanner" end={<span class="num text-sm text-muted">{head.scf} found</span>}>
      <p class="text-lg font-semibold">{says}</p>
      <p class="text-sm text-muted">{how}</p>
      <Bar pct={pct} label="Scan progress" class="mt-2" />
      <p class="num mt-1 text-sm text-muted">
        {head.scd} of {head.sct || '--'} channels
      </p>
      {/* One above the other on a phone, where side by side cuts the words. */}
      <div class="mt-4 grid grid-cols-1 gap-2 sm:grid-cols-2">
        {running ? (
          <Button icon="x" onClick={() => send({ scan: 0 }, 'Scan stopped.')}>
            Stop
          </Button>
        ) : (
          <Button icon="satellite-dish" onClick={() => send(head.on ? { scan: 1 } : { on: 1, scan: 1 }, head.scn === 'stopped' ? 'Scan goes on.' : 'Scan started.')}>
            {head.scn === 'stopped' ? 'Go on' : 'Start scan'}
          </Button>
        )}
        <Button kind="secondary" icon="list-music" onClick={() => send({ learn: 1 }, 'Learning the local stations.')}>
          Learn locals
        </Button>
      </div>
    </Card>
  );
}

// The stations caught in DX mode, newest first. A tap tunes there; Log
// writes one to the station log. `list` is null while not known.
function Catches({ list, error, send }) {
  return (
    <Card
      title={list ? `Catches · ${list.length}` : 'Catches'}
      end={
        <Button small kind="secondary" href="/api/dx.csv" download="dx.csv" icon="download">
          FMLIST CSV
        </Button>
      }
    >
      {!list && (error ? <Unread what="catches" /> : <Loading />)}
      {list?.length === 0 && <p class="text-muted">No catches yet. A station whose PI is confirmed in DX mode is caught.</p>}
      {list?.map((c) => (
        <StationRow
          e={c}
          tag={c.new && <Chip tone="good">New</Chip>}
          onClick={() => tune(c.khz)}
          after={
            <span class="flex items-center gap-2">
              {/* On a phone the row has no room beside the name: a tap
                  tunes there, and the station card has the map. */}
              <span class="hidden sm:flex">
                <MapLink khz={c.khz} band={c.band} pi={c.pi} />
              </span>
              {c.logged ? (
                <span class="flex items-center gap-1 text-sm text-muted">
                  <Icon name="circle-check" size={16} />
                  Logged
                </span>
              ) : (
                <Button kind="secondary" onClick={() => send({ logid: c.id }, `Logged ${freq(c.khz, c.band)}.`)}>
                  Log
                </Button>
              )}
            </span>
          }
        >
          <span class="num text-sm text-muted">
            PI {c.pi}
            {c.country ? ` · ${c.country}` : ''} · {level(c.level_dbuv)} · ×{c.count} · {c.real ? when(c.time) : 'clock not set'}
          </span>
        </StationRow>
      ))}
    </Card>
  );
}
