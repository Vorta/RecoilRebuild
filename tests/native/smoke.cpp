extern "C" int recoil_native_build_anchor(void);
extern "C" int recoil_legacy_directx_header_smoke(void);
extern "C" int recoil_mfc42_provider_smoke(void);
extern "C" int mfc_cstring_default_ctor_provider_smoke(void);
extern "C" int mfc_three_float_dialog_handlers_smoke(void);
extern "C" int zstr_contains_case_insensitive_smoke(void);
extern "C" int recoil_version_get_string_smoke(void);
extern "C" int ai_property_dialog_on_destroy_smoke(void);
extern "C" int ai_property_dialog_on_sel_change_smoke(void);
extern "C" int ai_property_dialog_update_property_labels_smoke(void);
extern "C" int directinput_create_import_provider_smoke(void);
extern "C" int zinput_init_fastpath_smoke(void);
extern "C" int zinput_mouse_update_acquire_state_smoke(void);
extern "C" int zinput_joystick_option_accessors_smoke(void);
extern "C" int zinput_shutdown_smoke(void);
extern "C" int zinput_force_feedback_effect_wrappers_smoke(void);
extern "C" int zinput_force_feedback_create_effect_smoke(void);
extern "C" int zinput_force_feedback_effect_set_smoke(void);
extern "C" int zinput_force_feedback_directional_runtime_smoke(void);
extern "C" int zinput_bindgroup_accessors_smoke(void);
extern "C" int zinput_bindgroup_static_lifetime_smoke(void);
extern "C" int zinput_global_state_static_lifetime_smoke(void);
extern "C" int zinput_keyboard_dik_ascii_smoke(void);
extern "C" int zinput_mouse_client_size_center_smoke(void);
extern "C" int zinput_mouse_apply_and_recenter_cursor_smoke(void);
extern "C" int zinput_mouse_coop_level_flags_smoke(void);
extern "C" int zinput_mouse_button_transition_state_smoke(void);
extern "C" int zinput_mouse_apply_accumulated_delta_smoke(void);
extern "C" int zinput_mouse_keyboard_small_accessors_smoke(void);
extern "C" int zinput_bindmap_name_tables_smoke(void);
extern "C" int zinput_bindmap_context_smoke(void);
extern "C" int zinput_bindmap_current_reset_all_bindings_smoke(void);
extern "C" int zinput_bindmap_dispatch_mouse_callbacks_smoke(void);
extern "C" int zinput_keyboard_clear_callbacks_smoke(void);
extern "C" int zinput_keyboard_mouse_addref_smoke(void);
extern "C" int zinput_keyboard_init_device_smoke(void);
extern "C" int zinput_mouse_init_device_smoke(void);
extern "C" int zinput_joystick_init_device_smoke(void);
extern "C" int zinput_joystick_acquire_device_smoke(void);
extern "C" int zinput_joystick_axis_property_smoke(void);
extern "C" int zinput_joystick_ref_and_enable_smoke(void);
extern "C" int zinput_joystick_poll_and_wait_smoke(void);
extern "C" int zinput_mouse_shutdown_device_smoke(void);
extern "C" int zinput_joystick_shutdown_device_smoke(void);
extern "C" int zinput_keyboard_shutdown_device_smoke(void);
extern "C" int zinput_mouse_poll_state_smoke(void);
extern "C" int zinput_suspend_flags_smoke(void);
extern "C" int zinput_on_app_deactivate_smoke(void);
extern "C" int zinput_joystick_reset_and_resume_smoke(void);
extern "C" int zinput_mouse_reset_and_resume_smoke(void);
extern "C" int zinput_directinput_report_error_smoke(void);
extern "C" int zinput_keyboard_reset_and_resume_smoke(void);
extern "C" int zinput_keyboard_reset_inputlost_smoke(void);
extern "C" int zinput_reset_all_transition_state_smoke(void);
extern "C" int zinput_on_app_activate_smoke(void);
extern "C" int zinput_keyboard_raw_callback_smoke(void);
extern "C" int zinput_keyboard_wait_for_key_press_smoke(void);
extern "C" int zinput_keyboard_poll_state_smoke(void);
extern "C" int zinput_poll_active_devices_smoke(void);
extern "C" int zopt_fullscreen_accessors_smoke(void);
extern "C" int zopt_section_accessor_smoke(void);
extern "C" int zopt_view_rect_target_side_effects_smoke(void);
extern "C" int zsnd_option_accessors_smoke(void);
extern "C" int zsnd_stream_request_stop_if_active_smoke(void);
extern "C" int zsnd_stream_mgr_ensure_init_smoke(void);
extern "C" int zsnd_stream_mgr_recycle_finished_request_smoke(void);
extern "C" int zsnd_stream_mgr_shutdown_lists_smoke(void);
extern "C" int zsnd_backend_shutdown_release_smoke(void);
extern "C" int zsnd_play_handle_stop_if_active_smoke(void);
extern "C" int zsnd_play_handle_try_disable_managed_smoke(void);
extern "C" int zsnd_play_handle_update3d_a3d_smoke(void);
extern "C" int zsnd_play_handle_update3d_directsound_smoke(void);
extern "C" int zsnd_update_listener_state_smoke(void);
extern "C" int zsnd_speed_of_sound_smoke(void);
extern "C" int zsnd_sample_play_simple_smoke(void);
extern "C" int zsnd_is_muted_smoke(void);
extern "C" int zsnd_gain_scale_to_directsound_attenuation_smoke(void);
extern "C" int zsnd_global_volume_and_flag_helpers_smoke(void);
extern "C" int zsnd_preinitialize_runtime_state_smoke(void);
extern "C" int zsnd_sample_play_a3d_simple_direct_smoke(void);
extern "C" int zsnd_sample_play_a3d_worldpos_smoke(void);
extern "C" int zsnd_sample_acquire_play_handle_smoke(void);
extern "C" int zsnd_stream_request_state_update_smoke(void);
extern "C" int zsnd_fade_entry_backend_and_dispatch_smoke(void);
extern "C" int zsnd_fade_list_cursor_helpers_smoke(void);
extern "C" int zsnd_fade_active_list_tick_compacts_smoke(void);
extern "C" int zsnd_fade_lists_stop_all_shutdown_smoke(void);
extern "C" int zsnd_tick_backend_markers_smoke(void);
extern "C" int zsnd_report_error_helpers_smoke(void);
extern "C" int zsnd_stream_request_queue_smoke(void);
extern "C" int zsnd_set_use_archive_banks_flag_smoke(void);
extern "C" int zsnd_sample_set_registry_init_shutdown_smoke(void);
extern "C" int zsnd_sample_set_registry_lookup_destroy_smoke(void);
extern "C" int zsnd_sample_destroy_owned_data_smoke(void);
extern "C" int zsnd_sample_set_destroy_owned_data_smoke(void);
extern "C" int zsnd_sample_set_get_sample_at_smoke(void);
extern "C" int zsnd_find_sample_by_name_smoke(void);
extern "C" int zsnd_sample_set_init_by_name_empty_smoke(void);
extern "C" int zsnd_sample_set_init_loose_file_smoke(void);
extern "C" int zsnd_sample_set_load_samples_from_index_archive_smoke(void);
extern "C" int zsnd_create_queued_streaming_sample_smoke(void);
extern "C" int zsnd_sample_init_from_wave_data_directsound_smoke(void);
extern "C" int zsnd_sample_init_from_wave_data_a3d_smoke(void);
extern "C" int zsnd_wave_data_load_parse_reset_smoke(void);
extern "C" int zsnd_wave_data_parse_chunks_smoke(void);
extern "C" int zsnd_wave_data_load_parse_edges_smoke(void);
extern "C" int zsnd_wave_data_archive_load_smoke(void);
extern "C" int zsnd_snapshot_create_from_active_samples_smoke(void);
extern "C" int zsnd_snapshot_payload_capture_smoke(void);
extern "C" int zsnd_snapshot_item_new_node_smoke(void);
extern "C" int zsnd_snapshot_restore_all_with_global_volume_delta_smoke(void);
extern "C" int zsnd_snapshot_destroy_smoke(void);
extern "C" int zsnd_snapshot_stop_all_if_playing_smoke(void);
extern "C" int zsnd_cd_reset_track_state_smoke(void);
extern "C" int zsnd_cd_is_stereo_aux_enabled_smoke(void);
extern "C" int zsnd_cd_get_volume_smoke(void);
extern "C" int zsnd_cd_set_volume_smoke(void);
extern "C" int zsnd_cd_not_ready_playback_smoke(void);
extern "C" int zsnd_cd_playback_mci_commands_smoke(void);
extern "C" int zsnd_cd_on_mci_notify_loop_smoke(void);
extern "C" int zsnd_cd_init_ready_guard_smoke(void);
extern "C" int zsnd_cd_init_success_with_tracks_smoke(void);
extern "C" int zsnd_cd_get_track_count_ready_guard_smoke(void);
extern "C" int zsnd_cd_shutdown_track_list_smoke(void);
extern "C" int zsnd_cd_track_list_static_constructor_smoke(void);
extern "C" int zsnd_cd_track_list_static_destructor_smoke(void);
extern "C" int zsnd_cd_track_list_static_init_atexit_child_smoke(void);
extern "C" int zsnd_cd_track_list_static_init_atexit_smoke(void);
extern "C" int zreader_named_int_lookup_smoke(void);
extern "C" int zreader_get_named_node_smoke(void);
extern "C" int zreader_named_string_float_lookup_smoke(void);
extern "C" int zreader_global_string_prefix_index_smoke(void);
extern "C" int zrndr_global_string_table_load_dynamic_entries_smoke(void);
extern "C" int zreader_load_node_from_archive_smoke(void);
extern "C" int zreader_file_exists_and_list_create_smoke(void);
extern "C" int znetwork_local_identity_smoke(void);
extern "C" int znetwork_dplay_close_release_smoke(void);
extern "C" int znetwork_unregister_packet_handler_smoke(void);
extern "C" int znetwork_clear_enumerated_session_list_smoke(void);
extern "C" int znetwork_clear_service_provider_list_smoke(void);
extern "C" int znetwork_clear_player_record_list_smoke(void);
extern "C" int znetwork_player_record_accessors_smoke(void);
extern "C" int znetwork_apply_pkt01_player_color_assignments_smoke(void);
extern "C" int znetwork_shutdown_session_runtime_smoke(void);
extern "C" int zfmv_script_init_null_path_smoke(void);
extern "C" int zfmv_script_reset_smoke(void);
extern "C" int zfmv_script_cleanup_smoke(void);
extern "C" int zfmv_script_append_action_smoke(void);
extern "C" int zfmv_script_begin_current_action_smoke(void);
extern "C" int zfmv_script_begin_at_time_smoke(void);
extern "C" int zfmv_script_update_smoke(void);
extern "C" int zfmv_script_update_at_time_smoke(void);
extern "C" int zfmv_script_begin_now_smoke(void);
extern "C" int zfmv_script_load_actions_from_zrd_smoke(void);
extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void);
extern "C" int zfmv_action_image_constructor_scaled_smoke(void);
extern "C" int zfmv_action_fade_constructor_smoke(void);
extern "C" int zfmv_action_play_avi_constructor_existing_file_smoke(void);
extern "C" int zfmv_action_play_avi_constructor_drive_fallback_smoke(void);
extern "C" int zfmv_playback_constructor_smoke(void);
extern "C" int zfmv_playback_destructor_smoke(void);
extern "C" int zfmv_playback_report_mci_error_smoke(void);
extern "C" int zfmv_playback_open_and_play_smoke(void);
extern "C" int zfmv_playback_stop_and_close_smoke(void);
extern "C" int zfmv_playback_set_dest_rect_smoke(void);
extern "C" int zfmv_action_play_mci_constructor_smoke(void);
extern "C" int zfmv_action_blur_constructor_smoke(void);
extern "C" int zfmv_action_blur_update_smoke(void);
extern "C" int hud_ui_save_load_entry_is_newer_than_smoke(void);
extern "C" int player_underwater_fx_pass3_ui_constructor_smoke(void);
extern "C" int player_projectile_camera_fx_pass3_ui_constructor_smoke(void);
extern "C" int player_underwater_fx_pass3_ui_apply_blue_tint_smoke(void);
extern "C" int player_projectile_camera_fx_pass3_ui_apply_green_mask_smoke(void);
extern "C" int hud_ui_save_load_list_item_constructor_smoke(void);
extern "C" int hud_ui_save_load_list_item_draw_smoke(void);
extern "C" int hud_ui_save_load_list_item_on_activate_smoke(void);
extern "C" int hud_ui_save_load_delete_button_on_activate_smoke(void);
extern "C" int hud_ui_save_load_delete_save_file_smoke(void);
extern "C" int hud_ui_save_load_next_button_on_activate_smoke(void);
extern "C" int hud_ui_save_load_prev_button_on_activate_smoke(void);
extern "C" int hud_ui_save_game_primary_action_button_on_activate_smoke(void);
extern "C" int hud_ui_load_game_dialog_constructor_smoke(void);
extern "C" int hud_ui_load_game_primary_action_button_on_activate_smoke(void);
extern "C" int hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke(void);
extern "C" int hud_ui_credits_quit_button_on_activate_smoke(void);
extern "C" int hud_cheat_clear_nanite_panel_cheat_sentinel_smoke(void);
extern "C" int hud_ui_cheat_code_title_widget_on_activate_smoke(void);
extern "C" int hud_ui_cheat_code_dialog_constructor_smoke(void);
extern "C" int hud_ui_cheat_code_dialog_destructor_smoke(void);
extern "C" int hud_ui_cheat_code_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int hud_ui_new_game_panel_constructor_cluster_smoke(void);
extern "C" int hud_ui_new_game_panel_overlay_owner_queue_enter_smoke(void);
extern "C" int hud_ui_new_game_panel_overlay_owner_on_try_become_current_smoke(void);
extern "C" int hud_ui_new_game_panel_overlay_owner_lifecycle_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_constructor_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_destructor_core_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_static_init_thunks_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_queue_enter_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_on_try_become_current_smoke(void);
extern "C" int recoil_app_state_queue_block_init_from_cursor_smoke(void);
extern "C" int recoil_app_queue_switch_current_state_smoke(void);
extern "C" int recoil_app_queue_push_state_smoke(void);
extern "C" int recoil_app_queue_exit_current_state_smoke(void);
extern "C" int recoil_app_mfc_ole_module_constructor_smoke(void);
extern "C" int recoil_app_mfc_ole_module_destructor_smoke(void);
extern "C" int recoil_app_constructor_destructor_smoke(void);
extern "C" int czgame_frame_constructor_smoke(void);
extern "C" int czrecoil_frame_constructor_smoke(void);
extern "C" int recoil_app_fmv_state_destructor_smoke(void);
extern "C" int recoil_app_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_app_mission_fmv_state_destructor_smoke(void);
extern "C" int recoil_app_initialize_display_failure_smoke(void);
extern "C" int recoil_app_start_engine_and_queue_startup_state_smoke(void);
extern "C" int recoil_state_main_menu_transition_constructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke(void);
extern "C" int hud_ui_main_menu_dialog_constructor_smoke(void);
extern "C" int recoil_state_cheat_code_constructor_smoke(void);
extern "C" int recoil_state_controls_lifecycle_smoke(void);
extern "C" int recoil_state_controls_activation_smoke(void);
extern "C" int recoil_state_controls_on_resume_smoke(void);
extern "C" int recoil_state_controls_queue_enter_smoke(void);
extern "C" int recoil_state_confirm_quit_queue_enter_smoke(void);
extern "C" int recoil_state_confirm_quit_destructor_smoke(void);
extern "C" int hud_ui_load_game_dialog_on_primary_action_smoke(void);
extern "C" int hud_ui_load_game_dialog_on_primary_action_thunk_smoke(void);
extern "C" int hud_ui_load_game_dialog_process_dialog_result_smoke(void);
extern "C" int hud_ui_save_load_process_dialog_result_smoke(void);
extern "C" int hud_ui_save_load_game_name_input_raw_keyboard_smoke(void);
extern "C" int hud_ui_save_load_game_name_input_smoke(void);
extern "C" int hud_ui_container_constructor_smoke(void);
extern "C" int hud_ui_container_set_enabled_smoke(void);
extern "C" int hud_ui_background_container_constructor_smoke(void);
extern "C" int hud_ui_save_load_insert_entry_sorted_prefix_smoke(void);
extern "C" int hud_ui_save_load_partition_entries_by_pivot_smoke(void);
extern "C" int hud_ui_save_load_sort_entry_range_smoke(void);
extern "C" int hud_ui_save_load_refresh_file_list_smoke(void);
extern "C" int hud_ui_save_load_initialize_file_entries_smoke(void);
extern "C" int hud_ui_save_load_set_selected_entry_index_smoke(void);
extern "C" int hud_ui_save_load_dialog_destructor_smoke(void);
extern "C" int hud_ui_save_game_dialog_destructor_smoke(void);
extern "C" int hud_ui_load_game_dialog_destructor_smoke(void);
extern "C" int hud_ui_save_game_dialog_init_layout_smoke(void);
extern "C" int hud_ui_main_menu_dialog_save_load_checks_smoke(void);
extern "C" int zarchive_list_get_at_smoke(void);
extern "C" int zarchive_list_get_count_smoke(void);
extern "C" int zreader_archive_list_and_search_paths_smoke(void);
extern "C" int zreader_zrdr_free_search_path_list_smoke(void);
extern "C" int zutil_set_mission_zrdr_paths_and_mount_zbd_smoke(void);
extern "C" int zreader_prealloc_and_pop_front_smoke(void);
extern "C" int zreader_zrdr_push_free_node_smoke(void);
extern "C" int zreader_zrdr_init_search_path_smoke(void);
extern "C" int zreader_zrdr_shutdown_smoke(void);
extern "C" int zreader_zrdr_wildcard_path_smoke(void);
extern "C" int zreader_zrdr_free_node_pool_smoke(void);
extern "C" int zreader_mount_index_archive_smoke(void);
extern "C" int zreader_retail_zrdr_archives_smoke(void);
extern "C" int zreader_index_archive_flush_close_smoke(void);
extern "C" int zreader_zrdr_get_file_size_smoke(void);
extern "C" int zreader_free_loaded_tree_smoke(void);
extern "C" int zreader_load_movers_from_zrd_smoke(void);
extern "C" int zreader_resolve_and_open_file_smoke(void);
extern "C" int briefing_runtime_constructor_smoke(void);
extern "C" int briefing_runtime_destructor_smoke(void);
extern "C" int briefing_locator_panel_constructor_smoke(void);
extern "C" int briefing_locator_panel_blit_dirty_rect_smoke(void);
extern "C" int briefing_locator_panel_update_smoke(void);
extern "C" int briefing_objective_picture_draw_noise_overlay_smoke(void);
extern "C" int briefing_runtime_update_smoke(void);
extern "C" int briefing_build_objective_actions_smoke(void);
extern "C" int zhud_element_invalidate_smoke(void);
extern "C" int zhud_element_clip_and_invalidate_smoke(void);
extern "C" int zhud_element_constructor_smoke(void);
extern "C" int zhud_element_copy_constructor_smoke(void);
extern "C" int zhud_element_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_element_destructor_smoke(void);
extern "C" int zhud_element_draw_dispatch_smoke(void);
extern "C" int zhud_element_draw_base_smoke(void);
extern "C" int zhud_element_update_smoke(void);
extern "C" int zhud_element_visible_smoke(void);
extern "C" int zhud_element_position_mutators_smoke(void);
extern "C" int zhud_element_get_xy_smoke(void);
extern "C" int zhud_element_get_rect_smoke(void);
extern "C" int zhud_element_hit_test_true_smoke(void);
extern "C" int zhud_circle_constructor_and_hit_test_smoke(void);
extern "C" int zhud_circle_draw_dirty_smoke(void);
extern "C" int zhud_widget_constructor_smoke(void);
extern "C" int zhud_widget_invalidate_rect_smoke(void);
extern "C" int zhud_widget_draw_smoke(void);
extern "C" int hud_ui_mp_exit_dialog_load_layout_smoke(void);
extern "C" int zhud_slot_draw_smoke(void);
extern "C" int zhud_triplet_panel_draw_smoke(void);
extern "C" int zhud_triplet_panel_set_visible_count_smoke(void);
extern "C" int zhud_triplet_interpolate_layout_smoke(void);
extern "C" int zhud_triplet_is_local_player_first_entry_smoke(void);
extern "C" int zhud_mgr_is_local_player_first_in_stats_list_smoke(void);
extern "C" int zhud_scoreboard_set_scale_and_rebuild_smoke(void);
extern "C" int zhud_triplet_scoreboard_entry_update_smoke(void);
extern "C" int zhud_text_stack_constructors_smoke(void);
extern "C" int zhud_text_stack_set_font_all_smoke(void);
extern "C" int zhud_element_set_timer_smoke(void);
extern "C" int zhud_text_stack_push_line_smoke(void);
extern "C" int zhud_text_stack_clear_and_enable_smoke(void);
extern "C" int zhud_text_stack_clear_and_disable_smoke(void);
extern "C" int zhud_text_stack_destructor_core_smoke(void);
extern "C" int zhud_text_stack_layout_mutators_smoke(void);
extern "C" int zhud_list_menu_entry_sort_smoke(void);
extern "C" int zhud_layout_hw_update_objective_dirty_rect_smoke(void);
extern "C" int zhud_objective_update_meter_xpoints_smoke(void);
extern "C" int zhud_mgr_trigger_current_layout_on_activated_smoke(void);
extern "C" int zhud_counter_constructor_smoke(void);
extern "C" int hud_ui_set_invalidate_mode_smoke(void);
extern "C" int zhud_bar_and_meter_constructor_smoke(void);
extern "C" int zhud_widget_release_image_if_owned_smoke(void);
extern "C" int zhud_widget_set_image_borrowed_and_invalidate_smoke(void);
extern "C" int zhud_widget_destructor_core_smoke(void);
extern "C" int zhud_fill_bitmap_core_smoke(void);
extern "C" int zhud_zrd_widget_ex17c_item_core_smoke(void);
extern "C" int zhud_widget_set_image_by_path_owned_smoke(void);
extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void);
extern "C" int zhud_background_cursor_widget_rebuild_captured_image_smoke(void);
extern "C" int zhud_background_cursor_widget_set_image_borrowed_refresh_smoke(void);
extern "C" int zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke(void);
extern "C" int zhud_background_video_widget_constructor_smoke(void);
extern "C" int zhud_background_video_widget_destructor_smoke(void);
extern "C" int zhud_background_constructor_smoke(void);
extern "C" int zhud_background_update_input_focus_smoke(void);
extern "C" int zhud_background_set_enabled_smoke(void);
extern "C" int zhud_text_label_constructor_and_extents_smoke(void);
extern "C" int zhud_panel_constructor_default_smoke(void);
extern "C" int zhud_panel_copy_construct_core_smoke(void);
extern "C" int zhud_panel_draw_smoke(void);
extern "C" int zhud_panel_set_font_smoke(void);
extern "C" int zhud_panel_set_text_fmt_smoke(void);
extern "C" int zhud_panel_query_text_height_smoke(void);
extern "C" int zhud_panel_layout_entry_copy_construct_smoke(void);
extern "C" int zhud_panel_layout_entry_copy_assign_smoke(void);
extern "C" int zhud_panel_layout_entry_copy_assign_range_smoke(void);
extern "C" int zhud_panel_layout_entry_destroy_range_smoke(void);
extern "C" int zhud_util_free_field_ptr_smoke(void);
extern "C" int zhud_cmd_binding_entry_copy_range_smoke(void);
extern "C" int zhud_cmd_binding_destroy_range_smoke(void);
extern "C" int zhud_cmd_command_list_destructor_smoke(void);
extern "C" int zhud_cmd_key_a_button_destructor_smoke(void);
extern "C" int zhud_cmd_key_b_button_destructor_smoke(void);
extern "C" int zhud_cmd_joy_button_destructor_smoke(void);
extern "C" int zhud_cmd_mouse_button_destructor_smoke(void);
extern "C" int zhud_composite_panel_vector_clear_smoke(void);
extern "C" int zhud_composite_panel_vector_insert_copies_smoke(void);
extern "C" int zhud_composite_panel_entry_copy_smoke(void);
extern "C" int zhud_composite_panel_constructor_with_entry_count_smoke(void);
extern "C" int zhud_composite_panel_destructor_smoke(void);
extern "C" int zhud_composite_panel_update_smoke(void);
extern "C" int zhud_transition_text_panel_update_smoke(void);
extern "C" int zhud_transition_text_panel_flash_rate_smoke(void);
extern "C" int zhud_composite_panel_layout_entries_smoke(void);
extern "C" int zhud_composite_panel_set_text_fmt_smoke(void);
extern "C" int zhud_composite_panel_set_font_smoke(void);
extern "C" int zhud_composite_panel_resize_entry_count_smoke(void);
extern "C" int zhud_composite_panel_resize_vector_relayout_smoke(void);
extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void);
extern "C" int zhud_container_child_list_smoke(void);
extern "C" int zhud_zrd_widget_constructor_smoke(void);
extern "C" int zhud_zrd_widget_helpers_smoke(void);
extern "C" int zhud_zrd_widget_load_from_zrd_smoke(void);
extern "C" int zhud_cycle_selector_widget_constructor_smoke(void);
extern "C" int zhud_cycle_selector_text_entry_smoke(void);
extern "C" int hud_ui_mp_exit_dialog_table_cluster_smoke(void);
extern "C" int recoil_app_mp_exit_dialog_state_on_enter_smoke(void);
extern "C" int recoil_app_mp_exit_dialog_state_on_deactivate_smoke(void);
extern "C" int recoil_app_mp_exit_dialog_state_on_try_become_current_smoke(void);
extern "C" int recoil_app_mp_exit_dialog_state_on_update_should_quit_smoke(void);
extern "C" int zhud_options_panel_lighting_init_from_options_smoke(void);
extern "C" int zhud_options_panel_lighting_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_perspective_init_from_options_smoke(void);
extern "C" int zhud_options_panel_full_hud_init_from_options_smoke(void);
extern "C" int zhud_options_panel_object_detail_init_from_options_smoke(void);
extern "C" int zhud_options_panel_object_detail_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_texture_memory_init_from_options_smoke(void);
extern "C" int zhud_options_panel_texture_memory_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_effects_init_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_active_init_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_quality_init_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_quality_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_volume_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_volume_on_activate_smoke(void);
extern "C" int zhud_options_panel_music_volume_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_music_volume_on_activate_smoke(void);
extern "C" int zhud_options_panel_resolution_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_resolution_on_activate_smoke(void);
extern "C" int zhud_options_dialog_constructor_smoke(void);
extern "C" int zhud_options_dialog_destructor_core_smoke(void);
extern "C" int zhud_options_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_credits_panel_constructor_smoke(void);
extern "C" int zhud_credits_panel_destructor_smoke(void);
extern "C" int zhud_credits_panel_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_scrolling_text_destructor_smoke(void);
extern "C" int zhud_scrolling_text_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_scrolling_text_load_from_zrd_smoke(void);
extern "C" int zhud_scrolling_text_update_smoke(void);
extern "C" int zhud_scrolling_text_on_activate_reset_owner_fade_smoke(void);
extern "C" int zhud_scrolling_text_update_scroll_positions_smoke(void);
extern "C" int zhud_credits_panel_update_fade_and_exit_smoke(void);
extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void);
extern "C" int zhud_cmd_bind_button_base_destructor_core_smoke(void);
extern "C" int zhud_check_toggle_widget_helpers_smoke(void);
extern "C" int zhud_check_toggle_widget_load_from_zrd_smoke(void);
extern "C" int zhud_cmd_dialog_on_command_selection_changed_smoke(void);
extern "C" int zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke(void);
extern "C" int zhud_cmd_bind_button_base_rebuild_binding_slot_widgets_smoke(void);
extern "C" int zhud_cmd_bind_button_base_load_from_zrd_smoke(void);
extern "C" int zhud_cmd_reset_button_on_activate_smoke(void);
extern "C" int zhud_cmd_set_list_widget_on_activate_smoke(void);
extern "C" int zhud_cmd_key_a_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_key_b_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_joy_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_mouse_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_key_a_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_key_b_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_joy_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_mouse_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_dialog_rebuild_command_binding_lists_smoke(void);
extern "C" int zhud_cmd_dialog_apply_primary_key_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_apply_secondary_key_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_apply_joystick_button_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_apply_mouse_button_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_update_capture_state_idle_smoke(void);
extern "C" int zhud_cmd_dialog_select_group_relative_smoke(void);
extern "C" int zhud_cmd_dialog_select_command_relative_smoke(void);
extern "C" int zhud_cmd_dialog_callback_navigation_smoke(void);
extern "C" int zhud_cmd_dialog_constructor_smoke(void);
extern "C" int zhud_cmd_dialog_destructor_smoke(void);
extern "C" int zhud_cmd_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_cmd_dialog_state_lifecycle_smoke(void);
extern "C" int zhud_cmd_dialog_state_on_try_become_current_smoke(void);
extern "C" int zhud_cmd_dialog_state_on_deactivate_smoke(void);
extern "C" int zhud_text_input_constructor_smoke(void);
extern "C" int zhud_text_input_destructor_core_smoke(void);
extern "C" int zhud_text_input_constructor_and_alloc_smoke(void);
extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void);
extern "C" int zhud_numeric_text_input_base_constructor_smoke(void);
extern "C" int zhud_background_bind_primitive_node_to_element_smoke(void);
extern "C" int zhud_std_ptr_vector_clear_no_op_destroy_smoke(void);
extern "C" int zmath_matrix_stack_and_direction_smoke(void);
extern "C" int zmath_crt_matherr_handler_smoke(void);
extern "C" int zmath_vec3_array_transform_direction_smoke(void);
extern "C" int zmath_mat_build_euler_rotation3x3_smoke(void);
extern "C" int zmath_extract_euler_smoke(void);
extern "C" int zmath_projection_setup_smoke(void);
extern "C" int zmath_projection_batches_smoke(void);
extern "C" int zmath_project_point_and_clamp_to_screen_clip_smoke(void);
extern "C" int zmath_clip_line_segment_z_range_smoke(void);
extern "C" int zmath_vec3_lerp_smoke(void);
extern "C" int zmath_vec3_lerp_normalize_smoke(void);
extern "C" int zmath_vec3_direction_to_smoke(void);
extern "C" int zmath_line_vs_sphere_hit_smoke(void);
extern "C" int zmath_vec3_perp2d_smoke(void);
extern "C" int zmath_vec3_perp_xz_smoke(void);
extern "C" int zmath_vec3_scale_add_smoke(void);
extern "C" int zmath_vec3_slerp_smoke(void);
extern "C" int zmath_vec3_midpoint_smoke(void);
extern "C" int zmath_perspective_texture_interpolants_smoke(void);
extern "C" int zmath_vec3_normalize_and_div_scalar_smoke(void);
extern "C" int zmath_array_add_scaled_and_transform_smoke(void);
extern "C" int zmath_load_view_smoke(void);
extern "C" int zmath_quaternion_helpers_smoke(void);
extern "C" int zmath_approx_exp_neg_smoke(void);
extern "C" int zrndr_get_active_region_state_smoke(void);
extern "C" int zrndr_framebuffer_and_stride_cache_smoke(void);
extern "C" int zrndr_init_globals_smoke(void);
extern "C" int zrndr_immediate_line_dispatch_smoke(void);
extern "C" int zrndr_lens_flare_queue_projected_sample_smoke(void);
extern "C" int zrndr_lens_flare_build_visible_sample_list_smoke(void);
extern "C" int zrndr_lens_flare_draw_queued_samples16_smoke(void);
extern "C" int zrndr_lens_flare_draw_sample_smoke(void);
extern "C" int zrndr_lens_flare_draw_queued_samples_scaled16_smoke(void);
extern "C" int zrndr_lens_flare_stage_helpers_smoke(void);
extern "C" int zrndr_span_occlusion_filter_sample_list_smoke(void);
extern "C" int zrndr_lens_flare_draw_sample_stage_clipped_smoke(void);
extern "C" int zrndr_lens_flare_draw_visible_sample_stages_smoke(void);
extern "C" int zrndr_lens_flare_draw_visible_sample_smoke(void);
extern "C" int zrndr_lens_flare_draw_visible_samples_smoke(void);
extern "C" int zrndr_span_occlusion_add_polygon_smoke(void);
extern "C" int zrndr_span_occlusion_submit_rect_smoke(void);
extern "C" int zrndr_span_occlusion_test_depth_order_pair_smoke(void);
extern "C" int zrndr_span_occlusion_insert_no_depth_smoke(void);
extern "C" int zrndr_span_occlusion_build_span_list_smoke(void);
extern "C" int zrndr_span_occlusion_insert_local_smoke(void);
extern "C" int zrndr_span_occlusion_build_span_list_fast_smoke(void);
extern "C" int zrndr_span_occlusion_test_column_visibility_smoke(void);
extern "C" int zscene_test_projected_sphere_visible_smoke(void);
extern "C" int zrndr_span_occlusion_test_point_visibility_smoke(void);
extern "C" int zrndr_span_occlusion_test_sample_smoke(void);
extern "C" int zrndr_draw_circle_octants_smoke(void);
extern "C" int zrndr_draw_circle_outline_smoke(void);
extern "C" int zrndr_plot_pixel16_smoke(void);
extern "C" int zrndr_draw_line16_smoke(void);
extern "C" int zrndr_draw_line16_segmented_smoke(void);
extern "C" int zrndr_draw_line16_clipped_smoke(void);
extern "C" int zcliprect_clip_poly_near_z_smoke(void);
extern "C" int zcliprect_clip_poly_near_z_attr0_smoke(void);
extern "C" int zcliprect_clip_poly_zrange_attr012_smoke(void);
extern "C" int zcliprect_clip_poly_zrange_no_uv_smoke(void);
extern "C" int zcliprect_clip_poly_zrange_no_uv_attribs_smoke(void);
extern "C" int zcliprect_clip_poly_no_uv_alt_smoke(void);
extern "C" int zcliprect_clip_poly_no_uv_smoke(void);
extern "C" int zcliprect_clip_poly_uv_smoke(void);
extern "C" int zcliprect_clip_poly_uv_attr012_smoke(void);
extern "C" int zcliprect_clip_poly_no_uv_attr0_alt_smoke(void);
extern "C" int zcliprect_clip_poly_no_uv_attr012_alt_smoke(void);
extern "C" int zcliprect_trivial_reject_poly_xy_smoke(void);
extern "C" int zrndr_span_occlusion_reset_shutdown_smoke(void);
extern "C" int zrndr_span_occlusion_init_build_smoke(void);
extern "C" int zrndr_span_occlusion_rasterize_smoke(void);
extern "C" int zrndr_rasterize_poly_with_span_list_smoke(void);
extern "C" int zrndr_rasterize_poly_smoke(void);
extern "C" int zrndr_draw_flat_immediate_smoke(void);
extern "C" int zrndr_submit_poly_with_span_list_smoke(void);
extern "C" int zrndr_submit_textured_poly_uniform_smoke(void);
extern "C" int zrndr_submit_textured_poly_per_vertex_smoke(void);
extern "C" int zrndr_flush_transparent_queue_smoke(void);
extern "C" int zrndr_flush_overwrite_queue_smoke(void);
extern "C" int zrndr_texture_mip_select_variant_smoke(void);
extern "C" int zrndr_draw_flat_queued_smoke(void);
extern "C" int zrndr_renderer_draw_poly_tlv_smoke(void);
extern "C" int zrndr_draw_textured_queued_smoke(void);
extern "C" int zrndr_draw_textured_queued_alpha_smoke(void);
extern "C" int zrndr_draw_textured_fan_tri_smoke(void);
extern "C" int zrndr_span_routine_selection_smoke(void);
extern "C" int zrndr_perspective_texture_delta_x_smoke(void);
extern "C" int zrndr_perspective_texture_far_z_smoke(void);
extern "C" int zrndr_set_inverse_z_tolerance_smoke(void);
extern "C" int zrndr_perspective_adaptive_span_params_smoke(void);
extern "C" int zrndr_overlay_rect_submit_smoke(void);
extern "C" int zrndr_overlay_and_mmx_masks_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_const_alpha_pal8_smoke(void);
extern "C" int zrndr_fill_span16_opaque_smoke(void);
extern "C" int zrndr_fill_span555_solid_smoke(void);
extern "C" int zrndr_fill_span565_solid_smoke(void);
extern "C" int zrndr_span_masked_tex16_to_565_smoke(void);
extern "C" int zrndr_span_masked_16_from_pal8_to565_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_const_alpha_tex16_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_from_tex16_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_const_alpha_from_tex16_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_mmx_from_tex16_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_from_tex16_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_const_alpha_from_tex16_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_mmx_from_tex16_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_from_pal8_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_from_pal8_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_const_alpha_from_pal8_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_const_alpha_from_pal8_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_mmx_from_pal8_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_mmx_from_pal8_alpha8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_const_alpha_tex16_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_const_alpha_fast_pal8_smoke(void);
extern "C" int zrndr_span_alpha_blend_555_const_alpha_fast_pal8_smoke(void);
extern "C" int zrndr_fog_blend_span_565_scalar_smoke(void);
extern "C" int zrndr_fog_blend_span_555_scalar_smoke(void);
extern "C" int zrndr_fog_blend_span_565_mmx_smoke(void);
extern "C" int zrndr_fog_blend_span_555_mmx_smoke(void);
extern "C" int zrndr_span_copy_16_from_tex16_smoke(void);
extern "C" int zrndr_span_copy_16_from_tex16_switch_vshift_smoke(void);
extern "C" int zrndr_span_masked_16_from_tex16_switch_vshift_smoke(void);
extern "C" int zrndr_span_copy_16_from_pal8_switch_vshift_smoke(void);
extern "C" int zrndr_span_masked_16_from_pal8_switch_vshift_smoke(void);
extern "C" int zrndr_span_shade_16_from_pal8_switch_vshift_smoke(void);
extern "C" int zrndr_palette_remap_key_smoke(void);
extern "C" int zrndr_fog_target_color_smoke(void);
extern "C" int zrndr_fog_commit_and_blend_smoke(void);
extern "C" int zrndr_and_zmodel_current_fog_color_smoke(void);
extern "C" int zvideo_fog_color_commit_smoke(void);
extern "C" int zvideo_fog_target_color_commit_smoke(void);
extern "C" int zturret_reset_iteration_state_smoke(void);
extern "C" int zturret_runtime_init_defaults_smoke(void);
extern "C" int zturret_runtime_has_active_node_smoke(void);
extern "C" int zturret_runtime_init_from_reader_node_smoke(void);
extern "C" int zturret_update_fire_position_from_parts_smoke(void);
extern "C" int zturret_update_aim_and_part_matrices_smoke(void);
extern "C" int zturret_select_fire_point_and_aim_smoke(void);
extern "C" int zturret_update_fire_burst_timer_smoke(void);
extern "C" int zturret_runtime_tick_smoke(void);
extern "C" int zturret_tick_all_runtimes_round_robin_smoke(void);
extern "C" int zturret_disable_tick_callback_smoke(void);
extern "C" int zturret_enable_tick_callback_smoke(void);
extern "C" int zturret_load_definitions_from_path_smoke(void);
extern "C" int zturret_fire_weapon_smoke(void);
extern "C" int zturret_fire_weapon_callback_smoke(void);
extern "C" int zturret_damage_and_on_damage_smoke(void);
extern "C" int zturret_shutdown_leaf_smoke(void);
extern "C" int zgame_return_only_stub_smoke(void);
extern "C" int zopt_network_enabled_accessor_smoke(void);
extern "C" int hud_sensor_mission_identity_smoke(void);
extern "C" int hud_sensor_tracker_get_objective_briefing_strings_smoke(void);
extern "C" int zclass_type_list_alloc_and_insert_smoke(void);
extern "C" int zclass_alloc_node_from_free_list_smoke(void);
extern "C" int zclass_node_propagate_transform_dirty_smoke(void);
extern "C" int zclass_object3d_reset_transform_dirty_smoke(void);
extern "C" int zclass_object3d_init_smoke(void);
extern "C" int zclass_node_action_callback_smoke(void);
extern "C" int zclass_node_priority_smoke(void);
extern "C" int zclass_find_by_name_and_filtered_iter_smoke(void);
extern "C" int zclass_sound_leaf_smoke(void);
extern "C" int zclass_sound_get_position_smoke(void);
extern "C" int zloc_message_lookup_failure_smoke(void);
extern "C" int zloc_load_unload_messages_dll_smoke(void);
extern "C" int zimage_font_glyph_scan_smoke(void);
extern "C" int zimage_font_measure_string_smoke(void);
extern "C" int zimage_font_blit_string_smoke(void);
extern "C" int zimage_fonts_load_missing_smoke(void);
extern "C" int zvid_pack_color_rgb_smoke(void);
extern "C" int zvid_pack_color_rgb_floats_smoke(void);
extern "C" int zvideo_palette_remap_no_recipes_smoke(void);
extern "C" int zvideo_palette_remap_recipe_variants_smoke(void);
extern "C" int zvid_image_create_format_size_pixels_smoke(void);
extern "C" int zvideo_image_set_pixels_smoke(void);
extern "C" int zvideo_capture_surface_to_image_smoke(void);
extern "C" int zvideo_fx_set_surface_state_smoke(void);
extern "C" int zvideo_surface_accessors_smoke(void);
extern "C" int zvideo_primary_surface_rect_scratch_smoke(void);
extern "C" int zvideo_image_alpha_clear_smoke(void);
extern "C" int zvideo_mode_geometry_and_set_video_mode_smoke(void);
extern "C" int zvideo_init_video_system_reentry_guard_smoke(void);
extern "C" int zvideo_bind_renderer_dispatch_smoke(void);
extern "C" int zvideo_module_init_smoke(void);
extern "C" int zvideo_at_exit_release_all_interfaces_and_surfaces_smoke(void);
extern "C" int zvideo_return_success_stub_smoke(void);
extern "C" int zvideo_clear_dispatch_and_exchange_smoke(void);
extern "C" int zvid_cached_client_rect_smoke(void);
extern "C" int zvideo_shutdown_video_system_smoke(void);
extern "C" int zvideo_frame_scratch_buffers_smoke(void);
extern "C" int zvideo_noise_shutdown_buffers_smoke(void);
extern "C" int zvideo_buff_clip_coord_to_range_smoke(void);
extern "C" int zvideo_buff_blt_source_to_primary_clipped_smoke(void);
extern "C" int zvid_image_blit_to_active_target_smoke(void);
extern "C" int zvid_image_blit_to_framebuffer_clipped_smoke(void);
extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void);
extern "C" int zvideo_draw_noise_rect_smoke(void);
extern "C" int zvideo_blur_region_horizontal_smoke(void);
extern "C" int zvideo_blur_region_vertical_smoke(void);
extern "C" int zvideo_blur_region_combined_smoke(void);
extern "C" int zvideo_blur_region_by_mode_smoke(void);
extern "C" int zvideo_texture_pack_load_image_smoke(void);
extern "C" int zvideo_dd_prepare_window_for_mode_smoke(void);
extern "C" int zvid_query_device_video_memory_bytes_smoke(void);
extern "C" int zvid_query_texture_memory_bytes_smoke(void);
extern "C" int zvideo_pixel_pack_setup_smoke(void);
extern "C" int zvideo_pixel_pack_getters_smoke(void);
extern "C" int zvideo_dd_report_error_smoke(void);
extern "C" int zvideo_dd_create_directdraw2_for_selected_device_smoke(void);
extern "C" int zvideo_dd_open_video_mode_smoke(void);
extern "C" int zvideo_dd_create_surface3_from_desc_smoke(void);
extern "C" int zvideo_dispatch_wrappers_smoke(void);
extern "C" int zvideo_dd_lock_directdraw_surface_smoke(void);
extern "C" int zvideo_dd_unlock_directdraw_surface_smoke(void);
extern "C" int zvideo_dd_lock_surface_wait_restore_smoke(void);
extern "C" int zvideo_dd_unlock_surface_wait_restore_smoke(void);
extern "C" int zvideo_surface_state_lock_skip_smoke(void);
extern "C" int zvideo_dd_lock_surface_state_smoke(void);
extern "C" int zvideo_dd_unlock_surface_state_smoke(void);
extern "C" int zvideo_dd_verify_fullscreen_surface_locks_smoke(void);
extern "C" int zvideo_texture_record_destroy_smoke(void);
extern "C" int zvideo_dd_release_all_interfaces_and_surfaces_smoke(void);
extern "C" int zvideo_dd_verify_surface_state_locking_smoke(void);
extern "C" int zvideo_dd_teardown_video_subsystem_smoke(void);
extern "C" int zvideo_dd_shutdown_video_system_smoke(void);
extern "C" int zvideo_dd_zbuffer_depth_fill_rect_smoke(void);
extern "C" int zvideo_dd_clear_screen_and_zbuffer_rect_smoke(void);
extern "C" int zvideo_dd_clear_sw_backbuffer_and_zbuffer_rects_smoke(void);
extern "C" int zvideo_dd_palette_set_entries_smoke(void);
extern "C" int zvideo_get_display_mode_bpp_smoke(void);
extern "C" int zvideo_dd_set_display_mode_smoke(void);
extern "C" int zvideo_dd_set_video_mode_smoke(void);
extern "C" int zvideo_dd_restore_display_surfaces_smoke(void);
extern "C" int zvideo_dd_init_fullscreen_software_pixel_pack_smoke(void);
extern "C" int zvideo_dd_create_half_res_backbuffer_surfaces_smoke(void);
extern "C" int zvideo_dd_create_fullscreen_software_surfaces_smoke(void);
extern "C" int zvideo_dd_create_fullscreen_hw_surfaces_smoke(void);
extern "C" int zvideo_dd_create_fullscreen_surfaces_for_renderer_smoke(void);
extern "C" int zvideo_dd_present_display_mode_surface_smoke(void);
extern "C" int zvideo_present_display_mode_surface_null_smoke(void);
extern "C" int zvideo_dd3d_present_display_mode_surface_smoke(void);
extern "C" int zvideo_image_lazy_create_backing_surface_guards_smoke(void);
extern "C" int zvideo_dd_image_populate_surface_from_heap_pixels_smoke(void);
extern "C" int zvideo_dd_image_lazy_create_backing_surface_smoke(void);
extern "C" int zvideo_dd_image_lazy_create_video_memory_surface_smoke(void);
extern "C" int zvideo_dd_image_upload_pixels_to_surface_smoke(void);
extern "C" int zvideo_dd_image_release_surface_smoke(void);
extern "C" int zvideo_image_surface_helpers_guard_smoke(void);
extern "C" int zvideo_set_renderer_type_smoke(void);
extern "C" int zvideo_set_half_res_adjust_mode_smoke(void);
extern "C" int zvid_texture_pack_load_state_getter_smoke(void);
extern "C" int zvid_texture_pack_load_state_setter_smoke(void);
extern "C" int zvid_option_accessors_smoke(void);
extern "C" int zvideo_init_set_surface_geometry_from_mode_index_smoke(void);
extern "C" int zvideo_select_hw_api_device_smoke(void);
extern "C" int zvideo_dd_enum_direct3d_device_callback_smoke(void);
extern "C" int zvideo_dd_enumerate_direct3d_devices_for_record_smoke(void);
extern "C" int zvideo_dd_enum_directdraw_device_callback_smoke(void);
extern "C" int zvideo_dd_run_device_enumeration_smoke(void);
extern "C" int zvideo_dd_startup_enumerate_default_select_smoke(void);
extern "C" int zvideo_flip_to_gdi_if_attached_null_smoke(void);
extern "C" int zvideo_dd3d_set_fog_enable_smoke(void);
extern "C" int zvideo_pending_wireframe_state_smoke(void);
extern "C" int zvideo_pending_dither_enable_smoke(void);
extern "C" int zvideo_dd3d_begin_scene_flush_pending_smoke(void);
extern "C" int zvideo_submit_poly_color_attr_smoke(void);
extern "C" int zvideo_submit_poly_color_attr_immediate_smoke(void);
extern "C" int zvideo_submit_polygon_queue_smoke(void);
extern "C" int zvideo_submit_polygon_immediate_smoke(void);
extern "C" int zvideo_submit_polygon_lit_queue_smoke(void);
extern "C" int zvideo_submit_polygon_lit_immediate_smoke(void);
extern "C" int zvideo_texture_record_release_upload_surface_smoke(void);
extern "C" int zvideo_texture_record_finalize_upload_smoke(void);
extern "C" int zvideo_texture_record_lock_upload_surface_smoke(void);
extern "C" int zvideo_texture_record_unlock_upload_surface_smoke(void);
extern "C" int zvideo_texture_record_create_and_power_smoke(void);
extern "C" int zvideo_create_texture_record_guards_smoke(void);
extern "C" int zvideo_dd3d_create_texture_record_smoke(void);
extern "C" int zvideo_frustum_test_sphere_clip_mask_smoke(void);
extern "C" int zvideo_quad_batch_depth_and_rhw_smoke(void);
extern "C" int zvideo_queue_solid_quad_smoke(void);
extern "C" int zvideo_flush_quad_batch_empty_smoke(void);
extern "C" int zvideo_flush_quad_batch_smoke(void);
extern "C" int zvideo_flush_sorted_polys_empty_smoke(void);
extern "C" int zvideo_flush_sorted_polys_smoke(void);
extern "C" int zvideo_flush_overwrite_polys_empty_smoke(void);
extern "C" int zvideo_flush_overwrite_polys_smoke(void);
extern "C" int zvideo_dd3d_create_device_state_smoke(void);
extern "C" int zvideo_convert_image_pixels_for_texture_smoke(void);
extern "C" int zvideo_dd3d_upload_image_to_surface_smoke(void);
extern "C" int zimage_texdir_find_or_create_missing_smoke(void);
extern "C" int zimage_texdir_build_mip_chain_smoke(void);
extern "C" int zvid_texture_pack_ensure_builtin_smoke(void);
extern "C" int zvid_texture_pack_ensure_default_smoke(void);
extern "C" int zimage_texdir_load_pending_entries_smoke(void);
extern "C" int zimage_texdir_load_pending_entries_renderer_smoke(void);
extern "C" int zclass_node_load_flag_bit8_material_images_and_texture_pack_smoke(void);
extern "C" int zimage_texdir_base_name_path_smoke(void);
extern "C" int zimage_texdir_variant_image_smoke(void);
extern "C" int zimage_texdir_find_by_name_smoke(void);
extern "C" int zimage_texdir_write_smoke(void);
extern "C" int zimage_init_option_fallback_smoke(void);
extern "C" int zimage_init_texture_directory_smoke(void);
extern "C" int zvid_image_resample_square_smoke(void);
extern "C" int zvid_image_release_owned_buffers_smoke(void);
extern "C" int zvid_image_destroy_smoke(void);
extern "C" int zimg_init_smoke(void);
extern "C" int zinterp_context_logf_smoke(void);
extern "C" int zinterp_context_report_errorf_smoke(void);
extern "C" int zinterp_context_inc_error_count_smoke(void);
extern "C" int zinterp_context_find_macro_value_smoke(void);
extern "C" int zinterp_context_is_macro_true_smoke(void);
extern "C" int zinterp_context_set_macro_smoke(void);
extern "C" int zinterp_context_clear_tables_smoke(void);
extern "C" int zinterp_context_destroy_smoke(void);
extern "C" int zinterp_context_destructor_smoke(void);
extern "C" int zinterp_context_constructor_smoke(void);
extern "C" int zinterp_global_context_constructor_smoke(void);
extern "C" int zinterp_global_context_hooks_smoke(void);
extern "C" int zinterp_global_context_static_init_smoke(void);
extern "C" int zinterp_global_context_static_init_register_smoke(void);
extern "C" int zinterp_scroll_always_callbacks_smoke(void);
extern "C" int zinterp_register_scroll_always_node_smoke(void);
extern "C" int zinterp_context_eval_condition_expr_smoke(void);
extern "C" int zinterp_context_expand_macro_refs_smoke(void);
extern "C" int zinterp_context_next_token_smoke(void);
extern "C" int zinterp_context_parse_scalar_tokens_smoke(void);
extern "C" int zinterp_context_var_entry_helpers_smoke(void);
extern "C" int zinterp_context_command_helpers_smoke(void);
extern "C" int zinterp_context_validate_args_and_node_type_smoke(void);
extern "C" int zinterp_context_tokenize_line_smoke(void);
extern "C" int zinterp_context_tokenize_comment_and_prepared_smoke(void);
extern "C" int zinterp_context_echo_tokens_smoke(void);
extern "C" int zinterp_context_push_file_frame_smoke(void);
extern "C" int zinterp_context_pop_file_frame_smoke(void);
extern "C" int zinterp_context_print_node_tree_smoke(void);
extern "C" int zinterp_context_read_text_line_smoke(void);
extern "C" int zinterp_context_read_prepared_tokens_smoke(void);
extern "C" int zinterp_context_read_prepared_empty_packet_smoke(void);
extern "C" int zinterp_context_load_prepared_script_index_smoke(void);
extern "C" int zinterp_context_load_prepared_script_index_stale_smoke(void);
extern "C" int zinterp_context_open_prepared_script_stream_smoke(void);
extern "C" int zinterp_context_open_prepared_script_stream_newer_source_smoke(void);
extern "C" int zinterp_context_handle_builtin_command_smoke(void);
extern "C" int zinterp_context_run_stream_builtin_smoke(void);
extern "C" int zinterp_context_run_script_file_nested_source_smoke(void);
extern "C" int zinterp_context_dispatch_core_node_flags_smoke(void);
extern "C" int zinterp_context_dispatch_core_camera_clip_smoke(void);
extern "C" int zinterp_context_dispatch_core_world_and_globals_smoke(void);
extern "C" int zinterp_context_dispatch_core_resource_globals_smoke(void);
extern "C" int zinterp_context_dispatch_core_object3d_smoke(void);
extern "C" int zinterp_context_dispatch_core_model_material_smoke(void);
extern "C" int zinterp_context_dispatch_core_light_lod_smoke(void);
extern "C" int zimage_init_mission_resources_smoke(void);
extern "C" int zimage_shutdown_texdir_smoke(void);
extern "C" int zsys_find_file_on_drive_type_negative_smoke(void);
extern "C" int zsys_runtime_probe_leaves_smoke(void);
extern "C" int zsys_cpuid_mmx_smoke(void);
extern "C" int zsys_cpu_leaf_helpers_smoke(void);
extern "C" int zsys_exit_process_with_cleanup_child_smoke(void);
extern "C" int zsys_exit_process_with_cleanup_smoke(void);
extern "C" int zerror_init_output_context_smoke(void);
extern "C" int zerror_emit_debug_buffer_smoke(void);
extern "C" int zerror_report_old_noop_smoke(void);
extern "C" int zcom_query_interface_from_interface_map_smoke(void);
extern "C" int zcom_connection_point_container_advise_smoke(void);
extern "C" int zcom_connection_point_container_unadvise_smoke(void);
extern "C" int time_reset_smoke(void);
extern "C" int time_tick_smoke(void);

#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <limits>

extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {
struct SmokeTest {
    const char *name;
    int (*run)();
};

int g_smokeTextureMemoryQueryCalls;
int g_smokeDeviceMemoryQueryCalls;
int g_smokeLastTextureMemoryQueryFlags;
int g_smokeLastDeviceMemoryQueryFlags;
void *g_smokeDirectDraw2VTable[24];
int g_smokeDirectDraw2QueryInterfaceCalls;
IDirectDraw2 *g_smokeDirectDraw2LastQueryInterfaceSelf;
const GUID *g_smokeDirectDraw2LastQueryInterfaceIid;
void **g_smokeDirectDraw2LastQueryInterfaceOut;
HRESULT g_smokeDirectDraw2QueryInterfaceResult;
void *g_smokeDirectDraw2QueryInterfaceValue;
int g_smokeDirectDraw2GetAvailableVidMemCalls;
int g_smokeDirectDraw2ReleaseCalls;
IDirectDraw2 *g_smokeDirectDraw2ReleaseSelf;
int g_smokeDirectDraw2SetCooperativeLevelCalls;
IDirectDraw2 *g_smokeDirectDraw2LastSetCooperativeSelf;
HWND g_smokeDirectDraw2LastSetCooperativeHwnd;
DWORD g_smokeDirectDraw2LastSetCooperativeFlags;
HRESULT g_smokeDirectDraw2SetCooperativeLevelResult;
int g_smokeDirectDraw2SetDisplayModeCalls;
IDirectDraw2 *g_smokeDirectDraw2LastSetDisplayModeSelf;
DWORD g_smokeDirectDraw2LastDisplayModeWidth;
DWORD g_smokeDirectDraw2LastDisplayModeHeight;
DWORD g_smokeDirectDraw2LastDisplayModeBpp;
DWORD g_smokeDirectDraw2LastDisplayModeRefreshRate;
DWORD g_smokeDirectDraw2LastDisplayModeFlags;
HRESULT g_smokeDirectDraw2SetDisplayModeResult;
int g_smokeDirectDraw2CreateSurfaceCalls;
IDirectDraw2 *g_smokeDirectDraw2LastCreateSurfaceSelf;
DDSURFACEDESC *g_smokeDirectDraw2LastCreateSurfaceDesc;
DDSURFACEDESC g_smokeDirectDraw2CreateSurfaceDescs[4];
IDirectDrawSurface **g_smokeDirectDraw2LastCreateSurfaceOut;
IUnknown *g_smokeDirectDraw2LastCreateSurfaceOuter;
HRESULT g_smokeDirectDraw2CreateSurfaceResult;
IDirectDrawSurface *g_smokeDirectDraw2CreateSurfaceValue;
zVidImagePartial *g_smokeDirectDraw2MutateImageOnFirstCreateSurface;
void *g_smokeDirectDraw2MutatedPalette;
short g_smokeDirectDraw2MutatedPaletteMetaPacked;
int g_smokeDirectDraw2CreatePaletteCalls;
IDirectDraw2 *g_smokeDirectDraw2LastCreatePaletteSelf;
DWORD g_smokeDirectDraw2LastCreatePaletteFlags;
PALETTEENTRY *g_smokeDirectDraw2LastCreatePaletteEntries;
IDirectDrawPalette **g_smokeDirectDraw2LastCreatePaletteOut;
IUnknown *g_smokeDirectDraw2LastCreatePaletteOuter;
HRESULT g_smokeDirectDraw2CreatePaletteResult;
IDirectDrawPalette *g_smokeDirectDraw2CreatePaletteValue;
int g_smokeDirectDraw2CreateClipperCalls;
IDirectDraw2 *g_smokeDirectDraw2LastCreateClipperSelf;
DWORD g_smokeDirectDraw2LastCreateClipperFlags;
IDirectDrawClipper **g_smokeDirectDraw2LastCreateClipperOut;
IUnknown *g_smokeDirectDraw2LastCreateClipperOuter;
HRESULT g_smokeDirectDraw2CreateClipperResult;
IDirectDrawClipper *g_smokeDirectDraw2CreateClipperValue;
DDSCAPS g_smokeDirectDraw2LastAvailableVidMemCaps;
DWORD *g_smokeDirectDraw2LastAvailableVidMemTotal;
DWORD *g_smokeDirectDraw2LastAvailableVidMemFree;
DWORD g_smokeDirectDraw2AvailableVidMemTotal;
DWORD g_smokeDirectDraw2AvailableVidMemFree;
HRESULT g_smokeDirectDraw2GetAvailableVidMemResult;
void *g_smokeDirectDraw1VTable[3];
int g_smokeDirectDrawCreateCalls;
GUID *g_smokeDirectDrawCreateGuid;
IDirectDraw **g_smokeDirectDrawCreateOut;
IUnknown *g_smokeDirectDrawCreateOuter;
HRESULT g_smokeDirectDrawCreateResult;
int g_smokeDirectDrawQueryInterfaceCalls;
IDirectDraw *g_smokeDirectDrawQueryInterfaceSelf;
const GUID *g_smokeDirectDrawQueryInterfaceIid;
void **g_smokeDirectDrawQueryInterfaceOut;
HRESULT g_smokeDirectDrawQueryInterfaceResult;
IDirectDraw2 *g_smokeDirectDrawQueryInterfaceValue;
int g_smokeDirectDrawReleaseCalls;
IDirectDraw *g_smokeDirectDrawReleaseSelf;
void *g_smokeDirectDrawSurfaceVTable[32];
int g_smokeDirectDrawSurfaceQueryInterfaceCalls;
IDirectDrawSurface *g_smokeDirectDrawSurfaceLastQueryInterfaceSelf;
const GUID *g_smokeDirectDrawSurfaceLastQueryInterfaceIid;
void **g_smokeDirectDrawSurfaceLastQueryInterfaceOut;
HRESULT g_smokeDirectDrawSurfaceQueryInterfaceResult;
void *g_smokeDirectDrawSurfaceQueryInterfaceValue;
void *g_smokeDirectDrawSurfaceQueryInterfaceValues[4];
int g_smokeDirectDrawSurfaceQueryInterfaceValueCount;
int g_smokeDirectDrawSurfaceReleaseCalls;
IDirectDrawSurface *g_smokeDirectDrawSurfaceReleaseSelf;
int g_smokeDirectDrawSurfaceSetPaletteCalls;
IDirectDrawSurface *g_smokeDirectDrawSurfaceSetPaletteSurfaces[4];
IDirectDrawPalette *g_smokeDirectDrawSurfaceSetPalettePalettes[4];
HRESULT g_smokeDirectDrawSurfaceSetPaletteResult;
void *g_smokeComReleaseVTable[3];
int g_smokeComReleaseCalls;
void *g_smokeComReleaseObjects[8];
void *g_smokeDirectDrawSurface3VTable[39];
int g_smokeDirectDrawSurface3BltCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3BltSurfaces[4];
DWORD g_smokeDirectDrawSurface3BltFlags[4];
DDBLTFX g_smokeDirectDrawSurface3BltFxValues[4];
int g_smokeDirectDrawSurface3BltFxPresent[4];
RECT *g_smokeDirectDrawSurface3LastBltDstRectArg;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastBltSource;
RECT *g_smokeDirectDrawSurface3LastBltSrcRectArg;
DWORD g_smokeDirectDrawSurface3LastBltFlags;
DDBLTFX *g_smokeDirectDrawSurface3LastBltFx;
DDBLTFX g_smokeDirectDrawSurface3LastBltFxValue;
int g_smokeDirectDrawSurface3LastBltFxPresent;
HRESULT g_smokeDirectDrawSurface3BltResult;
HRESULT g_smokeDirectDrawSurface3BltResults[4];
int g_smokeDirectDrawSurface3BltResultCount;
int g_smokeDirectDrawSurface3FlipCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3FlipSurfaces[4];
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastFlipTarget;
DWORD g_smokeDirectDrawSurface3LastFlipFlags;
HRESULT g_smokeDirectDrawSurface3FlipResults[4];
int g_smokeDirectDrawSurface3FlipResultCount;
int g_smokeDirectDrawSurface3ReleaseCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3ReleaseSurfaces[4];
int g_smokeDirectDrawSurface3RestoreCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3RestoreSurfaces[4];
HRESULT g_smokeDirectDrawSurface3RestoreResult;
int g_smokeDirectDrawSurface3GetPixelFormatCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastPixelFormatSurface;
DWORD g_smokeDirectDrawSurface3LastPixelFormatInputSize;
DDPIXELFORMAT g_smokeDirectDrawSurface3PixelFormat;
HRESULT g_smokeDirectDrawSurface3GetPixelFormatResult;
int g_smokeDirectDrawSurface3GetAttachedSurfaceCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastAttachedSurfaceSelf;
DDSCAPS g_smokeDirectDrawSurface3LastAttachedSurfaceCaps;
IDirectDrawSurface3 **g_smokeDirectDrawSurface3LastAttachedSurfaceOut;
HRESULT g_smokeDirectDrawSurface3GetAttachedSurfaceResult;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3AttachedSurfaceValue;
int g_smokeDirectDrawSurface3LockCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LockSurfaces[4];
RECT *g_smokeDirectDrawSurface3LastLockRect;
DDSURFACEDESC *g_smokeDirectDrawSurface3LastLockDesc;
DWORD g_smokeDirectDrawSurface3LastLockFlags;
HANDLE g_smokeDirectDrawSurface3LastLockEvent;
DWORD g_smokeDirectDrawSurface3LockDescSize;
unsigned char g_smokeDirectDrawSurface3LockPixels[4];
void *g_smokeDirectDrawSurface3LockPixelsValue;
LONG g_smokeDirectDrawSurface3LockPitchValue;
HRESULT g_smokeDirectDrawSurface3LockResults[4];
int g_smokeDirectDrawSurface3LockResultCount;
int g_smokeDirectDrawSurface3UnlockCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3UnlockSurfaces[4];
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastUnlockSurface;
void *g_smokeDirectDrawSurface3LastUnlockArg;
HRESULT g_smokeDirectDrawSurface3UnlockResults[4];
int g_smokeDirectDrawSurface3UnlockResultCount;
int g_smokeDirectDrawSurface3SetClipperCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastSetClipperSelf;
IDirectDrawClipper *g_smokeDirectDrawSurface3LastSetClipperValue;
HRESULT g_smokeDirectDrawSurface3SetClipperResult;
int g_smokeDirectDrawSurface3AddAttachedSurfaceCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastAddAttachedSurfaceSelf;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastAttachedSurfaceArg;
HRESULT g_smokeDirectDrawSurface3AddAttachedSurfaceResult;
int g_smokeDirectDrawSurface3PageLockCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastPageLockSurface;
DWORD g_smokeDirectDrawSurface3LastPageLockFlags;
HRESULT g_smokeDirectDrawSurface3PageLockResult;
int g_smokeDirectDrawSurface3PageUnlockCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastPageUnlockSurface;
DWORD g_smokeDirectDrawSurface3LastPageUnlockFlags;
HRESULT g_smokeDirectDrawSurface3PageUnlockResult;
int g_smokeDirectDrawSurface3GetDCCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastGetDCSurface;
HDC *g_smokeDirectDrawSurface3LastGetDCOut;
HDC g_smokeDirectDrawSurface3GetDCValue;
HRESULT g_smokeDirectDrawSurface3GetDCResult;
int g_smokeDirectDrawSurface3ReleaseDCCalls;
IDirectDrawSurface3 *g_smokeDirectDrawSurface3LastReleaseDCSurface;
HDC g_smokeDirectDrawSurface3LastReleaseDCHdc;
HRESULT g_smokeDirectDrawSurface3ReleaseDCResult;
void *g_smokeDirectDrawClipperVTable[9];
int g_smokeDirectDrawClipperSetHWndCalls;
IDirectDrawClipper *g_smokeDirectDrawClipperLastSetHWndSelf;
DWORD g_smokeDirectDrawClipperLastSetHWndFlags;
HWND g_smokeDirectDrawClipperLastSetHWndValue;
HRESULT g_smokeDirectDrawClipperSetHWndResult;
void *g_smokeSurfaceLockVerifierVTable[5];
int g_smokeSurfaceLockVerifierReleaseCalls;
int g_smokeSurfaceLockVerifierVerifyCalls;
zVideo_SurfaceLockVerifyArgs g_smokeSurfaceLockVerifierLastArgs;
void *g_smokeDirectDrawPaletteVTable[7];
int g_smokeDirectDrawPaletteSetEntriesCalls;
IDirectDrawPalette *g_smokeDirectDrawPaletteLastSetEntriesSelf;
DWORD g_smokeDirectDrawPaletteLastSetEntriesFlags;
DWORD g_smokeDirectDrawPaletteLastSetEntriesFirst;
DWORD g_smokeDirectDrawPaletteLastSetEntriesCount;
PALETTEENTRY *g_smokeDirectDrawPaletteLastSetEntriesEntries;
HRESULT g_smokeDirectDrawPaletteSetEntriesResult;
void *g_smokeDirect3DDevice2VTable[30];
HRESULT g_smokeDirect3DDevice2GetCapsResult;
int g_smokeDirect3DDevice2GetCapsCalls;
D3DDEVICEDESC *g_smokeDirect3DDevice2LastGetCapsHalDesc;
D3DDEVICEDESC *g_smokeDirect3DDevice2LastGetCapsHelDesc;
int g_smokeDirect3DDevice2AddViewportCalls;
IDirect3DViewport2 *g_smokeDirect3DDevice2LastAddViewport;
HRESULT g_smokeDirect3DDevice2AddViewportResult;
HRESULT g_smokeDirect3DDevice2BeginSceneResult;
HRESULT g_smokeDirect3DDevice2EndSceneResult;
int g_smokeDirect3DDevice2BeginSceneCalls;
int g_smokeDirect3DDevice2EndSceneCalls;
int g_smokeDirect3DDevice2SetCurrentViewportCalls;
IDirect3DViewport2 *g_smokeDirect3DDevice2LastSetCurrentViewport;
HRESULT g_smokeDirect3DDevice2SetCurrentViewportResult;
int g_smokeDirect3DDevice2SetRenderStateCalls;
D3DRENDERSTATETYPE g_smokeDirect3DDevice2RenderStates[16];
DWORD g_smokeDirect3DDevice2RenderStateValues[16];
int g_smokeDirect3DDevice2SetLightStateCalls;
D3DLIGHTSTATETYPE g_smokeDirect3DDevice2LightStates[4];
DWORD g_smokeDirect3DDevice2LightStateValues[4];
int g_smokeDirect3DDevice2DrawPrimitiveCalls;
D3DPRIMITIVETYPE g_smokeDirect3DDevice2PrimitiveTypes[8];
D3DVERTEXTYPE g_smokeDirect3DDevice2VertexTypes[8];
void *g_smokeDirect3DDevice2Vertices[8];
DWORD g_smokeDirect3DDevice2VertexCounts[8];
DWORD g_smokeDirect3DDevice2DrawFlags[8];
D3DPRIMITIVETYPE g_smokeDirect3DDevice2LastPrimitiveType;
D3DVERTEXTYPE g_smokeDirect3DDevice2LastVertexType;
void *g_smokeDirect3DDevice2LastVertices;
DWORD g_smokeDirect3DDevice2LastVertexCount;
DWORD g_smokeDirect3DDevice2LastDrawFlags;
HRESULT g_smokeDirect3DDevice2DrawPrimitiveResult;
void *g_smokeDirect3D2VTable[9];
HRESULT g_smokeDirect3D2CreateDeviceResult;
HRESULT g_smokeDirect3D2CreateViewportResult;
HRESULT g_smokeDirect3D2CreateMaterialResult;
int g_smokeDirect3D2CreateDeviceCalls;
const GUID *g_smokeDirect3D2LastCreateDeviceGuid;
IDirectDrawSurface *g_smokeDirect3D2LastCreateDeviceSurface;
IDirect3DDevice2 **g_smokeDirect3D2LastCreateDeviceOut;
IDirect3DDevice2 *g_smokeDirect3D2CreatedDevice;
int g_smokeDirect3D2CreateViewportCalls;
IDirect3DViewport2 **g_smokeDirect3D2LastCreateViewportOut;
IUnknown *g_smokeDirect3D2LastCreateViewportOuter;
IDirect3DViewport2 *g_smokeDirect3D2CreatedViewport;
int g_smokeDirect3D2CreateMaterialCalls;
IDirect3DMaterial2 **g_smokeDirect3D2LastCreateMaterialOut;
IUnknown *g_smokeDirect3D2LastCreateMaterialOuter;
IDirect3DMaterial2 *g_smokeDirect3D2CreatedMaterial;
void *g_smokeDirect3DViewport2VTable[18];
HRESULT g_smokeDirect3DViewport2SetViewport2Result;
int g_smokeDirect3DViewport2SetViewport2Calls;
D3DVIEWPORT2 g_smokeDirect3DViewport2LastViewportValue;
HRESULT g_smokeDirect3DViewport2SetBackgroundResult;
int g_smokeDirect3DViewport2SetBackgroundCalls;
D3DMATERIALHANDLE g_smokeDirect3DViewport2LastBackground;
void *g_smokeDirect3DMaterial2VTable[6];
HRESULT g_smokeDirect3DMaterial2SetMaterialResult;
int g_smokeDirect3DMaterial2SetMaterialCalls;
D3DMATERIAL g_smokeDirect3DMaterial2LastMaterialValue;
HRESULT g_smokeDirect3DMaterial2GetHandleResult;
int g_smokeDirect3DMaterial2GetHandleCalls;
IDirect3DDevice2 *g_smokeDirect3DMaterial2LastGetHandleDevice;
D3DMATERIALHANDLE *g_smokeDirect3DMaterial2LastGetHandleOut;
D3DMATERIALHANDLE g_smokeDirect3DMaterial2HandleValue;
void *g_smokeDirect3DTexture2VTable[6];
HRESULT g_smokeDirect3DTexture2LoadResult;
int g_smokeDirect3DTexture2LoadCalls;
IDirect3DTexture2 *g_smokeDirect3DTexture2LastLoadSelf;
IDirect3DTexture2 *g_smokeDirect3DTexture2LastLoadSource;
HRESULT g_smokeDirect3DTexture2GetHandleResult;
int g_smokeDirect3DTexture2GetHandleCalls;
IDirect3DTexture2 *g_smokeDirect3DTexture2LastGetHandleSelf;
IDirect3DDevice2 *g_smokeDirect3DTexture2LastGetHandleDevice;
D3DTEXTUREHANDLE *g_smokeDirect3DTexture2LastGetHandleOut;
D3DTEXTUREHANDLE g_smokeDirect3DTexture2HandleValue;
int g_smokeDirect3DTexture2ReleaseCalls;
IDirect3DTexture2 *g_smokeDirect3DTexture2ReleaseObjects[8];
int g_smokeUploadImageToSurfaceCalls;
IDirectDrawSurface *g_smokeUploadImageToSurfaceSurface;
zVidImagePartial *g_smokeUploadImageToSurfaceImage;
int g_smokeUploadImageToSurfaceUseAlpha;
int g_smokeUploadImageToSurfaceResult;

struct SmokeDirectDraw2Object {
    void **vtable;
};

struct SmokeDirectDraw1Object {
    void **vtable;
};

struct SmokeDirectDrawSurfaceObject {
    void **vtable;
};

struct SmokeComObject {
    void **vtable;
};

struct SmokeDirectDrawSurface3Object {
    void **vtable;
};

struct SmokeDirectDrawClipperObject {
    void **vtable;
};

struct SmokeSurfaceLockVerifierObject {
    void **vtable;
};

struct SmokeDirectDrawPaletteObject {
    void **vtable;
};

struct SmokeDirect3DDevice2Object {
    void **vtable;
};

struct SmokeDirect3D2Object {
    void **vtable;
};

struct SmokeDirect3DViewport2Object {
    void **vtable;
};

struct SmokeDirect3DMaterial2Object {
    void **vtable;
};

struct SmokeDirect3DTexture2Object {
    void **vtable;
};

struct SmokeImportPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct SmokeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
    int active;
};

bool PatchSmokeImportByName(
    const char *dllName,
    const char *functionName,
    void *replacement,
    SmokeImportPatch &patch
) {
    unsigned char *const imageBase = (unsigned char *)GetModuleHandleA(0);
    IMAGE_DOS_HEADER *const dos = (IMAGE_DOS_HEADER *)imageBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = (IMAGE_NT_HEADERS *)(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll = (const char *)(imageBase + descriptor->Name);
        if (lstrcmpiA(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(imageBase +
            (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                 : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *importName =
                (IMAGE_IMPORT_BY_NAME *)(imageBase + nameThunk->u1.AddressOfData);
            if (std::strcmp((const char *)importName->Name, functionName) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                patch.slot = 0;
                patch.original = 0;
                return false;
            }

            *patch.slot = (ULONG_PTR)replacement;
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

void RestoreSmokeImportPatch(
    SmokeImportPatch &patch
) {
    if (patch.slot == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.slot,
            sizeof(*patch.slot),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }
    patch.slot = 0;
    patch.original = 0;
}

bool PatchSmokeFunctionJump(
    void *target,
    void *replacement,
    SmokeFunctionPatch &patch
) {
    patch.address = (unsigned char *)(target);
    patch.active = 0;
    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        return false;
    }

    std::memcpy(patch.original, patch.address, sizeof(patch.original));
    const int relative = (int)((unsigned char *)(replacement) - patch.address - 5);
    patch.address[0] = 0xe9;
    std::memcpy(patch.address + 1, &relative, 4);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    patch.active = 1;
    return true;
}

void RestoreSmokeFunctionPatch(
    SmokeFunctionPatch &patch
) {
    if (patch.active == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.active = 0;
}

HRESULT __stdcall SmokeDirectDrawCreate(
    GUID *guid,
    IDirectDraw **outDirectDraw,
    IUnknown *outer
) {
    ++g_smokeDirectDrawCreateCalls;
    g_smokeDirectDrawCreateGuid = guid;
    g_smokeDirectDrawCreateOut = outDirectDraw;
    g_smokeDirectDrawCreateOuter = outer;
    if (g_smokeDirectDrawCreateResult == DD_OK) {
        *outDirectDraw = (IDirectDraw *)g_smokeDirectDrawQueryInterfaceSelf;
    }
    return g_smokeDirectDrawCreateResult;
}

HRESULT __stdcall SmokeDirectDrawQueryInterface(
    IDirectDraw *self,
    REFIID iid,
    void **outObject
) {
    ++g_smokeDirectDrawQueryInterfaceCalls;
    g_smokeDirectDrawQueryInterfaceSelf = self;
    g_smokeDirectDrawQueryInterfaceIid = &iid;
    g_smokeDirectDrawQueryInterfaceOut = outObject;
    if (g_smokeDirectDrawQueryInterfaceResult == DD_OK) {
        *outObject = g_smokeDirectDrawQueryInterfaceValue;
    }
    return g_smokeDirectDrawQueryInterfaceResult;
}

ULONG __stdcall SmokeDirectDrawRelease(
    IDirectDraw *self
) {
    ++g_smokeDirectDrawReleaseCalls;
    g_smokeDirectDrawReleaseSelf = self;
    return 0;
}

ULONG __stdcall SmokeDirectDraw2Release(
    IDirectDraw2 *self
) {
    ++g_smokeDirectDraw2ReleaseCalls;
    g_smokeDirectDraw2ReleaseSelf = self;
    return 0;
}

HRESULT __stdcall SmokeDirectDraw2QueryInterface(
    IDirectDraw2 *self,
    REFIID iid,
    void **outObject
) {
    ++g_smokeDirectDraw2QueryInterfaceCalls;
    g_smokeDirectDraw2LastQueryInterfaceSelf = self;
    g_smokeDirectDraw2LastQueryInterfaceIid = &iid;
    g_smokeDirectDraw2LastQueryInterfaceOut = outObject;
    if (g_smokeDirectDraw2QueryInterfaceResult == DD_OK) {
        *outObject = g_smokeDirectDraw2QueryInterfaceValue;
    }
    return g_smokeDirectDraw2QueryInterfaceResult;
}

HRESULT __stdcall SmokeDirectDraw2SetCooperativeLevel(
    IDirectDraw2 *self,
    HWND hwnd,
    DWORD flags
) {
    ++g_smokeDirectDraw2SetCooperativeLevelCalls;
    g_smokeDirectDraw2LastSetCooperativeSelf = self;
    g_smokeDirectDraw2LastSetCooperativeHwnd = hwnd;
    g_smokeDirectDraw2LastSetCooperativeFlags = flags;
    return g_smokeDirectDraw2SetCooperativeLevelResult;
}

HRESULT __stdcall SmokeDirectDraw2SetDisplayMode(
    IDirectDraw2 *self,
    DWORD width,
    DWORD height,
    DWORD bpp,
    DWORD refreshRate,
    DWORD flags
) {
    ++g_smokeDirectDraw2SetDisplayModeCalls;
    g_smokeDirectDraw2LastSetDisplayModeSelf = self;
    g_smokeDirectDraw2LastDisplayModeWidth = width;
    g_smokeDirectDraw2LastDisplayModeHeight = height;
    g_smokeDirectDraw2LastDisplayModeBpp = bpp;
    g_smokeDirectDraw2LastDisplayModeRefreshRate = refreshRate;
    g_smokeDirectDraw2LastDisplayModeFlags = flags;
    return g_smokeDirectDraw2SetDisplayModeResult;
}

HRESULT __stdcall SmokeDirectDraw2CreateSurface(
    IDirectDraw2 *self,
    DDSURFACEDESC *desc,
    IDirectDrawSurface **outSurface,
    IUnknown *outer
) {
    const int callIndex = g_smokeDirectDraw2CreateSurfaceCalls;
    ++g_smokeDirectDraw2CreateSurfaceCalls;
    g_smokeDirectDraw2LastCreateSurfaceSelf = self;
    g_smokeDirectDraw2LastCreateSurfaceDesc = desc;
    if (callIndex < 4) {
        g_smokeDirectDraw2CreateSurfaceDescs[callIndex] = *desc;
    }
    g_smokeDirectDraw2LastCreateSurfaceOut = outSurface;
    g_smokeDirectDraw2LastCreateSurfaceOuter = outer;
    if (g_smokeDirectDraw2CreateSurfaceResult == DD_OK) {
        *outSurface = g_smokeDirectDraw2CreateSurfaceValue;
    }
    if (callIndex == 0 && g_smokeDirectDraw2MutateImageOnFirstCreateSurface != 0) {
        g_smokeDirectDraw2MutateImageOnFirstCreateSurface->palette =
            g_smokeDirectDraw2MutatedPalette;
        g_smokeDirectDraw2MutateImageOnFirstCreateSurface->paletteMetaPacked =
            g_smokeDirectDraw2MutatedPaletteMetaPacked;
    }
    return g_smokeDirectDraw2CreateSurfaceResult;
}

HRESULT __stdcall SmokeDirectDraw2CreatePalette(
    IDirectDraw2 *self,
    DWORD flags,
    PALETTEENTRY *entries,
    IDirectDrawPalette **outPalette,
    IUnknown *outer
) {
    ++g_smokeDirectDraw2CreatePaletteCalls;
    g_smokeDirectDraw2LastCreatePaletteSelf = self;
    g_smokeDirectDraw2LastCreatePaletteFlags = flags;
    g_smokeDirectDraw2LastCreatePaletteEntries = entries;
    g_smokeDirectDraw2LastCreatePaletteOut = outPalette;
    g_smokeDirectDraw2LastCreatePaletteOuter = outer;
    if (g_smokeDirectDraw2CreatePaletteResult == DD_OK) {
        *outPalette = g_smokeDirectDraw2CreatePaletteValue;
    }
    return g_smokeDirectDraw2CreatePaletteResult;
}

HRESULT __stdcall SmokeDirectDrawSurfaceQueryInterface(
    IDirectDrawSurface *self,
    REFIID iid,
    void **outObject
) {
    ++g_smokeDirectDrawSurfaceQueryInterfaceCalls;
    g_smokeDirectDrawSurfaceLastQueryInterfaceSelf = self;
    g_smokeDirectDrawSurfaceLastQueryInterfaceIid = &iid;
    g_smokeDirectDrawSurfaceLastQueryInterfaceOut = outObject;
    if (g_smokeDirectDrawSurfaceQueryInterfaceResult == DD_OK) {
        if (g_smokeDirectDrawSurfaceQueryInterfaceValueCount != 0 &&
            g_smokeDirectDrawSurfaceQueryInterfaceCalls <=
                g_smokeDirectDrawSurfaceQueryInterfaceValueCount) {
            *outObject =
                g_smokeDirectDrawSurfaceQueryInterfaceValues[
                    g_smokeDirectDrawSurfaceQueryInterfaceCalls - 1
                ];
        } else {
            *outObject = g_smokeDirectDrawSurfaceQueryInterfaceValue;
        }
    }
    return g_smokeDirectDrawSurfaceQueryInterfaceResult;
}

ULONG __stdcall SmokeDirectDrawSurfaceRelease(
    IDirectDrawSurface *self
) {
    ++g_smokeDirectDrawSurfaceReleaseCalls;
    g_smokeDirectDrawSurfaceReleaseSelf = self;
    return 0;
}

HRESULT __stdcall SmokeDirectDrawSurfaceSetPalette(
    IDirectDrawSurface *self,
    IDirectDrawPalette *palette
) {
    if (g_smokeDirectDrawSurfaceSetPaletteCalls < 4) {
        g_smokeDirectDrawSurfaceSetPaletteSurfaces[
            g_smokeDirectDrawSurfaceSetPaletteCalls
        ] = self;
        g_smokeDirectDrawSurfaceSetPalettePalettes[
            g_smokeDirectDrawSurfaceSetPaletteCalls
        ] = palette;
    }
    ++g_smokeDirectDrawSurfaceSetPaletteCalls;
    return g_smokeDirectDrawSurfaceSetPaletteResult;
}

ULONG __stdcall SmokeComRelease(
    void *self
) {
    if (g_smokeComReleaseCalls < 8) {
        g_smokeComReleaseObjects[g_smokeComReleaseCalls] = self;
    }
    ++g_smokeComReleaseCalls;
    return 0;
}

ULONG __stdcall SmokeDirectDrawSurface3Release(
    IDirectDrawSurface3 *self
) {
    if (g_smokeDirectDrawSurface3ReleaseCalls < 4) {
        g_smokeDirectDrawSurface3ReleaseSurfaces[
            g_smokeDirectDrawSurface3ReleaseCalls
        ] = self;
    }
    ++g_smokeDirectDrawSurface3ReleaseCalls;
    return 0;
}

HRESULT __stdcall SmokeDirectDrawSurface3Blt(
    IDirectDrawSurface3 *self,
    RECT *dstRect,
    IDirectDrawSurface3 *srcSurface,
    RECT *srcRect,
    DWORD flags,
    DDBLTFX *fx
) {
    const int resultIndex = g_smokeDirectDrawSurface3BltCalls;
    if (g_smokeDirectDrawSurface3BltCalls < 4) {
        g_smokeDirectDrawSurface3BltSurfaces[
            g_smokeDirectDrawSurface3BltCalls
        ] = self;
        g_smokeDirectDrawSurface3BltFlags[
            g_smokeDirectDrawSurface3BltCalls
        ] = flags;
        if (fx != 0) {
            g_smokeDirectDrawSurface3BltFxValues[
                g_smokeDirectDrawSurface3BltCalls
            ] = *fx;
            g_smokeDirectDrawSurface3BltFxPresent[
                g_smokeDirectDrawSurface3BltCalls
            ] = 1;
        } else {
            g_smokeDirectDrawSurface3BltFxPresent[
                g_smokeDirectDrawSurface3BltCalls
            ] = 0;
        }
    }
    ++g_smokeDirectDrawSurface3BltCalls;
    g_smokeDirectDrawSurface3LastBltDstRectArg = dstRect;
    g_smokeDirectDrawSurface3LastBltSource = srcSurface;
    g_smokeDirectDrawSurface3LastBltSrcRectArg = srcRect;
    g_smokeDirectDrawSurface3LastBltFlags = flags;
    g_smokeDirectDrawSurface3LastBltFx = fx;
    g_smokeDirectDrawSurface3LastBltFxPresent = fx != 0 ? 1 : 0;
    if (fx != 0) {
        g_smokeDirectDrawSurface3LastBltFxValue = *fx;
    } else {
        std::memset(
            &g_smokeDirectDrawSurface3LastBltFxValue,
            0,
            sizeof(g_smokeDirectDrawSurface3LastBltFxValue)
        );
    }
    return resultIndex < g_smokeDirectDrawSurface3BltResultCount
               ? g_smokeDirectDrawSurface3BltResults[resultIndex]
               : g_smokeDirectDrawSurface3BltResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3Flip(
    IDirectDrawSurface3 *self,
    IDirectDrawSurface3 *targetOverride,
    DWORD flags
) {
    const int resultIndex = g_smokeDirectDrawSurface3FlipCalls;
    if (g_smokeDirectDrawSurface3FlipCalls < 4) {
        g_smokeDirectDrawSurface3FlipSurfaces[
            g_smokeDirectDrawSurface3FlipCalls
        ] = self;
    }
    ++g_smokeDirectDrawSurface3FlipCalls;
    g_smokeDirectDrawSurface3LastFlipTarget = targetOverride;
    g_smokeDirectDrawSurface3LastFlipFlags = flags;
    return resultIndex < g_smokeDirectDrawSurface3FlipResultCount
               ? g_smokeDirectDrawSurface3FlipResults[resultIndex]
               : DD_OK;
}

HRESULT __stdcall SmokeDirectDrawSurface3Restore(
    IDirectDrawSurface3 *self
) {
    if (g_smokeDirectDrawSurface3RestoreCalls < 4) {
        g_smokeDirectDrawSurface3RestoreSurfaces[
            g_smokeDirectDrawSurface3RestoreCalls
        ] = self;
    }
    ++g_smokeDirectDrawSurface3RestoreCalls;
    return g_smokeDirectDrawSurface3RestoreResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3GetPixelFormat(
    IDirectDrawSurface3 *self,
    DDPIXELFORMAT *pixelFormat
) {
    ++g_smokeDirectDrawSurface3GetPixelFormatCalls;
    g_smokeDirectDrawSurface3LastPixelFormatSurface = self;
    g_smokeDirectDrawSurface3LastPixelFormatInputSize = pixelFormat->dwSize;
    if (g_smokeDirectDrawSurface3GetPixelFormatResult == DD_OK) {
        *pixelFormat = g_smokeDirectDrawSurface3PixelFormat;
    }
    return g_smokeDirectDrawSurface3GetPixelFormatResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3GetAttachedSurface(
    IDirectDrawSurface3 *self,
    DDSCAPS *caps,
    IDirectDrawSurface3 **attachedSurface
) {
    ++g_smokeDirectDrawSurface3GetAttachedSurfaceCalls;
    g_smokeDirectDrawSurface3LastAttachedSurfaceSelf = self;
    g_smokeDirectDrawSurface3LastAttachedSurfaceCaps = *caps;
    g_smokeDirectDrawSurface3LastAttachedSurfaceOut = attachedSurface;
    if (g_smokeDirectDrawSurface3GetAttachedSurfaceResult == DD_OK) {
        *attachedSurface = g_smokeDirectDrawSurface3AttachedSurfaceValue;
    }
    return g_smokeDirectDrawSurface3GetAttachedSurfaceResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3AddAttachedSurface(
    IDirectDrawSurface3 *self,
    IDirectDrawSurface3 *attachedSurface
) {
    ++g_smokeDirectDrawSurface3AddAttachedSurfaceCalls;
    g_smokeDirectDrawSurface3LastAddAttachedSurfaceSelf = self;
    g_smokeDirectDrawSurface3LastAttachedSurfaceArg = attachedSurface;
    return g_smokeDirectDrawSurface3AddAttachedSurfaceResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3Lock(
    IDirectDrawSurface3 *self,
    RECT *rect,
    DDSURFACEDESC *desc,
    DWORD flags,
    HANDLE eventHandle
) {
    const int resultIndex = g_smokeDirectDrawSurface3LockCalls;
    if (g_smokeDirectDrawSurface3LockCalls < 4) {
        g_smokeDirectDrawSurface3LockSurfaces[
            g_smokeDirectDrawSurface3LockCalls
        ] = self;
    }
    ++g_smokeDirectDrawSurface3LockCalls;
    g_smokeDirectDrawSurface3LastLockRect = rect;
    g_smokeDirectDrawSurface3LastLockDesc = desc;
    g_smokeDirectDrawSurface3LastLockFlags = flags;
    g_smokeDirectDrawSurface3LastLockEvent = eventHandle;
    g_smokeDirectDrawSurface3LockDescSize = desc->dwSize;
    const HRESULT result =
        resultIndex < g_smokeDirectDrawSurface3LockResultCount
            ? g_smokeDirectDrawSurface3LockResults[resultIndex]
            : DD_OK;
    if (result == DD_OK) {
        desc->dwWidth = 640;
        desc->dwHeight = 480;
        desc->lPitch = g_smokeDirectDrawSurface3LockPitchValue;
        desc->lpSurface = g_smokeDirectDrawSurface3LockPixelsValue;
    }
    return result;
}

HRESULT __stdcall SmokeDirectDrawSurface3GetDC(
    IDirectDrawSurface3 *self,
    HDC *hdc
) {
    ++g_smokeDirectDrawSurface3GetDCCalls;
    g_smokeDirectDrawSurface3LastGetDCSurface = self;
    g_smokeDirectDrawSurface3LastGetDCOut = hdc;
    if (g_smokeDirectDrawSurface3GetDCResult == DD_OK) {
        *hdc = g_smokeDirectDrawSurface3GetDCValue;
    }
    return g_smokeDirectDrawSurface3GetDCResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3ReleaseDC(
    IDirectDrawSurface3 *self,
    HDC hdc
) {
    ++g_smokeDirectDrawSurface3ReleaseDCCalls;
    g_smokeDirectDrawSurface3LastReleaseDCSurface = self;
    g_smokeDirectDrawSurface3LastReleaseDCHdc = hdc;
    return g_smokeDirectDrawSurface3ReleaseDCResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3Unlock(
    IDirectDrawSurface3 *self,
    void *surfaceData
) {
    const int resultIndex = g_smokeDirectDrawSurface3UnlockCalls;
    if (g_smokeDirectDrawSurface3UnlockCalls < 4) {
        g_smokeDirectDrawSurface3UnlockSurfaces[
            g_smokeDirectDrawSurface3UnlockCalls
        ] = self;
    }
    ++g_smokeDirectDrawSurface3UnlockCalls;
    g_smokeDirectDrawSurface3LastUnlockSurface = self;
    g_smokeDirectDrawSurface3LastUnlockArg = surfaceData;
    return resultIndex < g_smokeDirectDrawSurface3UnlockResultCount
               ? g_smokeDirectDrawSurface3UnlockResults[resultIndex]
               : DD_OK;
}

HRESULT __stdcall SmokeDirectDrawSurface3SetClipper(
    IDirectDrawSurface3 *self,
    IDirectDrawClipper *clipper
) {
    ++g_smokeDirectDrawSurface3SetClipperCalls;
    g_smokeDirectDrawSurface3LastSetClipperSelf = self;
    g_smokeDirectDrawSurface3LastSetClipperValue = clipper;
    return g_smokeDirectDrawSurface3SetClipperResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3PageLock(
    IDirectDrawSurface3 *self,
    DWORD flags
) {
    ++g_smokeDirectDrawSurface3PageLockCalls;
    g_smokeDirectDrawSurface3LastPageLockSurface = self;
    g_smokeDirectDrawSurface3LastPageLockFlags = flags;
    return g_smokeDirectDrawSurface3PageLockResult;
}

HRESULT __stdcall SmokeDirectDrawSurface3PageUnlock(
    IDirectDrawSurface3 *self,
    DWORD flags
) {
    ++g_smokeDirectDrawSurface3PageUnlockCalls;
    g_smokeDirectDrawSurface3LastPageUnlockSurface = self;
    g_smokeDirectDrawSurface3LastPageUnlockFlags = flags;
    return g_smokeDirectDrawSurface3PageUnlockResult;
}

ULONG __stdcall SmokeSurfaceLockVerifierRelease(
    zVideo_SurfaceLockVerifier *
) {
    ++g_smokeSurfaceLockVerifierReleaseCalls;
    return 0;
}

HRESULT __stdcall SmokeSurfaceLockVerifierVerify(
    zVideo_SurfaceLockVerifier *,
    zVideo_SurfaceLockVerifyArgs *args
) {
    ++g_smokeSurfaceLockVerifierVerifyCalls;
    g_smokeSurfaceLockVerifierLastArgs = *args;
    return DD_OK;
}

HRESULT __stdcall SmokeDirectDraw2GetAvailableVidMem(
    IDirectDraw2 *,
    DDSCAPS *caps,
    DWORD *totalBytes,
    DWORD *freeBytes
) {
    ++g_smokeDirectDraw2GetAvailableVidMemCalls;
    g_smokeDirectDraw2LastAvailableVidMemCaps = *caps;
    g_smokeDirectDraw2LastAvailableVidMemTotal = totalBytes;
    g_smokeDirectDraw2LastAvailableVidMemFree = freeBytes;
    if (g_smokeDirectDraw2GetAvailableVidMemResult == DD_OK) {
        *totalBytes = g_smokeDirectDraw2AvailableVidMemTotal;
        *freeBytes = g_smokeDirectDraw2AvailableVidMemFree;
    }
    return g_smokeDirectDraw2GetAvailableVidMemResult;
}

HRESULT __stdcall SmokeDirectDraw2CreateClipper(
    IDirectDraw2 *self,
    DWORD flags,
    IDirectDrawClipper **clipper,
    IUnknown *outer
) {
    ++g_smokeDirectDraw2CreateClipperCalls;
    g_smokeDirectDraw2LastCreateClipperSelf = self;
    g_smokeDirectDraw2LastCreateClipperFlags = flags;
    g_smokeDirectDraw2LastCreateClipperOut = clipper;
    g_smokeDirectDraw2LastCreateClipperOuter = outer;
    if (g_smokeDirectDraw2CreateClipperResult == DD_OK) {
        *clipper = g_smokeDirectDraw2CreateClipperValue;
    }
    return g_smokeDirectDraw2CreateClipperResult;
}

HRESULT __stdcall SmokeDirectDrawClipperSetHWnd(
    IDirectDrawClipper *self,
    DWORD flags,
    HWND hwnd
) {
    ++g_smokeDirectDrawClipperSetHWndCalls;
    g_smokeDirectDrawClipperLastSetHWndSelf = self;
    g_smokeDirectDrawClipperLastSetHWndFlags = flags;
    g_smokeDirectDrawClipperLastSetHWndValue = hwnd;
    return g_smokeDirectDrawClipperSetHWndResult;
}

HRESULT __stdcall SmokeDirectDrawPaletteSetEntries(
    IDirectDrawPalette *self,
    DWORD flags,
    DWORD firstEntry,
    DWORD entryCount,
    PALETTEENTRY *entries
) {
    ++g_smokeDirectDrawPaletteSetEntriesCalls;
    g_smokeDirectDrawPaletteLastSetEntriesSelf = self;
    g_smokeDirectDrawPaletteLastSetEntriesFlags = flags;
    g_smokeDirectDrawPaletteLastSetEntriesFirst = firstEntry;
    g_smokeDirectDrawPaletteLastSetEntriesCount = entryCount;
    g_smokeDirectDrawPaletteLastSetEntriesEntries = entries;
    return g_smokeDirectDrawPaletteSetEntriesResult;
}

HRESULT __stdcall SmokeDirect3DDevice2BeginScene(
    IDirect3DDevice2 *
) {
    ++g_smokeDirect3DDevice2BeginSceneCalls;
    return g_smokeDirect3DDevice2BeginSceneResult;
}

HRESULT __stdcall SmokeDirect3DDevice2EndScene(
    IDirect3DDevice2 *
) {
    ++g_smokeDirect3DDevice2EndSceneCalls;
    return g_smokeDirect3DDevice2EndSceneResult;
}

HRESULT __stdcall SmokeDirect3DDevice2SetRenderState(
    IDirect3DDevice2 *,
    D3DRENDERSTATETYPE renderState,
    DWORD value
) {
    if (g_smokeDirect3DDevice2SetRenderStateCalls < 16) {
        g_smokeDirect3DDevice2RenderStates[
            g_smokeDirect3DDevice2SetRenderStateCalls
        ] = renderState;
        g_smokeDirect3DDevice2RenderStateValues[
            g_smokeDirect3DDevice2SetRenderStateCalls
        ] = value;
    }
    ++g_smokeDirect3DDevice2SetRenderStateCalls;
    return DD_OK;
}

HRESULT __stdcall SmokeDirect3DDevice2SetLightState(
    IDirect3DDevice2 *,
    D3DLIGHTSTATETYPE lightState,
    DWORD value
) {
    if (g_smokeDirect3DDevice2SetLightStateCalls < 4) {
        g_smokeDirect3DDevice2LightStates[
            g_smokeDirect3DDevice2SetLightStateCalls
        ] = lightState;
        g_smokeDirect3DDevice2LightStateValues[
            g_smokeDirect3DDevice2SetLightStateCalls
        ] = value;
    }
    ++g_smokeDirect3DDevice2SetLightStateCalls;
    return DD_OK;
}

HRESULT __stdcall SmokeDirect3DDevice2DrawPrimitive(
    IDirect3DDevice2 *,
    D3DPRIMITIVETYPE primitiveType,
    D3DVERTEXTYPE vertexType,
    void *vertices,
    DWORD vertexCount,
    DWORD flags
) {
    if (g_smokeDirect3DDevice2DrawPrimitiveCalls < 8) {
        g_smokeDirect3DDevice2PrimitiveTypes[
            g_smokeDirect3DDevice2DrawPrimitiveCalls
        ] = primitiveType;
        g_smokeDirect3DDevice2VertexTypes[
            g_smokeDirect3DDevice2DrawPrimitiveCalls
        ] = vertexType;
        g_smokeDirect3DDevice2Vertices[
            g_smokeDirect3DDevice2DrawPrimitiveCalls
        ] = vertices;
        g_smokeDirect3DDevice2VertexCounts[
            g_smokeDirect3DDevice2DrawPrimitiveCalls
        ] = vertexCount;
        g_smokeDirect3DDevice2DrawFlags[
            g_smokeDirect3DDevice2DrawPrimitiveCalls
        ] = flags;
    }
    ++g_smokeDirect3DDevice2DrawPrimitiveCalls;
    g_smokeDirect3DDevice2LastPrimitiveType = primitiveType;
    g_smokeDirect3DDevice2LastVertexType = vertexType;
    g_smokeDirect3DDevice2LastVertices = vertices;
    g_smokeDirect3DDevice2LastVertexCount = vertexCount;
    g_smokeDirect3DDevice2LastDrawFlags = flags;
    return g_smokeDirect3DDevice2DrawPrimitiveResult;
}

HRESULT __stdcall SmokeDirect3DDevice2GetCaps(
    IDirect3DDevice2 *,
    D3DDEVICEDESC *halDesc,
    D3DDEVICEDESC *helDesc
) {
    ++g_smokeDirect3DDevice2GetCapsCalls;
    g_smokeDirect3DDevice2LastGetCapsHalDesc = halDesc;
    g_smokeDirect3DDevice2LastGetCapsHelDesc = helDesc;
    return g_smokeDirect3DDevice2GetCapsResult;
}

HRESULT __stdcall SmokeDirect3DDevice2AddViewport(
    IDirect3DDevice2 *,
    IDirect3DViewport2 *viewport
) {
    ++g_smokeDirect3DDevice2AddViewportCalls;
    g_smokeDirect3DDevice2LastAddViewport = viewport;
    return g_smokeDirect3DDevice2AddViewportResult;
}

HRESULT __stdcall SmokeDirect3DDevice2SetCurrentViewport(
    IDirect3DDevice2 *,
    IDirect3DViewport2 *viewport
) {
    ++g_smokeDirect3DDevice2SetCurrentViewportCalls;
    g_smokeDirect3DDevice2LastSetCurrentViewport = viewport;
    return g_smokeDirect3DDevice2SetCurrentViewportResult;
}

HRESULT __stdcall SmokeDirect3D2CreateDevice(
    IDirect3D2 *,
    REFCLSID deviceGuid,
    IDirectDrawSurface *renderTarget,
    IDirect3DDevice2 **outDevice
) {
    ++g_smokeDirect3D2CreateDeviceCalls;
    g_smokeDirect3D2LastCreateDeviceGuid = &deviceGuid;
    g_smokeDirect3D2LastCreateDeviceSurface = renderTarget;
    g_smokeDirect3D2LastCreateDeviceOut = outDevice;
    if (g_smokeDirect3D2CreateDeviceResult == DD_OK) {
        *outDevice = g_smokeDirect3D2CreatedDevice;
    }
    return g_smokeDirect3D2CreateDeviceResult;
}

HRESULT __stdcall SmokeDirect3D2CreateViewport(
    IDirect3D2 *,
    IDirect3DViewport2 **outViewport,
    IUnknown *outer
) {
    ++g_smokeDirect3D2CreateViewportCalls;
    g_smokeDirect3D2LastCreateViewportOut = outViewport;
    g_smokeDirect3D2LastCreateViewportOuter = outer;
    if (g_smokeDirect3D2CreateViewportResult == DD_OK) {
        *outViewport = g_smokeDirect3D2CreatedViewport;
    }
    return g_smokeDirect3D2CreateViewportResult;
}

HRESULT __stdcall SmokeDirect3D2CreateMaterial(
    IDirect3D2 *,
    IDirect3DMaterial2 **outMaterial,
    IUnknown *outer
) {
    ++g_smokeDirect3D2CreateMaterialCalls;
    g_smokeDirect3D2LastCreateMaterialOut = outMaterial;
    g_smokeDirect3D2LastCreateMaterialOuter = outer;
    if (g_smokeDirect3D2CreateMaterialResult == DD_OK) {
        *outMaterial = g_smokeDirect3D2CreatedMaterial;
    }
    return g_smokeDirect3D2CreateMaterialResult;
}

HRESULT __stdcall SmokeDirect3DViewport2SetViewport2(
    IDirect3DViewport2 *,
    D3DVIEWPORT2 *viewport
) {
    ++g_smokeDirect3DViewport2SetViewport2Calls;
    if (viewport != 0) {
        g_smokeDirect3DViewport2LastViewportValue = *viewport;
    }
    return g_smokeDirect3DViewport2SetViewport2Result;
}

HRESULT __stdcall SmokeDirect3DViewport2SetBackground(
    IDirect3DViewport2 *,
    D3DMATERIALHANDLE handle
) {
    ++g_smokeDirect3DViewport2SetBackgroundCalls;
    g_smokeDirect3DViewport2LastBackground = handle;
    return g_smokeDirect3DViewport2SetBackgroundResult;
}

HRESULT __stdcall SmokeDirect3DMaterial2SetMaterial(
    IDirect3DMaterial2 *,
    D3DMATERIAL *material
) {
    ++g_smokeDirect3DMaterial2SetMaterialCalls;
    if (material != 0) {
        g_smokeDirect3DMaterial2LastMaterialValue = *material;
    }
    return g_smokeDirect3DMaterial2SetMaterialResult;
}

HRESULT __stdcall SmokeDirect3DMaterial2GetHandle(
    IDirect3DMaterial2 *,
    IDirect3DDevice2 *device,
    D3DMATERIALHANDLE *outHandle
) {
    ++g_smokeDirect3DMaterial2GetHandleCalls;
    g_smokeDirect3DMaterial2LastGetHandleDevice = device;
    g_smokeDirect3DMaterial2LastGetHandleOut = outHandle;
    if (g_smokeDirect3DMaterial2GetHandleResult == DD_OK) {
        *outHandle = g_smokeDirect3DMaterial2HandleValue;
    }
    return g_smokeDirect3DMaterial2GetHandleResult;
}

ULONG __stdcall SmokeDirect3DTexture2Release(
    IDirect3DTexture2 *texture
) {
    if (g_smokeDirect3DTexture2ReleaseCalls < 8) {
        g_smokeDirect3DTexture2ReleaseObjects[
            g_smokeDirect3DTexture2ReleaseCalls
        ] = texture;
    }
    ++g_smokeDirect3DTexture2ReleaseCalls;
    return 1;
}

HRESULT __stdcall SmokeDirect3DTexture2Load(
    IDirect3DTexture2 *texture,
    IDirect3DTexture2 *sourceTexture
) {
    ++g_smokeDirect3DTexture2LoadCalls;
    g_smokeDirect3DTexture2LastLoadSelf = texture;
    g_smokeDirect3DTexture2LastLoadSource = sourceTexture;
    return g_smokeDirect3DTexture2LoadResult;
}

HRESULT __stdcall SmokeDirect3DTexture2GetHandle(
    IDirect3DTexture2 *texture,
    IDirect3DDevice2 *device,
    D3DTEXTUREHANDLE *outHandle
) {
    ++g_smokeDirect3DTexture2GetHandleCalls;
    g_smokeDirect3DTexture2LastGetHandleSelf = texture;
    g_smokeDirect3DTexture2LastGetHandleDevice = device;
    g_smokeDirect3DTexture2LastGetHandleOut = outHandle;
    if (g_smokeDirect3DTexture2GetHandleResult == DD_OK) {
        *outHandle = g_smokeDirect3DTexture2HandleValue;
    }
    return g_smokeDirect3DTexture2GetHandleResult;
}

int __fastcall SmokeUploadImageToSurface(
    IDirectDrawSurface *uploadSurface,
    zVidImagePartial *image,
    int useAlpha
) {
    ++g_smokeUploadImageToSurfaceCalls;
    g_smokeUploadImageToSurfaceSurface = uploadSurface;
    g_smokeUploadImageToSurfaceImage = image;
    g_smokeUploadImageToSurfaceUseAlpha = useAlpha;
    return g_smokeUploadImageToSurfaceResult;
}

void InstallSmokeComReleaseObject(
    SmokeComObject &object
) {
    std::memset(g_smokeComReleaseVTable, 0, sizeof(g_smokeComReleaseVTable));
    g_smokeComReleaseVTable[2] = (void *)(&SmokeComRelease);
    object.vtable = g_smokeComReleaseVTable;
}

void ResetSmokeComReleaseTracking() {
    g_smokeComReleaseCalls = 0;
    std::memset(g_smokeComReleaseObjects, 0, sizeof(g_smokeComReleaseObjects));
}

void InstallSmokeDirectDraw2(SmokeDirectDraw2Object &directDraw) {
    std::memset(g_smokeDirectDraw2VTable, 0, sizeof(g_smokeDirectDraw2VTable));
    g_smokeDirectDraw2VTable[0] = (void *)(&SmokeDirectDraw2QueryInterface);
    g_smokeDirectDraw2VTable[2] = (void *)(&SmokeDirectDraw2Release);
    g_smokeDirectDraw2VTable[4] = (void *)(&SmokeDirectDraw2CreateClipper);
    g_smokeDirectDraw2VTable[5] = (void *)(&SmokeDirectDraw2CreatePalette);
    g_smokeDirectDraw2VTable[6] = (void *)(&SmokeDirectDraw2CreateSurface);
    g_smokeDirectDraw2VTable[8] = (void *)(&SmokeDirectDraw2CreateClipper);
    g_smokeDirectDraw2VTable[20] = (void *)(&SmokeDirectDraw2SetCooperativeLevel);
    g_smokeDirectDraw2VTable[21] = (void *)(&SmokeDirectDraw2SetDisplayMode);
    g_smokeDirectDraw2VTable[23] = (void *)(&SmokeDirectDraw2GetAvailableVidMem);
    directDraw.vtable = g_smokeDirectDraw2VTable;
    g_smokeDirectDraw2QueryInterfaceCalls = 0;
    g_smokeDirectDraw2LastQueryInterfaceSelf = 0;
    g_smokeDirectDraw2LastQueryInterfaceIid = 0;
    g_smokeDirectDraw2LastQueryInterfaceOut = 0;
    g_smokeDirectDraw2QueryInterfaceResult = DD_OK;
    g_smokeDirectDraw2QueryInterfaceValue = 0;
    g_smokeDirectDraw2GetAvailableVidMemCalls = 0;
    g_smokeDirectDraw2ReleaseCalls = 0;
    g_smokeDirectDraw2ReleaseSelf = 0;
    g_smokeDirectDraw2SetCooperativeLevelCalls = 0;
    g_smokeDirectDraw2LastSetCooperativeSelf = 0;
    g_smokeDirectDraw2LastSetCooperativeHwnd = 0;
    g_smokeDirectDraw2LastSetCooperativeFlags = 0;
    g_smokeDirectDraw2SetCooperativeLevelResult = DD_OK;
    g_smokeDirectDraw2SetDisplayModeCalls = 0;
    g_smokeDirectDraw2LastSetDisplayModeSelf = 0;
    g_smokeDirectDraw2LastDisplayModeWidth = 0;
    g_smokeDirectDraw2LastDisplayModeHeight = 0;
    g_smokeDirectDraw2LastDisplayModeBpp = 0;
    g_smokeDirectDraw2LastDisplayModeRefreshRate = 0;
    g_smokeDirectDraw2LastDisplayModeFlags = 0;
    g_smokeDirectDraw2SetDisplayModeResult = DD_OK;
    g_smokeDirectDraw2CreateSurfaceCalls = 0;
    g_smokeDirectDraw2LastCreateSurfaceSelf = 0;
    g_smokeDirectDraw2LastCreateSurfaceDesc = 0;
    std::memset(
        g_smokeDirectDraw2CreateSurfaceDescs,
        0,
        sizeof(g_smokeDirectDraw2CreateSurfaceDescs)
    );
    g_smokeDirectDraw2LastCreateSurfaceOut = 0;
    g_smokeDirectDraw2LastCreateSurfaceOuter = 0;
    g_smokeDirectDraw2CreateSurfaceResult = DD_OK;
    g_smokeDirectDraw2CreateSurfaceValue = 0;
    g_smokeDirectDraw2MutateImageOnFirstCreateSurface = 0;
    g_smokeDirectDraw2MutatedPalette = 0;
    g_smokeDirectDraw2MutatedPaletteMetaPacked = 0;
    g_smokeDirectDraw2CreatePaletteCalls = 0;
    g_smokeDirectDraw2LastCreatePaletteSelf = 0;
    g_smokeDirectDraw2LastCreatePaletteFlags = 0;
    g_smokeDirectDraw2LastCreatePaletteEntries = 0;
    g_smokeDirectDraw2LastCreatePaletteOut = 0;
    g_smokeDirectDraw2LastCreatePaletteOuter = 0;
    g_smokeDirectDraw2CreatePaletteResult = DD_OK;
    g_smokeDirectDraw2CreatePaletteValue = 0;
    g_smokeDirectDraw2CreateClipperCalls = 0;
    g_smokeDirectDraw2LastCreateClipperSelf = 0;
    g_smokeDirectDraw2LastCreateClipperFlags = 0;
    g_smokeDirectDraw2LastCreateClipperOut = 0;
    g_smokeDirectDraw2LastCreateClipperOuter = 0;
    g_smokeDirectDraw2CreateClipperResult = DD_OK;
    g_smokeDirectDraw2CreateClipperValue = 0;
    g_smokeDirectDraw2LastAvailableVidMemCaps = {};
    g_smokeDirectDraw2LastAvailableVidMemTotal = 0;
    g_smokeDirectDraw2LastAvailableVidMemFree = 0;
    g_smokeDirectDraw2AvailableVidMemTotal = 0;
    g_smokeDirectDraw2AvailableVidMemFree = 0;
    g_smokeDirectDraw2GetAvailableVidMemResult = DD_OK;
}

void InstallSmokeDirectDrawSurface(SmokeDirectDrawSurfaceObject &surface) {
    std::memset(
        g_smokeDirectDrawSurfaceVTable,
        0,
        sizeof(g_smokeDirectDrawSurfaceVTable)
    );
    g_smokeDirectDrawSurfaceVTable[0] =
        (void *)(&SmokeDirectDrawSurfaceQueryInterface);
    g_smokeDirectDrawSurfaceVTable[2] = (void *)(&SmokeDirectDrawSurfaceRelease);
    g_smokeDirectDrawSurfaceVTable[31] =
        (void *)(&SmokeDirectDrawSurfaceSetPalette);
    surface.vtable = g_smokeDirectDrawSurfaceVTable;
    g_smokeDirectDrawSurfaceQueryInterfaceCalls = 0;
    g_smokeDirectDrawSurfaceLastQueryInterfaceSelf = 0;
    g_smokeDirectDrawSurfaceLastQueryInterfaceIid = 0;
    g_smokeDirectDrawSurfaceLastQueryInterfaceOut = 0;
    g_smokeDirectDrawSurfaceQueryInterfaceResult = DD_OK;
    g_smokeDirectDrawSurfaceQueryInterfaceValue = 0;
    std::memset(
        g_smokeDirectDrawSurfaceQueryInterfaceValues,
        0,
        sizeof(g_smokeDirectDrawSurfaceQueryInterfaceValues)
    );
    g_smokeDirectDrawSurfaceQueryInterfaceValueCount = 0;
    g_smokeDirectDrawSurfaceReleaseCalls = 0;
    g_smokeDirectDrawSurfaceReleaseSelf = 0;
    g_smokeDirectDrawSurfaceSetPaletteCalls = 0;
    std::memset(
        g_smokeDirectDrawSurfaceSetPaletteSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurfaceSetPaletteSurfaces)
    );
    std::memset(
        g_smokeDirectDrawSurfaceSetPalettePalettes,
        0,
        sizeof(g_smokeDirectDrawSurfaceSetPalettePalettes)
    );
    g_smokeDirectDrawSurfaceSetPaletteResult = DD_OK;
}

void InstallSmokeDirectDrawSurface3(SmokeDirectDrawSurface3Object &surface) {
    std::memset(
        g_smokeDirectDrawSurface3VTable,
        0,
        sizeof(g_smokeDirectDrawSurface3VTable)
    );
    g_smokeDirectDrawSurface3VTable[2] = (void *)(&SmokeDirectDrawSurface3Release);
    g_smokeDirectDrawSurface3VTable[3] =
        (void *)(&SmokeDirectDrawSurface3AddAttachedSurface);
    g_smokeDirectDrawSurface3VTable[5] = (void *)(&SmokeDirectDrawSurface3Blt);
    g_smokeDirectDrawSurface3VTable[11] = (void *)(&SmokeDirectDrawSurface3Flip);
    g_smokeDirectDrawSurface3VTable[12] =
        (void *)(&SmokeDirectDrawSurface3GetAttachedSurface);
    g_smokeDirectDrawSurface3VTable[17] = (void *)(&SmokeDirectDrawSurface3GetDC);
    g_smokeDirectDrawSurface3VTable[21] =
        (void *)(&SmokeDirectDrawSurface3GetPixelFormat);
    g_smokeDirectDrawSurface3VTable[25] = (void *)(&SmokeDirectDrawSurface3Lock);
    g_smokeDirectDrawSurface3VTable[26] = (void *)(&SmokeDirectDrawSurface3ReleaseDC);
    g_smokeDirectDrawSurface3VTable[27] = (void *)(&SmokeDirectDrawSurface3Restore);
    g_smokeDirectDrawSurface3VTable[28] = (void *)(&SmokeDirectDrawSurface3SetClipper);
    g_smokeDirectDrawSurface3VTable[32] = (void *)(&SmokeDirectDrawSurface3Unlock);
    g_smokeDirectDrawSurface3VTable[37] =
        (void *)(&SmokeDirectDrawSurface3PageLock);
    g_smokeDirectDrawSurface3VTable[38] =
        (void *)(&SmokeDirectDrawSurface3PageUnlock);
    surface.vtable = g_smokeDirectDrawSurface3VTable;
    g_smokeDirectDrawSurface3BltCalls = 0;
    std::memset(
        g_smokeDirectDrawSurface3BltSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurface3BltSurfaces)
    );
    std::memset(
        g_smokeDirectDrawSurface3BltFlags,
        0,
        sizeof(g_smokeDirectDrawSurface3BltFlags)
    );
    std::memset(
        g_smokeDirectDrawSurface3BltFxValues,
        0,
        sizeof(g_smokeDirectDrawSurface3BltFxValues)
    );
    std::memset(
        g_smokeDirectDrawSurface3BltFxPresent,
        0,
        sizeof(g_smokeDirectDrawSurface3BltFxPresent)
    );
    g_smokeDirectDrawSurface3LastBltDstRectArg = 0;
    g_smokeDirectDrawSurface3LastBltSource = 0;
    g_smokeDirectDrawSurface3LastBltSrcRectArg = 0;
    g_smokeDirectDrawSurface3LastBltFlags = 0;
    g_smokeDirectDrawSurface3LastBltFx = 0;
    std::memset(
        &g_smokeDirectDrawSurface3LastBltFxValue,
        0,
        sizeof(g_smokeDirectDrawSurface3LastBltFxValue)
    );
    g_smokeDirectDrawSurface3LastBltFxPresent = 0;
    g_smokeDirectDrawSurface3BltResult = DD_OK;
    std::memset(
        g_smokeDirectDrawSurface3BltResults,
        0,
        sizeof(g_smokeDirectDrawSurface3BltResults)
    );
    g_smokeDirectDrawSurface3BltResultCount = 0;
    g_smokeDirectDrawSurface3FlipCalls = 0;
    std::memset(
        g_smokeDirectDrawSurface3FlipSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurface3FlipSurfaces)
    );
    g_smokeDirectDrawSurface3LastFlipTarget = 0;
    g_smokeDirectDrawSurface3LastFlipFlags = 0;
    std::memset(
        g_smokeDirectDrawSurface3FlipResults,
        0,
        sizeof(g_smokeDirectDrawSurface3FlipResults)
    );
    g_smokeDirectDrawSurface3FlipResultCount = 0;
    g_smokeDirectDrawSurface3ReleaseCalls = 0;
    std::memset(
        g_smokeDirectDrawSurface3ReleaseSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurface3ReleaseSurfaces)
    );
    g_smokeDirectDrawSurface3RestoreCalls = 0;
    std::memset(
        g_smokeDirectDrawSurface3RestoreSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurface3RestoreSurfaces)
    );
    g_smokeDirectDrawSurface3RestoreResult = DD_OK;
    g_smokeDirectDrawSurface3GetPixelFormatCalls = 0;
    g_smokeDirectDrawSurface3LastPixelFormatSurface = 0;
    g_smokeDirectDrawSurface3LastPixelFormatInputSize = 0;
    std::memset(
        &g_smokeDirectDrawSurface3PixelFormat,
        0,
        sizeof(g_smokeDirectDrawSurface3PixelFormat)
    );
    g_smokeDirectDrawSurface3GetPixelFormatResult = DD_OK;
    g_smokeDirectDrawSurface3GetAttachedSurfaceCalls = 0;
    g_smokeDirectDrawSurface3LastAttachedSurfaceSelf = 0;
    g_smokeDirectDrawSurface3LastAttachedSurfaceCaps = {};
    g_smokeDirectDrawSurface3LastAttachedSurfaceOut = 0;
    g_smokeDirectDrawSurface3GetAttachedSurfaceResult = DD_OK;
    g_smokeDirectDrawSurface3AttachedSurfaceValue = 0;
    g_smokeDirectDrawSurface3LockCalls = 0;
    std::memset(
        g_smokeDirectDrawSurface3LockSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurface3LockSurfaces)
    );
    g_smokeDirectDrawSurface3LastLockRect = 0;
    g_smokeDirectDrawSurface3LastLockDesc = 0;
    g_smokeDirectDrawSurface3LastLockFlags = 0;
    g_smokeDirectDrawSurface3LastLockEvent = 0;
    g_smokeDirectDrawSurface3LockDescSize = 0;
    std::memset(
        g_smokeDirectDrawSurface3LockPixels,
        0x5a,
        sizeof(g_smokeDirectDrawSurface3LockPixels)
    );
    g_smokeDirectDrawSurface3LockPixelsValue = g_smokeDirectDrawSurface3LockPixels;
    g_smokeDirectDrawSurface3LockPitchValue = 1280;
    std::memset(
        g_smokeDirectDrawSurface3LockResults,
        0,
        sizeof(g_smokeDirectDrawSurface3LockResults)
    );
    g_smokeDirectDrawSurface3LockResultCount = 0;
    g_smokeDirectDrawSurface3UnlockCalls = 0;
    std::memset(
        g_smokeDirectDrawSurface3UnlockSurfaces,
        0,
        sizeof(g_smokeDirectDrawSurface3UnlockSurfaces)
    );
    g_smokeDirectDrawSurface3LastUnlockSurface = 0;
    g_smokeDirectDrawSurface3LastUnlockArg = 0;
    std::memset(
        g_smokeDirectDrawSurface3UnlockResults,
        0,
        sizeof(g_smokeDirectDrawSurface3UnlockResults)
    );
    g_smokeDirectDrawSurface3UnlockResultCount = 0;
    g_smokeDirectDrawSurface3SetClipperCalls = 0;
    g_smokeDirectDrawSurface3LastSetClipperSelf = 0;
    g_smokeDirectDrawSurface3LastSetClipperValue = 0;
    g_smokeDirectDrawSurface3SetClipperResult = DD_OK;
    g_smokeDirectDrawSurface3AddAttachedSurfaceCalls = 0;
    g_smokeDirectDrawSurface3LastAddAttachedSurfaceSelf = 0;
    g_smokeDirectDrawSurface3LastAttachedSurfaceArg = 0;
    g_smokeDirectDrawSurface3AddAttachedSurfaceResult = DD_OK;
    g_smokeDirectDrawSurface3PageLockCalls = 0;
    g_smokeDirectDrawSurface3LastPageLockSurface = 0;
    g_smokeDirectDrawSurface3LastPageLockFlags = 0;
    g_smokeDirectDrawSurface3PageLockResult = DD_OK;
    g_smokeDirectDrawSurface3PageUnlockCalls = 0;
    g_smokeDirectDrawSurface3LastPageUnlockSurface = 0;
    g_smokeDirectDrawSurface3LastPageUnlockFlags = 0;
    g_smokeDirectDrawSurface3PageUnlockResult = DD_OK;
    g_smokeDirectDrawSurface3GetDCCalls = 0;
    g_smokeDirectDrawSurface3LastGetDCSurface = 0;
    g_smokeDirectDrawSurface3LastGetDCOut = 0;
    g_smokeDirectDrawSurface3GetDCValue = (HDC)(0x5a5a);
    g_smokeDirectDrawSurface3GetDCResult = DD_OK;
    g_smokeDirectDrawSurface3ReleaseDCCalls = 0;
    g_smokeDirectDrawSurface3LastReleaseDCSurface = 0;
    g_smokeDirectDrawSurface3LastReleaseDCHdc = 0;
    g_smokeDirectDrawSurface3ReleaseDCResult = DD_OK;
}

void InstallSmokeDirectDrawClipper(
    SmokeDirectDrawClipperObject &clipper
) {
    std::memset(
        g_smokeDirectDrawClipperVTable,
        0,
        sizeof(g_smokeDirectDrawClipperVTable)
    );
    g_smokeDirectDrawClipperVTable[4] =
        (void *)(&SmokeDirectDrawClipperSetHWnd);
    g_smokeDirectDrawClipperVTable[8] =
        (void *)(&SmokeDirectDrawClipperSetHWnd);
    clipper.vtable = g_smokeDirectDrawClipperVTable;
    g_smokeDirectDrawClipperSetHWndCalls = 0;
    g_smokeDirectDrawClipperLastSetHWndSelf = 0;
    g_smokeDirectDrawClipperLastSetHWndFlags = 0;
    g_smokeDirectDrawClipperLastSetHWndValue = 0;
    g_smokeDirectDrawClipperSetHWndResult = DD_OK;
}

void InstallSmokeSurfaceLockVerifier(
    SmokeSurfaceLockVerifierObject &verifier
) {
    std::memset(
        g_smokeSurfaceLockVerifierVTable,
        0,
        sizeof(g_smokeSurfaceLockVerifierVTable)
    );
    g_smokeSurfaceLockVerifierVTable[2] = (void *)(&SmokeSurfaceLockVerifierRelease);
    g_smokeSurfaceLockVerifierVTable[4] = (void *)(&SmokeSurfaceLockVerifierVerify);
    verifier.vtable = g_smokeSurfaceLockVerifierVTable;
    g_smokeSurfaceLockVerifierReleaseCalls = 0;
    g_smokeSurfaceLockVerifierVerifyCalls = 0;
    std::memset(
        &g_smokeSurfaceLockVerifierLastArgs,
        0,
        sizeof(g_smokeSurfaceLockVerifierLastArgs)
    );
}

void InstallSmokeDirectDraw1(
    SmokeDirectDraw1Object &directDraw,
    IDirectDraw2 *queryInterfaceValue
) {
    std::memset(g_smokeDirectDraw1VTable, 0, sizeof(g_smokeDirectDraw1VTable));
    g_smokeDirectDraw1VTable[0] = (void *)(&SmokeDirectDrawQueryInterface);
    g_smokeDirectDraw1VTable[2] = (void *)(&SmokeDirectDrawRelease);
    directDraw.vtable = g_smokeDirectDraw1VTable;

    g_smokeDirectDrawCreateCalls = 0;
    g_smokeDirectDrawCreateGuid = 0;
    g_smokeDirectDrawCreateOut = 0;
    g_smokeDirectDrawCreateOuter = 0;
    g_smokeDirectDrawCreateResult = DD_OK;
    g_smokeDirectDrawQueryInterfaceCalls = 0;
    g_smokeDirectDrawQueryInterfaceSelf = (IDirectDraw *)(&directDraw);
    g_smokeDirectDrawQueryInterfaceIid = 0;
    g_smokeDirectDrawQueryInterfaceOut = 0;
    g_smokeDirectDrawQueryInterfaceResult = DD_OK;
    g_smokeDirectDrawQueryInterfaceValue = queryInterfaceValue;
    g_smokeDirectDrawReleaseCalls = 0;
    g_smokeDirectDrawReleaseSelf = 0;
}

void InstallSmokeDirectDrawPalette(
    SmokeDirectDrawPaletteObject &palette
) {
    std::memset(
        g_smokeDirectDrawPaletteVTable,
        0,
        sizeof(g_smokeDirectDrawPaletteVTable)
    );
    g_smokeDirectDrawPaletteVTable[6] =
        (void *)(&SmokeDirectDrawPaletteSetEntries);
    palette.vtable = g_smokeDirectDrawPaletteVTable;
    g_smokeDirectDrawPaletteSetEntriesCalls = 0;
    g_smokeDirectDrawPaletteLastSetEntriesSelf = 0;
    g_smokeDirectDrawPaletteLastSetEntriesFlags = 0;
    g_smokeDirectDrawPaletteLastSetEntriesFirst = 0;
    g_smokeDirectDrawPaletteLastSetEntriesCount = 0;
    g_smokeDirectDrawPaletteLastSetEntriesEntries = 0;
    g_smokeDirectDrawPaletteSetEntriesResult = DD_OK;
}

void InstallSmokeDirect3DDevice2(
    SmokeDirect3DDevice2Object &device
) {
    std::memset(
        g_smokeDirect3DDevice2VTable,
        0,
        sizeof(g_smokeDirect3DDevice2VTable)
    );
    g_smokeDirect3DDevice2VTable[3] =
        (void *)(&SmokeDirect3DDevice2GetCaps);
    g_smokeDirect3DDevice2VTable[6] =
        (void *)(&SmokeDirect3DDevice2AddViewport);
    g_smokeDirect3DDevice2VTable[10] =
        (void *)(&SmokeDirect3DDevice2BeginScene);
    g_smokeDirect3DDevice2VTable[11] =
        (void *)(&SmokeDirect3DDevice2EndScene);
    g_smokeDirect3DDevice2VTable[13] =
        (void *)(&SmokeDirect3DDevice2SetCurrentViewport);
    g_smokeDirect3DDevice2VTable[23] =
        (void *)(&SmokeDirect3DDevice2SetRenderState);
    g_smokeDirect3DDevice2VTable[25] =
        (void *)(&SmokeDirect3DDevice2SetLightState);
    g_smokeDirect3DDevice2VTable[29] =
        (void *)(&SmokeDirect3DDevice2DrawPrimitive);
    device.vtable = g_smokeDirect3DDevice2VTable;
    g_zVideo_pD3DDevice = (IDirect3DDevice2 *)(&device);
    g_smokeDirect3DDevice2GetCapsResult = DD_OK;
    g_smokeDirect3DDevice2GetCapsCalls = 0;
    g_smokeDirect3DDevice2LastGetCapsHalDesc = 0;
    g_smokeDirect3DDevice2LastGetCapsHelDesc = 0;
    g_smokeDirect3DDevice2AddViewportCalls = 0;
    g_smokeDirect3DDevice2LastAddViewport = 0;
    g_smokeDirect3DDevice2AddViewportResult = DD_OK;
    g_smokeDirect3DDevice2BeginSceneResult = DD_OK;
    g_smokeDirect3DDevice2EndSceneResult = DD_OK;
    g_smokeDirect3DDevice2BeginSceneCalls = 0;
    g_smokeDirect3DDevice2EndSceneCalls = 0;
    g_smokeDirect3DDevice2SetCurrentViewportCalls = 0;
    g_smokeDirect3DDevice2LastSetCurrentViewport = 0;
    g_smokeDirect3DDevice2SetCurrentViewportResult = DD_OK;
    g_smokeDirect3DDevice2SetRenderStateCalls = 0;
    std::memset(
        g_smokeDirect3DDevice2RenderStates,
        0,
        sizeof(g_smokeDirect3DDevice2RenderStates)
    );
    std::memset(
        g_smokeDirect3DDevice2RenderStateValues,
        0,
        sizeof(g_smokeDirect3DDevice2RenderStateValues)
    );
    g_smokeDirect3DDevice2SetLightStateCalls = 0;
    std::memset(
        g_smokeDirect3DDevice2LightStates,
        0,
        sizeof(g_smokeDirect3DDevice2LightStates)
    );
    std::memset(
        g_smokeDirect3DDevice2LightStateValues,
        0,
        sizeof(g_smokeDirect3DDevice2LightStateValues)
    );
    g_smokeDirect3DDevice2DrawPrimitiveCalls = 0;
    std::memset(
        g_smokeDirect3DDevice2PrimitiveTypes,
        0,
        sizeof(g_smokeDirect3DDevice2PrimitiveTypes)
    );
    std::memset(
        g_smokeDirect3DDevice2VertexTypes,
        0,
        sizeof(g_smokeDirect3DDevice2VertexTypes)
    );
    std::memset(
        g_smokeDirect3DDevice2Vertices,
        0,
        sizeof(g_smokeDirect3DDevice2Vertices)
    );
    std::memset(
        g_smokeDirect3DDevice2VertexCounts,
        0,
        sizeof(g_smokeDirect3DDevice2VertexCounts)
    );
    std::memset(
        g_smokeDirect3DDevice2DrawFlags,
        0,
        sizeof(g_smokeDirect3DDevice2DrawFlags)
    );
    g_smokeDirect3DDevice2LastPrimitiveType = (D3DPRIMITIVETYPE)(0);
    g_smokeDirect3DDevice2LastVertexType = (D3DVERTEXTYPE)(0);
    g_smokeDirect3DDevice2LastVertices = 0;
    g_smokeDirect3DDevice2LastVertexCount = 0;
    g_smokeDirect3DDevice2LastDrawFlags = 0;
    g_smokeDirect3DDevice2DrawPrimitiveResult = DD_OK;
}

void InstallSmokeDirect3D2(
    SmokeDirect3D2Object &d3d,
    IDirect3DDevice2 *createdDevice,
    IDirect3DViewport2 *createdViewport,
    IDirect3DMaterial2 *createdMaterial
) {
    std::memset(g_smokeDirect3D2VTable, 0, sizeof(g_smokeDirect3D2VTable));
    g_smokeDirect3D2VTable[5] = (void *)(&SmokeDirect3D2CreateMaterial);
    g_smokeDirect3D2VTable[6] = (void *)(&SmokeDirect3D2CreateViewport);
    g_smokeDirect3D2VTable[8] = (void *)(&SmokeDirect3D2CreateDevice);
    d3d.vtable = g_smokeDirect3D2VTable;
    g_smokeDirect3D2CreateDeviceResult = DD_OK;
    g_smokeDirect3D2CreateViewportResult = DD_OK;
    g_smokeDirect3D2CreateMaterialResult = DD_OK;
    g_smokeDirect3D2CreateDeviceCalls = 0;
    g_smokeDirect3D2LastCreateDeviceGuid = 0;
    g_smokeDirect3D2LastCreateDeviceSurface = 0;
    g_smokeDirect3D2LastCreateDeviceOut = 0;
    g_smokeDirect3D2CreatedDevice = createdDevice;
    g_smokeDirect3D2CreateViewportCalls = 0;
    g_smokeDirect3D2LastCreateViewportOut = 0;
    g_smokeDirect3D2LastCreateViewportOuter = 0;
    g_smokeDirect3D2CreatedViewport = createdViewport;
    g_smokeDirect3D2CreateMaterialCalls = 0;
    g_smokeDirect3D2LastCreateMaterialOut = 0;
    g_smokeDirect3D2LastCreateMaterialOuter = 0;
    g_smokeDirect3D2CreatedMaterial = createdMaterial;
}

void InstallSmokeDirect3DViewport2(
    SmokeDirect3DViewport2Object &viewport
) {
    std::memset(
        g_smokeDirect3DViewport2VTable,
        0,
        sizeof(g_smokeDirect3DViewport2VTable)
    );
    g_smokeDirect3DViewport2VTable[8] =
        (void *)(&SmokeDirect3DViewport2SetBackground);
    g_smokeDirect3DViewport2VTable[17] =
        (void *)(&SmokeDirect3DViewport2SetViewport2);
    viewport.vtable = g_smokeDirect3DViewport2VTable;
    g_smokeDirect3DViewport2SetViewport2Result = DD_OK;
    g_smokeDirect3DViewport2SetViewport2Calls = 0;
    g_smokeDirect3DViewport2LastViewportValue = {};
    g_smokeDirect3DViewport2SetBackgroundResult = DD_OK;
    g_smokeDirect3DViewport2SetBackgroundCalls = 0;
    g_smokeDirect3DViewport2LastBackground = 0;
}

void InstallSmokeDirect3DMaterial2(
    SmokeDirect3DMaterial2Object &material
) {
    std::memset(
        g_smokeDirect3DMaterial2VTable,
        0,
        sizeof(g_smokeDirect3DMaterial2VTable)
    );
    g_smokeDirect3DMaterial2VTable[3] =
        (void *)(&SmokeDirect3DMaterial2SetMaterial);
    g_smokeDirect3DMaterial2VTable[5] =
        (void *)(&SmokeDirect3DMaterial2GetHandle);
    material.vtable = g_smokeDirect3DMaterial2VTable;
    g_smokeDirect3DMaterial2SetMaterialResult = DD_OK;
    g_smokeDirect3DMaterial2SetMaterialCalls = 0;
    g_smokeDirect3DMaterial2LastMaterialValue = {};
    g_smokeDirect3DMaterial2GetHandleResult = DD_OK;
    g_smokeDirect3DMaterial2GetHandleCalls = 0;
    g_smokeDirect3DMaterial2LastGetHandleDevice = 0;
    g_smokeDirect3DMaterial2LastGetHandleOut = 0;
    g_smokeDirect3DMaterial2HandleValue = 0;
}

void InstallSmokeDirect3DTexture2(
    SmokeDirect3DTexture2Object &uploadTexture,
    SmokeDirect3DTexture2Object &targetTexture
) {
    std::memset(
        g_smokeDirect3DTexture2VTable,
        0,
        sizeof(g_smokeDirect3DTexture2VTable)
    );
    g_smokeDirect3DTexture2VTable[2] = (void *)(&SmokeDirect3DTexture2Release);
    g_smokeDirect3DTexture2VTable[3] = (void *)(&SmokeDirect3DTexture2GetHandle);
    g_smokeDirect3DTexture2VTable[5] = (void *)(&SmokeDirect3DTexture2Load);
    uploadTexture.vtable = g_smokeDirect3DTexture2VTable;
    targetTexture.vtable = g_smokeDirect3DTexture2VTable;
    g_smokeDirect3DTexture2LoadResult = DD_OK;
    g_smokeDirect3DTexture2LoadCalls = 0;
    g_smokeDirect3DTexture2LastLoadSelf = 0;
    g_smokeDirect3DTexture2LastLoadSource = 0;
    g_smokeDirect3DTexture2GetHandleResult = DD_OK;
    g_smokeDirect3DTexture2GetHandleCalls = 0;
    g_smokeDirect3DTexture2LastGetHandleSelf = 0;
    g_smokeDirect3DTexture2LastGetHandleDevice = 0;
    g_smokeDirect3DTexture2LastGetHandleOut = 0;
    g_smokeDirect3DTexture2HandleValue = 0;
    g_smokeDirect3DTexture2ReleaseCalls = 0;
    std::memset(
        g_smokeDirect3DTexture2ReleaseObjects,
        0,
        sizeof(g_smokeDirect3DTexture2ReleaseObjects)
    );
}

int __fastcall SmokeTextureMemoryQuery(
    int flags,
    int *totalBytes,
    int *freeBytes
) {
    ++g_smokeTextureMemoryQueryCalls;
    g_smokeLastTextureMemoryQueryFlags = flags;
    *totalBytes = 0x1000;
    *freeBytes = 0x400;
    return 0;
}

int __fastcall SmokeDeviceMemoryQuery(
    int flags,
    int *totalBytes,
    int *freeBytes
) {
    ++g_smokeDeviceMemoryQueryCalls;
    g_smokeLastDeviceMemoryQueryFlags = flags;
    *totalBytes = 0x2000;
    *freeBytes = 0x800;
    return 0;
}

int RunSmokeTests(const SmokeTest *tests, int count, const char *onlyName) {
    int failures = 0;
    bool ran = false;
    for (int i = 0; i < count; ++i) {
        if (onlyName != nullptr && std::strcmp(tests[i].name, onlyName) != 0) {
            continue;
        }

        ran = true;
        const int result = tests[i].run();
        if (result != 0) {
            std::printf("%s failed: %d\n", tests[i].name, result);
            failures += result;
        }
    }
    if (onlyName != nullptr && !ran) {
        std::printf("%s not found\n", onlyName);
        return 1;
    }
    return failures;
}
} // namespace

extern "C" int zgame_return_only_stub_smoke(void) {
    zGame::ReturnOnlyStub();
    return 0;
}

extern "C" int zvid_query_device_video_memory_bytes_smoke(void) {
    SmokeDirectDraw2Object directDraw{};
    InstallSmokeDirectDraw2(directDraw);

    const int oldRendererType = g_zVideo_RendererType;
    IDirectDraw2 *const oldDirectDraw2 = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const oldSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const zVidHwApiDeviceRecordPartial oldRecord1 = g_zVideo_HwApiDeviceTable[1];
    const zVidHwApiDeviceRecordPartial oldRecord2 = g_zVideo_HwApiDeviceTable[2];

    int failCode = 0;
    int totalBytes = 123;
    int freeBytes = 456;
    g_zVideo_RendererType = 0;
    if (zVid::QueryDeviceVideoMemoryBytes(
            1,
            &totalBytes,
            &freeBytes
        ) != 0 ||
        totalBytes != 0 ||
        freeBytes != 0 ||
        g_smokeDirectDraw2GetAvailableVidMemCalls != 0) {
        failCode = 1;
    }

    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_textureMemTotalBytes = 0x2000;
    g_zVideo_RendererType = 1;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_smokeDirectDraw2AvailableVidMemTotal = 0x900000;
    g_smokeDirectDraw2AvailableVidMemFree = 0x700000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         g_smokeDirectDraw2GetAvailableVidMemCalls != 1 ||
         g_smokeDirectDraw2LastAvailableVidMemCaps.dwCaps != DDSCAPS_VIDEOMEMORY ||
         g_smokeDirectDraw2LastAvailableVidMemTotal != (DWORD *)(&totalBytes) ||
         g_smokeDirectDraw2LastAvailableVidMemFree != (DWORD *)(&freeBytes) ||
         totalBytes != 0x900000 ||
         freeBytes != 0x6fe000)) {
        failCode = 2;
    }

    g_smokeDirectDraw2GetAvailableVidMemResult = DDERR_INVALIDPARAMS;
    totalBytes = 333;
    freeBytes = 444;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         g_smokeDirectDraw2GetAvailableVidMemCalls != 2 ||
         totalBytes != 0 ||
         freeBytes != 0)) {
        failCode = 3;
    }

    g_zVideo_HwApiDeviceTable[1].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[1].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[1].m_textureMemTotalBytes = 0x200000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         totalBytes != 0x900000 ||
         freeBytes != 0x500000 ||
         g_smokeDirectDraw2GetAvailableVidMemCalls != 2)) {
        failCode = 4;
    }

    g_zVideo_HwApiDeviceTable[2].m_videoMemTotalBytes = 0x500000;
    g_zVideo_HwApiDeviceTable[2].m_videoMemFreeBytes = 0x480000;
    g_zVideo_HwApiDeviceTable[2].m_textureMemTotalBytes = 0x500000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             2,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         totalBytes != 0x500000 ||
         freeBytes != 0x28c000)) {
        failCode = 5;
    }

    g_zVideo_RendererType = oldRendererType;
    g_zVideo_pDirectDraw2 = oldDirectDraw2;
    g_zVideo_pSelectedHwApiDeviceRecord = oldSelectedDevice;
    g_zVideo_HwApiDeviceTable[1] = oldRecord1;
    g_zVideo_HwApiDeviceTable[2] = oldRecord2;
    return failCode;
}

extern "C" int zvid_query_texture_memory_bytes_smoke(void) {
    SmokeDirectDraw2Object directDraw{};
    InstallSmokeDirectDraw2(directDraw);

    IDirectDraw2 *const oldDirectDraw2 = g_zVideo_pDirectDraw2;
    const zVidHwApiDeviceRecordPartial oldRecord2 = g_zVideo_HwApiDeviceTable[2];

    int failCode = 0;
    int totalBytes = 123;
    int freeBytes = 456;
    g_zVideo_pDirectDraw2 = 0;
    if (zVid::QueryTextureMemoryBytes(
            2,
            &totalBytes,
            &freeBytes
        ) != 0 ||
        totalBytes != 0 ||
        freeBytes != 0 ||
        g_smokeDirectDraw2GetAvailableVidMemCalls != 0) {
        failCode = 1;
    }

    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2AvailableVidMemTotal = 0x810000;
    g_smokeDirectDraw2AvailableVidMemFree = 0x610000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryTextureMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         g_smokeDirectDraw2GetAvailableVidMemCalls != 1 ||
         g_smokeDirectDraw2LastAvailableVidMemCaps.dwCaps != DDSCAPS_TEXTURE ||
         g_smokeDirectDraw2LastAvailableVidMemTotal != (DWORD *)(&totalBytes) ||
         g_smokeDirectDraw2LastAvailableVidMemFree != (DWORD *)(&freeBytes) ||
         totalBytes != 0x810000 ||
         freeBytes != 0x610000)) {
        failCode = 2;
    }

    g_smokeDirectDraw2GetAvailableVidMemResult = DDERR_INVALIDPARAMS;
    totalBytes = 333;
    freeBytes = 444;
    if (failCode == 0 &&
        (zVid::QueryTextureMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         g_smokeDirectDraw2GetAvailableVidMemCalls != 2 ||
         totalBytes != 0 ||
         freeBytes != 0)) {
        failCode = 3;
    }

    g_zVideo_HwApiDeviceTable[2].m_textureMemTotalBytes = 0x500000;
    g_zVideo_HwApiDeviceTable[2].m_textureMemFreeBytes = 0x320000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryTextureMemoryBytes(
             2,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         totalBytes != 0x500000 ||
         freeBytes != 0x320000 ||
         g_smokeDirectDraw2GetAvailableVidMemCalls != 2)) {
        failCode = 4;
    }

    g_zVideo_pDirectDraw2 = oldDirectDraw2;
    g_zVideo_HwApiDeviceTable[2] = oldRecord2;
    return failCode;
}

extern "C" int zvideo_dd_prepare_window_for_mode_smoke(void) {
    const HWND oldHwnd = g_zVideo_hWnd;
    PALETTEENTRY oldPalette[0x100];
    PALETTEENTRY expectedPalette[0x100];
    std::memcpy(oldPalette, g_zVideo_SystemPaletteEntries, sizeof(oldPalette));
    std::memset(expectedPalette, 0, sizeof(expectedPalette));

    HDC screenDc = GetDC(0);
    const int palettizedDesktop =
        screenDc != 0 && (GetDeviceCaps(screenDc, RASTERCAPS) & RC_PALETTE) != 0;
    UINT expectedPaletteCount = 0;
    if (palettizedDesktop != 0) {
        expectedPaletteCount = GetSystemPaletteEntries(
            screenDc,
            0,
            0x100,
            expectedPalette
        );
    }
    if (screenDc != 0) {
        ReleaseDC(
            0,
            screenDc
        );
    }

    std::memset(g_zVideo_SystemPaletteEntries, 0x44, sizeof(g_zVideo_SystemPaletteEntries));
    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-video-prepare-window-test",
        WS_OVERLAPPEDWINDOW,
        20,
        30,
        160,
        120,
        0,
        0,
        GetModuleHandleA(0),
        0
    );
    if (hwnd == 0) {
        std::memcpy(g_zVideo_SystemPaletteEntries, oldPalette, sizeof(oldPalette));
        g_zVideo_hWnd = oldHwnd;
        return 1;
    }

    HMENU menu = CreateMenu();
    if (menu != 0) {
        SetMenu(
            hwnd,
            menu
        );
    }

    g_zVideo_hWnd = hwnd;
    const int result = zVideo_dd::PrepareWindowForMode();
    const LONG exStyle = GetWindowLongA(
        hwnd,
        GWL_EXSTYLE
    );
    const LONG style = GetWindowLongA(
        hwnd,
        GWL_STYLE
    );
    const int menuRemoved = GetMenu(hwnd) == 0;
    const int exStyleOk = (exStyle & WS_EX_APPWINDOW) == WS_EX_APPWINDOW;
    const int fullscreenStyleOk =
        (style & (LONG)(WS_POPUP | WS_CLIPCHILDREN)) ==
            (LONG)(WS_POPUP | WS_CLIPCHILDREN) &&
        (style & (LONG)(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                        WS_SYSMENU)) == 0;
    const int paletteOk =
        palettizedDesktop != 0 && expectedPaletteCount == 0x100
            ? std::memcmp(
                  g_zVideo_SystemPaletteEntries,
                  expectedPalette,
                  sizeof(expectedPalette)
              ) == 0
            : g_zVideo_SystemPaletteEntries[0].peRed == 0x44 &&
                  g_zVideo_SystemPaletteEntries[0].peGreen == 0x44 &&
                  g_zVideo_SystemPaletteEntries[0].peBlue == 0x44 &&
                  g_zVideo_SystemPaletteEntries[0].peFlags == 0x44;

    g_zVideo_hWnd = oldHwnd;
    std::memcpy(g_zVideo_SystemPaletteEntries, oldPalette, sizeof(oldPalette));
    DestroyWindow(hwnd);
    if (menu != 0) {
        DestroyMenu(menu);
    }

    if (result != 0) {
        return 2;
    }
    if (menuRemoved == 0) {
        return 3;
    }
    if (exStyleOk == 0 || fullscreenStyleOk == 0) {
        return 4;
    }
    return paletteOk != 0 ? 0 : 5;
}

extern "C" int zvideo_dd_report_error_smoke(void) {
    g_smokeTextureMemoryQueryCalls = 0;
    g_smokeDeviceMemoryQueryCalls = 0;
    g_smokeLastTextureMemoryQueryFlags = 0;
    g_smokeLastDeviceMemoryQueryFlags = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = SmokeTextureMemoryQuery;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = SmokeDeviceMemoryQuery;

    if (zVideo_dd::ReportError(DD_OK, "video.cpp", 10) != 0 ||
        g_smokeTextureMemoryQueryCalls != 0 ||
        g_smokeDeviceMemoryQueryCalls != 0) {
        return 1;
    }

    if (zVideo_dd::ReportError(DDERR_INVALIDPARAMS, "video.cpp", 20) != -1 ||
        g_smokeTextureMemoryQueryCalls != 0 ||
        g_smokeDeviceMemoryQueryCalls != 0) {
        return 2;
    }

    if (zVideo_dd::ReportError(DDERR_OUTOFVIDEOMEMORY, "video.cpp", 30) != -1 ||
        g_smokeTextureMemoryQueryCalls != 1 ||
        g_smokeDeviceMemoryQueryCalls != 1 ||
        g_smokeLastTextureMemoryQueryFlags != -1 ||
        g_smokeLastDeviceMemoryQueryFlags != -1) {
        return 3;
    }

    g_zVideo_pfnQueryTextureMemoryBytes = 0;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = 0;
    return 0;
}

extern "C" int zvideo_dd_create_directdraw2_for_selected_device_smoke(void) {
    SmokeDirectDraw1Object directDraw1{};
    SmokeDirectDraw2Object directDraw2{};
    SmokeImportPatch directDrawCreatePatch{};
    InstallSmokeDirectDraw1(
        directDraw1,
        (IDirectDraw2 *)(&directDraw2)
    );

    if (!PatchSmokeImportByName(
            "DDRAW.dll",
            "DirectDrawCreate",
            (void *)(&SmokeDirectDrawCreate),
            directDrawCreatePatch
        )) {
        return 1;
    }

    zVidHwApiDeviceRecordPartial *const oldSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    IDirectDraw2 *const oldDirectDraw2 = g_zVideo_pDirectDraw2;

    GUID directDrawGuid = {0x12345678, 0x1111, 0x2222, {3, 4, 5, 6, 7, 8, 9, 10}};
    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.pDirectDrawGuid = &directDrawGuid;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_pDirectDraw2 = 0;

    int failCode = 0;
    const int successResult = zVideo_dd::CreateDirectDraw2ForSelectedDevice();
    if (successResult != 0 ||
        g_smokeDirectDrawCreateCalls != 1 ||
        g_smokeDirectDrawCreateGuid != &directDrawGuid ||
        g_smokeDirectDrawCreateOut == 0 ||
        g_smokeDirectDrawCreateOuter != 0 ||
        g_smokeDirectDrawQueryInterfaceCalls != 1 ||
        g_smokeDirectDrawQueryInterfaceSelf != (IDirectDraw *)(&directDraw1) ||
        !IsEqualGUID(*g_smokeDirectDrawQueryInterfaceIid, IID_IDirectDraw2) ||
        g_smokeDirectDrawQueryInterfaceOut != (void **)(&g_zVideo_pDirectDraw2) ||
        g_zVideo_pDirectDraw2 != (IDirectDraw2 *)(&directDraw2) ||
        g_smokeDirectDrawReleaseCalls != 1 ||
        g_smokeDirectDrawReleaseSelf != (IDirectDraw *)(&directDraw1)) {
        failCode = 2;
    }

    InstallSmokeDirectDraw1(
        directDraw1,
        (IDirectDraw2 *)(&directDraw2)
    );
    g_smokeDirectDrawCreateResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(0x1357);
    const int createFailureResult = zVideo_dd::CreateDirectDraw2ForSelectedDevice();
    if (failCode == 0 &&
        (createFailureResult == 0 ||
         g_smokeDirectDrawCreateCalls != 1 ||
         g_smokeDirectDrawQueryInterfaceCalls != 0 ||
         g_smokeDirectDrawReleaseCalls != 0 ||
         g_zVideo_pDirectDraw2 != (IDirectDraw2 *)(0x1357))) {
        failCode = 3;
    }

    InstallSmokeDirectDraw1(
        directDraw1,
        (IDirectDraw2 *)(&directDraw2)
    );
    g_smokeDirectDrawQueryInterfaceResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(0x2468);
    const int queryFailureResult = zVideo_dd::CreateDirectDraw2ForSelectedDevice();
    if (failCode == 0 &&
        (queryFailureResult == 0 ||
         g_smokeDirectDrawCreateCalls != 1 ||
         g_smokeDirectDrawQueryInterfaceCalls != 1 ||
         g_smokeDirectDrawReleaseCalls != 0 ||
         g_zVideo_pDirectDraw2 != (IDirectDraw2 *)(0x2468))) {
        failCode = 4;
    }

    g_zVideo_pSelectedHwApiDeviceRecord = oldSelectedDevice;
    g_zVideo_pDirectDraw2 = oldDirectDraw2;
    RestoreSmokeImportPatch(directDrawCreatePatch);
    return failCode;
}

extern "C" int zvideo_dd_create_surface3_from_desc_smoke(void) {
    SmokeDirectDraw2Object directDraw{};
    SmokeDirectDrawSurfaceObject baseSurface{};
    SmokeDirectDrawSurface3Object surface3{};
    DDSURFACEDESC desc{};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;

    IDirectDrawSurface3 *outSurface = 0;
    HRESULT result = zVideo_dd::CreateSurface3FromDesc(
        (IDirectDraw2 *)(&directDraw),
        &desc,
        &outSurface,
        0
    );
    if (result != DD_OK ||
        g_smokeDirectDraw2CreateSurfaceCalls != 1 ||
        g_smokeDirectDraw2LastCreateSurfaceSelf != (IDirectDraw2 *)(&directDraw) ||
        g_smokeDirectDraw2LastCreateSurfaceDesc != &desc ||
        g_smokeDirectDraw2LastCreateSurfaceOut == 0 ||
        g_smokeDirectDraw2LastCreateSurfaceOuter != 0 ||
        g_smokeDirectDrawSurfaceQueryInterfaceCalls != 1 ||
        g_smokeDirectDrawSurfaceLastQueryInterfaceSelf !=
            (IDirectDrawSurface *)(&baseSurface) ||
        !IsEqualGUID(
            *g_smokeDirectDrawSurfaceLastQueryInterfaceIid,
            IID_IDirectDrawSurface3
        ) ||
        g_smokeDirectDrawSurfaceLastQueryInterfaceOut != (void **)(&outSurface) ||
        g_smokeDirectDrawSurfaceReleaseCalls != 1 ||
        g_smokeDirectDrawSurfaceReleaseSelf != (IDirectDrawSurface *)(&baseSurface) ||
        outSurface != (IDirectDrawSurface3 *)(&surface3)) {
        return 2;
    }

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDraw2CreateSurfaceResult = DDERR_INVALIDPARAMS;
    outSurface = 0;
    result = zVideo_dd::CreateSurface3FromDesc(
        (IDirectDraw2 *)(&directDraw),
        &desc,
        &outSurface,
        0
    );
    if (result != DDERR_INVALIDPARAMS ||
        g_smokeDirectDraw2CreateSurfaceCalls != 1 ||
        g_smokeDirectDrawSurfaceQueryInterfaceCalls != 0 ||
        g_smokeDirectDrawSurfaceReleaseCalls != 0 ||
        outSurface != 0) {
        return 3;
    }

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceResult = DDERR_GENERIC;
    outSurface = 0;
    result = zVideo_dd::CreateSurface3FromDesc(
        (IDirectDraw2 *)(&directDraw),
        &desc,
        &outSurface,
        0
    );
    return result == DDERR_GENERIC &&
                   g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
                   g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
                   g_smokeDirectDrawSurfaceReleaseCalls == 0 &&
                   outSurface == 0
               ? 0
               : 4;
}

extern "C" int zvideo_dd_lock_directdraw_surface_smoke(void) {
    SmokeDirectDrawSurface3Object surface{};
    DDSURFACEDESC surfaceDesc;

    InstallSmokeDirectDrawSurface3(surface);
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    if (zVideo_dd::LockDirectDrawSurface(
            (IDirectDrawSurface3 *)(&surface),
            &surfaceDesc
        ) != 0 ||
        g_smokeDirectDrawSurface3LockCalls != 1 ||
        g_smokeDirectDrawSurface3RestoreCalls != 0 ||
        g_smokeDirectDrawSurface3LastLockRect != 0 ||
        g_smokeDirectDrawSurface3LastLockDesc != &surfaceDesc ||
        g_smokeDirectDrawSurface3LastLockFlags != DDLOCK_WAIT ||
        g_smokeDirectDrawSurface3LastLockEvent != 0 ||
        g_smokeDirectDrawSurface3LockDescSize != sizeof(surfaceDesc) ||
        surfaceDesc.dwSize != sizeof(surfaceDesc) ||
        surfaceDesc.dwWidth != 640 ||
        surfaceDesc.dwHeight != 480 ||
        surfaceDesc.lPitch != 1280 ||
        surfaceDesc.lpSurface != g_smokeDirectDrawSurface3LockPixels) {
        return 1;
    }

    InstallSmokeDirectDrawSurface3(surface);
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3LockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3LockResultCount = 2;
    if (zVideo_dd::LockDirectDrawSurface(
            (IDirectDrawSurface3 *)(&surface),
            &surfaceDesc
        ) != 0 ||
        g_smokeDirectDrawSurface3LockCalls != 2 ||
        g_smokeDirectDrawSurface3RestoreCalls != 1 ||
        g_smokeDirectDrawSurface3LockDescSize != sizeof(surfaceDesc) ||
        surfaceDesc.dwSize != sizeof(surfaceDesc) ||
        surfaceDesc.dwWidth != 640 ||
        surfaceDesc.dwHeight != 480 ||
        surfaceDesc.lPitch != 1280 ||
        surfaceDesc.lpSurface != g_smokeDirectDrawSurface3LockPixels) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_dd_unlock_directdraw_surface_smoke(void) {
    SmokeDirectDrawSurface3Object surface{};

    InstallSmokeDirectDrawSurface3(surface);
    if (zVideo_dd::UnlockDirectDrawSurface(
            (IDirectDrawSurface3 *)(&surface)
        ) != 0 ||
        g_smokeDirectDrawSurface3UnlockCalls != 1 ||
        g_smokeDirectDrawSurface3RestoreCalls != 0 ||
        g_smokeDirectDrawSurface3LastUnlockSurface !=
            (IDirectDrawSurface3 *)(&surface) ||
        g_smokeDirectDrawSurface3LastUnlockArg != 0) {
        return 1;
    }

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3UnlockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3UnlockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3UnlockResultCount = 2;
    if (zVideo_dd::UnlockDirectDrawSurface(
            (IDirectDrawSurface3 *)(&surface)
        ) != 0 ||
        g_smokeDirectDrawSurface3UnlockCalls != 2 ||
        g_smokeDirectDrawSurface3RestoreCalls != 1 ||
        g_smokeDirectDrawSurface3LastUnlockArg != 0) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_surface_state_lock_skip_smoke(void) {
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;

    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = (IDirectDrawSurface3 *)(0x1234);
    g_zVideo_DisplayModeSurfaceState.locked = 1;
    g_zVideo_FullscreenOption = 0;

    if (zVideo_dd::LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ||
        g_zVideo_DisplayModeSurfaceState.locked != 1) {
        g_zVideo_FullscreenOption = savedFullscreenOption;
        g_zVideo_DisplayModeSurfaceState = savedDisplayState;
        return 1;
    }

    if (zVideo_dd::UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ||
        g_zVideo_DisplayModeSurfaceState.locked != 1) {
        g_zVideo_FullscreenOption = savedFullscreenOption;
        g_zVideo_DisplayModeSurfaceState = savedDisplayState;
        return 2;
    }

    zVideo_SurfaceStatePartial alreadyLocked{};
    alreadyLocked.locked = 1;
    zVideo_SurfaceStatePartial alreadyUnlocked{};
    g_zVideo_FullscreenOption = 1;
    const int skipOk =
        zVideo_dd::LockSurfaceState(&alreadyLocked) == 0 &&
        zVideo_dd::UnlockSurfaceState(&alreadyUnlocked) == 0;

    g_zVideo_FullscreenOption = savedFullscreenOption;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return skipOk != 0 ? 0 : 3;
}

extern "C" int zvideo_dd_lock_surface_state_smoke(void) {
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    SmokeDirectDrawSurface3Object surface{};
    zVideo_SurfaceStatePartial surfaceState{};

    InstallSmokeDirectDrawSurface3(surface);
    surfaceState.surf = (IDirectDrawSurface3 *)(&surface);
    g_zVideo_FullscreenOption = 1;

    const int result = zVideo_dd::LockSurfaceState(&surfaceState);
    const int ok =
        result == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3LastLockRect == 0 &&
        g_smokeDirectDrawSurface3LastLockFlags == DDLOCK_WAIT &&
        g_smokeDirectDrawSurface3LastLockEvent == 0 &&
        g_smokeDirectDrawSurface3LockDescSize == sizeof(DDSURFACEDESC) &&
        surfaceState.width == 640 &&
        surfaceState.height == 480 &&
        surfaceState.pitch == 1280 &&
        surfaceState.pixels == g_smokeDirectDrawSurface3LockPixels &&
        surfaceState.locked == 1 &&
        surfaceState.lockInfoValid == 1;

    g_zVideo_FullscreenOption = savedFullscreenOption;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd_unlock_surface_state_smoke(void) {
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    SmokeDirectDrawSurface3Object surface{};
    zVideo_SurfaceStatePartial surfaceState{};

    InstallSmokeDirectDrawSurface3(surface);
    surfaceState.locked = 1;
    surfaceState.surf = (IDirectDrawSurface3 *)(&surface);
    g_zVideo_FullscreenOption = 1;

    const int result = zVideo_dd::UnlockSurfaceState(&surfaceState);
    const int ok =
        result == 0 &&
        surfaceState.locked == 0 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3LastUnlockSurface ==
            (IDirectDrawSurface3 *)(&surface) &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    g_zVideo_FullscreenOption = savedFullscreenOption;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd_verify_fullscreen_surface_locks_smoke(void) {
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;

    SmokeDirectDrawSurface3Object swSurface{};
    SmokeDirectDrawSurface3Object primarySurface{};
    SmokeDirectDrawSurface3Object displaySurface{};
    InstallSmokeDirectDrawSurface3(swSurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    displaySurface.vtable = g_smokeDirectDrawSurface3VTable;

    IDirectDrawSurface3 *const swInterface =
        (IDirectDrawSurface3 *)(&swSurface);
    IDirectDrawSurface3 *const primaryInterface =
        (IDirectDrawSurface3 *)(&primarySurface);
    IDirectDrawSurface3 *const displayInterface =
        (IDirectDrawSurface3 *)(&displaySurface);

    g_zVideo_FullscreenOption = 1;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;

    const int successResult = zVideo_dd::VerifyFullscreenSurfaceLocks();
    const int successOk =
        successResult == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 3 &&
        g_smokeDirectDrawSurface3UnlockCalls == 3 &&
        g_smokeDirectDrawSurface3LockSurfaces[0] == swInterface &&
        g_smokeDirectDrawSurface3UnlockSurfaces[0] == swInterface &&
        g_smokeDirectDrawSurface3LockSurfaces[1] == primaryInterface &&
        g_smokeDirectDrawSurface3UnlockSurfaces[1] == primaryInterface &&
        g_smokeDirectDrawSurface3LockSurfaces[2] == displayInterface &&
        g_smokeDirectDrawSurface3UnlockSurfaces[2] == displayInterface &&
        g_zVideo_SwSurfaceState.locked == 0 &&
        g_zVideo_PrimarySurfaceState.locked == 0 &&
        g_zVideo_DisplayModeSurfaceState.locked == 0;

    InstallSmokeDirectDrawSurface3(swSurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    displaySurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3LockResultCount = 1;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;

    const int firstLockFailureResult =
        zVideo_dd::VerifyFullscreenSurfaceLocks();
    const int firstLockFailureOk =
        firstLockFailureResult == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 0 &&
        g_smokeDirectDrawSurface3LockSurfaces[0] == swInterface;

    InstallSmokeDirectDrawSurface3(swSurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    displaySurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_smokeDirectDrawSurface3UnlockResults[0] = DD_OK;
    g_smokeDirectDrawSurface3UnlockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3UnlockResults[2] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3UnlockResultCount = 3;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;

    const int finalUnlockFailureResult =
        zVideo_dd::VerifyFullscreenSurfaceLocks();
    const int finalUnlockFailureOk =
        finalUnlockFailureResult == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 3 &&
        g_smokeDirectDrawSurface3UnlockCalls == 3 &&
        g_smokeDirectDrawSurface3UnlockSurfaces[2] == displayInterface;

    g_zVideo_FullscreenOption = savedFullscreenOption;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return successOk != 0 && firstLockFailureOk != 0 &&
                   finalUnlockFailureOk != 0
               ? 0
               : 1;
}

extern "C" int zvideo_dd_open_video_mode_smoke(void) {
    SmokeDirectDraw1Object directDraw1{};
    SmokeDirectDraw2Object directDraw2{};
    SmokeImportPatch directDrawCreatePatch{};
    InstallSmokeDirectDraw1(
        directDraw1,
        (IDirectDraw2 *)(&directDraw2)
    );

    if (!PatchSmokeImportByName(
            "DDRAW.dll",
            "DirectDrawCreate",
            (void *)(&SmokeDirectDrawCreate),
            directDrawCreatePatch
        )) {
        return 1;
    }

    zVidHwApiDeviceRecordPartial *const oldSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    IDirectDraw2 *const oldDirectDraw2 = g_zVideo_pDirectDraw2;
    const HWND oldHwnd = g_zVideo_hWnd;
    PALETTEENTRY oldPalette[0x100];
    std::memcpy(oldPalette, g_zVideo_SystemPaletteEntries, sizeof(oldPalette));

    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-video-open-mode-test",
        WS_OVERLAPPEDWINDOW,
        20,
        30,
        160,
        120,
        0,
        0,
        GetModuleHandleA(0),
        0
    );
    if (hwnd == 0) {
        g_zVideo_pSelectedHwApiDeviceRecord = oldSelectedDevice;
        g_zVideo_pDirectDraw2 = oldDirectDraw2;
        g_zVideo_hWnd = oldHwnd;
        std::memcpy(g_zVideo_SystemPaletteEntries, oldPalette, sizeof(oldPalette));
        RestoreSmokeImportPatch(directDrawCreatePatch);
        return 2;
    }

    GUID directDrawGuid = {0x87654321, 0x3333, 0x4444, {10, 9, 8, 7, 6, 5, 4, 3}};
    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.pDirectDrawGuid = &directDrawGuid;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = hwnd;

    g_zVideo_pDirectDraw2 = 0;
    const int successResult = zVideo_dd::OpenVideoMode(99);
    const int successOk =
        successResult == 0 &&
        g_smokeDirectDrawCreateCalls == 1 &&
        g_smokeDirectDrawQueryInterfaceCalls == 1 &&
        g_smokeDirectDrawReleaseCalls == 1 &&
        g_zVideo_pDirectDraw2 == (IDirectDraw2 *)(&directDraw2);

    InstallSmokeDirectDraw1(
        directDraw1,
        (IDirectDraw2 *)(&directDraw2)
    );
    g_smokeDirectDrawCreateResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(0x9753);
    const int failureResult = zVideo_dd::OpenVideoMode(123);
    const int failureOk =
        failureResult == 1 &&
        g_smokeDirectDrawCreateCalls == 1 &&
        g_smokeDirectDrawQueryInterfaceCalls == 0 &&
        g_smokeDirectDrawReleaseCalls == 0 &&
        g_zVideo_pDirectDraw2 == (IDirectDraw2 *)(0x9753);

    DestroyWindow(hwnd);
    g_zVideo_pSelectedHwApiDeviceRecord = oldSelectedDevice;
    g_zVideo_pDirectDraw2 = oldDirectDraw2;
    g_zVideo_hWnd = oldHwnd;
    std::memcpy(g_zVideo_SystemPaletteEntries, oldPalette, sizeof(oldPalette));
    RestoreSmokeImportPatch(directDrawCreatePatch);

    return successOk != 0 && failureOk != 0 ? 0 : 3;
}

extern "C" int zvideo_texture_record_destroy_smoke(void) {
    zVideo_TextureRecordPartial *const savedDefaultRecord =
        g_zImage_DefaultTextureRecord;

    zVideo_TextureRecordPartial defaultRecord{};
    g_zImage_DefaultTextureRecord = &defaultRecord;
    ResetSmokeComReleaseTracking();
    zVideo_dd3d::TextureRecord_Destroy(&defaultRecord);
    const int defaultSkipOk = g_smokeComReleaseCalls == 0;

    SmokeComObject uploadSurface{};
    SmokeComObject textureSurface{};
    SmokeComObject texture{};
    InstallSmokeComReleaseObject(uploadSurface);
    InstallSmokeComReleaseObject(textureSurface);
    InstallSmokeComReleaseObject(texture);
    ResetSmokeComReleaseTracking();

    zVideo_TextureRecordPartial *textureRecord =
        (zVideo_TextureRecordPartial *)std::malloc(sizeof(zVideo_TextureRecordPartial));
    if (textureRecord == 0) {
        g_zImage_DefaultTextureRecord = savedDefaultRecord;
        return 1;
    }

    std::memset(textureRecord, 0, sizeof(*textureRecord));
    textureRecord->m_uploadSurface = (IDirectDrawSurface *)(&uploadSurface);
    textureRecord->m_textureSurface = (IDirectDrawSurface *)(&textureSurface);
    textureRecord->m_texture = (IDirect3DTexture2 *)(&texture);

    zVideo_dd3d::TextureRecord_Destroy(textureRecord);
    const int releaseOk =
        g_smokeComReleaseCalls == 3 &&
        g_smokeComReleaseObjects[0] == &uploadSurface &&
        g_smokeComReleaseObjects[1] == &textureSurface &&
        g_smokeComReleaseObjects[2] == &texture;

    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    return defaultSkipOk != 0 && releaseOk != 0 ? 0 : 2;
}

static int RunZVideoReleaseAllInterfacesAndSurfacesSmoke(int useAtExitWrapper) {
    SmokeComObject d3dMaterial{};
    SmokeComObject d3dViewport{};
    SmokeComObject d3dDevice{};
    SmokeComObject d3d{};
    SmokeComObject clipper{};
    SmokeComObject palette{};
    InstallSmokeComReleaseObject(d3dMaterial);
    InstallSmokeComReleaseObject(d3dViewport);
    InstallSmokeComReleaseObject(d3dDevice);
    InstallSmokeComReleaseObject(d3d);
    InstallSmokeComReleaseObject(clipper);
    InstallSmokeComReleaseObject(palette);
    ResetSmokeComReleaseTracking();

    SmokeDirectDrawSurface3Object zBufferSurface{};
    SmokeDirectDrawSurface3Object swSurface{};
    SmokeDirectDrawSurface3Object primarySurface{};
    SmokeDirectDrawSurface3Object displaySurface{};
    InstallSmokeDirectDrawSurface3(zBufferSurface);
    InstallSmokeDirectDrawSurface3(swSurface);
    InstallSmokeDirectDrawSurface3(primarySurface);
    InstallSmokeDirectDrawSurface3(displaySurface);

    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;

    g_zVideo_pD3DMaterial2 = (IDirect3DMaterial2 *)(&d3dMaterial);
    g_zVideo_pD3DViewport2 = (IDirect3DViewport2 *)(&d3dViewport);
    g_zVideo_pD3DDevice = (IDirect3DDevice2 *)(&d3dDevice);
    g_zVideo_pD3D2 = (IDirect3D2 *)(&d3d);
    g_zVideo_pClipper = (IDirectDrawClipper *)(&clipper);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_pDDPalette = (IDirectDrawPalette *)(&palette);
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_SwSurfaceState.pageLockActive = 1;
    g_zVideo_PrimarySurfaceState.surf =
        (IDirectDrawSurface3 *)(&primarySurface);
    g_zVideo_PrimarySurfaceState.pageLockActive = 1;
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);

    int result = 0;
    if (useAtExitWrapper != 0) {
        zVideo::AtExitReleaseAllInterfacesAndSurfaces();
    } else {
        result = zVideo_dd::ReleaseAllInterfacesAndSurfaces();
    }
    const int ok =
        result == 0 &&
        g_smokeComReleaseCalls == 6 &&
        g_smokeComReleaseObjects[0] == &d3dMaterial &&
        g_smokeComReleaseObjects[1] == &d3dViewport &&
        g_smokeComReleaseObjects[2] == &d3dDevice &&
        g_smokeComReleaseObjects[3] == &d3d &&
        g_smokeComReleaseObjects[4] == &clipper &&
        g_smokeComReleaseObjects[5] == &palette &&
        g_smokeDirectDrawSurface3ReleaseCalls == 4 &&
        g_smokeDirectDrawSurface3ReleaseSurfaces[0] ==
            (IDirectDrawSurface3 *)(&zBufferSurface) &&
        g_smokeDirectDrawSurface3ReleaseSurfaces[1] ==
            (IDirectDrawSurface3 *)(&swSurface) &&
        g_smokeDirectDrawSurface3ReleaseSurfaces[2] ==
            (IDirectDrawSurface3 *)(&primarySurface) &&
        g_smokeDirectDrawSurface3ReleaseSurfaces[3] ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3PageUnlockCalls == 2 &&
        g_smokeDirectDrawSurface3LastPageUnlockSurface ==
            (IDirectDrawSurface3 *)(&primarySurface) &&
        g_smokeDirectDrawSurface3LastPageUnlockFlags == 0 &&
        g_zVideo_pD3DMaterial2 == 0 &&
        g_zVideo_pD3DViewport2 == 0 &&
        g_zVideo_pD3DDevice == 0 &&
        g_zVideo_pD3D2 == 0 &&
        g_zVideo_pClipper == 0 &&
        g_zVideo_pZBufferSurface == 0 &&
        g_zVideo_pDDPalette == 0 &&
        g_zVideo_SwSurfaceState.surf == 0 &&
        g_zVideo_PrimarySurfaceState.surf == 0 &&
        g_zVideo_DisplayModeSurfaceState.surf == 0 &&
        g_zVideo_SwSurfaceState.pageLockActive == 0 &&
        g_zVideo_PrimarySurfaceState.pageLockActive == 0;

    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pDDPalette = savedPalette;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd_release_all_interfaces_and_surfaces_smoke(void) {
    return RunZVideoReleaseAllInterfacesAndSurfacesSmoke(0);
}

extern "C" int zvideo_at_exit_release_all_interfaces_and_surfaces_smoke(void) {
    return RunZVideoReleaseAllInterfacesAndSurfacesSmoke(1);
}

extern "C" int zvideo_dd_verify_surface_state_locking_smoke(void) {
    SmokeSurfaceLockVerifierObject verifier{};
    zVideo_SurfaceLockVerifier *const savedVerifier = g_zVideo_pSurfaceLockVerifier;
    const unsigned char savedFlags = g_zVideo_SurfaceLockVerifyFlags;

    InstallSmokeSurfaceLockVerifier(verifier);
    g_zVideo_pSurfaceLockVerifier = (zVideo_SurfaceLockVerifier *)(&verifier);
    g_zVideo_SurfaceLockVerifyFlags = 0;
    zVideo_dd::VerifySurfaceStateLocking(0x12345678);
    if (g_smokeSurfaceLockVerifierVerifyCalls != 0) {
        g_zVideo_pSurfaceLockVerifier = savedVerifier;
        g_zVideo_SurfaceLockVerifyFlags = savedFlags;
        return 1;
    }

    g_zVideo_SurfaceLockVerifyFlags = 0x20;
    zVideo_dd::VerifySurfaceStateLocking(0x12345678);
    const int ok =
        g_smokeSurfaceLockVerifierVerifyCalls == 1 &&
        g_smokeSurfaceLockVerifierLastArgs.size ==
            sizeof(g_smokeSurfaceLockVerifierLastArgs) &&
        g_smokeSurfaceLockVerifierLastArgs.callerContext == 0x12345678;

    g_zVideo_pSurfaceLockVerifier = savedVerifier;
    g_zVideo_SurfaceLockVerifyFlags = savedFlags;
    return ok != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_teardown_video_subsystem_smoke(void) {
    SmokeDirectDrawSurface3Object pageUnlockSurface{};
    SmokeDirectDraw2Object directDraw{};
    SmokeSurfaceLockVerifierObject verifier{};
    InstallSmokeDirectDrawSurface3(pageUnlockSurface);
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeSurfaceLockVerifier(verifier);

    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawSurface3 *const savedPageUnlock = g_zVideo_pPageUnlockSurface;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;
    zVideo_SurfaceLockVerifier *const savedVerifier = g_zVideo_pSurfaceLockVerifier;
    const int savedVerifyContext = g_zVideo_SurfaceLockVerifyContext;
    const unsigned char savedVerifyFlags = g_zVideo_SurfaceLockVerifyFlags;
    const HWND savedHwnd = g_zVideo_hWnd;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;

    g_zVideo_pD3DMaterial2 = 0;
    g_zVideo_pD3DViewport2 = 0;
    g_zVideo_pD3DDevice = 0;
    g_zVideo_pD3D2 = 0;
    g_zVideo_pClipper = 0;
    g_zVideo_pZBufferSurface = 0;
    g_zVideo_pDDPalette = 0;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_pPageUnlockSurface =
        (IDirectDrawSurface3 *)(&pageUnlockSurface);
    g_zVideo_pSurfaceLockVerifier = (zVideo_SurfaceLockVerifier *)(&verifier);
    g_zVideo_SurfaceLockVerifyContext = 0x13579;
    g_zVideo_SurfaceLockVerifyFlags = 0x20;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = (HWND)(0x2468);

    zVideo_dd::TeardownVideoSubsystem();
    const int ok =
        g_smokeDirectDrawSurface3PageUnlockCalls == 1 &&
        g_smokeDirectDrawSurface3LastPageUnlockSurface ==
            (IDirectDrawSurface3 *)(&pageUnlockSurface) &&
        g_smokeDirectDrawSurface3LastPageUnlockFlags == 0 &&
        g_smokeDirectDrawSurface3ReleaseCalls == 1 &&
        g_smokeDirectDrawSurface3ReleaseSurfaces[0] ==
            (IDirectDrawSurface3 *)(&pageUnlockSurface) &&
        g_smokeSurfaceLockVerifierVerifyCalls == 1 &&
        g_smokeSurfaceLockVerifierLastArgs.callerContext == 0x13579 &&
        g_smokeSurfaceLockVerifierReleaseCalls == 1 &&
        g_smokeDirectDraw2SetCooperativeLevelCalls == 1 &&
        g_smokeDirectDraw2LastSetCooperativeSelf == (IDirectDraw2 *)(&directDraw) &&
        g_smokeDirectDraw2LastSetCooperativeHwnd == (HWND)(0x2468) &&
        g_smokeDirectDraw2LastSetCooperativeFlags == 8 &&
        g_smokeDirectDraw2ReleaseCalls == 1 &&
        g_smokeDirectDraw2ReleaseSelf == (IDirectDraw2 *)(&directDraw) &&
        g_zVideo_pPageUnlockSurface == 0 &&
        g_zVideo_pSurfaceLockVerifier == 0 &&
        g_zVideo_pDirectDraw2 == 0;

    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pPageUnlockSurface = savedPageUnlock;
    g_zVideo_pDDPalette = savedPalette;
    g_zVideo_pSurfaceLockVerifier = savedVerifier;
    g_zVideo_SurfaceLockVerifyContext = savedVerifyContext;
    g_zVideo_SurfaceLockVerifyFlags = savedVerifyFlags;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd_shutdown_video_system_smoke(void) {
    zVideo_TextureRecordPartial *const savedDefaultRecord =
        g_zImage_DefaultTextureRecord;
    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawSurface3 *const savedPageUnlock = g_zVideo_pPageUnlockSurface;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;
    zVideo_SurfaceLockVerifier *const savedVerifier = g_zVideo_pSurfaceLockVerifier;
    const int savedVerifyContext = g_zVideo_SurfaceLockVerifyContext;
    const unsigned char savedVerifyFlags = g_zVideo_SurfaceLockVerifyFlags;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;

    g_zVideo_pD3DMaterial2 = 0;
    g_zVideo_pD3DViewport2 = 0;
    g_zVideo_pD3DDevice = 0;
    g_zVideo_pD3D2 = 0;
    g_zVideo_pClipper = 0;
    g_zVideo_pDirectDraw2 = 0;
    g_zVideo_pZBufferSurface = 0;
    g_zVideo_pPageUnlockSurface = 0;
    g_zVideo_pDDPalette = 0;
    g_zVideo_pSurfaceLockVerifier = 0;
    g_zVideo_SurfaceLockVerifyContext = 0;
    g_zVideo_SurfaceLockVerifyFlags = 0;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};

    zVideo_TextureRecordPartial defaultRecord{};
    g_zImage_DefaultTextureRecord = &defaultRecord;
    ResetSmokeComReleaseTracking();
    const int defaultResult = zVideo_dd::ShutdownVideoSystem();
    const int defaultOk =
        defaultResult == 0 &&
        g_zImage_DefaultTextureRecord == 0 &&
        g_smokeComReleaseCalls == 0;

    g_zImage_DefaultTextureRecord = 0;
    const int noDefaultResult = zVideo_dd::ShutdownVideoSystem();
    const int noDefaultOk =
        noDefaultResult == 0 &&
        g_zImage_DefaultTextureRecord == 0;

    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pPageUnlockSurface = savedPageUnlock;
    g_zVideo_pDDPalette = savedPalette;
    g_zVideo_pSurfaceLockVerifier = savedVerifier;
    g_zVideo_SurfaceLockVerifyContext = savedVerifyContext;
    g_zVideo_SurfaceLockVerifyFlags = savedVerifyFlags;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return defaultOk != 0 && noDefaultOk != 0 ? 0 : 2;
}

static int SmokeDirectDrawSurface3BltFxOk(
    int index,
    DWORD flags,
    DWORD fillValue
) {
    return g_smokeDirectDrawSurface3BltFxPresent[index] != 0 &&
           g_smokeDirectDrawSurface3BltFlags[index] == flags &&
           g_smokeDirectDrawSurface3BltFxValues[index].dwSize ==
               sizeof(DDBLTFX) &&
           g_smokeDirectDrawSurface3BltFxValues[index].dwFillDepth ==
               fillValue;
}

extern "C" int zvideo_dd_zbuffer_depth_fill_rect_smoke(void) {
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;

    SmokeDirectDrawSurface3Object zBufferSurface{};
    zVidRect32 rect = {1, 2, 21, 22};
    IDirectDrawSurface3 *const zBufferInterface =
        (IDirectDrawSurface3 *)(&zBufferSurface);

    InstallSmokeDirectDrawSurface3(zBufferSurface);
    g_zVideo_pZBufferSurface = 0;
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    const int nullOk =
        g_smokeDirectDrawSurface3BltCalls == 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0;

    InstallSmokeDirectDrawSurface3(zBufferSurface);
    g_zVideo_pZBufferSurface = zBufferInterface;
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    const int successOk =
        g_smokeDirectDrawSurface3BltCalls == 1 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == zBufferInterface &&
        g_smokeDirectDrawSurface3LastBltDstRectArg == (RECT *)(&rect) &&
        g_smokeDirectDrawSurface3LastBltSource == 0 &&
        g_smokeDirectDrawSurface3LastBltSrcRectArg == 0 &&
        SmokeDirectDrawSurface3BltFxOk(
            0,
            DDBLT_DEPTHFILL,
            0
        ) != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0;

    InstallSmokeDirectDrawSurface3(zBufferSurface);
    g_zVideo_pZBufferSurface = zBufferInterface;
    g_smokeDirectDrawSurface3BltResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3BltResults[1] = DD_OK;
    g_smokeDirectDrawSurface3BltResultCount = 2;
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    const int retryOk =
        g_smokeDirectDrawSurface3BltCalls == 2 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == zBufferInterface &&
        g_smokeDirectDrawSurface3BltSurfaces[1] == zBufferInterface &&
        SmokeDirectDrawSurface3BltFxOk(
            1,
            DDBLT_DEPTHFILL,
            0
        ) != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] == zBufferInterface;

    g_zVideo_pZBufferSurface = savedZBuffer;
    InstallSmokeDirectDrawSurface3(zBufferSurface);
    return nullOk != 0 && successOk != 0 && retryOk != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_clear_screen_and_zbuffer_rect_smoke(void) {
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    const int savedClearScreen = g_zVideo_ClearScreenBufferEnabled;
    const unsigned int savedClearColor = g_zVideo_ClearColorPacked16;

    SmokeDirectDrawSurface3Object colorSurface{};
    SmokeDirectDrawSurface3Object zBufferSurface{};
    zVideo_SurfaceStatePartial colorSurfaceState{};
    zVidRect32 rect = {3, 4, 31, 32};
    IDirectDrawSurface3 *const colorInterface =
        (IDirectDrawSurface3 *)(&colorSurface);
    IDirectDrawSurface3 *const zBufferInterface =
        (IDirectDrawSurface3 *)(&zBufferSurface);

    InstallSmokeDirectDrawSurface3(colorSurface);
    zBufferSurface.vtable = g_smokeDirectDrawSurface3VTable;
    colorSurfaceState.surf = colorInterface;
    g_zVideo_pZBufferSurface = zBufferInterface;
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x1357;
    g_smokeDirectDrawSurface3BltResults[0] = DD_OK;
    g_smokeDirectDrawSurface3BltResults[1] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3BltResults[2] = DD_OK;
    g_smokeDirectDrawSurface3BltResultCount = 3;

    zVideo_dd::ClearScreenAndZBufferRect(
        &rect,
        &colorSurfaceState
    );
    const int enabledOk =
        g_smokeDirectDrawSurface3BltCalls == 3 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == colorInterface &&
        g_smokeDirectDrawSurface3BltSurfaces[1] == zBufferInterface &&
        g_smokeDirectDrawSurface3BltSurfaces[2] == zBufferInterface &&
        SmokeDirectDrawSurface3BltFxOk(
            0,
            DDBLT_COLORFILL | DDBLT_WAIT,
            0x1357
        ) != 0 &&
        SmokeDirectDrawSurface3BltFxOk(
            2,
            DDBLT_DEPTHFILL,
            0
        ) != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] == zBufferInterface;

    InstallSmokeDirectDrawSurface3(colorSurface);
    zBufferSurface.vtable = g_smokeDirectDrawSurface3VTable;
    colorSurfaceState.surf = colorInterface;
    g_zVideo_pZBufferSurface = zBufferInterface;
    g_zVideo_ClearScreenBufferEnabled = 0;
    zVideo_dd::ClearScreenAndZBufferRect(
        &rect,
        &colorSurfaceState
    );
    const int disabledOk =
        g_smokeDirectDrawSurface3BltCalls == 1 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == zBufferInterface &&
        SmokeDirectDrawSurface3BltFxOk(
            0,
            DDBLT_DEPTHFILL,
            0
        ) != 0;

    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_ClearScreenBufferEnabled = savedClearScreen;
    g_zVideo_ClearColorPacked16 = savedClearColor;
    InstallSmokeDirectDrawSurface3(colorSurface);
    return enabledOk != 0 && disabledOk != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_clear_sw_backbuffer_and_zbuffer_rects_smoke(void) {
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    const int savedClearScreen = g_zVideo_ClearScreenBufferEnabled;
    const unsigned int savedClearColor = g_zVideo_ClearColorPacked16;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    SmokeDirectDrawSurface3Object swSurface{};
    SmokeDirectDrawSurface3Object zBufferSurface{};
    zVidRect32 colorRect = {5, 6, 41, 42};
    zVidRect32 zRect = {7, 8, 43, 44};
    IDirectDrawSurface3 *const swInterface =
        (IDirectDrawSurface3 *)(&swSurface);
    IDirectDrawSurface3 *const zBufferInterface =
        (IDirectDrawSurface3 *)(&zBufferSurface);

    InstallSmokeDirectDrawSurface3(swSurface);
    zBufferSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_pZBufferSurface = zBufferInterface;
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x2468;
    g_smokeDirectDrawSurface3BltResults[0] = DD_OK;
    g_smokeDirectDrawSurface3BltResults[1] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3BltResults[2] = DD_OK;
    g_smokeDirectDrawSurface3BltResultCount = 3;

    zVideo_dd::ClearSwBackbufferAndZBufferRects(
        &colorRect,
        &zRect
    );
    const int enabledOk =
        g_smokeDirectDrawSurface3BltCalls == 3 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == swInterface &&
        g_smokeDirectDrawSurface3BltSurfaces[1] == zBufferInterface &&
        g_smokeDirectDrawSurface3BltSurfaces[2] == zBufferInterface &&
        SmokeDirectDrawSurface3BltFxOk(
            0,
            DDBLT_COLORFILL | DDBLT_WAIT,
            0x2468
        ) != 0 &&
        SmokeDirectDrawSurface3BltFxOk(
            2,
            DDBLT_DEPTHFILL,
            0
        ) != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] == zBufferInterface &&
        g_smokeDirectDrawSurface3LastBltDstRectArg == (RECT *)(&zRect);

    InstallSmokeDirectDrawSurface3(swSurface);
    zBufferSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_pZBufferSurface = zBufferInterface;
    g_zVideo_ClearScreenBufferEnabled = 0;
    zVideo_dd::ClearSwBackbufferAndZBufferRects(
        &colorRect,
        &zRect
    );
    const int disabledOk =
        g_smokeDirectDrawSurface3BltCalls == 1 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == zBufferInterface &&
        g_smokeDirectDrawSurface3LastBltDstRectArg == (RECT *)(&zRect) &&
        SmokeDirectDrawSurface3BltFxOk(
            0,
            DDBLT_DEPTHFILL,
            0
        ) != 0;

    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_ClearScreenBufferEnabled = savedClearScreen;
    g_zVideo_ClearColorPacked16 = savedClearColor;
    g_zVideo_SwSurfaceState = savedSwState;
    InstallSmokeDirectDrawSurface3(swSurface);
    return enabledOk != 0 && disabledOk != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_palette_set_entries_smoke(void) {
    const int savedBpp = g_zVideo_DisplayModeBpp;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;

    SmokeDirectDrawPaletteObject palette{};
    PALETTEENTRY entries[3] = {};
    entries[0].peRed = 10;
    entries[1].peGreen = 20;
    entries[2].peBlue = 30;

    InstallSmokeDirectDrawPalette(palette);
    g_zVideo_DisplayModeBpp = 16;
    g_zVideo_pDDPalette = (IDirectDrawPalette *)(&palette);
    const int non8BppOk =
        zVideo_dd::PaletteSetEntries(1, 2, entries) == 0 &&
        g_smokeDirectDrawPaletteSetEntriesCalls == 0;

    InstallSmokeDirectDrawPalette(palette);
    g_zVideo_DisplayModeBpp = 8;
    g_zVideo_pDDPalette = (IDirectDrawPalette *)(&palette);
    const int successOk =
        zVideo_dd::PaletteSetEntries(5, 3, entries) == 0 &&
        g_smokeDirectDrawPaletteSetEntriesCalls == 1 &&
        g_smokeDirectDrawPaletteLastSetEntriesSelf ==
            (IDirectDrawPalette *)(&palette) &&
        g_smokeDirectDrawPaletteLastSetEntriesFlags == 0 &&
        g_smokeDirectDrawPaletteLastSetEntriesFirst == 5 &&
        g_smokeDirectDrawPaletteLastSetEntriesCount == 3 &&
        g_smokeDirectDrawPaletteLastSetEntriesEntries == entries;

    InstallSmokeDirectDrawPalette(palette);
    g_zVideo_DisplayModeBpp = 8;
    g_zVideo_pDDPalette = (IDirectDrawPalette *)(&palette);
    g_smokeDirectDrawPaletteSetEntriesResult = DDERR_GENERIC;
    const int failureOk =
        zVideo_dd::PaletteSetEntries(7, 1, entries) == 0x5a56ffff &&
        g_smokeDirectDrawPaletteSetEntriesCalls == 1 &&
        g_smokeDirectDrawPaletteLastSetEntriesFlags == 0 &&
        g_smokeDirectDrawPaletteLastSetEntriesFirst == 7 &&
        g_smokeDirectDrawPaletteLastSetEntriesCount == 1 &&
        g_smokeDirectDrawPaletteLastSetEntriesEntries == entries;

    g_zVideo_DisplayModeBpp = savedBpp;
    g_zVideo_pDDPalette = savedPalette;
    return non8BppOk != 0 && successOk != 0 && failureOk != 0 ? 0 : 1;
}

extern "C" int zvideo_get_display_mode_bpp_smoke(void) {
    const int savedBpp = g_zVideo_DisplayModeBpp;

    g_zVideo_DisplayModeBpp = 16;
    const int firstOk = zVideo::GetDisplayModeBpp() == 16;

    g_zVideo_DisplayModeBpp = 8;
    const int secondOk = zVideo::GetDisplayModeBpp() == 8;

    g_zVideo_DisplayModeBpp = savedBpp;
    return firstOk != 0 && secondOk != 0 ? 0 : 1;
}

extern "C" int zvideo_surface_accessors_smoke(void) {
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;

    int swPixels = 0x1234;
    int primaryPixels = 0x5678;
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 200;
    g_zVideo_SwSurfaceState.pitch = 640;
    g_zVideo_SwSurfaceState.locked = 1;
    g_zVideo_SwSurfaceState.pixels = &swPixels;
    g_zVideo_PrimarySurfaceState.width = 800;
    g_zVideo_PrimarySurfaceState.height = 600;
    g_zVideo_PrimarySurfaceState.pitch = 1600;
    g_zVideo_PrimarySurfaceState.pixels = &primaryPixels;

    const int ok =
        zVideo::GetSwSurfacePixels() == &swPixels &&
        zVideo::GetSwSurfaceWidth() == 320 &&
        zVideo::GetSwSurfaceHeight() == 200 &&
        zVideo::GetSwSurfacePitch() == 640 &&
        zVideo::GetSwSurfaceLockedFlag() == 1 &&
        zVideo::GetPrimarySurfacePixels() == &primaryPixels &&
        zVideo::GetPrimarySurfaceWidth() == 800 &&
        zVideo::GetPrimarySurfaceHeight() == 600 &&
        zVideo::GetPrimarySurfacePitch() == 1600;

    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_primary_surface_rect_scratch_smoke(void) {
    const zVidRect32 savedScratch = g_zVideo_PrimarySurfaceRectScratch;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;

    g_zVideo_PrimarySurfaceRectScratch.left = 11;
    g_zVideo_PrimarySurfaceRectScratch.top = 22;
    g_zVideo_PrimarySurfaceRectScratch.right = 33;
    g_zVideo_PrimarySurfaceRectScratch.bottom = 44;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    zVidRect32 *const scratch = zVideo::GetPrimarySurfaceRectScratch();
    const int ok =
        scratch == &g_zVideo_PrimarySurfaceRectScratch &&
        scratch->left == 11 &&
        scratch->top == 22 &&
        scratch->right == 640 &&
        scratch->bottom == 480;

    g_zVideo_PrimarySurfaceRectScratch = savedScratch;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd_set_display_mode_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    const HWND savedHwnd = g_zVideo_hWnd;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const int savedDisplayModeBpp = g_zVideo_DisplayModeBpp;

    SmokeDirectDraw2Object directDraw{};

    InstallSmokeDirectDraw2(directDraw);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = (HWND)(0x1357);
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;
    g_zVideo_DisplayModeBpp = 16;
    const int successOk =
        zVideo_dd::SetDisplayMode() == 1 &&
        g_smokeDirectDraw2SetCooperativeLevelCalls == 1 &&
        g_smokeDirectDraw2LastSetCooperativeHwnd == (HWND)(0x1357) &&
        g_smokeDirectDraw2LastSetCooperativeFlags == 0x13 &&
        g_smokeDirectDraw2SetDisplayModeCalls == 1 &&
        g_smokeDirectDraw2LastSetDisplayModeSelf ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeDirectDraw2LastDisplayModeWidth == 800 &&
        g_smokeDirectDraw2LastDisplayModeHeight == 600 &&
        g_smokeDirectDraw2LastDisplayModeBpp == 16 &&
        g_smokeDirectDraw2LastDisplayModeRefreshRate == 0 &&
        g_smokeDirectDraw2LastDisplayModeFlags == 0;

    InstallSmokeDirectDraw2(directDraw);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = (HWND)(0x2468);
    g_smokeDirectDraw2SetCooperativeLevelResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState.width = 1024;
    g_zVideo_DisplayModeSurfaceState.height = 768;
    g_zVideo_DisplayModeBpp = 32;
    const int cooperativeFailureOk =
        zVideo_dd::SetDisplayMode() == 0 &&
        g_smokeDirectDraw2SetCooperativeLevelCalls == 1 &&
        g_smokeDirectDraw2LastSetCooperativeHwnd == (HWND)(0x2468) &&
        g_smokeDirectDraw2LastSetCooperativeFlags == 0x13 &&
        g_smokeDirectDraw2SetDisplayModeCalls == 0;

    InstallSmokeDirectDraw2(directDraw);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = (HWND)(0x369c);
    g_smokeDirectDraw2SetDisplayModeResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState.width = 640;
    g_zVideo_DisplayModeSurfaceState.height = 480;
    g_zVideo_DisplayModeBpp = 8;
    const int displayFailureOk =
        zVideo_dd::SetDisplayMode() == 0 &&
        g_smokeDirectDraw2SetCooperativeLevelCalls == 1 &&
        g_smokeDirectDraw2SetDisplayModeCalls == 1 &&
        g_smokeDirectDraw2LastDisplayModeWidth == 640 &&
        g_smokeDirectDraw2LastDisplayModeHeight == 480 &&
        g_smokeDirectDraw2LastDisplayModeBpp == 8 &&
        g_smokeDirectDraw2LastDisplayModeRefreshRate == 0 &&
        g_smokeDirectDraw2LastDisplayModeFlags == 0;

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_DisplayModeBpp = savedDisplayModeBpp;
    return successOk != 0 && cooperativeFailureOk != 0 && displayFailureOk != 0
        ? 0
        : 1;
}

int g_smokeSetVideoModeStepCount;
int g_smokeSetVideoModeSteps[8];
int g_smokeSetVideoModeRestoreCalls;
int g_smokeSetVideoModeSetDisplayModeResult;
int g_smokeSetVideoModeRestoreResults[2];
int g_smokeSetVideoModeReleaseResult;
int g_smokeSetVideoModeCreateSurfacesResult;
int g_smokeSetVideoModeCreateDeviceResult;
int g_smokeSetVideoModeVerifyLocksResult;

void RecordSetVideoModeStep(
    int step
) {
    if (g_smokeSetVideoModeStepCount < 8) {
        g_smokeSetVideoModeSteps[g_smokeSetVideoModeStepCount] = step;
    }
    ++g_smokeSetVideoModeStepCount;
}

int SmokeSetVideoMode_SetDisplayMode() {
    RecordSetVideoModeStep(1);
    return g_smokeSetVideoModeSetDisplayModeResult;
}

int SmokeSetVideoMode_RestoreDisplaySurfaces() {
    RecordSetVideoModeStep(2);
    const int resultIndex = g_smokeSetVideoModeRestoreCalls;
    ++g_smokeSetVideoModeRestoreCalls;
    return resultIndex < 2 ? g_smokeSetVideoModeRestoreResults[resultIndex] : 0;
}

int SmokeSetVideoMode_ReleaseAllInterfacesAndSurfaces() {
    RecordSetVideoModeStep(3);
    return g_smokeSetVideoModeReleaseResult;
}

int SmokeSetVideoMode_CreateFullscreenSurfacesForRenderer() {
    RecordSetVideoModeStep(4);
    return g_smokeSetVideoModeCreateSurfacesResult;
}

int SmokeSetVideoMode_CreateDeviceState() {
    RecordSetVideoModeStep(5);
    return g_smokeSetVideoModeCreateDeviceResult;
}

int SmokeSetVideoMode_VerifyFullscreenSurfaceLocks() {
    RecordSetVideoModeStep(6);
    return g_smokeSetVideoModeVerifyLocksResult;
}

void ResetSetVideoModeCapture() {
    g_smokeSetVideoModeStepCount = 0;
    std::memset(
        g_smokeSetVideoModeSteps,
        0,
        sizeof(g_smokeSetVideoModeSteps)
    );
    g_smokeSetVideoModeRestoreCalls = 0;
    g_smokeSetVideoModeSetDisplayModeResult = 1;
    g_smokeSetVideoModeRestoreResults[0] = 0;
    g_smokeSetVideoModeRestoreResults[1] = 0;
    g_smokeSetVideoModeReleaseResult = 0;
    g_smokeSetVideoModeCreateSurfacesResult = 0;
    g_smokeSetVideoModeCreateDeviceResult = 0;
    g_smokeSetVideoModeVerifyLocksResult = 0;
}

extern "C" int zvideo_dd_set_video_mode_smoke(void) {
    SmokeFunctionPatch setDisplayModePatch = {};
    SmokeFunctionPatch restoreSurfacesPatch = {};
    SmokeFunctionPatch releaseSurfacesPatch = {};
    SmokeFunctionPatch createSurfacesPatch = {};
    SmokeFunctionPatch createDevicePatch = {};
    SmokeFunctionPatch verifyLocksPatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::SetDisplayMode),
            (void *)(&SmokeSetVideoMode_SetDisplayMode),
            setDisplayModePatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::RestoreDisplaySurfaces),
            (void *)(&SmokeSetVideoMode_RestoreDisplaySurfaces),
            restoreSurfacesPatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::ReleaseAllInterfacesAndSurfaces),
            (void *)(&SmokeSetVideoMode_ReleaseAllInterfacesAndSurfaces),
            releaseSurfacesPatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateFullscreenSurfacesForRenderer),
            (void *)(&SmokeSetVideoMode_CreateFullscreenSurfacesForRenderer),
            createSurfacesPatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd3d::CreateDeviceState),
            (void *)(&SmokeSetVideoMode_CreateDeviceState),
            createDevicePatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::VerifyFullscreenSurfaceLocks),
            (void *)(&SmokeSetVideoMode_VerifyFullscreenSurfaceLocks),
            verifyLocksPatch
        )) {
        RestoreSmokeFunctionPatch(verifyLocksPatch);
        RestoreSmokeFunctionPatch(createDevicePatch);
        RestoreSmokeFunctionPatch(createSurfacesPatch);
        RestoreSmokeFunctionPatch(releaseSurfacesPatch);
        RestoreSmokeFunctionPatch(restoreSurfacesPatch);
        RestoreSmokeFunctionPatch(setDisplayModePatch);
        return 1;
    }

    const int savedRendererType = g_zVideo_RendererType;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 0;
    const int softwareResult = zVideo_dd::SetVideoMode(77);
    const int softwareOk =
        softwareResult == 0 &&
        g_smokeSetVideoModeStepCount == 6 &&
        g_smokeSetVideoModeSteps[0] == 1 &&
        g_smokeSetVideoModeSteps[1] == 2 &&
        g_smokeSetVideoModeSteps[2] == 3 &&
        g_smokeSetVideoModeSteps[3] == 4 &&
        g_smokeSetVideoModeSteps[4] == 2 &&
        g_smokeSetVideoModeSteps[5] == 6 &&
        g_smokeSetVideoModeRestoreCalls == 2;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 1;
    const int hardwareResult = zVideo_dd::SetVideoMode(88);
    const int hardwareOk =
        hardwareResult == 0 &&
        g_smokeSetVideoModeStepCount == 7 &&
        g_smokeSetVideoModeSteps[0] == 1 &&
        g_smokeSetVideoModeSteps[1] == 2 &&
        g_smokeSetVideoModeSteps[2] == 3 &&
        g_smokeSetVideoModeSteps[3] == 4 &&
        g_smokeSetVideoModeSteps[4] == 5 &&
        g_smokeSetVideoModeSteps[5] == 2 &&
        g_smokeSetVideoModeSteps[6] == 6 &&
        g_smokeSetVideoModeRestoreCalls == 2;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 1;
    g_smokeSetVideoModeSetDisplayModeResult = 0;
    const int setDisplayFailureResult = zVideo_dd::SetVideoMode(99);
    const int setDisplayFailureOk =
        setDisplayFailureResult == 1 &&
        g_smokeSetVideoModeStepCount == 1 &&
        g_smokeSetVideoModeSteps[0] == 1;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 1;
    g_smokeSetVideoModeCreateDeviceResult = 1;
    const int createDeviceFailureResult = zVideo_dd::SetVideoMode(100);
    const int createDeviceFailureOk =
        createDeviceFailureResult == 1 &&
        g_smokeSetVideoModeStepCount == 5 &&
        g_smokeSetVideoModeSteps[4] == 5 &&
        g_smokeSetVideoModeRestoreCalls == 1;

    g_zVideo_RendererType = savedRendererType;
    RestoreSmokeFunctionPatch(verifyLocksPatch);
    RestoreSmokeFunctionPatch(createDevicePatch);
    RestoreSmokeFunctionPatch(createSurfacesPatch);
    RestoreSmokeFunctionPatch(releaseSurfacesPatch);
    RestoreSmokeFunctionPatch(restoreSurfacesPatch);
    RestoreSmokeFunctionPatch(setDisplayModePatch);

    return softwareOk != 0 &&
            hardwareOk != 0 &&
            setDisplayFailureOk != 0 &&
            createDeviceFailureOk != 0
        ? 0
        : 2;
}

extern "C" int zvideo_dd_restore_display_surfaces_smoke(void) {
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    SmokeDirectDrawSurface3Object displaySurface{};
    SmokeDirectDrawSurface3Object primarySurface{};
    SmokeDirectDrawSurface3Object swSurface{};

    InstallSmokeDirectDrawSurface3(displaySurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    g_zVideo_PrimarySurfaceState.surf =
        (IDirectDrawSurface3 *)(&primarySurface);
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    const int allPresentOk =
        zVideo_dd::RestoreDisplaySurfaces() == 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 3 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] ==
            g_zVideo_DisplayModeSurfaceState.surf &&
        g_smokeDirectDrawSurface3RestoreSurfaces[1] ==
            g_zVideo_PrimarySurfaceState.surf &&
        g_smokeDirectDrawSurface3RestoreSurfaces[2] ==
            g_zVideo_SwSurfaceState.surf;

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_smokeDirectDrawSurface3RestoreResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    const int displayFailureOk =
        zVideo_dd::RestoreDisplaySurfaces() != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] ==
            g_zVideo_DisplayModeSurfaceState.surf;

    InstallSmokeDirectDrawSurface3(primarySurface);
    g_smokeDirectDrawSurface3RestoreResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState.surf =
        (IDirectDrawSurface3 *)(&primarySurface);
    const int primaryFailureOk =
        zVideo_dd::RestoreDisplaySurfaces() != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] ==
            g_zVideo_PrimarySurfaceState.surf;

    InstallSmokeDirectDrawSurface3(swSurface);
    g_smokeDirectDrawSurface3RestoreResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    const int swFailureOk =
        zVideo_dd::RestoreDisplaySurfaces() != 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] ==
            g_zVideo_SwSurfaceState.surf;

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    const int noSurfaceOk =
        zVideo_dd::RestoreDisplaySurfaces() == 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return allPresentOk != 0 &&
            displayFailureOk != 0 &&
            primaryFailureOk != 0 &&
            swFailureOk != 0 &&
            noSurfaceOk != 0
        ? 0
        : 1;
}

extern "C" int zvideo_dd_init_fullscreen_software_pixel_pack_smoke(void) {
    SmokeDirectDrawSurface3Object displaySurface{};
    InstallSmokeDirectDrawSurface3(displaySurface);

    const zVideo_PixelPackParams savedPixelPack = g_zVideo_PixelPack;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);

    g_smokeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    g_smokeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    g_smokeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;
    const int firstOk =
        zVideo_dd::InitFullscreenSoftwarePixelPack(
            (IDirectDrawSurface3 *)(&displaySurface)
        ) == 0 &&
        g_smokeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        g_smokeDirectDrawSurface3LastPixelFormatSurface ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastPixelFormatInputSize ==
            sizeof(DDPIXELFORMAT) &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 &&
        g_zVideo_PixelPack.bMask == 0x001f;

    g_smokeDirectDrawSurface3PixelFormat.dwRBitMask = 0x7c00;
    g_smokeDirectDrawSurface3PixelFormat.dwGBitMask = 0x03e0;
    g_smokeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;
    const int secondOk =
        zVideo_dd::InitFullscreenSoftwarePixelPack(
            (IDirectDrawSurface3 *)(&displaySurface)
        ) == 0 &&
        g_smokeDirectDrawSurface3GetPixelFormatCalls == 2 &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 5 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0x7c00 &&
        g_zVideo_PixelPack.gMask == 0x03e0 &&
        g_zVideo_PixelPack.bMask == 0x001f;

    g_smokeDirectDrawSurface3PixelFormat.dwRBitMask = 0xff0000;
    g_smokeDirectDrawSurface3PixelFormat.dwGBitMask = 0xff00;
    g_smokeDirectDrawSurface3PixelFormat.dwBBitMask = 0x00ff;
    const int thirdOk =
        zVideo_dd::InitFullscreenSoftwarePixelPack(
            (IDirectDrawSurface3 *)(&displaySurface)
        ) == 0 &&
        g_smokeDirectDrawSurface3GetPixelFormatCalls == 3 &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0xff0000 &&
        g_zVideo_PixelPack.gMask == 0xff00 &&
        g_zVideo_PixelPack.bMask == 0x00ff;

    g_zVideo_PixelPack = savedPixelPack;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return firstOk != 0 && secondOk != 0 && thirdOk != 0 ? 0 : 1;
}

int g_smokeCreateSurface3FromDescCalls;
IDirectDraw2 *g_smokeCreateSurface3FromDescDirectDraws[4];
DDSURFACEDESC g_smokeCreateSurface3FromDescDescs[4];
IDirectDrawSurface3 **g_smokeCreateSurface3FromDescOuts[4];
IDirectDrawSurface3 *g_smokeCreateSurface3FromDescResults[4];
HRESULT g_smokeCreateSurface3FromDescResult;
int g_smokeInitFullscreenSoftwarePixelPackCalls;
IDirectDrawSurface3 *g_smokeInitFullscreenSoftwarePixelPackSurface;
int g_smokeInitFullscreenSoftwarePixelPackResult;
zOptionEntryPartial g_smokeHalfResGfxFlagsOption;
const char *g_smokeHalfResOptionName;

HRESULT __fastcall SmokeCreateSurface3FromDesc(
    IDirectDraw2 *directDraw,
    DDSURFACEDESC *desc,
    IDirectDrawSurface3 **surface,
    int reserved
) {
    reserved;
    const int callIndex = g_smokeCreateSurface3FromDescCalls;
    if (callIndex < 4) {
        g_smokeCreateSurface3FromDescDirectDraws[callIndex] = directDraw;
        g_smokeCreateSurface3FromDescDescs[callIndex] = *desc;
        g_smokeCreateSurface3FromDescOuts[callIndex] = surface;
    }
    ++g_smokeCreateSurface3FromDescCalls;
    if (g_smokeCreateSurface3FromDescResult == DD_OK && callIndex < 4) {
        *surface = g_smokeCreateSurface3FromDescResults[callIndex];
    }
    return g_smokeCreateSurface3FromDescResult;
}

int __fastcall SmokeInitFullscreenSoftwarePixelPack(
    IDirectDrawSurface3 *surface
) {
    ++g_smokeInitFullscreenSoftwarePixelPackCalls;
    g_smokeInitFullscreenSoftwarePixelPackSurface = surface;
    return g_smokeInitFullscreenSoftwarePixelPackResult;
}

zOptionEntryPartial *__fastcall SmokeFindHalfResGfxFlagsOption(
    const char *name
) {
    g_smokeHalfResOptionName = name;
    return &g_smokeHalfResGfxFlagsOption;
}

void ResetHalfResSurfaceBuilderTracking() {
    g_smokeCreateSurface3FromDescCalls = 0;
    std::memset(
        g_smokeCreateSurface3FromDescDirectDraws,
        0,
        sizeof(g_smokeCreateSurface3FromDescDirectDraws)
    );
    std::memset(
        g_smokeCreateSurface3FromDescDescs,
        0,
        sizeof(g_smokeCreateSurface3FromDescDescs)
    );
    std::memset(
        g_smokeCreateSurface3FromDescOuts,
        0,
        sizeof(g_smokeCreateSurface3FromDescOuts)
    );
    std::memset(
        g_smokeCreateSurface3FromDescResults,
        0,
        sizeof(g_smokeCreateSurface3FromDescResults)
    );
    g_smokeCreateSurface3FromDescResult = DD_OK;
    g_smokeInitFullscreenSoftwarePixelPackCalls = 0;
    g_smokeInitFullscreenSoftwarePixelPackSurface = 0;
    g_smokeInitFullscreenSoftwarePixelPackResult = 0;
    g_smokeHalfResGfxFlagsOption = {};
    g_smokeHalfResOptionName = 0;
}

extern "C" int zvideo_dd_create_half_res_backbuffer_surfaces_smoke(void) {
    SmokeDirectDraw2Object directDraw{};
    SmokeDirectDrawSurface3Object displaySurface{};
    SmokeDirectDrawSurface3Object primarySurface{};
    SmokeDirectDrawSurface3Object swSurface{};
    SmokeDirectDrawClipperObject clipper{};
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface3(displaySurface);
    InstallSmokeDirectDrawClipper(clipper);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    ResetHalfResSurfaceBuilderTracking();

    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    const int savedPrimaryHasAttachedBackbuffer =
        g_zVideo_PrimaryHasAttachedBackbuffer;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedRendererType = g_zVideo_RendererType;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    HWND const savedHwnd = g_zVideo_hWnd;

    SmokeFunctionPatch createSurfacePatch = {};
    SmokeFunctionPatch pixelPackPatch = {};
    SmokeFunctionPatch findOptionPatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateSurface3FromDesc),
            (void *)(&SmokeCreateSurface3FromDesc),
            createSurfacePatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::InitFullscreenSoftwarePixelPack),
            (void *)(&SmokeInitFullscreenSoftwarePixelPack),
            pixelPackPatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zGame::Options_FindOption),
            (void *)(&SmokeFindHalfResGfxFlagsOption),
            findOptionPatch
        )) {
        RestoreSmokeFunctionPatch(findOptionPatch);
        RestoreSmokeFunctionPatch(pixelPackPatch);
        RestoreSmokeFunctionPatch(createSurfacePatch);
        return 1;
    }

    zVidHwApiDeviceRecordPartial selectedDevice = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pClipper = 0;
    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_RendererType = 1;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = (HWND)(0x4321);
    g_smokeCreateSurface3FromDescResults[0] =
        (IDirectDrawSurface3 *)(&displaySurface);
    g_smokeCreateSurface3FromDescResults[1] =
        (IDirectDrawSurface3 *)(&swSurface);
    g_smokeDirectDrawSurface3AttachedSurfaceValue =
        (IDirectDrawSurface3 *)(&primarySurface);
    g_smokeDirectDraw2CreateClipperValue =
        (IDirectDrawClipper *)(&clipper);

    const int result = zVideo_dd::CreateHalfResBackbufferSurfaces();

    const int ok =
        result == 0 &&
        g_smokeHalfResOptionName != 0 &&
        std::strcmp(g_smokeHalfResOptionName, "GfxFlags_SW") == 0 &&
        g_smokeCreateSurface3FromDescCalls == 2 &&
        g_smokeCreateSurface3FromDescDirectDraws[0] ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeCreateSurface3FromDescOuts[0] ==
            &g_zVideo_DisplayModeSurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[0].dwSize ==
            sizeof(DDSURFACEDESC) &&
        g_smokeCreateSurface3FromDescDescs[0].dwFlags == 0x21 &&
        g_smokeCreateSurface3FromDescDescs[0].dwBackBufferCount == 1 &&
        g_smokeCreateSurface3FromDescDescs[0].ddsCaps.dwCaps == 0x218 &&
        g_smokeDirectDrawSurface3GetAttachedSurfaceCalls == 1 &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceSelf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceCaps.dwCaps ==
            DDSCAPS_BACKBUFFER &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceOut ==
            &g_zVideo_PrimarySurfaceState.surf &&
        g_zVideo_DisplayModeSurfaceState.surf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_zVideo_PrimarySurfaceState.surf ==
            (IDirectDrawSurface3 *)(&primarySurface) &&
        g_zVideo_PrimaryHasAttachedBackbuffer == 1 &&
        g_smokeCreateSurface3FromDescDirectDraws[1] ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeCreateSurface3FromDescOuts[1] ==
            &g_zVideo_SwSurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[1].dwFlags == 0x07 &&
        g_smokeCreateSurface3FromDescDescs[1].ddsCaps.dwCaps == 0x840 &&
        g_smokeCreateSurface3FromDescDescs[1].dwWidth == 320 &&
        g_smokeCreateSurface3FromDescDescs[1].dwHeight == 240 &&
        g_zVideo_SwSurfaceState.surf == (IDirectDrawSurface3 *)(&swSurface) &&
        g_smokeInitFullscreenSoftwarePixelPackCalls == 1 &&
        g_smokeInitFullscreenSoftwarePixelPackSurface ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDraw2CreateClipperCalls == 1 &&
        g_smokeDirectDraw2LastCreateClipperSelf ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeDirectDraw2LastCreateClipperFlags == 0 &&
        g_smokeDirectDraw2LastCreateClipperOut == &g_zVideo_pClipper &&
        g_smokeDirectDraw2LastCreateClipperOuter == 0 &&
        g_zVideo_pClipper == (IDirectDrawClipper *)(&clipper) &&
        g_smokeDirectDrawClipperSetHWndCalls == 1 &&
        g_smokeDirectDrawClipperLastSetHWndSelf ==
            (IDirectDrawClipper *)(&clipper) &&
        g_smokeDirectDrawClipperLastSetHWndFlags == 0 &&
        g_smokeDirectDrawClipperLastSetHWndValue == (HWND)(0x4321) &&
        g_smokeDirectDrawSurface3SetClipperCalls == 1 &&
        g_smokeDirectDrawSurface3LastSetClipperSelf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastSetClipperValue ==
            (IDirectDrawClipper *)(&clipper);

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryHasAttachedBackbuffer;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_hWnd = savedHwnd;
    RestoreSmokeFunctionPatch(findOptionPatch);
    RestoreSmokeFunctionPatch(pixelPackPatch);
    RestoreSmokeFunctionPatch(createSurfacePatch);
    return ok != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_create_fullscreen_software_surfaces_smoke(void) {
    SmokeDirectDraw2Object directDraw{};
    SmokeDirectDrawSurface3Object displaySurface{};
    SmokeDirectDrawSurface3Object primarySurface{};
    SmokeDirectDrawSurface3Object swSurface{};
    SmokeDirectDrawClipperObject clipper{};
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface3(displaySurface);
    InstallSmokeDirectDrawClipper(clipper);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    ResetHalfResSurfaceBuilderTracking();

    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    const int savedPrimaryHasAttachedBackbuffer =
        g_zVideo_PrimaryHasAttachedBackbuffer;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    HWND const savedHwnd = g_zVideo_hWnd;
    const zVideo_PixelPackParams savedPixelPack = g_zVideo_PixelPack;

    SmokeFunctionPatch createSurfacePatch = {};
    SmokeFunctionPatch findOptionPatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateSurface3FromDesc),
            (void *)(&SmokeCreateSurface3FromDesc),
            createSurfacePatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zGame::Options_FindOption),
            (void *)(&SmokeFindHalfResGfxFlagsOption),
            findOptionPatch
        )) {
        RestoreSmokeFunctionPatch(findOptionPatch);
        RestoreSmokeFunctionPatch(createSurfacePatch);
        return 1;
    }

    zVidHwApiDeviceRecordPartial selectedDevice = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pClipper = 0;
    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = (HWND)(0x7654);
    g_smokeCreateSurface3FromDescResults[0] =
        (IDirectDrawSurface3 *)(&displaySurface);
    g_smokeCreateSurface3FromDescResults[1] =
        (IDirectDrawSurface3 *)(&primarySurface);
    g_smokeCreateSurface3FromDescResults[2] =
        (IDirectDrawSurface3 *)(&swSurface);
    g_smokeDirectDraw2CreateClipperValue =
        (IDirectDrawClipper *)(&clipper);
    g_smokeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    g_smokeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    g_smokeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;

    const int result = zVideo_dd::CreateFullscreenSoftwareSurfaces();

    const int ok =
        result == 0 &&
        g_smokeHalfResOptionName != 0 &&
        std::strcmp(g_smokeHalfResOptionName, "GfxFlags_SW") == 0 &&
        g_smokeCreateSurface3FromDescCalls == 3 &&
        g_smokeCreateSurface3FromDescDirectDraws[0] ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeCreateSurface3FromDescOuts[0] ==
            &g_zVideo_DisplayModeSurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[0].dwSize ==
            sizeof(DDSURFACEDESC) &&
        g_smokeCreateSurface3FromDescDescs[0].dwFlags == 1 &&
        g_smokeCreateSurface3FromDescDescs[0].ddsCaps.dwCaps == 0x0a00 &&
        g_smokeCreateSurface3FromDescOuts[1] ==
            &g_zVideo_PrimarySurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[1].dwFlags == 7 &&
        g_smokeCreateSurface3FromDescDescs[1].ddsCaps.dwCaps == 0x0840 &&
        g_smokeCreateSurface3FromDescDescs[1].dwWidth == 640 &&
        g_smokeCreateSurface3FromDescDescs[1].dwHeight == 480 &&
        g_smokeCreateSurface3FromDescOuts[2] ==
            &g_zVideo_SwSurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[2].dwFlags == 7 &&
        g_smokeCreateSurface3FromDescDescs[2].ddsCaps.dwCaps == 0x0840 &&
        g_smokeCreateSurface3FromDescDescs[2].dwWidth == 640 &&
        g_smokeCreateSurface3FromDescDescs[2].dwHeight == 480 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3LastLockFlags == DDLOCK_WAIT &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3ReleaseCalls == 0 &&
        g_smokeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        g_smokeDirectDrawSurface3LastPixelFormatSurface ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDraw2CreateClipperCalls == 1 &&
        g_smokeDirectDraw2LastCreateClipperFlags == 0 &&
        g_smokeDirectDraw2LastCreateClipperOuter == 0 &&
        g_zVideo_pClipper == (IDirectDrawClipper *)(&clipper) &&
        g_smokeDirectDrawClipperSetHWndCalls == 1 &&
        g_smokeDirectDrawClipperLastSetHWndFlags == 0 &&
        g_smokeDirectDrawClipperLastSetHWndValue == (HWND)(0x7654) &&
        g_smokeDirectDrawSurface3SetClipperCalls == 1 &&
        g_smokeDirectDrawSurface3LastSetClipperSelf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastSetClipperValue ==
            (IDirectDrawClipper *)(&clipper) &&
        g_zVideo_DisplayModeSurfaceState.surf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_zVideo_PrimarySurfaceState.surf ==
            (IDirectDrawSurface3 *)(&primarySurface) &&
        g_zVideo_SwSurfaceState.surf == (IDirectDrawSurface3 *)(&swSurface) &&
        g_zVideo_DisplayModeSurfaceState.width == 640 &&
        g_zVideo_DisplayModeSurfaceState.height == 480 &&
        g_zVideo_DisplayModeSurfaceState.locked == 0 &&
        g_zVideo_PrimaryHasAttachedBackbuffer == 0 &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 &&
        g_zVideo_PixelPack.bMask == 0x001f;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryHasAttachedBackbuffer;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_FullscreenOption = savedFullscreenOption;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_PixelPack = savedPixelPack;
    RestoreSmokeFunctionPatch(findOptionPatch);
    RestoreSmokeFunctionPatch(createSurfacePatch);
    return ok != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_create_fullscreen_hw_surfaces_smoke(void) {
    SmokeDirectDraw2Object directDraw{};
    SmokeDirectDrawSurface3Object displaySurface{};
    SmokeDirectDrawSurface3Object primarySurface{};
    SmokeDirectDrawSurface3Object swSurface{};
    SmokeDirectDrawClipperObject clipper{};
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface3(displaySurface);
    InstallSmokeDirectDrawClipper(clipper);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    ResetHalfResSurfaceBuilderTracking();

    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    const int savedPrimaryHasAttachedBackbuffer =
        g_zVideo_PrimaryHasAttachedBackbuffer;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    HWND const savedHwnd = g_zVideo_hWnd;
    const zVideo_PixelPackParams savedPixelPack = g_zVideo_PixelPack;

    SmokeFunctionPatch createSurfacePatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateSurface3FromDesc),
            (void *)(&SmokeCreateSurface3FromDesc),
            createSurfacePatch
        )) {
        return 1;
    }

    zVidHwApiDeviceRecordPartial selectedDevice = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pClipper = 0;
    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = (HWND)(0x789a);
    g_smokeCreateSurface3FromDescResults[0] =
        (IDirectDrawSurface3 *)(&displaySurface);
    g_smokeCreateSurface3FromDescResults[1] =
        (IDirectDrawSurface3 *)(&primarySurface);
    g_smokeDirectDrawSurface3AttachedSurfaceValue =
        (IDirectDrawSurface3 *)(&swSurface);
    g_smokeDirectDraw2CreateClipperValue =
        (IDirectDrawClipper *)(&clipper);
    g_smokeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    g_smokeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    g_smokeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;

    const int result = zVideo_dd::CreateFullscreenHardwareSurfaces();

    const int ok =
        result == 0 &&
        g_smokeCreateSurface3FromDescCalls == 2 &&
        g_smokeCreateSurface3FromDescDirectDraws[0] ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeCreateSurface3FromDescOuts[0] ==
            &g_zVideo_DisplayModeSurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[0].dwSize ==
            sizeof(DDSURFACEDESC) &&
        g_smokeCreateSurface3FromDescDescs[0].dwBackBufferCount == 1 &&
        g_smokeCreateSurface3FromDescDescs[0].dwFlags == 0x21 &&
        g_smokeCreateSurface3FromDescDescs[0].ddsCaps.dwCaps == 0x2218 &&
        g_smokeDirectDrawSurface3GetAttachedSurfaceCalls == 1 &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceSelf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceCaps.dwCaps ==
            DDSCAPS_BACKBUFFER &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceOut ==
            &g_zVideo_SwSurfaceState.surf &&
        g_smokeCreateSurface3FromDescDirectDraws[1] ==
            (IDirectDraw2 *)(&directDraw) &&
        g_smokeCreateSurface3FromDescOuts[1] ==
            &g_zVideo_PrimarySurfaceState.surf &&
        g_smokeCreateSurface3FromDescDescs[1].dwFlags == 7 &&
        g_smokeCreateSurface3FromDescDescs[1].ddsCaps.dwCaps == 0x0840 &&
        g_smokeCreateSurface3FromDescDescs[1].dwWidth == 800 &&
        g_smokeCreateSurface3FromDescDescs[1].dwHeight == 600 &&
        g_smokeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        g_smokeDirectDrawSurface3LastPixelFormatSurface ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDraw2CreateClipperCalls == 1 &&
        g_smokeDirectDraw2LastCreateClipperFlags == 0 &&
        g_smokeDirectDraw2LastCreateClipperOuter == 0 &&
        g_zVideo_pClipper == (IDirectDrawClipper *)(&clipper) &&
        g_smokeDirectDrawClipperSetHWndCalls == 1 &&
        g_smokeDirectDrawClipperLastSetHWndFlags == 0 &&
        g_smokeDirectDrawClipperLastSetHWndValue == (HWND)(0x789a) &&
        g_smokeDirectDrawSurface3SetClipperCalls == 1 &&
        g_smokeDirectDrawSurface3LastSetClipperSelf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastSetClipperValue ==
            (IDirectDrawClipper *)(&clipper) &&
        g_zVideo_DisplayModeSurfaceState.surf ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_zVideo_PrimarySurfaceState.surf ==
            (IDirectDrawSurface3 *)(&primarySurface) &&
        g_zVideo_SwSurfaceState.surf == (IDirectDrawSurface3 *)(&swSurface) &&
        g_zVideo_DisplayModeSurfaceState.width == 800 &&
        g_zVideo_DisplayModeSurfaceState.height == 600 &&
        g_zVideo_PrimaryHasAttachedBackbuffer == 1 &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 &&
        g_zVideo_PixelPack.bMask == 0x001f;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryHasAttachedBackbuffer;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_PixelPack = savedPixelPack;
    RestoreSmokeFunctionPatch(createSurfacePatch);
    return ok != 0 ? 0 : 2;
}

int g_smokeCreateFullscreenHalfResCalls;
int g_smokeCreateFullscreenSoftwareCalls;
int g_smokeCreateFullscreenHardwareCalls;

int SmokeCreateFullscreenHalfResSurfaces() {
    ++g_smokeCreateFullscreenHalfResCalls;
    return 11;
}

int SmokeCreateFullscreenSoftwareSurfaces() {
    ++g_smokeCreateFullscreenSoftwareCalls;
    return 22;
}

int SmokeCreateFullscreenHardwareSurfaces() {
    ++g_smokeCreateFullscreenHardwareCalls;
    return 33;
}

extern "C" int zvideo_dd_create_fullscreen_surfaces_for_renderer_smoke(void) {
    SmokeFunctionPatch halfResPatch = {};
    SmokeFunctionPatch softwarePatch = {};
    SmokeFunctionPatch hardwarePatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateHalfResBackbufferSurfaces),
            (void *)(&SmokeCreateFullscreenHalfResSurfaces),
            halfResPatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateFullscreenSoftwareSurfaces),
            (void *)(&SmokeCreateFullscreenSoftwareSurfaces),
            softwarePatch
        ) ||
        !PatchSmokeFunctionJump(
            (void *)(&zVideo_dd::CreateFullscreenHardwareSurfaces),
            (void *)(&SmokeCreateFullscreenHardwareSurfaces),
            hardwarePatch
        )) {
        RestoreSmokeFunctionPatch(hardwarePatch);
        RestoreSmokeFunctionPatch(softwarePatch);
        RestoreSmokeFunctionPatch(halfResPatch);
        return 1;
    }

    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedRendererType = g_zVideo_RendererType;
    g_smokeCreateFullscreenHalfResCalls = 0;
    g_smokeCreateFullscreenSoftwareCalls = 0;
    g_smokeCreateFullscreenHardwareCalls = 0;

    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_RendererType = 1;
    const int halfResResult = zVideo_dd::CreateFullscreenSurfacesForRenderer();

    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 1;
    const int hardwareResult = zVideo_dd::CreateFullscreenSurfacesForRenderer();

    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 0;
    const int softwareResult = zVideo_dd::CreateFullscreenSurfacesForRenderer();

    const int ok =
        halfResResult == 11 &&
        hardwareResult == 33 &&
        softwareResult == 22 &&
        g_smokeCreateFullscreenHalfResCalls == 1 &&
        g_smokeCreateFullscreenHardwareCalls == 1 &&
        g_smokeCreateFullscreenSoftwareCalls == 1;

    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    g_zVideo_RendererType = savedRendererType;
    RestoreSmokeFunctionPatch(hardwarePatch);
    RestoreSmokeFunctionPatch(softwarePatch);
    RestoreSmokeFunctionPatch(halfResPatch);
    return ok != 0 ? 0 : 2;
}

extern "C" int zvideo_dd_present_display_mode_surface_smoke(void) {
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedSwapScratch =
        g_zVideo_SurfaceStateSwapScratch;
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedHalfResAdjustMode = g_zVideo_HalfResAdjustMode;

    SmokeDirectDrawSurface3Object displaySurface = {};
    SmokeDirectDrawSurface3Object primarySurface = {};
    SmokeDirectDrawSurface3Object swSurface = {};
    zVidRect32 srcRect = {1, 2, 9, 10};
    zVidRect32 dstRect = {3, 4, 11, 12};
    IDirectDrawSurface3 *const displayInterface =
        (IDirectDrawSurface3 *)(&displaySurface);
    IDirectDrawSurface3 *const primaryInterface =
        (IDirectDrawSurface3 *)(&primarySurface);
    IDirectDrawSurface3 *const swInterface =
        (IDirectDrawSurface3 *)(&swSurface);

    InstallSmokeDirectDrawSurface3(displaySurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_HalfResAdjustMode = 0;

    const int halfResOk =
        zVideo_dd::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            0,
            0
        ) == 0 &&
        g_smokeDirectDrawSurface3BltCalls == 1 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == displayInterface &&
        g_smokeDirectDrawSurface3LastBltSource == primaryInterface &&
        g_smokeDirectDrawSurface3LastBltDstRectArg == (RECT *)(&dstRect) &&
        g_smokeDirectDrawSurface3LastBltSrcRectArg == (RECT *)(&srcRect) &&
        g_smokeDirectDrawSurface3LastBltFlags == (DDBLT_WAIT | DDBLT_ASYNC) &&
        g_smokeDirectDrawSurface3LastBltFx == 0 &&
        g_smokeDirectDrawSurface3PageLockCalls == 0 &&
        g_smokeDirectDrawSurface3PageUnlockCalls == 0;

    InstallSmokeDirectDrawSurface3(displaySurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SurfaceStateSwapScratch = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 1;

    const int adjustedSwapOk =
        zVideo_dd::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            1,
            0
        ) == 0 &&
        g_smokeDirectDrawSurface3PageLockCalls == 1 &&
        g_smokeDirectDrawSurface3LastPageLockSurface == primaryInterface &&
        g_smokeDirectDrawSurface3LastPageLockFlags == 0 &&
        g_smokeDirectDrawSurface3PageUnlockCalls == 0 &&
        g_smokeDirectDrawSurface3BltCalls == 1 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] == displayInterface &&
        g_smokeDirectDrawSurface3LastBltSource == primaryInterface &&
        g_smokeDirectDrawSurface3LastBltFlags == DDBLT_ASYNC &&
        g_zVideo_PrimarySurfaceState.surf == swInterface &&
        g_zVideo_SwSurfaceState.surf == primaryInterface &&
        g_zVideo_PrimarySurfaceState.width == 320 &&
        g_zVideo_SwSurfaceState.width == 640;

    InstallSmokeDirectDrawSurface3(displaySurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 1;

    const int adjustedSkipSwapOk =
        zVideo_dd::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            1,
            1
        ) == 0 &&
        g_smokeDirectDrawSurface3PageLockCalls == 1 &&
        g_smokeDirectDrawSurface3PageUnlockCalls == 1 &&
        g_smokeDirectDrawSurface3LastPageUnlockSurface == primaryInterface &&
        g_smokeDirectDrawSurface3LastPageUnlockFlags == 0 &&
        g_zVideo_PrimarySurfaceState.pageLockActive == 0;

    InstallSmokeDirectDrawSurface3(displaySurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_smokeDirectDrawSurface3BltResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3BltResults[1] = DD_OK;
    g_smokeDirectDrawSurface3BltResultCount = 2;
    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_HalfResAdjustMode = 0;

    const int restoreRetryOk =
        zVideo_dd::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            1,
            0
        ) == 0 &&
        g_smokeDirectDrawSurface3BltCalls == 2 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] == displayInterface &&
        g_smokeDirectDrawSurface3LastBltFlags == DDBLT_WAIT;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_SurfaceStateSwapScratch = savedSwapScratch;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    g_zVideo_HalfResAdjustMode = savedHalfResAdjustMode;
    InstallSmokeDirectDrawSurface3(displaySurface);
    return halfResOk != 0 && adjustedSwapOk != 0 &&
                   adjustedSkipSwapOk != 0 && restoreRetryOk != 0
               ? 0
               : 1;
}

extern "C" int zvideo_present_display_mode_surface_null_smoke(void) {
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;

    g_zVideo_DisplayModeSurfaceState.surf = 0;
    zVidRect32 rect = {};
    const int ok =
        zVideo_dd3d::PresentDisplayModeSurface(
            &rect,
            &rect,
            0,
            0
        ) == 0x400;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd3d_present_display_mode_surface_smoke(void) {
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    SmokeDirectDrawSurface3Object displaySurface = {};
    SmokeDirectDrawSurface3Object primarySurface = {};
    SmokeDirectDrawSurface3Object swSurface = {};
    zVidRect32 srcRect = {1, 2, 11, 12};
    zVidRect32 dstRect = {3, 4, 13, 14};

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    const int simpleFlipOk =
        zVideo_dd3d::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            0,
            0
        ) == 0 &&
        g_smokeDirectDrawSurface3FlipCalls == 1 &&
        g_smokeDirectDrawSurface3FlipSurfaces[0] ==
            (IDirectDrawSurface3 *)(&displaySurface) &&
        g_smokeDirectDrawSurface3LastFlipTarget == 0 &&
        g_smokeDirectDrawSurface3LastFlipFlags == 0 &&
        g_smokeDirectDrawSurface3BltCalls == 0 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0;

    InstallSmokeDirectDrawSurface3(displaySurface);
    primarySurface.vtable = g_smokeDirectDrawSurface3VTable;
    swSurface.vtable = g_smokeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    g_zVideo_PrimarySurfaceState.surf =
        (IDirectDrawSurface3 *)(&primarySurface);
    g_zVideo_SwSurfaceState.surf =
        (IDirectDrawSurface3 *)(&swSurface);
    const int blitAndWaitOk =
        zVideo_dd3d::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            1,
            1
        ) == 0 &&
        g_smokeDirectDrawSurface3BltCalls == 1 &&
        g_smokeDirectDrawSurface3BltSurfaces[0] ==
            (IDirectDrawSurface3 *)(&swSurface) &&
        g_smokeDirectDrawSurface3LastBltDstRectArg == (RECT *)(&dstRect) &&
        g_smokeDirectDrawSurface3LastBltSource ==
            (IDirectDrawSurface3 *)(&primarySurface) &&
        g_smokeDirectDrawSurface3LastBltSrcRectArg == (RECT *)(&srcRect) &&
        g_smokeDirectDrawSurface3LastBltFlags == DDBLT_WAIT &&
        g_smokeDirectDrawSurface3LastBltFx == 0 &&
        g_smokeDirectDrawSurface3FlipCalls == 1 &&
        g_smokeDirectDrawSurface3LastFlipTarget == 0 &&
        g_smokeDirectDrawSurface3LastFlipFlags == DDFLIP_WAIT;

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_smokeDirectDrawSurface3FlipResults[0] = DDERR_WASSTILLDRAWING;
    g_smokeDirectDrawSurface3FlipResults[1] = DD_OK;
    g_smokeDirectDrawSurface3FlipResultCount = 2;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    const int stillDrawingRetryOk =
        zVideo_dd3d::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            0,
            0
        ) == 0 &&
        g_smokeDirectDrawSurface3FlipCalls == 2 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0;

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_smokeDirectDrawSurface3FlipResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3FlipResults[1] = DD_OK;
    g_smokeDirectDrawSurface3FlipResultCount = 2;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    const int surfaceLostRetryOk =
        zVideo_dd3d::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            0,
            0
        ) == 0 &&
        g_smokeDirectDrawSurface3FlipCalls == 2 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreSurfaces[0] ==
            (IDirectDrawSurface3 *)(&displaySurface);

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_smokeDirectDrawSurface3FlipResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3FlipResults[1] = DD_OK;
    g_smokeDirectDrawSurface3FlipResultCount = 2;
    g_smokeDirectDrawSurface3RestoreResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    const int restoreFailureOk =
        zVideo_dd3d::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            0,
            0
        ) == 0x5a56ffff &&
        g_smokeDirectDrawSurface3FlipCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1;

    InstallSmokeDirectDrawSurface3(displaySurface);
    g_smokeDirectDrawSurface3FlipResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3FlipResults[1] = DD_OK;
    g_smokeDirectDrawSurface3FlipResultCount = 2;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        (IDirectDrawSurface3 *)(&displaySurface);
    const int flipFailureOk =
        zVideo_dd3d::PresentDisplayModeSurface(
            &srcRect,
            &dstRect,
            0,
            0
        ) == 0x5a56ffff &&
        g_smokeDirectDrawSurface3FlipCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return simpleFlipOk != 0 && blitAndWaitOk != 0 &&
                   stillDrawingRetryOk != 0 && surfaceLostRetryOk != 0 &&
                   restoreFailureOk != 0 && flipFailureOk != 0
               ? 0
               : 1;
}

extern "C" int zvideo_image_lazy_create_backing_surface_guards_smoke(void) {
    zVidImagePartial image = {};
    image.width = 8;
    image.height = 4;
    image.pixels = (void *)(0x12345678);
    image.alphaMap = (char *)(0x1234);

    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != 0) {
        return 1;
    }

    image.alphaMap = 0;
    image.pixels = 0;
    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != 0) {
        return 2;
    }

    image.pixels = (void *)(0x12345678);
    image.width = 0;
    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != 0) {
        return 3;
    }

    image.width = 8;
    image.height = 0;
    return zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == 0 ? 0 : 4;
}

extern "C" int zvideo_dd_image_populate_surface_from_heap_pixels_smoke(void) {
    SmokeDirectDrawSurface3Object surface = {};
    unsigned char lockedPixels[24];
    std::memset(lockedPixels, 0xcc, sizeof(lockedPixels));

    unsigned char *sourcePixels = (unsigned char *)std::malloc(12);
    if (sourcePixels == 0) {
        return 1;
    }
    for (int i = 0; i < 12; ++i) {
        sourcePixels[i] = (unsigned char)(i + 1);
    }

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3LockPixelsValue = lockedPixels;
    g_smokeDirectDrawSurface3LockPitchValue = 10;

    zVidImagePartial image = {};
    image.width = 3;
    image.height = 2;
    image.pixels = sourcePixels;
    image.surface = (IDirectDrawSurface3 *)(&surface);

    const int successResult = zVideo_dd::Image_PopulateSurfaceFromHeapPixels(&image);
    const int successOk =
        successResult == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3LastLockRect == 0 &&
        g_smokeDirectDrawSurface3LastLockFlags == DDLOCK_WAIT &&
        g_smokeDirectDrawSurface3LastLockEvent == 0 &&
        g_smokeDirectDrawSurface3LockDescSize == sizeof(DDSURFACEDESC) &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3LastUnlockArg == g_smokeDirectDrawSurface3LastLockDesc &&
        image.pixels == lockedPixels &&
        image.pitchWords == 5 &&
        lockedPixels[0] == 1 &&
        lockedPixels[1] == 2 &&
        lockedPixels[2] == 3 &&
        lockedPixels[3] == 4 &&
        lockedPixels[4] == 5 &&
        lockedPixels[5] == 6 &&
        lockedPixels[6] == 0xcc &&
        lockedPixels[7] == 0xcc &&
        lockedPixels[8] == 0xcc &&
        lockedPixels[9] == 0xcc &&
        lockedPixels[10] == 7 &&
        lockedPixels[11] == 8 &&
        lockedPixels[12] == 9 &&
        lockedPixels[13] == 10 &&
        lockedPixels[14] == 11 &&
        lockedPixels[15] == 12;

    unsigned char retryPixels[12];
    std::memset(retryPixels, 0xdd, sizeof(retryPixels));
    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        return 2;
    }
    sourcePixels[0] = 0x31;
    sourcePixels[1] = 0x32;
    sourcePixels[2] = 0x33;
    sourcePixels[3] = 0x34;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3LockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3LockResultCount = 2;
    g_smokeDirectDrawSurface3LockPixelsValue = retryPixels;
    g_smokeDirectDrawSurface3LockPitchValue = 6;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    image.surface = (IDirectDrawSurface3 *)(&surface);

    const int retryResult = zVideo_dd::Image_PopulateSurfaceFromHeapPixels(&image);
    const int retryOk =
        retryResult == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 2 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        image.pixels == retryPixels &&
        image.pitchWords == 3 &&
        retryPixels[0] == 0x31 &&
        retryPixels[1] == 0x32 &&
        retryPixels[2] == 0x33 &&
        retryPixels[3] == 0x34;

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        return 3;
    }
    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3LockResults[1] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3LockResultCount = 2;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const int lockFailureResult = zVideo_dd::Image_PopulateSurfaceFromHeapPixels(&image);
    const int lockFailureOk =
        lockFailureResult == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3UnlockCalls == 0 &&
        image.pixels == sourcePixels;
    std::free(sourcePixels);

    return successOk && retryOk && lockFailureOk ? 0 : 4;
}

extern "C" int zvideo_dd_image_lazy_create_backing_surface_smoke(void) {
    SmokeDirectDraw2Object directDraw = {};
    SmokeDirectDrawSurfaceObject baseSurface = {};
    SmokeDirectDrawSurface3Object surface3 = {};
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;

    unsigned char lockedPixels[24];
    std::memset(lockedPixels, 0xcc, sizeof(lockedPixels));
    unsigned char *sourcePixels = (unsigned char *)std::malloc(12);
    if (sourcePixels == 0) {
        return 1;
    }
    for (int i = 0; i < 12; ++i) {
        sourcePixels[i] = (unsigned char)(0x41 + i);
    }

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_smokeDirectDrawSurface3LockPixelsValue = lockedPixels;
    g_smokeDirectDrawSurface3LockPitchValue = 8;

    zVidImagePartial image = {};
    image.width = 3;
    image.height = 2;
    image.pixels = sourcePixels;
    const DWORD requestedCaps = DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface3 *const result =
        zVideo_dd::Image_LazyCreateBackingSurface(&image, requestedCaps);
    const int successOk =
        result == (IDirectDrawSurface3 *)(&surface3) &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == lockedPixels &&
        image.pitchWords == 4 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDraw2LastCreateSurfaceOut != 0 &&
        g_smokeDirectDraw2LastCreateSurfaceOuter == 0 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwSize == sizeof(DDSURFACEDESC) &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwFlags == 0x10007 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwWidth == 3 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwHeight == 2 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (requestedCaps | DDSCAPS_OFFSCREENPLAIN) &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        IsEqualGUID(
            *g_smokeDirectDrawSurfaceLastQueryInterfaceIid,
            IID_IDirectDrawSurface3
        ) &&
        g_smokeDirectDrawSurfaceLastQueryInterfaceOut != 0 &&
        g_smokeDirectDrawSurfaceReleaseCalls == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        lockedPixels[0] == 0x41 &&
        lockedPixels[1] == 0x42 &&
        lockedPixels[2] == 0x43 &&
        lockedPixels[3] == 0x44 &&
        lockedPixels[4] == 0x45 &&
        lockedPixels[5] == 0x46 &&
        lockedPixels[6] == 0xcc &&
        lockedPixels[7] == 0xcc &&
        lockedPixels[8] == 0x47 &&
        lockedPixels[9] == 0x48 &&
        lockedPixels[10] == 0x49 &&
        lockedPixels[11] == 0x4a &&
        lockedPixels[12] == 0x4b &&
        lockedPixels[13] == 0x4c;

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        return 2;
    }
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDraw2CreateSurfaceResult = DDERR_GENERIC;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const int createFailureOk =
        zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == 0 &&
        image.surface == 0 &&
        image.pixels == sourcePixels &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 0;
    std::free(sourcePixels);

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        return 3;
    }
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceResult = DDERR_GENERIC;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const int queryFailureOk =
        zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == 0 &&
        image.surface == 0 &&
        image.pixels == sourcePixels &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 0;
    std::free(sourcePixels);

    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    return successOk && createFailureOk && queryFailureOk ? 0 : 4;
}

extern "C" int zvideo_dd_image_lazy_create_video_memory_surface_smoke(void) {
    SmokeDirectDraw2Object directDraw = {};
    SmokeDirectDrawSurfaceObject baseSurface = {};
    SmokeDirectDrawSurface3Object surface3 = {};
    zVidHwApiDeviceRecordPartial selectedDevice = {};
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_UseHalfResBackbuffer = 0;

    zVidImagePartial image = {};
    image.width = 2;
    image.height = 1;
    image.surface = (IDirectDrawSurface3 *)(0x1234);
    image.pixels = std::malloc(4);
    if (image.pixels == 0) {
        return 1;
    }
    const int skipOk =
        zVideo_dd::Image_LazyCreateVideoMemorySurface(&image) == 0 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 0 &&
        image.surface == (IDirectDrawSurface3 *)(0x1234);
    std::free(image.pixels);

    unsigned char halfResPixels[8];
    std::memset(halfResPixels, 0xcc, sizeof(halfResPixels));
    unsigned char *sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        return 2;
    }
    sourcePixels[0] = 0x11;
    sourcePixels[1] = 0x12;
    sourcePixels[2] = 0x13;
    sourcePixels[3] = 0x14;
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_UseHalfResBackbuffer = 1;
    g_smokeDirectDrawSurface3LockPixelsValue = halfResPixels;
    g_smokeDirectDrawSurface3LockPitchValue = 4;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    IDirectDrawSurface3 *const halfResResult =
        zVideo_dd::Image_LazyCreateVideoMemorySurface(&image);
    const int halfResCapsOk =
        halfResResult == (IDirectDrawSurface3 *)(&surface3) &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN) &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == halfResPixels &&
        image.pitchWords == 2 &&
        halfResPixels[0] == 0x11 &&
        halfResPixels[1] == 0x12 &&
        halfResPixels[2] == 0x13 &&
        halfResPixels[3] == 0x14;

    unsigned char featurePixels[8];
    std::memset(featurePixels, 0xdd, sizeof(featurePixels));
    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        return 3;
    }
    sourcePixels[0] = 0x21;
    sourcePixels[1] = 0x22;
    sourcePixels[2] = 0x23;
    sourcePixels[3] = 0x24;
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0x1357;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_smokeDirectDrawSurface3LockPixelsValue = featurePixels;
    g_smokeDirectDrawSurface3LockPitchValue = 4;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    IDirectDrawSurface3 *const featureResult =
        zVideo_dd::Image_LazyCreateVideoMemorySurface(&image);
    const int featureCapsOk =
        featureResult == (IDirectDrawSurface3 *)(&surface3) &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY |
             DDSCAPS_OFFSCREENPLAIN) &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == featurePixels &&
        image.pitchWords == 2 &&
        featurePixels[0] == 0x21 &&
        featurePixels[1] == 0x22 &&
        featurePixels[2] == 0x23 &&
        featurePixels[3] == 0x24;

    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    return skipOk && halfResCapsOk && featureCapsOk ? 0 : 4;
}

extern "C" int zvideo_dd_image_upload_pixels_to_surface_smoke(void) {
    const int savedRendererType = g_zVideo_RendererType;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    zVidHwApiDeviceRecordPartial selectedDevice = {};

    SmokeDirectDrawSurface3Object surface = {};
    InstallSmokeDirectDrawSurface3(surface);
    zVidImagePartial image = {};
    HDC hdc = (HDC)(0x1111);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    g_zVideo_RendererType = 2;
    const int rendererSkipOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 0 &&
        g_smokeDirectDrawSurface3GetDCCalls == 0 &&
        hdc == (HDC)(0x1111);

    InstallSmokeDirectDrawSurface3(surface);
    hdc = 0;
    image = {};
    image.surface = (IDirectDrawSurface3 *)(&surface);
    g_zVideo_RendererType = 0;
    const int existingSurfaceOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 1 &&
        g_smokeDirectDrawSurface3GetDCCalls == 1 &&
        g_smokeDirectDrawSurface3LastGetDCSurface == image.surface &&
        g_smokeDirectDrawSurface3LastGetDCOut == &hdc &&
        hdc == g_smokeDirectDrawSurface3GetDCValue;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3GetDCResult = DDERR_GENERIC;
    hdc = 0;
    image = {};
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const int getDcFailureOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 0 &&
        g_smokeDirectDrawSurface3GetDCCalls == 1 &&
        g_smokeDirectDrawSurface3LastGetDCOut == &hdc &&
        hdc == 0;

    SmokeDirectDraw2Object directDraw = {};
    SmokeDirectDrawSurfaceObject baseSurface = {};
    SmokeDirectDrawSurface3Object surface3 = {};
    unsigned char systemPixels[8];
    std::memset(systemPixels, 0xcc, sizeof(systemPixels));
    unsigned char *sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 1;
    }
    sourcePixels[0] = 0x31;
    sourcePixels[1] = 0x32;
    sourcePixels[2] = 0x33;
    sourcePixels[3] = 0x34;
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    g_smokeDirectDrawSurface3LockPixelsValue = systemPixels;
    g_smokeDirectDrawSurface3LockPitchValue = 4;
    hdc = 0;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const int lazySystemOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 1 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN) &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3GetDCCalls == 1 &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == systemPixels &&
        image.pitchWords == 2 &&
        hdc == g_smokeDirectDrawSurface3GetDCValue &&
        systemPixels[0] == 0x31 &&
        systemPixels[1] == 0x32 &&
        systemPixels[2] == 0x33 &&
        systemPixels[3] == 0x34;

    unsigned char featurePixels[8];
    std::memset(featurePixels, 0xdd, sizeof(featurePixels));
    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 2;
    }
    sourcePixels[0] = 0x41;
    sourcePixels[1] = 0x42;
    sourcePixels[2] = 0x43;
    sourcePixels[3] = 0x44;
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0x2468;
    g_smokeDirectDrawSurface3LockPixelsValue = featurePixels;
    g_smokeDirectDrawSurface3LockPitchValue = 4;
    hdc = 0;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const int lazyFeatureOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 1 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY |
             DDSCAPS_OFFSCREENPLAIN) &&
        g_smokeDirectDrawSurface3GetDCCalls == 1 &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == featurePixels &&
        image.pitchWords == 2 &&
        hdc == g_smokeDirectDrawSurface3GetDCValue &&
        featurePixels[0] == 0x41 &&
        featurePixels[1] == 0x42 &&
        featurePixels[2] == 0x43 &&
        featurePixels[3] == 0x44;

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == 0) {
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 3;
    }
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(baseSurface);
    InstallSmokeDirectDrawSurface3(surface3);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&baseSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &surface3;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    hdc = 0;
    image = {};
    image.alphaMap = (char *)(0x1234);
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const int lazyFailureOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 0 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 0 &&
        g_smokeDirectDrawSurface3GetDCCalls == 0 &&
        image.surface == 0 &&
        image.pixels == sourcePixels;
    std::free(sourcePixels);

    g_zVideo_RendererType = savedRendererType;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    return rendererSkipOk && existingSurfaceOk && getDcFailureOk && lazySystemOk &&
                   lazyFeatureOk && lazyFailureOk
               ? 0
               : 4;
}

extern "C" int zvideo_dd_image_release_surface_smoke(void) {
    zVidImagePartial image = {};
    SmokeDirectDrawSurface3Object surface = {};
    InstallSmokeDirectDrawSurface3(surface);

    HDC hdc = (HDC)(0x2468);
    const int nullSurfaceOk =
        zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 &&
        g_smokeDirectDrawSurface3ReleaseDCCalls == 0;

    InstallSmokeDirectDrawSurface3(surface);
    hdc = (HDC)(0x1357);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const int successOk =
        zVideo_dd::Image_ReleaseSurface(&image, hdc) == 1 &&
        g_smokeDirectDrawSurface3ReleaseDCCalls == 1 &&
        g_smokeDirectDrawSurface3LastReleaseDCSurface == image.surface &&
        g_smokeDirectDrawSurface3LastReleaseDCHdc == hdc;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3ReleaseDCResult = DDERR_GENERIC;
    hdc = (HDC)(0x9753);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const int failureOk =
        zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 &&
        g_smokeDirectDrawSurface3ReleaseDCCalls == 1 &&
        g_smokeDirectDrawSurface3LastReleaseDCSurface == image.surface &&
        g_smokeDirectDrawSurface3LastReleaseDCHdc == hdc;

    return nullSurfaceOk && successOk && failureOk ? 0 : 1;
}

extern "C" int zvideo_image_surface_helpers_guard_smoke(void) {
    const int savedVideoInitialized = g_zVideo_IsInitialized;
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedRendererType = g_zVideo_RendererType;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    zVidHwApiDeviceRecordPartial selectedDevice = {};

    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_UseHalfResBackbuffer = 0;

    zVidImagePartial image = {};
    if (zVideo_dd::Image_LazyCreateVideoMemorySurface(&image) != 0) {
        return 1;
    }

    image.surface = (IDirectDrawSurface3 *)(0x1234);
    image.pixels = (void *)(0x5678);
    g_zVideo_IsInitialized = 0;
    zVideo_dd::Image_EnsureSurfaceForCurrentDevice(&image);
    if (image.surface != 0 || image.pixels != 0) {
        g_zVideo_IsInitialized = savedVideoInitialized;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 2;
    }

    SmokeDirectDrawSurface3Object surface = {};
    InstallSmokeDirectDrawSurface3(surface);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    image.pixels = (void *)(0x5678);
    g_zVideo_IsInitialized = 1;
    zVideo_dd::Image_EnsureSurfaceForCurrentDevice(&image);
    if (g_smokeDirectDrawSurface3ReleaseCalls != 1 || image.surface != 0 ||
        image.pixels != 0) {
        g_zVideo_IsInitialized = savedVideoInitialized;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 3;
    }

    HDC hdc = 0;
    g_zVideo_RendererType = 2;
    if (zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) != 0) {
        g_zVideo_IsInitialized = savedVideoInitialized;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 4;
    }

    const int result = zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 ? 0 : 5;
    g_zVideo_IsInitialized = savedVideoInitialized;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    return result;
}

extern "C" int zvideo_set_renderer_type_smoke(void) {
    const int savedRendererType = g_zVideo_RendererType;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;

    g_zVideo_RendererType = 2;
    g_zVideo_ActiveRendererPath = 0;
    const int previous = zVideo::SetRendererTypeAndActivePath(1);
    const int ok =
        previous == 2 &&
        g_zVideo_RendererType == 1 &&
        g_zVideo_ActiveRendererPath == 1;

    g_zVideo_RendererType = savedRendererType;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    return ok != 0 ? 0 : 1;
}

namespace {
int g_smokeZVideoHalfResBltCalls;

void __fastcall SmokeZVideoHalfResBltFake(
    zVidRect32 *,
    zVidRect32 *
) {
    ++g_smokeZVideoHalfResBltCalls;
}
} // namespace

extern "C" int zvideo_set_half_res_adjust_mode_smoke(void) {
    const int savedHalfResAdjustMode = g_zVideo_HalfResAdjustMode;
    const int savedUseHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const int savedRendererType = g_zVideo_RendererType;
    const zVideo_BltRectDirectProc savedBltPrimaryToSw =
        g_zVideo_pfnBltPrimaryToSwRectDirect;

    int result = 0;
    g_smokeZVideoHalfResBltCalls = 0;
    g_zVideo_HalfResAdjustMode = 2;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_pfnBltPrimaryToSwRectDirect = SmokeZVideoHalfResBltFake;
    if (zVideo::SetHalfResAdjustMode(2) != 2 ||
        g_zVideo_HalfResAdjustMode != 2 ||
        g_smokeZVideoHalfResBltCalls != 0) {
        result = 1;
    }

    if (result == 0) {
        g_zVideo_HalfResAdjustMode = 1;
        g_zVideo_UseHalfResBackbuffer = 1;
        if (zVideo::SetHalfResAdjustMode(0) != 0 ||
            g_zVideo_HalfResAdjustMode != 1 ||
            g_smokeZVideoHalfResBltCalls != 0) {
            result = 2;
        }
    }

    if (result == 0) {
        g_zVideo_HalfResAdjustMode = 1;
        g_zVideo_UseHalfResBackbuffer = 0;
        if (zVideo::SetHalfResAdjustMode(0) != 1 ||
            g_zVideo_HalfResAdjustMode != 0 ||
            g_smokeZVideoHalfResBltCalls != 1) {
            result = 3;
        }
    }

    if (result == 0) {
        g_zVideo_HalfResAdjustMode = 0;
        g_zVideo_RendererType = 1;
        if (zVideo::SetHalfResAdjustMode(3) != 0 ||
            g_zVideo_HalfResAdjustMode != 3 ||
            g_smokeZVideoHalfResBltCalls != 1) {
            result = 4;
        }
    }

    g_zVideo_HalfResAdjustMode = savedHalfResAdjustMode;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfResBackbuffer;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_pfnBltPrimaryToSwRectDirect = savedBltPrimaryToSw;
    return result;
}

extern "C" int zvideo_flip_to_gdi_if_attached_null_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    const int savedPrimaryHasAttachedBackbuffer =
        g_zVideo_PrimaryHasAttachedBackbuffer;

    g_zVideo_pDirectDraw2 = 0;
    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    zVideo_dd::FlipToGDIIfAttached();

    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    zVideo_dd::FlipToGDIIfAttached();

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryHasAttachedBackbuffer;
    return 0;
}

extern "C" int zvid_texture_pack_load_state_getter_smoke(void) {
    const int savedLoadState = g_zVid_TexturePackLoadState;

    g_zVid_TexturePackLoadState = 0;
    if (zVid::GetTexturePackLoadState() != 0) {
        g_zVid_TexturePackLoadState = savedLoadState;
        return 1;
    }

    g_zVid_TexturePackLoadState = 7;
    const int result = zVid::GetTexturePackLoadState() == 7 ? 0 : 2;
    g_zVid_TexturePackLoadState = savedLoadState;
    return result;
}

extern "C" int zvid_texture_pack_load_state_setter_smoke(void) {
    const int savedLoadState = g_zVid_TexturePackLoadState;

    g_zVid_TexturePackLoadState = 0;
    zVid::SetTexturePackLoadState(3);
    if (g_zVid_TexturePackLoadState != 3) {
        g_zVid_TexturePackLoadState = savedLoadState;
        return 1;
    }

    zVid::SetTexturePackLoadState(0);
    const int result = g_zVid_TexturePackLoadState == 0 ? 0 : 2;
    g_zVid_TexturePackLoadState = savedLoadState;
    return result;
}

extern "C" int zvid_option_accessors_smoke(void) {
    int mode = 6;
    int acceleration = 1;
    int hwApi = 2;
    int *const savedVideoMode = ZOPT_VIDEO_MODE;
    int *const savedAcceleration = ZOPT_VIDEO_ACCELERATION;
    int *const savedHwApi = ZOPT_HW_API;
    const int savedHwMode = g_zOpt_HwMode;
    const int savedAcceptedDeviceCount = g_zVideo_NumAcceptedDirectDrawDevices;

    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;

    int result = 0;
    if (zVid::GetVideoModeIndexFromOptions() != 6 ||
        zVid::GetAccelerationOption() != 1 ||
        zVid::GetHwApiOption() != 2) {
        result = 1;
    } else {
        mode = 3;
        zVid::SetAccelerationOption(0);
        zVid::SetHwApiOption(1);
        g_zVideo_NumAcceptedDirectDrawDevices = 3;
        if (zVid::GetVideoModeIndexFromOptions() != 3 ||
            zVid::GetAccelerationOption() != 0 ||
            zVid::GetAcceptedDirectDrawDeviceCount() != 3 ||
            g_zOpt_HwMode != 0 ||
            hwApi != 1) {
            result = 2;
        }
    }

    ZOPT_VIDEO_MODE = savedVideoMode;
    ZOPT_VIDEO_ACCELERATION = savedAcceleration;
    ZOPT_HW_API = savedHwApi;
    g_zOpt_HwMode = savedHwMode;
    g_zVideo_NumAcceptedDirectDrawDevices = savedAcceptedDeviceCount;
    return result;
}

extern "C" int zvid_set_video_mode_index_smoke(void) {
    std::int32_t mode = -1;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render = {};
    zOpt_ViewRectSection display = {};
    zOpt_ViewRectSection window = {};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    int *const savedVideoMode = ZOPT_VIDEO_MODE;
    int *const savedReplicate = ZOPT_REPLICATE;
    zOpt_ViewRectSection **const savedRenderSection = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const savedDisplaySection = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const savedWindowSection = g_zOpt_WindowSectionOption;

    ZOPT_VIDEO_MODE = &mode;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    int result = 0;
    zVid::SetVideoModeIndex(2);
    if (mode != 2 || replicate != 1 || render.width != 320 || render.height != 200 ||
        window.width != 640 || window.height != 400 || display.width != 640 ||
        display.height != 400 || display.bitsPerPixel != 16) {
        result = 1;
    }

    if (result == 0) {
        zVid::SetVideoModeIndex(7);
        if (mode != 7 || replicate != 0 || render.width != 1024 ||
            render.height != 768 || window.width != 1024 || window.height != 768 ||
            display.maxXInclusive != 1023 || display.maxYInclusive != 767) {
            result = 2;
        }
    }

    if (result == 0) {
        zVid::SetVideoModeIndex(8);
        if (mode != 0) {
            result = 3;
        }
    }

    ZOPT_VIDEO_MODE = savedVideoMode;
    ZOPT_REPLICATE = savedReplicate;
    g_zOpt_RenderSectionOption = savedRenderSection;
    g_zOpt_DisplaySectionOption = savedDisplaySection;
    g_zOpt_WindowSectionOption = savedWindowSection;
    return result;
}

extern "C" int zopt_fullscreen_accessors_smoke(void) {
    int fullscreen = 0;
    int hudSw = 1;
    int hudHw = 0;
    int hudTypeSw = 0;
    int hudTypeHw = 0;
    int gameControl = 0;
    int difficulty = 0;
    int effectsSw = 0;
    int effectsHw = 0;
    int objectLodSw = 0;
    int objectLodHw = 0;
    int muteSound = 0;
    float soundVolume = 0.0f;
    float globalVolume = 0.0f;
    int soundLod = 0;
    int textureMemorySw = 0;
    int textureMemoryHw = 0;
    int gfxFlagsSw = 0;
    int gfxFlagsHw = 0;
    char playerNameBuffer[8] = {0};
    zOptionEntryPartial playerNameOption = {0};
    int *const savedFullscreen = ZOPT_VIDEO_FULLSCREEN;
    int *const savedHudSw = ZOPT_HUD_SW;
    int *const savedHudHw = ZOPT_HUD_HW;
    int *const savedHudTypeSw = ZOPT_HUD_TYPE_SW;
    int *const savedHudTypeHw = ZOPT_HUD_TYPE_HW;
    int *const savedGameControl = ZOPT_GAME_CONTROL_OPTIONS;
    int *const savedDifficulty = g_zOpt_GameDifficultyOption;
    int *const savedEffectsSw = ZOPT_EFFECTS_LEVEL_SW;
    int *const savedEffectsHw = ZOPT_EFFECTS_LEVEL_HW;
    int *const savedObjectLodSw = ZOPT_OBJECT_LOD_SW;
    int *const savedObjectLodHw = ZOPT_OBJECT_LOD_HW;
    int *const savedMuteSound = ZOPT_MUTE_SOUND;
    float *const savedSoundVolume = ZOPT_SOUND_VOLUME;
    void *const savedGlobalVolume = g_zSnd_GlobalVolumeScalePtr;
    int *const savedSoundLod = ZOPT_SOUND_LOD;
    int *const savedTextureMemorySw = ZOPT_TEXTURE_MEMORY_SW;
    int *const savedTextureMemoryHw = ZOPT_TEXTURE_MEMORY_HW;
    int *const savedGfxFlagsSw = ZOPT_GFX_FLAGS_SW;
    int *const savedGfxFlagsHw = ZOPT_GFX_FLAGS_HW;
    zOptionEntryPartial *const savedPlayerName = ZOPT_PLAYER_NAME;
    const int savedHwMode = g_zOpt_HwMode;
    const int savedConditionalEffectLevel = g_zEffect_ConditionalEffectLevel;

    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HUD_SW = &hudSw;
    ZOPT_HUD_HW = &hudHw;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    ZOPT_HUD_TYPE_HW = &hudTypeHw;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControl;
    g_zOpt_GameDifficultyOption = &difficulty;
    ZOPT_EFFECTS_LEVEL_SW = &effectsSw;
    ZOPT_EFFECTS_LEVEL_HW = &effectsHw;
    ZOPT_OBJECT_LOD_SW = &objectLodSw;
    ZOPT_OBJECT_LOD_HW = &objectLodHw;
    ZOPT_MUTE_SOUND = &muteSound;
    ZOPT_SOUND_VOLUME = &soundVolume;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    ZOPT_SOUND_LOD = &soundLod;
    ZOPT_TEXTURE_MEMORY_SW = &textureMemorySw;
    ZOPT_TEXTURE_MEMORY_HW = &textureMemoryHw;
    ZOPT_GFX_FLAGS_SW = &gfxFlagsSw;
    ZOPT_GFX_FLAGS_HW = &gfxFlagsHw;
    playerNameOption.payloadOrBuffer = (int)(playerNameBuffer);
    playerNameOption.dataSize = sizeof(playerNameBuffer);
    ZOPT_PLAYER_NAME = &playerNameOption;

    int result = 0;
    zOpt::SetFullscreenOption(1);
    if (fullscreen != 1 || zOpt::GetFullscreenOption() != 1) {
        result = 1;
    }

    zOpt::SetFullscreenOption(0);
    if (result == 0 && (fullscreen != 0 || zOpt::GetFullscreenOption() != 0)) {
        result = 2;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetHudVisibilityOption(0);
    if (result == 0 &&
        (hudSw != 0 || hudHw != 0 || zOpt::GetHudVisibilityOption() != 0)) {
        result = 3;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetHudVisibilityOption(1);
    if (result == 0 &&
        (hudSw != 0 || hudHw != 1 || zOpt::GetHudVisibilityOption() != 1)) {
        result = 4;
    }

    g_zOpt_HwMode = 0;
    hudTypeSw = 2;
    if (result == 0 && zOpt::GetHudTypeForCurrentHwMode() != 2) {
        result = 41;
    }

    g_zOpt_HwMode = 1;
    hudTypeHw = 3;
    if (result == 0 && zOpt::GetHudTypeForCurrentHwMode() != 3) {
        result = 42;
    }

    zOpt::SetGameControlOptions(0);
    zOpt::SetThrottleMode(1);
    zOpt::SetSteeringMode(1);
    zOpt::SetCursorMode(1);
    if (result == 0 &&
        (gameControl != 7 ||
         zOpt::GetThrottleMode() != 1 ||
         zOpt::GetSteeringMode() != 1 ||
         zOpt::GetCursorMode() != 1 ||
         zOpt::GetCameraModePlayerState() != 3)) {
        result = 5;
    }

    zOpt::SetThrottleMode(0);
    zOpt::SetSteeringMode(0);
    zOpt::SetCursorMode(0);
    gameControl |= 8;
    if (result == 0 &&
        (gameControl != 8 ||
         zOpt::GetThrottleMode() != 0 ||
         zOpt::GetSteeringMode() != 0 ||
         zOpt::GetCursorMode() != 0 ||
         zOpt::GetCameraModePlayerState() != 1)) {
        result = 6;
    }

    zOpt::SetGameDifficultyMode(2);
    if (result == 0 && (difficulty != 2 || zOpt::GetGameDifficultyMode() != 2)) {
        result = 7;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetEffectsLevelForCurrentHwMode(0);
    if (result == 0 &&
        (effectsSw != 0 ||
         effectsHw != 0 ||
         g_zEffect_ConditionalEffectLevel != 2 ||
         zOpt::GetEffectsLevelForCurrentHwMode() != 0)) {
        result = 8;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetEffectsLevelForCurrentHwMode(2);
    if (result == 0 &&
        (effectsHw != 2 ||
         g_zEffect_ConditionalEffectLevel != 0 ||
         zOpt::GetEffectsLevelForCurrentHwMode() != 2)) {
        result = 9;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetObjectLODForCurrentHwMode(1);
    if (result == 0 &&
        (objectLodSw != 1 ||
         objectLodHw != 0 ||
         zOpt::GetObjectLODForCurrentHwMode() != 1)) {
        result = 10;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetObjectLODForCurrentHwMode(2);
    if (result == 0 &&
        (objectLodHw != 2 || zOpt::GetObjectLODForCurrentHwMode() != 2)) {
        result = 11;
    }

    zOpt::SetMuteSoundOption(1);
    if (result == 0 && zOpt::GetMuteSoundOption() != 1) {
        result = 12;
    }

    zOpt::SetSoundVolumeOption(0.625f);
    if (result == 0 &&
        (soundVolume != 0.625f ||
         globalVolume != 0.625f ||
         zOpt::GetSoundVolumeOption() != 0.625f)) {
        result = 13;
    }

    zOpt::SetSoundLODOption(3);
    if (result == 0 && (soundLod != 3 || zOpt::GetSoundLODOption() != 3)) {
        result = 14;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetTextureMemoryForCurrentHwMode(16);
    if (result == 0 &&
        (textureMemorySw != 16 || zOpt::GetTextureMemoryForCurrentHwMode() != 16)) {
        result = 15;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetTextureMemoryForCurrentHwMode(32);
    if (result == 0 &&
        (textureMemoryHw != 32 || zOpt::GetTextureMemoryForCurrentHwMode() != 32)) {
        result = 16;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetGraphicsFlagsForCurrentHwMode(0x11);
    if (result == 0 && zOpt::GetGraphicsFlagsForCurrentHwMode() != 0x11) {
        result = 17;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetGraphicsFlagsForCurrentHwMode(0x22);
    if (result == 0 && zOpt::GetGraphicsFlagsForCurrentHwMode() != 0x22) {
        result = 18;
    }

    zOpt::SetPlayerName("Pilot");
    if (result == 0 &&
        (strcmp(playerNameBuffer, "Pilot") != 0 ||
         zOpt_GetPlayerName() != playerNameBuffer)) {
        result = 19;
    }

    zOpt::SetPlayerName("LongPlayerName");
    if (result == 0 && strcmp(playerNameBuffer, "LongPla") != 0) {
        result = 20;
    }

    ZOPT_VIDEO_FULLSCREEN = savedFullscreen;
    ZOPT_HUD_SW = savedHudSw;
    ZOPT_HUD_HW = savedHudHw;
    ZOPT_HUD_TYPE_SW = savedHudTypeSw;
    ZOPT_HUD_TYPE_HW = savedHudTypeHw;
    ZOPT_GAME_CONTROL_OPTIONS = savedGameControl;
    g_zOpt_GameDifficultyOption = savedDifficulty;
    ZOPT_EFFECTS_LEVEL_SW = savedEffectsSw;
    ZOPT_EFFECTS_LEVEL_HW = savedEffectsHw;
    ZOPT_OBJECT_LOD_SW = savedObjectLodSw;
    ZOPT_OBJECT_LOD_HW = savedObjectLodHw;
    ZOPT_MUTE_SOUND = savedMuteSound;
    ZOPT_SOUND_VOLUME = savedSoundVolume;
    g_zSnd_GlobalVolumeScalePtr = savedGlobalVolume;
    ZOPT_SOUND_LOD = savedSoundLod;
    ZOPT_TEXTURE_MEMORY_SW = savedTextureMemorySw;
    ZOPT_TEXTURE_MEMORY_HW = savedTextureMemoryHw;
    ZOPT_GFX_FLAGS_SW = savedGfxFlagsSw;
    ZOPT_GFX_FLAGS_HW = savedGfxFlagsHw;
    ZOPT_PLAYER_NAME = savedPlayerName;
    g_zOpt_HwMode = savedHwMode;
    g_zEffect_ConditionalEffectLevel = savedConditionalEffectLevel;
    return result;
}

extern "C" int zopt_view_rect_target_side_effects_smoke(void) {
    zOpt_ViewRectSection **const savedRenderSection = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const savedDisplaySection = g_zOpt_DisplaySectionOption;
    zOpt_CameraSection **const savedCameraSection = g_zOpt_CameraSectionOption;
    int *const savedObjectLodSw = ZOPT_OBJECT_LOD_SW;
    int *const savedObjectLodHw = ZOPT_OBJECT_LOD_HW;
    const int savedHwMode = g_zOpt_HwMode;

    zOpt_ViewRectSection render = {};
    zOpt_ViewRectSection display = {};
    zOpt_CameraSection cameraSection = {};
    zClass_WindowDataPartial windowData = {};
    zClass_DisplayDataPartial displayData = {};
    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial windowNode = {};
    zClass_NodePartial displayNode = {};
    zClass_NodePartial cameraNode = {};
    windowNode.classId = 3;
    windowNode.classData = &windowData;
    displayNode.classId = 4;
    displayNode.classData = &displayData;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    render.target = &windowNode;
    display.target = &displayNode;
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_CameraSection *cameraSectionPtr = &cameraSection;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_CameraSectionOption = &cameraSectionPtr;

    int result = 0;
    render.x = 5;
    render.y = 6;
    render.width = 320;
    render.height = 240;
    render.target = nullptr;
    zOpt::RenderSection_SetTargetWindow(&windowNode);
    if (zOpt::GetRenderSection() != &render || windowData.resolutionWidth != 320 ||
        windowData.resolutionHeight != 240 || windowData.viewportWidth != 5 ||
        windowData.viewportHeight != 6) {
        result = 3;
    }

    display.x = 7;
    display.y = 8;
    display.width = 400;
    display.height = 300;
    display.target = nullptr;
    zOpt::DisplaySection_SetTargetDisplay(&displayNode);
    if (result == 0 &&
        (displayData.width != 400 || displayData.height != 300 || displayData.x != 7 ||
         displayData.y != 8)) {
        result = 4;
    }

    zOpt::RenderSection_SetSize(640, 480);
    zOpt::RenderSection_SetPosition(10, 20);
    if (result == 0 &&
        (windowData.resolutionWidth != 640 || windowData.resolutionHeight != 480 ||
         windowData.viewportWidth != 10 || windowData.viewportHeight != 20)) {
        result = 1;
    }

    zOpt::DisplaySection_SetSize(800, 600);
    zOpt::DisplaySection_SetPosition(30, 40);
    if (result == 0 &&
        (displayData.width != 800 || displayData.height != 600 ||
         displayData.x != 30 || displayData.y != 40)) {
        result = 2;
    }

    int objectLodSw = 2;
    int objectLodHw = 1;
    ZOPT_OBJECT_LOD_SW = &objectLodSw;
    ZOPT_OBJECT_LOD_HW = &objectLodHw;
    g_zOpt_HwMode = 0;
    render.width = 800;
    render.height = 400;
    cameraData.viewportWidth = 400.0f;
    cameraData.viewportHeight = 200.0f;
    cameraData.frustumWidth = 0.25f;
    cameraData.frustumHeight = 0.5f;
    zOpt::CameraSection_SetActiveCamera(&cameraNode);
    if (result == 0 &&
        (cameraSection.m_pCamera != &cameraNode ||
         zOpt_CameraSection_GetActiveCamera() != &cameraNode ||
         cameraData.frustumWidth != 1.0f ||
         cameraData.frustumHeight != 0.5f ||
         cameraData.fovX != 1.0f / 400.0f ||
         cameraData.fovY != 0.5f / 200.0f ||
         cameraData.clipDistance != 0.5f)) {
        result = 5;
    }

    zOpt::CameraSection_SetActiveCamera(nullptr);
    if (result == 0 &&
        (cameraSection.m_pCamera != nullptr ||
         zOpt_CameraSection_GetActiveCamera() != nullptr)) {
        result = 6;
    }

    g_zOpt_RenderSectionOption = savedRenderSection;
    g_zOpt_DisplaySectionOption = savedDisplaySection;
    g_zOpt_CameraSectionOption = savedCameraSection;
    ZOPT_OBJECT_LOD_SW = savedObjectLodSw;
    ZOPT_OBJECT_LOD_HW = savedObjectLodHw;
    g_zOpt_HwMode = savedHwMode;
    return result;
}

extern "C" int zopt_section_accessor_smoke(void) {
    int *const savedVideoStride = ZOPT_VIDEO_STRIDE;
    int *const savedReplicate = ZOPT_REPLICATE;
    zOpt_ViewRectSection **const savedDisplaySection = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const savedWindowSection = g_zOpt_WindowSectionOption;

    int videoStride = 2048;
    zOpt_ViewRectSection display = {};
    zOpt_ViewRectSection window = {};
    display.width = 800;
    display.height = 600;
    display.bitsPerPixel = 16;
    window.height = 480;
    int replicate = 1;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_STRIDE = &videoStride;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    zOpt_ViewRectSection clamp = {};
    clamp.x = 10;
    clamp.y = 20;
    clamp.maxXInclusive = 30;
    clamp.maxYInclusive = 40;

    float belowPoint[2] = {5.0f, 15.0f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, belowPoint);
    const bool belowClamped = belowPoint[0] == 10.0f && belowPoint[1] == 20.0f;

    float insidePoint[2] = {11.5f, 25.25f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, insidePoint);
    const bool insideKept = insidePoint[0] == 11.5f && insidePoint[1] == 25.25f;

    float abovePoint[2] = {31.0f, 45.0f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, abovePoint);
    const bool aboveClamped = abovePoint[0] == 30.0f && abovePoint[1] == 40.0f;

    float nanPoint[2] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()
    };
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, nanPoint);
    const bool nanClampedToMax = nanPoint[0] == 30.0f && nanPoint[1] == 40.0f;

    const bool ok =
        zOpt::GetDisplaySection() == &display &&
        zOpt_DisplaySection_GetWidth() == 800 &&
        zOpt_DisplaySection_GetHeight() == 600 &&
        zOpt::GetDisplaySectionBitsPerPixel() == 16 &&
        zOpt::GetVideoStrideValue() == 2048 &&
        zOpt::GetReplicateMode() == 1 &&
        zOpt::GetWindowSection() == &window &&
        zOpt::GetWindowSectionHeight() == 480 &&
        belowClamped &&
        insideKept &&
        aboveClamped &&
        nanClampedToMax;

    ZOPT_VIDEO_STRIDE = savedVideoStride;
    ZOPT_REPLICATE = savedReplicate;
    g_zOpt_DisplaySectionOption = savedDisplaySection;
    g_zOpt_WindowSectionOption = savedWindowSection;
    return ok ? 0 : 1;
}

extern "C" int zvideo_init_set_surface_geometry_from_mode_index_smoke(void) {
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedResolutionMenuValid = gVideo_resolutionMenuValid;
    const int savedDisplayModeBpp = g_zVideo_DisplayModeBpp;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    int failCode = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_DisplayModeBpp = 0;
    gVideo_resolutionMenuValid = 1;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    zVideo::Init_SetSurfaceGeometryFromModeIndex(2);
    if (g_zVideo_UseHalfResBackbuffer != 1 ||
        g_zVideo_DisplayModeSurfaceState.width != 640 ||
        g_zVideo_DisplayModeSurfaceState.height != 400 ||
        g_zVideo_PrimarySurfaceState.width != 640 ||
        g_zVideo_PrimarySurfaceState.height != 400 ||
        g_zVideo_SwSurfaceState.width != 320 ||
        g_zVideo_SwSurfaceState.height != 200 ||
        g_zVideo_DisplayModeBpp != 16 ||
        gVideo_resolutionMenuValid != 1) {
        failCode = 1;
    }

    if (failCode == 0) {
        zVideo::Init_SetSurfaceGeometryFromModeIndex(7);
        if (g_zVideo_UseHalfResBackbuffer != 0 ||
            g_zVideo_DisplayModeSurfaceState.width != 1024 ||
            g_zVideo_DisplayModeSurfaceState.height != 768 ||
            g_zVideo_PrimarySurfaceState.width != 1024 ||
            g_zVideo_PrimarySurfaceState.height != 768 ||
            g_zVideo_SwSurfaceState.width != 1024 ||
            g_zVideo_SwSurfaceState.height != 768 ||
            g_zVideo_DisplayModeBpp != 16) {
            failCode = 2;
        }
    }

    if (failCode == 0) {
        gVideo_resolutionMenuValid = 1;
        zVideo::Init_SetSurfaceGeometryFromModeIndex(1);
        if (gVideo_resolutionMenuValid != 0) {
            failCode = 3;
        }
    }

    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    gVideo_resolutionMenuValid = savedResolutionMenuValid;
    g_zVideo_DisplayModeBpp = savedDisplayModeBpp;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return failCode;
}

namespace {
int g_smokeZVideoSetVideoModeCalls;
int g_smokeZVideoSetVideoModeResult = 0x1234;
int g_smokeZVideoShutdownCalls;

int __fastcall SmokeZVideoSetVideoModeFake(int) {
    ++g_smokeZVideoSetVideoModeCalls;
    return g_smokeZVideoSetVideoModeResult;
}

void SmokeZVideoShutdownFake() {
    ++g_smokeZVideoShutdownCalls;
}
} // namespace

extern "C" int zvideo_mode_geometry_and_set_video_mode_smoke(void) {
    const zVideo_StatusProc savedSetVideoMode = g_zVideo_pfnSetVideoMode;
    const int savedInitialized = g_zVideo_IsInitialized;

    int result = zvideo_init_set_surface_geometry_from_mode_index_smoke();
    if (result != 0) {
        return result;
    }

    g_smokeZVideoSetVideoModeCalls = 0;
    g_zVideo_pfnSetVideoMode = SmokeZVideoSetVideoModeFake;
    g_zVideo_IsInitialized = 0;
    if (zVideo::SetVideoMode(5) != 0x5a560000 ||
        g_smokeZVideoSetVideoModeCalls != 0) {
        result = 4;
    }

    if (result == 0) {
        g_zVideo_IsInitialized = 1;
        if (zVideo::SetVideoMode(5) != 0x1234 ||
            g_smokeZVideoSetVideoModeCalls != 1 ||
            g_zVideo_DisplayModeSurfaceState.height != 480) {
            result = 5;
        }
    }

    if (result == 0) {
        if (zVideo::Init_ApplyModeIndex(4) != 0x1234 ||
            g_smokeZVideoSetVideoModeCalls != 2 ||
            g_zVideo_DisplayModeSurfaceState.height != 400) {
            result = 6;
        }
    }

    g_zVideo_pfnSetVideoMode = savedSetVideoMode;
    g_zVideo_IsInitialized = savedInitialized;
    return result;
}

extern "C" int zvideo_init_video_system_reentry_guard_smoke(void) {
    const HWND savedHwnd = g_zVideo_hWnd;
    const int savedInitialized = g_zVideo_IsInitialized;
    const int savedFrameTick = g_zVideo_FrameTick;

    g_zVideo_IsInitialized = 1;
    g_zVideo_FrameTick = 77;

    const int result = zVideo::InitVideoSystem((HWND)(0x1234), 1, 1, 5);
    const int ok =
        result == 0x5a560001 &&
        g_zVideo_FrameTick == 77 &&
        g_zVideo_hWnd == savedHwnd;

    g_zVideo_hWnd = savedHwnd;
    g_zVideo_IsInitialized = savedInitialized;
    g_zVideo_FrameTick = savedFrameTick;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_bind_renderer_dispatch_smoke(void) {
    const int savedRendererType = g_zVideo_RendererType;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    zVidHwApiDeviceRecordPartial *const savedSelected =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const zVideo_StatusProc savedOpen = g_zVideo_pfnOpenVideoMode;
    const zVideo_StatusProc savedSet = g_zVideo_pfnSetVideoMode;
    const zVideo_CreateTextureRecordProc savedCreate =
        g_zVideo_pfnCreateTextureRecord;
    const zVideo_AdjustSurfacesProc savedAdjust = g_zVideo_pfnAdjustSurfaces;
    const zVideo_GetHwApiDeviceFeatureFlagsProc savedFeature =
        g_zVideo_pfnGetHwApiDeviceFeatureFlags;
    const zVideo_BltRectDirectProc savedBltSw = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_BltRectDirectProc savedBltPrimary =
        g_zVideo_pfnBltPrimaryToSwRectDirect;

    zVidHwApiDeviceRecordPartial selectedDevice = {};
    selectedDevice.m_deviceFeatureFlags = 0x1234;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;

    zVideo::BindRendererDispatch(1, 2);
    int result = 0;
    if (g_zVideo_RendererType != 1 ||
        g_zVideo_ActiveRendererPath != 1 ||
        g_zVideo_FullscreenOption != 2 ||
        g_zVideo_pfnOpenVideoMode != zVideo_dd::OpenVideoMode ||
        g_zVideo_pfnSetVideoMode != zVideo_dd::SetVideoMode ||
        g_zVideo_pfnCreateTextureRecord != zVideo_dd3d::CreateTextureRecord ||
        g_zVideo_pfnAdjustSurfaces != zVideo_dd3d::PresentDisplayModeSurface ||
        g_zVideo_pfnGetHwApiDeviceFeatureFlags !=
            zVideo_dd::GetHwApiDeviceFeatureFlags ||
        g_zVideo_pfnBltSwToPrimaryRectDirect !=
            zVideo_dd::BltSwToPrimaryRectDirect ||
        g_zVideo_pfnBltPrimaryToSwRectDirect !=
            zVideo_dd::BltPrimaryToSwRectDirect ||
        selectedDevice.m_deviceFeatureFlags != 0) {
        result = 1;
    }

    if (result == 0) {
        g_zVideo_pSelectedHwApiDeviceRecord = 0;
        zVideo::BindRendererDispatch(0, 1);
        if (g_zVideo_RendererType != 0 ||
            g_zVideo_ActiveRendererPath != 0 ||
            g_zVideo_FullscreenOption != 1 ||
            g_zVideo_pfnAdjustSurfaces != zVideo_dd::PresentDisplayModeSurface) {
            result = 2;
        }
    }

    g_zVideo_RendererType = savedRendererType;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_FullscreenOption = savedFullscreenOption;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelected;
    g_zVideo_pfnOpenVideoMode = savedOpen;
    g_zVideo_pfnSetVideoMode = savedSet;
    g_zVideo_pfnCreateTextureRecord = savedCreate;
    g_zVideo_pfnAdjustSurfaces = savedAdjust;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = savedFeature;
    g_zVideo_pfnBltSwToPrimaryRectDirect = savedBltSw;
    g_zVideo_pfnBltPrimaryToSwRectDirect = savedBltPrimary;
    return result;
}

extern "C" int zvideo_module_init_smoke(void) {
    const size_t resetBytes =
        (unsigned char *)(&g_zVideo_OverwriteQueueBase[0x180]) -
        (unsigned char *)(&g_zVideo_RendererType);
    unsigned char *const savedResetBlock =
        (unsigned char *)std::malloc(resetBytes);
    if (savedResetBlock == 0) {
        return 99;
    }
    std::memcpy(savedResetBlock, &g_zVideo_RendererType, resetBytes);

    const int savedRendererType = g_zVideo_RendererType;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedFrameTick = g_zVideo_FrameTick;
    const int savedResolutionMenuValid = gVideo_resolutionMenuValid;
    const int savedPaletteBrightness = g_zVideo_PaletteBrightnessLevel;
    const int savedClearColor = g_zVideo_ClearColorPacked16;
    const int savedFullscreen = g_zVideo_FullscreenOption;
    const int savedPendingDither = g_zVideo_PendingDitherEnable;
    const float savedInverseZTolerance = g_zVideo_InverseZTolerancePending;
    const int savedD3DAppendFanCloseVertex =
        g_zVideo_D3DAppendFanCloseVertexPending;
    zVidHwApiDeviceRecordPartial *const savedSelected =
        g_zVideo_pSelectedHwApiDeviceRecord;
    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;

    g_zVideo_RendererType = 7;
    g_zVideo_ActiveRendererPath = 7;
    g_zVideo_FrameTick = 9;
    gVideo_resolutionMenuValid = 1;
    g_zVideo_PaletteBrightnessLevel = 2;
    g_zVideo_ClearColorPacked16 = 0xabcd;
    g_zVideo_pSelectedHwApiDeviceRecord = 0;
    g_zVideo_pSelectedD3DDeviceInfo = (zVidD3DDriverRecordPartial *)(0x1);
    g_zVideo_pfnOpenVideoMode = (zVideo_StatusProc)(0x1);
    g_zVideo_QuadBatchItemsBase[0].vertices[0].specular = 0x11223344;
    g_zVideo_SortedPolyQueueBase[0].vertexCount = 5;
    g_zVideo_OverwriteQueueBase[0].type = 6;
    g_zVideo_OverwriteQueueBase[0x17f].vertexCount = 7;
    g_zVideo_SortedPolyQueueCount = 8;
    g_zVideo_OverwriteQueueCount = 9;

    int result = 0;
    if (zVideo::ModuleInit() != 0) {
        result = 1;
    } else if (
        g_zVideo_RendererType != 0 ||
        g_zVideo_ActiveRendererPath != 0 ||
        g_zVideo_FrameTick != 0 ||
        gVideo_resolutionMenuValid != 0 ||
        g_zVideo_PaletteBrightnessLevel != 4 ||
        g_zVideo_ClearColorPacked16 != 0 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_PendingDitherEnable != -1 ||
        g_zVideo_TexturePixelPack_ABits != 4 ||
        g_zVideo_TexturePixelPack_RMask != 0xf000 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[0] ||
        g_zVideo_pSelectedD3DDeviceInfo != 0) {
        result = 2;
    } else if (
        g_zVideo_pfnOpenVideoMode != zVideo_dd::OpenVideoMode ||
        g_zVideo_QuadBatchItemsBase[0].vertices[0].specular != 0 ||
        g_zVideo_SortedPolyQueueBase[0].vertexCount != 0 ||
        g_zVideo_OverwriteQueueBase[0].type != 0 ||
        g_zVideo_OverwriteQueueBase[0x17f].vertexCount != 0 ||
        g_zVideo_SortedPolyQueueCount != 0 ||
        g_zVideo_OverwriteQueueCount != 0) {
        result = 3;
    }

    std::memcpy(&g_zVideo_RendererType, savedResetBlock, resetBytes);
    std::free(savedResetBlock);
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_FrameTick = savedFrameTick;
    gVideo_resolutionMenuValid = savedResolutionMenuValid;
    g_zVideo_PaletteBrightnessLevel = savedPaletteBrightness;
    g_zVideo_ClearColorPacked16 = savedClearColor;
    g_zVideo_FullscreenOption = savedFullscreen;
    g_zVideo_PendingDitherEnable = savedPendingDither;
    g_zVideo_InverseZTolerancePending = savedInverseZTolerance;
    g_zVideo_D3DAppendFanCloseVertexPending = savedD3DAppendFanCloseVertex;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelected;
    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    return result;
}

extern "C" int zvideo_return_success_stub_smoke(void) {
    return zVideo::ReturnSuccessStub();
}

extern "C" int zvid_cached_client_rect_smoke(void) {
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedUpdateMask = g_zVid_CachedClientRectUpdateMask;
    const HWND savedHwnd = g_zVideo_hWnd;
    const RECT savedRect = g_zVideo_CachedClientRectScreen;

    zVid::SetCachedClientRectUpdateMask(0x55);
    g_zVideo_ActiveRendererPath = 1;
    int result = 0;
    if (zVid::QueryCachedClientRectUpdateMaskIf3dfx() != 0x55) {
        result = 1;
    }

    if (result == 0) {
        g_zVideo_ActiveRendererPath = 2;
        if (zVid::QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
            result = 2;
        }
    }

    HWND hwnd = 0;
    RECT client = {};
    if (result == 0) {
        hwnd = CreateWindowExA(
            0,
            "STATIC",
            "recoil-video-test",
            WS_OVERLAPPEDWINDOW,
            20,
            30,
            160,
            120,
            0,
            0,
            GetModuleHandleA(0),
            0
        );
        if (hwnd == 0) {
            result = 3;
        }
    }

    if (result == 0) {
        GetClientRect(hwnd, &client);
        g_zVideo_hWnd = hwnd;
        g_zVideo_CachedClientRectScreen.left = 100;
        g_zVideo_CachedClientRectScreen.top = 100;
        g_zVideo_CachedClientRectScreen.right = 100;
        g_zVideo_CachedClientRectScreen.bottom = 100;

        if (zVideo::UpdateCachedClientRectScreenCoords() != 0) {
            result = 4;
        }
    }

    if (result == 0) {
        const LONG cachedWidth =
            g_zVideo_CachedClientRectScreen.right -
            g_zVideo_CachedClientRectScreen.left;
        const LONG cachedHeight =
            g_zVideo_CachedClientRectScreen.bottom -
            g_zVideo_CachedClientRectScreen.top;

        g_zVideo_CachedClientRectScreen.left = 7;
        g_zVideo_CachedClientRectScreen.top = 8;
        g_zVideo_CachedClientRectScreen.right = 9;
        g_zVideo_CachedClientRectScreen.bottom = 10;
        zVid::SetCachedClientRectUpdateMask(0);
        zVid::UpdateCachedClientRectIfUpdateMaskEnabled();
        const int noUpdateOk =
            g_zVideo_CachedClientRectScreen.left == 7 &&
            g_zVideo_CachedClientRectScreen.top == 8 &&
            g_zVideo_CachedClientRectScreen.right == 9 &&
            g_zVideo_CachedClientRectScreen.bottom == 10;

        g_zVideo_ActiveRendererPath = 1;
        zVid::SetCachedClientRectUpdateMask(1);
        zVid::UpdateCachedClientRectIfUpdateMaskEnabled();
        const LONG helperWidth =
            g_zVideo_CachedClientRectScreen.right -
            g_zVideo_CachedClientRectScreen.left;
        const LONG helperHeight =
            g_zVideo_CachedClientRectScreen.bottom -
            g_zVideo_CachedClientRectScreen.top;

        if (cachedWidth != client.right - client.left ||
            cachedHeight != client.bottom - client.top ||
            noUpdateOk == 0 ||
            helperWidth != client.right - client.left ||
            helperHeight != client.bottom - client.top) {
            result = 5;
        }
    }

    if (hwnd != 0) {
        DestroyWindow(hwnd);
    }
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    zVid::SetCachedClientRectUpdateMask(savedUpdateMask);
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_CachedClientRectScreen = savedRect;
    return result;
}

extern "C" int zvideo_shutdown_video_system_smoke(void) {
    const int savedInitialized = g_zVideo_IsInitialized;
    const zVideo_ShutdownVideoSystemProc savedShutdown =
        g_zVideo_pfnShutdownVideoSystem;

    g_smokeZVideoShutdownCalls = 0;
    g_zVideo_IsInitialized = 0;
    g_zVideo_pfnShutdownVideoSystem = SmokeZVideoShutdownFake;

    int result = 0;
    if (zVideo::ShutdownVideoSystem() != 0x5a560000 ||
        g_smokeZVideoShutdownCalls != 0) {
        result = 1;
    }

    if (result == 0) {
        g_zVideo_IsInitialized = 1;
        if (zVideo::ShutdownVideoSystem() != 0 ||
            g_zVideo_IsInitialized != 0 ||
            g_smokeZVideoShutdownCalls != 1) {
            result = 2;
        }
    }

    g_zVideo_IsInitialized = savedInitialized;
    g_zVideo_pfnShutdownVideoSystem = savedShutdown;
    return result;
}

extern "C" int zvideo_select_hw_api_device_smoke(void) {
    g_zVideo_pSelectedHwApiDeviceRecord = 0;
    g_zVideo_pSelectedD3DDeviceInfo = 0;

    if (zVideo::SelectHwApiDeviceOrFallback(-1) != 0 ||
        g_zVideo_RendererType != 0 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[0] ||
        g_zVideo_pSelectedD3DDeviceInfo != 0) {
        return 1;
    }

    if (zVideo::SelectHwApiDeviceOrFallback(2) != 1 ||
        g_zVideo_RendererType != 1 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[2] ||
        g_zVideo_pSelectedD3DDeviceInfo != g_zVideo_HwApiDeviceTable[2].m_d3dDrivers) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_dd3d_set_fog_enable_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedFogEnable = g_zVideo_CachedFogEnableRenderState;
    const int savedFogMode = g_zVideo_CachedFogModeLightState;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_CachedFogEnableRenderState = 0;
    g_zVideo_CachedFogModeLightState = 0;
    zVideo_dd3d::SetFogEnable(1);
    const int firstCallOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 1 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_FOGENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 1 &&
        g_smokeDirect3DDevice2SetLightStateCalls == 1 &&
        g_smokeDirect3DDevice2LightStates[0] == D3DLIGHTSTATE_FOGMODE &&
        g_smokeDirect3DDevice2LightStateValues[0] == D3DFOG_LINEAR &&
        g_zVideo_CachedFogEnableRenderState == 1 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_CachedFogEnableRenderState = 1;
    g_zVideo_CachedFogModeLightState = D3DFOG_LINEAR;
    zVideo_dd3d::SetFogEnable(1);
    const int cacheHitOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 0 &&
        g_smokeDirect3DDevice2SetLightStateCalls == 0 &&
        g_zVideo_CachedFogEnableRenderState == 1 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_CachedFogEnableRenderState = 1;
    g_zVideo_CachedFogModeLightState = D3DFOG_LINEAR;
    zVideo_dd3d::SetFogEnable(0);
    const int renderOnlyOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 1 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_FOGENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 0 &&
        g_smokeDirect3DDevice2SetLightStateCalls == 0 &&
        g_zVideo_CachedFogEnableRenderState == 0 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_CachedFogEnableRenderState = savedFogEnable;
    g_zVideo_CachedFogModeLightState = savedFogMode;
    return firstCallOk && cacheHitOk && renderOnlyOk ? 0 : 1;
}

extern "C" int zvideo_pending_wireframe_state_smoke(void) {
    const int savedPendingWireframe = g_zVideo_PendingWireframeState;

    zVideo_dd3d::SetPendingWireframeState(1);
    const int oneOk = g_zVideo_PendingWireframeState == 1;
    zVideo_dd3d::SetPendingWireframeState(0);
    const int zeroOk = g_zVideo_PendingWireframeState == 0;
    zVideo_dd3d::SetPendingWireframeState(-1);
    const int sentinelOk = g_zVideo_PendingWireframeState == -1;

    g_zVideo_PendingWireframeState = savedPendingWireframe;
    return oneOk && zeroOk && sentinelOk ? 0 : 1;
}

extern "C" int zvideo_pending_dither_enable_smoke(void) {
    const int savedPendingDither = g_zVideo_PendingDitherEnable;

    zVideo_dd3d::SetPendingDitherEnable(1);
    const int oneOk = g_zVideo_PendingDitherEnable == 1;
    zVideo_dd3d::SetPendingDitherEnable(0);
    const int zeroOk = g_zVideo_PendingDitherEnable == 0;
    zVideo_dd3d::SetPendingDitherEnable(-1);
    const int sentinelOk = g_zVideo_PendingDitherEnable == -1;

    g_zVideo_PendingDitherEnable = savedPendingDither;
    return oneOk && zeroOk && sentinelOk ? 0 : 1;
}

extern "C" int zvideo_dd3d_begin_scene_flush_pending_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedPendingWireframe = g_zVideo_PendingWireframeState;
    const int savedPendingDither = g_zVideo_PendingDitherEnable;
    const int savedSceneDepth = g_zVideo_D3DSceneDepth;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_PendingWireframeState = 0;
    g_zVideo_PendingDitherEnable = 1;
    const int firstResult = zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
    const int solidFlushOk =
        firstResult == 0 &&
        g_smokeDirect3DDevice2BeginSceneCalls == 1 &&
        g_smokeDirect3DDevice2SetRenderStateCalls == 2 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_FILLMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DFILL_SOLID &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_DITHERENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 1 &&
        g_zVideo_PendingWireframeState == -1 &&
        g_zVideo_PendingDitherEnable == -1;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_PendingWireframeState = 1;
    g_zVideo_PendingDitherEnable = -1;
    zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
    const int wireframeFlushOk =
        g_smokeDirect3DDevice2BeginSceneCalls == 1 &&
        g_smokeDirect3DDevice2SetRenderStateCalls == 1 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_FILLMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DFILL_WIREFRAME &&
        g_zVideo_PendingWireframeState == -1 &&
        g_zVideo_PendingDitherEnable == -1;

    InstallSmokeDirect3DDevice2(device);
    g_smokeDirect3DDevice2BeginSceneResult = (HRESULT)(DDERR_INVALIDPARAMS);
    g_zVideo_PendingWireframeState = 0;
    g_zVideo_PendingDitherEnable = 0;
    const int errorResult = zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
    const int failureLeavesPendingOk =
        errorResult == -1 &&
        g_smokeDirect3DDevice2BeginSceneCalls == 1 &&
        g_smokeDirect3DDevice2SetRenderStateCalls == 0 &&
        g_zVideo_PendingWireframeState == 0 &&
        g_zVideo_PendingDitherEnable == 0;

    InstallSmokeDirect3DDevice2(device);
    const int endResult = zVideo_dd3d::EndScene();
    g_smokeDirect3DDevice2EndSceneResult = (HRESULT)(DDERR_INVALIDPARAMS);
    const int endErrorResult = zVideo_dd3d::EndScene();
    const int endSceneOk =
        endResult == 0 &&
        endErrorResult == -1 &&
        g_smokeDirect3DDevice2EndSceneCalls == 2;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_PendingWireframeState = -1;
    g_zVideo_PendingDitherEnable = -1;
    g_zVideo_D3DSceneDepth = 0;
    const int enterResult = zVideoD3D::SceneEnter();
    const int enterAgainResult = zVideoD3D::SceneEnter();
    const int enterDepthOk =
        enterResult == 0 &&
        enterAgainResult == 0 &&
        g_zVideo_D3DSceneDepth == 1 &&
        g_smokeDirect3DDevice2BeginSceneCalls == 1;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DSceneDepth = 2;
    const int leaveNestedResult = zVideoD3D::SceneLeave();
    const int leaveNestedOk =
        leaveNestedResult == 0 &&
        g_zVideo_D3DSceneDepth == 1 &&
        g_smokeDirect3DDevice2EndSceneCalls == 0;
    const int leaveFinalResult = zVideoD3D::SceneLeave();
    const int leaveIdleResult = zVideoD3D::SceneLeave();
    const int leaveDepthOk =
        leaveFinalResult == 0 &&
        leaveIdleResult == 0 &&
        g_zVideo_D3DSceneDepth == 0 &&
        g_smokeDirect3DDevice2EndSceneCalls == 1;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_PendingWireframeState = savedPendingWireframe;
    g_zVideo_PendingDitherEnable = savedPendingDither;
    g_zVideo_D3DSceneDepth = savedSceneDepth;

    return solidFlushOk &&
                   wireframeFlushOk &&
                   failureLeavesPendingOk &&
                   endSceneOk &&
                   enterDepthOk &&
                   leaveNestedOk &&
                   leaveDepthOk
               ? 0
               : 1;
}

extern "C" int zvideo_texture_record_release_upload_surface_smoke(void) {
    zVideo_TextureRecordPartial textureRecord = {};
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    const int nullOk =
        textureRecord.m_uploadSurface == 0 &&
        g_smokeComReleaseCalls == 0;

    SmokeComObject uploadSurface = {};
    InstallSmokeComReleaseObject(uploadSurface);
    ResetSmokeComReleaseTracking();
    textureRecord.m_uploadSurface = (IDirectDrawSurface *)(&uploadSurface);
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    const int releaseOk =
        textureRecord.m_uploadSurface == 0 &&
        g_smokeComReleaseCalls == 1 &&
        g_smokeComReleaseObjects[0] == &uploadSurface;

    return nullOk && releaseOk ? 0 : 1;
}

extern "C" int zvideo_texture_record_finalize_upload_smoke(void) {
    SmokeFunctionPatch uploadPatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd3d::UploadImageToSurface),
            (void *)(&SmokeUploadImageToSurface),
            uploadPatch
        )) {
        return 1;
    }

    SmokeDirectDrawSurfaceObject uploadSurface = {};
    SmokeDirect3DTexture2Object uploadTexture = {};
    SmokeDirect3DTexture2Object targetTexture = {};
    InstallSmokeDirectDrawSurface(uploadSurface);
    InstallSmokeDirect3DTexture2(uploadTexture, targetTexture);

    zVideo_TextureRecordPartial textureRecord = {};
    zVidImagePartial image = {};
    image.formatFlagsPacked = 3;
    g_smokeUploadImageToSurfaceCalls = 0;
    g_smokeUploadImageToSurfaceSurface = 0;
    zVideo_dd3d::TextureRecord_FinalizeUpload(&textureRecord, 0, &image);
    const int nullUploadOk =
        g_smokeUploadImageToSurfaceCalls == 0 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
        g_smokeDirect3DTexture2LoadCalls == 0 &&
        g_smokeDirect3DTexture2ReleaseCalls == 0;

    textureRecord.m_uploadSurface = (IDirectDrawSurface *)(&uploadSurface);
    textureRecord.m_texture = (IDirect3DTexture2 *)(&targetTexture);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &uploadTexture;
    g_smokeUploadImageToSurfaceCalls = 0;
    g_smokeUploadImageToSurfaceSurface = 0;
    g_smokeUploadImageToSurfaceImage = 0;
    g_smokeUploadImageToSurfaceUseAlpha = -1;
    g_smokeUploadImageToSurfaceResult = 1;
    zVideo_dd3d::TextureRecord_FinalizeUpload(&textureRecord, 0, &image);
    const int successOk =
        g_smokeUploadImageToSurfaceCalls == 1 &&
        g_smokeUploadImageToSurfaceSurface ==
            (IDirectDrawSurface *)(&uploadSurface) &&
        g_smokeUploadImageToSurfaceImage == &image &&
        g_smokeUploadImageToSurfaceUseAlpha == 2 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        g_smokeDirectDrawSurfaceLastQueryInterfaceIid != 0 &&
        IsEqualGUID(*g_smokeDirectDrawSurfaceLastQueryInterfaceIid, IID_IDirect3DTexture2) &&
        g_smokeDirect3DTexture2LoadCalls == 1 &&
        g_smokeDirect3DTexture2LastLoadSelf ==
            (IDirect3DTexture2 *)(&targetTexture) &&
        g_smokeDirect3DTexture2LastLoadSource ==
            (IDirect3DTexture2 *)(&uploadTexture) &&
        g_smokeDirect3DTexture2ReleaseCalls == 1 &&
        g_smokeDirect3DTexture2ReleaseObjects[0] ==
            (IDirect3DTexture2 *)(&uploadTexture);

    InstallSmokeDirectDrawSurface(uploadSurface);
    InstallSmokeDirect3DTexture2(uploadTexture, targetTexture);
    textureRecord.m_uploadSurface = (IDirectDrawSurface *)(&uploadSurface);
    textureRecord.m_texture = (IDirect3DTexture2 *)(&targetTexture);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &uploadTexture;
    g_smokeDirect3DTexture2LoadResult = DDERR_GENERIC;
    g_smokeUploadImageToSurfaceCalls = 0;
    zVideo_dd3d::TextureRecord_FinalizeUpload(&textureRecord, 0, 0);
    const int loadFailureOk =
        g_smokeUploadImageToSurfaceCalls == 0 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        g_smokeDirect3DTexture2LoadCalls == 1 &&
        g_smokeDirect3DTexture2ReleaseCalls == 0;

    RestoreSmokeFunctionPatch(uploadPatch);
    return nullUploadOk && successOk && loadFailureOk ? 0 : 2;
}

extern "C" int zvideo_texture_record_create_and_power_smoke(void) {
    if (zVideo_dd3d::FloorPowerOfTwo(1) != 1 ||
        zVideo_dd3d::FloorPowerOfTwo(3) != 2 ||
        zVideo_dd3d::FloorPowerOfTwo(64) != 64 ||
        zVideo_dd3d::FloorPowerOfTwo(65) != 64) {
        return 1;
    }

    zVideo_TextureRecordPartial *const textureRecord =
        zVideo_dd3d::TextureRecord_Create();
    if (textureRecord == 0) {
        return 2;
    }

    const int zeroed =
        textureRecord->m_uploadSurface == 0 &&
        textureRecord->m_textureSurface == 0 &&
        textureRecord->m_texture == 0 &&
        textureRecord->m_textureHandle == 0 &&
        textureRecord->m_alphaMode == 0 &&
        textureRecord->m_uWrapMode == 0 &&
        textureRecord->m_vWrapMode == 0;
    std::free(textureRecord);
    return zeroed != 0 ? 0 : 3;
}

extern "C" int zvideo_texture_record_lock_upload_surface_smoke(void) {
    SmokeDirectDrawSurface3Object uploadSurface = {};
    zVideo_TextureRecordPartial textureRecord = {};
    textureRecord.m_uploadSurface = (IDirectDrawSurface *)(&uploadSurface);

    unsigned char pixels[16] = {};
    void *outPixels = (void *)(0x12345678);
    int outPitchBytes = -1;
    InstallSmokeDirectDrawSurface3(uploadSurface);
    g_smokeDirectDrawSurface3LockPixelsValue = pixels;
    g_smokeDirectDrawSurface3LockPitchValue = 32;
    const int successOk =
        zVideo_dd3d::TextureRecord_LockUploadSurface(
            &textureRecord,
            &outPixels,
            &outPitchBytes
        ) == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3LockSurfaces[0] ==
            (IDirectDrawSurface3 *)(&uploadSurface) &&
        g_smokeDirectDrawSurface3LockDescSize == sizeof(DDSURFACEDESC) &&
        outPixels == pixels &&
        outPitchBytes == 32;

    outPixels = (void *)(0x12345678);
    outPitchBytes = -1;
    InstallSmokeDirectDrawSurface3(uploadSurface);
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3LockResultCount = 1;
    const int failureResult =
        zVideo_dd3d::TextureRecord_LockUploadSurface(
            &textureRecord,
            &outPixels,
            &outPitchBytes
        );
    const int failureOk =
        failureResult == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        outPixels == (void *)(0x12345678) &&
        outPitchBytes == -1;

    if (!successOk) {
        return 2;
    }
    if (!failureOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvideo_texture_record_unlock_upload_surface_smoke(void) {
    SmokeDirectDrawSurface3Object uploadSurface = {};
    zVideo_TextureRecordPartial textureRecord = {};
    textureRecord.m_uploadSurface = (IDirectDrawSurface *)(&uploadSurface);

    InstallSmokeDirectDrawSurface3(uploadSurface);
    const int successOk =
        zVideo_dd3d::TextureRecord_UnlockUploadSurface(&textureRecord) == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockSurfaces[0] ==
            (IDirectDrawSurface3 *)(&uploadSurface) &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    InstallSmokeDirectDrawSurface3(uploadSurface);
    g_smokeDirectDrawSurface3UnlockResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3UnlockResultCount = 1;
    const int failureOk =
        zVideo_dd3d::TextureRecord_UnlockUploadSurface(&textureRecord) == 0 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    return successOk && failureOk ? 0 : 1;
}

extern "C" int zvideo_create_texture_record_guards_smoke(void) {
    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;
    zVideo_TextureRecordPartial *const savedDefaultRecord =
        g_zImage_DefaultTextureRecord;
    const D3DDEVICEDESC savedHalDesc = g_zVideo_D3DHalDeviceDesc;

    zVidD3DDriverRecordPartial selectedD3DDevice = {};
    D3DDEVICEDESC *const selectedDesc = &selectedD3DDevice.m_hwDesc;
    selectedDesc->dwMaxTextureWidth = 64;
    selectedDesc->dwMaxTextureHeight = 64;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3DDevice;

    zVideo_TextureRecordPartial defaultRecord = {};
    g_zImage_DefaultTextureRecord = &defaultRecord;

    zVidImagePartial image = {};
    image.width = 128;
    image.height = 8;
    g_zVideo_D3DHalDeviceDesc = {};
    const int tooLargeOk =
        zVideo_dd3d::CreateTextureRecord("too-large", &image, 0, 0, 0) ==
        &defaultRecord;

    image.width = 10;
    image.height = 8;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = D3DPTEXTURECAPS_POW2;
    const int nonPow2Ok =
        zVideo_dd3d::CreateTextureRecord("non-pow2", &image, 0, 0, 0) ==
        &defaultRecord;

    image.width = 64;
    image.height = 4;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = 0;
    const int badAspectOk =
        zVideo_dd3d::CreateTextureRecord("bad-aspect", &image, 0, 0, 0) ==
        &defaultRecord;

    image.width = 8;
    image.height = 8;
    image.palette = (void *)(0x1234);
    const int palettedOk =
        zVideo_dd3d::CreateTextureRecord("paletted", &image, 0, 0, 0) ==
        &defaultRecord;

    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    g_zVideo_D3DHalDeviceDesc = savedHalDesc;
    return tooLargeOk && nonPow2Ok && badAspectOk && palettedOk ? 0 : 1;
}

extern "C" int zvideo_dd3d_create_texture_record_smoke(void) {
    SmokeFunctionPatch uploadPatch = {};
    if (!PatchSmokeFunctionJump(
            (void *)(&zVideo_dd3d::UploadImageToSurface),
            (void *)(&SmokeUploadImageToSurface),
            uploadPatch
        )) {
        return 1;
    }

    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;
    zVideo_TextureRecordPartial *const savedDefaultRecord =
        g_zImage_DefaultTextureRecord;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    IDirect3DDevice2 *const savedD3DDevice = g_zVideo_pD3DDevice;
    const D3DDEVICEDESC savedHalDesc = g_zVideo_D3DHalDeviceDesc;
    const int savedTextureRBits = g_zVideo_TexturePixelPack_RBits;
    const int savedTextureGBits = g_zVideo_TexturePixelPack_GBits;
    const int savedTextureBBits = g_zVideo_TexturePixelPack_BBits;
    const int savedTextureABits = g_zVideo_TexturePixelPack_ABits;
    const unsigned int savedTextureRMask = g_zVideo_TexturePixelPack_RMask;
    const unsigned int savedTextureGMask = g_zVideo_TexturePixelPack_GMask;
    const unsigned int savedTextureBMask = g_zVideo_TexturePixelPack_BMask;
    const unsigned int savedTextureAMask = g_zVideo_TexturePixelPack_AMask;
    const int savedTextureRgbBitsTotal = g_zVideo_TexturePixelPack_RGBBitsTotal;
    const int savedTextureRgbBitsTotalMinus8 =
        g_zVideo_TexturePixelPack_RGBBitsTotalMinus8;
    const int savedTextureGbBitsTotalMinus8 =
        g_zVideo_TexturePixelPack_GBBitsTotalMinus8;
    const int savedTextureBShiftTo8 = g_zVideo_TexturePixelPack_BShiftTo8;
    const int savedTextureRMaskShifted = g_zVideo_TexturePixelPack_RMaskShifted;
    const int savedTextureGMaskShifted = g_zVideo_TexturePixelPack_GMaskShifted;
    const int savedTextureBMaskShifted = g_zVideo_TexturePixelPack_BMaskShifted;
    const int savedTextureNonRgbMaskShifted =
        g_zVideo_TexturePixelPack_NonRgbMaskShifted;

    zVidD3DDriverRecordPartial selectedD3DDevice = {};
    D3DDEVICEDESC *const selectedDesc = &selectedD3DDevice.m_hwDesc;
    selectedDesc->dwMaxTextureWidth = 64;
    selectedDesc->dwMaxTextureHeight = 64;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3DDevice;

    zVideo_TextureRecordPartial defaultRecord = {};
    g_zImage_DefaultTextureRecord = &defaultRecord;

    SmokeDirectDraw2Object directDraw = {};
    SmokeDirectDrawSurfaceObject createdSurface = {};
    SmokeDirectDrawPaletteObject palette = {};
    SmokeDirect3DDevice2Object d3dDevice = {};
    SmokeDirect3DTexture2Object uploadTexture = {};
    SmokeDirect3DTexture2Object texture = {};
    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(createdSurface);
    InstallSmokeDirectDrawPalette(palette);
    InstallSmokeDirect3DDevice2(d3dDevice);
    InstallSmokeDirect3DTexture2(uploadTexture, texture);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pD3DDevice = (IDirect3DDevice2 *)(&d3dDevice);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&createdSurface);
    g_smokeDirectDraw2CreatePaletteValue = (IDirectDrawPalette *)(&palette);
    g_smokeDirectDrawSurfaceQueryInterfaceValues[0] = &uploadTexture;
    g_smokeDirectDrawSurfaceQueryInterfaceValues[1] = &texture;
    g_smokeDirectDrawSurfaceQueryInterfaceValueCount = 2;
    g_smokeDirect3DTexture2HandleValue = 0x3579;

    zVidImagePartial image = {};
    unsigned short pixels[32] = {};
    unsigned char alphaMap[32] = {};
    PALETTEENTRY deferredPalette[2] = {};
    deferredPalette[0].peRed = 10;
    deferredPalette[1].peBlue = 20;
    image.width = 8;
    image.height = 4;
    image.pixels = pixels;
    image.alphaMap = (char *)(alphaMap);
    image.palette = 0;
    image.paletteMetaPacked = (short)(2 * sizeof(PALETTEENTRY));

    g_smokeDirectDraw2MutateImageOnFirstCreateSurface = &image;
    g_smokeDirectDraw2MutatedPalette = deferredPalette;
    g_smokeDirectDraw2MutatedPaletteMetaPacked = image.paletteMetaPacked;
    g_zVideo_D3DHalDeviceDesc = {};
    g_zVideo_D3DHalDeviceDesc.dwDevCaps = D3DDEVCAPS_TEXTURENONLOCALVIDMEM;
    g_smokeUploadImageToSurfaceCalls = 0;
    g_smokeUploadImageToSurfaceSurface = 0;
    g_smokeUploadImageToSurfaceImage = 0;
    g_smokeUploadImageToSurfaceUseAlpha = -1;
    g_smokeUploadImageToSurfaceResult = 1;

    zVideo_TextureRecordPartial *result =
        zVideo_dd3d::CreateTextureRecord("success", &image, 1, 1, 0);
    const DDSURFACEDESC &uploadDesc = g_smokeDirectDraw2CreateSurfaceDescs[0];
    const DDSURFACEDESC &textureDesc = g_smokeDirectDraw2CreateSurfaceDescs[1];
    const int successOk =
        result != 0 &&
        result != &defaultRecord &&
        g_smokeDirectDraw2CreateSurfaceCalls == 2 &&
        uploadDesc.dwSize == sizeof(DDSURFACEDESC) &&
        uploadDesc.dwFlags == (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT) &&
        uploadDesc.dwWidth == 8 &&
        uploadDesc.dwHeight == 4 &&
        uploadDesc.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY) &&
        uploadDesc.ddpfPixelFormat.dwFlags == (DDPF_RGB | DDPF_ALPHAPIXELS) &&
        uploadDesc.ddpfPixelFormat.dwRGBBitCount == 16 &&
        uploadDesc.ddpfPixelFormat.dwRBitMask == 0x0f00 &&
        uploadDesc.ddpfPixelFormat.dwGBitMask == 0x00f0 &&
        uploadDesc.ddpfPixelFormat.dwBBitMask == 0x000f &&
        uploadDesc.ddpfPixelFormat.dwRGBAlphaBitMask == 0xf000 &&
        textureDesc.ddsCaps.dwCaps ==
            (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD |
             DDSCAPS_NONLOCALVIDMEM) &&
        g_smokeDirectDraw2CreatePaletteCalls == 1 &&
        g_smokeDirectDraw2LastCreatePaletteFlags ==
            (DDPCAPS_8BIT | DDPCAPS_ALLOW256) &&
        g_smokeDirectDraw2LastCreatePaletteEntries == deferredPalette &&
        g_smokeDirectDrawSurfaceSetPaletteCalls == 2 &&
        g_smokeDirectDrawSurfaceSetPalettePalettes[0] ==
            (IDirectDrawPalette *)(&palette) &&
        g_smokeDirectDrawSurfaceSetPalettePalettes[1] ==
            (IDirectDrawPalette *)(&palette) &&
        g_smokeUploadImageToSurfaceCalls == 1 &&
        g_smokeUploadImageToSurfaceSurface ==
            (IDirectDrawSurface *)(&createdSurface) &&
        g_smokeUploadImageToSurfaceImage == &image &&
        g_smokeUploadImageToSurfaceUseAlpha == 1 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 2 &&
        g_smokeDirect3DTexture2LoadCalls == 1 &&
        g_smokeDirect3DTexture2LastLoadSelf ==
            (IDirect3DTexture2 *)(&texture) &&
        g_smokeDirect3DTexture2LastLoadSource ==
            (IDirect3DTexture2 *)(&uploadTexture) &&
        g_smokeDirect3DTexture2GetHandleCalls == 1 &&
        g_smokeDirect3DTexture2LastGetHandleDevice == g_zVideo_pD3DDevice &&
        g_smokeDirect3DTexture2ReleaseCalls == 1 &&
        g_smokeDirect3DTexture2ReleaseObjects[0] ==
            (IDirect3DTexture2 *)(&uploadTexture) &&
        result->m_uploadSurface == (IDirectDrawSurface *)(&createdSurface) &&
        result->m_textureSurface == (IDirectDrawSurface *)(&createdSurface) &&
        result->m_texture == (IDirect3DTexture2 *)(&texture) &&
        result->m_textureHandle == 0x3579 &&
        result->m_alphaMode == 4 &&
        result->m_uWrapMode == D3DTADDRESS_CLAMP &&
        result->m_vWrapMode == D3DTADDRESS_WRAP &&
        g_zVideo_TexturePixelPack_RBits == 4 &&
        g_zVideo_TexturePixelPack_GBits == 4 &&
        g_zVideo_TexturePixelPack_BBits == 4 &&
        g_zVideo_TexturePixelPack_ABits == 4;
    if (result != 0 && result != &defaultRecord) {
        std::free(result);
    }

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(createdSurface);
    InstallSmokeDirect3DTexture2(uploadTexture, texture);
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pD3DDevice = (IDirect3DDevice2 *)(&d3dDevice);
    g_smokeDirectDraw2CreateSurfaceValue = (IDirectDrawSurface *)(&createdSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValues[0] = &uploadTexture;
    g_smokeDirectDrawSurfaceQueryInterfaceValues[1] = &texture;
    g_smokeDirectDrawSurfaceQueryInterfaceValueCount = 2;
    g_smokeDirect3DTexture2LoadResult = DDERR_GENERIC;
    g_smokeUploadImageToSurfaceCalls = 0;
    image.palette = 0;
    image.paletteMetaPacked = 0;

    zVideo_TextureRecordPartial *failureResult =
        zVideo_dd3d::CreateTextureRecord("load-failure", &image, 1, 0, 1);
    const int failureOk =
        failureResult == &defaultRecord &&
        g_smokeDirectDraw2CreateSurfaceCalls == 2 &&
        g_smokeDirectDraw2CreatePaletteCalls == 0 &&
        g_smokeUploadImageToSurfaceCalls == 1 &&
        g_smokeDirect3DTexture2LoadCalls == 1 &&
        g_smokeDirect3DTexture2ReleaseCalls == 2 &&
        g_smokeDirect3DTexture2ReleaseObjects[0] ==
            (IDirect3DTexture2 *)(&texture) &&
        g_smokeDirect3DTexture2ReleaseObjects[1] ==
            (IDirect3DTexture2 *)(&uploadTexture) &&
        g_smokeDirectDrawSurfaceReleaseCalls == 2;

    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pD3DDevice = savedD3DDevice;
    g_zVideo_D3DHalDeviceDesc = savedHalDesc;
    g_zVideo_TexturePixelPack_RBits = savedTextureRBits;
    g_zVideo_TexturePixelPack_GBits = savedTextureGBits;
    g_zVideo_TexturePixelPack_BBits = savedTextureBBits;
    g_zVideo_TexturePixelPack_ABits = savedTextureABits;
    g_zVideo_TexturePixelPack_RMask = savedTextureRMask;
    g_zVideo_TexturePixelPack_GMask = savedTextureGMask;
    g_zVideo_TexturePixelPack_BMask = savedTextureBMask;
    g_zVideo_TexturePixelPack_AMask = savedTextureAMask;
    g_zVideo_TexturePixelPack_RGBBitsTotal = savedTextureRgbBitsTotal;
    g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 =
        savedTextureRgbBitsTotalMinus8;
    g_zVideo_TexturePixelPack_GBBitsTotalMinus8 =
        savedTextureGbBitsTotalMinus8;
    g_zVideo_TexturePixelPack_BShiftTo8 = savedTextureBShiftTo8;
    g_zVideo_TexturePixelPack_RMaskShifted = savedTextureRMaskShifted;
    g_zVideo_TexturePixelPack_GMaskShifted = savedTextureGMaskShifted;
    g_zVideo_TexturePixelPack_BMaskShifted = savedTextureBMaskShifted;
    g_zVideo_TexturePixelPack_NonRgbMaskShifted =
        savedTextureNonRgbMaskShifted;
    RestoreSmokeFunctionPatch(uploadPatch);
    return successOk && failureOk ? 0 : 2;
}

extern "C" int zvideo_frustum_test_sphere_clip_mask_smoke(void) {
    zClass_CameraDataPartial *savedViewContext = g_zVideo_pActiveViewContext;
    zClass_CameraDataPartial viewContext = {};
    viewContext.cameraPos = {0.0f, 0.0f, 0.0f};
    viewContext.nearClipCenter = {0.0f, 0.0f, 1.0f};
    viewContext.farClipCenter = {0.0f, 0.0f, 10.0f};
    viewContext.worldFrustumNormals[0] = {1.0f, 0.0f, 0.0f};
    viewContext.worldFrustumNormals[4] = {0.0f, 0.0f, 1.0f};
    viewContext.worldFrustumNormals[5] = {0.0f, 0.0f, -1.0f};
    g_zVideo_pActiveViewContext = &viewContext;

    zVec3 sphere = {0.0f, 0.0f, 0.0f};
    int clipMask = 0x10;
    int result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0x10 || clipMask != 0) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 1;
    }

    sphere = {0.0f, 0.0f, 1.25f};
    clipMask = 0x10;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0 || clipMask != 0x10) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 2;
    }

    sphere = {-1.0f, 0.0f, 2.0f};
    clipMask = 1;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 1 || clipMask != 0) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 3;
    }

    sphere = {0.25f, 0.0f, 2.0f};
    clipMask = 1;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0 || clipMask != 1) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 4;
    }

    sphere = {2.0f, 0.0f, 9.75f};
    clipMask = 0x21;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0 || clipMask != 0x20) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 5;
    }

    sphere = {2.0f, 0.0f, 12.0f};
    clipMask = 0x20;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0x20 || clipMask != 0) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 6;
    }

    g_zVideo_pActiveViewContext = savedViewContext;
    return 0;
}

extern "C" int zvideo_quad_batch_depth_and_rhw_smoke(void) {
    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));

    zVideo_dd3d::SetQuadBatchDepthAndRhw(0.25f);

    for (int itemIndex = 0; itemIndex < 16; ++itemIndex) {
        for (int vertexIndex = 0; vertexIndex < 4; ++vertexIndex) {
            const D3DTLVERTEX &vertex =
                g_zVideo_QuadBatchItemsBase[itemIndex].vertices[vertexIndex];
            if (vertex.sz != 0.25f || vertex.rhw != 0.25f) {
                return itemIndex + vertexIndex + 1;
            }
        }
    }

    return 0;
}

extern "C" int zvideo_queue_solid_quad_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    zVideo_dd3d::QueueSolidQuad(0xf800, nullptr, 0.5);
    if (g_zVideo_QuadBatchCount != 1) {
        return 1;
    }

    const zVideo_QuadBatchItemPartial &full = g_zVideo_QuadBatchItemsBase[0];
    if (full.vertices[0].sx != 0.0f || full.vertices[0].sy != 0.0f ||
        full.vertices[1].sx != 480.0f || full.vertices[1].sy != 0.0f ||
        full.vertices[2].sx != 480.0f || full.vertices[2].sy != 640.0f ||
        full.vertices[3].sx != 0.0f || full.vertices[3].sy != 640.0f ||
        full.vertices[0].color != 0x7ff80000 || full.vertices[3].color != 0x7ff80000) {
        return 2;
    }

    zVidRect32 rect = {10, 20, 30, 40};
    zVideo_dd3d::QueueSolidQuad(0x07e0, &rect, 0.25);
    if (g_zVideo_QuadBatchCount != 2) {
        return 3;
    }

    const zVideo_QuadBatchItemPartial &clipped = g_zVideo_QuadBatchItemsBase[1];
    if (clipped.vertices[0].sx != 10.0f || clipped.vertices[0].sy != 20.0f ||
        clipped.vertices[1].sx != 30.0f || clipped.vertices[1].sy != 20.0f ||
        clipped.vertices[2].sx != 30.0f || clipped.vertices[2].sy != 40.0f ||
        clipped.vertices[3].sx != 10.0f || clipped.vertices[3].sy != 40.0f ||
        clipped.vertices[0].color != 0x3f00fc00 ||
        clipped.vertices[2].color != 0x3f00fc00) {
        return 4;
    }

    g_zVideo_QuadBatchCount = 0x10;
    zVideo_dd3d::QueueSolidQuad(0xffff, &rect, 1.0);
    return g_zVideo_QuadBatchCount == 0x10 ? 0 : 5;
}

extern "C" int zvideo_flush_quad_batch_empty_smoke(void) {
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice = 0;
    zVideo_dd3d::FlushQuadBatch();
    return g_zVideo_QuadBatchCount == 0 ? 0 : 1;
}

extern "C" int zvideo_flush_quad_batch_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedQuadBatchCount = g_zVideo_QuadBatchCount;
    const int savedShadeMode = g_zVideo_D3DRenderStateCache.shadeMode;
    const int savedAlphaBlendEnable = g_zVideo_D3DRenderStateCache.alphaBlendEnable;
    const int savedZWriteEnable = g_zVideo_D3DRenderStateCache.zWriteEnable;
    const D3DTEXTUREHANDLE savedTextureHandle =
        g_zVideo_D3DRenderStateCache.textureHandle;

    std::memset(
        g_zVideo_QuadBatchItemsBase,
        0,
        sizeof(g_zVideo_QuadBatchItemsBase[0]) * 2
    );
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    g_zVideo_D3DRenderStateCache.textureHandle = 0x2468;

    g_zVideo_QuadBatchItemsBase[0].vertices[0].sx = 10.0f;
    g_zVideo_QuadBatchItemsBase[0].vertices[1].sx = 11.0f;
    g_zVideo_QuadBatchItemsBase[0].vertices[2].sx = 12.0f;
    g_zVideo_QuadBatchItemsBase[0].vertices[3].sx = 13.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[0].sx = 20.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[1].sx = 21.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[2].sx = 22.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[3].sx = 23.0f;
    g_zVideo_QuadBatchCount = 2;

    zVideo_dd3d::FlushQuadBatch();

    const bool setupStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 8 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 2 &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 1 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 0 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 0 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[4] == D3DCMP_ALWAYS;

    const bool drawOk =
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 2 &&
        g_smokeDirect3DDevice2PrimitiveTypes[0] == D3DPT_TRIANGLEFAN &&
        g_smokeDirect3DDevice2PrimitiveTypes[1] == D3DPT_TRIANGLEFAN &&
        g_smokeDirect3DDevice2VertexTypes[0] == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2VertexTypes[1] == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2Vertices[0] == g_zVideo_QuadBatchItemsBase[0].vertices &&
        g_smokeDirect3DDevice2Vertices[1] == g_zVideo_QuadBatchItemsBase[1].vertices &&
        g_smokeDirect3DDevice2VertexCounts[0] == 4 &&
        g_smokeDirect3DDevice2VertexCounts[1] == 4 &&
        g_smokeDirect3DDevice2DrawFlags[0] == 0 &&
        g_smokeDirect3DDevice2DrawFlags[1] == 0;

    const bool restoreStateOk =
        g_smokeDirect3DDevice2RenderStates[5] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[5] == D3DCMP_GREATEREQUAL &&
        g_smokeDirect3DDevice2RenderStates[6] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[6] == 0 &&
        g_smokeDirect3DDevice2RenderStates[7] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[7] == 1 &&
        g_zVideo_QuadBatchCount == 0 &&
        g_zVideo_D3DRenderStateCache.shadeMode == 2 &&
        g_zVideo_D3DRenderStateCache.alphaBlendEnable == 0 &&
        g_zVideo_D3DRenderStateCache.zWriteEnable == 1 &&
        g_zVideo_D3DRenderStateCache.textureHandle == 0;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 2;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_QuadBatchCount = 1;
    zVideo_dd3d::FlushQuadBatch();

    const bool cacheHitOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 4 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DCMP_ALWAYS &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[1] == D3DCMP_GREATEREQUAL &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 0 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 1 &&
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_zVideo_QuadBatchCount == 0 &&
        g_zVideo_D3DRenderStateCache.zWriteEnable == 1;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_QuadBatchCount = savedQuadBatchCount;
    g_zVideo_D3DRenderStateCache.shadeMode = savedShadeMode;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = savedAlphaBlendEnable;
    g_zVideo_D3DRenderStateCache.zWriteEnable = savedZWriteEnable;
    g_zVideo_D3DRenderStateCache.textureHandle = savedTextureHandle;

    if (!setupStateOk) {
        return 1;
    }
    if (!drawOk) {
        return 2;
    }
    if (!restoreStateOk) {
        return 3;
    }
    return cacheHitOk ? 0 : 4;
}

extern "C" int zvideo_flush_overwrite_polys_empty_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedOverwriteCount = g_zVideo_OverwriteQueueCount;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_OverwriteQueueCount = 0;
    zVideo_dd3d::FlushOverwritePolys();

    const bool stateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 2 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DCMP_ALWAYS &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[1] == D3DCMP_GREATEREQUAL &&
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 0 &&
        g_zVideo_OverwriteQueueCount == 0;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_OverwriteQueueCount = savedOverwriteCount;
    return stateOk ? 0 : 1;
}

extern "C" int zvideo_flush_overwrite_polys_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedOverwriteCount = g_zVideo_OverwriteQueueCount;
    const int savedShadeMode = g_zVideo_D3DRenderStateCache.shadeMode;
    const int savedAlphaBlendEnable = g_zVideo_D3DRenderStateCache.alphaBlendEnable;
    const int savedZWriteEnable = g_zVideo_D3DRenderStateCache.zWriteEnable;
    const D3DTEXTUREHANDLE savedTextureHandle =
        g_zVideo_D3DRenderStateCache.textureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderStateCache.textureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderStateCache.textureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderStateCache.textureAddressV;

    zVideo_RenderClass transparentClass = {};
    transparentClass.textureHandle = 0x1111;
    transparentClass.textureMapBlend = (D3DTEXTUREBLEND)(2);
    transparentClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    transparentClass.textureAddressV = (D3DTEXTUREADDRESS)(2);

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(0);
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &transparentEntry = g_zVideo_OverwriteQueueBase[0];
    transparentEntry.type = 0;
    transparentEntry.vertexCount = 4;
    transparentEntry.renderClass = (int)(&transparentClass);
    transparentEntry.vertices[0].sx = 10.0f;
    transparentEntry.vertices[0].color = 0x7f112233;
    zVideo_dd3d::FlushOverwritePolys();

    const bool transparentStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 11 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DCMP_ALWAYS &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 2 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 1 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 0 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 0x1111 &&
        g_smokeDirect3DDevice2RenderStates[5] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[5] == 4 &&
        g_smokeDirect3DDevice2RenderStates[6] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[6] == 1 &&
        g_smokeDirect3DDevice2RenderStates[7] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[7] == 2 &&
        g_smokeDirect3DDevice2RenderStates[8] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[8] == 0 &&
        g_smokeDirect3DDevice2RenderStates[9] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[9] == 1 &&
        g_smokeDirect3DDevice2RenderStates[10] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[10] == D3DCMP_GREATEREQUAL &&
        g_zVideo_D3DRenderStateCache.textureMapBlend == (D3DTEXTUREBLEND)(4) &&
        g_zVideo_OverwriteQueueCount == 0;

    const bool transparentDrawOk =
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2PrimitiveTypes[0] == D3DPT_TRIANGLEFAN &&
        g_smokeDirect3DDevice2VertexTypes[0] == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2Vertices[0] == transparentEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[0] == 4 &&
        g_smokeDirect3DDevice2DrawFlags[0] == 0;

    zVideo_RenderClass texturedClass = {};
    texturedClass.textureHandle = 0x4444;
    texturedClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    texturedClass.textureAddressU = (D3DTEXTUREADDRESS)(5);
    texturedClass.textureAddressV = (D3DTEXTUREADDRESS)(6);

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 2;
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(0);
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &texturedEntry = g_zVideo_OverwriteQueueBase[0];
    texturedEntry.type = 4;
    texturedEntry.vertexCount = 3;
    texturedEntry.renderClass = (int)(&texturedClass);
    texturedEntry.vertices[0].sx = 20.0f;
    zVideo_dd3d::FlushOverwritePolys();

    const bool texturedStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 7 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DCMP_ALWAYS &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 1 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 0x4444 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 3 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 5 &&
        g_smokeDirect3DDevice2RenderStates[5] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[5] == 6 &&
        g_smokeDirect3DDevice2RenderStates[6] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[6] == D3DCMP_GREATEREQUAL &&
        g_smokeDirect3DDevice2Vertices[0] == texturedEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[0] == 3 &&
        g_zVideo_OverwriteQueueCount == 0;

    zVideo_RenderClass modulateClass = {};
    modulateClass.textureHandle = 0x5555;
    modulateClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
    modulateClass.textureAddressU = (D3DTEXTUREADDRESS)(7);
    modulateClass.textureAddressV = (D3DTEXTUREADDRESS)(8);

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(0);
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &modulateEntry = g_zVideo_OverwriteQueueBase[0];
    modulateEntry.type = 6;
    modulateEntry.vertexCount = 5;
    modulateEntry.renderClass = (int)(&modulateClass);
    modulateEntry.vertices[0].sx = 30.0f;
    zVideo_dd3d::FlushOverwritePolys();

    const bool modulateStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 7 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DCMP_ALWAYS &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 2 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 0x5555 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 2 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 7 &&
        g_smokeDirect3DDevice2RenderStates[5] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[5] == 8 &&
        g_smokeDirect3DDevice2RenderStates[6] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[6] == D3DCMP_GREATEREQUAL &&
        g_smokeDirect3DDevice2Vertices[0] == modulateEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[0] == 5 &&
        g_zVideo_D3DRenderStateCache.textureMapBlend == (D3DTEXTUREBLEND)(2) &&
        g_zVideo_OverwriteQueueCount == 0;

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 2;
    g_zVideo_D3DRenderStateCache.textureHandle = 0x9999;
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &flatEntry = g_zVideo_OverwriteQueueBase[0];
    flatEntry.type = 2;
    flatEntry.vertexCount = 2;
    flatEntry.renderClass = 0;
    flatEntry.vertices[0].sx = 40.0f;
    zVideo_dd3d::FlushOverwritePolys();

    const bool flatStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 4 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[0] == D3DCMP_ALWAYS &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 0 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 1 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[3] == D3DCMP_GREATEREQUAL &&
        g_smokeDirect3DDevice2Vertices[0] == flatEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[0] == 2 &&
        g_zVideo_D3DRenderStateCache.textureHandle == 0 &&
        g_zVideo_D3DRenderStateCache.shadeMode == 1 &&
        g_zVideo_OverwriteQueueCount == 0;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_OverwriteQueueCount = savedOverwriteCount;
    g_zVideo_D3DRenderStateCache.shadeMode = savedShadeMode;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = savedAlphaBlendEnable;
    g_zVideo_D3DRenderStateCache.zWriteEnable = savedZWriteEnable;
    g_zVideo_D3DRenderStateCache.textureHandle = savedTextureHandle;
    g_zVideo_D3DRenderStateCache.textureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderStateCache.textureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderStateCache.textureAddressV = savedTextureAddressV;

    if (!transparentStateOk || !transparentDrawOk) {
        return 1;
    }
    if (!texturedStateOk) {
        return 2;
    }
    if (!modulateStateOk) {
        return 3;
    }
    return flatStateOk ? 0 : 4;
}

extern "C" int zvideo_flush_sorted_polys_empty_smoke(void) {
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_pD3DDevice = 0;
    zVideo_dd3d::FlushSortedPolys();
    return g_zVideo_SortedPolyQueueCount == 0 ? 0 : 1;
}

extern "C" int zvideo_flush_sorted_polys_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedSortedCount = g_zVideo_SortedPolyQueueCount;
    const int savedShadeMode = g_zVideo_D3DRenderStateCache.shadeMode;
    const int savedAlphaBlendEnable = g_zVideo_D3DRenderStateCache.alphaBlendEnable;
    const int savedZWriteEnable = g_zVideo_D3DRenderStateCache.zWriteEnable;
    const D3DTEXTUREHANDLE savedTextureHandle =
        g_zVideo_D3DRenderStateCache.textureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderStateCache.textureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderStateCache.textureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderStateCache.textureAddressV;

    std::memset(
        g_zVideo_SortedPolyQueueBase,
        0,
        sizeof(g_zVideo_SortedPolyQueueBase[0]) * 3
    );
    std::memset(
        g_zVideo_SortedPolyDrawOrder,
        0,
        sizeof(g_zVideo_SortedPolyDrawOrder)
    );
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    g_zVideo_D3DRenderStateCache.textureHandle = 0x9999;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(0);

    zVideo_RenderClass farClass = {};
    farClass.textureHandle = 0x1111;
    farClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    farClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    farClass.textureAddressV = (D3DTEXTUREADDRESS)(2);

    zVideo_RenderClass middleClass = {};
    middleClass.textureHandle = 0x2222;
    middleClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    middleClass.textureAddressU = (D3DTEXTUREADDRESS)(5);
    middleClass.textureAddressV = (D3DTEXTUREADDRESS)(6);

    zVideo_SortedPolyQueueEntry &farEntry = g_zVideo_SortedPolyQueueBase[0];
    farEntry.vertexCount = 3;
    farEntry.renderClass = (int)(&farClass);
    farEntry.vertices[0].sx = 30.0f;
    farEntry.vertices[0].sz = 30.0f;
    farEntry.vertices[0].color = 0xff445566;

    zVideo_SortedPolyQueueEntry &nearEntry = g_zVideo_SortedPolyQueueBase[1];
    nearEntry.vertexCount = 1;
    nearEntry.renderClass = 0;
    nearEntry.vertices[0].sx = 10.0f;
    nearEntry.vertices[0].sz = 10.0f;
    nearEntry.vertices[0].color = 0xff101010;

    zVideo_SortedPolyQueueEntry &middleEntry = g_zVideo_SortedPolyQueueBase[2];
    middleEntry.vertexCount = 2;
    middleEntry.renderClass = (int)(&middleClass);
    middleEntry.vertices[0].sx = 20.0f;
    middleEntry.vertices[0].sz = 20.0f;
    middleEntry.vertices[0].color = 0x7f112233;

    g_zVideo_SortedPolyQueueCount = 3;
    zVideo_dd3d::FlushSortedPolys();

    const bool initialStateOk =
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 2 &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 1 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 0 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 0;

    const bool middleStateOk =
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 0x2222 &&
        g_smokeDirect3DDevice2RenderStates[5] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[5] == 4 &&
        g_smokeDirect3DDevice2RenderStates[6] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[6] == 5 &&
        g_smokeDirect3DDevice2RenderStates[7] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[7] == 6;

    const bool farStateOk =
        g_smokeDirect3DDevice2RenderStates[8] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[8] == 0x1111 &&
        g_smokeDirect3DDevice2RenderStates[9] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[9] == 3 &&
        g_smokeDirect3DDevice2RenderStates[10] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[10] == 1 &&
        g_smokeDirect3DDevice2RenderStates[11] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[11] == 2;

    const bool restoreStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 14 &&
        g_smokeDirect3DDevice2RenderStates[12] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[12] == 0 &&
        g_smokeDirect3DDevice2RenderStates[13] == D3DRENDERSTATE_ZWRITEENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[13] == 1 &&
        g_zVideo_D3DRenderStateCache.shadeMode == 2 &&
        g_zVideo_D3DRenderStateCache.alphaBlendEnable == 0 &&
        g_zVideo_D3DRenderStateCache.zWriteEnable == 1 &&
        g_zVideo_D3DRenderStateCache.textureHandle == 0x1111 &&
        g_zVideo_D3DRenderStateCache.textureMapBlend == (D3DTEXTUREBLEND)(3) &&
        g_zVideo_D3DRenderStateCache.textureAddressU == (D3DTEXTUREADDRESS)(1) &&
        g_zVideo_D3DRenderStateCache.textureAddressV == (D3DTEXTUREADDRESS)(2);

    const bool drawOrderOk =
        g_zVideo_SortedPolyDrawOrder[0] == 1 &&
        g_zVideo_SortedPolyDrawOrder[1] == 2 &&
        g_zVideo_SortedPolyDrawOrder[2] == 0 &&
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 3 &&
        g_smokeDirect3DDevice2Vertices[0] == nearEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[0] == 1 &&
        g_smokeDirect3DDevice2Vertices[1] == middleEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[1] == 2 &&
        g_smokeDirect3DDevice2Vertices[2] == farEntry.vertices &&
        g_smokeDirect3DDevice2VertexCounts[2] == 3;

    const bool drawArgsOk =
        g_smokeDirect3DDevice2PrimitiveTypes[0] == D3DPT_TRIANGLEFAN &&
        g_smokeDirect3DDevice2PrimitiveTypes[1] == D3DPT_TRIANGLEFAN &&
        g_smokeDirect3DDevice2PrimitiveTypes[2] == D3DPT_TRIANGLEFAN &&
        g_smokeDirect3DDevice2VertexTypes[0] == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2VertexTypes[1] == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2VertexTypes[2] == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2DrawFlags[0] == 0 &&
        g_smokeDirect3DDevice2DrawFlags[1] == 0 &&
        g_smokeDirect3DDevice2DrawFlags[2] == 0 &&
        g_zVideo_SortedPolyQueueCount == 0;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_SortedPolyQueueCount = savedSortedCount;
    g_zVideo_D3DRenderStateCache.shadeMode = savedShadeMode;
    g_zVideo_D3DRenderStateCache.alphaBlendEnable = savedAlphaBlendEnable;
    g_zVideo_D3DRenderStateCache.zWriteEnable = savedZWriteEnable;
    g_zVideo_D3DRenderStateCache.textureHandle = savedTextureHandle;
    g_zVideo_D3DRenderStateCache.textureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderStateCache.textureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderStateCache.textureAddressV = savedTextureAddressV;

    if (!initialStateOk) {
        return 1;
    }
    if (!middleStateOk) {
        return 2;
    }
    if (!farStateOk) {
        return 3;
    }
    if (!restoreStateOk) {
        return 4;
    }
    if (!drawOrderOk) {
        return 5;
    }
    return drawArgsOk ? 0 : 6;
}

extern "C" int zvideo_submit_poly_color_attr_smoke(void) {
    const int savedNormalizeChannel = g_zVideo_D3DColorNormalizeChannelIndex;
    const float savedBiasR = g_zVideo_D3DColorAttrBiasR;
    const float savedBiasG = g_zVideo_D3DColorAttrBiasG;
    const float savedBiasB = g_zVideo_D3DColorAttrBiasB;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    zVideo_ColorRgbFloat baseColor = {16.0f, 32.0f, 48.0f};
    float attr1[1] = {0.0f};
    float attr0[3] = {0.0f, 0.5f, 1.0f};
    float attr2[3] = {0.0f, 0.5f, 1.0f};

    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;
    zVideo_dd3d::SubmitPolyColorAttr(
        vertices,
        0,
        &baseColor,
        attr1,
        attr0,
        attr2,
        0x80,
        3,
        0x44,
        1
    );

    int status = 0;
    if (g_zVideo_OverwriteQueueCount != 0 || g_zVideo_SortedPolyQueueCount != 0) {
        status = 1;
    } else if (g_zVideo_D3DSubmitTempVertices[0].color != 0x8055aaff ||
               g_zVideo_D3DSubmitTempVertices[0].specular != 0x00000000 ||
               g_zVideo_D3DSubmitTempVertices[1].color != 0x804284c6 ||
               g_zVideo_D3DSubmitTempVertices[1].specular != 0x80000000 ||
               g_zVideo_D3DSubmitTempVertices[2].color != 0x80102030 ||
               g_zVideo_D3DSubmitTempVertices[2].specular != 0xff000000) {
        status = 2;
    } else {
        std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
        attr1[0] = 0.0f;
        zVideo_dd3d::SubmitPolyColorAttr(
            vertices,
            0,
            &baseColor,
            attr1,
            0,
            0,
            0xff,
            3,
            0x55,
            1
        );

        const D3DTLVERTEX &first = g_zVideo_OverwriteQueueBase[0].vertices[0];
        const D3DTLVERTEX &last = g_zVideo_OverwriteQueueBase[0].vertices[2];
        if (g_zVideo_OverwriteQueueCount != 1 ||
            g_zVideo_OverwriteQueueBase[0].type != 3 ||
            g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
            g_zVideo_OverwriteQueueBase[0].renderClass != 0 ||
            g_zVideo_OverwriteQueueBase[0].renderParam != 0x55) {
            status = 3;
        } else if (first.sx != 7.0f || first.sy != 8.0f ||
                   first.sz != 9.0f || first.rhw != 9.0f ||
                   first.color != 0xff102030 ||
                   first.specular != 0xff000000 ||
                   last.sx != 1.0f || last.color != 0xff102030) {
            status = 4;
        }
    }

    g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
    g_zVideo_D3DColorAttrBiasR = savedBiasR;
    g_zVideo_D3DColorAttrBiasG = savedBiasG;
    g_zVideo_D3DColorAttrBiasB = savedBiasB;
    return status;
}

extern "C" int zvideo_submit_poly_color_attr_immediate_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const D3DTEXTUREHANDLE savedTextureHandle =
        g_zVideo_D3DRenderStateCache.textureHandle;
    const int savedShadeMode = g_zVideo_D3DRenderStateCache.shadeMode;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.textureHandle = 0x2468;
    g_zVideo_D3DRenderStateCache.shadeMode = 2;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    zVideo_ColorRgbFloat baseColor = {16.0f, 32.0f, 48.0f};
    float attr1[1] = {0.0f};
    float attr2[3] = {0.0f, 0.5f, 1.0f};

    zVideo_dd3d::SubmitPolyColorAttr(
        vertices,
        0,
        &baseColor,
        attr1,
        0,
        attr2,
        0xff,
        3,
        0x66,
        0
    );

    const int renderStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 2 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 0 &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 1 &&
        g_zVideo_D3DRenderStateCache.textureHandle == 0 &&
        g_zVideo_D3DRenderStateCache.shadeMode == 1;
    const int drawOk =
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2LastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        g_smokeDirect3DDevice2LastVertexType == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2LastVertices == g_zVideo_D3DSubmitTempVertices &&
        g_smokeDirect3DDevice2LastVertexCount == 3 &&
        g_smokeDirect3DDevice2LastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0;
    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &last = g_zVideo_D3DSubmitTempVertices[2];
    const int verticesOk =
        first.sx == 7.0f && first.sy == 8.0f && first.sz == 9.0f &&
        first.rhw == 9.0f && first.color == 0xff102030 &&
        first.specular == 0x00000000 && last.sx == 1.0f &&
        last.sy == 2.0f && last.sz == 3.0f && last.rhw == 3.0f &&
        last.color == 0xff102030 && last.specular == 0xff000000;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    zVideo_dd3d::SubmitPolyColorAttr(
        vertices,
        0,
        &baseColor,
        attr1,
        0,
        0,
        0xff,
        2,
        0x77,
        0
    );
    const int cacheHitOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 0 &&
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2LastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xff102030 &&
        g_zVideo_D3DSubmitTempVertices[0].specular == 0xff000000;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderStateCache.textureHandle = savedTextureHandle;
    g_zVideo_D3DRenderStateCache.shadeMode = savedShadeMode;
    return renderStateOk && drawOk && verticesOk && cacheHitOk ? 0 : 1;
}

extern "C" int zvideo_submit_polygon_queue_smoke(void) {
    const int savedAppendFanClose = g_zVideo_D3DAppendFanCloseVertexPending;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f}
    };
    zVideo_RenderClass renderClass = {};
    renderClass.textureHandle = 0x2345;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    renderClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    renderClass.textureAddressV = (D3DTEXTUREADDRESS)(2);
    float attr1[1] = {0.0f};

    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        0,
        0,
        3,
        &renderClass,
        0x66,
        1.0f,
        1
    );

    int status = 0;
    const D3DTLVERTEX &opaqueFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &opaqueClose = g_zVideo_OverwriteQueueBase[0].vertices[3];
    if (g_zVideo_OverwriteQueueCount != 1 ||
        g_zVideo_OverwriteQueueBase[0].type != 5 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 4 ||
        g_zVideo_OverwriteQueueBase[0].renderClass != (int)(&renderClass) ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x66 ||
        g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        status = 1;
    } else if (opaqueFirst.sx != 7.0f || opaqueFirst.color != 0xffffffff ||
               opaqueFirst.specular != 0xff000000 || opaqueFirst.tu != 0.5f ||
               opaqueClose.sx != 4.0f || opaqueClose.tu != 0.3f) {
        status = 2;
    } else {
        renderClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
        attr1[0] = 0.5f;
        float attr2[2] = {0.0f, 1.0f};
        zVideo_dd3d::SubmitPolygon(
            vertices,
            uvPairs,
            attr1,
            0,
            attr2,
            2,
            &renderClass,
            0x77,
            0.5f,
            0
        );

        const D3DTLVERTEX &transparentFirst =
            g_zVideo_SortedPolyQueueBase[0].vertices[0];
        const D3DTLVERTEX &transparentLast =
            g_zVideo_SortedPolyQueueBase[0].vertices[1];
        if (g_zVideo_SortedPolyQueueCount != 1 ||
            g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
            g_zVideo_SortedPolyQueueBase[0].renderClass != (int)(&renderClass) ||
            g_zVideo_SortedPolyQueueBase[0].renderParam != 0x77) {
            status = 3;
        } else if (transparentFirst.sx != 4.0f ||
                   transparentFirst.color != 0x7f7f7f7f ||
                   transparentFirst.specular != 0x00000000 ||
                   transparentFirst.tu != 0.3f ||
                   transparentLast.sx != 1.0f ||
                   transparentLast.color != 0x7f7f7f7f ||
                   transparentLast.specular != 0xff000000) {
            status = 4;
        }
    }

    g_zVideo_D3DAppendFanCloseVertexPending = savedAppendFanClose;
    return status;
}

extern "C" int zvideo_submit_polygon_immediate_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedShadeMode = g_zVideo_D3DRenderStateCache.shadeMode;
    const D3DTEXTUREHANDLE savedTextureHandle =
        g_zVideo_D3DRenderStateCache.textureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderStateCache.textureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderStateCache.textureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderStateCache.textureAddressV;
    const int savedAppendFanClose = g_zVideo_D3DAppendFanCloseVertexPending;
    const int savedNormalizeChannel = g_zVideo_D3DColorNormalizeChannelIndex;
    const float savedBiasR = g_zVideo_D3DColorAttrBiasR;
    const float savedBiasG = g_zVideo_D3DColorAttrBiasG;
    const float savedBiasB = g_zVideo_D3DColorAttrBiasB;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DAppendFanCloseVertexPending = 1;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f}
    };
    zVideo_RenderClass renderClass = {};
    renderClass.textureHandle = 0x2345;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    renderClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    renderClass.textureAddressV = (D3DTEXTUREADDRESS)(2);
    float attr1[1] = {0.0f};

    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        0,
        0,
        3,
        &renderClass,
        0x66,
        1.0f,
        0
    );

    const int renderStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 5 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 2 &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 0x2345 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 2 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 1 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 2;
    const int drawOk =
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2LastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        g_smokeDirect3DDevice2LastVertexType == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2LastVertices == g_zVideo_D3DSubmitTempVertices &&
        g_smokeDirect3DDevice2LastVertexCount == 4 &&
        g_smokeDirect3DDevice2LastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0;
    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &close = g_zVideo_D3DSubmitTempVertices[3];
    const int verticesOk =
        first.sx == 7.0f && first.sy == 8.0f && first.sz == 9.0f &&
        first.rhw == 9.0f && first.color == 0xffffffff &&
        first.specular == 0xff000000 && first.tu == 0.5f &&
        first.tv == 0.6f && close.sx == 4.0f && close.sy == 5.0f &&
        close.sz == 6.0f && close.rhw == 6.0f &&
        close.color == 0xffffffff && close.specular == 0xff000000 &&
        close.tu == 0.3f && close.tv == 0.4f;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 2;
    g_zVideo_D3DRenderStateCache.textureHandle = 0x2345;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(1);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(2);
    g_zVideo_D3DAppendFanCloseVertexPending = 0;
    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        0,
        0,
        2,
        &renderClass,
        0x77,
        1.0f,
        0
    );
    const int cacheHitOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 0 &&
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2LastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].tu == 0.3f &&
        g_zVideo_D3DSubmitTempVertices[0].tv == 0.4f;

    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
    attr1[0] = 0.5f;
    float attr0[2] = {0.0f, 0.5f};
    float attr2[2] = {0.0f, 1.0f};
    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;
    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        attr0,
        attr2,
        2,
        &renderClass,
        0x88,
        0.5f,
        1
    );
    const D3DTLVERTEX &transparentFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &transparentClose = g_zVideo_OverwriteQueueBase[0].vertices[2];
    const int transparentOk =
        g_zVideo_OverwriteQueueCount == 1 &&
        g_zVideo_OverwriteQueueBase[0].type == 0 &&
        g_zVideo_OverwriteQueueBase[0].vertexCount == 3 &&
        g_zVideo_OverwriteQueueBase[0].renderClass == (int)(&renderClass) &&
        g_zVideo_OverwriteQueueBase[0].renderParam == 0x88 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0 &&
        transparentFirst.sx == 4.0f &&
        transparentFirst.color == 0x7fa3d1ff &&
        transparentFirst.specular == 0x00000000 &&
        transparentFirst.tu == 0.3f &&
        transparentClose.sx == 1.0f &&
        transparentClose.color == 0x7f7f7f7f &&
        transparentClose.specular == 0xff000000 &&
        transparentClose.tu == 0.1f;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderStateCache.shadeMode = savedShadeMode;
    g_zVideo_D3DRenderStateCache.textureHandle = savedTextureHandle;
    g_zVideo_D3DRenderStateCache.textureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderStateCache.textureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderStateCache.textureAddressV = savedTextureAddressV;
    g_zVideo_D3DAppendFanCloseVertexPending = savedAppendFanClose;
    g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
    g_zVideo_D3DColorAttrBiasR = savedBiasR;
    g_zVideo_D3DColorAttrBiasG = savedBiasG;
    g_zVideo_D3DColorAttrBiasB = savedBiasB;
    return renderStateOk && drawOk && verticesOk && cacheHitOk && transparentOk
               ? 0
               : 1;
}

extern "C" int zvideo_submit_polygon_lit_queue_smoke(void) {
    const int savedAppendFanClose = g_zVideo_D3DAppendFanCloseVertexPending;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f}
    };
    zVideo_RenderClass renderClass = {};
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    float attr1[3] = {0.0f, 0.5f, 1.0f};

    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        0,
        0,
        3,
        &renderClass,
        0x44,
        1.0f,
        1
    );

    int status = 0;
    if (g_zVideo_OverwriteQueueCount != 1 ||
        g_zVideo_OverwriteQueueBase[0].type != 6 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 4 ||
        g_zVideo_OverwriteQueueBase[0].renderClass != (int)(&renderClass) ||
        g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        status = 1;
    } else if (g_zVideo_OverwriteQueueBase[0].vertices[0].sx != 7.0f ||
               g_zVideo_OverwriteQueueBase[0].vertices[0].color != 0xff000000 ||
               g_zVideo_OverwriteQueueBase[0].vertices[1].color != 0xff7f7f7f ||
               g_zVideo_OverwriteQueueBase[0].vertices[2].color != 0xffffffff ||
               g_zVideo_OverwriteQueueBase[0].vertices[3].sx != 4.0f) {
        status = 2;
    } else {
        renderClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
        float attr2[2] = {0.0f, 1.0f};
        zVideo_dd3d::SubmitPolygonLit(
            vertices,
            uvPairs,
            attr1,
            0,
            attr2,
            2,
            &renderClass,
            0x55,
            0.5f,
            0
        );

        if (g_zVideo_SortedPolyQueueCount != 1 ||
            g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
            g_zVideo_SortedPolyQueueBase[0].renderClass != (int)(&renderClass) ||
            g_zVideo_SortedPolyQueueBase[0].renderParam != 0x55) {
            status = 3;
        } else if (g_zVideo_SortedPolyQueueBase[0].vertices[0].sx != 4.0f ||
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color != 0x7f7f7f7f ||
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].specular != 0x00000000 ||
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx != 1.0f ||
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].color != 0x7fffffff ||
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].specular != 0xff000000) {
            status = 4;
        }
    }

    g_zVideo_D3DAppendFanCloseVertexPending = savedAppendFanClose;
    return status;
}

extern "C" int zvideo_submit_polygon_lit_immediate_smoke(void) {
    SmokeDirect3DDevice2Object device = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedShadeMode = g_zVideo_D3DRenderStateCache.shadeMode;
    const D3DTEXTUREHANDLE savedTextureHandle =
        g_zVideo_D3DRenderStateCache.textureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderStateCache.textureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderStateCache.textureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderStateCache.textureAddressV;
    const int savedAppendFanClose = g_zVideo_D3DAppendFanCloseVertexPending;
    const int savedNormalizeChannel = g_zVideo_D3DColorNormalizeChannelIndex;
    const float savedBiasR = g_zVideo_D3DColorAttrBiasR;
    const float savedBiasG = g_zVideo_D3DColorAttrBiasG;
    const float savedBiasB = g_zVideo_D3DColorAttrBiasB;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 1;
    g_zVideo_D3DRenderStateCache.textureHandle = 0;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DAppendFanCloseVertexPending = 1;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f}
    };
    zVideo_RenderClass renderClass = {};
    renderClass.textureHandle = 0x3456;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    renderClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    renderClass.textureAddressV = (D3DTEXTUREADDRESS)(2);
    float attr1[3] = {0.0f, 0.5f, 1.0f};

    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        0,
        0,
        3,
        &renderClass,
        0x66,
        1.0f,
        0
    );

    const int renderStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 5 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 2 &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 0x3456 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 2 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 1 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 2;
    const int drawOk =
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2LastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        g_smokeDirect3DDevice2LastVertexType == (D3DVERTEXTYPE)(3) &&
        g_smokeDirect3DDevice2LastVertices == g_zVideo_D3DSubmitTempVertices &&
        g_smokeDirect3DDevice2LastVertexCount == 4 &&
        g_smokeDirect3DDevice2LastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0;
    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &close = g_zVideo_D3DSubmitTempVertices[3];
    const int verticesOk =
        first.sx == 7.0f && first.sy == 8.0f && first.sz == 9.0f &&
        first.rhw == 9.0f && first.color == 0xff000000 &&
        first.specular == 0xff000000 && first.tu == 0.5f &&
        first.tv == 0.6f && close.sx == 4.0f && close.sy == 5.0f &&
        close.sz == 6.0f && close.rhw == 6.0f &&
        close.color == 0xff7f7f7f && close.specular == 0xff000000 &&
        close.tu == 0.3f && close.tv == 0.4f;

    InstallSmokeDirect3DDevice2(device);
    g_zVideo_D3DRenderStateCache.shadeMode = 2;
    g_zVideo_D3DRenderStateCache.textureHandle = 0x3456;
    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
    g_zVideo_D3DRenderStateCache.textureAddressU = (D3DTEXTUREADDRESS)(1);
    g_zVideo_D3DRenderStateCache.textureAddressV = (D3DTEXTUREADDRESS)(2);
    g_zVideo_D3DAppendFanCloseVertexPending = 0;
    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        0,
        0,
        2,
        &renderClass,
        0x77,
        1.0f,
        0
    );
    const int cacheHitOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 0 &&
        g_smokeDirect3DDevice2DrawPrimitiveCalls == 1 &&
        g_smokeDirect3DDevice2LastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xff7f7f7f &&
        g_zVideo_D3DSubmitTempVertices[0].tu == 0.3f &&
        g_zVideo_D3DSubmitTempVertices[0].tv == 0.4f;

    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
    float attr0[2] = {0.0f, 0.5f};
    float attr2[2] = {0.0f, 1.0f};
    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;
    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        attr0,
        attr2,
        2,
        &renderClass,
        0x88,
        0.5f,
        1
    );
    const D3DTLVERTEX &transparentFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &transparentClose = g_zVideo_OverwriteQueueBase[0].vertices[2];
    const int transparentOk =
        g_zVideo_OverwriteQueueCount == 1 &&
        g_zVideo_OverwriteQueueBase[0].type == 0 &&
        g_zVideo_OverwriteQueueBase[0].vertexCount == 3 &&
        g_zVideo_OverwriteQueueBase[0].renderClass == (int)(&renderClass) &&
        g_zVideo_OverwriteQueueBase[0].renderParam == 0x88 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0 &&
        transparentFirst.sx == 4.0f &&
        transparentFirst.color == 0x7fa3d1ff &&
        transparentFirst.specular == 0x00000000 &&
        transparentFirst.tu == 0.3f &&
        transparentFirst.tv == 0.4f &&
        transparentClose.sx == 1.0f &&
        transparentClose.color == 0x7fffffff &&
        transparentClose.specular == 0xff000000 &&
        transparentClose.tu == 0.1f &&
        transparentClose.tv == 0.2f;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderStateCache.shadeMode = savedShadeMode;
    g_zVideo_D3DRenderStateCache.textureHandle = savedTextureHandle;
    g_zVideo_D3DRenderStateCache.textureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderStateCache.textureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderStateCache.textureAddressV = savedTextureAddressV;
    g_zVideo_D3DAppendFanCloseVertexPending = savedAppendFanClose;
    g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
    g_zVideo_D3DColorAttrBiasR = savedBiasR;
    g_zVideo_D3DColorAttrBiasG = savedBiasG;
    g_zVideo_D3DColorAttrBiasB = savedBiasB;
    return renderStateOk && drawOk && verticesOk && cacheHitOk && transparentOk
               ? 0
               : 1;
}

extern "C" int zvideo_dd3d_create_device_state_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawSurface *const savedZBufferAttach = g_zVideo_pZBufferAttachSurface;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const int savedClearScreen = g_zVideo_ClearScreenBufferEnabled;
    const int savedPendingWireframe = g_zVideo_PendingWireframeState;
    const int savedCachedFogEnable = g_zVideo_CachedFogEnableRenderState;
    const int savedCachedFogMode = g_zVideo_CachedFogModeLightState;
    const D3DMATERIALHANDLE savedMaterialHandle = g_zVideo_D3DMaterialHandle;
    const D3DDEVICEDESC savedHalDesc = g_zVideo_D3DHalDeviceDesc;
    const D3DDEVICEDESC savedHelDesc = g_zVideo_D3DHelDeviceDesc;
    zVideo_QuadBatchItemPartial savedQuadItems[16];
    std::memcpy(
        savedQuadItems,
        g_zVideo_QuadBatchItemsBase,
        sizeof(savedQuadItems)
    );

    SmokeDirectDraw2Object directDraw = {};
    SmokeDirectDrawSurfaceObject createdSurface = {};
    SmokeDirectDrawSurface3Object zBufferSurface3 = {};
    SmokeDirectDrawSurface3Object swSurface = {};
    SmokeDirect3D2Object d3d = {};
    SmokeDirect3DDevice2Object d3dDevice = {};
    SmokeDirect3DViewport2Object d3dViewport = {};
    SmokeDirect3DMaterial2Object d3dMaterial = {};

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(createdSurface);
    InstallSmokeDirectDrawSurface3(zBufferSurface3);
    InstallSmokeDirectDrawSurface3(swSurface);
    InstallSmokeDirect3DDevice2(d3dDevice);
    InstallSmokeDirect3DViewport2(d3dViewport);
    InstallSmokeDirect3DMaterial2(d3dMaterial);
    InstallSmokeDirect3D2(
        d3d,
        (IDirect3DDevice2 *)(&d3dDevice),
        (IDirect3DViewport2 *)(&d3dViewport),
        (IDirect3DMaterial2 *)(&d3dMaterial)
    );
    g_smokeDirectDraw2CreateSurfaceValue =
        (IDirectDrawSurface *)(&createdSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &zBufferSurface3;
    g_smokeDirectDraw2QueryInterfaceValue = &d3d;
    g_smokeDirect3DMaterial2HandleValue = 0x2468;

    GUID deviceGuid = {
        0x33445566,
        0x7788,
        0x99aa,
        {1, 3, 5, 7, 9, 11, 13, 15}
    };
    zVidD3DDriverRecordPartial selectedD3D = {};
    selectedD3D.pD3DDeviceGuid = &deviceGuid;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3D;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pZBufferSurface = 0;
    g_zVideo_pZBufferAttachSurface = 0;
    g_zVideo_pD3D2 = 0;
    g_zVideo_pD3DDevice = 0;
    g_zVideo_pD3DViewport2 = 0;
    g_zVideo_pD3DMaterial2 = 0;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;
    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVideo_PendingWireframeState = 17;
    g_zVideo_CachedFogEnableRenderState = 0;
    g_zVideo_CachedFogModeLightState = 0;
    g_zVideo_D3DMaterialHandle = 0;

    const int successResult = zVideo_dd3d::CreateDeviceState();
    const int renderStateOk =
        g_smokeDirect3DDevice2SetRenderStateCalls == 11 &&
        g_smokeDirect3DDevice2RenderStates[0] == D3DRENDERSTATE_CULLMODE &&
        g_smokeDirect3DDevice2RenderStateValues[0] == 1 &&
        g_smokeDirect3DDevice2RenderStates[1] == D3DRENDERSTATE_ZENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[1] == 1 &&
        g_smokeDirect3DDevice2RenderStates[2] == D3DRENDERSTATE_ZFUNC &&
        g_smokeDirect3DDevice2RenderStateValues[2] == 7 &&
        g_smokeDirect3DDevice2RenderStates[3] == D3DRENDERSTATE_SPECULARENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[3] == 0 &&
        g_smokeDirect3DDevice2RenderStates[4] == D3DRENDERSTATE_SHADEMODE &&
        g_smokeDirect3DDevice2RenderStateValues[4] == 1 &&
        g_smokeDirect3DDevice2RenderStates[5] == D3DRENDERSTATE_TEXTUREPERSPECTIVE &&
        g_smokeDirect3DDevice2RenderStateValues[5] == 1 &&
        g_smokeDirect3DDevice2RenderStates[6] == D3DRENDERSTATE_TEXTUREMAG &&
        g_smokeDirect3DDevice2RenderStateValues[6] == 2 &&
        g_smokeDirect3DDevice2RenderStates[7] == D3DRENDERSTATE_TEXTUREMIN &&
        g_smokeDirect3DDevice2RenderStateValues[7] == 2 &&
        g_smokeDirect3DDevice2RenderStates[8] == D3DRENDERSTATE_SRCBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[8] == 5 &&
        g_smokeDirect3DDevice2RenderStates[9] == D3DRENDERSTATE_DESTBLEND &&
        g_smokeDirect3DDevice2RenderStateValues[9] == 6 &&
        g_smokeDirect3DDevice2RenderStates[10] == D3DRENDERSTATE_FOGENABLE &&
        g_smokeDirect3DDevice2RenderStateValues[10] == 1;
    const int successOk =
        successResult == 0 &&
        g_zVideo_ClearScreenBufferEnabled == 1 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwSize == sizeof(DDSURFACEDESC) &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwFlags == 0x47 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwWidth == 320 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwHeight == 240 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps == 0x24000 &&
        g_smokeDirectDraw2CreateSurfaceDescs[0].dwMipMapCount == 0x10 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        g_smokeDirectDrawSurfaceLastQueryInterfaceOut ==
            (void **)(&g_zVideo_pZBufferAttachSurface) &&
        g_smokeDirectDrawSurface3AddAttachedSurfaceCalls == 1 &&
        g_smokeDirectDrawSurface3LastAddAttachedSurfaceSelf ==
            (IDirectDrawSurface3 *)(&swSurface) &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceArg ==
            (IDirectDrawSurface3 *)(g_zVideo_pZBufferAttachSurface) &&
        g_smokeDirectDraw2QueryInterfaceCalls == 1 &&
        g_smokeDirectDraw2LastQueryInterfaceOut == (void **)(&g_zVideo_pD3D2) &&
        g_smokeDirect3D2CreateDeviceCalls == 1 &&
        std::memcmp(
            g_smokeDirect3D2LastCreateDeviceGuid,
            &deviceGuid,
            sizeof(deviceGuid)
        ) == 0 &&
        g_smokeDirect3D2LastCreateDeviceSurface ==
            (IDirectDrawSurface *)(g_zVideo_SwSurfaceState.surf) &&
        g_smokeDirect3D2LastCreateDeviceOut == &g_zVideo_pD3DDevice &&
        g_smokeDirect3D2CreateViewportCalls == 1 &&
        g_smokeDirect3D2LastCreateViewportOut == &g_zVideo_pD3DViewport2 &&
        g_smokeDirect3DDevice2AddViewportCalls == 1 &&
        g_smokeDirect3DDevice2LastAddViewport == g_zVideo_pD3DViewport2 &&
        g_smokeDirect3DViewport2SetViewport2Calls == 1 &&
        g_smokeDirect3DViewport2LastViewportValue.dwWidth == 800 &&
        g_smokeDirect3DViewport2LastViewportValue.dwHeight == 600 &&
        g_smokeDirect3DDevice2SetCurrentViewportCalls == 1 &&
        g_smokeDirect3DDevice2LastSetCurrentViewport == g_zVideo_pD3DViewport2 &&
        g_smokeDirect3D2CreateMaterialCalls == 1 &&
        g_smokeDirect3DMaterial2SetMaterialCalls == 1 &&
        g_smokeDirect3DMaterial2LastMaterialValue.dwRampSize == 0x100 &&
        g_smokeDirect3DMaterial2GetHandleCalls == 1 &&
        g_smokeDirect3DMaterial2LastGetHandleDevice == g_zVideo_pD3DDevice &&
        g_zVideo_D3DMaterialHandle == 0x2468 &&
        g_smokeDirect3DViewport2SetBackgroundCalls == 1 &&
        g_smokeDirect3DViewport2LastBackground == 0x2468 &&
        g_smokeDirect3DDevice2GetCapsCalls == 1 &&
        g_smokeDirect3DDevice2LastGetCapsHalDesc == &g_zVideo_D3DHalDeviceDesc &&
        g_smokeDirect3DDevice2LastGetCapsHelDesc == &g_zVideo_D3DHelDeviceDesc &&
        renderStateOk &&
        g_smokeDirect3DDevice2SetLightStateCalls == 1 &&
        g_smokeDirect3DDevice2LightStates[0] == D3DLIGHTSTATE_FOGMODE &&
        g_smokeDirect3DDevice2LightStateValues[0] == D3DFOG_LINEAR &&
        g_zVideo_PendingWireframeState == -1 &&
        g_zVideo_CachedFogEnableRenderState == 1 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR &&
        g_zVideo_QuadBatchItemsBase[0].vertices[0].sz == 0.99000001f &&
        g_zVideo_QuadBatchItemsBase[15].vertices[3].rhw == 0.99000001f;

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(createdSurface);
    InstallSmokeDirectDrawSurface3(zBufferSurface3);
    InstallSmokeDirectDrawSurface3(swSurface);
    InstallSmokeDirect3DDevice2(d3dDevice);
    InstallSmokeDirect3DViewport2(d3dViewport);
    InstallSmokeDirect3DMaterial2(d3dMaterial);
    InstallSmokeDirect3D2(
        d3d,
        (IDirect3DDevice2 *)(&d3dDevice),
        (IDirect3DViewport2 *)(&d3dViewport),
        (IDirect3DMaterial2 *)(&d3dMaterial)
    );
    g_smokeDirectDraw2CreateSurfaceResult = DDERR_GENERIC;
    g_smokeDirectDraw2CreateSurfaceValue =
        (IDirectDrawSurface *)(&createdSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &zBufferSurface3;
    g_smokeDirectDraw2QueryInterfaceValue = &d3d;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3D;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);

    const int createSurfaceFailureResult = zVideo_dd3d::CreateDeviceState();
    const int createSurfaceFailureOk =
        createSurfaceFailureResult != 0 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
        g_smokeDirectDraw2QueryInterfaceCalls == 0 &&
        g_smokeDirect3D2CreateDeviceCalls == 0;

    InstallSmokeDirectDraw2(directDraw);
    InstallSmokeDirectDrawSurface(createdSurface);
    InstallSmokeDirectDrawSurface3(zBufferSurface3);
    InstallSmokeDirectDrawSurface3(swSurface);
    InstallSmokeDirect3DDevice2(d3dDevice);
    InstallSmokeDirect3DViewport2(d3dViewport);
    InstallSmokeDirect3DMaterial2(d3dMaterial);
    InstallSmokeDirect3D2(
        d3d,
        (IDirect3DDevice2 *)(&d3dDevice),
        (IDirect3DViewport2 *)(&d3dViewport),
        (IDirect3DMaterial2 *)(&d3dMaterial)
    );
    g_smokeDirectDraw2CreateSurfaceValue =
        (IDirectDrawSurface *)(&createdSurface);
    g_smokeDirectDrawSurfaceQueryInterfaceValue = &zBufferSurface3;
    g_smokeDirectDrawSurface3AddAttachedSurfaceResult = DDERR_GENERIC;
    g_smokeDirectDraw2QueryInterfaceValue = &d3d;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3D;
    g_zVideo_pZBufferSurface = 0;
    g_zVideo_pZBufferAttachSurface = 0;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;

    const int addAttachedFailureResult = zVideo_dd3d::CreateDeviceState();
    const int addAttachedFailureOk =
        addAttachedFailureResult != 0 &&
        g_smokeDirectDraw2CreateSurfaceCalls == 1 &&
        g_smokeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        g_smokeDirectDrawSurface3AddAttachedSurfaceCalls == 1 &&
        g_smokeDirectDrawSurface3LastAddAttachedSurfaceSelf ==
            (IDirectDrawSurface3 *)(&swSurface) &&
        g_smokeDirectDrawSurface3LastAttachedSurfaceArg ==
            (IDirectDrawSurface3 *)(g_zVideo_pZBufferAttachSurface) &&
        g_smokeDirectDraw2QueryInterfaceCalls == 0 &&
        g_smokeDirect3D2CreateDeviceCalls == 0;

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pZBufferAttachSurface = savedZBufferAttach;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_ClearScreenBufferEnabled = savedClearScreen;
    g_zVideo_PendingWireframeState = savedPendingWireframe;
    g_zVideo_CachedFogEnableRenderState = savedCachedFogEnable;
    g_zVideo_CachedFogModeLightState = savedCachedFogMode;
    g_zVideo_D3DMaterialHandle = savedMaterialHandle;
    g_zVideo_D3DHalDeviceDesc = savedHalDesc;
    g_zVideo_D3DHelDeviceDesc = savedHelDesc;
    std::memcpy(
        g_zVideo_QuadBatchItemsBase,
        savedQuadItems,
        sizeof(savedQuadItems)
    );

    if (!successOk) {
        return 1;
    }
    if (!createSurfaceFailureOk) {
        return 2;
    }
    if (!addAttachedFailureOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvid_image_resample_square_smoke(void) {
    unsigned short *pixels =
        (unsigned short *)(std::malloc(8 * sizeof(unsigned short)));
    char *alphaMap = (char *)(std::malloc(8));
    if (pixels == 0 || alphaMap == 0) {
        std::free(pixels);
        std::free(alphaMap);
        return 1;
    }

    for (int i = 0; i < 8; ++i) {
        pixels[i] = (unsigned short)(0x10 + i);
        alphaMap[i] = (char)(0x40 + i);
    }

    zVidImagePartial image = {};
    image.width = 4;
    image.height = 2;
    image.pixels = pixels;
    image.alphaMap = alphaMap;

    zVid_Image::ResampleSquare(&image, 2);

    unsigned short *const newPixels = (unsigned short *)(image.pixels);
    char *const newAlphaMap = image.alphaMap;
    const int ok =
        image.width == 2 &&
        image.height == 2 &&
        newPixels != 0 &&
        newAlphaMap != 0 &&
        newPixels[0] == 0x10 &&
        newPixels[1] == 0x12 &&
        newPixels[2] == 0x14 &&
        newPixels[3] == 0x16 &&
        newAlphaMap[0] == (char)(0x40) &&
        newAlphaMap[1] == (char)(0x42) &&
        newAlphaMap[2] == (char)(0x44) &&
        newAlphaMap[3] == (char)(0x46);

    std::free(image.pixels);
    std::free(image.alphaMap);
    return ok != 0 ? 0 : 2;
}

extern "C" int zvideo_pixel_pack_setup_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );

    if (g_zVideo_PixelPack.rBits != 5 ||
        g_zVideo_PixelPack.gBits != 6 ||
        g_zVideo_PixelPack.bBits != 5 ||
        g_zVideo_PixelPack.packedBase != 8 ||
        g_zVideo_PixelPack.sumMinus8 != 3 ||
        g_zVideo_PixelPack.bShiftTo8 != 3 ||
        g_zVideo_PixelPack.rMaskShifted != 0xf8 ||
        g_zVideo_PixelPack.gMaskShifted != 0xfc ||
        g_zVideo_PixelPack.bMaskShifted != 0xf8) {
        return 1;
    }

    zVideo::TexturePixelPack_SetupFromMasks(
        4,
        4,
        4,
        4,
        0xf000,
        0x0f00,
        0x00f0,
        0x000f
    );
    return g_zVideo_TexturePixelPack_RBits == 4 &&
                   g_zVideo_TexturePixelPack_GBits == 4 &&
                   g_zVideo_TexturePixelPack_BBits == 4 &&
                   g_zVideo_TexturePixelPack_ABits == 4 &&
                   g_zVideo_TexturePixelPack_RGBBitsTotal == 12 &&
                   g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 == 4 &&
                   g_zVideo_TexturePixelPack_GBBitsTotalMinus8 == 0 &&
                   g_zVideo_TexturePixelPack_BShiftTo8 == 4 &&
                   g_zVideo_TexturePixelPack_RMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_GMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_BMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_NonRgbMaskShifted == ~0xf0
               ? 0
               : 2;
}

extern "C" int zvideo_pixel_pack_getters_smoke(void) {
    const zVideo_PixelPackParams savedPixelPack = g_zVideo_PixelPack;

    zVideo::PixelPack_SetupFromMasks(
        5,
        5,
        5,
        0x7c00,
        0x03e0,
        0x001f
    );

    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    unsigned int rMask = 0;
    unsigned int gMask = 0;
    unsigned int bMask = 0;
    int packedBase = 0;
    int sumMinus8 = 0;
    int bShiftTo8 = 0;

    zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);
    zVideo::PixelPack_GetRgbMasks(&rMask, &gMask, &bMask);
    zVideo::PixelPack_GetPackingParams(&packedBase, &sumMinus8, &bShiftTo8);

    const int ok =
        rBits == 5 &&
        gBits == 5 &&
        bBits == 5 &&
        rMask == 0x7c00 &&
        gMask == 0x03e0 &&
        bMask == 0x001f &&
        packedBase == 7 &&
        sumMinus8 == 2 &&
        bShiftTo8 == 3;

    g_zVideo_PixelPack = savedPixelPack;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvideo_dd_lock_surface_wait_restore_smoke(void) {
    SmokeDirectDrawSurface3Object surface = {};
    IDirectDrawSurface3 *const surfaceInterface = (IDirectDrawSurface3 *)(&surface);
    DDSURFACEDESC surfaceDesc;

    InstallSmokeDirectDrawSurface3(surface);
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    const int successOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3LastLockRect == 0 &&
        g_smokeDirectDrawSurface3LastLockDesc == &surfaceDesc &&
        g_smokeDirectDrawSurface3LastLockFlags == DDLOCK_WAIT &&
        g_smokeDirectDrawSurface3LastLockEvent == 0 &&
        g_smokeDirectDrawSurface3LockDescSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwWidth == 640 &&
        surfaceDesc.dwHeight == 480 &&
        surfaceDesc.lPitch == g_smokeDirectDrawSurface3LockPitchValue &&
        surfaceDesc.lpSurface == g_smokeDirectDrawSurface3LockPixelsValue;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3LockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3LockResultCount = 2;
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    const int retryOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 2 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3LockDescSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwWidth == 640 &&
        surfaceDesc.dwHeight == 480 &&
        surfaceDesc.lPitch == g_smokeDirectDrawSurface3LockPitchValue &&
        surfaceDesc.lpSurface == g_smokeDirectDrawSurface3LockPixelsValue;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3LockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3LockResultCount = 2;
    g_smokeDirectDrawSurface3RestoreResult = DDERR_GENERIC;
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    const int restoreFailureOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) ==
            0x5a56ffff &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3LockResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3LockResultCount = 1;
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    const int lockFailureOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) ==
            0x5a56ffff &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        surfaceDesc.dwSize == sizeof(surfaceDesc);

    return successOk && retryOk && restoreFailureOk && lockFailureOk ? 0 : 1;
}

extern "C" int zvideo_dd_unlock_surface_wait_restore_smoke(void) {
    SmokeDirectDrawSurface3Object surface = {};
    IDirectDrawSurface3 *const surfaceInterface = (IDirectDrawSurface3 *)(&surface);

    InstallSmokeDirectDrawSurface3(surface);
    const int successOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3UnlockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3UnlockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3UnlockResultCount = 2;
    const int retryOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0 &&
        g_smokeDirectDrawSurface3UnlockCalls == 2 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3UnlockResults[0] = DDERR_SURFACELOST;
    g_smokeDirectDrawSurface3UnlockResults[1] = DD_OK;
    g_smokeDirectDrawSurface3UnlockResultCount = 2;
    g_smokeDirectDrawSurface3RestoreResult = DDERR_GENERIC;
    const int restoreFailureOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0x5a56ffff &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 1 &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    InstallSmokeDirectDrawSurface3(surface);
    g_smokeDirectDrawSurface3UnlockResults[0] = DDERR_GENERIC;
    g_smokeDirectDrawSurface3UnlockResultCount = 1;
    const int unlockFailureOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0x5a56ffff &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3RestoreCalls == 0 &&
        g_smokeDirectDrawSurface3LastUnlockArg == 0;

    return successOk && retryOk && restoreFailureOk && unlockFailureOk ? 0 : 1;
}

extern "C" int zvideo_convert_image_pixels_for_texture_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );

    unsigned short srcPixels[4] = {0xf800, 0x07e0, 0x001f, 0};
    unsigned short dstPixels[8] = {};
    zVidImagePartial image = {};
    image.width = 2;
    image.height = 2;
    image.pixels = srcPixels;

    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 0);

    if (dstPixels[0] != 0xfc00 ||
        dstPixels[1] != 0x83f0 ||
        dstPixels[4] != 0x801f ||
        dstPixels[5] != 0) {
        return 1;
    }

    unsigned char alphaMap[4] = {0xf0, 0x80, 0x10, 0};
    std::memset(dstPixels, 0, sizeof(dstPixels));
    image.alphaMap = (char *)(alphaMap);
    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 1);

    if (dstPixels[0] != 0xff00 ||
        dstPixels[1] != 0x80f0 ||
        dstPixels[4] != 0x100f ||
        dstPixels[5] != 0) {
        return 2;
    }

    zVideo::PixelPack_SetupFromMasks(
        5,
        5,
        5,
        0x7c00,
        0x03e0,
        0x001f
    );

    unsigned short src555 = 0x4210;
    unsigned char alpha555 = 0xa0;
    std::memset(dstPixels, 0, sizeof(dstPixels));
    image.width = 1;
    image.height = 1;
    image.pixels = &src555;
    image.alphaMap = (char *)(&alpha555);
    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 0);

    return dstPixels[0] == 0xa888 ? 0 : 3;
}

extern "C" int zvideo_dd3d_upload_image_to_surface_smoke(void) {
    const int savedDisplayModeBpp = g_zVideo_DisplayModeBpp;
    const int savedPackedBase = g_zVideo_PixelPack.packedBase;
    const int savedSumMinus8 = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedBMaskShifted = g_zVideo_PixelPack.bMaskShifted;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    SmokeDirectDrawSurface3Object uploadSurface = {};
    IDirectDrawSurface *const uploadSurfaceInterface =
        (IDirectDrawSurface *)(&uploadSurface);
    zVidImagePartial image = {};
    unsigned char srcContiguous[16];
    unsigned char dstContiguous[24];
    for (int i = 0; i < 16; ++i) {
        srcContiguous[i] = (unsigned char)(0x20 + i);
    }
    std::memset(dstContiguous, 0xcc, sizeof(dstContiguous));

    InstallSmokeDirectDrawSurface3(uploadSurface);
    g_zVideo_DisplayModeBpp = 16;
    g_smokeDirectDrawSurface3LockPixelsValue = dstContiguous;
    g_smokeDirectDrawSurface3LockPitchValue = 4;
    image.width = 4;
    image.height = 2;
    image.pixels = srcContiguous;
    image.alphaMap = 0;
    const int contiguousOk =
        zVideo_dd3d::UploadImageToSurface(uploadSurfaceInterface, &image, 0) == 1 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockSurfaces[0] ==
            (IDirectDrawSurface3 *)(&uploadSurface) &&
        std::memcmp(dstContiguous, srcContiguous, sizeof(srcContiguous)) == 0 &&
        dstContiguous[16] == 0xcc;

    unsigned char srcRows[8];
    unsigned char dstRows[20];
    for (int rowByte = 0; rowByte < 8; ++rowByte) {
        srcRows[rowByte] = (unsigned char)(0x40 + rowByte);
    }
    std::memset(dstRows, 0xcc, sizeof(dstRows));

    InstallSmokeDirectDrawSurface3(uploadSurface);
    g_zVideo_DisplayModeBpp = 16;
    g_smokeDirectDrawSurface3LockPixelsValue = dstRows;
    g_smokeDirectDrawSurface3LockPitchValue = 8;
    image.width = 2;
    image.height = 2;
    image.pixels = srcRows;
    image.alphaMap = 0;
    const int rowCopyOk =
        zVideo_dd3d::UploadImageToSurface(uploadSurfaceInterface, &image, 0) == 1 &&
        std::memcmp(dstRows, srcRows, 4) == 0 &&
        dstRows[4] == 0xcc &&
        std::memcmp(dstRows + 8, srcRows + 4, 4) == 0 &&
        dstRows[12] == 0xcc &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1;

    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );

    unsigned short srcAlpha[4] = {0xf800, 0x07e0, 0x001f, 0};
    unsigned char alphaMap[4] = {0xf0, 0x80, 0x10, 0};
    unsigned short dstAlpha[8] = {};

    InstallSmokeDirectDrawSurface3(uploadSurface);
    g_smokeDirectDrawSurface3LockPixelsValue = dstAlpha;
    g_smokeDirectDrawSurface3LockPitchValue = 8;
    image.width = 2;
    image.height = 2;
    image.pixels = srcAlpha;
    image.alphaMap = (char *)(alphaMap);
    const int alphaConvertOk =
        zVideo_dd3d::UploadImageToSurface(uploadSurfaceInterface, &image, 1) == 1 &&
        dstAlpha[0] == 0xff00 &&
        dstAlpha[1] == 0x80f0 &&
        dstAlpha[4] == 0x100f &&
        dstAlpha[5] == 0 &&
        g_smokeDirectDrawSurface3LockCalls == 1 &&
        g_smokeDirectDrawSurface3UnlockCalls == 1;

    g_zVideo_DisplayModeBpp = savedDisplayModeBpp;
    g_zVideo_PixelPack.packedBase = savedPackedBase;
    g_zVideo_PixelPack.sumMinus8 = savedSumMinus8;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;
    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.bMaskShifted = savedBMaskShifted;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return contiguousOk && rowCopyOk && alphaConvertOk ? 0 : 1;
}

extern "C" int zhud_counter_constructor_smoke(void) {
    HudUiCounter counter{};
    zVidImagePartial state0{};
    zVidImagePartial state1{};
    zVidImagePartial state2{};

    counter.stateImages[0] = &state0;
    counter.stateImages[1] = &state1;
    counter.stateImages[2] = &state2;
    counter.imageStateWord = 0xabcd1234;
    counter.layoutX = 77;
    counter.layoutY = 88;

    HudUiCounter *const result = new (&counter) HudUiCounter;

    bool dirtyFramesCleared = true;
    for (const HudUiRectDirty &dirtyRect : counter.dirtyRects) {
        dirtyFramesCleared = dirtyFramesCleared && dirtyRect.framesRemaining == 0;
    }

    const bool counterFieldsCleared =
        counter.stateImages[0] == nullptr &&
        counter.stateImages[1] == nullptr &&
        counter.stateImages[2] == nullptr;
    const bool inheritedWidgetDefaults =
        counter.image == nullptr &&
        counter.ownsImage == 0 &&
        counter.bltClipRectOrNull == nullptr &&
        counter.dirtyRectCount == 0 &&
        counter.alignFlags == 0 &&
        counter.state == 0 &&
        (counter.imageStateWord & 0xffffu) == 0 &&
        dirtyFramesCleared;

    return result == &counter && counterFieldsCleared && inheritedWidgetDefaults ? 0 : 1;
}

extern "C" int zhud_element_position_mutators_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x40;

    HudUiElement element{};
    element.Constructor(1, 2);
    element.flags = 0;

    element.SetPos(3, 4);
    const bool pos = element.x == 3 && element.y == 4 && element.flags == 0x40;

    element.flags = 0;
    element.SetX(5);
    const bool xOnly = element.x == 5 && element.y == 4 && element.flags == 0x40;

    element.flags = 0;
    element.SetY(6);
    const bool yOnly = element.x == 5 && element.y == 6 && element.flags == 0x40;

    g_HudUi_InvalidateMask = oldMask;
    return pos && xOnly && yOnly ? 0 : 1;
}

extern "C" int zhud_element_get_xy_smoke(void) {
    HudUiElement element{};
    element.Constructor(-12, 345);

    const bool initial = element.GetCenterX() == -12 && element.GetCenterY() == 345;
    element.SetPos(78, -90);

    HudUiRect textRect{};
    element.GetTextRect(&textRect);
    const bool moved = element.GetCenterX() == 78 && element.GetCenterY() == -90;
    const bool rectMatches =
        textRect.left == 78 &&
        textRect.right == 78 &&
        textRect.top == -90 &&
        textRect.bottom == -90;

    return initial && moved && rectMatches ? 0 : 1;
}

extern "C" int zhud_element_get_rect_smoke(void) {
    HudUiElement element{};
    element.Constructor(11, -22);

    HudUiRect rect{};
    element.GetTextRect(&rect);
    const bool initial =
        rect.left == 11 &&
        rect.right == 11 &&
        rect.top == -22 &&
        rect.bottom == -22;

    element.SetPos(-33, 44);
    element.GetTextRect(&rect);
    const bool moved =
        rect.left == -33 &&
        rect.right == -33 &&
        rect.top == 44 &&
        rect.bottom == 44;

    return initial && moved ? 0 : 1;
}

extern "C" int zhud_element_hit_test_true_smoke(void) {
    HudUiElement element{};

    return element.HitTestTrue(-100, 200) == 1 &&
                   element.HitTestTrue(999, -999) == 1
               ? 0
               : 1;
}

extern "C" int zhud_bar_and_meter_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x40;

    void *const barStorage = ::operator new(sizeof(HudUiBar));
    std::memset(barStorage, 0xff, sizeof(HudUiBar));
    HudUiBar *const bar = new (barStorage) HudUiBar;

    bool barPointsCleared = true;
    for (const HudUiBarPoint &point : bar->points) {
        barPointsCleared = barPointsCleared &&
                           point.x == 0.0f &&
                           point.y == 0.0f &&
                           point.reserved == 0;
    }

    const bool barConstructed =
        bar->drawVertexCount == 0 &&
        (bar->flags & 0x40u) != 0 &&
        barPointsCleared;
    bar->~HudUiBar();
    ::operator delete(barStorage);

    void *const meterStorage = ::operator new(sizeof(HudUiMeter));
    std::memset(meterStorage, 0xff, sizeof(HudUiMeter));
    HudUiMeter *const meter = new (meterStorage) HudUiMeter;
    const bool meterConstructed =
        meter->fillPixelsMax == 0 &&
        meter->meterFlags == 0 &&
        meter->drawVertexCount == 0 &&
        (meter->flags & 0x40u) != 0;
    meter->~HudUiMeter();
    ::operator delete(meterStorage);

    g_HudUi_InvalidateMask = oldMask;
    return barConstructed && meterConstructed ? 0 : 1;
}

extern "C" int hud_ui_set_invalidate_mode_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    g_HudUi_InvalidateMask = 0x40;
    HudUi::SetInvalidateMode(0);
    const bool disabledMode = g_HudUi_InvalidateMask == 0x04u;

    g_HudUi_InvalidateMask = 0x40;
    HudUi::SetInvalidateMode(1);
    const bool enabledMode = g_HudUi_InvalidateMask == 0x0cu;

    g_HudUi_InvalidateMask = oldMask;
    return disabledMode && enabledMode ? 0 : 1;
}

int main(int argc, char **argv) {
    const SmokeTest tests[] = {
        {"recoil_native_build_anchor", recoil_native_build_anchor},
        {"recoil_mfc42_provider_smoke", recoil_mfc42_provider_smoke},
        {"mfc_cstring_default_ctor_provider_smoke",
         mfc_cstring_default_ctor_provider_smoke},
        {"mfc_three_float_dialog_handlers_smoke", mfc_three_float_dialog_handlers_smoke},
        {"zstr_contains_case_insensitive_smoke", zstr_contains_case_insensitive_smoke},
        {"recoil_version_get_string_smoke", recoil_version_get_string_smoke},
        {"ai_property_dialog_on_destroy_smoke", ai_property_dialog_on_destroy_smoke},
        {"ai_property_dialog_on_sel_change_smoke", ai_property_dialog_on_sel_change_smoke},
        {"ai_property_dialog_update_property_labels_smoke",
         ai_property_dialog_update_property_labels_smoke},
        {"directinput_create_import_provider_smoke", directinput_create_import_provider_smoke},
        {"zinput_init_fastpath_smoke", zinput_init_fastpath_smoke},
        {"zinput_mouse_update_acquire_state_smoke", zinput_mouse_update_acquire_state_smoke},
        {"zinput_joystick_option_accessors_smoke", zinput_joystick_option_accessors_smoke},
        {"zinput_shutdown_smoke", zinput_shutdown_smoke},
        {"zinput_force_feedback_effect_wrappers_smoke",
         zinput_force_feedback_effect_wrappers_smoke},
        {"zinput_force_feedback_create_effect_smoke", zinput_force_feedback_create_effect_smoke},
        {"zinput_force_feedback_effect_set_smoke", zinput_force_feedback_effect_set_smoke},
        {"zinput_force_feedback_directional_runtime_smoke",
         zinput_force_feedback_directional_runtime_smoke},
        {"zinput_bindgroup_accessors_smoke", zinput_bindgroup_accessors_smoke},
        {"zinput_bindgroup_static_lifetime_smoke", zinput_bindgroup_static_lifetime_smoke},
        {"zinput_global_state_static_lifetime_smoke",
         zinput_global_state_static_lifetime_smoke},
        {"zinput_keyboard_dik_ascii_smoke", zinput_keyboard_dik_ascii_smoke},
        {"zinput_mouse_client_size_center_smoke", zinput_mouse_client_size_center_smoke},
        {"zinput_mouse_apply_and_recenter_cursor_smoke",
         zinput_mouse_apply_and_recenter_cursor_smoke},
        {"zinput_mouse_coop_level_flags_smoke", zinput_mouse_coop_level_flags_smoke},
        {"zinput_mouse_button_transition_state_smoke",
         zinput_mouse_button_transition_state_smoke},
        {"zinput_mouse_apply_accumulated_delta_smoke",
         zinput_mouse_apply_accumulated_delta_smoke},
        {"zinput_mouse_keyboard_small_accessors_smoke",
         zinput_mouse_keyboard_small_accessors_smoke},
        {"zinput_bindmap_name_tables_smoke", zinput_bindmap_name_tables_smoke},
        {"zinput_bindmap_context_smoke", zinput_bindmap_context_smoke},
        {"zinput_bindmap_current_reset_all_bindings_smoke",
         zinput_bindmap_current_reset_all_bindings_smoke},
        {"zinput_bindmap_dispatch_mouse_callbacks_smoke",
         zinput_bindmap_dispatch_mouse_callbacks_smoke},
        {"zinput_keyboard_clear_callbacks_smoke", zinput_keyboard_clear_callbacks_smoke},
        {"zinput_keyboard_mouse_addref_smoke", zinput_keyboard_mouse_addref_smoke},
        {"zinput_keyboard_init_device_smoke", zinput_keyboard_init_device_smoke},
        {"zinput_mouse_init_device_smoke", zinput_mouse_init_device_smoke},
        {"zinput_joystick_init_device_smoke", zinput_joystick_init_device_smoke},
        {"zinput_joystick_acquire_device_smoke", zinput_joystick_acquire_device_smoke},
        {"zinput_joystick_axis_property_smoke", zinput_joystick_axis_property_smoke},
        {"zinput_joystick_ref_and_enable_smoke", zinput_joystick_ref_and_enable_smoke},
        {"zinput_joystick_poll_and_wait_smoke", zinput_joystick_poll_and_wait_smoke},
        {"zinput_mouse_shutdown_device_smoke", zinput_mouse_shutdown_device_smoke},
        {"zinput_joystick_shutdown_device_smoke", zinput_joystick_shutdown_device_smoke},
        {"zinput_keyboard_shutdown_device_smoke", zinput_keyboard_shutdown_device_smoke},
        {"zinput_mouse_poll_state_smoke", zinput_mouse_poll_state_smoke},
        {"zinput_suspend_flags_smoke", zinput_suspend_flags_smoke},
        {"zinput_on_app_deactivate_smoke", zinput_on_app_deactivate_smoke},
        {"zinput_joystick_reset_and_resume_smoke", zinput_joystick_reset_and_resume_smoke},
        {"zinput_mouse_reset_and_resume_smoke", zinput_mouse_reset_and_resume_smoke},
        {"zinput_directinput_report_error_smoke", zinput_directinput_report_error_smoke},
        {"zinput_keyboard_reset_and_resume_smoke", zinput_keyboard_reset_and_resume_smoke},
        {"zinput_keyboard_reset_inputlost_smoke", zinput_keyboard_reset_inputlost_smoke},
        {"zinput_reset_all_transition_state_smoke", zinput_reset_all_transition_state_smoke},
        {"zinput_on_app_activate_smoke", zinput_on_app_activate_smoke},
        {"zinput_keyboard_raw_callback_smoke", zinput_keyboard_raw_callback_smoke},
        {"zinput_keyboard_wait_for_key_press_smoke", zinput_keyboard_wait_for_key_press_smoke},
        {"zinput_keyboard_poll_state_smoke", zinput_keyboard_poll_state_smoke},
        {"zinput_poll_active_devices_smoke", zinput_poll_active_devices_smoke},
        {"zopt_fullscreen_accessors_smoke", zopt_fullscreen_accessors_smoke},
        {"zopt_section_accessor_smoke", zopt_section_accessor_smoke},
        {"zopt_view_rect_target_side_effects_smoke",
         zopt_view_rect_target_side_effects_smoke},
        {"zsnd_option_accessors_smoke", zsnd_option_accessors_smoke},
        {"zsnd_stream_request_stop_if_active_smoke", zsnd_stream_request_stop_if_active_smoke},
        {"zsnd_stream_mgr_ensure_init_smoke", zsnd_stream_mgr_ensure_init_smoke},
        {"zsnd_stream_mgr_recycle_finished_request_smoke",
         zsnd_stream_mgr_recycle_finished_request_smoke},
        {"zsnd_stream_mgr_shutdown_lists_smoke", zsnd_stream_mgr_shutdown_lists_smoke},
        {"zsnd_backend_shutdown_release_smoke", zsnd_backend_shutdown_release_smoke},
        {"zsnd_play_handle_stop_if_active_smoke", zsnd_play_handle_stop_if_active_smoke},
        {"zsnd_play_handle_try_disable_managed_smoke",
         zsnd_play_handle_try_disable_managed_smoke},
        {"zsnd_play_handle_update3d_a3d_smoke", zsnd_play_handle_update3d_a3d_smoke},
        {"zsnd_play_handle_update3d_directsound_smoke",
         zsnd_play_handle_update3d_directsound_smoke},
        {"zsnd_update_listener_state_smoke", zsnd_update_listener_state_smoke},
        {"zsnd_speed_of_sound_smoke", zsnd_speed_of_sound_smoke},
        {"zsnd_sample_play_simple_smoke", zsnd_sample_play_simple_smoke},
        {"zsnd_is_muted_smoke", zsnd_is_muted_smoke},
        {"zsnd_gain_scale_to_directsound_attenuation_smoke",
         zsnd_gain_scale_to_directsound_attenuation_smoke},
        {"zsnd_global_volume_and_flag_helpers_smoke",
         zsnd_global_volume_and_flag_helpers_smoke},
        {"zsnd_preinitialize_runtime_state_smoke", zsnd_preinitialize_runtime_state_smoke},
        {"zsnd_sample_play_a3d_simple_direct_smoke",
         zsnd_sample_play_a3d_simple_direct_smoke},
        {"zsnd_sample_play_a3d_worldpos_smoke", zsnd_sample_play_a3d_worldpos_smoke},
        {"zsnd_sample_acquire_play_handle_smoke", zsnd_sample_acquire_play_handle_smoke},
        {"zsnd_stream_request_state_update_smoke",
         zsnd_stream_request_state_update_smoke},
        {"zsnd_fade_entry_backend_and_dispatch_smoke",
         zsnd_fade_entry_backend_and_dispatch_smoke},
        {"zsnd_fade_list_cursor_helpers_smoke", zsnd_fade_list_cursor_helpers_smoke},
        {"zsnd_fade_active_list_tick_compacts_smoke",
         zsnd_fade_active_list_tick_compacts_smoke},
        {"zsnd_fade_lists_stop_all_shutdown_smoke",
         zsnd_fade_lists_stop_all_shutdown_smoke},
        {"zsnd_tick_backend_markers_smoke", zsnd_tick_backend_markers_smoke},
        {"zsnd_report_error_helpers_smoke", zsnd_report_error_helpers_smoke},
        {"zsnd_stream_request_queue_smoke", zsnd_stream_request_queue_smoke},
        {"zsnd_set_use_archive_banks_flag_smoke",
         zsnd_set_use_archive_banks_flag_smoke},
        {"zsnd_sample_set_registry_init_shutdown_smoke",
         zsnd_sample_set_registry_init_shutdown_smoke},
        {"zsnd_sample_set_registry_lookup_destroy_smoke",
         zsnd_sample_set_registry_lookup_destroy_smoke},
        {"zsnd_sample_destroy_owned_data_smoke", zsnd_sample_destroy_owned_data_smoke},
        {"zsnd_sample_set_destroy_owned_data_smoke",
         zsnd_sample_set_destroy_owned_data_smoke},
        {"zsnd_sample_set_get_sample_at_smoke", zsnd_sample_set_get_sample_at_smoke},
        {"zsnd_find_sample_by_name_smoke", zsnd_find_sample_by_name_smoke},
        {"zsnd_sample_set_init_by_name_empty_smoke",
         zsnd_sample_set_init_by_name_empty_smoke},
        {"zsnd_sample_set_init_loose_file_smoke",
         zsnd_sample_set_init_loose_file_smoke},
        {"zsnd_sample_set_load_samples_from_index_archive_smoke",
         zsnd_sample_set_load_samples_from_index_archive_smoke},
        {"zsnd_create_queued_streaming_sample_smoke",
         zsnd_create_queued_streaming_sample_smoke},
        {"zsnd_sample_init_from_wave_data_directsound_smoke",
         zsnd_sample_init_from_wave_data_directsound_smoke},
        {"zsnd_sample_init_from_wave_data_a3d_smoke",
         zsnd_sample_init_from_wave_data_a3d_smoke},
        {"zsnd_wave_data_load_parse_reset_smoke",
         zsnd_wave_data_load_parse_reset_smoke},
        {"zsnd_wave_data_parse_chunks_smoke", zsnd_wave_data_parse_chunks_smoke},
        {"zsnd_wave_data_load_parse_edges_smoke",
         zsnd_wave_data_load_parse_edges_smoke},
        {"zsnd_wave_data_archive_load_smoke", zsnd_wave_data_archive_load_smoke},
        {"zsnd_snapshot_create_from_active_samples_smoke",
         zsnd_snapshot_create_from_active_samples_smoke},
        {"zsnd_snapshot_payload_capture_smoke", zsnd_snapshot_payload_capture_smoke},
        {"zsnd_snapshot_item_new_node_smoke", zsnd_snapshot_item_new_node_smoke},
        {"zsnd_snapshot_restore_all_with_global_volume_delta_smoke",
         zsnd_snapshot_restore_all_with_global_volume_delta_smoke},
        {"zsnd_snapshot_destroy_smoke", zsnd_snapshot_destroy_smoke},
        {"zsnd_snapshot_stop_all_if_playing_smoke",
         zsnd_snapshot_stop_all_if_playing_smoke},
        {"zsnd_cd_reset_track_state_smoke", zsnd_cd_reset_track_state_smoke},
        {"zsnd_cd_is_stereo_aux_enabled_smoke", zsnd_cd_is_stereo_aux_enabled_smoke},
        {"zsnd_cd_get_volume_smoke", zsnd_cd_get_volume_smoke},
        {"zsnd_cd_set_volume_smoke", zsnd_cd_set_volume_smoke},
        {"zsnd_cd_not_ready_playback_smoke", zsnd_cd_not_ready_playback_smoke},
        {"zsnd_cd_playback_mci_commands_smoke", zsnd_cd_playback_mci_commands_smoke},
        {"zsnd_cd_on_mci_notify_loop_smoke", zsnd_cd_on_mci_notify_loop_smoke},
        {"zsnd_cd_init_ready_guard_smoke", zsnd_cd_init_ready_guard_smoke},
        {"zsnd_cd_init_success_with_tracks_smoke",
         zsnd_cd_init_success_with_tracks_smoke},
        {"zsnd_cd_get_track_count_ready_guard_smoke",
         zsnd_cd_get_track_count_ready_guard_smoke},
        {"zsnd_cd_shutdown_track_list_smoke", zsnd_cd_shutdown_track_list_smoke},
        {"zsnd_cd_track_list_static_constructor_smoke",
         zsnd_cd_track_list_static_constructor_smoke},
        {"zsnd_cd_track_list_static_destructor_smoke",
         zsnd_cd_track_list_static_destructor_smoke},
        {"zsnd_cd_track_list_static_init_atexit_child_smoke",
         zsnd_cd_track_list_static_init_atexit_child_smoke},
        {"zsnd_cd_track_list_static_init_atexit_smoke",
         zsnd_cd_track_list_static_init_atexit_smoke},
        {"zreader_named_int_lookup_smoke", zreader_named_int_lookup_smoke},
        {"zreader_get_named_node_smoke", zreader_get_named_node_smoke},
        {"zreader_named_string_float_lookup_smoke", zreader_named_string_float_lookup_smoke},
        {"zreader_global_string_prefix_index_smoke", zreader_global_string_prefix_index_smoke},
        {"zrndr_global_string_table_load_dynamic_entries_smoke",
         zrndr_global_string_table_load_dynamic_entries_smoke},
        {"zreader_load_node_from_archive_smoke", zreader_load_node_from_archive_smoke},
        {"zreader_file_exists_and_list_create_smoke", zreader_file_exists_and_list_create_smoke},
        {"znetwork_local_identity_smoke", znetwork_local_identity_smoke},
        {"znetwork_dplay_close_release_smoke", znetwork_dplay_close_release_smoke},
        {"znetwork_unregister_packet_handler_smoke", znetwork_unregister_packet_handler_smoke},
        {"znetwork_clear_enumerated_session_list_smoke",
         znetwork_clear_enumerated_session_list_smoke},
        {"znetwork_clear_service_provider_list_smoke",
         znetwork_clear_service_provider_list_smoke},
        {"znetwork_clear_player_record_list_smoke", znetwork_clear_player_record_list_smoke},
        {"znetwork_player_record_accessors_smoke", znetwork_player_record_accessors_smoke},
        {"znetwork_apply_pkt01_player_color_assignments_smoke",
         znetwork_apply_pkt01_player_color_assignments_smoke},
        {"znetwork_shutdown_session_runtime_smoke", znetwork_shutdown_session_runtime_smoke},
        {"zfmv_script_init_null_path_smoke", zfmv_script_init_null_path_smoke},
        {"zfmv_script_reset_smoke", zfmv_script_reset_smoke},
        {"zfmv_script_cleanup_smoke", zfmv_script_cleanup_smoke},
        {"zfmv_script_append_action_smoke", zfmv_script_append_action_smoke},
        {"zfmv_script_begin_current_action_smoke",
         zfmv_script_begin_current_action_smoke},
        {"zfmv_script_begin_at_time_smoke", zfmv_script_begin_at_time_smoke},
        {"zfmv_script_update_smoke", zfmv_script_update_smoke},
        {"zfmv_script_update_at_time_smoke", zfmv_script_update_at_time_smoke},
        {"zfmv_script_begin_now_smoke", zfmv_script_begin_now_smoke},
        {"zfmv_script_load_actions_from_zrd_smoke",
         zfmv_script_load_actions_from_zrd_smoke},
        {"zfmv_action_image_constructor_with_screen_rect_smoke",
         zfmv_action_image_constructor_with_screen_rect_smoke},
        {"zfmv_action_image_constructor_scaled_smoke",
         zfmv_action_image_constructor_scaled_smoke},
        {"zfmv_action_fade_constructor_smoke",
         zfmv_action_fade_constructor_smoke},
        {"zfmv_action_play_avi_constructor_existing_file_smoke",
         zfmv_action_play_avi_constructor_existing_file_smoke},
        {"zfmv_action_play_avi_constructor_drive_fallback_smoke",
         zfmv_action_play_avi_constructor_drive_fallback_smoke},
        {"zfmv_playback_constructor_smoke", zfmv_playback_constructor_smoke},
        {"zfmv_playback_destructor_smoke", zfmv_playback_destructor_smoke},
        {"zfmv_playback_report_mci_error_smoke",
         zfmv_playback_report_mci_error_smoke},
        {"zfmv_playback_open_and_play_smoke", zfmv_playback_open_and_play_smoke},
        {"zfmv_playback_stop_and_close_smoke", zfmv_playback_stop_and_close_smoke},
        {"zfmv_playback_set_dest_rect_smoke", zfmv_playback_set_dest_rect_smoke},
        {"zfmv_action_play_mci_constructor_smoke",
         zfmv_action_play_mci_constructor_smoke},
        {"zfmv_action_blur_constructor_smoke",
         zfmv_action_blur_constructor_smoke},
        {"zfmv_action_blur_update_smoke",
         zfmv_action_blur_update_smoke},
        {"hud_ui_save_load_entry_is_newer_than_smoke",
         hud_ui_save_load_entry_is_newer_than_smoke},
        {"player_underwater_fx_pass3_ui_constructor_smoke",
         player_underwater_fx_pass3_ui_constructor_smoke},
        {"player_projectile_camera_fx_pass3_ui_constructor_smoke",
         player_projectile_camera_fx_pass3_ui_constructor_smoke},
        {"player_underwater_fx_pass3_ui_apply_blue_tint_smoke",
         player_underwater_fx_pass3_ui_apply_blue_tint_smoke},
        {"player_projectile_camera_fx_pass3_ui_apply_green_mask_smoke",
         player_projectile_camera_fx_pass3_ui_apply_green_mask_smoke},
        {"hud_ui_save_load_list_item_constructor_smoke",
         hud_ui_save_load_list_item_constructor_smoke},
        {"hud_ui_save_load_list_item_draw_smoke",
         hud_ui_save_load_list_item_draw_smoke},
        {"hud_ui_save_load_list_item_on_activate_smoke",
         hud_ui_save_load_list_item_on_activate_smoke},
        {"hud_ui_save_load_delete_button_on_activate_smoke",
         hud_ui_save_load_delete_button_on_activate_smoke},
        {"hud_ui_save_load_delete_save_file_smoke",
         hud_ui_save_load_delete_save_file_smoke},
        {"hud_ui_save_load_next_button_on_activate_smoke",
         hud_ui_save_load_next_button_on_activate_smoke},
        {"hud_ui_save_load_prev_button_on_activate_smoke",
         hud_ui_save_load_prev_button_on_activate_smoke},
        {"hud_ui_save_game_primary_action_button_on_activate_smoke",
         hud_ui_save_game_primary_action_button_on_activate_smoke},
        {"hud_ui_load_game_dialog_constructor_smoke",
         hud_ui_load_game_dialog_constructor_smoke},
        {"hud_ui_load_game_primary_action_button_on_activate_smoke",
         hud_ui_load_game_primary_action_button_on_activate_smoke},
        {"hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke",
         hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke},
        {"hud_ui_credits_quit_button_on_activate_smoke",
         hud_ui_credits_quit_button_on_activate_smoke},
        {"hud_cheat_clear_nanite_panel_cheat_sentinel_smoke",
         hud_cheat_clear_nanite_panel_cheat_sentinel_smoke},
        {"hud_ui_cheat_code_title_widget_on_activate_smoke",
         hud_ui_cheat_code_title_widget_on_activate_smoke},
        {"hud_ui_cheat_code_dialog_constructor_smoke",
         hud_ui_cheat_code_dialog_constructor_smoke},
        {"hud_ui_cheat_code_dialog_destructor_smoke",
         hud_ui_cheat_code_dialog_destructor_smoke},
        {"hud_ui_cheat_code_dialog_scalar_deleting_destructor_smoke",
         hud_ui_cheat_code_dialog_scalar_deleting_destructor_smoke},
        {"hud_ui_new_game_panel_constructor_cluster_smoke",
         hud_ui_new_game_panel_constructor_cluster_smoke},
        {"hud_ui_new_game_panel_overlay_owner_queue_enter_smoke",
         hud_ui_new_game_panel_overlay_owner_queue_enter_smoke},
        {"hud_ui_new_game_panel_overlay_owner_on_try_become_current_smoke",
         hud_ui_new_game_panel_overlay_owner_on_try_become_current_smoke},
        {"hud_ui_new_game_panel_overlay_owner_lifecycle_smoke",
         hud_ui_new_game_panel_overlay_owner_lifecycle_smoke},
        {"hud_ui_options_panel_overlay_owner_constructor_smoke",
         hud_ui_options_panel_overlay_owner_constructor_smoke},
        {"hud_ui_options_panel_overlay_owner_destructor_core_smoke",
         hud_ui_options_panel_overlay_owner_destructor_core_smoke},
        {"hud_ui_options_panel_overlay_owner_static_init_thunks_smoke",
         hud_ui_options_panel_overlay_owner_static_init_thunks_smoke},
        {"hud_ui_options_panel_overlay_owner_queue_enter_smoke",
         hud_ui_options_panel_overlay_owner_queue_enter_smoke},
        {"hud_ui_options_panel_overlay_owner_on_try_become_current_smoke",
         hud_ui_options_panel_overlay_owner_on_try_become_current_smoke},
        {"recoil_app_state_queue_block_init_from_cursor_smoke",
         recoil_app_state_queue_block_init_from_cursor_smoke},
        {"recoil_app_queue_switch_current_state_smoke",
         recoil_app_queue_switch_current_state_smoke},
        {"recoil_app_queue_push_state_smoke",
         recoil_app_queue_push_state_smoke},
        {"recoil_app_queue_exit_current_state_smoke",
         recoil_app_queue_exit_current_state_smoke},
        {"recoil_app_mfc_ole_module_constructor_smoke",
         recoil_app_mfc_ole_module_constructor_smoke},
        {"recoil_app_mfc_ole_module_destructor_smoke",
         recoil_app_mfc_ole_module_destructor_smoke},
        {"recoil_app_constructor_destructor_smoke",
         recoil_app_constructor_destructor_smoke},
        {"czgame_frame_constructor_smoke", czgame_frame_constructor_smoke},
        {"czrecoil_frame_constructor_smoke", czrecoil_frame_constructor_smoke},
        {"recoil_app_fmv_state_destructor_smoke",
         recoil_app_fmv_state_destructor_smoke},
        {"recoil_app_scalar_deleting_destructor_smoke",
         recoil_app_scalar_deleting_destructor_smoke},
        {"recoil_app_mission_fmv_state_destructor_smoke",
         recoil_app_mission_fmv_state_destructor_smoke},
        {"recoil_app_initialize_display_failure_smoke",
         recoil_app_initialize_display_failure_smoke},
        {"recoil_app_start_engine_and_queue_startup_state_smoke",
         recoil_app_start_engine_and_queue_startup_state_smoke},
        {"recoil_state_main_menu_transition_constructor_smoke",
         recoil_state_main_menu_transition_constructor_smoke},
        {"recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke",
         recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke},
        {"hud_ui_main_menu_dialog_constructor_smoke",
         hud_ui_main_menu_dialog_constructor_smoke},
        {"recoil_state_cheat_code_constructor_smoke",
         recoil_state_cheat_code_constructor_smoke},
        {"recoil_state_controls_lifecycle_smoke",
         recoil_state_controls_lifecycle_smoke},
        {"recoil_state_controls_activation_smoke",
         recoil_state_controls_activation_smoke},
        {"recoil_state_controls_on_resume_smoke",
         recoil_state_controls_on_resume_smoke},
        {"recoil_state_controls_queue_enter_smoke",
         recoil_state_controls_queue_enter_smoke},
        {"recoil_state_confirm_quit_queue_enter_smoke",
         recoil_state_confirm_quit_queue_enter_smoke},
        {"recoil_state_confirm_quit_destructor_smoke",
         recoil_state_confirm_quit_destructor_smoke},
        {"hud_ui_load_game_dialog_on_primary_action_smoke",
         hud_ui_load_game_dialog_on_primary_action_smoke},
        {"hud_ui_load_game_dialog_on_primary_action_thunk_smoke",
         hud_ui_load_game_dialog_on_primary_action_thunk_smoke},
        {"hud_ui_load_game_dialog_process_dialog_result_smoke",
         hud_ui_load_game_dialog_process_dialog_result_smoke},
        {"hud_ui_save_load_process_dialog_result_smoke",
         hud_ui_save_load_process_dialog_result_smoke},
        {"hud_ui_save_load_game_name_input_raw_keyboard_smoke",
         hud_ui_save_load_game_name_input_raw_keyboard_smoke},
        {"hud_ui_save_load_game_name_input_smoke",
         hud_ui_save_load_game_name_input_smoke},
        {"hud_ui_container_constructor_smoke", hud_ui_container_constructor_smoke},
        {"hud_ui_container_set_enabled_smoke",
         hud_ui_container_set_enabled_smoke},
        {"hud_ui_background_container_constructor_smoke",
         hud_ui_background_container_constructor_smoke},
        {"hud_ui_save_load_insert_entry_sorted_prefix_smoke",
         hud_ui_save_load_insert_entry_sorted_prefix_smoke},
        {"hud_ui_save_load_partition_entries_by_pivot_smoke",
         hud_ui_save_load_partition_entries_by_pivot_smoke},
        {"hud_ui_save_load_sort_entry_range_smoke", hud_ui_save_load_sort_entry_range_smoke},
        {"hud_ui_save_load_refresh_file_list_smoke",
         hud_ui_save_load_refresh_file_list_smoke},
        {"hud_ui_save_load_initialize_file_entries_smoke",
         hud_ui_save_load_initialize_file_entries_smoke},
        {"hud_ui_save_load_set_selected_entry_index_smoke",
         hud_ui_save_load_set_selected_entry_index_smoke},
        {"hud_ui_save_load_dialog_destructor_smoke",
         hud_ui_save_load_dialog_destructor_smoke},
        {"hud_ui_save_game_dialog_destructor_smoke",
         hud_ui_save_game_dialog_destructor_smoke},
        {"hud_ui_load_game_dialog_destructor_smoke",
         hud_ui_load_game_dialog_destructor_smoke},
        {"hud_ui_save_game_dialog_init_layout_smoke",
         hud_ui_save_game_dialog_init_layout_smoke},
        {"hud_ui_main_menu_dialog_save_load_checks_smoke",
         hud_ui_main_menu_dialog_save_load_checks_smoke},
        {"zarchive_list_get_at_smoke", zarchive_list_get_at_smoke},
        {"zarchive_list_get_count_smoke", zarchive_list_get_count_smoke},
        {"zreader_archive_list_and_search_paths_smoke",
         zreader_archive_list_and_search_paths_smoke},
        {"zreader_zrdr_free_search_path_list_smoke",
         zreader_zrdr_free_search_path_list_smoke},
        {"zutil_set_mission_zrdr_paths_and_mount_zbd_smoke",
         zutil_set_mission_zrdr_paths_and_mount_zbd_smoke},
        {"zreader_prealloc_and_pop_front_smoke", zreader_prealloc_and_pop_front_smoke},
        {"zreader_zrdr_push_free_node_smoke", zreader_zrdr_push_free_node_smoke},
        {"zreader_zrdr_init_search_path_smoke", zreader_zrdr_init_search_path_smoke},
        {"zreader_zrdr_shutdown_smoke", zreader_zrdr_shutdown_smoke},
        {"zreader_zrdr_wildcard_path_smoke", zreader_zrdr_wildcard_path_smoke},
        {"zreader_zrdr_free_node_pool_smoke", zreader_zrdr_free_node_pool_smoke},
        {"zreader_mount_index_archive_smoke", zreader_mount_index_archive_smoke},
        {"zreader_retail_zrdr_archives_smoke", zreader_retail_zrdr_archives_smoke},
        {"zreader_index_archive_flush_close_smoke", zreader_index_archive_flush_close_smoke},
        {"zreader_zrdr_get_file_size_smoke", zreader_zrdr_get_file_size_smoke},
        {"zreader_free_loaded_tree_smoke", zreader_free_loaded_tree_smoke},
        {"zreader_load_movers_from_zrd_smoke", zreader_load_movers_from_zrd_smoke},
        {"zreader_resolve_and_open_file_smoke", zreader_resolve_and_open_file_smoke},
        {"briefing_runtime_constructor_smoke",
         briefing_runtime_constructor_smoke},
        {"briefing_runtime_destructor_smoke",
         briefing_runtime_destructor_smoke},
        {"briefing_locator_panel_constructor_smoke",
         briefing_locator_panel_constructor_smoke},
        {"briefing_locator_panel_blit_dirty_rect_smoke",
         briefing_locator_panel_blit_dirty_rect_smoke},
        {"briefing_locator_panel_update_smoke",
         briefing_locator_panel_update_smoke},
        {"briefing_objective_picture_draw_noise_overlay_smoke",
         briefing_objective_picture_draw_noise_overlay_smoke},
        {"briefing_runtime_update_smoke", briefing_runtime_update_smoke},
        {"briefing_build_objective_actions_smoke",
         briefing_build_objective_actions_smoke},
        {"zhud_element_invalidate_smoke", zhud_element_invalidate_smoke},
        {"zhud_element_clip_and_invalidate_smoke",
         zhud_element_clip_and_invalidate_smoke},
        {"zhud_element_constructor_smoke", zhud_element_constructor_smoke},
        {"zhud_element_copy_constructor_smoke",
         zhud_element_copy_constructor_smoke},
        {"zhud_element_scalar_deleting_destructor_smoke",
         zhud_element_scalar_deleting_destructor_smoke},
        {"zhud_element_destructor_smoke", zhud_element_destructor_smoke},
        {"zhud_element_draw_dispatch_smoke", zhud_element_draw_dispatch_smoke},
        {"zhud_element_draw_base_smoke", zhud_element_draw_base_smoke},
        {"zhud_element_update_smoke", zhud_element_update_smoke},
        {"zhud_element_visible_smoke", zhud_element_visible_smoke},
        {"zhud_element_position_mutators_smoke",
         zhud_element_position_mutators_smoke},
        {"zhud_element_get_xy_smoke", zhud_element_get_xy_smoke},
        {"zhud_element_get_rect_smoke", zhud_element_get_rect_smoke},
        {"zhud_element_hit_test_true_smoke",
         zhud_element_hit_test_true_smoke},
        {"zhud_circle_constructor_and_hit_test_smoke",
         zhud_circle_constructor_and_hit_test_smoke},
        {"zhud_circle_draw_dirty_smoke", zhud_circle_draw_dirty_smoke},
        {"zhud_widget_constructor_smoke", zhud_widget_constructor_smoke},
        {"zhud_widget_invalidate_rect_smoke",
         zhud_widget_invalidate_rect_smoke},
        {"zhud_widget_draw_smoke", zhud_widget_draw_smoke},
        {"hud_ui_mp_exit_dialog_load_layout_smoke",
         hud_ui_mp_exit_dialog_load_layout_smoke},
        {"zhud_slot_draw_smoke", zhud_slot_draw_smoke},
        {"zhud_triplet_panel_draw_smoke", zhud_triplet_panel_draw_smoke},
        {"zhud_triplet_panel_set_visible_count_smoke",
         zhud_triplet_panel_set_visible_count_smoke},
        {"zhud_triplet_interpolate_layout_smoke",
         zhud_triplet_interpolate_layout_smoke},
        {"zhud_triplet_is_local_player_first_entry_smoke",
         zhud_triplet_is_local_player_first_entry_smoke},
        {"zhud_mgr_is_local_player_first_in_stats_list_smoke",
         zhud_mgr_is_local_player_first_in_stats_list_smoke},
        {"zhud_scoreboard_set_scale_and_rebuild_smoke",
         zhud_scoreboard_set_scale_and_rebuild_smoke},
        {"zhud_triplet_scoreboard_entry_update_smoke",
         zhud_triplet_scoreboard_entry_update_smoke},
        {"zhud_text_stack_constructors_smoke",
         zhud_text_stack_constructors_smoke},
        {"zhud_text_stack_set_font_all_smoke",
         zhud_text_stack_set_font_all_smoke},
        {"zhud_element_set_timer_smoke",
         zhud_element_set_timer_smoke},
        {"zhud_text_stack_push_line_smoke",
         zhud_text_stack_push_line_smoke},
        {"zhud_text_stack_clear_and_enable_smoke",
         zhud_text_stack_clear_and_enable_smoke},
        {"zhud_text_stack_clear_and_disable_smoke",
         zhud_text_stack_clear_and_disable_smoke},
        {"zhud_text_stack_destructor_core_smoke",
         zhud_text_stack_destructor_core_smoke},
        {"zhud_text_stack_layout_mutators_smoke",
         zhud_text_stack_layout_mutators_smoke},
        {"zhud_list_menu_entry_sort_smoke", zhud_list_menu_entry_sort_smoke},
        {"zhud_layout_hw_update_objective_dirty_rect_smoke",
         zhud_layout_hw_update_objective_dirty_rect_smoke},
        {"zhud_objective_update_meter_xpoints_smoke",
         zhud_objective_update_meter_xpoints_smoke},
        {"zhud_mgr_trigger_current_layout_on_activated_smoke",
         zhud_mgr_trigger_current_layout_on_activated_smoke},
        {"zhud_counter_constructor_smoke", zhud_counter_constructor_smoke},
        {"hud_ui_set_invalidate_mode_smoke", hud_ui_set_invalidate_mode_smoke},
        {"zhud_bar_and_meter_constructor_smoke", zhud_bar_and_meter_constructor_smoke},
        {"zhud_widget_release_image_if_owned_smoke",
         zhud_widget_release_image_if_owned_smoke},
        {"zhud_widget_set_image_borrowed_and_invalidate_smoke",
         zhud_widget_set_image_borrowed_and_invalidate_smoke},
        {"zhud_widget_destructor_core_smoke",
         zhud_widget_destructor_core_smoke},
        {"zhud_fill_bitmap_core_smoke", zhud_fill_bitmap_core_smoke},
        {"zhud_zrd_widget_ex17c_item_core_smoke",
         zhud_zrd_widget_ex17c_item_core_smoke},
        {"zhud_widget_set_image_by_path_owned_smoke",
         zhud_widget_set_image_by_path_owned_smoke},
        {"zhud_background_cursor_widget_member_constructor_smoke",
         zhud_background_cursor_widget_member_constructor_smoke},
        {"zhud_background_cursor_widget_rebuild_captured_image_smoke",
         zhud_background_cursor_widget_rebuild_captured_image_smoke},
        {"zhud_background_cursor_widget_set_image_borrowed_refresh_smoke",
         zhud_background_cursor_widget_set_image_borrowed_refresh_smoke},
        {"zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke",
         zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke},
        {"zhud_background_video_widget_constructor_smoke",
         zhud_background_video_widget_constructor_smoke},
        {"zhud_background_video_widget_destructor_smoke",
         zhud_background_video_widget_destructor_smoke},
        {"zhud_background_constructor_smoke", zhud_background_constructor_smoke},
        {"zhud_background_update_input_focus_smoke",
         zhud_background_update_input_focus_smoke},
        {"zhud_background_set_enabled_smoke", zhud_background_set_enabled_smoke},
        {"zhud_text_label_constructor_and_extents_smoke",
         zhud_text_label_constructor_and_extents_smoke},
        {"zhud_panel_constructor_default_smoke", zhud_panel_constructor_default_smoke},
        {"zhud_panel_copy_construct_core_smoke", zhud_panel_copy_construct_core_smoke},
        {"zhud_panel_draw_smoke", zhud_panel_draw_smoke},
        {"zhud_panel_set_font_smoke", zhud_panel_set_font_smoke},
        {"zhud_panel_set_text_fmt_smoke", zhud_panel_set_text_fmt_smoke},
        {"zhud_panel_query_text_height_smoke",
         zhud_panel_query_text_height_smoke},
        {"zhud_panel_layout_entry_copy_construct_smoke",
         zhud_panel_layout_entry_copy_construct_smoke},
        {"zhud_panel_layout_entry_copy_assign_smoke",
         zhud_panel_layout_entry_copy_assign_smoke},
        {"zhud_panel_layout_entry_copy_assign_range_smoke",
         zhud_panel_layout_entry_copy_assign_range_smoke},
        {"zhud_panel_layout_entry_destroy_range_smoke",
         zhud_panel_layout_entry_destroy_range_smoke},
        {"zhud_util_free_field_ptr_smoke", zhud_util_free_field_ptr_smoke},
        {"zhud_cmd_binding_entry_copy_range_smoke",
         zhud_cmd_binding_entry_copy_range_smoke},
        {"zhud_cmd_binding_destroy_range_smoke",
         zhud_cmd_binding_destroy_range_smoke},
        {"zhud_cmd_command_list_destructor_smoke",
         zhud_cmd_command_list_destructor_smoke},
        {"zhud_cmd_key_a_button_destructor_smoke",
         zhud_cmd_key_a_button_destructor_smoke},
        {"zhud_cmd_key_b_button_destructor_smoke",
         zhud_cmd_key_b_button_destructor_smoke},
        {"zhud_cmd_joy_button_destructor_smoke",
         zhud_cmd_joy_button_destructor_smoke},
        {"zhud_cmd_mouse_button_destructor_smoke",
         zhud_cmd_mouse_button_destructor_smoke},
        {"zhud_composite_panel_vector_clear_smoke",
         zhud_composite_panel_vector_clear_smoke},
        {"zhud_composite_panel_vector_insert_copies_smoke",
         zhud_composite_panel_vector_insert_copies_smoke},
        {"zhud_composite_panel_entry_copy_smoke",
         zhud_composite_panel_entry_copy_smoke},
        {"zhud_composite_panel_constructor_with_entry_count_smoke",
         zhud_composite_panel_constructor_with_entry_count_smoke},
        {"zhud_composite_panel_destructor_smoke", zhud_composite_panel_destructor_smoke},
        {"zhud_composite_panel_update_smoke", zhud_composite_panel_update_smoke},
        {"zhud_transition_text_panel_update_smoke",
         zhud_transition_text_panel_update_smoke},
        {"zhud_transition_text_panel_flash_rate_smoke",
         zhud_transition_text_panel_flash_rate_smoke},
        {"zhud_composite_panel_layout_entries_smoke",
         zhud_composite_panel_layout_entries_smoke},
        {"zhud_composite_panel_set_text_fmt_smoke",
         zhud_composite_panel_set_text_fmt_smoke},
        {"zhud_composite_panel_set_font_smoke", zhud_composite_panel_set_font_smoke},
        {"zhud_composite_panel_resize_entry_count_smoke",
         zhud_composite_panel_resize_entry_count_smoke},
        {"zhud_composite_panel_resize_vector_relayout_smoke",
         zhud_composite_panel_resize_vector_relayout_smoke},
        {"zhud_primitive_bind_target_set_segment_endpoints_smoke",
         zhud_primitive_bind_target_set_segment_endpoints_smoke},
        {"zhud_container_child_list_smoke", zhud_container_child_list_smoke},
        {"zhud_zrd_widget_constructor_smoke", zhud_zrd_widget_constructor_smoke},
        {"zhud_zrd_widget_helpers_smoke", zhud_zrd_widget_helpers_smoke},
        {"zhud_zrd_widget_load_from_zrd_smoke",
         zhud_zrd_widget_load_from_zrd_smoke},
        {"zhud_cycle_selector_widget_constructor_smoke",
         zhud_cycle_selector_widget_constructor_smoke},
        {"zhud_cycle_selector_text_entry_smoke",
         zhud_cycle_selector_text_entry_smoke},
        {"hud_ui_mp_exit_dialog_table_cluster_smoke",
         hud_ui_mp_exit_dialog_table_cluster_smoke},
        {"recoil_app_mp_exit_dialog_state_on_enter_smoke",
         recoil_app_mp_exit_dialog_state_on_enter_smoke},
        {"recoil_app_mp_exit_dialog_state_on_deactivate_smoke",
         recoil_app_mp_exit_dialog_state_on_deactivate_smoke},
        {"recoil_app_mp_exit_dialog_state_on_try_become_current_smoke",
         recoil_app_mp_exit_dialog_state_on_try_become_current_smoke},
        {"recoil_app_mp_exit_dialog_state_on_update_should_quit_smoke",
         recoil_app_mp_exit_dialog_state_on_update_should_quit_smoke},
        {"zhud_options_panel_lighting_init_from_options_smoke",
         zhud_options_panel_lighting_init_from_options_smoke},
        {"zhud_options_panel_lighting_sync_from_options_smoke",
         zhud_options_panel_lighting_sync_from_options_smoke},
        {"zhud_options_panel_perspective_init_from_options_smoke",
         zhud_options_panel_perspective_init_from_options_smoke},
        {"zhud_options_panel_full_hud_init_from_options_smoke",
         zhud_options_panel_full_hud_init_from_options_smoke},
        {"zhud_options_panel_object_detail_init_from_options_smoke",
         zhud_options_panel_object_detail_init_from_options_smoke},
        {"zhud_options_panel_object_detail_sync_from_options_smoke",
         zhud_options_panel_object_detail_sync_from_options_smoke},
        {"zhud_options_panel_texture_memory_init_from_options_smoke",
         zhud_options_panel_texture_memory_init_from_options_smoke},
        {"zhud_options_panel_texture_memory_sync_from_options_smoke",
         zhud_options_panel_texture_memory_sync_from_options_smoke},
        {"zhud_options_panel_effects_init_from_options_smoke",
         zhud_options_panel_effects_init_from_options_smoke},
        {"zhud_options_panel_sound_active_init_from_options_smoke",
         zhud_options_panel_sound_active_init_from_options_smoke},
        {"zhud_options_panel_sound_quality_init_from_options_smoke",
         zhud_options_panel_sound_quality_init_from_options_smoke},
        {"zhud_options_panel_sound_quality_sync_from_options_smoke",
         zhud_options_panel_sound_quality_sync_from_options_smoke},
        {"zhud_options_panel_sound_volume_sync_from_options_smoke",
         zhud_options_panel_sound_volume_sync_from_options_smoke},
        {"zhud_options_panel_sound_volume_on_activate_smoke",
         zhud_options_panel_sound_volume_on_activate_smoke},
        {"zhud_options_panel_music_volume_sync_from_options_smoke",
         zhud_options_panel_music_volume_sync_from_options_smoke},
        {"zhud_options_panel_music_volume_on_activate_smoke",
         zhud_options_panel_music_volume_on_activate_smoke},
        {"zhud_options_panel_resolution_sync_from_options_smoke",
         zhud_options_panel_resolution_sync_from_options_smoke},
        {"zhud_options_panel_resolution_on_activate_smoke",
         zhud_options_panel_resolution_on_activate_smoke},
        {"zhud_options_dialog_constructor_smoke", zhud_options_dialog_constructor_smoke},
        {"zhud_options_dialog_destructor_core_smoke",
         zhud_options_dialog_destructor_core_smoke},
        {"zhud_options_dialog_scalar_deleting_destructor_smoke",
         zhud_options_dialog_scalar_deleting_destructor_smoke},
        {"zhud_credits_panel_constructor_smoke", zhud_credits_panel_constructor_smoke},
        {"zhud_credits_panel_destructor_smoke", zhud_credits_panel_destructor_smoke},
        {"zhud_credits_panel_scalar_deleting_destructor_smoke",
         zhud_credits_panel_scalar_deleting_destructor_smoke},
        {"zhud_scrolling_text_destructor_smoke", zhud_scrolling_text_destructor_smoke},
        {"zhud_scrolling_text_scalar_deleting_destructor_smoke",
         zhud_scrolling_text_scalar_deleting_destructor_smoke},
        {"zhud_scrolling_text_load_from_zrd_smoke",
         zhud_scrolling_text_load_from_zrd_smoke},
        {"zhud_scrolling_text_update_smoke", zhud_scrolling_text_update_smoke},
        {"zhud_scrolling_text_on_activate_reset_owner_fade_smoke",
         zhud_scrolling_text_on_activate_reset_owner_fade_smoke},
        {"zhud_scrolling_text_update_scroll_positions_smoke",
         zhud_scrolling_text_update_scroll_positions_smoke},
        {"zhud_credits_panel_update_fade_and_exit_smoke",
         zhud_credits_panel_update_fade_and_exit_smoke},
        {"zhud_cmd_bind_button_base_constructor_smoke",
         zhud_cmd_bind_button_base_constructor_smoke},
        {"zhud_cmd_bind_button_base_destructor_core_smoke",
         zhud_cmd_bind_button_base_destructor_core_smoke},
        {"zhud_check_toggle_widget_helpers_smoke",
         zhud_check_toggle_widget_helpers_smoke},
        {"zhud_check_toggle_widget_load_from_zrd_smoke",
         zhud_check_toggle_widget_load_from_zrd_smoke},
        {"zhud_cmd_dialog_on_command_selection_changed_smoke",
         zhud_cmd_dialog_on_command_selection_changed_smoke},
        {"zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke",
         zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke},
        {"zhud_cmd_bind_button_base_rebuild_binding_slot_widgets_smoke",
         zhud_cmd_bind_button_base_rebuild_binding_slot_widgets_smoke},
        {"zhud_cmd_bind_button_base_load_from_zrd_smoke",
         zhud_cmd_bind_button_base_load_from_zrd_smoke},
        {"zhud_cmd_reset_button_on_activate_smoke",
         zhud_cmd_reset_button_on_activate_smoke},
        {"zhud_cmd_set_list_widget_on_activate_smoke",
         zhud_cmd_set_list_widget_on_activate_smoke},
        {"zhud_cmd_key_a_button_on_begin_capture_smoke",
         zhud_cmd_key_a_button_on_begin_capture_smoke},
        {"zhud_cmd_key_b_button_on_begin_capture_smoke",
         zhud_cmd_key_b_button_on_begin_capture_smoke},
        {"zhud_cmd_joy_button_on_begin_capture_smoke",
         zhud_cmd_joy_button_on_begin_capture_smoke},
        {"zhud_cmd_mouse_button_on_begin_capture_smoke",
         zhud_cmd_mouse_button_on_begin_capture_smoke},
        {"zhud_cmd_key_a_button_on_clear_binding_smoke",
         zhud_cmd_key_a_button_on_clear_binding_smoke},
        {"zhud_cmd_key_b_button_on_clear_binding_smoke",
         zhud_cmd_key_b_button_on_clear_binding_smoke},
        {"zhud_cmd_joy_button_on_clear_binding_smoke",
         zhud_cmd_joy_button_on_clear_binding_smoke},
        {"zhud_cmd_mouse_button_on_clear_binding_smoke",
         zhud_cmd_mouse_button_on_clear_binding_smoke},
        {"zhud_cmd_dialog_rebuild_command_binding_lists_smoke",
         zhud_cmd_dialog_rebuild_command_binding_lists_smoke},
        {"zhud_cmd_dialog_apply_primary_key_rebind_smoke",
         zhud_cmd_dialog_apply_primary_key_rebind_smoke},
        {"zhud_cmd_dialog_apply_secondary_key_rebind_smoke",
         zhud_cmd_dialog_apply_secondary_key_rebind_smoke},
        {"zhud_cmd_dialog_apply_joystick_button_rebind_smoke",
         zhud_cmd_dialog_apply_joystick_button_rebind_smoke},
        {"zhud_cmd_dialog_apply_mouse_button_rebind_smoke",
         zhud_cmd_dialog_apply_mouse_button_rebind_smoke},
        {"zhud_cmd_dialog_update_capture_state_idle_smoke",
         zhud_cmd_dialog_update_capture_state_idle_smoke},
        {"zhud_cmd_dialog_select_group_relative_smoke",
         zhud_cmd_dialog_select_group_relative_smoke},
        {"zhud_cmd_dialog_select_command_relative_smoke",
         zhud_cmd_dialog_select_command_relative_smoke},
        {"zhud_cmd_dialog_callback_navigation_smoke",
         zhud_cmd_dialog_callback_navigation_smoke},
        {"zhud_cmd_dialog_constructor_smoke", zhud_cmd_dialog_constructor_smoke},
        {"zhud_cmd_dialog_destructor_smoke", zhud_cmd_dialog_destructor_smoke},
        {"zhud_cmd_dialog_scalar_deleting_destructor_smoke",
         zhud_cmd_dialog_scalar_deleting_destructor_smoke},
        {"zhud_cmd_dialog_state_lifecycle_smoke",
         zhud_cmd_dialog_state_lifecycle_smoke},
        {"zhud_cmd_dialog_state_on_try_become_current_smoke",
         zhud_cmd_dialog_state_on_try_become_current_smoke},
        {"zhud_cmd_dialog_state_on_deactivate_smoke",
         zhud_cmd_dialog_state_on_deactivate_smoke},
        {"zhud_text_input_constructor_smoke", zhud_text_input_constructor_smoke},
        {"zhud_text_input_destructor_core_smoke",
         zhud_text_input_destructor_core_smoke},
        {"zhud_text_input_constructor_and_alloc_smoke",
         zhud_text_input_constructor_and_alloc_smoke},
        {"zhud_polyline_and_slider_border_constructor_smoke",
         zhud_polyline_and_slider_border_constructor_smoke},
        {"zhud_numeric_text_input_base_constructor_smoke",
         zhud_numeric_text_input_base_constructor_smoke},
        {"zhud_background_bind_primitive_node_to_element_smoke",
         zhud_background_bind_primitive_node_to_element_smoke},
        {"zhud_std_ptr_vector_clear_no_op_destroy_smoke",
         zhud_std_ptr_vector_clear_no_op_destroy_smoke},
        {"zmath_crt_matherr_handler_smoke", zmath_crt_matherr_handler_smoke},
        {"zmath_matrix_stack_and_direction_smoke", zmath_matrix_stack_and_direction_smoke},
        {"zmath_vec3_array_transform_direction_smoke",
         zmath_vec3_array_transform_direction_smoke},
        {"zmath_mat_build_euler_rotation3x3_smoke",
         zmath_mat_build_euler_rotation3x3_smoke},
        {"zmath_extract_euler_smoke", zmath_extract_euler_smoke},
        {"zmath_projection_setup_smoke", zmath_projection_setup_smoke},
        {"zmath_projection_batches_smoke", zmath_projection_batches_smoke},
        {"zmath_project_point_and_clamp_to_screen_clip_smoke",
         zmath_project_point_and_clamp_to_screen_clip_smoke},
        {"zmath_clip_line_segment_z_range_smoke", zmath_clip_line_segment_z_range_smoke},
        {"zmath_vec3_lerp_smoke", zmath_vec3_lerp_smoke},
        {"zmath_vec3_lerp_normalize_smoke", zmath_vec3_lerp_normalize_smoke},
        {"zmath_vec3_direction_to_smoke", zmath_vec3_direction_to_smoke},
        {"zmath_line_vs_sphere_hit_smoke", zmath_line_vs_sphere_hit_smoke},
        {"zmath_vec3_perp2d_smoke", zmath_vec3_perp2d_smoke},
        {"zmath_vec3_perp_xz_smoke", zmath_vec3_perp_xz_smoke},
        {"zmath_vec3_scale_add_smoke", zmath_vec3_scale_add_smoke},
        {"zmath_vec3_slerp_smoke", zmath_vec3_slerp_smoke},
        {"zmath_vec3_midpoint_smoke", zmath_vec3_midpoint_smoke},
        {"zmath_perspective_texture_interpolants_smoke",
         zmath_perspective_texture_interpolants_smoke},
        {"zmath_vec3_normalize_and_div_scalar_smoke", zmath_vec3_normalize_and_div_scalar_smoke},
        {"zmath_array_add_scaled_and_transform_smoke", zmath_array_add_scaled_and_transform_smoke},
        {"zmath_load_view_smoke", zmath_load_view_smoke},
        {"zmath_quaternion_helpers_smoke", zmath_quaternion_helpers_smoke},
        {"zmath_approx_exp_neg_smoke", zmath_approx_exp_neg_smoke},
        {"zrndr_get_active_region_state_smoke", zrndr_get_active_region_state_smoke},
        {"zrndr_framebuffer_and_stride_cache_smoke", zrndr_framebuffer_and_stride_cache_smoke},
        {"zrndr_init_globals_smoke", zrndr_init_globals_smoke},
        {"zrndr_immediate_line_dispatch_smoke", zrndr_immediate_line_dispatch_smoke},
        {"zrndr_lens_flare_queue_projected_sample_smoke",
         zrndr_lens_flare_queue_projected_sample_smoke},
        {"zrndr_lens_flare_build_visible_sample_list_smoke",
         zrndr_lens_flare_build_visible_sample_list_smoke},
        {"zrndr_lens_flare_draw_queued_samples16_smoke",
         zrndr_lens_flare_draw_queued_samples16_smoke},
        {"zrndr_lens_flare_draw_sample_smoke", zrndr_lens_flare_draw_sample_smoke},
        {"zrndr_lens_flare_draw_queued_samples_scaled16_smoke",
         zrndr_lens_flare_draw_queued_samples_scaled16_smoke},
        {"zrndr_lens_flare_stage_helpers_smoke", zrndr_lens_flare_stage_helpers_smoke},
        {"zrndr_span_occlusion_filter_sample_list_smoke",
         zrndr_span_occlusion_filter_sample_list_smoke},
        {"zrndr_lens_flare_draw_sample_stage_clipped_smoke",
         zrndr_lens_flare_draw_sample_stage_clipped_smoke},
        {"zrndr_lens_flare_draw_visible_sample_stages_smoke",
         zrndr_lens_flare_draw_visible_sample_stages_smoke},
        {"zrndr_lens_flare_draw_visible_sample_smoke", zrndr_lens_flare_draw_visible_sample_smoke},
        {"zrndr_lens_flare_draw_visible_samples_smoke",
         zrndr_lens_flare_draw_visible_samples_smoke},
        {"zrndr_span_occlusion_add_polygon_smoke", zrndr_span_occlusion_add_polygon_smoke},
        {"zrndr_span_occlusion_submit_rect_smoke", zrndr_span_occlusion_submit_rect_smoke},
        {"zrndr_span_occlusion_test_depth_order_pair_smoke",
         zrndr_span_occlusion_test_depth_order_pair_smoke},
        {"zrndr_span_occlusion_insert_no_depth_smoke", zrndr_span_occlusion_insert_no_depth_smoke},
        {"zrndr_span_occlusion_build_span_list_smoke", zrndr_span_occlusion_build_span_list_smoke},
        {"zrndr_span_occlusion_insert_local_smoke", zrndr_span_occlusion_insert_local_smoke},
        {"zrndr_span_occlusion_build_span_list_fast_smoke",
         zrndr_span_occlusion_build_span_list_fast_smoke},
        {"zrndr_span_occlusion_test_column_visibility_smoke",
         zrndr_span_occlusion_test_column_visibility_smoke},
        {"zscene_test_projected_sphere_visible_smoke",
         zscene_test_projected_sphere_visible_smoke},
        {"zrndr_span_occlusion_test_point_visibility_smoke",
         zrndr_span_occlusion_test_point_visibility_smoke},
        {"zrndr_span_occlusion_test_sample_smoke", zrndr_span_occlusion_test_sample_smoke},
        {"zrndr_draw_circle_octants_smoke", zrndr_draw_circle_octants_smoke},
        {"zrndr_draw_circle_outline_smoke", zrndr_draw_circle_outline_smoke},
        {"zrndr_plot_pixel16_smoke", zrndr_plot_pixel16_smoke},
        {"zrndr_draw_line16_smoke", zrndr_draw_line16_smoke},
        {"zrndr_draw_line16_segmented_smoke", zrndr_draw_line16_segmented_smoke},
        {"zrndr_draw_line16_clipped_smoke", zrndr_draw_line16_clipped_smoke},
        {"zcliprect_clip_poly_near_z_smoke", zcliprect_clip_poly_near_z_smoke},
        {"zcliprect_clip_poly_near_z_attr0_smoke", zcliprect_clip_poly_near_z_attr0_smoke},
        {"zcliprect_clip_poly_zrange_attr012_smoke", zcliprect_clip_poly_zrange_attr012_smoke},
        {"zcliprect_clip_poly_zrange_no_uv_smoke", zcliprect_clip_poly_zrange_no_uv_smoke},
        {"zcliprect_clip_poly_zrange_no_uv_attribs_smoke",
         zcliprect_clip_poly_zrange_no_uv_attribs_smoke},
        {"zcliprect_clip_poly_no_uv_alt_smoke", zcliprect_clip_poly_no_uv_alt_smoke},
        {"zcliprect_clip_poly_no_uv_smoke", zcliprect_clip_poly_no_uv_smoke},
        {"zcliprect_clip_poly_uv_smoke", zcliprect_clip_poly_uv_smoke},
        {"zcliprect_clip_poly_uv_attr012_smoke", zcliprect_clip_poly_uv_attr012_smoke},
        {"zcliprect_clip_poly_no_uv_attr0_alt_smoke", zcliprect_clip_poly_no_uv_attr0_alt_smoke},
        {"zcliprect_clip_poly_no_uv_attr012_alt_smoke",
         zcliprect_clip_poly_no_uv_attr012_alt_smoke},
        {"zcliprect_trivial_reject_poly_xy_smoke", zcliprect_trivial_reject_poly_xy_smoke},
        {"zrndr_span_occlusion_reset_shutdown_smoke", zrndr_span_occlusion_reset_shutdown_smoke},
        {"zrndr_span_occlusion_init_build_smoke", zrndr_span_occlusion_init_build_smoke},
        {"zrndr_span_occlusion_rasterize_smoke", zrndr_span_occlusion_rasterize_smoke},
        {"zrndr_rasterize_poly_with_span_list_smoke", zrndr_rasterize_poly_with_span_list_smoke},
        {"zrndr_rasterize_poly_smoke", zrndr_rasterize_poly_smoke},
        {"zrndr_draw_flat_immediate_smoke", zrndr_draw_flat_immediate_smoke},
        {"zrndr_submit_poly_with_span_list_smoke", zrndr_submit_poly_with_span_list_smoke},
        {"zrndr_submit_textured_poly_uniform_smoke", zrndr_submit_textured_poly_uniform_smoke},
        {"zrndr_submit_textured_poly_per_vertex_smoke",
         zrndr_submit_textured_poly_per_vertex_smoke},
        {"zrndr_flush_transparent_queue_smoke", zrndr_flush_transparent_queue_smoke},
        {"zrndr_flush_overwrite_queue_smoke", zrndr_flush_overwrite_queue_smoke},
        {"zrndr_texture_mip_select_variant_smoke", zrndr_texture_mip_select_variant_smoke},
        {"zrndr_draw_flat_queued_smoke", zrndr_draw_flat_queued_smoke},
        {"zrndr_renderer_draw_poly_tlv_smoke", zrndr_renderer_draw_poly_tlv_smoke},
        {"zrndr_draw_textured_queued_smoke", zrndr_draw_textured_queued_smoke},
        {"zrndr_draw_textured_queued_alpha_smoke", zrndr_draw_textured_queued_alpha_smoke},
        {"zrndr_draw_textured_fan_tri_smoke", zrndr_draw_textured_fan_tri_smoke},
        {"zrndr_span_routine_selection_smoke", zrndr_span_routine_selection_smoke},
        {"zrndr_perspective_texture_delta_x_smoke", zrndr_perspective_texture_delta_x_smoke},
        {"zrndr_perspective_texture_far_z_smoke", zrndr_perspective_texture_far_z_smoke},
        {"zrndr_set_inverse_z_tolerance_smoke", zrndr_set_inverse_z_tolerance_smoke},
        {"zrndr_perspective_adaptive_span_params_smoke",
         zrndr_perspective_adaptive_span_params_smoke},
        {"zrndr_overlay_rect_submit_smoke", zrndr_overlay_rect_submit_smoke},
        {"zrndr_overlay_and_mmx_masks_smoke", zrndr_overlay_and_mmx_masks_smoke},
        {"zrndr_span_alpha_blend_565_const_alpha_pal8_smoke",
         zrndr_span_alpha_blend_565_const_alpha_pal8_smoke},
        {"zrndr_fill_span16_opaque_smoke", zrndr_fill_span16_opaque_smoke},
        {"zrndr_fill_span555_solid_smoke", zrndr_fill_span555_solid_smoke},
        {"zrndr_fill_span565_solid_smoke", zrndr_fill_span565_solid_smoke},
        {"zrndr_span_masked_tex16_to_565_smoke", zrndr_span_masked_tex16_to_565_smoke},
        {"zrndr_span_masked_16_from_pal8_to565_smoke", zrndr_span_masked_16_from_pal8_to565_smoke},
        {"zrndr_span_alpha_blend_565_const_alpha_tex16_smoke",
         zrndr_span_alpha_blend_565_const_alpha_tex16_smoke},
        {"zrndr_span_alpha_blend_565_from_tex16_alpha8_smoke",
         zrndr_span_alpha_blend_565_from_tex16_alpha8_smoke},
        {"zrndr_span_alpha_blend_565_const_alpha_from_tex16_alpha8_smoke",
         zrndr_span_alpha_blend_565_const_alpha_from_tex16_alpha8_smoke},
        {"zrndr_span_alpha_blend_565_mmx_from_tex16_alpha8_smoke",
         zrndr_span_alpha_blend_565_mmx_from_tex16_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_from_tex16_alpha8_smoke",
         zrndr_span_alpha_blend_555_from_tex16_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_const_alpha_from_tex16_alpha8_smoke",
         zrndr_span_alpha_blend_555_const_alpha_from_tex16_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_mmx_from_tex16_alpha8_smoke",
         zrndr_span_alpha_blend_555_mmx_from_tex16_alpha8_smoke},
        {"zrndr_span_alpha_blend_565_from_pal8_alpha8_smoke",
         zrndr_span_alpha_blend_565_from_pal8_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_from_pal8_alpha8_smoke",
         zrndr_span_alpha_blend_555_from_pal8_alpha8_smoke},
        {"zrndr_span_alpha_blend_565_const_alpha_from_pal8_alpha8_smoke",
         zrndr_span_alpha_blend_565_const_alpha_from_pal8_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_const_alpha_from_pal8_alpha8_smoke",
         zrndr_span_alpha_blend_555_const_alpha_from_pal8_alpha8_smoke},
        {"zrndr_span_alpha_blend_565_mmx_from_pal8_alpha8_smoke",
         zrndr_span_alpha_blend_565_mmx_from_pal8_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_mmx_from_pal8_alpha8_smoke",
         zrndr_span_alpha_blend_555_mmx_from_pal8_alpha8_smoke},
        {"zrndr_span_alpha_blend_555_const_alpha_tex16_smoke",
         zrndr_span_alpha_blend_555_const_alpha_tex16_smoke},
        {"zrndr_span_alpha_blend_565_const_alpha_fast_pal8_smoke",
         zrndr_span_alpha_blend_565_const_alpha_fast_pal8_smoke},
        {"zrndr_span_alpha_blend_555_const_alpha_fast_pal8_smoke",
         zrndr_span_alpha_blend_555_const_alpha_fast_pal8_smoke},
        {"zrndr_fog_blend_span_565_scalar_smoke", zrndr_fog_blend_span_565_scalar_smoke},
        {"zrndr_fog_blend_span_555_scalar_smoke", zrndr_fog_blend_span_555_scalar_smoke},
        {"zrndr_fog_blend_span_565_mmx_smoke", zrndr_fog_blend_span_565_mmx_smoke},
        {"zrndr_fog_blend_span_555_mmx_smoke", zrndr_fog_blend_span_555_mmx_smoke},
        {"zrndr_span_copy_16_from_tex16_smoke", zrndr_span_copy_16_from_tex16_smoke},
        {"zrndr_span_copy_16_from_tex16_switch_vshift_smoke",
         zrndr_span_copy_16_from_tex16_switch_vshift_smoke},
        {"zrndr_span_masked_16_from_tex16_switch_vshift_smoke",
         zrndr_span_masked_16_from_tex16_switch_vshift_smoke},
        {"zrndr_span_copy_16_from_pal8_switch_vshift_smoke",
         zrndr_span_copy_16_from_pal8_switch_vshift_smoke},
        {"zrndr_span_masked_16_from_pal8_switch_vshift_smoke",
         zrndr_span_masked_16_from_pal8_switch_vshift_smoke},
        {"zrndr_span_shade_16_from_pal8_switch_vshift_smoke",
         zrndr_span_shade_16_from_pal8_switch_vshift_smoke},
        {"zrndr_palette_remap_key_smoke", zrndr_palette_remap_key_smoke},
        {"zrndr_fog_target_color_smoke", zrndr_fog_target_color_smoke},
        {"zrndr_fog_commit_and_blend_smoke", zrndr_fog_commit_and_blend_smoke},
        {"zrndr_and_zmodel_current_fog_color_smoke", zrndr_and_zmodel_current_fog_color_smoke},
        {"zvideo_fog_color_commit_smoke", zvideo_fog_color_commit_smoke},
        {"zvideo_fog_target_color_commit_smoke", zvideo_fog_target_color_commit_smoke},
        {"zturret_reset_iteration_state_smoke", zturret_reset_iteration_state_smoke},
        {"zturret_runtime_init_defaults_smoke", zturret_runtime_init_defaults_smoke},
        {"zturret_runtime_has_active_node_smoke", zturret_runtime_has_active_node_smoke},
        {"zturret_runtime_init_from_reader_node_smoke",
         zturret_runtime_init_from_reader_node_smoke},
        {"zturret_update_fire_position_from_parts_smoke",
         zturret_update_fire_position_from_parts_smoke},
        {"zturret_update_aim_and_part_matrices_smoke",
         zturret_update_aim_and_part_matrices_smoke},
        {"zturret_select_fire_point_and_aim_smoke",
         zturret_select_fire_point_and_aim_smoke},
        {"zturret_update_fire_burst_timer_smoke",
         zturret_update_fire_burst_timer_smoke},
        {"zturret_runtime_tick_smoke", zturret_runtime_tick_smoke},
        {"zturret_tick_all_runtimes_round_robin_smoke",
         zturret_tick_all_runtimes_round_robin_smoke},
        {"zturret_disable_tick_callback_smoke", zturret_disable_tick_callback_smoke},
        {"zturret_enable_tick_callback_smoke", zturret_enable_tick_callback_smoke},
        {"zturret_load_definitions_from_path_smoke",
         zturret_load_definitions_from_path_smoke},
        {"zturret_fire_weapon_smoke", zturret_fire_weapon_smoke},
        {"zturret_fire_weapon_callback_smoke", zturret_fire_weapon_callback_smoke},
        {"zturret_damage_and_on_damage_smoke", zturret_damage_and_on_damage_smoke},
        {"zturret_shutdown_leaf_smoke", zturret_shutdown_leaf_smoke},
        {"zgame_return_only_stub_smoke", zgame_return_only_stub_smoke},
        {"zopt_network_enabled_accessor_smoke", zopt_network_enabled_accessor_smoke},
        {"hud_sensor_mission_identity_smoke", hud_sensor_mission_identity_smoke},
        {"hud_sensor_tracker_get_objective_briefing_strings_smoke",
         hud_sensor_tracker_get_objective_briefing_strings_smoke},
        {"zclass_type_list_alloc_and_insert_smoke",
         zclass_type_list_alloc_and_insert_smoke},
        {"zclass_alloc_node_from_free_list_smoke",
         zclass_alloc_node_from_free_list_smoke},
        {"zclass_node_propagate_transform_dirty_smoke",
         zclass_node_propagate_transform_dirty_smoke},
        {"zclass_object3d_reset_transform_dirty_smoke",
         zclass_object3d_reset_transform_dirty_smoke},
        {"zclass_object3d_init_smoke", zclass_object3d_init_smoke},
        {"zclass_node_action_callback_smoke", zclass_node_action_callback_smoke},
        {"zclass_node_priority_smoke", zclass_node_priority_smoke},
        {"zclass_find_by_name_and_filtered_iter_smoke",
         zclass_find_by_name_and_filtered_iter_smoke},
        {"zclass_sound_leaf_smoke", zclass_sound_leaf_smoke},
        {"zclass_sound_get_position_smoke", zclass_sound_get_position_smoke},
        {"zloc_message_lookup_failure_smoke", zloc_message_lookup_failure_smoke},
        {"zloc_load_unload_messages_dll_smoke", zloc_load_unload_messages_dll_smoke},
        {"zimage_font_glyph_scan_smoke", zimage_font_glyph_scan_smoke},
        {"zimage_font_measure_string_smoke", zimage_font_measure_string_smoke},
        {"zimage_font_blit_string_smoke", zimage_font_blit_string_smoke},
        {"zimage_fonts_load_missing_smoke", zimage_fonts_load_missing_smoke},
        {"zvid_pack_color_rgb_smoke", zvid_pack_color_rgb_smoke},
        {"zvid_pack_color_rgb_floats_smoke", zvid_pack_color_rgb_floats_smoke},
        {"zvideo_palette_remap_no_recipes_smoke",
         zvideo_palette_remap_no_recipes_smoke},
        {"zvideo_palette_remap_recipe_variants_smoke",
         zvideo_palette_remap_recipe_variants_smoke},
        {"zvid_image_create_format_size_pixels_smoke",
         zvid_image_create_format_size_pixels_smoke},
        {"zvideo_image_set_pixels_smoke", zvideo_image_set_pixels_smoke},
        {"zvideo_capture_surface_to_image_smoke", zvideo_capture_surface_to_image_smoke},
        {"zvideo_fx_set_surface_state_smoke", zvideo_fx_set_surface_state_smoke},
        {"zvideo_surface_accessors_smoke", zvideo_surface_accessors_smoke},
        {"zvideo_primary_surface_rect_scratch_smoke",
         zvideo_primary_surface_rect_scratch_smoke},
        {"zvideo_image_alpha_clear_smoke", zvideo_image_alpha_clear_smoke},
        {"zvideo_mode_geometry_and_set_video_mode_smoke",
         zvideo_mode_geometry_and_set_video_mode_smoke},
        {"zvideo_init_video_system_reentry_guard_smoke",
         zvideo_init_video_system_reentry_guard_smoke},
        {"zvideo_bind_renderer_dispatch_smoke", zvideo_bind_renderer_dispatch_smoke},
        {"zvideo_module_init_smoke", zvideo_module_init_smoke},
        {"zvideo_at_exit_release_all_interfaces_and_surfaces_smoke",
         zvideo_at_exit_release_all_interfaces_and_surfaces_smoke},
        {"zvideo_return_success_stub_smoke", zvideo_return_success_stub_smoke},
        {"zvideo_clear_dispatch_and_exchange_smoke",
         zvideo_clear_dispatch_and_exchange_smoke},
        {"zvid_cached_client_rect_smoke", zvid_cached_client_rect_smoke},
        {"zvideo_shutdown_video_system_smoke", zvideo_shutdown_video_system_smoke},
        {"zvideo_frame_scratch_buffers_smoke", zvideo_frame_scratch_buffers_smoke},
        {"zvideo_noise_shutdown_buffers_smoke", zvideo_noise_shutdown_buffers_smoke},
        {"zvideo_buff_clip_coord_to_range_smoke", zvideo_buff_clip_coord_to_range_smoke},
        {"zvideo_buff_blt_source_to_primary_clipped_smoke",
         zvideo_buff_blt_source_to_primary_clipped_smoke},
        {"zvid_image_blit_to_active_target_smoke",
         zvid_image_blit_to_active_target_smoke},
        {"zvid_image_blit_to_framebuffer_clipped_smoke",
         zvid_image_blit_to_framebuffer_clipped_smoke},
        {"zvideo_buff_copy_surface_rect_to_image_smoke",
         zvideo_buff_copy_surface_rect_to_image_smoke},
        {"zvideo_draw_noise_rect_smoke", zvideo_draw_noise_rect_smoke},
        {"zvideo_blur_region_horizontal_smoke", zvideo_blur_region_horizontal_smoke},
        {"zvideo_blur_region_vertical_smoke", zvideo_blur_region_vertical_smoke},
        {"zvideo_blur_region_combined_smoke", zvideo_blur_region_combined_smoke},
        {"zvideo_blur_region_by_mode_smoke", zvideo_blur_region_by_mode_smoke},
        {"zvideo_texture_pack_load_image_smoke", zvideo_texture_pack_load_image_smoke},
        {"zvideo_dd_prepare_window_for_mode_smoke",
         zvideo_dd_prepare_window_for_mode_smoke},
        {"zvid_query_device_video_memory_bytes_smoke",
         zvid_query_device_video_memory_bytes_smoke},
        {"zvid_query_texture_memory_bytes_smoke", zvid_query_texture_memory_bytes_smoke},
        {"zvideo_pixel_pack_setup_smoke", zvideo_pixel_pack_setup_smoke},
        {"zvideo_pixel_pack_getters_smoke", zvideo_pixel_pack_getters_smoke},
        {"zvideo_dd_report_error_smoke", zvideo_dd_report_error_smoke},
        {"zvideo_dd_create_directdraw2_for_selected_device_smoke",
         zvideo_dd_create_directdraw2_for_selected_device_smoke},
        {"zvideo_dd_open_video_mode_smoke", zvideo_dd_open_video_mode_smoke},
        {"zvideo_dd_create_surface3_from_desc_smoke",
         zvideo_dd_create_surface3_from_desc_smoke},
        {"zvideo_dispatch_wrappers_smoke", zvideo_dispatch_wrappers_smoke},
        {"zvideo_dd_lock_directdraw_surface_smoke",
         zvideo_dd_lock_directdraw_surface_smoke},
        {"zvideo_dd_unlock_directdraw_surface_smoke",
         zvideo_dd_unlock_directdraw_surface_smoke},
        {"zvideo_dd_lock_surface_wait_restore_smoke",
         zvideo_dd_lock_surface_wait_restore_smoke},
        {"zvideo_dd_unlock_surface_wait_restore_smoke",
         zvideo_dd_unlock_surface_wait_restore_smoke},
        {"zvideo_surface_state_lock_skip_smoke",
         zvideo_surface_state_lock_skip_smoke},
        {"zvideo_dd_lock_surface_state_smoke",
         zvideo_dd_lock_surface_state_smoke},
        {"zvideo_dd_unlock_surface_state_smoke",
         zvideo_dd_unlock_surface_state_smoke},
        {"zvideo_dd_verify_fullscreen_surface_locks_smoke",
         zvideo_dd_verify_fullscreen_surface_locks_smoke},
        {"zvideo_texture_record_destroy_smoke", zvideo_texture_record_destroy_smoke},
        {"zvideo_dd_release_all_interfaces_and_surfaces_smoke",
         zvideo_dd_release_all_interfaces_and_surfaces_smoke},
        {"zvideo_dd_verify_surface_state_locking_smoke",
         zvideo_dd_verify_surface_state_locking_smoke},
        {"zvideo_dd_teardown_video_subsystem_smoke",
         zvideo_dd_teardown_video_subsystem_smoke},
        {"zvideo_dd_shutdown_video_system_smoke",
         zvideo_dd_shutdown_video_system_smoke},
        {"zvideo_dd_zbuffer_depth_fill_rect_smoke",
         zvideo_dd_zbuffer_depth_fill_rect_smoke},
        {"zvideo_dd_clear_screen_and_zbuffer_rect_smoke",
         zvideo_dd_clear_screen_and_zbuffer_rect_smoke},
        {"zvideo_dd_clear_sw_backbuffer_and_zbuffer_rects_smoke",
         zvideo_dd_clear_sw_backbuffer_and_zbuffer_rects_smoke},
        {"zvideo_dd_palette_set_entries_smoke",
         zvideo_dd_palette_set_entries_smoke},
        {"zvideo_get_display_mode_bpp_smoke",
         zvideo_get_display_mode_bpp_smoke},
        {"zvideo_dd_set_display_mode_smoke",
         zvideo_dd_set_display_mode_smoke},
        {"zvideo_dd_set_video_mode_smoke",
         zvideo_dd_set_video_mode_smoke},
        {"zvideo_dd_restore_display_surfaces_smoke",
         zvideo_dd_restore_display_surfaces_smoke},
        {"zvideo_dd_init_fullscreen_software_pixel_pack_smoke",
         zvideo_dd_init_fullscreen_software_pixel_pack_smoke},
        {"zvideo_dd_create_half_res_backbuffer_surfaces_smoke",
         zvideo_dd_create_half_res_backbuffer_surfaces_smoke},
        {"zvideo_dd_create_fullscreen_software_surfaces_smoke",
         zvideo_dd_create_fullscreen_software_surfaces_smoke},
        {"zvideo_dd_create_fullscreen_hw_surfaces_smoke",
         zvideo_dd_create_fullscreen_hw_surfaces_smoke},
        {"zvideo_dd_create_fullscreen_surfaces_for_renderer_smoke",
         zvideo_dd_create_fullscreen_surfaces_for_renderer_smoke},
        {"zvideo_dd_present_display_mode_surface_smoke",
         zvideo_dd_present_display_mode_surface_smoke},
        {"zvideo_present_display_mode_surface_null_smoke",
         zvideo_present_display_mode_surface_null_smoke},
        {"zvideo_dd3d_present_display_mode_surface_smoke",
         zvideo_dd3d_present_display_mode_surface_smoke},
        {"zvideo_image_lazy_create_backing_surface_guards_smoke",
         zvideo_image_lazy_create_backing_surface_guards_smoke},
        {"zvideo_dd_image_populate_surface_from_heap_pixels_smoke",
         zvideo_dd_image_populate_surface_from_heap_pixels_smoke},
        {"zvideo_dd_image_lazy_create_backing_surface_smoke",
         zvideo_dd_image_lazy_create_backing_surface_smoke},
        {"zvideo_dd_image_lazy_create_video_memory_surface_smoke",
         zvideo_dd_image_lazy_create_video_memory_surface_smoke},
        {"zvideo_dd_image_upload_pixels_to_surface_smoke",
         zvideo_dd_image_upload_pixels_to_surface_smoke},
        {"zvideo_dd_image_release_surface_smoke",
         zvideo_dd_image_release_surface_smoke},
        {"zvideo_image_surface_helpers_guard_smoke",
         zvideo_image_surface_helpers_guard_smoke},
        {"zvideo_set_renderer_type_smoke", zvideo_set_renderer_type_smoke},
        {"zvideo_set_half_res_adjust_mode_smoke",
         zvideo_set_half_res_adjust_mode_smoke},
        {"zvid_texture_pack_load_state_getter_smoke",
         zvid_texture_pack_load_state_getter_smoke},
        {"zvid_texture_pack_load_state_setter_smoke",
         zvid_texture_pack_load_state_setter_smoke},
        {"zvid_option_accessors_smoke", zvid_option_accessors_smoke},
        {"zvid_set_video_mode_index_smoke",
         zvid_set_video_mode_index_smoke},
        {"zvideo_init_set_surface_geometry_from_mode_index_smoke",
         zvideo_init_set_surface_geometry_from_mode_index_smoke},
        {"zvideo_select_hw_api_device_smoke", zvideo_select_hw_api_device_smoke},
        {"zvideo_dd_enum_direct3d_device_callback_smoke",
         zvideo_dd_enum_direct3d_device_callback_smoke},
        {"zvideo_dd_enumerate_direct3d_devices_for_record_smoke",
         zvideo_dd_enumerate_direct3d_devices_for_record_smoke},
        {"zvideo_dd_enum_directdraw_device_callback_smoke",
         zvideo_dd_enum_directdraw_device_callback_smoke},
        {"zvideo_dd_run_device_enumeration_smoke",
         zvideo_dd_run_device_enumeration_smoke},
        {"zvideo_dd_startup_enumerate_default_select_smoke",
         zvideo_dd_startup_enumerate_default_select_smoke},
        {"zvideo_flip_to_gdi_if_attached_null_smoke",
         zvideo_flip_to_gdi_if_attached_null_smoke},
        {"zvideo_dd3d_set_fog_enable_smoke", zvideo_dd3d_set_fog_enable_smoke},
        {"zvideo_pending_wireframe_state_smoke",
         zvideo_pending_wireframe_state_smoke},
        {"zvideo_pending_dither_enable_smoke",
         zvideo_pending_dither_enable_smoke},
        {"zvideo_dd3d_begin_scene_flush_pending_smoke",
         zvideo_dd3d_begin_scene_flush_pending_smoke},
        {"zvideo_submit_poly_color_attr_smoke",
         zvideo_submit_poly_color_attr_smoke},
        {"zvideo_submit_poly_color_attr_immediate_smoke",
         zvideo_submit_poly_color_attr_immediate_smoke},
        {"zvideo_submit_polygon_queue_smoke",
         zvideo_submit_polygon_queue_smoke},
        {"zvideo_submit_polygon_immediate_smoke",
         zvideo_submit_polygon_immediate_smoke},
        {"zvideo_submit_polygon_lit_queue_smoke",
         zvideo_submit_polygon_lit_queue_smoke},
        {"zvideo_submit_polygon_lit_immediate_smoke",
         zvideo_submit_polygon_lit_immediate_smoke},
        {"zvideo_texture_record_release_upload_surface_smoke",
         zvideo_texture_record_release_upload_surface_smoke},
        {"zvideo_texture_record_finalize_upload_smoke",
         zvideo_texture_record_finalize_upload_smoke},
        {"zvideo_texture_record_create_and_power_smoke",
         zvideo_texture_record_create_and_power_smoke},
        {"zvideo_texture_record_lock_upload_surface_smoke",
         zvideo_texture_record_lock_upload_surface_smoke},
        {"zvideo_texture_record_unlock_upload_surface_smoke",
         zvideo_texture_record_unlock_upload_surface_smoke},
        {"zvideo_frustum_test_sphere_clip_mask_smoke",
         zvideo_frustum_test_sphere_clip_mask_smoke},
        {"zvideo_quad_batch_depth_and_rhw_smoke",
         zvideo_quad_batch_depth_and_rhw_smoke},
        {"zvideo_queue_solid_quad_smoke",
         zvideo_queue_solid_quad_smoke},
        {"zvideo_flush_quad_batch_empty_smoke",
         zvideo_flush_quad_batch_empty_smoke},
        {"zvideo_flush_quad_batch_smoke",
         zvideo_flush_quad_batch_smoke},
        {"zvideo_flush_sorted_polys_empty_smoke",
         zvideo_flush_sorted_polys_empty_smoke},
        {"zvideo_flush_sorted_polys_smoke",
         zvideo_flush_sorted_polys_smoke},
        {"zvideo_flush_overwrite_polys_empty_smoke",
         zvideo_flush_overwrite_polys_empty_smoke},
        {"zvideo_flush_overwrite_polys_smoke",
         zvideo_flush_overwrite_polys_smoke},
        {"zvideo_create_texture_record_guards_smoke",
         zvideo_create_texture_record_guards_smoke},
        {"zvideo_dd3d_create_texture_record_smoke",
         zvideo_dd3d_create_texture_record_smoke},
        {"zvideo_dd3d_create_device_state_smoke",
         zvideo_dd3d_create_device_state_smoke},
        {"zvideo_convert_image_pixels_for_texture_smoke",
         zvideo_convert_image_pixels_for_texture_smoke},
        {"zvideo_dd3d_upload_image_to_surface_smoke",
         zvideo_dd3d_upload_image_to_surface_smoke},
        {"zimage_texdir_find_or_create_missing_smoke", zimage_texdir_find_or_create_missing_smoke},
        {"zimage_texdir_build_mip_chain_smoke", zimage_texdir_build_mip_chain_smoke},
        {"zvid_texture_pack_ensure_builtin_smoke", zvid_texture_pack_ensure_builtin_smoke},
        {"zvid_texture_pack_ensure_default_smoke", zvid_texture_pack_ensure_default_smoke},
        {"zimage_texdir_load_pending_entries_smoke", zimage_texdir_load_pending_entries_smoke},
        {"zimage_texdir_load_pending_entries_renderer_smoke",
         zimage_texdir_load_pending_entries_renderer_smoke},
        {"zclass_node_load_flag_bit8_material_images_and_texture_pack_smoke",
         zclass_node_load_flag_bit8_material_images_and_texture_pack_smoke},
        {"zimage_texdir_base_name_path_smoke", zimage_texdir_base_name_path_smoke},
        {"zimage_texdir_variant_image_smoke", zimage_texdir_variant_image_smoke},
        {"zimage_texdir_find_by_name_smoke", zimage_texdir_find_by_name_smoke},
        {"zimage_texdir_write_smoke", zimage_texdir_write_smoke},
        {"zimage_init_option_fallback_smoke", zimage_init_option_fallback_smoke},
        {"zimage_init_texture_directory_smoke", zimage_init_texture_directory_smoke},
        {"zvid_image_resample_square_smoke", zvid_image_resample_square_smoke},
        {"zvid_image_release_owned_buffers_smoke", zvid_image_release_owned_buffers_smoke},
        {"zvid_image_destroy_smoke", zvid_image_destroy_smoke},
        {"zimg_init_smoke", zimg_init_smoke},
        {"zinterp_context_logf_smoke", zinterp_context_logf_smoke},
        {"zinterp_context_report_errorf_smoke", zinterp_context_report_errorf_smoke},
        {"zinterp_context_inc_error_count_smoke", zinterp_context_inc_error_count_smoke},
        {"zinterp_context_find_macro_value_smoke", zinterp_context_find_macro_value_smoke},
        {"zinterp_context_is_macro_true_smoke", zinterp_context_is_macro_true_smoke},
        {"zinterp_context_set_macro_smoke", zinterp_context_set_macro_smoke},
        {"zinterp_context_clear_tables_smoke", zinterp_context_clear_tables_smoke},
        {"zinterp_context_destroy_smoke", zinterp_context_destroy_smoke},
        {"zinterp_context_destructor_smoke", zinterp_context_destructor_smoke},
        {"zinterp_context_constructor_smoke", zinterp_context_constructor_smoke},
        {"zinterp_global_context_constructor_smoke", zinterp_global_context_constructor_smoke},
        {"zinterp_global_context_hooks_smoke", zinterp_global_context_hooks_smoke},
        {"zinterp_global_context_static_init_smoke", zinterp_global_context_static_init_smoke},
        {"zinterp_scroll_always_callbacks_smoke", zinterp_scroll_always_callbacks_smoke},
        {"zinterp_register_scroll_always_node_smoke",
         zinterp_register_scroll_always_node_smoke},
        {"zinterp_context_eval_condition_expr_smoke",
         zinterp_context_eval_condition_expr_smoke},
        {"zinterp_context_expand_macro_refs_smoke", zinterp_context_expand_macro_refs_smoke},
        {"zinterp_context_next_token_smoke", zinterp_context_next_token_smoke},
        {"zinterp_context_parse_scalar_tokens_smoke",
         zinterp_context_parse_scalar_tokens_smoke},
        {"zinterp_context_var_entry_helpers_smoke", zinterp_context_var_entry_helpers_smoke},
        {"zinterp_context_command_helpers_smoke", zinterp_context_command_helpers_smoke},
        {"zinterp_context_validate_args_and_node_type_smoke",
         zinterp_context_validate_args_and_node_type_smoke},
        {"zinterp_context_tokenize_line_smoke", zinterp_context_tokenize_line_smoke},
        {"zinterp_context_tokenize_comment_and_prepared_smoke",
         zinterp_context_tokenize_comment_and_prepared_smoke},
        {"zinterp_context_echo_tokens_smoke", zinterp_context_echo_tokens_smoke},
        {"zinterp_context_push_file_frame_smoke", zinterp_context_push_file_frame_smoke},
        {"zinterp_context_pop_file_frame_smoke", zinterp_context_pop_file_frame_smoke},
        {"zinterp_context_print_node_tree_smoke", zinterp_context_print_node_tree_smoke},
        {"zinterp_context_read_text_line_smoke", zinterp_context_read_text_line_smoke},
        {"zinterp_context_read_prepared_tokens_smoke",
         zinterp_context_read_prepared_tokens_smoke},
        {"zinterp_context_read_prepared_empty_packet_smoke",
         zinterp_context_read_prepared_empty_packet_smoke},
        {"zinterp_context_load_prepared_script_index_smoke",
         zinterp_context_load_prepared_script_index_smoke},
        {"zinterp_context_load_prepared_script_index_stale_smoke",
         zinterp_context_load_prepared_script_index_stale_smoke},
        {"zinterp_context_open_prepared_script_stream_smoke",
         zinterp_context_open_prepared_script_stream_smoke},
        {"zinterp_context_open_prepared_script_stream_newer_source_smoke",
         zinterp_context_open_prepared_script_stream_newer_source_smoke},
        {"zinterp_context_handle_builtin_command_smoke",
         zinterp_context_handle_builtin_command_smoke},
        {"zinterp_context_run_stream_builtin_smoke", zinterp_context_run_stream_builtin_smoke},
        {"zinterp_context_run_script_file_nested_source_smoke",
         zinterp_context_run_script_file_nested_source_smoke},
        {"zinterp_context_dispatch_core_node_flags_smoke",
         zinterp_context_dispatch_core_node_flags_smoke},
        {"zinterp_context_dispatch_core_camera_clip_smoke",
         zinterp_context_dispatch_core_camera_clip_smoke},
        {"zinterp_context_dispatch_core_world_and_globals_smoke",
         zinterp_context_dispatch_core_world_and_globals_smoke},
        {"zinterp_context_dispatch_core_resource_globals_smoke",
         zinterp_context_dispatch_core_resource_globals_smoke},
        {"zinterp_context_dispatch_core_object3d_smoke",
         zinterp_context_dispatch_core_object3d_smoke},
        {"zinterp_context_dispatch_core_model_material_smoke",
         zinterp_context_dispatch_core_model_material_smoke},
        {"zinterp_context_dispatch_core_light_lod_smoke",
         zinterp_context_dispatch_core_light_lod_smoke},
        {"zinterp_global_context_static_init_register_smoke",
         zinterp_global_context_static_init_register_smoke},
        {"zimage_init_mission_resources_smoke", zimage_init_mission_resources_smoke},
        {"zimage_shutdown_texdir_smoke", zimage_shutdown_texdir_smoke},
        {"zsys_find_file_on_drive_type_negative_smoke",
         zsys_find_file_on_drive_type_negative_smoke},
        {"zsys_runtime_probe_leaves_smoke", zsys_runtime_probe_leaves_smoke},
        {"zsys_cpuid_mmx_smoke", zsys_cpuid_mmx_smoke},
        {"zsys_cpu_leaf_helpers_smoke", zsys_cpu_leaf_helpers_smoke},
        {"zsys_exit_process_with_cleanup_child_smoke",
         zsys_exit_process_with_cleanup_child_smoke},
        {"zsys_exit_process_with_cleanup_smoke", zsys_exit_process_with_cleanup_smoke},
        {"zerror_init_output_context_smoke", zerror_init_output_context_smoke},
        {"zerror_emit_debug_buffer_smoke", zerror_emit_debug_buffer_smoke},
        {"zerror_report_old_noop_smoke", zerror_report_old_noop_smoke},
        {"zcom_query_interface_from_interface_map_smoke",
         zcom_query_interface_from_interface_map_smoke},
        {"zcom_connection_point_container_advise_smoke",
         zcom_connection_point_container_advise_smoke},
        {"zcom_connection_point_container_unadvise_smoke",
         zcom_connection_point_container_unadvise_smoke},
        {"time_reset_smoke", time_reset_smoke},
        {"time_tick_smoke", time_tick_smoke},
    };

    const int directxResult = recoil_legacy_directx_header_smoke() > 0 ? 0 : 1;
    if (directxResult != 0) {
        std::printf("recoil_legacy_directx_header_smoke failed: %d\n", directxResult);
    }

    return directxResult + RunSmokeTests(tests, static_cast<int>(sizeof(tests) / sizeof(tests[0])),
                                         argc > 1 ? argv[1] : nullptr);
}
