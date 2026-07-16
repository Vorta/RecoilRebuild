/* Body include for the physical hud.cpp command-binding layer [0x40a5b0,0x40c370). */
/* Included by src/Battlesport/hud.cpp; keep this file body-only. */

/**
 * Reimplements 0x40a5b0: HudCmdDialog::HudCmdDialog.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: construct the command-binding dialog, bind its ZRD widgets, and
 * populate command groups before enabling the container children.
 */
HudCmdDialog::HudCmdDialog() {
    zReader::Node *const loadedSection = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        "COMMANDS_DIALOG",
        0
    );
    if (loadedSection != 0) {
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&resumeButton),
            "CMD_RESUME_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&resetButton),
            "CMD_RESET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&commandList),
            "CMD_COMMAND_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&keyAButton),
            "CMD_KEYA_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&keyBButton),
            "CMD_KEYB_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&joyButton),
            "CMD_JOY_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&mouseButton),
            "CMD_MOUSE_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&setList),
            "CMD_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&nextSetButton),
            "CMD_NEXT_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&prevSetButton),
            "CMD_PREV_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&nextCommandButton),
            "CMD_NEXT_CMD_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&prevCommandButton),
            "CMD_PREV_CMD_BTN"
        );

        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&promptPanel),
            "PRESS_A_KEY"
        );
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&descriptionPanel),
            "CMD_DESCRIPTION"
        );
        HudUiBackground::FreeLoadedTreeRoots(0);
        promptPanel.SetFlashRate(1.0f);
    }

    promptPanel.SetVisible(0);

    for (int groupIndex = 0; groupIndex < zInput::BindGroupList_GetCount(); ++groupIndex) {
        setList.AddTextEntry(
            groupIndex,
            zInput::BindGroupList_GetGroupTitle(groupIndex),
            setList.textOffsetX,
            setList.textOffsetY
        );
        setList.ApplyFontStyleForEntry(
            groupIndex,
            (int)((unsigned int)(setList.fontStyleRef))
        );
    }

    RebuildCommandBindingListsForGroup(0);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    ((HudUiContainer *)(this))->SetChildFlags(0);
}

/**
 * Reimplements 0x40adf0: HudCmdDialog::~HudCmdDialog.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: let ordinary C++ member and base lifetime rules tear down the
 * command dialog in reverse construction order.
 */
HudCmdDialog::~HudCmdDialog() {
}

/**
 * Reimplements 0x40b140: HudCmdDialog::UpdateAll.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: advance the recovered HUD update path through the dialog's primary virtual update.
 */
void HudCmdDialog::UpdateAll(
    float deltaTime
) {
    HudUiBackgroundContainer::UpdateAll(deltaTime);

    switch (descriptionPanel.captureState) {
    case 0:
        promptPanel.SetVisible(0);
        --g_HudCmdMouseDebounceFrames;
        break;

    case 1: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired keyboard key.");
        keyBButton.SetChecked(0);
        joyButton.SetChecked(0);
        mouseButton.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0) {
            ApplyPrimaryKeyRebind(
                keyCode,
                keyAButton.selectedBindingIndex
            );
            keyAButton.SetChecked(0);
        }
        break;
    }

    case 2: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired keyboard key.");
        keyAButton.SetChecked(0);
        joyButton.SetChecked(0);
        mouseButton.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0) {
            ApplySecondaryKeyRebind(
                keyCode,
                keyBButton.selectedBindingIndex
            );
            keyBButton.SetChecked(0);
        }
        break;
    }

    case 3: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired joystick button.");
        keyAButton.SetChecked(0);
        keyBButton.SetChecked(0);
        mouseButton.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1) {
            descriptionPanel.captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::DI_WaitForButtonPress(0);
        if (buttonCode != 0) {
            ApplyJoystickButtonRebind(
                buttonCode,
                joyButton.selectedBindingIndex
            );
            joyButton.SetChecked(0);
        }
        break;
    }

    case 4: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired mouse button.");
        keyAButton.SetChecked(0);
        keyBButton.SetChecked(0);
        joyButton.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1) {
            descriptionPanel.captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::Mouse_WaitForButtonPress(0);
        if (buttonCode != 0) {
            ApplyMouseButtonRebind(
                buttonCode,
                mouseButton.selectedBindingIndex
            );
            mouseButton.SetChecked(0);
            g_HudCmdMouseDebounceFrames = 10;
        }
        break;
    }

    default:
        break;
    }
}

/**
 * Reimplements 0x40b3e0: HudCmdDialog::ApplyPrimaryKeyRebind.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplyPrimaryKeyRebind.
 */
