add_library(recoil_native STATIC
    ../Battlesport/about.cpp
    ../Battlesport/ai_net.cpp
    ../Battlesport/Briefing.cpp
    ../Battlesport/hud.cpp
    ../Battlesport/map.cpp
    ../Battlesport/mission.cpp
    ../Battlesport/pickup.cpp
    ../Battlesport/player.cpp
    ../Battlesport/RecoilApp.cpp
    ../Battlesport/turret.cpp
    ../Battlesport/util.cpp
    ../Battlesport/version.cpp
    ../Battlesport/weapon.cpp
    ../Battlesport/WinSock.cpp
    ../Battlesport/WOL.cpp
    ../CZGameFrame/CZGameFrame.cpp
    ../GameZRecoil/zClass/Animate.c
    ../GameZRecoil/zClass/Camera.c
    ../GameZRecoil/zClass/Class.c
    ../GameZRecoil/zClass/cls_di.c
    ../GameZRecoil/zClass/cls_util.c
    ../GameZRecoil/zClass/cls_world.c
    ../GameZRecoil/zClass/cls_zbd.c
    ../GameZRecoil/zClass/Display.c
    ../GameZRecoil/zClass/Light.c
    ../GameZRecoil/zClass/List.c
    ../GameZRecoil/zClass/Object3d.c
    ../GameZRecoil/zClass/Seq.c
    ../GameZRecoil/zClass/Sound.c
    ../GameZRecoil/zClass/Switch.c
    ../GameZRecoil/zClass/Window.c
    ../GameZRecoil/zDEClient/zdec_init.cpp
    ../GameZRecoil/zEffect/zeff_anim_run.c
    ../GameZRecoil/zError/zerr_old.c
    ../GameZRecoil/zFMV/fmv_script.cpp
    ../GameZRecoil/zGame/zgame_opt.c
    ../GameZRecoil/zGeometry/zgeo_convexify.cpp
    ../GameZRecoil/zGeometry/zgeo_model.cpp
    ../GameZRecoil/zGeometry/zgeo_weiler.cpp
    ../GameZRecoil/zImage/zimg_texture.cpp
    ../GameZRecoil/zInput/zInput.cpp
    ../GameZRecoil/zInterp/zinterp_parse.cpp
    ../GameZRecoil/zMath/zmth_main.c
    ../GameZRecoil/zModel/gmod_const.c
    ../GameZRecoil/zModel/gmod_init.c
    ../GameZRecoil/zModel/gmod_light.c
    ../GameZRecoil/zModel/gmod_matl.c
    ../GameZRecoil/zNetwork/znet_dplay.cpp
    ../GameZRecoil/zReader/zreader.cpp
    ../GameZRecoil/zRender/zrndr_draw.c
    ../GameZRecoil/zSound/zsnd_3d.cpp
    ../GameZRecoil/zSound/zsnd_cd.cpp
    ../GameZRecoil/zSound/zsnd_create.cpp
    ../GameZRecoil/zSound/zsnd_error.cpp
    ../GameZRecoil/zSound/zsnd_fade.cpp
    ../GameZRecoil/zSound/zsnd_grp.cpp
    ../GameZRecoil/zSound/zsnd_init.cpp
    ../GameZRecoil/zSound/zsnd_parm.cpp
    ../GameZRecoil/zSound/zsnd_play.cpp
    ../GameZRecoil/zSound/zsnd.cpp
    ../GameZRecoil/zSys/zsys_cpu.cpp
    ../GameZRecoil/zSys/zsys.cpp
    ../GameZRecoil/zUI/zui.cpp
    ../GameZRecoil/zUtil/zutl_zar.cpp
    ../GameZRecoil/zUtil/zutl_zbd.cpp
    ../GameZRecoil/zVideo/zvid_buff.c
    ../GameZRecoil/zVideo/zvid_dd.c
    ../GameZRecoil/zVideo/zvid_ddd3d.c
    ../GameZRecoil/zVideo/zvid_init.c
    ../GameZRecoil/zVideo/zvid_main.c
    ../GameZRecoil/zWeapon/zwep_init.c
    ../WinMain.cpp
    recoil_build_anchor.cpp
)

set_source_files_properties(
    ../GameZRecoil/zClass/Animate.c
    ../GameZRecoil/zClass/Camera.c
    ../GameZRecoil/zClass/Class.c
    ../GameZRecoil/zClass/cls_di.c
    ../GameZRecoil/zClass/cls_util.c
    ../GameZRecoil/zClass/cls_world.c
    ../GameZRecoil/zClass/cls_zbd.c
    ../GameZRecoil/zClass/Display.c
    ../GameZRecoil/zClass/Light.c
    ../GameZRecoil/zClass/List.c
    ../GameZRecoil/zClass/Object3d.c
    ../GameZRecoil/zClass/Seq.c
    ../GameZRecoil/zClass/Sound.c
    ../GameZRecoil/zClass/Switch.c
    ../GameZRecoil/zClass/Window.c
    ../GameZRecoil/zEffect/zeff_anim_run.c
    ../GameZRecoil/zError/zerr_old.c
    ../GameZRecoil/zGame/zgame_opt.c
    ../GameZRecoil/zMath/zmth_main.c
    ../GameZRecoil/zModel/gmod_const.c
    ../GameZRecoil/zModel/gmod_init.c
    ../GameZRecoil/zModel/gmod_light.c
    ../GameZRecoil/zModel/gmod_matl.c
    ../GameZRecoil/zRender/zrndr_draw.c
    ../GameZRecoil/zVideo/zvid_buff.c
    ../GameZRecoil/zVideo/zvid_dd.c
    ../GameZRecoil/zVideo/zvid_ddd3d.c
    ../GameZRecoil/zVideo/zvid_init.c
    ../GameZRecoil/zVideo/zvid_main.c
    ../GameZRecoil/zWeapon/zwep_init.c
    PROPERTIES LANGUAGE CXX
)

if(MSVC)
    set_source_files_properties(
        ../Battlesport/hud.cpp
        ../GameZRecoil/zUI/zui.cpp
        PROPERTIES COMPILE_OPTIONS /Zc:preprocessor
    )
    set_source_files_properties(
        ../GameZRecoil/zInput/zInput.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/Battlesport
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/include
)

# Modern smoke builds use C++17 for test/toolchain compatibility only. Production
# source under src/ still follows the VC-era rules enforced by the source guards.
target_compile_features(recoil_native PUBLIC cxx_std_17)

# Add reimplemented source files here only after their direct dependencies compile.
# Keep partially reconstructed or dependency-blocked files out of the default target.
