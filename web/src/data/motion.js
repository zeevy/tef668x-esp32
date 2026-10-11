// A change of screen or tab made as one view transition: the browser fades
// from the old view to the new one, and the parts given a
// view-transition-name, the dial's frequency and the station's name, glide
// from their old place to their new one. A browser without view transitions,
// or a device that asks for less motion, gets the change at once.
const less = matchMedia('(prefers-reduced-motion: reduce)');

export function withMotion(change) {
  if (!document.startViewTransition || less.matches) return change();
  // The page draws the change on the next turn, after it is made; the
  // transition waits for that before it takes its picture of the new view.
  document.startViewTransition(() => {
    change();
    return new Promise((done) => setTimeout(done));
  });
}
