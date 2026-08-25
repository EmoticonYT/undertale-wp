#ifndef _BS_TOUCH_OVERLAY_H_
#define _BS_TOUCH_OVERLAY_H_

#include <stdbool.h>
#include <stdint.h>
#include "runner.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOUCH_BTN_NONE  = 0,
    TOUCH_BTN_UP    = 1 << 0,
    TOUCH_BTN_DOWN  = 1 << 1,
    TOUCH_BTN_LEFT  = 1 << 2,
    TOUCH_BTN_RIGHT = 1 << 3,
    TOUCH_BTN_Z     = 1 << 4, // Confirm / Action
    TOUCH_BTN_X     = 1 << 5, // Cancel / Run
    TOUCH_BTN_C     = 1 << 6, // Menu / Inventory
} TouchButtonMask;

typedef struct {
    uint32_t pointerId;
    float x;
    float y;
    bool active;
    TouchButtonMask assignedBtn;
} TouchPointer;

#define MAX_TOUCH_POINTERS 10

typedef struct {
    TouchPointer pointers[MAX_TOUCH_POINTERS];
    TouchButtonMask currentMask;
    TouchButtonMask prevMask;
    int screenWidth;
    int screenHeight;
    bool visible;
    float opacity;
} TouchOverlayState;

extern TouchOverlayState g_touchOverlay;

void TouchOverlay_Init(int screenW, int screenH);
void TouchOverlay_UpdateScreenSize(int screenW, int screenH);
void TouchOverlay_OnPointerDown(uint32_t pointerId, float x, float y, RunnerKeyboard* keyboard);
void TouchOverlay_OnPointerMove(uint32_t pointerId, float x, float y, RunnerKeyboard* keyboard);
void TouchOverlay_OnPointerUp(uint32_t pointerId, float x, float y, RunnerKeyboard* keyboard);
void TouchOverlay_OnPointerCancel(uint32_t pointerId, RunnerKeyboard* keyboard);

#ifdef __cplusplus
}
#endif

#endif /* _BS_TOUCH_OVERLAY_H_ */
