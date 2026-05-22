extern "C" int recoil_native_build_anchor(void);
extern "C" int recoil_legacy_directx_header_smoke(void);
extern "C" int recoil_mfc42_provider_smoke(void);
extern "C" int czrecoil_frame_build_window_title_smoke(void);
extern "C" int czframe_metadata_accessors_smoke(void);
extern "C" int czrecoil_frame_set_menu_bar_visibility_smoke(void);
extern "C" int czrecoil_frame_configure_mode_feature_flags_smoke(void);
extern "C" int czrecoil_frame_set_hw_api_and_init_mode_smoke(void);
extern "C" int czrecoil_frame_init_fallback_mode_smoke(void);
extern "C" int czrecoil_frame_ensure_hw_api_initialized_smoke(void);
extern "C" int czrecoil_frame_init_startup_hw_api_from_options_smoke(void);
extern "C" int czrecoil_frame_menu_toggle_smoke(void);
extern "C" int czrecoil_frame_menu_exit_game_smoke(void);
extern "C" int czrecoil_frame_update_video_mode_cmd_ui_smoke(void);
extern "C" int czrecoil_frame_hw_api_menu_cmd_ui_smoke(void);
extern "C" int czrecoil_frame_audio_input_menu_smoke(void);
extern "C" int czgame_frame_is_window_valid_smoke(void);
extern "C" int czgame_frame_build_window_title_smoke(void);
extern "C" int czgame_frame_constructor_smoke(void);
extern "C" int czrecoil_frame_destructor_smoke(void);
extern "C" int czgame_frame_on_app_idle_dispatch_message_smoke(void);
extern "C" int briefing_stop_and_shutdown_thread_smoke(void);
extern "C" int gamenet_list_reset_smoke(void);
extern "C" int hud_timer_panel_net_state_clear_tail_flags_smoke(void);
extern "C" int gamenet_find_player_row_and_status_bits_smoke(void);
extern "C" int gamenet_reset_hud_timer_panel_net_state_smoke(void);
extern "C" int gamenet_host_update_session_status_fields_smoke(void);
extern "C" int gamenet_timer_status_packet_smoke(void);
extern "C" int gamenet_scoreboard_snapshot_packet_smoke(void);
extern "C" int gamenet_chat_message_packet_smoke(void);
extern "C" int gamenet_show_player_kill_message_smoke(void);
extern "C" int gamenet_player_kill_event_packet_smoke(void);
extern "C" int gamenet_reassign_player_colors_smoke(void);
extern "C" int gamenet_player_row_apply_color_tint_smoke(void);
extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void);
extern "C" int ainet_free_all_smoke(void);
extern "C" int player_ai_discard_negative_branch_nodes_smoke(void);
extern "C" int player_find_alt_gun_controller_smoke(void);
extern "C" int player_alt_gun_fire_point_selection_smoke(void);
extern "C" int zutil_save_game_state_free_owned_resources_smoke(void);
extern "C" int player_destroy_save_game_state_smoke(void);
extern "C" int player_shutdown_mission_runtime_smoke(void);
extern "C" int zfmv_script_reset_smoke(void);
extern "C" int zfmv_script_init_null_path_smoke(void);
extern "C" int zfmv_script_cleanup_smoke(void);
extern "C" int zfmv_script_append_action_smoke(void);
extern "C" int zfmv_script_run_blocking_empty_smoke(void);
extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void);
extern "C" int zfmv_action_image_constructor_scaled_smoke(void);
extern "C" int zfmv_action_fade_constructor_smoke(void);
extern "C" int zfmv_playback_init_smoke(void);
extern "C" int zfmv_playback_set_dest_rect_smoke(void);
extern "C" int zfmv_playback_destructor_smoke(void);
extern "C" int zfmv_playback_report_mci_error_smoke(void);
extern "C" int zfmv_stream_destructor_empty_smoke(void);
extern "C" int zfmv_stream_constructor_missing_file_smoke(void);
extern "C" int zfmv_stream_init_missing_file_smoke(void);
extern "C" int zfmv_action_play_avi_constructor_existing_file_smoke(void);
extern "C" int zfmv_action_play_mci_constructor_smoke(void);
extern "C" int zfmv_action_blur_constructor_smoke(void);
extern "C" int zfmv_script_load_actions_from_zrd_smoke(void);
extern "C" int zfmv_action_wait_begin_update_smoke(void);
extern "C" int zfmv_action_base_destructor_smoke(void);
extern "C" int zfmv_action_derived_scalar_deleting_destructor_smoke(void);
extern "C" int zfmv_action_play_sound_begin_missing_sample_smoke(void);
extern "C" int zgame_return_only_stub_smoke(void);
extern "C" int zgame_options_init_registry_context_smoke(void);
extern "C" int zmodel_display_init_smoke(void);
extern "C" int zmodel_damage_mask_uv_smoke(void);
extern "C" int zmodel_display_shutdown_smoke(void);
extern "C" int zmodel_material_pool_entry_smoke(void);
extern "C" int zmodel_material_defaults_and_find_smoke(void);
extern "C" int zmodel_material_write_gamez_smoke(void);
extern "C" int zmodel_material_read_gamez_smoke(void);
extern "C" int zmodel_dipool_write_to_stream_smoke(void);
extern "C" int zmodel_dipool_read_from_stream_smoke(void);
extern "C" int zmodel_dipool_read_entry_by_index_from_stream_smoke(void);
extern "C" int zmodel_material_and_di_clone_smoke(void);
extern "C" int zmodel_scrolling_texture_update_smoke(void);
extern "C" int zmodel_render_point_queue_entry_smoke(void);
extern "C" int zmodel_light_fog_fade_smoke(void);
extern "C" int zmodel_light_build_light_weights_smoke(void);
extern "C" int zmodel_light_point_in_polygon_init_smoke(void);
extern "C" int zmodel_light_build_attr0_depth_fade_smoke(void);
extern "C" int zclass_world_apply_pending_fog_settings_smoke(void);
extern "C" int zclass_shutdown_core_smoke(void);
extern "C" int zclass_gwnode_update_smoke(void);
extern "C" int zgame_options_find_option_smoke(void);
extern "C" int zopt_eval_int_compare_op_smoke(void);
extern "C" int zopt_fullscreen_accessors_smoke(void);
extern "C" int zopt_network_enabled_accessor_smoke(void);
extern "C" int hud_sensor_objective_slot_reset_smoke(void);
extern "C" int hud_sensor_tracker_unload_objectives_smoke(void);
extern "C" int zopt_view_rect_target_side_effects_smoke(void);
extern "C" int zopt_section_accessor_smoke(void);
extern "C" int zclipalt_set_source_rect_smoke(void);
extern "C" int zclipalt_set_target_rect_smoke(void);
extern "C" int zclass_cls_di_set_stop_after_first_hit_smoke(void);
extern "C" int zhud_element_constructor_smoke(void);
extern "C" int zhud_element_copy_constructor_smoke(void);
extern "C" int zhud_element_set_timer_smoke(void);
extern "C" int zhud_circle_constructor_and_hit_test_smoke(void);
extern "C" int zhud_composite_panel_vector_clear_smoke(void);
extern "C" int zhud_composite_panel_entry_copy_smoke(void);
extern "C" int zhud_flash_panel_compute_blend_color_smoke(void);
extern "C" int zhud_layout_shutdown_stub_smoke(void);
extern "C" int zhud_layout_hw_release_images_smoke(void);
extern "C" int zhud_element_clip_and_invalidate_smoke(void);
extern "C" int zhud_element_visible_smoke(void);
extern "C" int zhud_element_update_smoke(void);
extern "C" int zhud_element_position_mutators_smoke(void);
extern "C" int zhud_container_child_list_smoke(void);
extern "C" int zhud_container_destructor_core_smoke(void);
extern "C" int zhud_widget_constructor_smoke(void);
extern "C" int zhud_widget_default_ctor_thunk_smoke(void);
extern "C" int zhud_zrd_widget_constructor_smoke(void);
extern "C" int zhud_zrd_widget_helpers_smoke(void);
extern "C" int zhud_zrd_widget_load_from_zrd_smoke(void);
extern "C" int zhud_check_toggle_widget_helpers_smoke(void);
extern "C" int zhud_check_toggle_widget_load_from_zrd_smoke(void);
extern "C" int zhud_cycle_selector_widget_constructor_smoke(void);
extern "C" int zhud_cycle_selector_widget_load_from_zrd_smoke(void);
extern "C" int zhud_fill_bitmap_core_smoke(void);
extern "C" int zhud_zrd_widget_ex17c_item_core_smoke(void);
extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void);
extern "C" int zhud_message_box_leaf_handlers_smoke(void);
extern "C" int zhud_background_container_focus_smoke(void);
extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void);
extern "C" int zhud_background_constructor_smoke(void);
extern "C" int zhud_background_bind_widget_by_name_smoke(void);
extern "C" int zhud_background_free_loaded_tree_roots_smoke(void);
extern "C" int zhud_background_set_enabled_smoke(void);
extern "C" int zhud_dialog_controller_blit_owned_surface_smoke(void);
extern "C" int zhud_font_style_constructor_smoke(void);
extern "C" int zhud_numeric_text_input_base_constructor_smoke(void);
extern "C" int zhud_widget_release_and_destructor_core_smoke(void);
extern "C" int zhud_triplet_panel_constructor_smoke(void);
extern "C" int zhud_triplet_panel_set_visible_count_smoke(void);
extern "C" int zhud_triplet_panel_shutdown_items_stub_smoke(void);
extern "C" int zhud_triplet_panel_destructor_core_smoke(void);
extern "C" int zhud_text_input_destructor_core_smoke(void);
extern "C" int zhud_text_input_constructor_and_alloc_smoke(void);
extern "C" int zhud_mgr_objective_block_destructor_smoke(void);
extern "C" int hud_ui_net_exit_destroy_global_smoke(void);
extern "C" int hud_ui_net_exit_show_tick_smoke(void);
extern "C" int hud_ui_aux_overlay_text_lines_smoke(void);
extern "C" int zhud_mgr_destroy_sensor_window_null_smoke(void);
extern "C" int zhud_mgr_disable_hud_smoke(void);
extern "C" int zhud_slot_destructors_smoke(void);
extern "C" int zhud_slot_draw_smoke(void);
extern "C" int zhud_stats_list_element_update_smoke(void);
extern "C" int zhud_counter_constructor_smoke(void);
extern "C" int zhud_counter_release_state_images_smoke(void);
extern "C" int zhud_message_release_images_smoke(void);
extern "C" int zhud_message_constructor_smoke(void);
extern "C" int zhud_message_destructors_smoke(void);
extern "C" int zhud_shield_message_widget_destructor_smoke(void);
extern "C" int zhud_bar_and_meter_constructor_smoke(void);
extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void);
extern "C" int zhud_polyline_draw_and_slider_update_smoke(void);
extern "C" int zhud_panel_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_panel_text_color_shadow_smoke(void);
extern "C" int zhud_panel_constructor_default_smoke(void);
extern "C" int zhud_panel_copy_construct_core_smoke(void);
extern "C" int zhud_text_label_constructor_and_extents_smoke(void);
extern "C" int zhud_panel_set_font_smoke(void);
extern "C" int zhud_panel_set_text_fmt_smoke(void);
extern "C" int zhud_panel_measure_text_prefix_rect_smoke(void);
extern "C" int zhud_panel_query_text_height_smoke(void);
extern "C" int zhud_panel_simple_constructor_smoke(void);
extern "C" int zhud_timer_panel_set_time_smoke(void);
extern "C" int zhud_timer_panel_update_hms_smoke(void);
extern "C" int zhud_timer_panel_global_accessors_smoke(void);
extern "C" int zhud_timer_panel_update_smoke(void);
extern "C" int zhud_timer_panel_zar_read_smoke(void);
extern "C" int zhud_timer_panel_zar_write_smoke(void);
extern "C" int zhud_timer_and_counter_constructor_smoke(void);
extern "C" int zhud_timer_panel_float_constructor_smoke(void);
extern "C" int zhud_triplet_constructor_smoke(void);
extern "C" int zhud_triplet_destructor_core_smoke(void);
extern "C" int zhud_triplet_scoreboard_entry_update_smoke(void);
extern "C" int zhud_stats_list_destructors_smoke(void);
extern "C" int zhud_string_menu_destructor_core_smoke(void);
extern "C" int zhud_text_stack_constructors_smoke(void);
extern "C" int zhud_text_stack_push_line_smoke(void);
extern "C" int zhud_text_stack_destructor_core_smoke(void);
extern "C" int zhud_sensor_viewport_rect_smoke(void);
extern "C" int zhud_objective_update_smoke(void);
extern "C" int zhud_objective_begin_smoke(void);
extern "C" int zhud_mgr_viewport_activation_smoke(void);
extern "C" int zhud_mgr_update_target_reticle_smoke(void);
extern "C" int zhud_mgr_init_layouts_reentry_smoke(void);
extern "C" int zhud_mgr_shutdown_resources_smoke(void);
extern "C" int zclass_node_propagate_transform_dirty_smoke(void);
extern "C" int player_add_scaled_hud_counter_smoke(void);
extern "C" int zutil_save_game_state_list_smoke(void);
extern "C" int zclass_object3d_visible_and_color_smoke(void);
extern "C" int zclass_object3d_alpha_scale_and_lit_smoke(void);
extern "C" int zclass_object3d_transform_getters_smoke(void);
extern "C" int zclass_object3d_transform_setters_smoke(void);
extern "C" int zclass_object3d_reset_transform_dirty_smoke(void);
extern "C" int zclass_model_ref_lerp_queue_reset_smoke(void);
extern "C" int zclass_type_list_alloc_and_insert_smoke(void);
extern "C" int zclass_animate_update_smoke(void);
extern "C" int zclass_sequence_new_add_child_smoke(void);
extern "C" int zclass_object3d_set_matrix_smoke(void);
extern "C" int zclass_node_free_and_deferred_work_smoke(void);
extern "C" int zclass_alloc_node_from_free_list_smoke(void);
extern "C" int zclass_node_flag16_flag17_smoke(void);
extern "C" int zclass_node_metadata_accessors_smoke(void);
extern "C" int zclass_copy_node_display_instance_smoke(void);
extern "C" int zclass_copy_node_base_data_smoke(void);
extern "C" int zclass_copy_node_unimplemented_stubs_smoke(void);
extern "C" int zclass_copy_object3d_and_lod_smoke(void);
extern "C" int zclass_copy_node_dispatch_and_wrappers_smoke(void);
extern "C" int zclass_node_action_callback_smoke(void);
extern "C" int zclass_node_priority_smoke(void);
extern "C" int zclass_node_pick_flag_accessors_smoke(void);
extern "C" int zclass_node_extra_flag_setters_smoke(void);
extern "C" int zclass_node_vertex_alpha_and_root_smoke(void);
extern "C" int zclass_node_world_child_smoke(void);
extern "C" int zclass_gwnode_get_world_position_smoke(void);
extern "C" int zclass_world_add_child_at_grid_smoke(void);
extern "C" int zclass_world_remove_light_sound_smoke(void);
extern "C" int zclass_child_generic_link_smoke(void);
extern "C" int zclass_child_generic_remove_smoke(void);
extern "C" int zclass_object3d_child_wrappers_smoke(void);
extern "C" int zclass_delete_node_from_lists_smoke(void);
extern "C" int zclass_find_by_name_and_filtered_iter_smoke(void);
extern "C" int zclass_node_predicate_helpers_smoke(void);
extern "C" int zclass_lifecycle_leaf_smoke(void);
extern "C" int zclass_zbd_leaf_helpers_smoke(void);
extern "C" int zclass_zbd_node_ref_list_indices_smoke(void);
extern "C" int zclass_zbd_single_node_class_data_smoke(void);
extern "C" int zclass_zbd_read_single_node_class_data_smoke(void);
extern "C" int zclass_zbd_read_node_table_smoke(void);
extern "C" int zclass_zbd_write_node_table_smoke(void);
extern "C" int gamez_write_zbd_file_smoke(void);
extern "C" int gamez_read_zbd_file_smoke(void);
extern "C" int gamez_open_and_read_zbd_header_smoke(void);
extern "C" int gamez_reload_display_instances_smoke(void);
extern "C" int zclass_try_free_node_smoke(void);
extern "C" int zdi_ref_and_pool_free_smoke(void);
extern "C" int zclass_set_display_instance_smoke(void);
extern "C" int zclass_remove_dispatch_smoke(void);
extern "C" int zclass_destroy_node_recursive_display_smoke(void);
extern "C" int zclass_object3d_delete_node_smoke(void);
extern "C" int zclass_sound_leaf_smoke(void);
extern "C" int zclass_object3d_init_smoke(void);
extern "C" int zclass_window_new_smoke(void);
extern "C" int zclass_display_init_smoke(void);
extern "C" int zclass_lod_leaf_smoke(void);
extern "C" int zclass_light_new_smoke(void);
extern "C" int zclass_damage_handler_smoke(void);
extern "C" int zclass_camera_view_distance_smoke(void);
extern "C" int zinput_mouse_update_acquire_state_smoke(void);
extern "C" int zinput_joystick_option_accessors_smoke(void);
extern "C" int zinput_force_feedback_effect_wrappers_smoke(void);
extern "C" int zinput_force_feedback_create_effect_smoke(void);
extern "C" int zinput_force_feedback_effect_set_smoke(void);
extern "C" int zinput_force_feedback_directional_runtime_smoke(void);
extern "C" int zinput_bindgroup_accessors_smoke(void);
extern "C" int zinput_keyboard_dik_ascii_smoke(void);
extern "C" int zinput_mouse_client_size_center_smoke(void);
extern "C" int zinput_mouse_coop_level_flags_smoke(void);
extern "C" int zinput_mouse_keyboard_small_accessors_smoke(void);
extern "C" int zinput_bindmap_name_tables_smoke(void);
extern "C" int zinput_bindmap_context_smoke(void);
extern "C" int zinput_keyboard_clear_callbacks_smoke(void);
extern "C" int zinput_keyboard_mouse_addref_smoke(void);
extern "C" int zinput_keyboard_init_device_smoke(void);
extern "C" int zinput_joystick_init_device_smoke(void);
extern "C" int zinput_joystick_acquire_device_smoke(void);
extern "C" int zinput_joystick_ref_and_enable_smoke(void);
extern "C" int zinput_joystick_poll_and_wait_smoke(void);
extern "C" int zinput_mouse_shutdown_device_smoke(void);
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
extern "C" int znetwork_local_identity_smoke(void);
extern "C" int znetwork_destroy_cached_local_player_smoke(void);
extern "C" int znetwork_dplay_report_error_smoke(void);
extern "C" int znetwork_dispatch_handler_list_smoke(void);
extern "C" int znetwork_player_record_accessors_smoke(void);
extern "C" int znetwork_packet_send_wrappers_smoke(void);
extern "C" int znetwork_session_status_fields_smoke(void);
extern "C" int znetwork_shutdown_session_runtime_smoke(void);
extern "C" int zreader_named_int_lookup_smoke(void);
extern "C" int zreader_named_string_float_lookup_smoke(void);
extern "C" int zreader_load_node_from_archive_smoke(void);
extern "C" int zreader_file_exists_and_list_create_smoke(void);
extern "C" int zreader_archive_list_and_search_paths_smoke(void);
extern "C" int zreader_prealloc_and_pop_front_smoke(void);
extern "C" int zreader_zrdr_init_search_path_smoke(void);
extern "C" int zreader_mount_index_archive_smoke(void);
extern "C" int zreader_free_loaded_tree_smoke(void);
extern "C" int zreader_load_movers_from_zrd_smoke(void);
extern "C" int zreader_resolve_and_open_file_smoke(void);
extern "C" int zmath_matrix_stack_and_direction_smoke(void);
extern "C" int zmath_projection_batches_smoke(void);
extern "C" int zmath_perspective_texture_interpolants_smoke(void);
extern "C" int zmath_vec3_normalize_and_div_scalar_smoke(void);
extern "C" int zmath_array_add_scaled_and_transform_smoke(void);
extern "C" int zmath_load_view_smoke(void);
extern "C" int zmath_quaternion_helpers_smoke(void);
extern "C" int recoil_app_get_message_map_smoke(void);
extern "C" int recoil_app_accessor_and_skip_wait_smoke(void);
extern "C" int recoil_app_activate_existing_instance_absent_smoke(void);
extern "C" int recoil_app_get_current_state_smoke(void);
extern "C" int recoil_app_state_queue_block_init_from_cursor_smoke(void);
extern "C" int recoil_app_state_queue_grow_chunk_base_list_smoke(void);
extern "C" int recoil_app_queue_switch_current_state_smoke(void);
extern "C" int recoil_app_queue_exit_current_state_smoke(void);
extern "C" int recoil_app_start_engine_and_queue_startup_state_smoke(void);
extern "C" int recoil_app_initialize_display_failure_smoke(void);
extern "C" int recoil_app_on_idle_or_dispatch_smoke(void);
extern "C" int recoil_app_mfc_ole_module_constructor_smoke(void);
extern "C" int recoil_app_mfc_ole_module_destructor_smoke(void);
extern "C" int recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_app_play_state_constructor_smoke(void);
extern "C" int recoil_app_fmv_state_constructor_smoke(void);
extern "C" int recoil_app_constructor_destructor_smoke(void);
extern "C" int recoil_app_istate_destructor_smoke(void);
extern "C" int recoil_app_fmv_state_destructor_smoke(void);
extern "C" int recoil_app_mission_fmv_state_destructor_smoke(void);
extern "C" int recoil_app_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_state_base_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_constructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_destructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_clear_paused_audio_snapshot_smoke(void);
extern "C" int recoil_state_main_menu_transition_queue_enter_smoke(void);
extern "C" int recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke(void);
extern "C" int hud_ui_main_menu_dialog_constructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_on_try_become_current_smoke(void);
extern "C" int zsnd_set_use_archive_banks_flag_smoke(void);
extern "C" int zsnd_sample_set_registry_init_shutdown_smoke(void);
extern "C" int zsnd_sample_set_registry_lookup_destroy_smoke(void);
extern "C" int zsnd_option_accessors_smoke(void);
extern "C" int zsnd_speed_of_sound_smoke(void);
extern "C" int zsnd_backend_error_reporters_smoke(void);
extern "C" int zsnd_cached_directsound_get_caps_smoke(void);
extern "C" int zsnd_cd_reset_track_state_smoke(void);
extern "C" int zsnd_cd_is_stereo_aux_enabled_smoke(void);
extern "C" int zsnd_cd_not_ready_playback_smoke(void);
extern "C" int zsnd_cd_init_ready_guard_smoke(void);
extern "C" int zsnd_cd_get_track_count_ready_guard_smoke(void);
extern "C" int zsnd_cd_shutdown_track_list_smoke(void);
extern "C" int zsnd_sample_set_get_sample_at_smoke(void);
extern "C" int zsnd_sample_set_registry_add_entry_smoke(void);
extern "C" int zsnd_find_sample_by_name_smoke(void);
extern "C" int zsnd_gain_scale_to_directsound_attenuation_smoke(void);
extern "C" int zsnd_is_muted_smoke(void);
extern "C" int zsnd_sample_play_simple_smoke(void);
extern "C" int zsnd_sample_acquire_play_handle_smoke(void);
extern "C" int zsnd_system_named_sets_syntax_smoke(void);
extern "C" int zsnd_system_legacy_sets_syntax_smoke(void);
extern "C" int zsnd_system_init_gate_and_missing_config_smoke(void);
extern "C" int zsnd_preinitialize_runtime_state_smoke(void);
extern "C" int zsnd_group_load_config_block_smoke(void);
extern "C" int zsnd_group_load_and_queue_smoke(void);
extern "C" int zsnd_stream_mgr_shutdown_lists_smoke(void);
extern "C" int zsnd_backend_shutdown_release_smoke(void);
extern "C" int zsnd_play_handle_stop_if_active_smoke(void);
extern "C" int zsnd_snapshot_create_from_active_samples_smoke(void);
extern "C" int zsnd_snapshot_restore_all_with_global_volume_delta_smoke(void);
extern "C" int zsnd_snapshot_destroy_smoke(void);
extern "C" int zsnd_snapshot_stop_all_if_playing_smoke(void);
extern "C" int zsnd_play_handle_update3d_a3d_smoke(void);
extern "C" int zsnd_update_listener_state_smoke(void);
extern "C" int zsnd_play_handle_update3d_directsound_smoke(void);
extern "C" int zsnd_sample_play_a3d_simple_direct_smoke(void);
extern "C" int zsnd_sample_destroy_owned_data_smoke(void);
extern "C" int zsnd_fade_entry_backend_and_dispatch_smoke(void);
extern "C" int zsnd_fade_active_list_tick_compacts_smoke(void);
extern "C" int zsnd_fade_list_cursor_helpers_smoke(void);
extern "C" int zsnd_fade_lists_stop_all_shutdown_smoke(void);
extern "C" int zsnd_tick_backend_markers_smoke(void);
extern "C" int zsnd_sample_set_init_by_name_empty_smoke(void);
extern "C" int zsnd_wave_data_load_parse_reset_smoke(void);
extern "C" int zsnd_wave_data_archive_load_smoke(void);
extern "C" int zsnd_sample_backend_buffer_lock_unlock_smoke(void);
extern "C" int zsnd_sample_init_from_wave_data_directsound_smoke(void);
extern "C" int zsnd_sample_init_from_wave_data_a3d_smoke(void);
extern "C" int zrndr_get_active_region_state_smoke(void);
extern "C" int zrndr_framebuffer_and_stride_cache_smoke(void);
extern "C" int zrndr_init_globals_smoke(void);
extern "C" int zrndr_immediate_line_dispatch_smoke(void);
extern "C" int zrndr_lens_flare_queue_projected_sample_smoke(void);
extern "C" int zrndr_lens_flare_draw_sample_smoke(void);
extern "C" int zrndr_lens_flare_stage_helpers_smoke(void);
extern "C" int zrndr_span_occlusion_filter_sample_list_smoke(void);
extern "C" int zrndr_lens_flare_draw_sample_stage_clipped_smoke(void);
extern "C" int zrndr_lens_flare_draw_visible_sample_stages_smoke(void);
extern "C" int zrndr_lens_flare_draw_visible_sample_smoke(void);
extern "C" int zrndr_lens_flare_draw_visible_samples_smoke(void);
extern "C" int zrndr_span_occlusion_add_polygon_smoke(void);
extern "C" int zrndr_span_occlusion_test_depth_order_pair_smoke(void);
extern "C" int zrndr_span_occlusion_insert_no_depth_smoke(void);
extern "C" int zrndr_span_occlusion_build_span_list_smoke(void);
extern "C" int zrndr_span_occlusion_insert_local_smoke(void);
extern "C" int zrndr_span_occlusion_build_span_list_fast_smoke(void);
extern "C" int zrndr_span_occlusion_test_column_visibility_smoke(void);
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
extern "C" int zrndr_overlay_and_mmx_masks_smoke(void);
extern "C" int zrndr_span_alpha_blend_565_const_alpha_pal8_smoke(void);
extern "C" int zrndr_fill_span16_opaque_smoke(void);
extern "C" int zrndr_fill_span555_solid_smoke(void);
extern "C" int zrndr_fill_span565_solid_smoke(void);
extern "C" int zrndr_span_masked_tex16_to_565_smoke(void);
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
extern "C" int zturret_reset_iteration_state_smoke(void);
extern "C" int zturret_shutdown_leaf_smoke(void);
extern "C" int zutil_zar_register_section_handler_smoke(void);
extern "C" int zutil_zbd_init_smoke(void);
extern "C" int hud_sensor_register_mission_sections_smoke(void);
extern "C" int hud_sensor_mission_identity_smoke(void);
extern "C" int hud_sensor_map_node_basics_smoke(void);
extern "C" int hud_sensor_map_bounds_and_save_state_smoke(void);
extern "C" int hud_sensor_map_remove_and_shutdown_smoke(void);
extern "C" int hud_sensor_reset_mission_state_smoke(void);
extern "C" int zutil_zar_write_section_blob_smoke(void);
extern "C" int zeffect_anim_shutdown_entry_smoke(void);
extern "C" int zeffect_anim_find_entry_by_name_smoke(void);
extern "C" int zeffect_anim_find_node_recursive_by_name_smoke(void);
extern "C" int zeffect_anim_ref_resolution_smoke(void);
extern "C" int zeffect_anim_find_or_create_runtime_refs_smoke(void);
extern "C" int zeffect_anim_rebind_entry_to_node_smoke(void);
extern "C" int zeffect_anim_ensure_copied_root_tree_smoke(void);
extern "C" int zeffect_anim_clone_entry_for_node_smoke(void);
extern "C" int zeffect_cleanup_light_sound_refs_smoke(void);
extern "C" int zeffect_handle_sample_ref_offset_event_smoke(void);
extern "C" int zeffect_handle_sound_light_events_smoke(void);
extern "C" int zeffect_anim_activation_prereqs_smoke(void);
extern "C" int zeffect_anim_activation_record_queue_smoke(void);
extern "C" int zeffect_handle_emitter_reset_event_smoke(void);
extern "C" int zeffect_handle_emitter_loop_event_smoke(void);
extern "C" int zeffect_handle_emitter_stop_event_smoke(void);
extern "C" int zeffect_conditional_ref_pos_smoke(void);
extern "C" int zeffect_skip_conditional_chain_smoke(void);
extern "C" int zeffect_marker_and_callback_event_smoke(void);
extern "C" int zeffect_anim_set_on_state_done_callback_smoke(void);
extern "C" int zeffect_world_and_zbd_setters_smoke(void);
extern "C" int zeffect_anim_find_next_async_entry_smoke(void);
extern "C" int zeffect_anim_get_root_node_or_null_smoke(void);
extern "C" int zeffect_anim_capture_node_states_smoke(void);
extern "C" int zeffect_anim_restore_node_states_smoke(void);
extern "C" int zeffect_anim_reset_for_node_smoke(void);
extern "C" int zeffect_anim_init_shutdown_smoke(void);
extern "C" int zeffect_find_template_index_by_name_smoke(void);
extern "C" int zeffect_init_smoke(void);
extern "C" int zdeclient_set_camera_node_smoke(void);
extern "C" int zdeclient_load_material_from_texture_path_smoke(void);
extern "C" int zdeclient_shutdown_globals_smoke(void);
extern "C" int zloc_message_lookup_failure_smoke(void);
extern "C" int zloc_load_unload_messages_dll_smoke(void);
extern "C" int zvid_pack_color_rgb_smoke(void);
extern "C" int zvideo_pixel_pack_setup_smoke(void);
extern "C" int zvideo_pixel_pack_getters_smoke(void);
extern "C" int zvideo_set_renderer_type_smoke(void);
extern "C" int zvideo_pending_dither_enable_smoke(void);
extern "C" int zvideo_surface_accessors_smoke(void);
extern "C" int zvideo_primary_surface_rect_scratch_smoke(void);
extern "C" int zvideo_frame_scratch_buffers_smoke(void);
extern "C" int zvideo_clear_dispatch_and_exchange_smoke(void);
extern "C" int zvideo_mode_geometry_and_set_video_mode_smoke(void);
extern "C" int zvideo_init_video_system_reentry_guard_smoke(void);
extern "C" int zvideo_dispatch_wrappers_smoke(void);
extern "C" int zvideo_bind_renderer_dispatch_smoke(void);
extern "C" int zvideo_adjust_surfaces_if_enabled_smoke(void);
extern "C" int zvideo_set_half_res_adjust_mode_smoke(void);
extern "C" int zvideo_dd_report_error_smoke(void);
extern "C" int zvid_hw_api_accessors_smoke(void);
extern "C" int zvid_cached_renderer_and_texture_counts_smoke(void);
extern "C" int zvid_option_accessors_smoke(void);
extern "C" int zvid_set_video_mode_index_smoke(void);
extern "C" int zvideo_buff_clip_coord_to_range_smoke(void);
extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void);
extern "C" int zvideo_surface_state_lock_skip_smoke(void);
extern "C" int zvideo_capture_surface_to_image_smoke(void);
extern "C" int zvideo_image_lazy_create_backing_surface_guards_smoke(void);
extern "C" int zvideo_blt_sw_to_primary_rect_lazy_failure_smoke(void);
extern "C" int zvideo_flip_to_gdi_if_attached_null_smoke(void);
extern "C" int zvideo_clear_rect_skip_paths_smoke(void);
extern "C" int zvideo_palette_set_entries_non8bpp_smoke(void);
extern "C" int zvideo_quad_batch_depth_and_rhw_smoke(void);
extern "C" int zvideo_queue_solid_quad_smoke(void);
extern "C" int zvideo_flush_quad_batch_empty_smoke(void);
extern "C" int zvideo_flush_sorted_polys_empty_smoke(void);
extern "C" int zvideo_submit_poly_flat_color16_queue_smoke(void);
extern "C" int zvideo_submit_poly_gouraud_color16_queue_smoke(void);
extern "C" int zvideo_submit_poly_color_attr_smoke(void);
extern "C" int zvideo_submit_poly_render_class_queue_smoke(void);
extern "C" int zvideo_submit_polygon_queue_smoke(void);
extern "C" int zvideo_submit_polygon_lit_queue_smoke(void);
extern "C" int zvideo_present_display_mode_surface_null_smoke(void);
extern "C" int zvideo_texture_record_release_upload_null_smoke(void);
extern "C" int zvideo_texture_record_create_and_power_smoke(void);
extern "C" int zvideo_convert_image_pixels_for_texture_smoke(void);
extern "C" int zvideo_image_alpha_clear_smoke(void);
extern "C" int zvideo_image_set_pixels_smoke(void);
extern "C" int zvideo_image_file_read_helpers_smoke(void);
extern "C" int zvideo_palette_remap_no_recipes_smoke(void);
extern "C" int zvideo_texture_pack_load_image_smoke(void);
extern "C" int zimage_font_glyph_scan_smoke(void);
extern "C" int zimage_font_measure_string_smoke(void);
extern "C" int zimage_texdir_find_or_create_missing_smoke(void);
extern "C" int zimage_texdir_build_mip_chain_smoke(void);
extern "C" int zvid_texture_pack_ensure_builtin_smoke(void);
extern "C" int zimage_texdir_load_pending_entries_smoke(void);
extern "C" int zimage_texdir_base_name_path_smoke(void);
extern "C" int zimage_texdir_variant_image_smoke(void);
extern "C" int zimage_texdir_find_by_name_smoke(void);
extern "C" int zimage_texdir_write_smoke(void);
extern "C" int zimage_init_option_fallback_smoke(void);
extern "C" int zimage_shutdown_texdir_smoke(void);
extern "C" int zvid_image_resample_square_smoke(void);
extern "C" int zvid_image_calc_pow2_scratch_fields_smoke(void);
extern "C" int zvid_image_release_owned_buffers_smoke(void);
extern "C" int zvid_image_destroy_smoke(void);
extern "C" int zvideo_create_texture_record_guards_smoke(void);
extern "C" int zvideo_image_surface_helpers_guard_smoke(void);
extern "C" int zvideo_restore_display_surfaces_null_smoke(void);
extern "C" int zvideo_select_hw_api_device_smoke(void);
extern "C" int zvideo_return_success_stub_smoke(void);
extern "C" int zvid_cached_client_rect_smoke(void);
extern "C" int zweapon_init_smoke(void);
extern "C" int zweapon_optcatalog_shutdown_smoke(void);
extern "C" int zweapon_optcatalog_find_entry_by_id_smoke(void);
extern "C" int zweapon_optcatalog_pending_spawn_override_smoke(void);
extern "C" int zweapon_optcatalog_runtime_free_list_helpers_smoke(void);
extern "C" int light_alloc_from_free_list_and_attach_smoke(void);
extern "C" int zweapon_optcatalog_warning_samples_smoke(void);
extern "C" int zvideo_restore_iconic_fullscreen_window_smoke(void);
extern "C" int zvideo_shutdown_video_system_smoke(void);
extern "C" int zsys_find_file_on_drive_type_negative_smoke(void);
extern "C" int zsys_runtime_probe_leaves_smoke(void);
extern "C" int zsys_cpuid_mmx_smoke(void);
extern "C" int zerror_init_output_context_smoke(void);
extern "C" int zcom_query_interface_from_interface_map_smoke(void);
extern "C" int zcom_connection_point_container_advise_smoke(void);
extern "C" int zcom_connection_point_container_unadvise_smoke(void);
extern "C" int pickup_type_table_free_opt_meta_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_shutdown_global_smoke(void);
extern "C" int pickup_spawn_list_clear_smoke(void);
extern "C" int pickup_respawn_queue_clear_smoke(void);
extern "C" int pickup_shutdown_smoke(void);
extern "C" int pickup_leaf_helpers_smoke(void);
extern "C" int time_reset_smoke(void);

