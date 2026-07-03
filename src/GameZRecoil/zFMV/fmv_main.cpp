/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */

/**
 * Reimplements 0x462330: zFMV_Playback::Constructor.
 * Purpose: initialize an MCI playback object with a duplicated media path and window handle.
 */
zFMV_Playback::zFMV_Playback(
    const char *mediaPath,
    HWND hwnd
) {
    mediaPathDup = DuplicateCString(mediaPath);
    notifyHwnd = hwnd;
    mciPutFlags = 0;
}

/**
 * Reimplements 0x462360: zFMV_Playback::Destructor.
 * Purpose: release the duplicated MCI media path.
 */
void zFMV_Playback::Destructor() {
    free(mediaPathDup);
}

/**
 * Reimplements 0x462370: zFMV_Playback::OpenAndPlay.
 * Purpose: open an MCI MPEG device, configure its window/rect/time format, and start playback.
 */
void zFMV_Playback::OpenAndPlay(
    unsigned int startMs,
    int endMs,
    int notifyFlag
) {
    zVideo_dd::FlipToGDIIfAttached();

    // Retail writes only the MCI fields consumed by each command.
    zFMV_MciPlayParams playParams;
    zFMV_MciSetParams setParams;
    zFMV_MciWindowParams windowParams;
    zFMV_MciRectParams rectParams;
    MCI_DGV_OPEN_PARMSA openParams;

    openParams.lpstrDeviceType = (LPSTR)(g_zFMV_MpegVideoString);
    openParams.lpstrElementName = mediaPathDup;
    DWORD mciError = mciSendCommandA(
        0,
        0x803,
        0x2202,
        (DWORD_PTR)(&openParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
    }

    mciDeviceId = (unsigned short)(openParams.wDeviceID);

    windowParams.hwnd = notifyHwnd;
    mciError = mciSendCommandA(
        mciDeviceId,
        0x841,
        0x10002,
        (DWORD_PTR)(&windowParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
        return;
    }

    if ((mciPutFlags & 0x40000) != 0) {
        rectParams.left = destinationRect.left;
        rectParams.width = destinationRect.right - destinationRect.left;
        rectParams.top = destinationRect.top;
        rectParams.height = destinationRect.bottom - destinationRect.top;
        mciError = mciSendCommandA(
            mciDeviceId,
            0x842,
            0x50002,
            (DWORD_PTR)(&rectParams)
        );
        if (mciError != 0) {
            ReportMciError(mciError);
            return;
        }
    }

    if ((mciPutFlags & 0x20000) != 0) {
        rectParams.left = sourceRect.left;
        rectParams.width = sourceRect.right - sourceRect.left;
        rectParams.top = sourceRect.top;
        rectParams.height = sourceRect.bottom - sourceRect.top;
        mciError = mciSendCommandA(
            mciDeviceId,
            0x842,
            0x30002,
            (DWORD_PTR)(&rectParams)
        );
        if (mciError != 0) {
            ReportMciError(mciError);
            return;
        }
    }

    setParams.timeFormat = 0x1b;
    setParams.audio = (DWORD)((unsigned int)(notifyHwnd));
    mciError = mciSendCommandA(
        mciDeviceId,
        0x811,
        0x302,
        (DWORD_PTR)(&setParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
        return;
    }

    DWORD playFlags = 0x6;
    playParams.callback = (DWORD_PTR)(notifyHwnd);
    playParams.from = startMs;
    if (endMs >= 0) {
        playParams.to = (DWORD)(endMs);
        playFlags = 0xe;
    }
    if (notifyFlag == 1) {
        playFlags |= 0x10000;
    }

    mciError = mciSendCommandA(
        mciDeviceId,
        0x806,
        playFlags,
        (DWORD_PTR)(&playParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
    }
}

/**
 * Reimplements 0x4624f0: zFMV_Playback::StopAndClose.
 * Purpose: stop and close the active MCI device, reporting any failure.
 */
void zFMV_Playback::StopAndClose() {
    DWORD mciError = mciSendCommandA(
        mciDeviceId,
        0x808,
        0x2,
        0
    );
    if (mciError == 0) {
        MCI_GENERIC_PARMS closeParams;
        mciError = mciSendCommandA(
            mciDeviceId,
            0x804,
            0x2,
            (DWORD_PTR)(&closeParams)
        );
    }

    if (mciError != 0) {
        ReportMciError(mciError);
    }
}

/**
 * Reimplements 0x462540: zFMV_Playback::SetDestRect.
 * Purpose: copy the destination rectangle and mark it for the next MCI put command.
 */
int zFMV_Playback::SetDestRect(
    const zFMV_Rect *rect
) {
    destinationRect = *rect;
    const int result = mciPutFlags | 0x40000;
    mciPutFlags = result;
    return result;
}

/**
 * Reimplements 0x462570: zFMV_Playback::ReportMciError.
 * Purpose: translate an MCI error code and report it through the old zError path.
 */
int zFMV_Playback::ReportMciError(
    unsigned int mciError
) {
    char errorText[0x80];
    if (mciGetErrorStringA(
        mciError,
        errorText,
        sizeof(errorText)
    ) == 0) {
        strcpy(
            errorText,
            g_zFMV_UnknownErrorIdMsg
        );
    }

    zError::ReportOld(
        0x200,
        g_zFMV_SourceFile_FmvMainCpp,
        0xc4,
        errorText
    );
    return 0;
}

