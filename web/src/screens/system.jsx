// System: the firmware and its updates, installing one from a file,
// restarting and sleep, and the radio's health. As Network: a status card,
// a card of rows, then the health figures.
import { useEffect, useState } from 'preact/hooks';
import { state, upload } from '../data/radio.js';
import { useStoredSettings } from '../data/reads.js';
import { duration } from '../data/format.js';
import { LIVE_PAGES_MAX, LOG_MAX, PC_LINK_MAX, PRESET_SLOTS } from '../data/limits.js';
import { act, refused, say } from '../data/act.js';
import { Ask, Bar, Button, Card, Chip, FactRows, FileButton, SettingRow, StatusHead, Switch } from '../ui/kit.jsx';
import { Board } from '../ui/backs.jsx';

// What the update check says, and how its chip looks.
const CHECK = {
  off: ['Checks are off', 'off'],
  wait: ['Checks when the Wi-Fi is up', 'off'],
  checking: ['Checking now', 'off'],
  none: ['Up to date', 'good'],
  found: ['A newer version is out', 'accent'],
  failed: ['The last check failed', 'warn'],
};

// What the update check says, with the version when it found one.
export const checkSays = (upd) => (upd?.st === 'found' ? `${upd.ver} is out` : (CHECK[upd?.st]?.[0] ?? upd?.st ?? '--'));

export function System() {
  const s = state.value;
  // While the radio restarts: the up time before it, to see it come back.
  const [away, setAway] = useState(null);
  const [asking, setAsking] = useState(null);
  // The stored Check for Updates setting: the check's own state says what
  // the last check found, and does not change when the setting does. Until
  // the settings are read, which needs the PIN, the check's state stands in.
  const { stored, set } = useStoredSettings();
  useEffect(() => {
    if (away && s.up < away.up) {
      setAway(null);
      say(`The radio is back, version ${s.ver}.`);
    }
  }, [s.up]);
  const goneFor = (why) => setAway({ up: s.up, why });
  const found = s.upd?.st === 'found';
  const asks = {
    install: ['Install the newer version?', `Version ${s.upd?.ver} is downloaded from GitHub and written, then the radio restarts into it. If it does not join the Wi-Fi after, it goes back to this version.`, 'Install', () => act('/update/install', {}).then((ok) => ok && goneFor('Downloading and installing. The radio restarts when it is done.'))],
    reboot: ['Restart the radio?', 'It is silent for about 10 s and this page waits for it.', 'Restart', () => act('/reboot', {}).then((ok) => ok && goneFor('Restarting.'))],
    sleep: ['Put the radio to sleep?', 'Press the tuning knob on the radio to wake it. This page cannot.', 'Sleep', () => act('/api/sleep', {})],
  };
  // The firmware with Restart and Sleep across the page; the radio's health
  // under it as two cards side by side, one column on a phone; the updates
  // last.
  return (
    <div class="grid grid-cols-1 gap-4">
      {/* The firmware on the left; the update check, then Restart and Sleep, on the right. */}
      <Card title="Firmware" back={<Board />} class="@container">
        <div class="grid grid-cols-1 gap-4 @3xl:grid-cols-2">
          <div>
            <StatusHead icon="cpu" title={`Version ${s.ver}`} sub={`Build ${s.bid} · ${s.brd?.toUpperCase()} · slot ${s.slt}`} />
            <div class="mt-4 flex flex-wrap gap-2">
              {!s.cnf && <Chip tone="warn">On trial until it joins the Wi-Fi</Chip>}
              <Chip tone={CHECK[s.upd?.st]?.[1] ?? 'off'}>{checkSays(s.upd)}</Chip>
            </div>
            {away && <p class="mt-4 text-sm font-semibold text-accent-ink">{away.why} This page waits for it.</p>}
          </div>
          <div class="grid content-center gap-2">
            <Switch label="Check for updates" note="At each start, once the Wi-Fi is up" on={stored ? !!stored.upc : s.upd?.st !== 'off'} onChange={(on) => set({ upc: +on })} />
            <div class="grid grid-cols-2 gap-2">
              <Button kind="secondary" icon="power" onClick={() => setAsking('reboot')}>
                Restart
              </Button>
              <Button kind="secondary" icon="moon" onClick={() => setAsking('sleep')}>
                Sleep
              </Button>
            </div>
          </div>
        </div>
      </Card>
      <Health s={s} />
      <Card title="Updates" warn>
        {found && (
          <SettingRow icon="download" title={`Version ${s.upd.ver} is out`} note={`${s.upd.sz ? `${(s.upd.sz / 1048576).toFixed(1)} MB, ` : ''}from GitHub`}>
            <Button icon="download" onClick={() => setAsking('install')}>
              Install {s.upd.ver}
            </Button>
          </SettingRow>
        )}
        <SettingRow icon="upload" title="Update firmware" note="From a firmware .bin built for this radio">
          <FromFile onSent={() => goneFor('Written. The radio restarts into it.')} />
        </SettingRow>
      </Card>
      {asking && <Ask title={asks[asking][0]} text={asks[asking][1]} no="Not now" yes={asks[asking][2]} onYes={asks[asking][3]} onClose={() => setAsking(null)} />}
    </div>
  );
}

