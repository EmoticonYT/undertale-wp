#include "touch_overlay.h"
#include "runner_keyboard.h"
#include <math.h>
#include <string.h>

TouchOverlayState g_touchOverlay = {0};

void TouchOverlay_Init(int screenW, int screenH) {
    memset(&g_touchOverlay, 0, sizeof(g_touchOverlay));
    g_touchOverlay.screenWidth = screenW > 0 ? screenW : 800;
    g_touchOverlay.screenHeight = screenH > 0 ? screenH : 480;
    g_touchOverlay.visible = true;
    g_touchOverlay.opacity = 0.6f;
}

void TouchOverlay_UpdateScreenSize(int screenW, int screenH) {
    if (screenW > 0) g_touchOverlay.screenWidth = screenW;
    if (screenH > 0) g_touchOverlay.screenHeight = screenH;
}

static TouchButtonMask HitTestButtons(float x, float y, int screenW, int screenH) {
    TouchButtonMask mask = TOUCH_BTN_NONE;

    // D-Pad zone on the left half of the screen
    float dpadCenterX = screenW * 0.16f;
    float dpadCenterY = screenH * 0.72f;
    float dpadRadius = screenW * 0.13f;
    if (dpadRadius < 70.0f) dpadRadius = 70.0f;

    float dx = x - dpadCenterX;
    float dy = y - dpadCenterY;
    float distSq = dx * dx + dy * dy;

    if (distSq < (dpadRadius * 1.5f) * (dpadRadius * 1.5f) && x < (screenW * 0.45f)) {
        float deadZone = dpadRadius * 0.20f;
        if (fabsf(dx) > deadZone || fabsf(dy) > deadZone) {
            float angle = atan2f(dy, dx) * (180.0f / 3.14159265f); // -180 to 180
            // 8-way directional mapping
            if (angle >= -67.5f && angle <= 67.5f) {
                mask |= TOUCH_BTN_RIGHT;
            }
            if (angle >= 22.5f && angle <= 157.5f) {
                mask |= TOUCH_BTN_DOWN;
            }
            if (angle >= 112.5f || angle <= -112.5f) {
                mask |= TOUCH_BTN_LEFT;
            }
            if (angle >= -157.5f && angle <= -22.5f) {
                mask |= TOUCH_BTN_UP;
            }
        }
    }

    // Action buttons on the right side of the screen
    float btnRadius = screenW * 0.07f;
    if (btnRadius < 42.0f) btnRadius = 42.0f;

    // Z (Confirm) - bottom right
    float zX = screenW * 0.88f;
    float zY = screenH * 0.75f;
    float zDistSq = (x - zX) * (x - zX) + (y - zY) * (y - zY);
    if (zDistSq <= (btnRadius * 1.3f) * (btnRadius * 1.3f)) {
        mask |= TOUCH_BTN_Z;
    }

    // X (Cancel / Run) - above & left of Z
    float xX = screenW * 0.76f;
    float xY = screenH * 0.62f;
    float xDistSq = (x - xX) * (x - xX) + (y - xY) * (y - xY);
    if (xDistSq <= (btnRadius * 1.3f) * (btnRadius * 1.3f)) {
        mask |= TOUCH_BTN_X;
    }

    // C (Menu / Inventory) - above Z
    float cX = screenW * 0.88f;
    float cY = screenH * 0.48f;
    float cDistSq = (x - cX) * (x - cX) + (y - cY) * (y - cY);
    if (cDistSq <= (btnRadius * 1.3f) * (btnRadius * 1.3f)) {
        mask |= TOUCH_BTN_C;
    }

    return mask;
}

