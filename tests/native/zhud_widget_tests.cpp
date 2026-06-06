#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;
extern "C" int g_RecoilApp_QuitAfterCredits;

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method> void *MethodAddress(Method method) {
    union MethodToFunction {
        Method method;
        void *function;
    };

    MethodToFunction thunk{};
    thunk.method = method;
    return thunk.function;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
        FlushInstructionCache(
            GetCurrentProcess(),
            patch.address,
            sizeof(patch.original)
        );
    }

    patch.address = nullptr;
}

int g_optionsDialogLoadCalls;
bool g_optionsDialogLoadArgsOk;
int g_creditsPanelLoadCalls;
bool g_creditsPanelLoadArgsOk;
int g_cmdDialogLoadCalls;
bool g_cmdDialogLoadArgsOk;
int g_cmdDialogRebuildCalls;
int g_cmdDialogRebuildGroup;
int g_cmdDialogSetChildFlagsCalls;
int g_cmdDialogSetChildFlagsValue;
int g_cmdResetDefaultBindingCalls;
int g_cmdResetLookupRebuildCalls;
int g_cmdResetBaseActivateCalls;

struct OptionsDialogLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * OptionsDialogLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_optionsDialogLoadCalls;
    g_optionsDialogLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "OPTIONSPANEL"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *HudUiBackgroundLoadFromZrdAddress() {
    return MethodAddress(&HudUiBackground::LoadFromZrd);
}

void *OptionsDialogLoadProbeAddress() {
    return MethodAddress(&OptionsDialogLoadProbe::LoadFromZrd);
}

struct CreditsPanelLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * CreditsPanelLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_creditsPanelLoadCalls;
    g_creditsPanelLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "CREDITSPANEL"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *CreditsPanelLoadProbeAddress() {
    return MethodAddress(&CreditsPanelLoadProbe::LoadFromZrd);
}

struct CmdDialogLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * CmdDialogLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_cmdDialogLoadCalls;
    g_cmdDialogLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(zrdPath, "dialog.zrd") == 0 &&
        sectionName != nullptr &&
        std::strcmp(sectionName, "COMMANDS_DIALOG") == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *CmdDialogLoadProbeAddress() {
    return MethodAddress(&CmdDialogLoadProbe::LoadFromZrd);
}

struct CmdDialogRebuildProbe {
    void RebuildCommandBindingListsForGroup(int groupIndex);
};

void CmdDialogRebuildProbe::RebuildCommandBindingListsForGroup(
    int groupIndex
) {
    ++g_cmdDialogRebuildCalls;
    g_cmdDialogRebuildGroup = groupIndex;
}

void *CmdDialogRebuildProbeAddress() {
    return MethodAddress(&CmdDialogRebuildProbe::RebuildCommandBindingListsForGroup);
}

struct HudUiContainerSetChildFlagsProbe {
    void SetChildFlags(int flags);
};

void HudUiContainerSetChildFlagsProbe::SetChildFlags(
    int flags
) {
    ++g_cmdDialogSetChildFlagsCalls;
    g_cmdDialogSetChildFlagsValue = flags;
}

void *HudUiContainerSetChildFlagsProbeAddress() {
    return MethodAddress(&HudUiContainerSetChildFlagsProbe::SetChildFlags);
}

int __fastcall FakeHudCmdBindGroupCount() {
    return 0;
}

void __fastcall FakeHudCmdInitDefaultBindings() {
    ++g_cmdResetDefaultBindingCalls;
}

void __fastcall FakeHudCmdRebuildLookupIndices() {
    ++g_cmdResetLookupRebuildCalls;
}

struct HudCmdZrdActivateProbe {
    void OnActivate();
};

void HudCmdZrdActivateProbe::OnActivate() {
    ++g_cmdResetBaseActivateCalls;
}

void *HudCmdZrdActivateProbeAddress() {
    return MethodAddress(&HudCmdZrdActivateProbe::OnActivate);
}

char *__fastcall FakeHudCmdCommandLabel(int commandId) {
    return commandId == 7 ? const_cast<char *>("CommandSeven")
                          : const_cast<char *>("CommandFive");
}

char *__fastcall FakeHudCmdCommandHint(int commandId) {
    return commandId == 7 ? const_cast<char *>("HintSeven")
                          : const_cast<char *>("HintFive");
}

void SetupHudCmdButton(
    HudCmdBindButtonBase &button,
    const char *firstText,
    const char *secondText,
    int firstCommandId,
    int secondCommandId
) {
    button.AddBindingEntry(
        firstText,
        firstCommandId
    );
    button.AddBindingEntry(
        secondText,
        secondCommandId
    );
}

void CleanupHudCmdButton(HudCmdBindButtonBase &button) {
    button.DestructorCore();
}

void InstallHudCmdDialogBinding(
    HudCmdBindButtonBase &button,
    const char *text
) {
    HudCmdBindingEntry *const entry =
        static_cast<HudCmdBindingEntry *>(::operator new(sizeof(HudCmdBindingEntry)));
    new (entry) HudCmdBindingEntry;
    entry->displayText = _strdup(text);
    entry->commandId = 5;

    HudCmdBindingEntry **const slots =
        static_cast<HudCmdBindingEntry **>(::operator new(sizeof(HudCmdBindingEntry *)));
    slots[0] = entry;
    button.bindingVec.begin = slots;
    button.bindingVec.end = slots + 1;
    button.bindingVec.capacity = slots + 1;
}

void SetupHudCmdDialogButtons(HudCmdDialog &dialog) {
    SetupHudCmdButton(
        dialog.commandList,
        "CommandZero",
        "CommandSeven",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.keyAButton,
        "KeyA0",
        "KeyA1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.keyBButton,
        "KeyB0",
        "KeyB1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.joyButton,
        "Joy0",
        "Joy1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.mouseButton,
        "Mouse0",
        "Mouse1",
        5,
        7
    );
}

void CleanupHudCmdDialogButtons(HudCmdDialog &dialog) {
    CleanupHudCmdButton(dialog.mouseButton);
    CleanupHudCmdButton(dialog.joyButton);
    CleanupHudCmdButton(dialog.keyBButton);
    CleanupHudCmdButton(dialog.keyAButton);
    CleanupHudCmdButton(dialog.commandList);
}

void InitHudCmdInputTables() {
    zInput::BindMap_InitDikKeyNameTable();
    zInput::BindMap_InitJoystickButtonNameTable();
    zInput::BindMap_InitMouseButtonNameTable();
}

bool HudCmdButtonDestructorCleared(const HudCmdBindButtonBase &button) {
    return button.bindingVec.begin == nullptr &&
           button.bindingVec.end == nullptr &&
           button.bindingVec.capacity == nullptr;
}

void FreeHudCmdBindingSlotPanels(HudCmdBindButtonBase &button) {
    if (button.bindingSlotPanels == nullptr) {
        return;
    }

    int *const header = reinterpret_cast<int *>(button.bindingSlotPanels) - 1;
    const int count = *header;
    for (int index = 0; index < count; ++index) {
        reinterpret_cast<HudUiPanel *>(&button.bindingSlotPanels[index])->Destructor();
    }

    ::operator delete(header);
    button.bindingSlotPanels = nullptr;
    button.bindingSlotTotalCount = 0;
    button.visibleBindingSlotCount = 0;
}
} // namespace

extern "C" int zhud_element_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    HudUiElement element{};
    element.flags = 0x01;
    g_HudUi_InvalidateMask = 0x24;
    element.Invalidate();
    const bool ok = element.flags == 0x25;

    g_HudUi_InvalidateMask = oldMask;
    return ok ? 0 : 1;
}

extern "C" int zhud_element_clip_and_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiElement element{};
    element.Constructor(0, 0);
    element.flags = 0x01;
    g_HudUi_InvalidateMask = 0x24;
    element.Invalidate();

    HudUiRect rect{1, 2, 3, 4};
    element.SetBltSourceAndClipRect(&element, &rect);

    const bool updated =
        element.flags == 0x25 &&
        element.bltSource == &element &&
        std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    element.SetClipRect(nullptr);
    const bool nullPreserved = std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = oldMask;
    return updated && nullPreserved ? 0 : 1;
}

extern "C" int zhud_element_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiElement element{};
    HudUiElement *const result = element.Constructor(17, 29);
    const bool initialized =
        result == &element &&
        element.next == nullptr &&
        element.parent == nullptr &&
        element.flags == 0 &&
        element.timer == 0.0f &&
        element.x == 17 &&
        element.y == 29 &&
        element.bltSource == nullptr &&
        element.state == 0;

    HudUiRect rect{4, 5, 6, 7};
    element.SetClipRect(&rect);
    const bool virtualsReady = std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = oldMask;
    return initialized && virtualsReady ? 0 : 1;
}

extern "C" int zhud_widget_constructor_smoke(void) {
    HudUiWidget widget{};
    widget.imageStateWord = 0xabcd1234;
    for (HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyRect.framesRemaining = 7;
    }

    HudUiWidget *const result = widget.Constructor(0x55aa);

    bool dirtyFramesCleared = true;
    for (const HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyFramesCleared = dirtyFramesCleared && dirtyRect.framesRemaining == 0;
    }

    const bool initialized =
        result == &widget &&
        widget.alignFlags == 0x55aa &&
        widget.image == nullptr &&
        widget.ownsImage == 0 &&
        widget.bltClipRectOrNull == nullptr &&
        (widget.imageStateWord & 0xffffu) == 0 &&
        widget.dirtyRectCount == 0 &&
        widget.state == 0 &&
        dirtyFramesCleared;

    zVidImagePartial image{};
    image.width = 11;
    image.height = 6;
    widget.x = 10;
    widget.y = 20;
    widget.image = &image;
    widget.alignFlags = 1;
    const bool virtualsReady = widget.GetCenterX() == 15 && widget.GetCenterY() == 23;

    return initialized && virtualsReady ? 0 : 1;
}

