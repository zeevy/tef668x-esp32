// The form for the Wi-Fi network the radio joins, on Network and on the
// first start.
import { useState } from 'preact/hooks';
import { say } from '../data/act.js';
import { Button, INPUT, Switch } from './kit.jsx';

// The Wi-Fi network the radio joins. The passphrase is always sent: one
// left empty is stored as an open network.
export function WiFiForm({ stored, set, done }) {
  const [sid, setSid] = useState(stored.sid ?? '');
  const [pwd, setPwd] = useState('');
  const [open, setOpen] = useState(false);
  const [show, setShow] = useState(false);
  async function join(e) {
    e.preventDefault();
    if (!sid.trim()) return say('Give the network name.', 'bad');
    if (!pwd && !open) return say('Type the passphrase, or tick that the network has none.', 'bad');
    // The passphrase is sent, never drawn, so the page keeps no copy of it.
    if (await set({ sid: sid.trim(), pwd: open ? '' : pwd }, '/api/settings', { sid: sid.trim(), pss: !open })) {
      say(`The radio is moving to ${sid.trim()}. This page may lose it while it does.`);
      done?.();
    }
  }
  return (
    <form onSubmit={join} class="grid gap-4">
        <label class="grid gap-1">
          <span class="label">Network name</span>
          <input class={INPUT} value={sid} onInput={(e) => setSid(e.currentTarget.value)} autocomplete="off" autocapitalize="none" autocorrect="off" spellcheck={false} maxLength={32} />
        </label>
        {!open && (
        <label class="grid gap-1">
          <span class="flex items-baseline justify-between gap-2">
            <span class="label">Passphrase</span>
            <button type="button" onClick={() => setShow(!show)} class="hit relative text-sm text-accent-ink">
              {show ? 'Hide' : 'Show'}
            </button>
          </span>
          <input aria-label="Passphrase" class={INPUT} type={show ? 'text' : 'password'} value={pwd} placeholder={stored.pss ? 'Stored. Type to change it' : ''} onInput={(e) => setPwd(e.currentTarget.value)} autocomplete="new-password" autocapitalize="none" autocorrect="off" spellcheck={false} maxLength={64} />
        </label>
        )}
        <Switch label="This network has no passphrase" text="No passphrase" on={open} onChange={setOpen} />
        <Button type="submit" icon="wifi">
          Save and join
        </Button>
        <p class="text-sm text-muted">The radio moves to it at once. If it cannot join, its own hotspot comes up while Hotspot is Auto.</p>
    </form>
  );
}
