add_library(recoil_native STATIC
    ../Battlesport/RecoilApp.cpp
    ../Battlesport/RecoilApp_PlayState_TickAndRenderFrame.cpp
    ../Battlesport/Recoil.cpp
    ../Battlesport/RecoilVersion.cpp
    ../Battlesport/Briefing.cpp
    ../Battlesport/ainet.cpp
    ../Battlesport/GameNet.cpp
    ../Battlesport/hud.cpp
    ../Battlesport/HudUiMainMenuDialog.cpp
    ../Battlesport/RecoilStateCredits.cpp
    ../Battlesport/HudUiNetExitPanel.cpp
    ../Battlesport/HudSensorTracker.cpp
    ../Battlesport/player.cpp
    ../Battlesport/pickup.cpp
    ../Battlesport/zUtil/zutil.cpp
    ../Battlesport/zModel/gdi.c
    ../Battlesport/zModel/zModel_Display.cpp
    ../Battlesport/CZGameFrame.cpp
    ../Battlesport/CZRecoilFrame.cpp
    ../GameZRecoil/RecoilApp/RecoilStateBase.cpp
    ../GameZRecoil/RecoilApp/RecoilStateDialogHost.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnTryBecomeCurrent.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnResume.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnDeactivate.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_ClearPausedAudioSnapshot.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_QueueEnter.cpp
    ../GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_SetDeferredVideoModeIndex.cpp
    ../GameZRecoil/zSound/zsnd_cd.cpp
    ../GameZRecoil/zSound/zsnd_create.cpp
    ../GameZRecoil/zSound/zsnd_group.cpp
    ../GameZRecoil/zSound/zsnd_init.cpp
    ../GameZRecoil/zSound/zsnd_parm.cpp
    ../GameZRecoil/zSound/zsnd_play.cpp
    ../GameZRecoil/zSound/zsnd_sample_set.cpp
    ../GameZRecoil/zSound/zsnd_system.cpp
    ../GameZRecoil/Time/Time.cpp
    ../GameZRecoil/zEffect/zEffect.cpp
    ../GameZRecoil/zFMV/fmv_script.cpp
    ../GameZRecoil/zGame/zGame.cpp
    ../GameZRecoil/zGeometry/zClipAlt.cpp
    ../GameZRecoil/zGeometry/zClipRect.cpp
    ../GameZRecoil/zGeometry/zgeo_convexify.cpp
    ../GameZRecoil/zGeometry/zgeo_model.cpp
    ../GameZRecoil/zGeometry/zgeo_weiler.cpp
    ../GameZRecoil/zHud/zhud_ui.cpp
    ../GameZRecoil/zImage/zimg_texture.cpp
    ../GameZRecoil/zLoc/zLoc.cpp
    ../GameZRecoil/zMath/zMath.cpp
    ../GameZRecoil/zDEClient/zdec_init.cpp
    ../GameZRecoil/zReader/zreader_load.cpp
    ../GameZRecoil/zReader/zreader_lookup.cpp
    ../GameZRecoil/zModel/zModel.cpp
    ../GameZRecoil/zModel/gmod_const.c
    ../GameZRecoil/zModel/gmod_light.c
    ../GameZRecoil/zModel/gmod_matl.c
    ../GameZRecoil/zWeapon/OptCatalog.c
    ../GameZRecoil/zWeapon/zWeapon.cpp
    ../GameZRecoil/zClass/Class.c
    ../GameZRecoil/zClass/cls_stubs.cpp
    ../GameZRecoil/zClass/Animate.c
    ../GameZRecoil/zClass/Camera.c
    ../GameZRecoil/zClass/cls_di.c
    ../GameZRecoil/zClass/cls_util.c
    ../GameZRecoil/zClass/cls_world.c
    ../GameZRecoil/zClass/cls_zbd.c
    ../GameZRecoil/zClass/Display.c
    ../GameZRecoil/zClass/Light.c
    ../GameZRecoil/zClass/List.c
    ../GameZRecoil/zClass/List_RenderActiveCameras.cpp
    ../GameZRecoil/zClass/Lod.c
    ../GameZRecoil/zClass/Object3d.c
    ../GameZRecoil/zClass/Seq.c
    ../GameZRecoil/zClass/Sound.c
    ../GameZRecoil/zClass/Switch.c
    ../GameZRecoil/zClass/Window.c
    ../GameZRecoil/zInput/zInput.cpp
    ../GameZRecoil/zNetwork/znet_dplay.cpp
    ../GameZRecoil/zError/zerr_old.c
    ../GameZRecoil/zRndr/zRndr.cpp
    ../GameZRecoil/zTurret/zTurret.cpp
    ../GameZRecoil/zUtil/zZbd.cpp
    ../GameZRecoil/zUtil/zSaveGame.cpp
    ../GameZRecoil/zVideo/zVideo.cpp
    ../GameZRecoil/zCom/zCom.cpp
    ../GameZRecoil/zSys/zSys.cpp
    recoil_build_anchor.cpp
)

set_source_files_properties(
    ../Battlesport/zModel/gdi.c
    ../GameZRecoil/zModel/gmod_const.c
    ../GameZRecoil/zModel/gmod_light.c
    ../GameZRecoil/zModel/gmod_matl.c
    ../GameZRecoil/zWeapon/OptCatalog.c
    ../GameZRecoil/zError/zerr_old.c
    ../GameZRecoil/zClass/Animate.c
    ../GameZRecoil/zClass/Class.c
    ../GameZRecoil/zClass/Camera.c
    ../GameZRecoil/zClass/cls_di.c
    ../GameZRecoil/zClass/cls_util.c
    ../GameZRecoil/zClass/cls_world.c
    ../GameZRecoil/zClass/cls_zbd.c
    ../GameZRecoil/zClass/Display.c
    ../GameZRecoil/zClass/Light.c
    ../GameZRecoil/zClass/List.c
    ../GameZRecoil/zClass/Lod.c
    ../GameZRecoil/zClass/Object3d.c
    ../GameZRecoil/zClass/Seq.c
    ../GameZRecoil/zClass/Sound.c
    ../GameZRecoil/zClass/Switch.c
    ../GameZRecoil/zClass/Window.c
    PROPERTIES LANGUAGE CXX
)

if(MSVC)
    set_source_files_properties(
        ../Battlesport/HudUiMainMenuDialog.cpp
        ../GameZRecoil/zHud/zhud_ui.cpp
        PROPERTIES COMPILE_OPTIONS /Zc:preprocessor
    )
    set_source_files_properties(
        ../GameZRecoil/zInput/zInput.cpp
        PROPERTIES COMPILE_OPTIONS /Zc:throwingNew-
    )
endif()

target_link_libraries(recoil_native PUBLIC
    recoil_compiler_options
    recoil_directx_legacy
    recoil_mfc42_legacy
    winmm
    vfw32
    ddraw
    dinput
    dsound
    dxguid
    comdlg32
    ole32
)

target_include_directories(recoil_native PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/Battlesport
    ${PROJECT_SOURCE_DIR}/src/GameZRecoil/include
)

target_compile_features(recoil_native PUBLIC cxx_std_17)

# Add reimplemented source files here only after their direct dependencies compile.
# Keep partially reconstructed or dependency-blocked files out of the default target.