extern "C" int zhud_widget_release_image_if_owned_smoke(void) {
    HudUiWidget borrowed{};
    zVidImagePartial borrowedImage{};
    borrowed.image = &borrowedImage;
    borrowed.ownsImage = 0;
    borrowed.ReleaseImageIfOwned();
    const bool borrowedOk = borrowed.image == &borrowedImage && borrowed.ownsImage == 0;

    HudUiWidget owned{};
    owned.image = &zVid_Image::g_zImage_DefaultImage;
    owned.ownsImage = 1;
    owned.ReleaseImageIfOwned();
    const bool ownedOk = owned.image == nullptr && owned.ownsImage == 0;

    HudUiWidget empty{};
    empty.ownsImage = 1;
    empty.ReleaseImageIfOwned();
    const bool emptyOk = empty.image == nullptr && empty.ownsImage == 0;

    return borrowedOk && ownedOk && emptyOk ? 0 : 1;
}

extern "C" int zhud_widget_destructor_core_smoke(void) {
    HudUiWidget *borrowed = new HudUiWidget();
    zVidImagePartial borrowedImage{};
    borrowed->image = &borrowedImage;
    borrowed->ownsImage = 0;
    borrowed->DestructorCore();
    const bool borrowedOk = borrowed->image == &borrowedImage && borrowed->ownsImage == 0;
    ::operator delete(borrowed);

    HudUiWidget *owned = new HudUiWidget();
    owned->image = &zVid_Image::g_zImage_DefaultImage;
    owned->ownsImage = 1;
    owned->DestructorCore();
    const bool ownedOk = owned->image == nullptr && owned->ownsImage == 0;
    ::operator delete(owned);

    return borrowedOk && ownedOk ? 0 : 1;
}

extern "C" int zhud_widget_set_image_by_path_owned_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiWidget nullPath{};
    nullPath.image = &zVid_Image::g_zImage_DefaultImage;
    nullPath.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const nullResult = nullPath.SetImageByPathOwned(nullptr);
    const bool nullPathOk =
        nullResult == nullptr &&
        nullPath.image == &zVid_Image::g_zImage_DefaultImage &&
        nullPath.ownsImage == 1 &&
        (nullPath.flags & 0x80) == 0;

    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_TexturePackLoadState = 0;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePacks = nullptr;

    HudUiWidget missingPath{};
    missingPath.image = &zVid_Image::g_zImage_DefaultImage;
    missingPath.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const missingResult =
        missingPath.SetImageByPathOwned("__recoil_missing_widget_image__");
    const bool missingPathOk =
        missingResult == nullptr &&
        missingPath.image == nullptr &&
        missingPath.ownsImage == 0 &&
        (missingPath.flags & 0x80) != 0;

    g_HudUi_InvalidateMask = oldMask;
    std::free(g_zVid_TexturePacks);
    g_zVid_TexturePacks = nullptr;
    return nullPathOk && missingPathOk ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void) {
    HudUiBackgroundCursorWidget cursor(nullptr, 7);

    const bool constructed =
        cursor.image == nullptr &&
        cursor.alignFlags == 0 &&
        cursor.capturedImage == nullptr &&
        cursor.captureEnabled == 7 &&
        cursor.captureSourceSelector == 1 &&
        cursor.reservedC8 == 0 &&
        cursor.reservedCC == 0;

    zVidImagePartial image{};
    image.width = 4;
    image.height = 2;
    cursor.image = &image;
    cursor.alignFlags = 1;
    cursor.x = 8;
    cursor.y = 10;
    const bool virtualsReady = cursor.GetCenterX() == 10 && cursor.GetCenterY() == 11;

    cursor.DestructorCore();
    return constructed && virtualsReady ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_rebuild_captured_image_smoke(void) {
    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    unsigned short capturedPixels[4] = {};
    zVidImagePartial sourceImage{};
    sourceImage.width = 2;
    sourceImage.height = 2;
    zVidImagePartial capturedImage{};
    capturedImage.width = 2;
    capturedImage.height = 2;
    capturedImage.pixelCount = 4;
    capturedImage.pixels = capturedPixels;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.image = &sourceImage;
    cursor.capturedImage = &capturedImage;
    cursor.captureSourceSelector = 0;

    cursor.RebuildCapturedImage(1, 1);

    const bool ok =
        cursor.bltSource == &capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;

    return ok ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_set_image_borrowed_refresh_smoke(void) {
    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    zVidImagePartial sourceImage{};
    sourceImage.width = 2;
    sourceImage.height = 2;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.x = 1;
    cursor.y = 1;
    cursor.image = &sourceImage;
    cursor.captureSourceSelector = 0;

    cursor.SetImageBorrowedAndRefresh();
    unsigned short *const capturedPixels =
        static_cast<unsigned short *>(cursor.capturedImage != nullptr
                                          ? cursor.capturedImage->pixels
                                          : nullptr);
    const bool ok =
        cursor.capturedImage != nullptr &&
        cursor.capturedImage->width == 2 &&
        cursor.capturedImage->height == 2 &&
        (cursor.capturedImage->formatFlagsPacked & 0x20) != 0 &&
        cursor.bltSource == cursor.capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels != nullptr &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;

    if (cursor.capturedImage != nullptr) {
        zVid_Image::Destroy(cursor.capturedImage);
        cursor.capturedImage = nullptr;
    }
    g_zVideo_SwSurfaceState = oldSwSurfaceState;

    return ok ? 0 : 1;
}

extern "C" int
zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hcr", 0, packPath) == 0) {
        return 1;
    }

    const char *const imageName = "cursor_path.tex";
    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 1;

    zVidTexturePackRecord record{};
    std::strcpy(record.name, imageName);
    record.fileOffset = sizeof(packHeader) + sizeof(record);
    record.paletteIndex = -1;

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }

    unsigned char imageHeader[0x10] = {};
    imageHeader[0] = 1;
    *reinterpret_cast<short *>(&imageHeader[4]) = 2;
    *reinterpret_cast<short *>(&imageHeader[6]) = 2;
    const unsigned short imagePixels[4] = {0x1001, 0x1002, 0x1003, 0x1004};
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(&record, sizeof(record), 1, out);
    std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
    std::fwrite(imagePixels, sizeof(imagePixels), 1, out);
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldBuiltinPacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinPackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;
    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;

    g_zVid_TexturePackLoadState = 1;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        if (entry.fileHandle != nullptr) {
            std::fclose(entry.fileHandle);
        }
        std::free(entry.records);
        g_zVid_TexturePacks = oldTexturePacks;
        g_zVid_TexturePackCount = oldTexturePackCount;
        g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
        g_zVid_TexturePackLoadState = oldTexturePackLoadState;
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;

    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.x = 1;
    cursor.y = 1;
    cursor.captureSourceSelector = 0;

    cursor.SetImageByPathOwnedAndRefresh(imageName);
    unsigned short *const capturedPixels =
        static_cast<unsigned short *>(cursor.capturedImage != nullptr
                                          ? cursor.capturedImage->pixels
                                          : nullptr);
    const bool loadedAndRefreshed =
        cursor.image != nullptr &&
        cursor.image->width == 2 &&
        cursor.image->height == 2 &&
        cursor.ownsImage == 1 &&
        cursor.capturedImage != nullptr &&
        cursor.bltSource == cursor.capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels != nullptr &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;

    zVidImagePartial *const capturedBeforeMissing = cursor.capturedImage;
    void *const bltSourceBeforeMissing = cursor.bltSource;
    cursor.SetImageByPathOwnedAndRefresh("__recoil_missing_cursor_path_image__");
    const bool missingPathSkippedRefresh =
        cursor.image == nullptr &&
        cursor.ownsImage == 0 &&
        cursor.capturedImage == capturedBeforeMissing &&
        cursor.bltSource == bltSourceBeforeMissing;

    if (cursor.capturedImage != nullptr) {
        zVid_Image::Destroy(cursor.capturedImage);
        cursor.capturedImage = nullptr;
    }
    cursor.ReleaseImageIfOwned();
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = oldTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;
    DeleteFileA(packPath);

    return loadedAndRefreshed && missingPathSkippedRefresh ? 0 : 1;
}

extern "C" int zhud_background_video_widget_constructor_smoke(void) {
    HudUiBackgroundVideoWidget widget{};
    const bool initialized =
        widget.x == 0 &&
        widget.y == 0 &&
        widget.stream == nullptr &&
        widget.elapsedTimeSec == 0.0f &&
        widget.mediaPath[0] == '\0';

    return initialized ? 0 : 1;
}

extern "C" int zhud_background_video_widget_destructor_smoke(void) {
    HudUiBackgroundVideoWidget *widget = new HudUiBackgroundVideoWidget();
    widget->stream = nullptr;
    widget->Destructor();
    const bool destroyed = widget->stream == nullptr;
    ::operator delete(widget);

    return destroyed ? 0 : 1;
}

extern "C" int zhud_background_constructor_smoke(void) {
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    vmodeOption.next = nullptr;

    zOptionEntryPartial *const oldOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    HudUiBackground *const backgroundPtr = new HudUiBackground;
    HudUiBackground &background = *backgroundPtr;

    bool soundsInitialized = true;
    for (int index = 0; index < 10; ++index) {
        soundsInitialized =
            soundsInitialized && background.backgroundSounds[index].sample == nullptr &&
            background.backgroundSounds[index].volume == 1.0f &&
            background.backgroundSounds[index].playHandle == nullptr;
    }

    const bool constructed =
        background.enabled == 0 &&
        background.childHead == nullptr &&
        background.childTail == nullptr &&
        background.inputFocusElement == nullptr &&
        background.captureTransitionMask == 1 &&
        background.cursorWidget.image == nullptr &&
        background.cursorWidget.alignFlags == 0 &&
        background.cursorWidget.captureEnabled == 1 &&
        background.cursorWidget.captureSourceSelector == 1 &&
        background.primaryClipImage == nullptr &&
        background.capturedCompositeImage == nullptr &&
        background.backgroundImageWidgets[0].image == nullptr &&
        background.backgroundImageWidgets[19].image == nullptr &&
        background.backgroundVideoWidgets[0].stream == nullptr &&
        background.backgroundVideoWidgets[9].stream == nullptr &&
        background.fontStyles[0].validMarker == 0 &&
        background.fontStyles[0].fontWeight == 500 &&
        background.fontStyles[19].fontWeight == 500 &&
        background.backgroundTextPanels[0].flashResetValue == 0.349999994f &&
        background.backgroundTextPanels[0].flashDirectionSign == 1 &&
        soundsInitialized &&
        background.uiOriginX == 0 &&
        background.uiOriginY == 60;

    delete backgroundPtr;
    g_zGame_Options_OptionListHead = oldOptionsHead;
    return constructed ? 0 : 1;
}

