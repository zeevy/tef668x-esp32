// Presets, the Presets tab of Presets & Log: the station on now with the
// tools and a band filter across the top; under it keys like a radio's own
// preset buttons. Recall, add, change, rename, delete, download and upload.
import { useRef, useState } from 'preact/hooks';
import { change, postBody, state } from '../data/radio.js';
import { presets } from '../data/reads.js';
import { bandAt, bandOf } from '../data/bands.js';
import { BANDS, country, freq, printable, typedKhz, unit, when } from '../data/format.js';
import { PRESET_NAME_MAX, PRESET_SLOTS } from '../data/limits.js';
import { act, refused, say } from '../data/act.js';
import { AddKey, PresetKey } from '../ui/keys.jsx';
import { recall, storeStation } from '../data/commands.js';
import { withMotion } from '../data/motion.js';
import { Banner, Button, Card, Figure, FileButton, Icon, INPUT, Loading, Modal, Segmented, Select, Unread } from '../ui/kit.jsx';
import { StationNow } from '../ui/station.jsx';
import { Stars } from '../ui/backs.jsx';

const firstFreeSlot = (list) => {
  const used = new Set(list.map((p) => p.slot));
  for (let i = 1; i <= PRESET_SLOTS; i++) if (!used.has(i)) return i;
  return null;
};

// The largest body the radio takes in one request, in bytes; it refuses a
// bigger one.
const BODY_MAX = 16384;

export function Presets({ tabs }) {
  const s = state.value;
  // Null while not known: not read yet, or the read failed (`error`).
  const { data: list, error } = presets.use();
  const [editing, setEditing] = useState(null); // a preset, or { slot } for a new one
  const [upload, setUpload] = useState(null); // the CSV picked, until merge or replace is chosen
  const [editMode, setEditMode] = useState(false);
  const [band, setBand] = useState('All');

  // Drawn at once, the two swapped in the list, then read back from the radio.
  async function move(from, to) {
    presets.set(list.map((p) => (p.slot === from ? { ...p, slot: to } : p.slot === to ? { ...p, slot: from } : p)).sort((a, b) => a.slot - b.slot));
    await act('/api/presets', { do: 'move', slot: from, to });
    presets.again();
  }

  const t = s?.tun;
  const on = list?.find((p) => p.slot === t?.pst);
  // The filter offers only the bands that hold presets, and goes when there
  // is one band or none. Change mode shows every slot, whatever the filter.
  const bands = ['All', ...BANDS.filter((b) => list?.some((p) => p.band === b))];
  const showing = bands.includes(band) ? band : 'All';
  const shown = (list ?? []).filter((p) => showing === 'All' || p.band === showing);
  // The radio's own word on its list: a write to flash that failed, and
  // presets it could not read at start up.
  const lost = s?.pst?.lost;
  return (
    <div class="grid grid-cols-1 gap-4">
      {s?.pst?.bad && <Banner>The radio could not save the preset list. A change may be lost when it is turned off.</Banner>}
      {lost > 0 && <Banner>{lost === 1 ? '1 preset' : `${lost} presets`} could not be read at start up and {lost === 1 ? 'was' : 'were'} left out.</Banner>}
      <StationNow s={s} back={<Stars />} presetName={on?.name?.trim()} sub={list && `${list.length} of ${PRESET_SLOTS} slots used`}>
        {/* On a phone the four tools are two by two, with no icons, so they
            fit at 320 px. */}
        <div class="grid grid-cols-2 gap-2">
          <Button aria-label="Save current station" class="px-3 text-sm sm:text-base" onClick={() => s && storeStation(s.tun)}>
            <Icon name="star" class="hidden sm:block" />
            Save current
          </Button>
          <Button kind="secondary" class="px-3 text-sm sm:text-base" disabled={!list} onClick={() => setEditing({ slot: firstFreeSlot(list) ?? 1, isNew: true, offered: true })}>
            <Icon name="plus" class="hidden sm:block" />
            Add
          </Button>
          <Button kind="secondary" href="/api/presets.csv" download="presets.csv" class="px-3 text-sm sm:text-base">
            <Icon name="download" class="hidden sm:block" />
            Download
          </Button>
          <FileButton
            accept=".csv,text/csv"
            class="text-sm sm:text-base"
            onFile={async (file) => {
              if (!file.size) return say('That file is empty.', 'bad');
              if (file.size > BODY_MAX) return say(`${file.name} is ${Math.ceil(file.size / 1024)} KB; the radio takes a preset file of up to 16 KB.`, 'bad');
              setUpload({ name: file.name, text: await file.text() });
            }}
          >
            <Icon name="upload" class="hidden sm:block" />
            Upload
          </FileButton>
        </div>
        {tabs}
      </StationNow>
      {list === null ? (
        <Card>
          {error ? (
            <div class="flex flex-wrap items-center justify-between gap-2">
              <Unread what="presets" />
              <Button kind="secondary" icon="refresh-cw" onClick={presets.again}>
                Try again
              </Button>
            </div>
          ) : (
            <Loading />
          )}
        </Card>
      ) : list.length === 0 ? (
        <Card>
          <p class="text-muted">No presets stored yet. Add one, or store the station you are on.</p>
        </Card>
      ) : (
        <Keys
          list={editMode ? list : shown}
          title={editMode || showing === 'All' ? `${list.length} stored` : `${shown.length} on ${showing}`}
          current={t?.pst}
          edit={setEditing}
          add={(slot) => setEditing({ slot, isNew: true })}
          move={move}
          editMode={editMode}
          setEditMode={setEditMode}
        >
          {bands.length > 2 && <Segmented label="Band" options={bands} value={showing} onChange={(b) => withMotion(() => setBand(b))} />}
        </Keys>
      )}
      {editing && <PresetDialog preset={editing} list={list ?? []} close={() => setEditing(null)} saved={presets.again} />}
      {upload && <UploadDialog upload={upload} close={() => setUpload(null)} saved={presets.again} />}
    </div>
  );
}

