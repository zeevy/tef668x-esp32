/*
 * The DX pages on the panel, for the screen task. See screen_task_dx.cpp.
 */
#ifndef SCREEN_TASK_DX_H
#define SCREEN_TASK_DX_H

#include <stdbool.h>
#include <stdint.h>

/* Every page's view as on opening DX mode: no history and the Scope
 * cursor on the dial's channel. */
void screenTaskDxReset(void);

/*
 * Build and draw DX page `page`, taking the DX session's step first, with
 * `cursor` the Catches page's row, which follows its catch when the list
 * moves. False when the radio has left FM, which closes DX mode.
 */
bool screenTaskDxDraw(uint8_t page, uint8_t *cursor);

/* The Scope page's cursor: back to the dial's channel, a turn, and the
 * channel it is on, false with no sweep. */
void screenTaskDxScopeReset(void);
void screenTaskDxScopeTurn(int32_t clicks);
bool screenTaskDxScopeCursorKHz(uint32_t *khz);

#endif /* SCREEN_TASK_DX_H */