extern "C" int zhud_text_label_constructor_and_extents_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    zImage_Font *const oldFont1 = g_zImage_FontTable[1];
    g_HudUi_InvalidateMask = 0x80;

    HudUiTextLabel label{};
    HudUiTextLabel *const result = label.ConstructorWithPosAndFlags("Speed", 12, 34, 3);

    const bool constructed =
        result == &label &&
        std::strcmp(label.textBuffer, "Speed") == 0 &&
        label.x == 12 &&
        label.y == 34 &&
        label.fontHandle == 3 &&
        label.centerText == 0 &&
        label.alignMode == 0 &&
        (label.flags & 0x80) != 0;

    HudUiTextLabel assigned{};
    HudUiTextLabel *const assignedResult = assigned.Constructor(&label);
    const bool assignedLabel =
        assignedResult == &assigned &&
        std::strcmp(assigned.textBuffer, "Speed") == 0 &&
        assigned.x == 12 &&
        assigned.y == 34 &&
        assigned.fontHandle == 3 &&
        assigned.centerText == 0 &&
        assigned.alignMode == 0;

    zVidImagePartial image{};
    image.height = 6;

    zImage_Font font{};
    font.image = &image;
    font.glyphRects['A' - 0x21].left = 2;
    font.glyphRects['A' - 0x21].right = 12;
    font.glyphRects['B' - 0x21].left = 4;
    font.glyphRects['B' - 0x21].right = 10;
    g_zImage_FontTable[1] = &font;

    label.fontHandle = 1;
    label.centerText = 1;
    label.centerBoundsLeft = 100;
    label.centerBoundsRight = 150;
    label.y = 20;
    label.bltSource = &label;
    label.flags = 0;
    label.SetTextFmt("AB");

    const bool centered =
        label.x == 117 &&
        label.clipRect.left == 117 &&
        label.clipRect.top == 20 &&
        label.clipRect.right == 133 &&
        label.clipRect.bottom == 26 &&
        (label.flags & 0x80) != 0;

    HudUiTextLabel copied{};
    copied.padding32 = 0x7777;
    copied.CopyConstructor(&label);
    const bool copiedLabel =
        copied.next == nullptr &&
        copied.parent == nullptr &&
        copied.x == label.x &&
        copied.y == label.y &&
        copied.state == label.state &&
        copied.padding32 == 0x7777 &&
        std::strcmp(copied.textBuffer, label.textBuffer) == 0 &&
        copied.fontHandle == label.fontHandle &&
        copied.centerText == label.centerText &&
        copied.centerBoundsLeft == label.centerBoundsLeft &&
        copied.centerBoundsRight == label.centerBoundsRight &&
        copied.alignMode == label.alignMode;

    label.flags = 0;
    std::memset(label.textBuffer, 'X', sizeof(label.textBuffer));
    label.SetTextFmt(nullptr);

    const bool cleared = label.textBuffer[0] == 0 &&
                         label.textBuffer[sizeof(label.textBuffer) - 1] == 0 &&
                         label.flags == 0;

    g_zImage_FontTable[1] = oldFont1;
    g_HudUi_InvalidateMask = oldMask;
    return constructed && assignedLabel && centered && copiedLabel && cleared ? 0 : 1;
}

extern "C" int zhud_panel_constructor_default_smoke(void) {
    HudUiPanel panel{};
    HudUiPanel *const returned = panel.ConstructorDefault("TXT", 3, 28);

    HudUiPanel thunkPanel{};
    HudUiPanel *const thunkReturned = thunkPanel.ConstructorDefaultThunk();

    const bool defaultCtor =
        returned == &panel &&
        std::strcmp(panel.textBuffer, "TXT") == 0 &&
        panel.x == 3 &&
        panel.y == 28 &&
        panel.textPick == nullptr &&
        panel.textColor0 == 0x00ffffff &&
        panel.textColor1 == 0x00ffffff &&
        panel.hFont != nullptr &&
        panel.cachedText[0] == '\0' &&
        panel.textWidthPx == 0 &&
        panel.textHeightPx == 0 &&
        panel.shadowEnabled == 0 &&
        panel.bkMode == TRANSPARENT &&
        panel.textDirty == 1 &&
        panel.GetWrapRect() == &panel.wrapRect &&
        panel.wrapRect.left == 0 &&
        panel.wrapRect.top == 0 &&
        panel.wrapRect.right == 0 &&
        panel.wrapRect.bottom == 0 &&
        panel.textRect.left == 0 &&
        panel.textRect.top == 0 &&
        panel.textRect.bottom == 0 &&
        panel.shadowOffsetY == 0;

    const bool thunkCtor =
        thunkReturned == &thunkPanel &&
        thunkPanel.textBuffer[0] == '\0' &&
        thunkPanel.x == 0 &&
        thunkPanel.y == 0 &&
        thunkPanel.textPick == nullptr &&
        thunkPanel.textColor0 == 0x00ffffff &&
        thunkPanel.textColor1 == 0x00ffffff &&
        thunkPanel.hFont != nullptr &&
        thunkPanel.shadowEnabled == 0 &&
        thunkPanel.bkMode == TRANSPARENT &&
        thunkPanel.textDirty == 1;

    return defaultCtor && thunkCtor ? 0 : 1;
}

namespace {
int g_primitiveSetPosCount = 0;
HudUiPrimitiveBindTarget *g_primitiveSetPosThis = nullptr;
int g_primitiveSetPosX = 0;
int g_primitiveSetPosY = 0;

struct TestPrimitiveBindTarget : HudUiPrimitiveBindTarget {
    void SetPos(int newX, int newY) override {
        ++g_primitiveSetPosCount;
        g_primitiveSetPosThis = this;
        g_primitiveSetPosX = newX;
        g_primitiveSetPosY = newY;
        HudUiElement::SetPos(newX, newY);
    }
};
} // namespace

extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void) {
    TestPrimitiveBindTarget target{};
    target.endX = -1;
    target.endY = -1;
    g_primitiveSetPosCount = 0;
    g_primitiveSetPosThis = nullptr;
    g_primitiveSetPosX = 0;
    g_primitiveSetPosY = 0;

    target.SetSegmentEndpoints(11, 22, 33, 44);

    return g_primitiveSetPosCount == 1 &&
                   g_primitiveSetPosThis == &target &&
                   g_primitiveSetPosX == 11 &&
                   g_primitiveSetPosY == 22 &&
                   target.x == 11 &&
                   target.y == 22 &&
                   target.endX == 33 &&
                   target.endY == 44
               ? 0
               : 1;
}

extern "C" int zhud_background_bind_primitive_node_to_element_smoke(void) {
    const int oldRMaskShifted = g_zVideo_PixelPack_RMaskShifted;
    const int oldGMaskShifted = g_zVideo_PixelPack_GMaskShifted;
    const int oldRShift = g_zVideo_PixelPack_RShift;
    const int oldGShift = g_zVideo_PixelPack_GShift;
    const int oldBShiftTo8 = g_zVideo_PixelPack_BShiftTo8;

    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    zReader::Node positionItems[3] = {};
    positionItems[0].value.i32 = 3;
    positionItems[1].type = zReader::ZRDR_NODE_INT;
    positionItems[1].value.i32 = 5;
    positionItems[2].type = zReader::ZRDR_NODE_INT;
    positionItems[2].value.i32 = 7;

    zReader::Node colorItems[4] = {};
    colorItems[0].value.i32 = 4;
    colorItems[1].type = zReader::ZRDR_NODE_INT;
    colorItems[1].value.i32 = 0x20;
    colorItems[2].type = zReader::ZRDR_NODE_INT;
    colorItems[2].value.i32 = 0x60;
    colorItems[3].type = zReader::ZRDR_NODE_INT;
    colorItems[3].value.i32 = 0x40;

    zReader::Node endRelItems[3] = {};
    endRelItems[0].value.i32 = 3;
    endRelItems[1].type = zReader::ZRDR_NODE_INT;
    endRelItems[1].value.i32 = 2;
    endRelItems[2].type = zReader::ZRDR_NODE_INT;
    endRelItems[2].value.i32 = 3;

    zReader::Node primitiveItems[7] = {};
    primitiveItems[0].value.i32 = 7;
    primitiveItems[1].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[1].value.str = const_cast<char *>("POSITION");
    primitiveItems[2].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[2].value.nodes = positionItems;
    primitiveItems[3].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[3].value.str = const_cast<char *>("COLOR");
    primitiveItems[4].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[4].value.nodes = colorItems;
    primitiveItems[5].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[5].value.str = const_cast<char *>("ENDP_REL");
    primitiveItems[6].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[6].value.nodes = endRelItems;

    zReader::Node primitivesItems[3] = {};
    primitivesItems[0].value.i32 = 3;
    primitivesItems[1].type = zReader::ZRDR_NODE_STRING;
    primitivesItems[1].value.str = const_cast<char *>("LINE");
    primitivesItems[2].type = zReader::ZRDR_NODE_ARRAY;
    primitivesItems[2].value.nodes = primitiveItems;

    zReader::Node rootItems[3] = {};
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("PRIMITIVES");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = primitivesItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiBackground background{};
    background.cfgRoot = &root;
    background.uiOriginX = 10;
    background.uiOriginY = 20;
    background.capturedCompositeImage = reinterpret_cast<zVidImagePartial *>(0x1234);

    TestPrimitiveBindTarget target{};
    target.color565 = 0xffff;
    const int result = background.BindPrimitiveNodeToElement(nullptr, &target, "LINE");

    const bool linked =
        result == 0 &&
        background.childHead == &target &&
        background.childTail == &target &&
        target.parent == &background;
    const bool primitive =
        target.x == 15 &&
        target.y == 27 &&
        target.endX == 17 &&
        target.endY == 30 &&
        target.color565 == (zVid_PackColorRGB(0x20, 0x60, 0x40) & 0xffffu) &&
        target.bltSource == reinterpret_cast<zVidImagePartial *>(0x1234) &&
        target.clipRect.left == 15 &&
        target.clipRect.top == 27 &&
        target.clipRect.right == 15 &&
        target.clipRect.bottom == 27 &&
        (target.flags & 0x02) != 0;

    g_zVideo_PixelPack_RMaskShifted = oldRMaskShifted;
    g_zVideo_PixelPack_GMaskShifted = oldGMaskShifted;
    g_zVideo_PixelPack_RShift = oldRShift;
    g_zVideo_PixelPack_GShift = oldGShift;
    g_zVideo_PixelPack_BShiftTo8 = oldBShiftTo8;
    return linked && primitive ? 0 : 1;
}

