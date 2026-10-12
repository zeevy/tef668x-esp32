// Station log, the Log tab of Presets & Log: every station logged, newest
// first, in a card for each day; log the one on the dial, search, see every
// reading, rename, write a note, tune to it, delete it, download it.
import { useState } from 'preact/hooks';
import { state } from '../data/radio.js';
import { logFits, stationLog } from '../data/reads.js';
import { logStation, tune } from '../data/commands.js';
import { country, freq, isFm, printable, readings, unit } from '../data/format.js';
import { LOG_MAX, LOG_NAME_MAX, LOG_NOTE_MAX } from '../data/limits.js';
import { act, say } from '../data/act.js';
import { Button, Card, Figure, Icon, INPUT, Loading, Modal, Unread } from '../ui/kit.jsx';
import { StationNow, StationRow } from '../ui/station.jsx';
import { Stars } from '../ui/backs.jsx';

// Every reading of an entry, as the screens show a station's.
function entryReadings(e) {
  const r = readings(e.band, { sig: e.level_dbuv, snr: e.snr, usn: e.usn, wam: isFm(e.band) ? e.multipath : e.cochannel, bw: e.bw_khz });
  return [{ label: 'Band', value: e.band }, r.level, r.snr, r.noise, r.wam, r.width, { label: 'Sound', value: e.stereo ? 'Stereo' : 'Mono' }, { label: 'PI', value: e.pi ?? '---' }].filter(Boolean);
}

// What RDS said of the station besides its name and PI, for the dialog: each
// only when the radio kept it with the entry.
function entryRds(e) {
  return [e.ptn && { label: 'Programme type', value: e.ptn }, e.cty && { label: 'Country', value: country(e.cty) }].filter(Boolean);
}

// When an entry was logged: the date and time, or the time alone under its
// day's heading, or, when the radio's clock was not set then, how long it
// had been on.
function whenText(e, withDate = true) {
  if (!e.real) return `${Math.round(e.time / 60000)} min after start`;
  const time = { hour: '2-digit', minute: '2-digit' };
  return new Date(e.time * 1000).toLocaleString([], withDate ? { day: 'numeric', month: 'short', ...time } : time);
}

function dayOf(e, now = new Date()) {
  if (!e.real) return 'Clock not set';
  const d = new Date(e.time * 1000);
  const days = Math.round((new Date(now.toDateString()) - new Date(d.toDateString())) / 86400000);
  if (days === 0) return 'Today';
  if (days === 1) return 'Yesterday';
  return d.toLocaleDateString([], { day: 'numeric', month: 'long', year: d.getFullYear() === now.getFullYear() ? undefined : 'numeric' });
}

export function StationLog({ tabs }) {
  const { data: log, error } = stationLog.use();
  const [open, setOpen] = useState(null);
  const [query, setQuery] = useState('');

  const logThis = async () => (await logStation()) && stationLog.again();

  const q = query.trim().toLowerCase();
  const shown = (log ?? [])
    .filter((e) => !q || [freq(e.khz, e.band), e.band, e.name, e.pi, e.note, e.rt].some((v) => v && String(v).toLowerCase().includes(q)))
    .reverse();

  return (
    <div class="grid grid-cols-1 gap-4">
      <LiveStation back={<Stars />} sub={log && `${log.length} of ${LOG_MAX} logged`}>
        <div class="grid grid-cols-2 gap-2">
          <Button icon="plus" aria-label="Log this station" class="px-3 text-sm sm:text-base" onClick={logThis}>
            Log this
          </Button>
          <Button kind="secondary" href="/api/log.csv" download="logbook.csv" class="px-3 text-sm sm:text-base">
            <Icon name="download" class="hidden sm:block" />
            Download
          </Button>
        </div>
        <input aria-label="Search" type="search" placeholder="Search name, frequency, note" value={query} onInput={(e) => setQuery(e.currentTarget.value)} class={INPUT} />
        {tabs}
      </LiveStation>
      {!logFits.value ? (
        <Card>
          <p class="text-muted">The station log is not available: its storage did not start.</p>
        </Card>
      ) : log === null ? (
        <Card>{error ? <Unread what="log" /> : <Loading />}</Card>
      ) : log.length === 0 ? (
        <Card>
          <p class="text-muted">Nothing logged yet. Log the station you are on, or hold ENTER on the radio.</p>
        </Card>
      ) : (
        <ByDay entries={shown} open={setOpen} total={log.length} />
      )}
      {open && <EntryDialog entry={open} close={() => setOpen(null)} saved={stationLog.again} />}
    </div>
  );
}

