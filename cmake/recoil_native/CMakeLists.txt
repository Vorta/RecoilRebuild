add_library(recoil_native STATIC
    ${PROJECT_SOURCE_DIR}/src/Battlesport/about.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/ai_net.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/Briefing.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/hud.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/map.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/mission.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/pickup.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/player.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/RecoilApp.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/turret.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/util.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/version.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/weapon.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/WinSock.cpp
    ${PROJECT_SOURCE_DIR}/src/Battlesport/WOL.cpp
    ${PROJECT_SOURCE_DIR}/src/CZGameFrame/CZGameFrame.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Animate.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Camera.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Class.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_di.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_util.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_world.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_zbd.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Display.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Light.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/List.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Object3d.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Seq.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Sound.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Switch.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Window.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zDEClient/zdec_crater.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zDEClient/zdec_init.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zDEClient/zdec_qsand.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_anim_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_anim_run.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_anim_save.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zError/zerr_old.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zFMV/fmv_main.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zFMV/fmv_script.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zFMV/fmv_stream.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zGame/zgame_opt.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zGeometry/zgeo_convexify.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zGeometry/zgeo_model.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zGeometry/zgeo_weiler.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zImage/zimg_fonts.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zImage/zimg_texture.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInput/zin_init.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInput/zin_joystick.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInput/zin_kbd.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInput/zin_mouse.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInput/zInput.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInterp/zinterp_parse.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zMath/zmth_main.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_const.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_light.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_matl.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zNetwork/znet_dplay.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zReader/zreader.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zRender/zrndr_draw.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_3d.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_cd.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_create.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_error.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_fade.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_grp.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_init.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_parm.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd_play.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSound/zsnd.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/Time/Time.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSys/zsys_cpu.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zSys/zsys.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zUI/zui.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zUI/zui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zUtil/zutl_zar.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zUtil/zutl_zbd.cpp
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_buff.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_dd.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_ddd3d.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_main.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zWeapon/zwep_init.c
    ${PROJECT_SOURCE_DIR}/src/WinMain.cpp
    recoil_build_anchor.cpp
)

set_source_files_properties(
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Animate.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Camera.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Class.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_di.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_util.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_world.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/cls_zbd.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Display.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Light.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/List.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Object3d.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Seq.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Sound.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Switch.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zClass/Window.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_anim_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_anim_run.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_anim_save.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zEffect/zeff_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zError/zerr_old.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zGame/zgame_opt.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zMath/zmth_main.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_const.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_light.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zModel/gmod_matl.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zRender/zrndr_draw.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_buff.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_dd.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_ddd3d.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_init.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zVideo/zvid_main.c
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zWeapon/zwep_init.c
    PROPERTIES LANGUAGE CXX
)

if(MSVC)
    set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/src/Battlesport/hud.cpp
        ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zUI/zui.cpp
        ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zUI/zui_widgets.cpp
        PROPERTIES COMPILE_OPTIONS /Zc:preprocessor
    )
    set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/src/GameZRecoil/zInput/zInput.cpp
        PROPERTIES COMPILE_OPTIONS /Zc:throwingNew-
    )
endif()

target_link_libraries(recoil_native PUBLIC
    recoil_compiler_options
    recoil_aureal_a3d20_legacy
    recoil_directx_legacy
    recoil_mfc42_legacy
    winmm
    vfw32
    dplayx
    ddraw
    dinput
    dsound
    dxguid
    comdlg32
    ole32
    ws2_32
)

target_include_directories(recoil_native PUBLIC
    ${PROJECT_SOURCE_DIR}/tools/_recoil/compat/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/Battlesport
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/include
)

# Modern smoke builds use C++17 for test/toolchain compatibility only. Production
# source under src/ still follows the VC-era rules enforced by the source guards.
target_compile_features(recoil_native PUBLIC cxx_std_17)

# This target covers every current Recoil.exe production translation unit under
# src/.  The companion Messages DLL remains a separate target and source closure.