// A tap recalls. In Change mode a tap opens a preset, a tap on an empty slot
// adds one there, and a drag moves a preset: onto another the two swap, onto
// an empty slot it goes there. Pointer events, so a finger drags as a mouse
// does; the browser's own drag does not work on a phone.
// `children`, the band filter, goes over the keys outside Change mode.
function Keys({ list, title, current, edit, add, move, editMode, setEditMode, children }) {
  const [drag, setDrag] = useState(null); // { from, x, y, over }
  const dragged = useRef(false);
  const bySlot = new Map(list.map((p) => [p.slot, p]));
  // In Change mode every slot up to one past the last used, so a gap or the
  // next free slot can be dropped on.
  const last = Math.min(PRESET_SLOTS, Math.max(0, ...list.map((p) => p.slot)) + 1);
  const slots = editMode ? Array.from({ length: last }, (_, i) => i + 1) : list.map((p) => p.slot);
  const slotUnder = (x, y) => {
    const el = document.elementFromPoint(x, y)?.closest('[data-slot]');
    return el ? Number(el.dataset.slot) : null;
  };

  function down(e, p) {
    if (!editMode) return;
    const key = e.currentTarget;
    const start = [e.clientX, e.clientY];
    dragged.current = false;
    key.setPointerCapture(e.pointerId);
    const onMove = (m) => {
      if (!dragged.current && Math.hypot(m.clientX - start[0], m.clientY - start[1]) < 8) return;
      dragged.current = true;
      setDrag({ from: p.slot, x: m.clientX, y: m.clientY, over: slotUnder(m.clientX, m.clientY) });
      // The page moves under the finger near its top and bottom edges.
      if (m.clientY < 64) scrollBy(0, -12);
      else if (m.clientY > innerHeight - 64) scrollBy(0, 12);
    };
    const onEnd = (u) => {
      key.removeEventListener('pointermove', onMove);
      key.removeEventListener('pointerup', onEnd);
      key.removeEventListener('pointercancel', onEnd);
      setDrag(null);
      const to = u.type === 'pointerup' && dragged.current ? slotUnder(u.clientX, u.clientY) : null;
      if (to && to !== p.slot) move(p.slot, to);
    };
    key.addEventListener('pointermove', onMove);
    key.addEventListener('pointerup', onEnd);
    key.addEventListener('pointercancel', onEnd);
  }

  return (
    <Card
      title={title}
      end={
        <button type="button" aria-pressed={editMode} onClick={() => withMotion(() => setEditMode(!editMode))} class="hit relative text-sm font-semibold text-accent-ink">
          {editMode ? 'Done' : 'Change'}
        </button>
      }
    >
      {editMode && <p class="mb-4 text-sm text-muted">Drag a preset onto another slot to move it. Tap one to change it, or an empty slot to add one.</p>}
      {!editMode && children && <div class="mb-4">{children}</div>}
      <div class="grid grid-cols-3 gap-2 sm:grid-cols-4 lg:grid-cols-6">
        {slots.map((n) => {
          const p = bySlot.get(n);
          const over = drag?.over === n && drag.from !== n ? 'ring-2 ring-accent' : '';
          if (!p) {
            return (
              <AddKey data-slot={n} aria-label={`Add a preset in slot ${n}`} onClick={() => add(n)} class={over}>
                <span class="num">{n}</span>
              </AddKey>
            );
          }
          const on = p.slot === current;
          return (
            <PresetKey
              p={p}
              on={on}
              data-slot={n}
              aria-label={editMode ? `Change preset ${p.slot}` : undefined}
              onPointerDown={(e) => down(e, p)}
              onClick={() => {
                // The click that ends a drag is not a tap. Cleared here, so
                // the next tap counts in or out of Change mode.
                if (dragged.current) {
                  dragged.current = false;
                  return;
                }
                editMode ? edit(p) : recall(p);
              }}
              class={`${editMode ? 'touch-none select-none' : ''} ${drag?.from === n ? 'opacity-40' : ''} ${over}`}
            >
              {editMode && <Icon name="pencil" size={16} class="absolute right-3 bottom-2" />}
            </PresetKey>
          );
        })}
      </div>
      {drag && (
        <div
          class="num pointer-events-none fixed z-30 -translate-x-1/2 -translate-y-1/2 rounded-ctl bg-accent px-3 py-2 font-semibold text-on-accent shadow-lg"
          style={{ left: `${drag.x}px`, top: `${drag.y}px` }}
        >
          {freq(bySlot.get(drag.from).khz, bySlot.get(drag.from).band)}
        </div>
      )}
    </Card>
  );
}