extern "C" int zhud_container_child_list_smoke(void) {
    HudUiContainer container{};
    HudUiElement first{};
    HudUiElement second{};
    HudUiElement third{};

    const int firstResult = container.AddChild(&first);
    const int secondResult = container.AddChild(&second);
    const int thirdResult = container.AddChild(&third);
    const bool linked =
        container.enabled == 0 &&
        container.childHead == &first &&
        container.childTail == &third &&
        first.next == &second &&
        second.next == &third &&
        third.next == nullptr &&
        first.parent == &container &&
        second.parent == &container &&
        third.parent == &container;

    HudUiElement *previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundHead =
        container.FindChildWithPrev(&first, &previous) == 1 && previous == nullptr;
    previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundSecond =
        container.FindChildWithPrev(&second, &previous) == 1 && previous == &first;
    const bool foundThirdNoPrevious = container.FindChildWithPrev(&third, nullptr) == 1;
    previous = reinterpret_cast<HudUiElement *>(0x9abc);
    const bool missing =
        container.FindChildWithPrev(reinterpret_cast<HudUiElement *>(0x5678), &previous) == 0 &&
        previous == reinterpret_cast<HudUiElement *>(0x9abc);

    HudUiContainer emptyContainer{};
    previous = reinterpret_cast<HudUiElement *>(0xdef0);
    const bool emptyMissing =
        emptyContainer.FindChildWithPrev(&first, &previous) == 0 &&
        previous == reinterpret_cast<HudUiElement *>(0xdef0);

    first.flags = 0;
    second.flags = 0x10;
    third.flags = 0x20;
    container.SetChildFlags(0x02);
    const bool flagsSet = first.flags == 0x02 && second.flags == 0x12 && third.flags == 0x02;

    const bool removedMiddle =
        container.RemoveChild(&second) == 1 &&
        first.next == &third &&
        second.next == nullptr &&
        second.parent == nullptr &&
        container.childTail == &third;

    const bool removedHead =
        container.RemoveChild(&first) == 1 &&
        container.childHead == &third &&
        first.next == nullptr &&
        first.parent == nullptr;

    const bool removedTail =
        container.RemoveChild(&third) == 1 &&
        container.childHead == nullptr &&
        container.childTail == nullptr &&
        third.next == nullptr &&
        third.parent == nullptr;

    return firstResult == 1 && secondResult == 1 && thirdResult == 1 && linked &&
                   foundHead && foundSecond && foundThirdNoPrevious && missing && emptyMissing &&
                   flagsSet && removedMiddle && removedHead && removedTail
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdWidget));
    std::memset(storage, 0xcc, sizeof(HudUiZrdWidget));

    HudUiZrdWidget *const widget = reinterpret_cast<HudUiZrdWidget *>(storage);
    HudUiZrdWidget *const result = widget->Constructor();

    const bool initialized =
        result == widget &&
        widget->originX == 0 &&
        widget->originY == 0 &&
        widget->modeOrEnabled == 1 &&
        widget->owner == nullptr &&
        widget->defaultImage == nullptr &&
        widget->disabledImage == nullptr &&
        widget->rolloverImage == nullptr &&
        widget->rolloverSound == nullptr &&
        widget->rolloverPlayHandle == nullptr &&
        widget->rolloverSoundScale == 1.0f &&
        widget->activateImage == nullptr &&
        widget->activateSound == nullptr &&
        widget->activatePlayHandle == nullptr &&
        widget->activateSoundScale == 1.0f &&
        widget->labelPanels.begin == nullptr &&
        widget->labelPanels.end == nullptr &&
        widget->labelPanels.capacityEnd == nullptr &&
        widget->rolloverLabelPanels.begin == nullptr &&
        widget->rolloverLabelPanels.end == nullptr &&
        widget->rolloverLabelPanels.capacityEnd == nullptr &&
        widget->activateLabelPanels.begin == nullptr &&
        widget->activateLabelPanels.end == nullptr &&
        widget->activateLabelPanels.capacityEnd == nullptr &&
        widget->disabledLabelPanels.begin == nullptr &&
        widget->disabledLabelPanels.end == nullptr &&
        widget->disabledLabelPanels.capacityEnd == nullptr &&
        (widget->flags & 0x02u) != 0 &&
        (widget->flags & ~0x12u) == 0 &&
        (widget->imageStateWord & 0xffffu) == 1;

    widget->DestructorCore();
    ::operator delete(storage);
    return initialized ? 0 : 1;
}

extern "C" int zhud_options_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_optionsDialogLoadCalls = 0;
    g_optionsDialogLoadArgsOk = false;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            OptionsDialogLoadProbeAddress(),
            loadPatch
        )) {
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudOptionsDialog));
    HudOptionsDialog *const dialog = new (storage) HudOptionsDialog;
    RestoreFunctionPatch(loadPatch);

    const bool constructed =
        dialog != nullptr &&
        g_optionsDialogLoadCalls == 1 &&
        g_optionsDialogLoadArgsOk &&
        dialog->soundVolumeWidget.normalizedValue == 0.0f &&
        dialog->musicVolumeWidget.normalizedValue == 0.0f &&
        dialog->resolutionSelector.selectedIndex == 0 &&
        dialog->backButton.owner == nullptr;

    dialog->DestructorCore();
    ::operator delete(storage);
    return constructed ? 0 : 1;
}

extern "C" int zhud_credits_panel_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_creditsPanelLoadCalls = 0;
    g_creditsPanelLoadArgsOk = false;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    g_RecoilApp_QuitAfterCredits = 0;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CreditsPanelLoadProbeAddress(),
            loadPatch
        )) {
        g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    HudUiCreditsPanel *const panel = new (storage) HudUiCreditsPanel;
    RestoreFunctionPatch(loadPatch);

    const bool constructed =
        panel != nullptr &&
        g_creditsPanelLoadCalls == 1 &&
        g_creditsPanelLoadArgsOk &&
        panel->fadeProgress == 0.0f &&
        panel->fadeStep > 0.049f &&
        panel->fadeStep < 0.051f &&
        panel->backButton.owner == nullptr &&
        panel->quitButton.owner == nullptr &&
        panel->creditsScreen.rows.begin == nullptr &&
        panel->creditsScreen.rows.end == nullptr &&
        panel->creditsScreen.rows.cap == nullptr &&
        (panel->creditsScreen.flags & ~0x10u) == 0;

    panel->Destructor();
    ::operator delete(storage);
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    return constructed ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void) {
    void *const itemStorage = ::operator new(sizeof(HudUiListSelectorItem));
    void *const buttonStorage = ::operator new(sizeof(HudCmdBindButtonBase));
    std::memset(itemStorage, 0, sizeof(HudUiListSelectorItem));
    std::memset(buttonStorage, 0, sizeof(HudCmdBindButtonBase));
    HudUiListSelectorItem *const item = new (itemStorage) HudUiListSelectorItem;
    HudCmdBindButtonBase *const button = new (buttonStorage) HudCmdBindButtonBase;

    const bool listItemConstructed =
        item->textBuffer[0] == 0 &&
        item->x == 0 &&
        item->y == 0 &&
        item->textPick == nullptr &&
        item->textColor0 == 0x00ffffff &&
        item->textColor1 == 0x00ffffff &&
        item->textDirty == 1;

    const bool buttonConstructed =
        button->bindingSlotTotalCount == 0 &&
        button->bindingSlotPanels == nullptr &&
        button->bindingVec.begin == nullptr &&
        button->bindingVec.end == nullptr &&
        button->bindingVec.capacity == nullptr &&
        button->bindingSlotSpacing == 0xf &&
        button->selectedBindingIndex == -1 &&
        button->visibleListOffsetX == 0.0f &&
        button->visibleListOffsetY == 0.0f &&
        button->overflowListOffsetX == 0.0f &&
        button->overflowListOffsetY == 0.0f &&
        button->bindPanel.textBuffer[0] == 0 &&
        button->bindPanel.x == 0 &&
        button->bindPanel.y == 0 &&
        button->bindPanel.textPick == nullptr &&
        button->bindPanel.textDirty == 1;

    ::operator delete(buttonStorage);
    ::operator delete(itemStorage);
    return listItemConstructed && buttonConstructed ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_destructor_core_smoke(void) {
    HudCmdBindButtonBase button{};
    InstallHudCmdDialogBinding(button, "Base");

    button.DestructorCore();

    return HudCmdButtonDestructorCleared(button) ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_rebuild_binding_slot_widgets_smoke(void) {
    HudCmdBindButtonBase button{};
    button.originX = 100;
    button.originY = 50;
    button.visibleListOffsetX = 5.0f;
    button.visibleListOffsetY = 6.0f;
    button.overflowListOffsetX = 7.0f;
    button.overflowListOffsetY = 8.0f;
    button.bindingSlotSpacing = 10;

    button.RebuildBindingSlotWidgets(4, 2);

    int *const header = reinterpret_cast<int *>(button.bindingSlotPanels) - 1;
    const bool rebuilt =
        *header == 4 &&
        button.bindingSlotTotalCount == 4 &&
        button.visibleBindingSlotCount == 2 &&
        button.bindingSlotPanels[0].x == 105 &&
        button.bindingSlotPanels[0].y == 36 &&
        button.bindingSlotPanels[1].x == 105 &&
        button.bindingSlotPanels[1].y == 46 &&
        button.bindPanel.x == 100 &&
        button.bindPanel.y == 50 &&
        button.bindingSlotPanels[2].x == 107 &&
        button.bindingSlotPanels[2].y == 68 &&
        button.bindingSlotPanels[3].x == 107 &&
        button.bindingSlotPanels[3].y == 78;

    FreeHudCmdBindingSlotPanels(button);
    button.bindPanel.Destructor();
    return rebuilt ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_load_from_zrd_smoke(void) {
    return 2;
}

extern "C" int zhud_cmd_dialog_on_command_selection_changed_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);

    dialog.OnCommandSelectionChanged(1);

    const bool selected =
        dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        dialog.keyBButton.selectedBindingIndex == 1 &&
        dialog.joyButton.selectedBindingIndex == 1 &&
        dialog.mouseButton.selectedBindingIndex == 1;
    const bool labels =
        std::strcmp(dialog.commandList.bindPanel.textBuffer, "CommandSeven") == 0 &&
        std::strcmp(dialog.keyAButton.bindPanel.textBuffer, "KeyA1") == 0 &&
        std::strcmp(dialog.keyBButton.bindPanel.textBuffer, "KeyB1") == 0 &&
        std::strcmp(dialog.joyButton.bindPanel.textBuffer, "Joy1") == 0 &&
        std::strcmp(dialog.mouseButton.bindPanel.textBuffer, "Mouse1") == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return selected && labels ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);
    dialog.keyAButton.owner = &dialog;

    dialog.keyAButton.OnSelectionChangedRefresh(1);

    const bool selected =
        dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        dialog.keyBButton.selectedBindingIndex == 1 &&
        dialog.joyButton.selectedBindingIndex == 1 &&
        dialog.mouseButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return selected ? 0 : 1;
}

