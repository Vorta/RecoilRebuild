// Battlesport compilation unit between player_move.cpp and player_state.cpp,
// inferred from the retail object boundary [0x429f10, 0x42a9f0): the zInput
// binding-group code starts with its own CRT initializer (0x429f10) and ends
// with its vector helper instantiation (0x42a9d0); it reads no pooled
// constant. Original filename unresolved; player_remap.cpp is a provisional
// name (2026-10-02).

#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"

#include <algorithm>
#include <atlbase.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern char g_HudUiCounterText_PlayerLabel[];

namespace zInput {
/**
 * Purpose: Delete an owned binding group and return null for its vector slot.
 */
struct CBindGroupDelete {
    CZInputBindGroupInfo* operator()(CZInputBindGroupInfo* group) const
    {
        delete group;
        return 0;
    }
};

/**
 * @recoil-anchor recoil:anchor:battlesport-player-input-bind-group-list-clear
 * @recoil-artifact defines .text recoil:function:0x429f80: zInput::BindGroupListClear.
 * @recoil-match byte
 *
 * Purpose: Delete and null all owned binding groups, then empty the vector.
 */
void __cdecl BindGroupListClear()
{
    std::transform(
        g_zInput_BindGroupInfoList.begin(),
        g_zInput_BindGroupInfoList.end(),
        g_zInput_BindGroupInfoList.begin(),
        CBindGroupDelete()
    );
    g_zInput_BindGroupInfoList.clear();
}
} // namespace zInput

/**
 * @recoil-anchor recoil:anchor:battlesport-player-input-bind-group-destructor
 * @recoil-artifact defines .text recoil:function:0x42a000: CZInputBindGroupInfo destructor.
 * @recoil-match byte
 *
 * Purpose: Release the title and native command-id vector storage.
 */
CZInputBindGroupInfo::~CZInputBindGroupInfo()
{
    title.Empty();
}
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindgrouplistaddgroup
 * @recoil-artifact defines .text recoil:function:0x42a070: zInput::BindGroupListAddGroup.
 * @recoil-match byte
 *
 * Purpose: Allocates a bind-group record and appends it to the global vector.
 */
