// DX: the station on the dial with DX mode and its width across the top;
// under it, as four tabs, the signal in detail, the band swept against its
// usual levels, the scanner, and the stations it caught, the same four pages
// the radio's own DX mode has.
import { useEffect, useRef, useState } from 'preact/hooks';
import { state } from '../data/radio.js';
import { catches, dxSweep } from '../data/reads.js';
import { dbuv, freq, isFm, readings, when } from '../data/format.js';
import { act, say } from '../data/act.js';
import { tune } from '../data/commands.js';
import { bandOf } from '../data/bands.js';
import { Bar, Button, Card, Chip, Figure, Icon, Loading, Segmented, Select, Switch, Unread } from '../ui/kit.jsx';
import { BlockTile, RadioText, StationNow, StationRow } from '../ui/station.jsx';
import { withMotion } from '../data/motion.js';
import { SweepButton, SweepChart, age, useAsked } from '../ui/sweep.jsx';
import { LevelHistory, useLevelHistory } from '../ui/level.jsx';
import { Radar } from '../ui/backs.jsx';

// Where a scan goes, by the radio's number for it.
export const SCAN_RANGE = ['Band and presets', 'Whole band', 'Presets only'];

// DX mode's head comes in the live frame (`dx` in the state); the catches
// and the sweep are read only when it says they changed.
export function Dx() {
  const s = state.value;
  const head = s.dx;
  const caught = catches.use();
  const sweep = dxSweep.use();
  const history = useLevelHistory(s.tun);
  const [part, setPart] = useState('Signal');
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
    Signal: () => <Signal s={s} history={history} />,
    Scope: () => <Scope s={s} head={head} sweep={sweep.data} error={sweep.error} send={send} />,
    Scanner: () => <Scanner head={head} send={send} />,
    Catches: () => <Catches list={caught.data} error={caught.error} send={send} />,
  };
  // As Presets & Log: the station on the left, DX mode, its width and the
  // tabs on the right, the open tab under it.
  return (
    <div class="grid grid-cols-1 gap-4">
      <StationNow s={s} back={<Radar />} name={<Name r={r} />} sub={[`PI ${piText(r)}`, r.cty, r.pty > 0 && r.ptn].filter(Boolean).join(' · ')}>
        <DxControls s={s} head={head} send={send} />
        {!isFm(t.bnd) && <p class="text-sm text-muted">DX mode works on FM and OIRT. Switch to FM first.</p>}
        <Segmented label="DX page" options={Object.keys(parts)} value={part} onChange={(p) => withMotion(() => setPart(p))} />
      </StationNow>
      {parts[part]()}
    </div>
  );
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
// only.
function DxControls({ s, head, send }) {
  const t = s.tun;
  const set = head.on && t.dxw;
  return (
    <div class="grid grid-cols-1 gap-2 @sm:grid-cols-2">
      <Switch label="DX mode" on={head.on} onChange={(on) => send({ on: on ? 1 : 0 })} />
      <Select label="DX width" value={set ? t.dxw : ''} onChange={(khz) => send({ khz })}>
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

// The Signal tab: blocks A to D of the last group beside the readings,
// the radio text, and the level of the last minute.
function Signal({ s, history }) {
  const t = s.tun;
  const r = t.rds ?? {};
  const q = readings(t.bnd, t);
  return (
    <Card title="Signal" class="@container">
      <div class="grid gap-4">
        <div class="grid grid-cols-1 items-start gap-4 @xs:grid-cols-[7rem_1fr]">
          <BlockTile r={r} />
          <div class="grid grid-cols-2 gap-x-2 gap-y-2 @md:grid-cols-3">
            {[q.level, q.noise, q.wam, q.offset, q.width, q.mod].filter(Boolean).map((f) => (
              <Figure {...f} />
            ))}
          </div>
        </div>
        <RadioText r={r} class="" />
        <LevelHistory levels={history} class="h-28" />
      </div>
    </Card>
  );
}

// The last sweep with its baseline, peaks and rise, as the shared chart
// draws it; the baseline is the median or a kept sweep.
function Scope({ s, head, sweep, error, send }) {
  const t = s.tun;
  const [asked, markAsked] = useAsked(sweep?.rev);
  const sweeping = head.swp || asked;
  async function sweepNow() {
    if (await send({ sweep: 1 }, 'Sweeping the band, about 4 s.')) markAsked();
  }
  const button = <SweepButton sweeping={sweeping} onClick={sweepNow} />;
  if (!sweep?.level) {
    return (
      <Card title="Scope" end={button}>
        {!sweep && error ? <Unread what="sweep" /> : <p class="text-muted">No sweep yet. Open DX mode and sweep the band.</p>}
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
          Floor {sweep.floor != null ? dbuv(sweep.floor) : '--'} dBµV · width {sweep.width} kHz
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
      <div class="mt-4 grid grid-cols-2 gap-2">
        {running ? (
          <Button icon="x" onClick={() => send({ scan: 0 }, 'Scan stopped.')}>
            Stop
          </Button>
        ) : (
          <Button icon="satellite-dish" onClick={() => send({ scan: 1 }, head.scn === 'stopped' ? 'Scan goes on.' : 'Scan started.')}>
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
            c.logged ? (
              <span class="flex items-center gap-1 text-sm text-muted">
                <Icon name="circle-check" size={16} />
                Logged
              </span>
            ) : (
              <Button kind="secondary" onClick={() => send({ logid: c.id }, `Logged ${freq(c.khz, c.band)}.`)}>
                Log
              </Button>
            )
          }
        >
          <span class="num text-sm text-muted">
            PI {c.pi}
            {c.country ? ` · ${c.country}` : ''} · {dbuv(c.level_dbuv)} dBµV · ×{c.count} · {c.real ? when(c.time) : 'clock not set'}
          </span>
        </StationRow>
      ))}
    </Card>
  );
}