// A firmware file chosen and sent, with how much of it has gone.
function FromFile({ onSent }) {
  const [file, setFile] = useState(null);
  const [sent, setSent] = useState(null);
  // A refusal for the PIN opens the PIN prompt; any other shows the radio's
  // words.
  async function send() {
    const form = new FormData();
    form.append('firmware', file);
    setSent(0);
    try {
      await upload('/update', form, setSent);
      setFile(null);
      onSent();
    } catch (e) {
      refused(e);
    }
    setSent(null);
  }
  if (sent !== null) {
    return (
      <div class="grid gap-1">
        <Bar pct={sent * 100} label="Sent" />
        <span class="num text-sm text-muted">{Math.round(sent * 100)} % sent</span>
      </div>
    );
  }
  return (
    <div class="grid gap-2">
      <FileButton accept=".bin" label="Firmware file" onFile={setFile}>
        <span class="truncate">{file ? file.name : 'Choose a file'}</span>
      </FileButton>
      {file && (
        <Button icon="upload" onClick={send}>
          Install this file
        </Button>
      )}
    </div>
  );
}

// The radio's own figures as rows, name left and value right, as the RDS
// card has them, in two cards of eight: how it is running, then its stores
// and links. Side by side from laptop width, the same height.
function Health({ s }) {
  const kb = (b) => (b == null ? '--' : `${Math.round(b / 1024)} KB`);
  const cards = [
    [
      'Running',
      [
        ['Up for', duration(s.up)],
        ['Started because', s.rst],
        ['Free memory', kb(s.hep)],
        ['Lowest since start', kb(s.hmn)],
        ['Largest free piece', kb(s.hlb)],
        ['Screen memory used', s.pnl?.lvp != null ? `${s.pnl.lvp} %` : '--'],
        ['Radio task stack left', s.stk ? `${s.stk.rad} bytes` : '--'],
        ['Loop stack left', s.stk ? `${s.stk.lop} bytes` : '--'],
      ],
    ],
    [
      'Stores and links',
      [
        ['Station log', s.log ? `${s.log.n} of ${LOG_MAX}` : '--'],
        ['Presets', s.pst ? `${s.pst.n} of ${PRESET_SLOTS}` : '--'],
        ['Saved by itself', s.asv ? `${s.asv.n} times${s.asv.bad ? ', the last failed' : ''}` : '--'],
        ['Clock', s.clk?.syn ? `Set, ${s.clk.now}` : 'Not set'],
        ['Tuner', s.tun?.prt ?? '--'],
        ['Battery at start', s.bat?.boot ? `${(s.bat.boot / 1000).toFixed(2)} V` : '--'],
        ['Web pages open', s.liv ? `${s.liv.n} of ${LIVE_PAGES_MAX}` : '--'],
        ['PCs on the PC Link', s.pcl ? `${s.pcl.cli ?? 0} of ${PC_LINK_MAX}` : '--'],
      ],
    ],
  ];
  return (
    <div class="grid grid-cols-1 gap-4 lg:grid-cols-2">
      {cards.map(([title, facts]) => (
        <Card title={title}>
          <FactRows rows={facts.map(([k, v]) => [k, v, true])} class="grid [&>:last-child]:border-b-0" />
        </Card>
      ))}
    </div>
  );
}

