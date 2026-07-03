#include "Battlesport/player.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <string.h>

namespace {
struct SavedFxSurfaceState {
    unsigned short *pixels;
    int width;
    int height;
    int pitchBytes;
    int pitchPixels16;
    int rendererPath;
};

void SaveFxSurfaceState(
    SavedFxSurfaceState *state
) {
    state->pixels = g_zVideo_FxSurfacePixels16;
    state->width = g_zVideo_FxSurfaceWidth;
    state->height = g_zVideo_FxSurfaceHeight;
    state->pitchBytes = g_zVideo_FxSurfacePitchBytes;
    state->pitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    state->rendererPath = g_zVideo_ActiveRendererPath;
}

void RestoreFxSurfaceState(
    const SavedFxSurfaceState *state
) {
    g_zVideo_FxSurfacePixels16 = state->pixels;
    g_zVideo_FxSurfaceWidth = state->width;
    g_zVideo_FxSurfaceHeight = state->height;
    g_zVideo_FxSurfacePitchBytes = state->pitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = state->pitchPixels16;
    g_zVideo_ActiveRendererPath = state->rendererPath;
}

void InitSoftwareFxSurface(
    unsigned short *pixels
) {
    int index;
    for (index = 0; index < 24; ++index) {
        pixels[index] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 4;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    g_zVideo_ActiveRendererPath = 0;
    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );
}

bool ClippedPixelsMatch(
    const unsigned short *pixels,
    unsigned short expected
) {
    return pixels[1 + 1 * 6] == expected &&
        pixels[4 + 1 * 6] == expected &&
        pixels[1 + 2 * 6] == expected &&
        pixels[4 + 2 * 6] == expected &&
        pixels[0 + 1 * 6] == 0xffff &&
        pixels[5 + 1 * 6] == 0xffff &&
        pixels[1 + 3 * 6] == 0xffff;
}

void *ReadObjectVtable(
    void *object
) {
    return *((void **)object);
}
} // namespace

extern "C" int player_underwater_fx_pass3_ui_constructor_smoke(void) {
    Player_UnderwaterFxPass3Ui ui;
    ui.next = (HudUiElement *)(&ui);
    ui.parent = &ui;
    ui.flags = 0xffffffffu;
    ui.timer = 5.0f;
    ui.x = 77;
    ui.y = 88;
    ui.state = 0xffffu;
    ui.clipRectOrNull = (HudUiRect *)(&ui);

    Player_UnderwaterFxPass3Ui *const result = ui.Constructor();

    const bool baseOk =
        result == &ui && ui.next == 0 && ui.parent == 0 &&
        ui.flags == 0 && ui.timer == 0.0f &&
        ui.x == 0 && ui.y == 0 && ui.state == 0;
    return baseOk && ui.clipRectOrNull == 0 ? 0 : 1;
}

extern "C" int player_init_underwater_fx_pass3_ui_singleton_smoke(void) {
    unsigned char oldUnderwaterFxPass3Ui[sizeof(g_Player_UnderwaterFxPass3Ui)];
    memcpy(
        oldUnderwaterFxPass3Ui,
        &g_Player_UnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );

    memset(
        &g_Player_UnderwaterFxPass3Ui,
        0xff,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );
    Player::InitUnderwaterFxPass3UiSingleton();

    Player_UnderwaterFxPass3Ui probe;
    probe.Constructor();
    const bool ok =
        ReadObjectVtable(&g_Player_UnderwaterFxPass3Ui) == ReadObjectVtable(&probe) &&
        g_Player_UnderwaterFxPass3Ui.next == 0 &&
        g_Player_UnderwaterFxPass3Ui.parent == 0 &&
        g_Player_UnderwaterFxPass3Ui.flags == 0 &&
        g_Player_UnderwaterFxPass3Ui.timer == 0.0f &&
        g_Player_UnderwaterFxPass3Ui.x == 0 &&
        g_Player_UnderwaterFxPass3Ui.y == 0 &&
        g_Player_UnderwaterFxPass3Ui.state == 0 &&
        g_Player_UnderwaterFxPass3Ui.clipRectOrNull == 0;

    memcpy(
        &g_Player_UnderwaterFxPass3Ui,
        oldUnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );
    return ok ? 0 : 1;
}

extern "C" int player_register_underwater_fx_pass3_ui_on_exit_smoke(void) {
    unsigned char oldUnderwaterFxPass3Ui[sizeof(g_Player_UnderwaterFxPass3Ui)];
    memcpy(
        oldUnderwaterFxPass3Ui,
        &g_Player_UnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );

    Player::InitUnderwaterFxPass3UiSingleton();
    Player_UnderwaterFxPass3Ui probe;
    probe.Constructor();
    void *const derivedTable = ReadObjectVtable(&probe);

    Player::RegisterUnderwaterFxPass3UiOnExit();

    const bool ok = ReadObjectVtable(&g_Player_UnderwaterFxPass3Ui) == derivedTable;

    memcpy(
        &g_Player_UnderwaterFxPass3Ui,
        oldUnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );
    return ok ? 0 : 1;
}