int HudCmdDialog::ApplyPrimaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    if (keyCode != 1) {
        const int primaryCommand = zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode);
        const int groupIndex = setList.selectedIndex;
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        if (primaryCommand == 0 && zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode) != 0) {
            zInput::BindMapCurrent_SetSecondaryKeyBinding(
                keyCode,
                0
            );
        }

        zInput::BindMapCurrent_SetPrimaryKeyBinding(
            keyCode,
            commandId
        );
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * Reimplements 0x40b460: HudCmdDialog::ApplySecondaryKeyRebind.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplySecondaryKeyRebind.
 */
int HudCmdDialog::ApplySecondaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    if (keyCode != 1) {
        const int secondaryCommand = zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode);
        const int groupIndex = setList.selectedIndex;
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        if (secondaryCommand == 0 && zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode) != 0) {
            zInput::BindMapCurrent_SetPrimaryKeyBinding(
                keyCode,
                0
            );
        }

        zInput::BindMapCurrent_SetSecondaryKeyBinding(
            keyCode,
            commandId
        );
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * Reimplements 0x40b4e0: HudCmdDialog::ApplyJoystickButtonRebind.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplyJoystickButtonRebind.
 */
int HudCmdDialog::ApplyJoystickButtonRebind(
    int buttonCode,
    int commandIndex
) {
    const int joystickCommand = zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode);
    const int groupIndex = setList.selectedIndex;
    const int commandId = zInput::BindGroupList_GetGroupCommandId(
        groupIndex,
        commandIndex
    );
    if (joystickCommand == 0 && zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode) != 0) {
        zInput::BindMapCurrent_SetJoystickBinding(
            buttonCode,
            0
        );
    }

    zInput::BindMapCurrent_SetJoystickBinding(
        buttonCode,
        commandId
    );
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * Reimplements 0x40b560: HudCmdDialog::ApplyMouseButtonRebind.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplyMouseButtonRebind.
 */
int HudCmdDialog::ApplyMouseButtonRebind(
    int buttonCode,
    int commandIndex
) {
    const int mouseCommand = zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode);
    const int groupIndex = setList.selectedIndex;
    const int commandId = zInput::BindGroupList_GetGroupCommandId(
        groupIndex,
        commandIndex
    );
    if (mouseCommand == 0 && zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode) != 0) {
        zInput::BindMapCurrent_SetMouseBinding(
            buttonCode,
            0
        );
    }

    zInput::BindMapCurrent_SetMouseBinding(
        buttonCode,
        commandId
    );
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * Reimplements 0x40b5e0: HudCmdDialog::SelectGroupRelative.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for HudCmdDialog::SelectGroupRelative.
 */
int HudCmdDialog::SelectGroupRelative(
    int delta
) {
    int groupIndex = setList.selectedIndex + delta;
    if (groupIndex >= setList.itemCount) {
        groupIndex = 0;
    } else if (groupIndex < 0) {
        groupIndex = setList.itemCount - 1;
    }

    setList.SetIndexClamped(groupIndex);
    const int selectedIndex = setList.selectedIndex;
    RebuildCommandBindingListsForGroup(selectedIndex);
    return selectedIndex;
}

/**
 * Reimplements 0x40b630: HudCmdDialog::SelectCommandRelative.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for HudCmdDialog::SelectCommandRelative.
 */
int HudCmdDialog::SelectCommandRelative(
    int delta
) {
    int selectedIndex = delta;
    selectedIndex += commandList.selectedBindingIndex;
    if (selectedIndex >= 0) {
        HudCmdBindingEntry **const begin = commandList.bindingVec.begin();
        int count;
        if (begin == 0) {
            count = 0;
        } else {
            count = (int)(commandList.bindingVec.end() - begin);
        }
        if (selectedIndex < count) {
            commandList.SetSelectedEntry(selectedIndex);
        }
    }

    const int currentIndex = commandList.selectedBindingIndex;
    OnCommandSelectionChanged(currentIndex);
    return currentIndex;
}

/**
 * Reimplements 0x40b680: HudCmdDialog::RebuildCommandBindingListsForGroup.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for HudCmdDialog::RebuildCommandBindingListsForGroup.
 */