static void RecomputeAndDispatchKeys(RunnerKeyboard* keyboard) {
    TouchButtonMask newMask = TOUCH_BTN_NONE;
    for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
        if (g_touchOverlay.pointers[i].active) {
            newMask |= g_touchOverlay.pointers[i].assignedBtn;
        }
    }

    TouchButtonMask changed = newMask ^ g_touchOverlay.currentMask;
    if (!changed || !keyboard) {
        g_touchOverlay.currentMask = newMask;
        return;
    }

    // Helper macro to update key down/up
    #define SYNC_KEY(maskFlag, gmlVk) do {         if (changed & (maskFlag)) {             if (newMask & (maskFlag)) {                 RunnerKeyboard_onKeyDown(keyboard, gmlVk);             } else {                 RunnerKeyboard_onKeyUp(keyboard, gmlVk);             }         }     } while(0)

    SYNC_KEY(TOUCH_BTN_UP, VK_UP);
    SYNC_KEY(TOUCH_BTN_DOWN, VK_DOWN);
    SYNC_KEY(TOUCH_BTN_LEFT, VK_LEFT);
    SYNC_KEY(TOUCH_BTN_RIGHT, VK_RIGHT);
    SYNC_KEY(TOUCH_BTN_Z, Z);
    SYNC_KEY(TOUCH_BTN_X, X);
    SYNC_KEY(TOUCH_BTN_C, C);

    #undef SYNC_KEY

    g_touchOverlay.prevMask = g_touchOverlay.currentMask;
    g_touchOverlay.currentMask = newMask;
}

void TouchOverlay_OnPointerDown(uint32_t pointerId, float x, float y, RunnerKeyboard* keyboard) {
    int freeSlot = -1;
    for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
        if (g_touchOverlay.pointers[i].active && g_touchOverlay.pointers[i].pointerId == pointerId) {
            freeSlot = i;
            break;
        }
        if (!g_touchOverlay.pointers[i].active && freeSlot == -1) {
            freeSlot = i;
        }
    }

    if (freeSlot != -1) {
        g_touchOverlay.pointers[freeSlot].pointerId = pointerId;
        g_touchOverlay.pointers[freeSlot].x = x;
        g_touchOverlay.pointers[freeSlot].y = y;
        g_touchOverlay.pointers[freeSlot].active = true;
        g_touchOverlay.pointers[freeSlot].assignedBtn = HitTestButtons(x, y, g_touchOverlay.screenWidth, g_touchOverlay.screenHeight);
    }

    RecomputeAndDispatchKeys(keyboard);
}

void TouchOverlay_OnPointerMove(uint32_t pointerId, float x, float y, RunnerKeyboard* keyboard) {
    for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
        if (g_touchOverlay.pointers[i].active && g_touchOverlay.pointers[i].pointerId == pointerId) {
            g_touchOverlay.pointers[i].x = x;
            g_touchOverlay.pointers[i].y = y;
            g_touchOverlay.pointers[i].assignedBtn = HitTestButtons(x, y, g_touchOverlay.screenWidth, g_touchOverlay.screenHeight);
            break;
        }
    }

    RecomputeAndDispatchKeys(keyboard);
}

void TouchOverlay_OnPointerUp(uint32_t pointerId, float x, float y, RunnerKeyboard* keyboard) {
    (void)x; (void)y;
    for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
        if (g_touchOverlay.pointers[i].active && g_touchOverlay.pointers[i].pointerId == pointerId) {
            g_touchOverlay.pointers[i].active = false;
            g_touchOverlay.pointers[i].assignedBtn = TOUCH_BTN_NONE;
            break;
        }
    }

    RecomputeAndDispatchKeys(keyboard);
}

void TouchOverlay_OnPointerCancel(uint32_t pointerId, RunnerKeyboard* keyboard) {
    for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
        if (g_touchOverlay.pointers[i].active && (g_touchOverlay.pointers[i].pointerId == pointerId || pointerId == 0)) {
            g_touchOverlay.pointers[i].active = false;
            g_touchOverlay.pointers[i].assignedBtn = TOUCH_BTN_NONE;
        }
    }

    RecomputeAndDispatchKeys(keyboard);
}