extern "C" int zhud_cmd_reset_button_on_activate_smoke(void) {
    CodeFunctionPatch defaultsPatch{};
    CodeFunctionPatch lookupPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch activatePatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_InitDefaultBindings),
            reinterpret_cast<void *>(&FakeHudCmdInitDefaultBindings),
            defaultsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_Current_RebuildLookupIndices),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            lookupPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            HudCmdZrdActivateProbeAddress(),
            activatePatch
        )) {
        RestoreFunctionPatch(defaultsPatch);
        RestoreFunctionPatch(lookupPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(activatePatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.resetButton.owner = &dialog;
    dialog.setList.selectedIndex = 3;
    g_cmdResetDefaultBindingCalls = 0;
    g_cmdResetLookupRebuildCalls = 0;
    g_cmdResetBaseActivateCalls = 0;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;

    dialog.resetButton.OnActivate();

    int failureCode = 0;
    if (g_cmdResetDefaultBindingCalls != 1) {
        failureCode = 3;
    } else if (g_cmdResetLookupRebuildCalls != 1) {
        failureCode = 4;
    } else if (g_cmdDialogRebuildCalls != 1) {
        failureCode = 5;
    } else if (g_cmdDialogRebuildGroup != 3) {
        failureCode = 6;
    }

    RestoreFunctionPatch(defaultsPatch);
    RestoreFunctionPatch(lookupPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(activatePatch);
    return failureCode;
}

extern "C" int zhud_cmd_key_a_button_on_begin_capture_smoke(void) {
    CodeFunctionPatch resetPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::ResetAllTransitionState),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            resetPatch
        )) {
        RestoreFunctionPatch(resetPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.keyAButton.owner = &dialog;
    g_cmdResetLookupRebuildCalls = 0;

    dialog.keyAButton.OnBeginCapture();

    int failureCode = 0;
    if (dialog.descriptionPanel.captureState != 1) {
        failureCode = 3;
    }

    RestoreFunctionPatch(resetPatch);
    return failureCode;
}

extern "C" int zhud_cmd_key_b_button_on_begin_capture_smoke(void) {
    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.keyBButton.owner = &dialog;

    dialog.keyBButton.OnBeginCapture();

    return dialog.descriptionPanel.captureState == 2 ? 0 : 1;
}

extern "C" int zhud_cmd_joy_button_on_begin_capture_smoke(void) {
    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.joyButton.owner = &dialog;

    dialog.joyButton.OnBeginCapture();

    return dialog.descriptionPanel.captureState == 3 ? 0 : 1;
}

extern "C" int zhud_cmd_mouse_button_on_begin_capture_smoke(void) {
    const int oldMouseDebounceFrames = g_HudCmdMouseDebounceFrames;
    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 8;
    dialog.mouseButton.owner = &dialog;

    g_HudCmdMouseDebounceFrames = 2;
    dialog.mouseButton.OnBeginCapture();
    const bool debouncePath =
        dialog.descriptionPanel.captureState == 8;

    g_HudCmdMouseDebounceFrames = 0;
    dialog.mouseButton.OnBeginCapture();
    const bool activePath =
        dialog.descriptionPanel.captureState == 4;

    g_HudCmdMouseDebounceFrames = oldMouseDebounceFrames;
    return debouncePath && activePath ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_joystick_button_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "JoystickHolder", 0, 0, 4, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int reboundResult = dialog.ApplyJoystickButtonRebind(4, 0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const joyBegin =
        static_cast<HudCmdBindingEntry **>(dialog.joyButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 &&
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetJoystickButtonSlot(5) == 4 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.joyButton.bindingVec.end == joyBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        joyBegin[0]->commandId == 5 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.joyButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_mouse_button_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "MouseHolder", 0, 0, 0, 2);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 4, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int reboundResult = dialog.ApplyMouseButtonRebind(2, 0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const mouseBegin =
        static_cast<HudCmdBindingEntry **>(dialog.mouseButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 &&
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetMouseButtonSlot(5) == 2 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.mouseButton.bindingVec.end == mouseBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        mouseBegin[0]->commandId == 5 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.mouseButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_primary_key_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "SecondaryHolder", 0, 0x42, 0, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int ignoredResult = dialog.ApplyPrimaryKeyRebind(1, 0);
    const bool ignored =
        ignoredResult == 1 &&
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0x1e &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(3) == 0x42;

    dialog.descriptionPanel.captureState = 77;
    const int reboundResult = dialog.ApplyPrimaryKeyRebind(0x42, 0);

    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 &&
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(3) == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0x42 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        keyABegin[0]->commandId == 5 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return ignored && rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_secondary_key_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "PrimaryHolder", 0x42, 0, 0, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int ignoredResult = dialog.ApplySecondaryKeyRebind(1, 0);
    const bool ignored =
        ignoredResult == 1 &&
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0x30 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(3) == 0x42;

    dialog.descriptionPanel.captureState = 77;
    const int reboundResult = dialog.ApplySecondaryKeyRebind(0x42, 0);

    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 &&
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(3) == 0 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0x42 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        keyBBegin[0]->commandId == 5 &&
        dialog.keyBButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return ignored && rebound ? 0 : 1;
}

extern "C" int zhud_cmd_joy_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    dialog.joyButton.owner = &dialog;
    dialog.joyButton.selectedBindingIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "JoystickHolder", 0, 0, 4, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.joyButton.OnClearBinding();

    HudCmdBindingEntry **const joyBegin =
        static_cast<HudCmdBindingEntry **>(dialog.joyButton.bindingVec.begin);
    const bool cleared =
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetJoystickButtonSlot(5) == 0 &&
        dialog.joyButton.bindingVec.end == joyBegin + 1 &&
        joyBegin[0]->commandId == 5 &&
        dialog.joyButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return cleared ? 0 : 1;
}

extern "C" int zhud_cmd_mouse_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    const int oldMouseDebounceFrames = g_HudCmdMouseDebounceFrames;
    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    dialog.mouseButton.owner = &dialog;
    dialog.mouseButton.selectedBindingIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "MouseHolder", 0, 0, 0, 2);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 4, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    g_HudCmdMouseDebounceFrames = 2;
    dialog.mouseButton.OnClearBinding();
    const bool debouncePath =
        dialog.descriptionPanel.captureState == 77 &&
        zInput::BindMapCurrent_GetMouseButtonSlot(5) == 1;

    g_HudCmdMouseDebounceFrames = 0;
    dialog.mouseButton.OnClearBinding();

    HudCmdBindingEntry **const mouseBegin =
        static_cast<HudCmdBindingEntry **>(dialog.mouseButton.bindingVec.begin);
    const bool cleared =
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetMouseButtonSlot(5) == 0 &&
        dialog.mouseButton.bindingVec.end == mouseBegin + 1 &&
        mouseBegin[0]->commandId == 5 &&
        dialog.mouseButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_HudCmdMouseDebounceFrames = oldMouseDebounceFrames;
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return debouncePath && cleared ? 0 : 1;
}

extern "C" int zhud_cmd_key_a_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    dialog.keyAButton.owner = &dialog;
    dialog.keyAButton.selectedBindingIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.keyAButton.OnClearBinding();

    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    const bool cleared =
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0x30 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        keyABegin[0]->commandId == 5 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return cleared ? 0 : 1;
}

extern "C" int zhud_cmd_key_b_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    dialog.keyBButton.owner = &dialog;
    dialog.keyBButton.selectedBindingIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.keyBButton.OnClearBinding();

    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    const bool cleared =
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0x1e &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        keyBBegin[0]->commandId == 5 &&
        dialog.keyBButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return cleared ? 0 : 1;
}