// The station card, the one part of this screen that follows the live
// state, so a new state draws it again and leaves the list alone.
function LiveStation(props) {
  return <StationNow s={state.value} {...props} />;
}

// Each reading a small chip, wrapping as the width needs.
function Pills({ list }) {
  return (
    <span class="flex flex-wrap gap-1">
      {list.map(({ label, value, unit }) => (
        <span class="inline-flex h-6 items-center gap-1 rounded-ctl bg-raised px-2 text-xs">
          <span class="text-muted">{label}</span>
          <span class="num font-semibold">
            {value}
            {unit && ` ${unit}`}
          </span>
        </span>
      ))}
    </span>
  );
}

// The rows under a heading for each day, newest day first.
function ByDay({ entries, open, total }) {
  const days = new Map();
  for (const e of entries) {
    const d = dayOf(e);
    if (!days.has(d)) days.set(d, []);
    days.get(d).push(e);
  }
  return (
    <>
      <p class="text-sm text-muted">
        {entries.length} of {total}
      </p>
      {[...days].map(([day, list]) => (
        <Card title={`${day} · ${list.length}`}>
          {list.map((e) => (
            <StationRow e={e} onClick={() => open(e)} time={whenText(e, false)} tag={e.renamed && <span class="hidden text-sm text-muted sm:inline">renamed</span>}>
              <Pills list={entryReadings(e)} />
              {e.note && <span class="truncate text-sm italic">{e.note}</span>}
            </StationRow>
          ))}
        </Card>
      ))}
    </>
  );
}

// Every reading of one entry, its name and note to change, tune to it, delete.
function EntryDialog({ entry: e, close, saved }) {
  const [name, setName] = useState(e.name?.trim() ?? '');
  const [note, setNote] = useState(e.note ?? '');
  const editable = e.id !== undefined;
  async function save(ev) {
    ev.preventDefault();
    // The name only when it was changed, so a note alone does not mark a
    // received name as typed.
    const fields = { do: 'edit', id: e.id, time: e.time, khz: e.khz, note };
    if (name !== (e.name?.trim() ?? '')) fields.name = name;
    if (await act('/api/log', fields)) {
      say('Entry changed.');
      saved();
      close();
    }
  }
  async function remove() {
    if (await act('/api/log', { do: 'delete', id: e.id, time: e.time, khz: e.khz })) {
      say('Entry deleted.');
      saved();
      close();
    }
  }
  const tuneHere = async () => (await tune(e.khz)) && close();
  return (
    <Modal
      title={`${freq(e.khz, e.band)} ${unit(e.band)}`}
      size="lg"
      onClose={close}
      onSubmit={save}
      danger={editable && <Button kind="danger" icon="trash" onClick={remove}>Delete</Button>}
      actions={
        <>
          <Button kind="secondary" icon="radio" onClick={tuneHere}>Tune</Button>
          {editable && <Button type="submit">Save</Button>}
        </>
      }
    >
      <p class="text-sm text-muted">{whenText(e)}</p>
      {e.rt && <p class="text-sm">{e.rt}</p>}
      <div class="grid grid-cols-3 gap-4">
        {[...entryReadings(e), ...entryRds(e)].map((f) => (
          <Figure {...f} />
        ))}
      </div>
      <div class="grid grid-cols-1 gap-4 sm:grid-cols-3">
        <label class="grid gap-1">
          <span class="label">{e.renamed ? 'Name, typed' : 'Name, as sent'}</span>
          <input aria-label="Name" maxLength={LOG_NAME_MAX} value={name} disabled={!editable} onInput={(ev) => setName(printable(ev.currentTarget.value, LOG_NAME_MAX))} class={INPUT} />
        </label>
        <label class="grid gap-1 sm:col-span-2">
          <span class="label">Note, up to {LOG_NOTE_MAX} letters</span>
          <input aria-label="Note" maxLength={LOG_NOTE_MAX} value={note} disabled={!editable} onInput={(ev) => setNote(printable(ev.currentTarget.value, LOG_NOTE_MAX))} class={INPUT} />
        </label>
      </div>
      {!editable && <p class="text-sm text-muted">This radio's firmware cannot change or delete an entry yet.</p>}
    </Modal>
  );
}