#include <cstdio>
#include <cstring>

namespace {
struct SmokeTest {
    const char *name;
    int (*run)();
};

int RunSmokeTests(const SmokeTest *tests, int count, const char *onlyName) {
    int failures = 0;
    for (int i = 0; i < count; ++i) {
        if (onlyName != nullptr && std::strcmp(tests[i].name, onlyName) != 0) {
            continue;
        }

        const int result = tests[i].run();
        if (result != 0) {
            std::printf("%s failed: %d\n", tests[i].name, result);
            failures += result;
        }
    }
    return failures;
}
} // namespace

int main(int argc, char **argv) {
    const SmokeTest tests[] = {
        {"recoil_native_build_anchor", recoil_native_build_anchor},
        {"recoil_mfc42_provider_smoke", recoil_mfc42_provider_smoke},
        {"czrecoil_frame_build_window_title_smoke", czrecoil_frame_build_window_title_smoke},
        {"czframe_metadata_accessors_smoke", czframe_metadata_accessors_smoke},
        {"czrecoil_frame_set_menu_bar_visibility_smoke",
         czrecoil_frame_set_menu_bar_visibility_smoke},
        {"czrecoil_frame_configure_mode_feature_flags_smoke",
         czrecoil_frame_configure_mode_feature_flags_smoke},
        {"czrecoil_frame_set_hw_api_and_init_mode_smoke",
         czrecoil_frame_set_hw_api_and_init_mode_smoke},
        {"czrecoil_frame_init_fallback_mode_smoke", czrecoil_frame_init_fallback_mode_smoke},
        {"czrecoil_frame_ensure_hw_api_initialized_smoke",
         czrecoil_frame_ensure_hw_api_initialized_smoke},
        {"czrecoil_frame_init_startup_hw_api_from_options_smoke",
         czrecoil_frame_init_startup_hw_api_from_options_smoke},
        {"czrecoil_frame_menu_toggle_smoke", czrecoil_frame_menu_toggle_smoke},
        {"czrecoil_frame_menu_exit_game_smoke", czrecoil_frame_menu_exit_game_smoke},
        {"czrecoil_frame_update_video_mode_cmd_ui_smoke",
         czrecoil_frame_update_video_mode_cmd_ui_smoke},
        {"czrecoil_frame_hw_api_menu_cmd_ui_smoke", czrecoil_frame_hw_api_menu_cmd_ui_smoke},
        {"czrecoil_frame_audio_input_menu_smoke", czrecoil_frame_audio_input_menu_smoke},
        {"czgame_frame_is_window_valid_smoke", czgame_frame_is_window_valid_smoke},
        {"czgame_frame_build_window_title_smoke", czgame_frame_build_window_title_smoke},
        {"czgame_frame_constructor_smoke", czgame_frame_constructor_smoke},
        {"czrecoil_frame_destructor_smoke", czrecoil_frame_destructor_smoke},
        {"czgame_frame_on_app_idle_dispatch_message_smoke",
         czgame_frame_on_app_idle_dispatch_message_smoke},
        {"briefing_stop_and_shutdown_thread_smoke", briefing_stop_and_shutdown_thread_smoke},
        {"gamenet_list_reset_smoke", gamenet_list_reset_smoke},
        {"hud_timer_panel_net_state_clear_tail_flags_smoke",
         hud_timer_panel_net_state_clear_tail_flags_smoke},
        {"gamenet_find_player_row_and_status_bits_smoke",
         gamenet_find_player_row_and_status_bits_smoke},
        {"gamenet_reset_hud_timer_panel_net_state_smoke",
         gamenet_reset_hud_timer_panel_net_state_smoke},
        {"gamenet_host_update_session_status_fields_smoke",
         gamenet_host_update_session_status_fields_smoke},
        {"gamenet_timer_status_packet_smoke", gamenet_timer_status_packet_smoke},
        {"gamenet_scoreboard_snapshot_packet_smoke", gamenet_scoreboard_snapshot_packet_smoke},
        {"gamenet_chat_message_packet_smoke", gamenet_chat_message_packet_smoke},
        {"gamenet_show_player_kill_message_smoke", gamenet_show_player_kill_message_smoke},
        {"gamenet_player_kill_event_packet_smoke", gamenet_player_kill_event_packet_smoke},
        {"gamenet_reassign_player_colors_smoke", gamenet_reassign_player_colors_smoke},
        {"gamenet_player_row_apply_color_tint_smoke", gamenet_player_row_apply_color_tint_smoke},
        {"gamenet_player_row_destroy_embedded_panel_smoke",
         gamenet_player_row_destroy_embedded_panel_smoke},
        {"ainet_free_all_smoke", ainet_free_all_smoke},
        {"player_ai_discard_negative_branch_nodes_smoke",
         player_ai_discard_negative_branch_nodes_smoke},
        {"player_find_alt_gun_controller_smoke", player_find_alt_gun_controller_smoke},
        {"player_alt_gun_fire_point_selection_smoke", player_alt_gun_fire_point_selection_smoke},
        {"zutil_save_game_state_free_owned_resources_smoke",
         zutil_save_game_state_free_owned_resources_smoke},
        {"player_destroy_save_game_state_smoke", player_destroy_save_game_state_smoke},
        {"player_shutdown_mission_runtime_smoke", player_shutdown_mission_runtime_smoke},
        {"zfmv_script_reset_smoke", zfmv_script_reset_smoke},
        {"zfmv_script_init_null_path_smoke", zfmv_script_init_null_path_smoke},
        {"zfmv_script_cleanup_smoke", zfmv_script_cleanup_smoke},
        {"zfmv_script_append_action_smoke", zfmv_script_append_action_smoke},
        {"zfmv_script_run_blocking_empty_smoke", zfmv_script_run_blocking_empty_smoke},
        {"zfmv_action_image_constructor_with_screen_rect_smoke",
         zfmv_action_image_constructor_with_screen_rect_smoke},
        {"zfmv_action_image_constructor_scaled_smoke", zfmv_action_image_constructor_scaled_smoke},
        {"zfmv_action_fade_constructor_smoke", zfmv_action_fade_constructor_smoke},
        {"zfmv_playback_init_smoke", zfmv_playback_init_smoke},
        {"zfmv_playback_set_dest_rect_smoke", zfmv_playback_set_dest_rect_smoke},
        {"zfmv_playback_destructor_smoke", zfmv_playback_destructor_smoke},
        {"zfmv_playback_report_mci_error_smoke", zfmv_playback_report_mci_error_smoke},
        {"zfmv_stream_destructor_empty_smoke", zfmv_stream_destructor_empty_smoke},
        {"zfmv_stream_constructor_missing_file_smoke", zfmv_stream_constructor_missing_file_smoke},
        {"zfmv_stream_init_missing_file_smoke", zfmv_stream_init_missing_file_smoke},
        {"zfmv_action_play_avi_constructor_existing_file_smoke",
         zfmv_action_play_avi_constructor_existing_file_smoke},
        {"zfmv_action_play_mci_constructor_smoke", zfmv_action_play_mci_constructor_smoke},
        {"zfmv_action_blur_constructor_smoke", zfmv_action_blur_constructor_smoke},
        {"zfmv_script_load_actions_from_zrd_smoke", zfmv_script_load_actions_from_zrd_smoke},
        {"zfmv_action_wait_begin_update_smoke", zfmv_action_wait_begin_update_smoke},
        {"zfmv_action_base_destructor_smoke", zfmv_action_base_destructor_smoke},
        {"zfmv_action_derived_scalar_deleting_destructor_smoke",
         zfmv_action_derived_scalar_deleting_destructor_smoke},
        {"zfmv_action_play_sound_begin_missing_sample_smoke",
         zfmv_action_play_sound_begin_missing_sample_smoke},
        {"zgame_return_only_stub_smoke", zgame_return_only_stub_smoke},
        {"zgame_options_init_registry_context_smoke", zgame_options_init_registry_context_smoke},
        {"zmodel_display_init_smoke", zmodel_display_init_smoke},
        {"zmodel_damage_mask_uv_smoke", zmodel_damage_mask_uv_smoke},
        {"zmodel_display_shutdown_smoke", zmodel_display_shutdown_smoke},
        {"zmodel_material_pool_entry_smoke", zmodel_material_pool_entry_smoke},
        {"zmodel_material_defaults_and_find_smoke", zmodel_material_defaults_and_find_smoke},
        {"zmodel_material_write_gamez_smoke", zmodel_material_write_gamez_smoke},
        {"zmodel_material_read_gamez_smoke", zmodel_material_read_gamez_smoke},
        {"zmodel_dipool_write_to_stream_smoke", zmodel_dipool_write_to_stream_smoke},
        {"zmodel_dipool_read_from_stream_smoke", zmodel_dipool_read_from_stream_smoke},
        {"zmodel_dipool_read_entry_by_index_from_stream_smoke",
         zmodel_dipool_read_entry_by_index_from_stream_smoke},
        {"zmodel_material_and_di_clone_smoke", zmodel_material_and_di_clone_smoke},
        {"zmodel_scrolling_texture_update_smoke", zmodel_scrolling_texture_update_smoke},
        {"zmodel_render_point_queue_entry_smoke", zmodel_render_point_queue_entry_smoke},
        {"zmodel_light_fog_fade_smoke", zmodel_light_fog_fade_smoke},
        {"zmodel_light_build_light_weights_smoke", zmodel_light_build_light_weights_smoke},
        {"zmodel_light_point_in_polygon_init_smoke", zmodel_light_point_in_polygon_init_smoke},
        {"zmodel_light_build_attr0_depth_fade_smoke", zmodel_light_build_attr0_depth_fade_smoke},
        {"zclass_world_apply_pending_fog_settings_smoke",
         zclass_world_apply_pending_fog_settings_smoke},
        {"zclass_shutdown_core_smoke", zclass_shutdown_core_smoke},
        {"zclass_gwnode_update_smoke", zclass_gwnode_update_smoke},
        {"zgame_options_find_option_smoke", zgame_options_find_option_smoke},
        {"zopt_eval_int_compare_op_smoke", zopt_eval_int_compare_op_smoke},
        {"zopt_fullscreen_accessors_smoke", zopt_fullscreen_accessors_smoke},
        {"zopt_network_enabled_accessor_smoke", zopt_network_enabled_accessor_smoke},
        {"hud_sensor_objective_slot_reset_smoke", hud_sensor_objective_slot_reset_smoke},
        {"hud_sensor_tracker_unload_objectives_smoke", hud_sensor_tracker_unload_objectives_smoke},
        {"zopt_view_rect_target_side_effects_smoke", zopt_view_rect_target_side_effects_smoke},
        {"zopt_section_accessor_smoke", zopt_section_accessor_smoke},
        {"zclipalt_set_source_rect_smoke", zclipalt_set_source_rect_smoke},
        {"zclipalt_set_target_rect_smoke", zclipalt_set_target_rect_smoke},
        {"zclass_cls_di_set_stop_after_first_hit_smoke",
         zclass_cls_di_set_stop_after_first_hit_smoke},
        {"zhud_element_constructor_smoke", zhud_element_constructor_smoke},
        {"zhud_element_copy_constructor_smoke", zhud_element_copy_constructor_smoke},
        {"zhud_element_set_timer_smoke", zhud_element_set_timer_smoke},
        {"zhud_circle_constructor_and_hit_test_smoke", zhud_circle_constructor_and_hit_test_smoke},
        {"zhud_composite_panel_vector_clear_smoke", zhud_composite_panel_vector_clear_smoke},
        {"zhud_composite_panel_entry_copy_smoke", zhud_composite_panel_entry_copy_smoke},
        {"zhud_flash_panel_compute_blend_color_smoke", zhud_flash_panel_compute_blend_color_smoke},
        {"zhud_layout_shutdown_stub_smoke", zhud_layout_shutdown_stub_smoke},
        {"zhud_layout_hw_release_images_smoke", zhud_layout_hw_release_images_smoke},
        {"zhud_element_clip_and_invalidate_smoke", zhud_element_clip_and_invalidate_smoke},
        {"zhud_element_visible_smoke", zhud_element_visible_smoke},
        {"zhud_element_update_smoke", zhud_element_update_smoke},
        {"zhud_element_position_mutators_smoke", zhud_element_position_mutators_smoke},
        {"zhud_container_child_list_smoke", zhud_container_child_list_smoke},
        {"zhud_container_destructor_core_smoke", zhud_container_destructor_core_smoke},
        {"zhud_widget_constructor_smoke", zhud_widget_constructor_smoke},
        {"zhud_widget_default_ctor_thunk_smoke", zhud_widget_default_ctor_thunk_smoke},
        {"zhud_zrd_widget_constructor_smoke", zhud_zrd_widget_constructor_smoke},
        {"zhud_zrd_widget_helpers_smoke", zhud_zrd_widget_helpers_smoke},
        {"zhud_zrd_widget_load_from_zrd_smoke", zhud_zrd_widget_load_from_zrd_smoke},
        {"zhud_check_toggle_widget_helpers_smoke", zhud_check_toggle_widget_helpers_smoke},
        {"zhud_check_toggle_widget_load_from_zrd_smoke",
         zhud_check_toggle_widget_load_from_zrd_smoke},
        {"zhud_cycle_selector_widget_constructor_smoke",
         zhud_cycle_selector_widget_constructor_smoke},
        {"zhud_cycle_selector_widget_load_from_zrd_smoke",
         zhud_cycle_selector_widget_load_from_zrd_smoke},
        {"zhud_fill_bitmap_core_smoke", zhud_fill_bitmap_core_smoke},
        {"zhud_zrd_widget_ex17c_item_core_smoke", zhud_zrd_widget_ex17c_item_core_smoke},
        {"zhud_cmd_bind_button_base_constructor_smoke",
         zhud_cmd_bind_button_base_constructor_smoke},
        {"zhud_message_box_leaf_handlers_smoke", zhud_message_box_leaf_handlers_smoke},
        {"zhud_background_container_focus_smoke", zhud_background_container_focus_smoke},
        {"zhud_background_cursor_widget_member_constructor_smoke",
         zhud_background_cursor_widget_member_constructor_smoke},
        {"zhud_background_constructor_smoke", zhud_background_constructor_smoke},
        {"zhud_background_bind_widget_by_name_smoke", zhud_background_bind_widget_by_name_smoke},
        {"zhud_background_free_loaded_tree_roots_smoke",
         zhud_background_free_loaded_tree_roots_smoke},
        {"zhud_background_set_enabled_smoke", zhud_background_set_enabled_smoke},
        {"zhud_dialog_controller_blit_owned_surface_smoke",
         zhud_dialog_controller_blit_owned_surface_smoke},
        {"zhud_font_style_constructor_smoke", zhud_font_style_constructor_smoke},
        {"zhud_numeric_text_input_base_constructor_smoke",
         zhud_numeric_text_input_base_constructor_smoke},
        {"zhud_widget_release_and_destructor_core_smoke",
         zhud_widget_release_and_destructor_core_smoke},
        {"zhud_triplet_panel_constructor_smoke", zhud_triplet_panel_constructor_smoke},
        {"zhud_triplet_panel_set_visible_count_smoke", zhud_triplet_panel_set_visible_count_smoke},
        {"zhud_triplet_panel_shutdown_items_stub_smoke",
         zhud_triplet_panel_shutdown_items_stub_smoke},
        {"zhud_triplet_panel_destructor_core_smoke", zhud_triplet_panel_destructor_core_smoke},
        {"zhud_text_input_destructor_core_smoke", zhud_text_input_destructor_core_smoke},
        {"zhud_text_input_constructor_and_alloc_smoke",
         zhud_text_input_constructor_and_alloc_smoke},
        {"zhud_mgr_objective_block_destructor_smoke", zhud_mgr_objective_block_destructor_smoke},
        {"hud_ui_net_exit_destroy_global_smoke", hud_ui_net_exit_destroy_global_smoke},
        {"hud_ui_net_exit_show_tick_smoke", hud_ui_net_exit_show_tick_smoke},
        {"hud_ui_aux_overlay_text_lines_smoke", hud_ui_aux_overlay_text_lines_smoke},
        {"zhud_mgr_destroy_sensor_window_null_smoke", zhud_mgr_destroy_sensor_window_null_smoke},
        {"zhud_mgr_disable_hud_smoke", zhud_mgr_disable_hud_smoke},
        {"zhud_slot_destructors_smoke", zhud_slot_destructors_smoke},
        {"zhud_slot_draw_smoke", zhud_slot_draw_smoke},
        {"zhud_stats_list_element_update_smoke", zhud_stats_list_element_update_smoke},
        {"zhud_counter_constructor_smoke", zhud_counter_constructor_smoke},
        {"zhud_counter_release_state_images_smoke", zhud_counter_release_state_images_smoke},
        {"zhud_message_release_images_smoke", zhud_message_release_images_smoke},
        {"zhud_message_constructor_smoke", zhud_message_constructor_smoke},
        {"zhud_message_destructors_smoke", zhud_message_destructors_smoke},
        {"zhud_shield_message_widget_destructor_smoke",
         zhud_shield_message_widget_destructor_smoke},
        {"zhud_bar_and_meter_constructor_smoke", zhud_bar_and_meter_constructor_smoke},
        {"zhud_polyline_and_slider_border_constructor_smoke",
         zhud_polyline_and_slider_border_constructor_smoke},
        {"zhud_polyline_draw_and_slider_update_smoke", zhud_polyline_draw_and_slider_update_smoke},
        {"zhud_panel_scalar_deleting_destructor_smoke",
         zhud_panel_scalar_deleting_destructor_smoke},
        {"zhud_panel_text_color_shadow_smoke", zhud_panel_text_color_shadow_smoke},
        {"zhud_panel_constructor_default_smoke", zhud_panel_constructor_default_smoke},
        {"zhud_panel_copy_construct_core_smoke", zhud_panel_copy_construct_core_smoke},
        {"zhud_text_label_constructor_and_extents_smoke",
         zhud_text_label_constructor_and_extents_smoke},
        {"zhud_panel_set_font_smoke", zhud_panel_set_font_smoke},
        {"zhud_panel_set_text_fmt_smoke", zhud_panel_set_text_fmt_smoke},
        {"zhud_panel_measure_text_prefix_rect_smoke", zhud_panel_measure_text_prefix_rect_smoke},
        {"zhud_panel_query_text_height_smoke", zhud_panel_query_text_height_smoke},
        {"zhud_panel_simple_constructor_smoke", zhud_panel_simple_constructor_smoke},
        {"zhud_timer_panel_set_time_smoke", zhud_timer_panel_set_time_smoke},
        {"zhud_timer_panel_update_hms_smoke", zhud_timer_panel_update_hms_smoke},
        {"zhud_timer_panel_global_accessors_smoke", zhud_timer_panel_global_accessors_smoke},
        {"zhud_timer_panel_update_smoke", zhud_timer_panel_update_smoke},
        {"zhud_timer_panel_zar_read_smoke", zhud_timer_panel_zar_read_smoke},
        {"zhud_timer_panel_zar_write_smoke", zhud_timer_panel_zar_write_smoke},
        {"zhud_timer_and_counter_constructor_smoke", zhud_timer_and_counter_constructor_smoke},
        {"zhud_timer_panel_float_constructor_smoke", zhud_timer_panel_float_constructor_smoke},
        {"zhud_triplet_constructor_smoke", zhud_triplet_constructor_smoke},
        {"zhud_triplet_destructor_core_smoke", zhud_triplet_destructor_core_smoke},
        {"zhud_triplet_scoreboard_entry_update_smoke", zhud_triplet_scoreboard_entry_update_smoke},
        {"zhud_stats_list_destructors_smoke", zhud_stats_list_destructors_smoke},
        {"zhud_string_menu_destructor_core_smoke", zhud_string_menu_destructor_core_smoke},
        {"zhud_text_stack_constructors_smoke", zhud_text_stack_constructors_smoke},
        {"zhud_text_stack_push_line_smoke", zhud_text_stack_push_line_smoke},
        {"zhud_text_stack_destructor_core_smoke", zhud_text_stack_destructor_core_smoke},
        {"zhud_sensor_viewport_rect_smoke", zhud_sensor_viewport_rect_smoke},
        {"zhud_objective_update_smoke", zhud_objective_update_smoke},
        {"zhud_objective_begin_smoke", zhud_objective_begin_smoke},
        {"zhud_mgr_viewport_activation_smoke", zhud_mgr_viewport_activation_smoke},
        {"zhud_mgr_update_target_reticle_smoke", zhud_mgr_update_target_reticle_smoke},
        {"zhud_mgr_init_layouts_reentry_smoke", zhud_mgr_init_layouts_reentry_smoke},
        {"zhud_mgr_shutdown_resources_smoke", zhud_mgr_shutdown_resources_smoke},
        {"zclass_node_propagate_transform_dirty_smoke",
         zclass_node_propagate_transform_dirty_smoke},
        {"player_add_scaled_hud_counter_smoke", player_add_scaled_hud_counter_smoke},
        {"zutil_save_game_state_list_smoke", zutil_save_game_state_list_smoke},
        {"zclass_object3d_visible_and_color_smoke", zclass_object3d_visible_and_color_smoke},
        {"zclass_object3d_alpha_scale_and_lit_smoke", zclass_object3d_alpha_scale_and_lit_smoke},
        {"zclass_object3d_transform_getters_smoke", zclass_object3d_transform_getters_smoke},
        {"zclass_object3d_transform_setters_smoke", zclass_object3d_transform_setters_smoke},
        {"zclass_object3d_reset_transform_dirty_smoke",
         zclass_object3d_reset_transform_dirty_smoke},
        {"zclass_model_ref_lerp_queue_reset_smoke", zclass_model_ref_lerp_queue_reset_smoke},
        {"zclass_type_list_alloc_and_insert_smoke", zclass_type_list_alloc_and_insert_smoke},
        {"zclass_animate_update_smoke", zclass_animate_update_smoke},
        {"zclass_sequence_new_add_child_smoke", zclass_sequence_new_add_child_smoke},
        {"zclass_object3d_set_matrix_smoke", zclass_object3d_set_matrix_smoke},
        {"zclass_node_free_and_deferred_work_smoke", zclass_node_free_and_deferred_work_smoke},
        {"zclass_alloc_node_from_free_list_smoke", zclass_alloc_node_from_free_list_smoke},
        {"zclass_node_flag16_flag17_smoke", zclass_node_flag16_flag17_smoke},
        {"zclass_node_metadata_accessors_smoke", zclass_node_metadata_accessors_smoke},
        {"zclass_copy_node_display_instance_smoke", zclass_copy_node_display_instance_smoke},
        {"zclass_copy_node_base_data_smoke", zclass_copy_node_base_data_smoke},
        {"zclass_copy_node_unimplemented_stubs_smoke", zclass_copy_node_unimplemented_stubs_smoke},
        {"zclass_copy_object3d_and_lod_smoke", zclass_copy_object3d_and_lod_smoke},
        {"zclass_copy_node_dispatch_and_wrappers_smoke",
         zclass_copy_node_dispatch_and_wrappers_smoke},
        {"zclass_node_action_callback_smoke", zclass_node_action_callback_smoke},
        {"zclass_node_priority_smoke", zclass_node_priority_smoke},
        {"zclass_node_pick_flag_accessors_smoke", zclass_node_pick_flag_accessors_smoke},
        {"zclass_node_extra_flag_setters_smoke", zclass_node_extra_flag_setters_smoke},
        {"zclass_node_vertex_alpha_and_root_smoke", zclass_node_vertex_alpha_and_root_smoke},
        {"zclass_node_world_child_smoke", zclass_node_world_child_smoke},
        {"zclass_gwnode_get_world_position_smoke", zclass_gwnode_get_world_position_smoke},
        {"zclass_world_add_child_at_grid_smoke", zclass_world_add_child_at_grid_smoke},
        {"zclass_world_remove_light_sound_smoke", zclass_world_remove_light_sound_smoke},
        {"zclass_child_generic_link_smoke", zclass_child_generic_link_smoke},
        {"zclass_child_generic_remove_smoke", zclass_child_generic_remove_smoke},
        {"zclass_object3d_child_wrappers_smoke", zclass_object3d_child_wrappers_smoke},
        {"zclass_delete_node_from_lists_smoke", zclass_delete_node_from_lists_smoke},
        {"zclass_find_by_name_and_filtered_iter_smoke",
         zclass_find_by_name_and_filtered_iter_smoke},
        {"zclass_node_predicate_helpers_smoke", zclass_node_predicate_helpers_smoke},
        {"zclass_lifecycle_leaf_smoke", zclass_lifecycle_leaf_smoke},
        {"zclass_zbd_leaf_helpers_smoke", zclass_zbd_leaf_helpers_smoke},
        {"zclass_zbd_node_ref_list_indices_smoke", zclass_zbd_node_ref_list_indices_smoke},
        {"zclass_zbd_single_node_class_data_smoke", zclass_zbd_single_node_class_data_smoke},
        {"zclass_zbd_read_single_node_class_data_smoke",
         zclass_zbd_read_single_node_class_data_smoke},
        {"zclass_zbd_read_node_table_smoke", zclass_zbd_read_node_table_smoke},
        {"zclass_zbd_write_node_table_smoke", zclass_zbd_write_node_table_smoke},
        {"gamez_write_zbd_file_smoke", gamez_write_zbd_file_smoke},
        {"gamez_read_zbd_file_smoke", gamez_read_zbd_file_smoke},
        {"gamez_open_and_read_zbd_header_smoke", gamez_open_and_read_zbd_header_smoke},
        {"gamez_reload_display_instances_smoke", gamez_reload_display_instances_smoke},
        {"zclass_try_free_node_smoke", zclass_try_free_node_smoke},
        {"zdi_ref_and_pool_free_smoke", zdi_ref_and_pool_free_smoke},
        {"zclass_set_display_instance_smoke", zclass_set_display_instance_smoke},
        {"zclass_remove_dispatch_smoke", zclass_remove_dispatch_smoke},
        {"zclass_destroy_node_recursive_display_smoke",
         zclass_destroy_node_recursive_display_smoke},
        {"zclass_object3d_delete_node_smoke", zclass_object3d_delete_node_smoke},
        {"zclass_sound_leaf_smoke", zclass_sound_leaf_smoke},
        {"zclass_object3d_init_smoke", zclass_object3d_init_smoke},
        {"zclass_window_new_smoke", zclass_window_new_smoke},
        {"zclass_display_init_smoke", zclass_display_init_smoke},
        {"zclass_lod_leaf_smoke", zclass_lod_leaf_smoke},
        {"zclass_light_new_smoke", zclass_light_new_smoke},
        {"zclass_damage_handler_smoke", zclass_damage_handler_smoke},
        {"zclass_camera_view_distance_smoke", zclass_camera_view_distance_smoke},
        {"zinput_mouse_update_acquire_state_smoke", zinput_mouse_update_acquire_state_smoke},
        {"zinput_joystick_option_accessors_smoke", zinput_joystick_option_accessors_smoke},
        {"zinput_force_feedback_effect_wrappers_smoke",
         zinput_force_feedback_effect_wrappers_smoke},
        {"zinput_force_feedback_create_effect_smoke", zinput_force_feedback_create_effect_smoke},
        {"zinput_force_feedback_effect_set_smoke", zinput_force_feedback_effect_set_smoke},
        {"zinput_force_feedback_directional_runtime_smoke",
         zinput_force_feedback_directional_runtime_smoke},
        {"zinput_bindgroup_accessors_smoke", zinput_bindgroup_accessors_smoke},
        {"zinput_keyboard_dik_ascii_smoke", zinput_keyboard_dik_ascii_smoke},
        {"zinput_mouse_client_size_center_smoke", zinput_mouse_client_size_center_smoke},
        {"zinput_mouse_coop_level_flags_smoke", zinput_mouse_coop_level_flags_smoke},
        {"zinput_mouse_keyboard_small_accessors_smoke",
         zinput_mouse_keyboard_small_accessors_smoke},
        {"zinput_bindmap_name_tables_smoke", zinput_bindmap_name_tables_smoke},
        {"zinput_bindmap_context_smoke", zinput_bindmap_context_smoke},
        {"zinput_keyboard_clear_callbacks_smoke", zinput_keyboard_clear_callbacks_smoke},
        {"zinput_keyboard_mouse_addref_smoke", zinput_keyboard_mouse_addref_smoke},
        {"zinput_keyboard_init_device_smoke", zinput_keyboard_init_device_smoke},
        {"zinput_joystick_init_device_smoke", zinput_joystick_init_device_smoke},
        {"zinput_joystick_acquire_device_smoke", zinput_joystick_acquire_device_smoke},
        {"zinput_joystick_ref_and_enable_smoke", zinput_joystick_ref_and_enable_smoke},
        {"zinput_joystick_poll_and_wait_smoke", zinput_joystick_poll_and_wait_smoke},
        {"zinput_mouse_shutdown_device_smoke", zinput_mouse_shutdown_device_smoke},
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
        {"znetwork_local_identity_smoke", znetwork_local_identity_smoke},
        {"znetwork_destroy_cached_local_player_smoke", znetwork_destroy_cached_local_player_smoke},
        {"znetwork_dplay_report_error_smoke", znetwork_dplay_report_error_smoke},
        {"znetwork_dispatch_handler_list_smoke", znetwork_dispatch_handler_list_smoke},
        {"znetwork_player_record_accessors_smoke", znetwork_player_record_accessors_smoke},
        {"znetwork_packet_send_wrappers_smoke", znetwork_packet_send_wrappers_smoke},
        {"znetwork_session_status_fields_smoke", znetwork_session_status_fields_smoke},
        {"znetwork_shutdown_session_runtime_smoke", znetwork_shutdown_session_runtime_smoke},
        {"zreader_named_int_lookup_smoke", zreader_named_int_lookup_smoke},
        {"zreader_named_string_float_lookup_smoke", zreader_named_string_float_lookup_smoke},
        {"zreader_load_node_from_archive_smoke", zreader_load_node_from_archive_smoke},
        {"zreader_file_exists_and_list_create_smoke", zreader_file_exists_and_list_create_smoke},
        {"zreader_archive_list_and_search_paths_smoke",
         zreader_archive_list_and_search_paths_smoke},
        {"zreader_prealloc_and_pop_front_smoke", zreader_prealloc_and_pop_front_smoke},
        {"zreader_zrdr_init_search_path_smoke", zreader_zrdr_init_search_path_smoke},
        {"zreader_mount_index_archive_smoke", zreader_mount_index_archive_smoke},
        {"zreader_free_loaded_tree_smoke", zreader_free_loaded_tree_smoke},
        {"zreader_load_movers_from_zrd_smoke", zreader_load_movers_from_zrd_smoke},
        {"zreader_resolve_and_open_file_smoke", zreader_resolve_and_open_file_smoke},
        {"zmath_matrix_stack_and_direction_smoke", zmath_matrix_stack_and_direction_smoke},
        {"zmath_projection_batches_smoke", zmath_projection_batches_smoke},
        {"zmath_perspective_texture_interpolants_smoke",
         zmath_perspective_texture_interpolants_smoke},
        {"zmath_vec3_normalize_and_div_scalar_smoke", zmath_vec3_normalize_and_div_scalar_smoke},
        {"zmath_array_add_scaled_and_transform_smoke", zmath_array_add_scaled_and_transform_smoke},
        {"zmath_load_view_smoke", zmath_load_view_smoke},
        {"zmath_quaternion_helpers_smoke", zmath_quaternion_helpers_smoke},
        {"recoil_app_get_message_map_smoke", recoil_app_get_message_map_smoke},
        {"recoil_app_accessor_and_skip_wait_smoke", recoil_app_accessor_and_skip_wait_smoke},
        {"recoil_app_activate_existing_instance_absent_smoke",
         recoil_app_activate_existing_instance_absent_smoke},
        {"recoil_app_get_current_state_smoke", recoil_app_get_current_state_smoke},
        {"recoil_app_state_queue_block_init_from_cursor_smoke",
         recoil_app_state_queue_block_init_from_cursor_smoke},
        {"recoil_app_state_queue_grow_chunk_base_list_smoke",
         recoil_app_state_queue_grow_chunk_base_list_smoke},
        {"recoil_app_queue_switch_current_state_smoke",
         recoil_app_queue_switch_current_state_smoke},
        {"recoil_app_queue_exit_current_state_smoke", recoil_app_queue_exit_current_state_smoke},
        {"recoil_app_start_engine_and_queue_startup_state_smoke",
         recoil_app_start_engine_and_queue_startup_state_smoke},
        {"recoil_app_initialize_display_failure_smoke",
         recoil_app_initialize_display_failure_smoke},
        {"recoil_app_on_idle_or_dispatch_smoke", recoil_app_on_idle_or_dispatch_smoke},
        {"recoil_app_mfc_ole_module_constructor_smoke",
         recoil_app_mfc_ole_module_constructor_smoke},
        {"recoil_app_mfc_ole_module_destructor_smoke", recoil_app_mfc_ole_module_destructor_smoke},
        {"recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke",
         recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke},
        {"recoil_app_play_state_constructor_smoke", recoil_app_play_state_constructor_smoke},
        {"recoil_app_fmv_state_constructor_smoke", recoil_app_fmv_state_constructor_smoke},
        {"recoil_app_constructor_destructor_smoke", recoil_app_constructor_destructor_smoke},
        {"recoil_app_istate_destructor_smoke", recoil_app_istate_destructor_smoke},
        {"recoil_app_fmv_state_destructor_smoke", recoil_app_fmv_state_destructor_smoke},
        {"recoil_app_mission_fmv_state_destructor_smoke",
         recoil_app_mission_fmv_state_destructor_smoke},
        {"recoil_app_scalar_deleting_destructor_smoke",
         recoil_app_scalar_deleting_destructor_smoke},
        {"recoil_state_base_scalar_deleting_destructor_smoke",
         recoil_state_base_scalar_deleting_destructor_smoke},
        {"recoil_state_main_menu_transition_constructor_smoke",
         recoil_state_main_menu_transition_constructor_smoke},
        {"recoil_state_main_menu_transition_destructor_smoke",
         recoil_state_main_menu_transition_destructor_smoke},
        {"recoil_state_main_menu_transition_clear_paused_audio_snapshot_smoke",
         recoil_state_main_menu_transition_clear_paused_audio_snapshot_smoke},
        {"recoil_state_main_menu_transition_queue_enter_smoke",
         recoil_state_main_menu_transition_queue_enter_smoke},
        {"recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke",
         recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke},
        {"hud_ui_main_menu_dialog_constructor_smoke",
         hud_ui_main_menu_dialog_constructor_smoke},
        {"recoil_state_main_menu_transition_on_try_become_current_smoke",
         recoil_state_main_menu_transition_on_try_become_current_smoke},
        {"zsnd_set_use_archive_banks_flag_smoke", zsnd_set_use_archive_banks_flag_smoke},
        {"zsnd_sample_set_registry_init_shutdown_smoke",
         zsnd_sample_set_registry_init_shutdown_smoke},
        {"zsnd_sample_set_registry_lookup_destroy_smoke",
         zsnd_sample_set_registry_lookup_destroy_smoke},
        {"zsnd_option_accessors_smoke", zsnd_option_accessors_smoke},
        {"zsnd_speed_of_sound_smoke", zsnd_speed_of_sound_smoke},
        {"zsnd_backend_error_reporters_smoke", zsnd_backend_error_reporters_smoke},
        {"zsnd_cached_directsound_get_caps_smoke", zsnd_cached_directsound_get_caps_smoke},
        {"zsnd_cd_reset_track_state_smoke", zsnd_cd_reset_track_state_smoke},
        {"zsnd_cd_is_stereo_aux_enabled_smoke", zsnd_cd_is_stereo_aux_enabled_smoke},
        {"zsnd_cd_not_ready_playback_smoke", zsnd_cd_not_ready_playback_smoke},
        {"zsnd_cd_init_ready_guard_smoke", zsnd_cd_init_ready_guard_smoke},
        {"zsnd_cd_get_track_count_ready_guard_smoke", zsnd_cd_get_track_count_ready_guard_smoke},
        {"zsnd_cd_shutdown_track_list_smoke", zsnd_cd_shutdown_track_list_smoke},
        {"zsnd_sample_set_get_sample_at_smoke", zsnd_sample_set_get_sample_at_smoke},
        {"zsnd_sample_set_registry_add_entry_smoke", zsnd_sample_set_registry_add_entry_smoke},
        {"zsnd_find_sample_by_name_smoke", zsnd_find_sample_by_name_smoke},
        {"zsnd_gain_scale_to_directsound_attenuation_smoke",
         zsnd_gain_scale_to_directsound_attenuation_smoke},
        {"zsnd_is_muted_smoke", zsnd_is_muted_smoke},
        {"zsnd_sample_play_simple_smoke", zsnd_sample_play_simple_smoke},
        {"zsnd_sample_acquire_play_handle_smoke", zsnd_sample_acquire_play_handle_smoke},
        {"zsnd_system_named_sets_syntax_smoke", zsnd_system_named_sets_syntax_smoke},
        {"zsnd_system_legacy_sets_syntax_smoke", zsnd_system_legacy_sets_syntax_smoke},
        {"zsnd_system_init_gate_and_missing_config_smoke",
         zsnd_system_init_gate_and_missing_config_smoke},
        {"zsnd_preinitialize_runtime_state_smoke", zsnd_preinitialize_runtime_state_smoke},
        {"zsnd_group_load_config_block_smoke", zsnd_group_load_config_block_smoke},
        {"zsnd_group_load_and_queue_smoke", zsnd_group_load_and_queue_smoke},
        {"zsnd_stream_mgr_shutdown_lists_smoke", zsnd_stream_mgr_shutdown_lists_smoke},
        {"zsnd_backend_shutdown_release_smoke", zsnd_backend_shutdown_release_smoke},
        {"zsnd_play_handle_stop_if_active_smoke", zsnd_play_handle_stop_if_active_smoke},
        {"zsnd_snapshot_create_from_active_samples_smoke",
         zsnd_snapshot_create_from_active_samples_smoke},
        {"zsnd_snapshot_restore_all_with_global_volume_delta_smoke",
         zsnd_snapshot_restore_all_with_global_volume_delta_smoke},
        {"zsnd_snapshot_destroy_smoke", zsnd_snapshot_destroy_smoke},
        {"zsnd_snapshot_stop_all_if_playing_smoke", zsnd_snapshot_stop_all_if_playing_smoke},
        {"zsnd_play_handle_update3d_a3d_smoke", zsnd_play_handle_update3d_a3d_smoke},
        {"zsnd_update_listener_state_smoke", zsnd_update_listener_state_smoke},
        {"zsnd_play_handle_update3d_directsound_smoke",
         zsnd_play_handle_update3d_directsound_smoke},
        {"zsnd_sample_play_a3d_simple_direct_smoke", zsnd_sample_play_a3d_simple_direct_smoke},
        {"zsnd_sample_destroy_owned_data_smoke", zsnd_sample_destroy_owned_data_smoke},
        {"zsnd_fade_entry_backend_and_dispatch_smoke", zsnd_fade_entry_backend_and_dispatch_smoke},
        {"zsnd_fade_active_list_tick_compacts_smoke", zsnd_fade_active_list_tick_compacts_smoke},
        {"zsnd_fade_list_cursor_helpers_smoke", zsnd_fade_list_cursor_helpers_smoke},
        {"zsnd_fade_lists_stop_all_shutdown_smoke", zsnd_fade_lists_stop_all_shutdown_smoke},
        {"zsnd_tick_backend_markers_smoke", zsnd_tick_backend_markers_smoke},
        {"zsnd_sample_set_init_by_name_empty_smoke", zsnd_sample_set_init_by_name_empty_smoke},
        {"zsnd_wave_data_load_parse_reset_smoke", zsnd_wave_data_load_parse_reset_smoke},
        {"zsnd_wave_data_archive_load_smoke", zsnd_wave_data_archive_load_smoke},
        {"zsnd_sample_backend_buffer_lock_unlock_smoke",
         zsnd_sample_backend_buffer_lock_unlock_smoke},
        {"zsnd_sample_init_from_wave_data_directsound_smoke",
         zsnd_sample_init_from_wave_data_directsound_smoke},
        {"zsnd_sample_init_from_wave_data_a3d_smoke", zsnd_sample_init_from_wave_data_a3d_smoke},
        {"zrndr_get_active_region_state_smoke", zrndr_get_active_region_state_smoke},
        {"zrndr_framebuffer_and_stride_cache_smoke", zrndr_framebuffer_and_stride_cache_smoke},
        {"zrndr_init_globals_smoke", zrndr_init_globals_smoke},
        {"zrndr_immediate_line_dispatch_smoke", zrndr_immediate_line_dispatch_smoke},
        {"zrndr_lens_flare_queue_projected_sample_smoke",
         zrndr_lens_flare_queue_projected_sample_smoke},
        {"zrndr_lens_flare_draw_sample_smoke", zrndr_lens_flare_draw_sample_smoke},
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
        {"zrndr_span_occlusion_test_depth_order_pair_smoke",
         zrndr_span_occlusion_test_depth_order_pair_smoke},
        {"zrndr_span_occlusion_insert_no_depth_smoke", zrndr_span_occlusion_insert_no_depth_smoke},
        {"zrndr_span_occlusion_build_span_list_smoke", zrndr_span_occlusion_build_span_list_smoke},
        {"zrndr_span_occlusion_insert_local_smoke", zrndr_span_occlusion_insert_local_smoke},
        {"zrndr_span_occlusion_build_span_list_fast_smoke",
         zrndr_span_occlusion_build_span_list_fast_smoke},
        {"zrndr_span_occlusion_test_column_visibility_smoke",
         zrndr_span_occlusion_test_column_visibility_smoke},
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
        {"zrndr_overlay_and_mmx_masks_smoke", zrndr_overlay_and_mmx_masks_smoke},
        {"zrndr_span_alpha_blend_565_const_alpha_pal8_smoke",
         zrndr_span_alpha_blend_565_const_alpha_pal8_smoke},
        {"zrndr_fill_span16_opaque_smoke", zrndr_fill_span16_opaque_smoke},
        {"zrndr_fill_span555_solid_smoke", zrndr_fill_span555_solid_smoke},
        {"zrndr_fill_span565_solid_smoke", zrndr_fill_span565_solid_smoke},
        {"zrndr_span_masked_tex16_to_565_smoke", zrndr_span_masked_tex16_to_565_smoke},
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
        {"zturret_reset_iteration_state_smoke", zturret_reset_iteration_state_smoke},
        {"zturret_shutdown_leaf_smoke", zturret_shutdown_leaf_smoke},
        {"zutil_zar_register_section_handler_smoke", zutil_zar_register_section_handler_smoke},
        {"zutil_zbd_init_smoke", zutil_zbd_init_smoke},
        {"hud_sensor_register_mission_sections_smoke", hud_sensor_register_mission_sections_smoke},
        {"hud_sensor_mission_identity_smoke", hud_sensor_mission_identity_smoke},
        {"hud_sensor_map_node_basics_smoke", hud_sensor_map_node_basics_smoke},
        {"hud_sensor_map_bounds_and_save_state_smoke", hud_sensor_map_bounds_and_save_state_smoke},
        {"hud_sensor_map_remove_and_shutdown_smoke", hud_sensor_map_remove_and_shutdown_smoke},
        {"hud_sensor_reset_mission_state_smoke", hud_sensor_reset_mission_state_smoke},
        {"zutil_zar_write_section_blob_smoke", zutil_zar_write_section_blob_smoke},
        {"zeffect_anim_shutdown_entry_smoke", zeffect_anim_shutdown_entry_smoke},
        {"zeffect_anim_find_entry_by_name_smoke", zeffect_anim_find_entry_by_name_smoke},
        {"zeffect_anim_find_node_recursive_by_name_smoke",
         zeffect_anim_find_node_recursive_by_name_smoke},
        {"zeffect_anim_ref_resolution_smoke", zeffect_anim_ref_resolution_smoke},
        {"zeffect_anim_find_or_create_runtime_refs_smoke",
         zeffect_anim_find_or_create_runtime_refs_smoke},
        {"zeffect_anim_rebind_entry_to_node_smoke", zeffect_anim_rebind_entry_to_node_smoke},
        {"zeffect_anim_ensure_copied_root_tree_smoke", zeffect_anim_ensure_copied_root_tree_smoke},
        {"zeffect_anim_clone_entry_for_node_smoke", zeffect_anim_clone_entry_for_node_smoke},
        {"zeffect_cleanup_light_sound_refs_smoke", zeffect_cleanup_light_sound_refs_smoke},
        {"zeffect_handle_sample_ref_offset_event_smoke",
         zeffect_handle_sample_ref_offset_event_smoke},
        {"zeffect_handle_sound_light_events_smoke", zeffect_handle_sound_light_events_smoke},
        {"zeffect_anim_activation_prereqs_smoke", zeffect_anim_activation_prereqs_smoke},
        {"zeffect_anim_activation_record_queue_smoke", zeffect_anim_activation_record_queue_smoke},
        {"zeffect_handle_emitter_reset_event_smoke", zeffect_handle_emitter_reset_event_smoke},
        {"zeffect_handle_emitter_loop_event_smoke", zeffect_handle_emitter_loop_event_smoke},
        {"zeffect_handle_emitter_stop_event_smoke", zeffect_handle_emitter_stop_event_smoke},
        {"zeffect_conditional_ref_pos_smoke", zeffect_conditional_ref_pos_smoke},
        {"zeffect_skip_conditional_chain_smoke", zeffect_skip_conditional_chain_smoke},
        {"zeffect_marker_and_callback_event_smoke", zeffect_marker_and_callback_event_smoke},
        {"zeffect_anim_set_on_state_done_callback_smoke",
         zeffect_anim_set_on_state_done_callback_smoke},
        {"zeffect_world_and_zbd_setters_smoke", zeffect_world_and_zbd_setters_smoke},
        {"zeffect_anim_find_next_async_entry_smoke", zeffect_anim_find_next_async_entry_smoke},
        {"zeffect_anim_get_root_node_or_null_smoke", zeffect_anim_get_root_node_or_null_smoke},
        {"zeffect_anim_capture_node_states_smoke", zeffect_anim_capture_node_states_smoke},
        {"zeffect_anim_restore_node_states_smoke", zeffect_anim_restore_node_states_smoke},
        {"zeffect_anim_reset_for_node_smoke", zeffect_anim_reset_for_node_smoke},
        {"zeffect_anim_init_shutdown_smoke", zeffect_anim_init_shutdown_smoke},
        {"zeffect_find_template_index_by_name_smoke", zeffect_find_template_index_by_name_smoke},
        {"zeffect_init_smoke", zeffect_init_smoke},
        {"zdeclient_set_camera_node_smoke", zdeclient_set_camera_node_smoke},
        {"zdeclient_load_material_from_texture_path_smoke",
         zdeclient_load_material_from_texture_path_smoke},
        {"zdeclient_shutdown_globals_smoke", zdeclient_shutdown_globals_smoke},
        {"zloc_message_lookup_failure_smoke", zloc_message_lookup_failure_smoke},
        {"zloc_load_unload_messages_dll_smoke", zloc_load_unload_messages_dll_smoke},
        {"zvid_pack_color_rgb_smoke", zvid_pack_color_rgb_smoke},
        {"zvideo_pixel_pack_setup_smoke", zvideo_pixel_pack_setup_smoke},
        {"zvideo_pixel_pack_getters_smoke", zvideo_pixel_pack_getters_smoke},
        {"zvideo_set_renderer_type_smoke", zvideo_set_renderer_type_smoke},
        {"zvideo_pending_dither_enable_smoke", zvideo_pending_dither_enable_smoke},
        {"zvideo_surface_accessors_smoke", zvideo_surface_accessors_smoke},
        {"zvideo_primary_surface_rect_scratch_smoke", zvideo_primary_surface_rect_scratch_smoke},
        {"zvideo_frame_scratch_buffers_smoke", zvideo_frame_scratch_buffers_smoke},
        {"zvideo_clear_dispatch_and_exchange_smoke", zvideo_clear_dispatch_and_exchange_smoke},
        {"zvideo_mode_geometry_and_set_video_mode_smoke",
         zvideo_mode_geometry_and_set_video_mode_smoke},
        {"zvideo_init_video_system_reentry_guard_smoke",
         zvideo_init_video_system_reentry_guard_smoke},
        {"zvideo_dispatch_wrappers_smoke", zvideo_dispatch_wrappers_smoke},
        {"zvideo_bind_renderer_dispatch_smoke", zvideo_bind_renderer_dispatch_smoke},
        {"zvideo_adjust_surfaces_if_enabled_smoke", zvideo_adjust_surfaces_if_enabled_smoke},
        {"zvideo_set_half_res_adjust_mode_smoke", zvideo_set_half_res_adjust_mode_smoke},
        {"zvideo_dd_report_error_smoke", zvideo_dd_report_error_smoke},
        {"zvid_hw_api_accessors_smoke", zvid_hw_api_accessors_smoke},
        {"zvid_cached_renderer_and_texture_counts_smoke",
         zvid_cached_renderer_and_texture_counts_smoke},
        {"zvid_option_accessors_smoke", zvid_option_accessors_smoke},
        {"zvid_set_video_mode_index_smoke", zvid_set_video_mode_index_smoke},
        {"zvideo_buff_clip_coord_to_range_smoke", zvideo_buff_clip_coord_to_range_smoke},
        {"zvideo_buff_copy_surface_rect_to_image_smoke",
         zvideo_buff_copy_surface_rect_to_image_smoke},
        {"zvideo_surface_state_lock_skip_smoke", zvideo_surface_state_lock_skip_smoke},
        {"zvideo_capture_surface_to_image_smoke", zvideo_capture_surface_to_image_smoke},
        {"zvideo_image_lazy_create_backing_surface_guards_smoke",
         zvideo_image_lazy_create_backing_surface_guards_smoke},
        {"zvideo_blt_sw_to_primary_rect_lazy_failure_smoke",
         zvideo_blt_sw_to_primary_rect_lazy_failure_smoke},
        {"zvideo_flip_to_gdi_if_attached_null_smoke", zvideo_flip_to_gdi_if_attached_null_smoke},
        {"zvideo_clear_rect_skip_paths_smoke", zvideo_clear_rect_skip_paths_smoke},
        {"zvideo_palette_set_entries_non8bpp_smoke", zvideo_palette_set_entries_non8bpp_smoke},
        {"zvideo_quad_batch_depth_and_rhw_smoke", zvideo_quad_batch_depth_and_rhw_smoke},
        {"zvideo_queue_solid_quad_smoke", zvideo_queue_solid_quad_smoke},
        {"zvideo_flush_quad_batch_empty_smoke", zvideo_flush_quad_batch_empty_smoke},
        {"zvideo_flush_sorted_polys_empty_smoke", zvideo_flush_sorted_polys_empty_smoke},
        {"zvideo_submit_poly_flat_color16_queue_smoke",
         zvideo_submit_poly_flat_color16_queue_smoke},
        {"zvideo_submit_poly_gouraud_color16_queue_smoke",
         zvideo_submit_poly_gouraud_color16_queue_smoke},
        {"zvideo_submit_poly_color_attr_smoke", zvideo_submit_poly_color_attr_smoke},
        {"zvideo_submit_poly_render_class_queue_smoke",
         zvideo_submit_poly_render_class_queue_smoke},
        {"zvideo_submit_polygon_queue_smoke", zvideo_submit_polygon_queue_smoke},
        {"zvideo_submit_polygon_lit_queue_smoke", zvideo_submit_polygon_lit_queue_smoke},
        {"zvideo_present_display_mode_surface_null_smoke",
         zvideo_present_display_mode_surface_null_smoke},
        {"zvideo_texture_record_release_upload_null_smoke",
         zvideo_texture_record_release_upload_null_smoke},
        {"zvideo_texture_record_create_and_power_smoke",
         zvideo_texture_record_create_and_power_smoke},
        {"zvideo_convert_image_pixels_for_texture_smoke",
         zvideo_convert_image_pixels_for_texture_smoke},
        {"zvideo_image_alpha_clear_smoke", zvideo_image_alpha_clear_smoke},
        {"zvideo_image_set_pixels_smoke", zvideo_image_set_pixels_smoke},
        {"zvideo_image_file_read_helpers_smoke", zvideo_image_file_read_helpers_smoke},
        {"zvideo_palette_remap_no_recipes_smoke", zvideo_palette_remap_no_recipes_smoke},
        {"zvideo_texture_pack_load_image_smoke", zvideo_texture_pack_load_image_smoke},
        {"zimage_font_glyph_scan_smoke", zimage_font_glyph_scan_smoke},
        {"zimage_font_measure_string_smoke", zimage_font_measure_string_smoke},
        {"zimage_texdir_find_or_create_missing_smoke", zimage_texdir_find_or_create_missing_smoke},
        {"zimage_texdir_build_mip_chain_smoke", zimage_texdir_build_mip_chain_smoke},
        {"zvid_texture_pack_ensure_builtin_smoke", zvid_texture_pack_ensure_builtin_smoke},
        {"zimage_texdir_load_pending_entries_smoke", zimage_texdir_load_pending_entries_smoke},
        {"zimage_texdir_base_name_path_smoke", zimage_texdir_base_name_path_smoke},
        {"zimage_texdir_variant_image_smoke", zimage_texdir_variant_image_smoke},
        {"zimage_texdir_find_by_name_smoke", zimage_texdir_find_by_name_smoke},
        {"zimage_texdir_write_smoke", zimage_texdir_write_smoke},
        {"zimage_init_option_fallback_smoke", zimage_init_option_fallback_smoke},
        {"zimage_shutdown_texdir_smoke", zimage_shutdown_texdir_smoke},
        {"zvid_image_resample_square_smoke", zvid_image_resample_square_smoke},
        {"zvid_image_calc_pow2_scratch_fields_smoke", zvid_image_calc_pow2_scratch_fields_smoke},
        {"zvid_image_release_owned_buffers_smoke", zvid_image_release_owned_buffers_smoke},
        {"zvid_image_destroy_smoke", zvid_image_destroy_smoke},
        {"zvideo_create_texture_record_guards_smoke", zvideo_create_texture_record_guards_smoke},
        {"zvideo_image_surface_helpers_guard_smoke", zvideo_image_surface_helpers_guard_smoke},
        {"zvideo_restore_display_surfaces_null_smoke", zvideo_restore_display_surfaces_null_smoke},
        {"zvideo_select_hw_api_device_smoke", zvideo_select_hw_api_device_smoke},
        {"zvideo_return_success_stub_smoke", zvideo_return_success_stub_smoke},
        {"zvid_cached_client_rect_smoke", zvid_cached_client_rect_smoke},
        {"zweapon_init_smoke", zweapon_init_smoke},
        {"zweapon_optcatalog_shutdown_smoke", zweapon_optcatalog_shutdown_smoke},
        {"zweapon_optcatalog_find_entry_by_id_smoke", zweapon_optcatalog_find_entry_by_id_smoke},
        {"zweapon_optcatalog_pending_spawn_override_smoke",
         zweapon_optcatalog_pending_spawn_override_smoke},
        {"zweapon_optcatalog_runtime_free_list_helpers_smoke",
         zweapon_optcatalog_runtime_free_list_helpers_smoke},
        {"light_alloc_from_free_list_and_attach_smoke",
         light_alloc_from_free_list_and_attach_smoke},
        {"zweapon_optcatalog_warning_samples_smoke", zweapon_optcatalog_warning_samples_smoke},
        {"zvideo_restore_iconic_fullscreen_window_smoke",
         zvideo_restore_iconic_fullscreen_window_smoke},
        {"zvideo_shutdown_video_system_smoke", zvideo_shutdown_video_system_smoke},
        {"zsys_find_file_on_drive_type_negative_smoke",
         zsys_find_file_on_drive_type_negative_smoke},
        {"zsys_runtime_probe_leaves_smoke", zsys_runtime_probe_leaves_smoke},
        {"zsys_cpuid_mmx_smoke", zsys_cpuid_mmx_smoke},
        {"zerror_init_output_context_smoke", zerror_init_output_context_smoke},
        {"zcom_query_interface_from_interface_map_smoke",
         zcom_query_interface_from_interface_map_smoke},
        {"zcom_connection_point_container_advise_smoke",
         zcom_connection_point_container_advise_smoke},
        {"zcom_connection_point_container_unadvise_smoke",
         zcom_connection_point_container_unadvise_smoke},
        {"pickup_type_table_free_opt_meta_smoke", pickup_type_table_free_opt_meta_smoke},
        {"pickup_airdrop_spawn_ref_shutdown_global_smoke",
         pickup_airdrop_spawn_ref_shutdown_global_smoke},
        {"pickup_spawn_list_clear_smoke", pickup_spawn_list_clear_smoke},
        {"pickup_respawn_queue_clear_smoke", pickup_respawn_queue_clear_smoke},
        {"pickup_shutdown_smoke", pickup_shutdown_smoke},
        {"pickup_leaf_helpers_smoke", pickup_leaf_helpers_smoke},
        {"time_reset_smoke", time_reset_smoke},
    };

    const int directxResult = recoil_legacy_directx_header_smoke() > 0 ? 0 : 1;
    if (directxResult != 0) {
        std::printf("recoil_legacy_directx_header_smoke failed: %d\n", directxResult);
    }

    return directxResult + RunSmokeTests(tests, static_cast<int>(sizeof(tests) / sizeof(tests[0])),
                                         argc > 1 ? argv[1] : nullptr);
}