extern "C" int player_init_and_register_underwater_fx_pass3_ui_singleton_smoke(void) {
    unsigned char oldUnderwaterFxPass3Ui[sizeof(g_Player_UnderwaterFxPass3Ui)];
    memcpy(
        oldUnderwaterFxPass3Ui,
        &g_Player_UnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );

    memset(
        &g_Player_UnderwaterFxPass3Ui,
        0xff,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );
    Player::InitAndRegisterUnderwaterFxPass3UiSingleton();

    Player_UnderwaterFxPass3Ui probe;
    probe.Constructor();
    const bool ok =
        ReadObjectVtable(&g_Player_UnderwaterFxPass3Ui) == ReadObjectVtable(&probe) &&
        g_Player_UnderwaterFxPass3Ui.next == 0 &&
        g_Player_UnderwaterFxPass3Ui.parent == 0 &&
        g_Player_UnderwaterFxPass3Ui.flags == 0 &&
        g_Player_UnderwaterFxPass3Ui.timer == 0.0f &&
        g_Player_UnderwaterFxPass3Ui.x == 0 &&
        g_Player_UnderwaterFxPass3Ui.y == 0 &&
        g_Player_UnderwaterFxPass3Ui.state == 0 &&
        g_Player_UnderwaterFxPass3Ui.clipRectOrNull == 0;

    memcpy(
        &g_Player_UnderwaterFxPass3Ui,
        oldUnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );
    return ok ? 0 : 1;
}

extern "C" int player_reset_underwater_fx_pass3_ui_singleton_smoke(void) {
    unsigned char oldUnderwaterFxPass3Ui[sizeof(g_Player_UnderwaterFxPass3Ui)];
    memcpy(
        oldUnderwaterFxPass3Ui,
        &g_Player_UnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );

    Player::InitUnderwaterFxPass3UiSingleton();
    void *const derivedTable = ReadObjectVtable(&g_Player_UnderwaterFxPass3Ui);
    HudUiRect sentinel = {1, 2, 3, 4};
    g_Player_UnderwaterFxPass3Ui.clipRectOrNull = &sentinel;

    Player::ResetUnderwaterFxPass3UiSingleton();

    HudUiElement baseProbe;
    void *const baseTable = ReadObjectVtable(&baseProbe);
    const bool ok =
        derivedTable != baseTable &&
        ReadObjectVtable(&g_Player_UnderwaterFxPass3Ui) == baseTable &&
        g_Player_UnderwaterFxPass3Ui.clipRectOrNull == &sentinel;

    memcpy(
        &g_Player_UnderwaterFxPass3Ui,
        oldUnderwaterFxPass3Ui,
        sizeof(g_Player_UnderwaterFxPass3Ui)
    );
    return ok ? 0 : 1;
}

extern "C" int player_projectile_camera_fx_pass3_ui_constructor_smoke(void) {
    Player_ProjectileCameraFxPass3Ui ui;
    ui.next = (HudUiElement *)(&ui);
    ui.parent = &ui;
    ui.flags = 0xffffffffu;
    ui.timer = 5.0f;
    ui.x = 77;
    ui.y = 88;
    ui.state = 0xffffu;
    ui.clipRectOrNull = (HudUiRect *)(&ui);

    Player_ProjectileCameraFxPass3Ui *const result = ui.Constructor();

    const bool baseOk =
        result == &ui && ui.next == 0 && ui.parent == 0 &&
        ui.flags == 0 && ui.timer == 0.0f &&
        ui.x == 0 && ui.y == 0 && ui.state == 0;
    return baseOk && ui.clipRectOrNull == 0 ? 0 : 1;
}

extern "C" int player_underwater_fx_pass3_ui_apply_blue_tint_smoke(void) {
    SavedFxSurfaceState oldState;
    SaveFxSurfaceState(&oldState);

    unsigned short pixels[24];
    InitSoftwareFxSurface(pixels);

    HudUiRect rect = {1, 1, 5, 3};
    Player_UnderwaterFxPass3Ui ui;
    ui.Constructor();
    ui.clipRectOrNull = &rect;
    zVideoFxPass3Element *const element = &ui;
    element->ApplyPass3();

    const bool ok = ClippedPixelsMatch(
        pixels,
        0x7bff
    );

    RestoreFxSurfaceState(&oldState);
    return ok ? 0 : 1;
}

extern "C" int player_projectile_camera_fx_pass3_ui_apply_green_mask_smoke(void) {
    SavedFxSurfaceState oldState;
    SaveFxSurfaceState(&oldState);

    unsigned short pixels[24];
    InitSoftwareFxSurface(pixels);

    HudUiRect rect = {1, 1, 5, 3};
    Player_ProjectileCameraFxPass3Ui ui;
    ui.Constructor();
    ui.clipRectOrNull = &rect;
    zVideoFxPass3Element *const element = &ui;
    element->ApplyPass3();

    const bool ok = ClippedPixelsMatch(
        pixels,
        0x07e0
    );

    RestoreFxSurfaceState(&oldState);
    return ok ? 0 : 1;
}
