#include "recoil/recoil_types.h"

extern "C" {
/**
 * Reimplements data 0x4e2fbc: g_zApp_DefaultStdoutLogName.
 *
 * Purpose: names the fallback stdout log file appended under the temp path.
 */
char g_zApp_DefaultStdoutLogName[0x0a] = "gamez.out";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_DefaultStdoutLogName) == 0x0a);
/**
 * Reimplements data 0x4e2fc8: g_zApp_StdoutLogSuffix.
 *
 * Purpose: supplies the stdout log suffix appended to the executable path.
 */
char g_zApp_StdoutLogSuffix[0x05] = ".out";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_StdoutLogSuffix) == 0x05);
/**
 * Reimplements data 0x4e2fd0: g_zApp_LogFileStartBanner.
 *
 * Purpose: writes the startup banner to each redirected standard log stream.
 */
char g_zApp_LogFileStartBanner[0x12] = "File started\n---\n";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_LogFileStartBanner) == 0x12);
/**
 * Reimplements data 0x4e2fe4: g_zApp_DefaultStderrLogName.
 *
 * Purpose: names the fallback stderr log file appended under the temp path.
 */
char g_zApp_DefaultStderrLogName[0x0a] = "gamez.err";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_DefaultStderrLogName) == 0x0a);
/**
 * Reimplements data 0x4e2ff0: g_zApp_StderrLogSuffix.
 *
 * Purpose: supplies the stderr log suffix appended to the executable path.
 */
char g_zApp_StderrLogSuffix[0x05] = ".err";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_StderrLogSuffix) == 0x05);
}