extern "C" int zhud_cmd_set_list_widget_on_activate_smoke(void) {
    CodeFunctionPatch rebuildPatch{};
    if (!PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        )) {
        RestoreFunctionPatch(rebuildPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.setList.owner = &dialog;
    dialog.setList.firstIndex = 0;
    dialog.setList.selectedIndex = 1;
    dialog.setList.visibleCount = 3;
    dialog.setList.itemCount = 3;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;

    dialog.setList.OnActivate();
    const bool advanced =
        dialog.setList.selectedIndex == 2 &&
        g_cmdDialogRebuildCalls == 1 &&
        g_cmdDialogRebuildGroup == 2;

    dialog.setList.OnActivate();
    const bool wrapped =
        dialog.setList.selectedIndex == 0 &&
        g_cmdDialogRebuildCalls == 2 &&
        g_cmdDialogRebuildGroup == 0;

    RestoreFunctionPatch(rebuildPatch);
    return advanced && wrapped ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_update_capture_state_idle_smoke(void) {
    const int oldMouseDebounceFrames = g_HudCmdMouseDebounceFrames;

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.promptPanel.flags = 0;
    g_HudCmdMouseDebounceFrames = 3;

    dialog.UpdateCaptureState(0.016f);

    const bool idleUpdated =
        g_HudCmdMouseDebounceFrames == 2 &&
        (dialog.promptPanel.flags & 0x10u) != 0;

    g_HudCmdMouseDebounceFrames = oldMouseDebounceFrames;
    return idleUpdated ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_rebuild_command_binding_lists_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(
        5,
        "CmdFiveCurrent",
        0x1e,
        0x30,
        2,
        1
    );
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.RebuildCommandBindingListsForGroup(0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    HudCmdBindingEntry **const joyBegin =
        static_cast<HudCmdBindingEntry **>(dialog.joyButton.bindingVec.begin);
    HudCmdBindingEntry **const mouseBegin =
        static_cast<HudCmdBindingEntry **>(dialog.mouseButton.bindingVec.begin);
    const bool rebuilt =
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        dialog.joyButton.bindingVec.end == joyBegin + 1 &&
        dialog.mouseButton.bindingVec.end == mouseBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        keyABegin[0]->commandId == 5 &&
        keyBBegin[0]->commandId == 5 &&
        joyBegin[0]->commandId == 5 &&
        mouseBegin[0]->commandId == 5 &&
        std::strcmp(commandBegin[0]->displayText, "CommandFive") == 0 &&
        std::strcmp(keyABegin[0]->displayText, "A") == 0 &&
        std::strcmp(keyBBegin[0]->displayText, "B") == 0 &&
        std::strcmp(joyBegin[0]->displayText, "Button 2") == 0 &&
        std::strcmp(mouseBegin[0]->displayText, "Left") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        dialog.keyBButton.selectedBindingIndex == 0 &&
        dialog.joyButton.selectedBindingIndex == 0 &&
        dialog.mouseButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebuilt ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_select_group_relative_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.setList.selectedIndex = 1;
    dialog.setList.itemCount = 3;
    dialog.setList.firstIndex = 0;
    dialog.setList.visibleCount = 3;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandSevenLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    labels[7] = commandSevenLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    context.SetBindingRecord(7, "CmdSevenCurrent", 0x20, 0x31, 3, 3);
    g_zInput_BindMap_Current = &context;

    int groupZeroCommandIds[] = {5};
    int groupOneCommandIds[] = {5};
    int groupTwoCommandIds[] = {7};
    zInput_BindGroupInfo groupsStorage[3] = {};
    groupsStorage[0].commandIdsBegin = groupZeroCommandIds;
    groupsStorage[0].commandIdsEnd = groupZeroCommandIds + 1;
    groupsStorage[0].commandIdsCapacity = groupZeroCommandIds + 1;
    groupsStorage[1].commandIdsBegin = groupOneCommandIds;
    groupsStorage[1].commandIdsEnd = groupOneCommandIds + 1;
    groupsStorage[1].commandIdsCapacity = groupOneCommandIds + 1;
    groupsStorage[2].commandIdsBegin = groupTwoCommandIds;
    groupsStorage[2].commandIdsEnd = groupTwoCommandIds + 1;
    groupsStorage[2].commandIdsCapacity = groupTwoCommandIds + 1;
    zInput_BindGroupInfo *groups[] = {&groupsStorage[0], &groupsStorage[1], &groupsStorage[2]};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 3;
    g_zInput_BindGroupInfoList.capacity = groups + 3;

    const int forwardResult = dialog.SelectGroupRelative(1);
    HudCmdBindingEntry **const forwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool forward =
        forwardResult == 2 &&
        dialog.setList.selectedIndex == 2 &&
        dialog.commandList.bindingVec.end == forwardBegin + 1 &&
        forwardBegin[0]->commandId == 7;

    const int wrapForwardResult = dialog.SelectGroupRelative(1);
    HudCmdBindingEntry **const wrapForwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool wrapForward =
        wrapForwardResult == 0 &&
        dialog.setList.selectedIndex == 0 &&
        dialog.commandList.bindingVec.end == wrapForwardBegin + 1 &&
        wrapForwardBegin[0]->commandId == 5;

    const int wrapBackwardResult = dialog.SelectGroupRelative(-1);
    HudCmdBindingEntry **const wrapBackwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool wrapBackward =
        wrapBackwardResult == 2 &&
        dialog.setList.selectedIndex == 2 &&
        dialog.commandList.bindingVec.end == wrapBackwardBegin + 1 &&
        wrapBackwardBegin[0]->commandId == 7;

    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return forward && wrapForward && wrapBackward ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_select_command_relative_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    SetupHudCmdDialogButtons(dialog);
    dialog.commandList.selectedBindingIndex = 0;

    const int forwardResult = dialog.SelectCommandRelative(1);
    const bool forward =
        forwardResult == 1 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    const int outOfRangeForward = dialog.SelectCommandRelative(1);
    const bool clampedForward =
        outOfRangeForward == 1 &&
        dialog.commandList.selectedBindingIndex == 1;

    const int backwardResult = dialog.SelectCommandRelative(-1);
    const bool backward =
        backwardResult == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    const int outOfRangeBackward = dialog.SelectCommandRelative(-1);
    const bool clampedBackward =
        outOfRangeBackward == 0 &&
        dialog.commandList.selectedBindingIndex == 0;

    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return forward && clampedForward && backward && clampedBackward ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_callback_navigation_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.setList.selectedIndex = 0;
    dialog.setList.itemCount = 3;
    dialog.setList.firstIndex = 0;
    dialog.setList.visibleCount = 3;
    dialog.nextSetButton.owner = &dialog;
    dialog.prevSetButton.owner = &dialog;
    dialog.nextCommandButton.owner = &dialog;
    dialog.prevCommandButton.owner = &dialog;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandSevenLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    labels[7] = commandSevenLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    context.SetBindingRecord(7, "CmdSevenCurrent", 0x20, 0x31, 3, 3);
    g_zInput_BindMap_Current = &context;

    int groupZeroCommandIds[] = {5};
    int groupOneCommandIds[] = {7};
    int groupTwoCommandIds[] = {5};
    zInput_BindGroupInfo groupsStorage[3] = {};
    groupsStorage[0].commandIdsBegin = groupZeroCommandIds;
    groupsStorage[0].commandIdsEnd = groupZeroCommandIds + 1;
    groupsStorage[0].commandIdsCapacity = groupZeroCommandIds + 1;
    groupsStorage[1].commandIdsBegin = groupOneCommandIds;
    groupsStorage[1].commandIdsEnd = groupOneCommandIds + 1;
    groupsStorage[1].commandIdsCapacity = groupOneCommandIds + 1;
    groupsStorage[2].commandIdsBegin = groupTwoCommandIds;
    groupsStorage[2].commandIdsEnd = groupTwoCommandIds + 1;
    groupsStorage[2].commandIdsCapacity = groupTwoCommandIds + 1;
    zInput_BindGroupInfo *groups[] = {&groupsStorage[0], &groupsStorage[1], &groupsStorage[2]};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 3;
    g_zInput_BindGroupInfoList.capacity = groups + 3;

    dialog.nextSetButton.OnActivate();
    HudCmdBindingEntry **const nextSetBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool nextSet =
        dialog.setList.selectedIndex == 1 &&
        dialog.commandList.bindingVec.end == nextSetBegin + 1 &&
        nextSetBegin[0]->commandId == 7;

    dialog.prevSetButton.OnActivate();
    HudCmdBindingEntry **const prevSetBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool prevSet =
        dialog.setList.selectedIndex == 0 &&
        dialog.commandList.bindingVec.end == prevSetBegin + 1 &&
        prevSetBegin[0]->commandId == 5;

    CleanupHudCmdDialogButtons(dialog);
    SetupHudCmdDialogButtons(dialog);
    dialog.commandList.selectedBindingIndex = 0;

    dialog.nextCommandButton.OnActivate();
    const bool nextCommand =
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    dialog.prevCommandButton.OnActivate();
    const bool prevCommand =
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    dialog.descriptionPanel.Destructor();
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return nextSet && prevSet && nextCommand && prevCommand ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.title = const_cast<char *>("GroupOne");
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 1;
    group.commandIdsCapacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    g_cmdDialogLoadCalls = 0;
    g_cmdDialogLoadArgsOk = false;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;
    g_cmdDialogSetChildFlagsCalls = 0;
    g_cmdDialogSetChildFlagsValue = -1;
    void *const storage = ::operator new(sizeof(HudCmdDialog));
    std::memset(storage, 0, sizeof(HudCmdDialog));
    HudCmdDialog *const dialog = static_cast<HudCmdDialog *>(storage);
    HudCmdDialog *const result = dialog->Constructor();

    int failureCode = 0;
    if (result != dialog) {
        failureCode = 3;
    } else if (g_cmdDialogLoadCalls != 1 || !g_cmdDialogLoadArgsOk) {
        failureCode = 4;
    } else if (dialog->descriptionPanel.captureState != 0) {
        failureCode = 5;
    } else if (dialog->setList.itemCount != 0) {
        failureCode = 6;
    } else if (g_cmdDialogRebuildCalls != 1 || g_cmdDialogRebuildGroup != 0) {
        failureCode = 7;
    } else if (g_cmdDialogSetChildFlagsCalls != 1 || g_cmdDialogSetChildFlagsValue != 0) {
        failureCode = 8;
    } else if (dialog->commandList.bindingVec.begin != nullptr ||
               dialog->keyAButton.bindingVec.begin != nullptr ||
               dialog->mouseButton.bindingVec.begin != nullptr) {
        failureCode = 9;
    }

    CleanupHudCmdDialogButtons(*dialog);
    dialog->descriptionPanel.Destructor();
    ::operator delete(storage);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return failureCode;
}

extern "C" int zhud_cmd_dialog_destructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    HudCmdDialog *const dialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(dialog, 0, sizeof(HudCmdDialog));
    dialog->Constructor();

    InstallHudCmdDialogBinding(dialog->commandList, "Command");
    InstallHudCmdDialogBinding(dialog->keyAButton, "KeyA");
    InstallHudCmdDialogBinding(dialog->keyBButton, "KeyB");
    InstallHudCmdDialogBinding(dialog->joyButton, "Joy");
    InstallHudCmdDialogBinding(dialog->mouseButton, "Mouse");

    dialog->Destructor();

    const bool bindingsCleared =
        dialog->commandList.bindingVec.begin == nullptr &&
        dialog->commandList.bindingVec.end == nullptr &&
        dialog->commandList.bindingVec.capacity == nullptr &&
        dialog->keyAButton.bindingVec.begin == nullptr &&
        dialog->keyAButton.bindingVec.end == nullptr &&
        dialog->keyAButton.bindingVec.capacity == nullptr &&
        dialog->keyBButton.bindingVec.begin == nullptr &&
        dialog->keyBButton.bindingVec.end == nullptr &&
        dialog->keyBButton.bindingVec.capacity == nullptr &&
        dialog->joyButton.bindingVec.begin == nullptr &&
        dialog->joyButton.bindingVec.end == nullptr &&
        dialog->joyButton.bindingVec.capacity == nullptr &&
        dialog->mouseButton.bindingVec.begin == nullptr &&
        dialog->mouseButton.bindingVec.end == nullptr &&
        dialog->mouseButton.bindingVec.capacity == nullptr;

    ::operator delete(dialog);
    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return bindingsCleared ? 0 : 1;
}

extern "C" int zhud_cmd_command_list_destructor_smoke(void) {
    HudCmdCommandList button{};
    InstallHudCmdDialogBinding(button, "Command");

    button.Destructor();

    return HudCmdButtonDestructorCleared(button) ? 0 : 1;
}

extern "C" int zhud_cmd_binding_entry_copy_range_smoke(void) {
    HudCmdBindingEntry first{};
    HudCmdBindingEntry second{};
    HudCmdBindingEntry third{};
    HudCmdBindingEntry *source[3] = {&first, &second, &third};
    HudCmdBindingEntry *dest[4] = {};

    HudCmdBindingEntry **const emptyResult =
        HudCmdBindingEntry::CopyRange(source, source, dest);
    if (emptyResult != dest || dest[0] != nullptr) {
        return 1;
    }

    HudCmdBindingEntry **const result =
        HudCmdBindingEntry::CopyRange(source, source + 3, dest);
    HudCmdBindingEntry owned{};
    owned.displayText = static_cast<char *>(std::malloc(6));
    std::strcpy(owned.displayText, "Entry");
    owned.~HudCmdBindingEntry();

    HudCmdBindingEntry *const deleteEntry =
        static_cast<HudCmdBindingEntry *>(::operator new(sizeof(HudCmdBindingEntry)));
    new (deleteEntry) HudCmdBindingEntry;
    deleteEntry->displayText = _strdup("Delete");
    deleteEntry->commandId = 9;
    HudCmdBindingEntry *const deleteResult =
        HudCmdBindingEntry::DeleteAndReturnNull(deleteEntry);
    HudCmdBindingEntry *const nullDeleteResult =
        HudCmdBindingEntry::DeleteAndReturnNull(nullptr);

    return result == dest + 3 && dest[0] == &first && dest[1] == &second &&
                   dest[2] == &third && dest[3] == nullptr &&
                   owned.displayText == nullptr && deleteResult == nullptr &&
                   nullDeleteResult == nullptr
               ? 0
               : 2;
}

extern "C" int zhud_cmd_command_list_scalar_deleting_destructor_smoke(void) {
    HudCmdCommandList button{};
    InstallHudCmdDialogBinding(button, "Command");

    HudUiElement *const returned = button.ScalarDeletingDestructor(0);
    const bool noDeletePath =
        returned == &button &&
        HudCmdButtonDestructorCleared(button);

    HudCmdCommandList *const deletingButton =
        static_cast<HudCmdCommandList *>(::operator new(sizeof(HudCmdCommandList)));
    std::memset(deletingButton, 0, sizeof(HudCmdCommandList));
    new (deletingButton) HudCmdCommandList;
    InstallHudCmdDialogBinding(*deletingButton, "DeleteCommand");
    HudUiElement *const deletedReturn = deletingButton->ScalarDeletingDestructor(1);
    const bool deletePath = deletedReturn == deletingButton;

    return noDeletePath && deletePath ? 0 : 1;
}

extern "C" int zhud_cmd_key_a_button_destructor_smoke(void) {
    HudCmdKeyAButton button{};
    InstallHudCmdDialogBinding(button, "KeyA");

    button.Destructor();

    return HudCmdButtonDestructorCleared(button) ? 0 : 1;
}

extern "C" int zhud_cmd_key_b_button_destructor_smoke(void) {
    HudCmdKeyBButton button{};
    InstallHudCmdDialogBinding(button, "KeyB");

    button.Destructor();

    return HudCmdButtonDestructorCleared(button) ? 0 : 1;
}

extern "C" int zhud_cmd_joy_button_destructor_smoke(void) {
    HudCmdJoyButton button{};
    InstallHudCmdDialogBinding(button, "Joy");

    button.Destructor();

    return HudCmdButtonDestructorCleared(button) ? 0 : 1;
}

extern "C" int zhud_cmd_mouse_button_destructor_smoke(void) {
    HudCmdMouseButton button{};
    InstallHudCmdDialogBinding(button, "Mouse");

    button.Destructor();

    return HudCmdButtonDestructorCleared(button) ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_scalar_deleting_destructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    HudCmdDialog *const dialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(dialog, 0, sizeof(HudCmdDialog));
    dialog->Constructor();
    InstallHudCmdDialogBinding(dialog->commandList, "Command");

    HudCmdDialog *const returned = dialog->ScalarDeletingDestructor(0);
    const bool noDeletePath =
        returned == dialog &&
        dialog->commandList.bindingVec.begin == nullptr &&
        dialog->commandList.bindingVec.end == nullptr &&
        dialog->commandList.bindingVec.capacity == nullptr;
    ::operator delete(dialog);

    HudCmdDialog *const deletingDialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(deletingDialog, 0, sizeof(HudCmdDialog));
    deletingDialog->Constructor();
    InstallHudCmdDialogBinding(deletingDialog->commandList, "DeleteCommand");
    deletingDialog->ScalarDeletingDestructor(1);

    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return noDeletePath ? 0 : 1;
}

extern "C" int zhud_text_input_constructor_smoke(void) {
    HudUiTextInput input{};
    HudUiTextInput *const result = input.Constructor(8);

    const bool constructed =
        result == &input &&
        input.buffer != nullptr &&
        input.capacity == 8 &&
        input.cursor == 0 &&
        input.keyActionMap[0x20] == 0 &&
        input.keyActionMap[0x2e] == 0 &&
        input.keyActionMap[0x1b] == 2 &&
        input.keyActionMap[0x0d] == 3 &&
        input.keyActionMap[0x08] == 4 &&
        input.keyActionMap[0x7f] == 5 &&
        input.keyActionMap[0x02] == 6 &&
        input.keyActionMap[0x06] == 7 &&
        input.keyActionMap['A'] == 0 &&
        input.keyActionMap[1] == 1;

    input.SetContents("abcdef");
    const bool contents =
        std::strcmp(input.GetBuffer(), "abcdef") == 0 &&
        input.cursor == 0;

    input.DestructorCore();
    return constructed && contents ? 0 : 1;
}

extern "C" int zhud_text_input_constructor_and_alloc_smoke(void) {
    return zhud_text_input_constructor_smoke();
}

extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x20;

    HudUiPolyline polyline{};
    RECT clip{1, 2, 3, 4};
    polyline.color565 = 0x1234;
    polyline.clipRect = &clip;
    polyline.points[5].x = 11;
    polyline.points[5].y = 12;
    polyline.pointCount = 9;
    polyline.Constructor();

    bool pointsCleared = true;
    for (const HudUiPolylinePoint &point : polyline.points) {
        pointsCleared = pointsCleared && point.x == 0 && point.y == 0;
    }

    const bool constructed =
        (polyline.flags & 0x20) != 0 &&
        polyline.pointCount == 0 &&
        polyline.color565 == 0x1234 &&
        polyline.clipRect == nullptr &&
        pointsCleared;

    polyline.flags = 0;
    polyline.SetPoint(0, 5, 6);
    const bool firstPoint =
        polyline.points[0].x == 5 &&
        polyline.points[0].y == 6 &&
        polyline.pointCount == 1 &&
        polyline.x == 5 &&
        polyline.y == 6 &&
        (polyline.flags & 0x20) != 0;

    polyline.flags = 0;
    polyline.SetPoint(3, 7, 8);
    const bool laterPoint =
        polyline.points[3].x == 7 &&
        polyline.points[3].y == 8 &&
        polyline.pointCount == 4 &&
        polyline.x == 5 &&
        polyline.y == 6 &&
        (polyline.flags & 0x20) != 0;

    HudUiSliderBorder border{};
    border.caretHalfWidth = 0x55;
    border.inputActive = 0x66;
    border.sliderVisibleWhenInputActive = 0x12;
    border.rawKeyFilterEnabled = 0x34;
    border.Constructor();

    const HudUiPolylinePoint expected[] = {
        {-1, 0},  {1, 0},  {1, 1}, {0, 1}, {0, 9},  {1, 9},  {1, 10},
        {-1, 10}, {-1, 9}, {0, 9}, {0, 1}, {-1, 1}, {-1, 0},
    };

    bool borderPoints = true;
    for (int index = 0; index < 13; ++index) {
        borderPoints = borderPoints &&
                       border.points[index].x == expected[index].x &&
                       border.points[index].y == expected[index].y;
    }

    const bool borderConstructed =
        border.pointCount == 13 &&
        border.x == -1 &&
        border.y == 0 &&
        border.originX == 0 &&
        border.originY == 0 &&
        border.halfWidth == 1 &&
        border.height == 10 &&
        border.blinkEnabled == 0 &&
        border.blinkPeriodSec == 0.35f &&
        border.blinkTimeRemainingSec == 0.0f &&
        border.blinkDirSign == 1 &&
        border.caretHalfWidth == 0x55 &&
        border.inputActive == 0x66 &&
        border.sliderVisibleWhenInputActive == 0x12 &&
        border.rawKeyFilterEnabled == 0x34 &&
        borderPoints;

    border.SetBounds(10, 20, 3, 8);
    const HudUiPolylinePoint expectedBounds[] = {
        {7, 20}, {13, 20}, {13, 21}, {10, 21}, {10, 27}, {13, 27}, {13, 28},
        {7, 28}, {7, 27},  {10, 27}, {10, 21}, {7, 21},  {7, 20},
    };

    bool boundsPoints = true;
    for (int index = 0; index < 13; ++index) {
        boundsPoints = boundsPoints &&
                       border.points[index].x == expectedBounds[index].x &&
                       border.points[index].y == expectedBounds[index].y;
    }

    const bool boundsSet =
        border.originX == 10 &&
        border.originY == 20 &&
        border.halfWidth == 3 &&
        border.height == 8 &&
        border.pointCount == 13 &&
        border.x == 7 &&
        border.y == 20 &&
        boundsPoints;

    g_HudUi_InvalidateMask = oldMask;
    return constructed && firstPoint && laterPoint && borderConstructed && boundsSet ? 0 : 1;
}

extern "C" int zhud_numeric_text_input_base_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiNumericTextInput input{};
    HudUiNumericTextInput *const result = input.BaseConstructor();

    const bool constructed =
        result == &input &&
        input.modeOrEnabled == 1 &&
        input.textInput.buffer != nullptr &&
        input.textInput.capacity == 0x100 &&
        input.textInput.cursor == 0 &&
        input.textInput.owner == &input &&
        input.sliderBorder.sliderVisibleWhenInputActive == 0 &&
        input.sliderBorder.rawKeyFilterEnabled == 0 &&
        input.sliderBorder.inputActive == 1 &&
        input.sliderBorder.caretHalfWidth == 0 &&
        (input.sliderBorder.flags & 0x10u) == 0 &&
        (input.flags & 0x10u) == 0;

    input.Update("42");
    const bool updated =
        std::strcmp(input.textInput.buffer, "42") == 0 &&
        input.textInput.cursor == 2 &&
        (input.flags & 0x80u) != 0;

    const int previousActive = input.SetInputActive(0);
    const bool disabled =
        previousActive == 1 &&
        input.sliderBorder.inputActive == 0 &&
        (input.flags & 0x10u) != 0 &&
        (input.sliderBorder.flags & 0x10u) != 0;

    const int previousInactive = input.SetInputActive(1);
    const bool enabled =
        previousInactive == 0 &&
        input.sliderBorder.inputActive == 1 &&
        (input.flags & 0x10u) == 0 &&
        (input.sliderBorder.flags & 0x10u) == 0;

    input.Destructor();
    g_HudUi_InvalidateMask = oldMask;
    return constructed && updated && disabled && enabled ? 0 : 1;
}