int __fastcall BindGroupListAddGroup(const char* title)
{
    const int groupIndex = (int)g_zInput_BindGroupInfoList.size();
    CZInputBindGroupInfo* group = new CZInputBindGroupInfo(title);
    g_zInput_BindGroupInfoList.push_back(group);
    return groupIndex;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindgrouplistaddcommandtogroup
 * @recoil-artifact defines .text recoil:function:0x42a2c0: zInput::BindGroupListAddCommandToGroup.
 * @recoil-match byte
 *
 * Purpose: Appends a command id to the selected bind group's command-id vector.
 */
void __fastcall BindGroupListAddCommandToGroup(int groupIndex, int commandId)
{
    std::vector<int>& commandIds = g_zInput_BindGroupInfoList[groupIndex]->commandIds;
    {
        int savedCommandId = commandId;
        commandIds.push_back(savedCommandId);
    }
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindgrouplistgetcount
 * @recoil-artifact defines .text recoil:function:0x42a480: zInput::BindGroupListGetCount.
 * @recoil-match byte
 *
 * Purpose: Returns the number of active bind groups.
 */
int __cdecl BindGroupListGetCount()
{
    return (int)g_zInput_BindGroupInfoList.size();
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindgrouplistgetgrouptitle
 * @recoil-artifact defines .text recoil:function:0x42a4a0: zInput::BindGroupListGetGroupTitle.
 * @recoil-match byte
 *
 * Purpose: Returns the selected bind-group title buffer.
 */
char* __fastcall BindGroupListGetGroupTitle(int groupIndex)
{
    CZInputBindGroupInfo* const group = g_zInput_BindGroupInfoList[groupIndex];
    return (char*)(LPCTSTR)(group->title);
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindgrouplistgetgroupcommandcount
 * @recoil-artifact defines .text recoil:function:0x42a4b0: zInput::BindGroupListGetGroupCommandCount.
 * @recoil-match byte
 *
 * Purpose: Returns the number of command IDs in the selected bind group.
 */
int __fastcall BindGroupListGetGroupCommandCount(int groupIndex)
{
    CZInputBindGroupInfo* const group = g_zInput_BindGroupInfoList[groupIndex];
    return (int)group->commandIds.size();
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindgrouplistgetgroupcommandid
 * @recoil-artifact defines .text recoil:function:0x42a4d0: zInput::BindGroupListGetGroupCommandId.
 * @recoil-match byte
 *
 * Purpose: Returns one command ID from the selected bind group.
 */
int __fastcall BindGroupListGetGroupCommandId(int groupIndex, int commandIndex)
{
    CZInputBindGroupInfo* const group = g_zInput_BindGroupInfoList[groupIndex];
    return group->commandIds[commandIndex];
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-remap.z-input-bind-map-get-command-label
 * @recoil-artifact defines .text recoil:function:0x42a4e0: zInput::BindMapGetCommandLabel.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_bindmap.cpp.
 * Binary Ninja indexes g_zInput_CommandLocIdTable by command id and tail-calls
 * zLoc::GetMessageString for the command's localized label.
 * Purpose: Resolve a bind-map command id to its localized display label.
 */
char* __fastcall BindMapGetCommandLabel(int commandId)
{
    return zLoc::GetMessageString(g_zInput_CommandLocIdTable[commandId]);
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-remap.z-input-bind-map-get-command-hint
 * @recoil-artifact defines .text recoil:function:0x42a4f0: zInput::BindMapGetCommandHint.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_bindmap.cpp.
 * Binary Ninja indexes g_zInput_CommandLocIdTable by command id, increments
 * the recovered localization id, and tail-calls zLoc::GetMessageString for
 * the command hint.
 * Purpose: Resolve a bind-map command id to its localized hint text.
 */
char* __fastcall BindMapGetCommandHint(int commandId)
{
    return zLoc::GetMessageString(g_zInput_CommandLocIdTable[commandId] + 1);
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-remap.z-input-bind-map-add-default-binding
 * @recoil-artifact defines .text recoil:function:0x42a500: zInput::BindMapAddDefaultBinding.
 * @recoil-match byte
 *
 * Purpose: Add one localized default command binding to the active bind map and bind-group list.
 */
void __fastcall BindMapAddDefaultBinding(
    int commandId,
    int messageId,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
)
{
    const int boundCommandId = BindMapCurrentSetBindingRecord(
        commandId,
        zLoc::GetMessageString(messageId),
        primaryKey,
        secondaryKey,
        joystickSlot,
        mouseSlot
    );
    BindGroupListAddCommandToGroup(g_zInput_CurrentBindGroupIndex, boundCommandId);
    g_zInput_CommandLocIdTable[commandId] = messageId;
}
} // namespace zInput
namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zinput-bindmapinitdefaultbindings
 * @recoil-artifact defines .text recoil:function:0x42a550: zInput::BindMapInitDefaultBindings.
 * @recoil-match byte
 *
 * Purpose: Clear the bind-group list and seed the retail default command bindings.
 */
int __fastcall BindMapInitDefaultBindings()
{
    BindGroupListClear();
    g_zInput_CurrentBindGroupIndex = BindGroupListAddGroup(zLoc::GetMessageString(0x750));
    BindMapAddDefaultBinding(0x04, 0x806, 0x0c8, 0, 0, 0);
    BindMapAddDefaultBinding(0x01, 0x800, 0x0d0, 0, 0, 0);
    BindMapAddDefaultBinding(0x02, 0x802, 0x0cb, 0, 0, 0);
    BindMapAddDefaultBinding(0x03, 0x804, 0x0cd, 0, 0, 0);
    BindMapAddDefaultBinding(0x2b, 0x8c6, 0x01f, 0, 0, 0);
    BindMapAddDefaultBinding(0x25, 0x874, 0x03b, 0, 0, 0);
    BindMapAddDefaultBinding(0x26, 0x876, 0x03c, 0, 0, 0);
    BindMapAddDefaultBinding(0x27, 0x878, 0x03d, 0, 0, 0);
    BindMapAddDefaultBinding(0x28, 0x87a, 0x03e, 0, 0, 0);
    BindMapAddDefaultBinding(0x05, 0x80e, 0x01e, 0, 0, 0);
    BindMapAddDefaultBinding(0x06, 0x810, 0x02c, 0, 0, 0);
    BindMapAddDefaultBinding(0x07, 0x82a, 0x02e, 0, 6, 0);
    BindMapAddDefaultBinding(0x08, 0x82c, 0x02b, 0, 5, 0);
    BindMapAddDefaultBinding(0x09, 0x872, 0x030, 0, 0, 0);
    BindMapAddDefaultBinding(0x0a, 0x8c2, 0x230, 0, 0, 0);
    g_zInput_CurrentBindGroupIndex = BindGroupListAddGroup(zLoc::GetMessageString(0x751));
    BindMapAddDefaultBinding(0x0b, 0x88c, 0, 0, 1, 1);
    BindMapAddDefaultBinding(0x0c, 0x88e, 0, 0, 2, 2);
    BindMapAddDefaultBinding(0x0d, 0x8b8, 0x039, 0, 3, 0);
    BindMapAddDefaultBinding(0x0f, 0x812, 0x002, 0x04f, 0, 0);
    BindMapAddDefaultBinding(0x10, 0x814, 0x003, 0x050, 0, 0);
    BindMapAddDefaultBinding(0x11, 0x816, 0x004, 0x051, 0, 0);
    BindMapAddDefaultBinding(0x12, 0x818, 0x005, 0x04b, 0, 0);
    BindMapAddDefaultBinding(0x13, 0x81a, 0x006, 0x04c, 0, 0);
    BindMapAddDefaultBinding(0x14, 0x81c, 0x007, 0x04d, 0, 0);
    BindMapAddDefaultBinding(0x15, 0x81e, 0x008, 0x047, 0, 0);
    BindMapAddDefaultBinding(0x16, 0x820, 0x009, 0x048, 0, 0);
    BindMapAddDefaultBinding(0x17, 0x822, 0x00a, 0x049, 0, 0);
    g_zInput_CurrentBindGroupIndex = BindGroupListAddGroup(zLoc::GetMessageString(0x752));
    BindMapAddDefaultBinding(0x1e, 0x84e, 0x02f, 0, 0, 0);
    BindMapAddDefaultBinding(0x20, 0x888, 0x03f, 0, 0, 0);
    BindMapAddDefaultBinding(0x21, 0x8a6, 0x040, 0, 0, 0);
    BindMapAddDefaultBinding(0x22, 0x8a8, 0x041, 0, 0, 0);
    g_zInput_CurrentBindGroupIndex = BindGroupListAddGroup(zLoc::GetMessageString(0x753));
    BindMapAddDefaultBinding(0x19, 0x8a4, 0x013, 0, 0, 0);
    BindMapAddDefaultBinding(0x18, 0x826, 0x018, 0, 0, 0);
    BindMapAddDefaultBinding(0x1a, 0x8c4, 0x011, 0, 0, 0);
    g_zInput_CurrentBindGroupIndex = BindGroupListAddGroup(zLoc::GetMessageString(0x754));
    BindMapAddDefaultBinding(0x2d, 0x8b6, 0x042, 0, 0, 0);
    BindMapAddDefaultBinding(0x2c, 0x8b4, 0x043, 0, 0, 0);
    BindMapAddDefaultBinding(0x2a, 0x8bc, 0x014, 0, 0, 0);
    BindMapAddDefaultBinding(0x1b, 0x864, 0x032, 0, 0, 0);
    BindMapAddDefaultBinding(0x1c, 0x866, 0x034, 0, 0, 0);
    BindMapAddDefaultBinding(0x1d, 0x868, 0x033, 0, 0, 0);
    BindMapAddDefaultBinding(0x23, 0x88a, 0x22d, 0, 0, 0);

    BindMapCurrentSetBindingRecord(0x24, zLoc::GetMessageString(0x83c), 0x418, 0, 0, 0);
    BindMapCurrentSetBindingRecord(0x1f, zLoc::GetMessageString(0x850), 0x22, 0, 0, 0);
    return 1;
}
} // namespace zInput
