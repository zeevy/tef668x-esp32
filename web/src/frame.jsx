// The frame round every screen: the header with the radio's status and the
// theme switch, the way between screens, the PIN prompt, the notes, and the
// footer.
import { computed } from '@preact/signals';
import { useState } from 'preact/hooks';
import { link, pinAsked, signedIn, signIn, state } from './data/radio.js';
import { notes } from './data/act.js';
import { presetTag } from './data/format.js';
import { Banner, Button, Card, Dialog, Icon, IconButton, Modal, Toast } from './ui/kit.jsx';
import { dark, flipTheme } from './ui/theme.js';
import { VolumeBox } from './ui/volume.jsx';
import { Battery } from './ui/battery.jsx';

// npm run dev answers from data recorded off the radio; npm run dev:radio
// and the built page talk to the radio itself.
const RECORDED = import.meta.env.DEV && import.meta.env.MODE !== 'radio';

// The pages keep one width on a desktop, centred, the menu above them as wide.
const PAGE = 'mx-auto w-full max-w-5xl pr-[max(1rem,env(safe-area-inset-right))] pl-[max(1rem,env(safe-area-inset-left))]';

// True until the radio's first state comes. Read here and not from the
// state itself, so a new state, twice a second, does not draw the frame again.
const waiting = computed(() => !state.value);

// Tabs along the foot on a phone, More opening the rest; a menu across the
// top from tablet width up.
export function Frame({ screens, here, children }) {
  const [more, setMore] = useState(false);
  const close = () => setMore(false);
  const tabs = screens.filter((s) => s.tab);
  const rest = screens.filter((s) => !s.tab);
  const lost = link.value === 'lost';
  return (
    <div class="min-h-dvh pb-[calc(4rem+env(safe-area-inset-bottom))] md:pb-0">
      <div class="sticky top-0 z-10 border-b border-line bg-bg/95 backdrop-blur">
        <Header title={here.name} />
        {/* Every screen in one row under the header, the one open
            underlined. The icons only from laptop width, so all nine names
            fit on a tablet. */}
        <nav class="hidden md:block">
          <div class={`${PAGE} flex gap-1 overflow-x-auto`}>
            <Links
              screens={screens}
              here={here}
              cls="-mb-px flex h-11 shrink-0 items-center gap-2 border-b-2 px-3 text-sm"
              on="border-accent font-semibold text-accent-ink"
              off="border-transparent text-muted hover:text-ink"
              icon={{ size: 18, class: 'hidden lg:block' }}
            />
          </div>
        </nav>
      </div>
      <main class={`${PAGE} grid grid-cols-1 gap-4 py-4`}>
        {RECORDED && <Banner tone="off" icon="info">Recorded data. A change is not sent to any radio.</Banner>}
        {lost && <Banner tone="bad">The radio is not answering. Trying again.</Banner>}
        {/* Every screen waits here for the radio's first state, so none
            has to. */}
        <div class={`grid grid-cols-1 gap-4 ${lost ? 'opacity-60' : ''}`}>{waiting.value ? <Card>Waiting for the radio…</Card> : children}</div>
      </main>
      <Footer />
      <nav class="fixed inset-x-0 bottom-0 z-10 grid grid-cols-5 border-t border-line bg-surface pb-[env(safe-area-inset-bottom)] md:hidden">
        {tabs.map((s) => (
          <Tab icon={s.icon} name={s.name} href={`#${s.id}`} on={s === here} />
        ))}
        <Tab icon="menu" name="More" on={rest.includes(here)} onClick={() => setMore(true)} />
      </nav>
      {/* One live region, so a reader hears each note as it comes. */}
      <div aria-live="polite" class="fixed inset-x-4 bottom-[calc(5rem+env(safe-area-inset-bottom))] z-20 grid gap-2 md:bottom-4 md:left-auto md:w-96">
        {notes.value.map((n) => (
          <Toast tone={n.tone}>{n.text}</Toast>
        ))}
      </div>
      {more && (
        <Dialog onClose={close} class="mb-0 mt-auto w-full max-w-none rounded-t-card p-4 pb-[max(1rem,env(safe-area-inset-bottom))] md:hidden">
          <nav class="grid gap-1">
            <Links screens={rest} here={here} cls="flex h-11 items-center gap-2 rounded-ctl px-3" on="bg-accent/15 font-semibold text-accent-ink" off="hover:bg-raised" onPick={close} />
          </nav>
        </Dialog>
      )}
    </div>
  );
}

function Tab({ icon, name, href, on, onClick }) {
  const Tag = href ? 'a' : 'button';
  return (
    <Tag
      href={href}
      aria-current={href && on ? 'page' : undefined}
      onClick={onClick}
      class={`flex h-16 flex-col items-center justify-start gap-1 pt-2 text-center text-xs leading-tight ${on ? 'font-semibold text-accent-ink' : 'text-muted'}`}
    >
      <Icon name={icon} size={22} />
      {name}
    </Tag>
  );
}

// Each screen as a link with its icon, the one open marked: `cls` on every
// link, then `on` for the open one and `off` for the rest. `icon` sizes the
// icon.
function Links({ screens, here, cls, on, off, icon, onPick }) {
  return screens.map((s) => (
    <a href={`#${s.id}`} aria-current={s === here ? 'page' : undefined} onClick={onPick} class={`${cls} ${s === here ? on : off}`}>
      <Icon name={s.icon} {...icon} />
      {s.name}
    </a>
  ));
}