extern "C" int zhud_cycle_selector_text_entry_smoke(void) {
    HudUiBackground background{};
    background.fontStyles[1].validMarker = 1;
    background.fontStyles[1].fontName = "Arial";
    background.fontStyles[1].fontSize = 12;
    background.fontStyles[1].fontWeight = FW_BOLD;
    background.fontStyles[1].textColor = 0x00112233;
    background.fontStyles[1].shadowEnabled = 1;
    background.fontStyles[1].alignMode = 2;
    background.fontStyles[1].bkMode = 3;
    background.fontStyles[1].bkColor = 0x00445566;

    HudUiCycleSelectorWidget widget{};
    widget.owner = &background;
    widget.visibleCount = 1;
    widget.textOffsetX = 11;
    widget.textOffsetY = -2;

    widget.AddTextEntry(
        3,
        "Bravo",
        7,
        9
    );
    widget.ApplyFontStyleForEntry(
        3,
        1
    );

    HudUiTransitionTextPanel *const textEntry =
        reinterpret_cast<HudUiTransitionTextPanel *>(widget.entriesA[3]);
    const bool added =
        widget.itemCount == 4 &&
        widget.visibleCount == 4 &&
        textEntry != nullptr &&
        std::strcmp(textEntry->textBuffer, "Bravo") == 0 &&
        textEntry->x == 18 &&
        textEntry->y == 7 &&
        (textEntry->flags & 0x10u) != 0 &&
        textEntry->flashCountdown == 0.0f &&
        textEntry->flashResetValue == 0.349999994f &&
        textEntry->flashEnabled == 0 &&
        textEntry->flashMode == 0 &&
        textEntry->flashDirectionSign == 1 &&
        background.childHead == textEntry &&
        background.childTail == textEntry &&
        textEntry->parent == &background;

    const bool styled =
        textEntry != nullptr &&
        textEntry->textColor0 == 0x00112233 &&
        textEntry->textColor1 == 0x00112233 &&
        textEntry->textDirty == 1 &&
        textEntry->shadowEnabled == 1 &&
        textEntry->shadowOffsetX == 1 &&
        textEntry->shadowOffsetY == 1 &&
        textEntry->alignMode == 2 &&
        textEntry->bkMode == 3 &&
        textEntry->bkColor == 0x00445566;

    if (textEntry != nullptr) {
        textEntry->Destructor();
        ::operator delete(textEntry);
        widget.entriesA[3] = nullptr;
    }
    background.childHead = nullptr;
    background.childTail = nullptr;

    return added && styled ? 0 : 1;
}