// What the radio learnt or kept of a preset's station, each only when it
// has it: the PI, the programme type, the country and when it was saved.
function PresetFacts({ p }) {
  const facts = [
    p.pi && { label: 'PI', value: p.pi },
    p.ptn && { label: 'Programme type', value: p.ptn },
    p.cty && { label: 'Country', value: country(p.cty) },
    p.saved > 0 && { label: 'Saved', value: when(p.saved) },
  ].filter(Boolean);
  if (!facts.length) return null;
  return (
    <div class="grid grid-cols-2 gap-4">
      {facts.map((f) => (
        <Figure {...f} />
      ))}
    </div>
  );
}

// Add or change one preset: slot, frequency, width and name; Delete for one
// that is stored. Another slot for a stored preset moves it there, so a
// preset moves without a drag: onto a taken slot the two swap. A new preset
// in the slot Add offered goes where the radio puts it, the lowest free slot
// of the list it holds, so a list read before cannot make it write over one;
// a slot typed, or tapped in Change mode, is written to.
function PresetDialog({ preset, list, close, saved }) {
  const isNew = !!preset.isNew;
  const [slot, setSlot] = useState(String(preset.slot));
  const [chosen, setChosen] = useState(!preset.offered);
  const [typed, setTyped] = useState(preset.khz ? freq(preset.khz, preset.band) : '');
  const [bw, setBw] = useState(String(preset.bw ?? 0));
  const [name, setName] = useState(preset.name ?? '');
  const [error, setError] = useState('');
  const khz = typedKhz(typed);
  // The preset's own band while its frequency is the same, so 68 MHz on FM
  // stays FM; else the band the radio puts the frequency on.
  const band = !khz ? null : khz === preset.khz ? preset.band : bandAt(khz);
  // The widths the band offers, from the radio's band plan, with 0 for the
  // radio's own choice. While the plan is not known, the preset's own width,
  // so a rename keeps it.
  const offered = bandOf(band)?.widths ?? (preset.bw ? [preset.bw] : []);
  const widths = offered.includes(0) ? offered : [0, ...offered];
  const n = Number(slot);
  const moving = !isNew && n !== preset.slot;
  const taken = chosen && (isNew || moving) && list.find((p) => p.slot === n);
  const twin = khz && list.find((p) => p.slot !== preset.slot && p.band === band && p.khz === khz);

  async function save(e) {
    e.preventDefault();
    if (!Number.isInteger(n) || n < 1 || n > PRESET_SLOTS) return setError(`A slot is 1 to ${PRESET_SLOTS}.`);
    if (!khz) return setError('Type a frequency, such as 98.3 or 738.');
    if (twin) return setError(`${freq(khz, band)} ${unit(band)} is already preset ${twin.slot}.`);
    const fields = { ...(chosen ? { do: 'set', slot: n } : { do: 'add' }), khz, ...(band && { bnd: band }), bw: widths.includes(+bw) ? +bw : 0, name };
    // Moved first, then the rest written in its new slot.
    if (moving && !(await act('/api/presets', { do: 'move', slot: preset.slot, to: n }))) return;
    try {
      say(await change('/api/presets', fields));
    } catch (x) {
      refused(x);
      // The dialog stays open, unless the move took: its slot is then out
      // of date.
      if (!moving) return;
    }
    saved();
    close();
  }
  async function remove() {
    if (await act('/api/presets', { do: 'clear', slot: preset.slot })) {
      say(`Preset ${preset.slot} deleted.`);
      saved();
      close();
    }
  }
  return (
    <Modal
      title={isNew ? 'Add a preset' : `Preset ${preset.slot}`}
      onClose={close}
      onSubmit={save}
      danger={!isNew && <Button kind="danger" icon="trash" onClick={remove}>Delete</Button>}
      actions={<Button type="submit">Save</Button>}
    >
      <div class="grid grid-cols-3 gap-4">
        <label class="grid gap-1">
          <span class="label">Slot</span>
          <input
            aria-label="Slot"
            inputmode="numeric"
            value={slot}
            onInput={(e) => {
              setSlot(e.currentTarget.value.replace(/\D/g, ''));
              setChosen(true);
            }}
            class={`num ${INPUT}`}
          />
        </label>
        <label class="col-span-2 grid gap-1">
          <span class="label">Frequency</span>
          <input aria-label="Frequency" inputmode="decimal" placeholder="98.3 or 738" value={typed} onInput={(e) => setTyped(e.currentTarget.value)} class={`num ${INPUT}`} />
        </label>
      </div>
      <p class="text-sm text-muted">{!khz ? 'MHz with a point, such as 98.3; kHz without, such as 738.' : band ? `${band}, ${freq(khz, band)} ${unit(band)}` : 'That frequency is in no band.'}</p>
      {taken && (
        <p class="text-sm text-warn">
          Slot {n} holds {freq(taken.khz, taken.band)} {unit(taken.band)}; {isNew ? 'saving replaces it.' : `saving swaps the two, so that one goes to slot ${preset.slot}.`}
        </p>
      )}
      <label class="grid gap-1">
        <span class="label">Name, up to {PRESET_NAME_MAX} letters</span>
        <input aria-label="Name" maxLength={PRESET_NAME_MAX} value={name} onInput={(e) => setName(printable(e.currentTarget.value, PRESET_NAME_MAX))} class={INPUT} />
      </label>
      <div class="grid gap-1">
        <span class="label">Bandwidth</span>
        <Select label="Bandwidth" value={widths.includes(+bw) ? bw : '0'} onChange={setBw}>
          {widths.map((k) => (
            <option value={k}>{k === 0 ? 'The radio chooses' : `${k} kHz`}</option>
          ))}
        </Select>
      </div>
      {!isNew && <PresetFacts p={preset} />}
      {error && <p class="text-sm text-bad">{error}</p>}
    </Modal>
  );
}

// A picked CSV file: merged into the list, or put in place of it.
function UploadDialog({ upload, close, saved }) {
  const [busy, setBusy] = useState(false);
  async function send(mode) {
    setBusy(true);
    try {
      say(await postBody(`/api/presets/import?mode=${mode}`, upload.text, 'text/csv'));
      saved();
      close();
    } catch (e) {
      refused(e);
    }
    setBusy(false);
  }
  return (
    <Modal
      title={`Upload ${upload.name}`}
      onClose={close}
      danger={<Button kind="danger" busy={busy} onClick={() => send('replace')}>Replace</Button>}
      actions={<Button busy={busy} onClick={() => send('merge')}>Merge</Button>}
    >
      <p class="text-sm text-muted">Merge adds its presets into empty slots and keeps the rest. Replace makes the list exactly the file.</p>
    </Modal>
  );
}