// The header's small keys in a box as the frequency's beside them.
const BOX = 'border border-line bg-surface';

// How the link to the radio is, shown only when it is not well.
const LINK = {
  slow: ['bg-warn', 'Slow'],
  lost: ['bg-bad', 'No answer'],
};

// The screen's name on a phone; the radio's name from tablet width up, where
// the menu shows which screen is open. The volume on every screen: mute and
// the slider in the header from tablet width up; on a phone the speaker opens
// them in a strip under the header. The battery last, at the right end.
function Header({ title }) {
  const tun = state.value?.tun;
  const [sound, setSound] = useState(false);
  return (
    <>
    <header class={`${PAGE} flex h-14 items-center gap-2`}>
      <h1 class="mr-auto min-w-0 truncate text-base font-semibold md:sr-only">{title}</h1>
      <span class="mr-auto hidden items-center gap-2 md:flex" aria-hidden="true">
        <span class="grid size-8 place-items-center rounded-ctl bg-accent text-on-accent">
          <Icon name="radio" size={18} />
        </span>
        <span class="font-semibold">TEF668X</span>
      </span>
      {tun && <VolumeBox tun={tun} class="hidden md:flex" />}
      {tun && (
        <a href="#radio" class="hit relative hidden h-9 items-center gap-1 rounded-ctl border border-line bg-surface px-3 min-[380px]:flex">
          <span class="num">{tun.f}</span>
          {/* The preset over the unit, one over the other, so the box keeps
              its height. */}
          <span class="flex flex-col items-start leading-none">
            {tun.pst > 0 && <span class="num text-xs leading-none font-semibold text-accent-ink">{presetTag(tun.pst)}</span>}
            <span class="text-xs text-muted">{tun.unt}</span>
          </span>
        </a>
      )}
      <span class="flex items-center gap-1">
        {tun && (
          <span class="md:hidden">
            <IconButton small kind="quiet" class={BOX} icon={tun.mut ? 'volume-x' : 'volume-2'} label="Volume" aria-expanded={sound} onClick={() => setSound(!sound)} />
          </span>
        )}
        {link.value !== 'ok' && (
          <span class="flex items-center gap-1 px-1 text-xs text-muted" title={LINK[link.value][1]}>
            <span class={`size-2.5 rounded-full ${LINK[link.value][0]}`} />
            <span class="sr-only md:not-sr-only">{LINK[link.value][1]}</span>
          </span>
        )}
        <IconButton small kind="quiet" class={BOX} icon={dark() ? 'sun' : 'moon'} label={dark() ? 'Light theme' : 'Dark theme'} onClick={flipTheme} />
        <IconButton
          small
          kind="quiet"
          class={BOX}
          icon={signedIn.value ? 'lock-open' : 'lock'}
          label={signedIn.value ? 'Signed in' : 'Sign in'}
          onClick={() => (pinAsked.value = true)}
        />
        <Battery bat={state.value?.bat} />
      </span>
      {pinAsked.value && <PinDialog />}
    </header>
    {tun && sound && (
      <div class={`${PAGE} pb-2 md:hidden`}>
        <VolumeBox tun={tun} class="flex" />
      </div>
    )}
    </>
  );
}

function PinDialog() {
  const [pin, setPin] = useState('');
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState('');
  // However it closes, the digits typed go with it, as it is drawn only
  // while open.
  const close = () => (pinAsked.value = false);
  async function submit(e) {
    e.preventDefault();
    setBusy(true);
    setError('');
    try {
      await signIn(pin);
      close();
    } catch (x) {
      setError(x.message);
    }
    setBusy(false);
  }
  // Signed in, it has no button, so the Close key by its title closes it.
  return (
    <Modal
      title={signedIn.value ? 'Signed in' : 'Sign in'}
      onClose={close}
      onSubmit={submit}
      cancel="Not now"
      actions={!signedIn.value && <Button type="submit" busy={busy} disabled={pin.length !== 6}>Sign in</Button>}
    >
      <p class="text-sm text-muted">{signedIn.value ? 'This browser can change the radio.' : "Changes need the radio's six digit PIN."}</p>
      {!signedIn.value && (
        <input
          type="password"
          aria-label="PIN"
          inputmode="numeric"
          autocomplete="current-password"
          maxLength={6}
          value={pin}
          onInput={(e) => setPin(e.currentTarget.value.replace(/\D/g, ''))}
          class="num h-14 w-full min-w-0 rounded-ctl border border-line bg-bg text-center text-2xl tracking-[0.5em] outline-none focus:border-accent"
        />
      )}
      {error && <p class="text-sm text-bad">{error}</p>}
    </Modal>
  );
}

function Footer() {
  const s = state.value;
  return (
    <footer class={`${PAGE} flex flex-wrap items-center justify-between gap-4 py-4 text-sm text-muted`}>
      <span>{s ? `TEF668X ${s.ver}` : 'TEF668X'}</span>
      {/* The firmware's source; it opens only with an internet connection. */}
      <a href="https://github.com/zeevy/tef668x-esp32" target="_blank" rel="noopener noreferrer" class="text-accent-ink underline">
        Source on GitHub
      </a>
    </footer>
  );
}