extern "C" int zhud_transition_text_panel_flash_rate_smoke(void) {
    HudUiTransitionTextPanel resetPanel{};
    resetPanel.flashResetValue = 0.25f;
    resetPanel.ResetFlashState(6.0f);
    const bool reset =
        resetPanel.flashEnabled == 1 &&
        resetPanel.flashMode == 0 &&
        resetPanel.flashResetValue == 3.0f &&
        resetPanel.flashCountdown == 3.0f &&
        resetPanel.flashDirectionSign == 1;
    resetPanel.Destructor();

    HudUiTransitionTextPanel ratePanel{};
    ratePanel.flashMode = 0;
    ratePanel.SetFlashRate(4.0f);
    const bool rateSet =
        ratePanel.flashEnabled == 1 &&
        ratePanel.flashMode == 1 &&
        ratePanel.flashResetValue == 2.0f &&
        ratePanel.flashCountdown == 2.0f &&
        ratePanel.flashDirectionSign == 1;

    ratePanel.flashResetValue = 0.25f;
    ratePanel.flashCountdown = 0.5f;
    ratePanel.flashDirectionSign = -1;
    ratePanel.SetFlashRate(8.0f);
    const bool alreadySet =
        ratePanel.flashMode == 1 &&
        ratePanel.flashResetValue == 0.25f &&
        ratePanel.flashCountdown == 0.5f &&
        ratePanel.flashDirectionSign == -1;
    ratePanel.Destructor();

    return reset && rateSet && alreadySet ? 0 : 1;
}

extern "C" int zhud_std_ptr_vector_clear_no_op_destroy_smoke(void) {
    int values[3] = {1, 2, 3};
    StdPtrVector vector{};
    vector.ClearNoOpDestroy(values, values + 3);
    vector.ClearNoOpDestroy(values + 1, values + 1);

    return values[0] == 1 && values[1] == 2 && values[2] == 3 ? 0 : 1;
}

extern "C" int zhud_zrd_widget_helpers_smoke(void) {
    HudUiPanel *panels[] = {
        reinterpret_cast<HudUiPanel *>(0x1000),
        reinterpret_cast<HudUiPanel *>(0x2000),
        reinterpret_cast<HudUiPanel *>(0x3000),
        reinterpret_cast<HudUiPanel *>(0x4000),
    };
    HudUiPanelPtrVector vector{};
    vector.begin = panels;
    vector.end = panels + 4;
    vector.capacityEnd = panels + 4;

    HudUiPanel **const result = vector.EraseRange(panels + 1, panels + 3);
    const bool erased =
        result == panels + 1 &&
        vector.end == panels + 2 &&
        panels[0] == reinterpret_cast<HudUiPanel *>(0x1000) &&
        panels[1] == reinterpret_cast<HudUiPanel *>(0x4000);

    HudUiPanelPtrVector insertVector{};
    HudUiPanel *insertA = reinterpret_cast<HudUiPanel *>(0x1110);
    HudUiPanel *insertB = reinterpret_cast<HudUiPanel *>(0x2220);
    insertVector.InsertN(nullptr, 1, &insertA);
    insertVector.InsertN(insertVector.begin, 2, &insertB);
    const bool inserted =
        insertVector.begin != nullptr &&
        insertVector.end == insertVector.begin + 3 &&
        insertVector.capacityEnd >= insertVector.end &&
        insertVector.begin[0] == reinterpret_cast<HudUiPanel *>(0x2220) &&
        insertVector.begin[1] == reinterpret_cast<HudUiPanel *>(0x2220) &&
        insertVector.begin[2] == reinterpret_cast<HudUiPanel *>(0x1110);
    ::operator delete(insertVector.begin);

    return erased && inserted ? 0 : 1;
}