void HudCmdDialog::RebuildCommandBindingListsForGroup(
    int groupIndex
) {
    commandList.ClearBindingEntries();
    keyAButton.ClearBindingEntries();
    keyBButton.ClearBindingEntries();
    joyButton.ClearBindingEntries();
    mouseButton.ClearBindingEntries();

    int commandIndex;
    for (commandIndex = 0; commandIndex < zInput::BindGroupList_GetGroupCommandCount(groupIndex);
        ++commandIndex) {
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        char labelBuffer[40];
        zInput::BindMapCurrent_CopyCommandLabel(
            commandId,
            labelBuffer,
            sizeof(labelBuffer)
        );
        if (strlen(labelBuffer) != 0) {
            commandList.AddBindingEntry(
                zInput::BindMap_GetCommandLabel(commandId),
                commandId
            );
            keyAButton.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetPrimaryKeyboardKey(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            keyBButton.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetSecondaryKeyboardKey(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            joyButton.AddBindingEntry(
                zInput::BindMapCurrent_CopyJoystickButtonName(
                    zInput::BindMapCurrent_GetJoystickButtonSlot(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            mouseButton.AddBindingEntry(
                zInput::BindMapCurrent_CopyMouseButtonName(
                    zInput::BindMapCurrent_GetMouseButtonSlot(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
        }
    }

    OnCommandSelectionChanged(0);
}

/**
 * Reimplements 0x40b930: HudCmdResetButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdResetButton::OnActivate.
 */
void HudCmdResetButton::OnActivate() {
    HudCmdDialog *const dialog = (HudCmdDialog *)(owner);
    zInput::BindMap_InitDefaultBindings();
    zInput::BindMap_Current_RebuildLookupIndices();
    dialog->RebuildCommandBindingListsForGroup(dialog->setList.selectedIndex);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40b960: HudCmdSetListWidget::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Advance the set-list selector and rebuild command bindings for the
 * selected group.
 */
void HudCmdSetListWidget::OnActivate() {
    AdvanceSelectionAndActivate();
    ((HudCmdDialog *)(owner))->RebuildCommandBindingListsForGroup(selectedIndex);
}

/**
 * Reimplements 0x40b980: HudCmdDialog::OnCommandSelectionChanged.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Binary Ninja clears the description panel capture state, resets zInput
 * transition state, selects the same entry in each command binding list, then
 * resolves the selected command hint through zInput::BindMap_GetCommandHint.
 * Purpose: Refresh the command dialog selection and description text.
 */
void HudCmdDialog::OnCommandSelectionChanged(
    int commandIndex
) {
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    HudCmdBindButtonBase *const commandButton = &commandList;
    commandButton->SetSelectedEntry(commandIndex);
    keyAButton.SetSelectedEntry(commandIndex);
    keyBButton.SetSelectedEntry(commandIndex);
    joyButton.SetSelectedEntry(commandIndex);
    mouseButton.SetSelectedEntry(commandIndex);

    HudCmdBindingEntry **const entries = commandButton->bindingVec.begin();
    HudCmdBindingEntry *const selectedEntry = entries[commandButton->selectedBindingIndex];
    char *const hint = zInput::BindMap_GetCommandHint(selectedEntry->commandId);
    if (hint != 0) {
        descriptionPanel.SetTextFmt(
            "%s",
            hint
        );
    } else {
        descriptionPanel.SetTextFmt("");
    }
}

/**
 * Reimplements 0x40ba30: HudCmdKeyAButton::OnBeginCapture.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdKeyAButton::OnBeginCapture.
 */
void HudCmdKeyAButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 1;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40ba60: HudCmdKeyAButton::OnClearBinding.
 * Original file: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the primary-key binding for the selected command row.
 */
void HudCmdKeyAButton::OnClearBinding() {
    const int selectedIndex = selectedBindingIndex;
    ((HudCmdDialog *)(owner))->ApplyPrimaryKeyRebind(
        0,
        selectedIndex
    );
    SetSelectedEntry(selectedIndex);
}

/**
 * Reimplements 0x40ba90: HudCmdBindButtonBase::OnSelectionChangedRefresh.
 * Original file: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: forward a bind-button selection change to the owning command dialog.
 */
void HudCmdBindButtonBase::OnSelectionChangedRefresh(
    int selectedIndex
) {
    ((HudCmdDialog *)(owner))->OnCommandSelectionChanged(selectedIndex);
}

/**
 * Reimplements 0x40bab0: HudCmdKeyBButton::OnBeginCapture.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdKeyBButton::OnBeginCapture.
 */
void HudCmdKeyBButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 2;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bae0: HudCmdKeyBButton::OnClearBinding.
 * Original file: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the secondary-key binding for the selected command row.
 */
void HudCmdKeyBButton::OnClearBinding() {
    ((HudCmdDialog *)(owner))->ApplySecondaryKeyRebind(
        0,
        selectedBindingIndex
    );
}

/**
 * Reimplements 0x40bb00: HudCmdJoyButton::OnBeginCapture.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdJoyButton::OnBeginCapture.
 */
void HudCmdJoyButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 3;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bb30: HudCmdJoyButton::OnClearBinding.
 * Original file: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the joystick binding for the selected command row.
 */
void HudCmdJoyButton::OnClearBinding() {
    ((HudCmdDialog *)(owner))
        ->ApplyJoystickButtonRebind(
            0,
            selectedBindingIndex
        );
}

/**
 * Reimplements 0x40bb50: HudCmdMouseButton::OnBeginCapture.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdMouseButton::OnBeginCapture.
 */
void HudCmdMouseButton::OnBeginCapture() {
    if (g_HudCmdMouseDebounceFrames > 0) {
        return;
    }

    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 4;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bb80: HudCmdMouseButton::OnClearBinding.
 * Original file: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the mouse binding for the selected command row when debounce is inactive.
 */
void HudCmdMouseButton::OnClearBinding() {
    if (g_HudCmdMouseDebounceFrames > 0) {
        return;
    }

    ((HudCmdDialog *)(owner))->ApplyMouseButtonRebind(
        0,
        selectedBindingIndex
    );
}

/**
 * Reimplements 0x40bba0: HudCmdNextSetButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdNextSetButton::OnActivate.
 */
void HudCmdNextSetButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectGroupRelative(1);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bbc0: HudCmdPrevSetButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdPrevSetButton::OnActivate.
 */
void HudCmdPrevSetButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectGroupRelative(-1);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bbe0: HudCmdNextCommandButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdNextCommandButton::OnActivate.
 */
void HudCmdNextCommandButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectCommandRelative(1);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bc00: HudCmdPrevCommandButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdPrevCommandButton::OnActivate.
 */
void HudCmdPrevCommandButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectCommandRelative(-1);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40bc20: HudCmdDialogState::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Construct the global command-dialog state and register its at-exit teardown.
 */
void HudCmdDialogState::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x40bc30: HudCmdDialogState::StaticInit.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Construct the command-dialog state in its static storage.
 */
HudCmdDialogState *HudCmdDialogState::StaticInit() {
    return new (&g_HudCmdDialogState) HudCmdDialogState;
}

/**
 * Reimplements 0x40bc40: HudCmdDialogState::RegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Register the command-dialog state static destructor with the CRT.
 */
void HudCmdDialogState::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x40bc50: HudCmdDialogState::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Destroy the global command-dialog state during CRT at-exit cleanup.
 */
void HudCmdDialogState::AtExitDestructor() {
    g_HudCmdDialogState.HudCmdDialogState::~HudCmdDialogState();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudCmdDialogStateCrtInitializerFn)();
/* VC5 emits this command-dialog startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
HudCmdDialogStateCrtInitializerFn s_HudCmdDialogStateCrtInit =
    HudCmdDialogState::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Reimplements 0x40bc60: HudCmdDialogState::HudCmdDialogState.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Initialize the command-dialog app state with no active dialog.
 */
HudCmdDialogState::HudCmdDialogState() {
    m_dialog = 0;
}

/**
 * Reimplements 0x40bc90: HudCmdDialogState::~HudCmdDialogState.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Delete any active command dialog owned by the state during teardown.
 */
HudCmdDialogState::~HudCmdDialogState() {
    HudCmdDialog *const dialog = (HudCmdDialog *)m_dialog;
    if (dialog != 0) {
        delete dialog;
        m_dialog = 0;
    }
}

/**
 * Reimplements 0x40bcf0: HudCmdDialogState::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Allocate the 0xce00-byte command dialog, construct and store it,
 * enable it, suspend keyboard input, and accept the state transition.
 */
int HudCmdDialogState::OnTryBecomeCurrent() {
    HudCmdDialog *dialog = new HudCmdDialog;
    m_dialog = dialog;

    dialog->SetEnabled(1);
    zInput::Keyboard_Suspend();
    return 1;
}

/**
 * Reimplements 0x40bd60: HudCmdDialogState::OnDeactivate.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Resume keyboard input, disable and dispose the active command
 * dialog, clear it, and rebuild current input-map lookup indices.
 */
void HudCmdDialogState::OnDeactivate() {
    zInput::Keyboard_ResumeFromSuspend();

    HudCmdDialog *dialog = (HudCmdDialog *)m_dialog;
    if (dialog == 0) {
        return;
    }

    dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    dialog = (HudCmdDialog *)m_dialog;
    if (dialog != 0) {
        delete dialog;
    }

    m_dialog = 0;
    zInput::BindMap_Current_RebuildLookupIndices();
}

/**
 * Reimplements 0x40bda0: HudCmdDialogState::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Queue the global command-dialog app state for entry.
 */
void HudCmdDialogState::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudCmdDialogState,
        0
    );
}

/**
 * Reimplements 0x40bdf0: StdPtrVector::ClearNoOpDestroy.
 * Purpose: preserve the recovered HUD behavior for StdPtrVector::ClearNoOpDestroy.
 */
void StdPtrVector::ClearNoOpDestroy(
    int *begin,
    int *end
) {
    (void)begin;
    (void)end;
}

/**
 * Reimplements 0x40be90: HudUiPanel::Invalidate.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::Invalidate.
 */
void HudUiPanel::Invalidate() {
    textDirty = 1;
    HudUiElement::Invalidate();
}

/**
 * Reimplements 0x40bea0: HudUiPanel::GetFont.
 * Purpose: return the recovered HUD value exposed by HudUiPanel::GetFont.
 */
HGDIOBJ HudUiPanel::GetFont() {
    return hFont;
}

/**
 * Reimplements 0x40beb0: HudUiPanel::SetFontHandle.
 * Purpose: apply the recovered HUD state change handled by HudUiPanel::SetFontHandle.
 */
void HudUiPanel::SetFontHandle(
    HGDIOBJ fontHandle
) {
    hFont = fontHandle;
}

/**
 * Reimplements 0x40bec0: HudUiPanel::EnableWordWrapWithRect.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::EnableWordWrapWithRect.
 */
void HudUiPanel::EnableWordWrapWithRect(
    const HudUiRect *rect
) {
    wordWrapEnabled = 1;
    wrapRect = *rect;
}

/**
 * Reimplements 0x40bef0: HudUiPanel::DestructorThunk.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: tail-call the panel destructor through the callback-compatible
 * panel method slot.
 */
void HudUiPanel::DestructorThunk() {
    this->~HudUiPanel();
}

/**
 * Reimplements 0x40bf00: HudCmdBindingEntry::~HudCmdBindingEntry.
 * Binary Ninja shows six ordinary destructor calls from the five concrete
 * bind-button destructors and the addressable base destructor.
 * Purpose: release the entry-owned display string before scalar delete.
 */
inline HudCmdBindingEntry::~HudCmdBindingEntry() {
    if (displayText != 0) {
        free(displayText);
        displayText = 0;
    }
}

/**
 * Reimplements 0x40bf20: HudCmdBindingEntryDelete::operator().
 * Purpose: delete one binding entry and replace its vector slot with null.
 */
inline HudCmdBindingEntry *HudCmdBindingEntryDelete::operator()(
    HudCmdBindingEntry *entry
) const {
    delete entry;
    return 0;
}

/**
 * Reimplements 0x40bf80: HudCmdBindButtonBase::AddBindingEntry.
 * Binary Ninja shows the HudCmdBindButton.cpp method allocating a
 * HudCmdBindingEntry, duplicating the display text, assigning the command id,
 * and appending it to the binding vector with growth when capacity is full.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindButtonBase::AddBindingEntry.
 */
int HudCmdBindButtonBase::AddBindingEntry(
    const char *displayText,
    int commandId
) {
    const int oldCount = (int)bindingVec.size();
    HudCmdBindingEntry *const entry = new HudCmdBindingEntry(
        displayText,
        commandId
    );
    bindingVec.push_back(entry);
    return oldCount;
}

/**
 * Reimplements 0x40c1d0: HudCmdBindButtonBase::ClearBindingEntries.
 * Provider boundary 0x40be60: canonical VC5 std::copy specialization
 * selected by vector::clear().
 * Reimplements 0x40be60: through that canonical provider instantiation.
 * Purpose: delete and null every owned entry, then clear the pointer range.
 */
inline void HudCmdBindButtonBase::ClearBindingEntries() {
    std::transform(
        bindingVec.begin(),
        bindingVec.end(),
        bindingVec.begin(),
        HudCmdBindingEntryDelete()
    );
    bindingVec.clear();
}

/**
 * Reimplements 0x40c280: HudCmdBindButtonBase::~HudCmdBindButtonBase.
 * Purpose: run the optimizer-visible entry cleanup before ordinary vector,
 * panel, and widget-base lifetime teardown.
 */
inline HudCmdBindButtonBase::~HudCmdBindButtonBase() {
    ClearBindingEntries();
}
