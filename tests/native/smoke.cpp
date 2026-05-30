extern "C" int recoil_native_build_anchor(void);
extern "C" int recoil_legacy_directx_header_smoke(void);
extern "C" int recoil_mfc42_provider_smoke(void);
extern "C" int mfc_cstring_default_ctor_provider_smoke(void);
extern "C" int czrecoil_frame_build_window_title_smoke(void);
extern "C" int czframe_metadata_accessors_smoke(void);
extern "C" int get_open_file_name_import_provider_smoke(void);
extern "C" int czrecoil_frame_set_menu_bar_visibility_smoke(void);
extern "C" int czrecoil_frame_configure_mode_feature_flags_smoke(void);
extern "C" int czrecoil_frame_video_mode_menu_handlers_smoke(void);
extern "C" int czrecoil_frame_set_hw_api_and_init_mode_smoke(void);
extern "C" int czrecoil_frame_init_fallback_mode_smoke(void);
extern "C" int czrecoil_frame_ensure_hw_api_initialized_smoke(void);
extern "C" int czrecoil_frame_init_startup_hw_api_from_options_smoke(void);
extern "C" int czrecoil_frame_menu_toggle_smoke(void);
extern "C" int czrecoil_frame_menu_exit_game_smoke(void);
extern "C" int czrecoil_frame_update_video_mode_cmd_ui_smoke(void);
extern "C" int czrecoil_frame_hw_api_menu_cmd_ui_smoke(void);
extern "C" int czrecoil_frame_audio_input_menu_smoke(void);
extern "C" int czrecoil_frame_on_size_smoke(void);
extern "C" int czgame_frame_is_window_valid_smoke(void);
extern "C" int czgame_frame_build_window_title_smoke(void);
extern "C" int czgame_frame_constructor_smoke(void);
extern "C" int czgame_frame_create_object_smoke(void);
extern "C" int czgame_frame_destructor_smoke(void);
extern "C" int czgame_frame_on_close_smoke(void);
extern "C" int czgame_frame_on_create_smoke(void);
extern "C" int czgame_frame_on_destroy_smoke(void);
extern "C" int czgame_frame_on_activate_smoke(void);
extern "C" int czgame_frame_on_paint_smoke(void);
extern "C" int czrecoil_frame_constructor_smoke(void);
extern "C" int czrecoil_frame_create_object_smoke(void);
extern "C" int recoil_app_create_main_wnd_smoke(void);
extern "C" int recoil_app_init_main_window_smoke(void);
extern "C" int czrecoil_frame_destructor_smoke(void);
extern "C" int czrecoil_frame_on_menu_start_single_player_smoke(void);
extern "C" int czrecoil_frame_on_open_file_dialog_smoke(void);
extern "C" int czrecoil_frame_on_menu_open_campaign_smoke(void);
extern "C" int czrecoil_frame_on_menu_open_help_docs_smoke(void);
extern "C" int cabout_dlg_constructor_smoke(void);
extern "C" int czrecoil_frame_on_menu_about_smoke(void);
extern "C" int mfc_three_float_dialog_handlers_smoke(void);
extern "C" int zstr_contains_case_insensitive_smoke(void);
extern "C" int czrecoil_frame_start_mode_menu_handlers_smoke(void);
extern "C" int czrecoil_frame_select_hw_api_menu_handlers_smoke(void);
extern "C" int czrecoil_frame_toggle_archive_banks_smoke(void);
extern "C" int czrecoil_frame_toggle_texture_packs_smoke(void);
extern "C" int hud_ui_save_load_entry_is_newer_than_smoke(void);
extern "C" int hud_ui_save_load_list_item_constructor_smoke(void);
extern "C" int hud_ui_save_load_list_item_on_activate_smoke(void);
extern "C" int hud_ui_save_load_insert_entry_sorted_prefix_smoke(void);
extern "C" int hud_ui_save_load_partition_entries_by_pivot_smoke(void);
extern "C" int hud_ui_save_load_sort_entry_range_smoke(void);
extern "C" int hud_ui_save_load_refresh_file_list_smoke(void);
extern "C" int hud_ui_save_load_initialize_file_entries_smoke(void);
extern "C" int hud_ui_save_load_delete_save_file_smoke(void);
extern "C" int hud_ui_save_load_set_selected_entry_index_smoke(void);
extern "C" int hud_ui_save_load_game_name_input_smoke(void);
extern "C" int hud_ui_save_load_next_prev_buttons_smoke(void);
extern "C" int hud_ui_save_load_delete_button_on_activate_smoke(void);
extern "C" int czgame_frame_on_app_idle_dispatch_message_smoke(void);
extern "C" int recoil_state_controls_queue_enter_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_queue_enter_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_constructor_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_destructor_core_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_scalar_deleting_destructor_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_static_init_thunks_smoke(void);
extern "C" int hud_ui_options_panel_overlay_owner_on_try_become_current_smoke(void);
extern "C" int briefing_stop_and_shutdown_thread_smoke(void);
extern "C" int briefing_thread_main_one_iteration_smoke(void);
extern "C" int briefing_start_for_mission_smoke(void);
extern "C" int briefing_set_progress_and_sleep_smoke(void);
extern "C" int briefing_runtime_constructor_smoke(void);
extern "C" int briefing_locator_panel_constructor_smoke(void);
extern "C" int briefing_locator_panel_blit_dirty_rect_smoke(void);
extern "C" int briefing_locator_panel_update_smoke(void);
extern "C" int briefing_runtime_destructor_smoke(void);
extern "C" int briefing_runtime_update_smoke(void);
extern "C" int briefing_objective_picture_draw_noise_overlay_smoke(void);
extern "C" int briefing_build_objective_actions_smoke(void);
extern "C" int gamenet_list_reset_smoke(void);
extern "C" int gamenet_player_row_append_smoke(void);
extern "C" int gamenet_unregister_gameplay_packet_handlers_smoke(void);
extern "C" int gamenet_register_gameplay_handlers_and_callbacks_smoke(void);
extern "C" int gamenet_chat_compose_key_callback_smoke(void);
extern "C" int gamenet_begin_chat_compose_smoke(void);
extern "C" int hud_ui_handle_hotkey_command_begin_chat_smoke(void);
extern "C" int hud_timer_panel_net_state_clear_tail_flags_smoke(void);
extern "C" int gamenet_find_player_row_and_status_bits_smoke(void);
extern "C" int gamenet_update_remote_player_hud_widget_screen_pos_smoke(void);
extern "C" int gamenet_get_local_player_color_index_smoke(void);
extern "C" int gamenet_get_nearest_other_player_distance_to_spawn_point_smoke(void);
extern "C" int gamenet_respawn_player_color_indexed_spawn_smoke(void);
extern "C" int gamenet_reset_hud_timer_panel_net_state_smoke(void);
extern "C" int gamenet_wait_for_local_player_color_index_smoke(void);
extern "C" int net_init_from_zrd_smoke(void);
extern "C" int gamenet_host_update_session_status_fields_smoke(void);
extern "C" int gamenet_timer_status_packet_smoke(void);
extern "C" int gamenet_timer_panel_state_packet_smoke(void);
extern "C" int gamenet_tick_local_player_pkt06_and_timer_smoke(void);
extern "C" int gamenet_scoreboard_snapshot_packet_smoke(void);
extern "C" int gamenet_lap_progress_packet_smoke(void);
extern "C" int gamenet_chat_message_packet_smoke(void);
extern "C" int gamenet_show_player_kill_message_smoke(void);
extern "C" int gamenet_player_kill_event_packet_smoke(void);
extern "C" int gamenet_reassign_player_colors_smoke(void);
extern "C" int gamenet_player_row_apply_color_tint_smoke(void);
extern "C" int gamenet_apply_pkt06_player_state_snapshot_smoke(void);
extern "C" int gamenet_handle_pkt06_player_state_snapshot_smoke(void);
extern "C" int gamenet_spawn_remote_player_missing_template_smoke(void);
extern "C" int gamenet_handle_pkt07_alt_gun_dispatch_smoke(void);
extern "C" int gamenet_send_pkt07_alt_gun_dispatch_smoke(void);
extern "C" int optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke(void);
extern "C" int gamenet_alt_gun_dispatch_no_op_callback_smoke(void);
extern "C" int optcatalog_handle_pkt0a_remove_runtime_relay_smoke(void);
extern "C" int optcatalog_send_pkt0a_remove_runtime_relay_smoke(void);
extern "C" int gamenet_host_send_pkt10_qsand_feature_smoke(void);
extern "C" int gamenet_send_pkt10_qsand_event_smoke(void);
extern "C" int gamenet_host_send_pkt0f_crater_feature_smoke(void);
extern "C" int gamenet_send_pkt13_effect_anim_activation_record_smoke(void);
extern "C" int gamenet_handle_pkt13_effect_anim_activation_record_smoke(void);
extern "C" int gamenet_send_all_pkt13_effect_anim_activation_records_smoke(void);
extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void);
extern "C" int gamenet_reset_remote_players_and_spawn_lists_smoke(void);
extern "C" int gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke(void);
extern "C" int gamenet_handle_pkt03_remove_remote_player_smoke(void);
extern "C" int hud_low_meter_loop_sound_set_loop_active_smoke(void);
extern "C" int hud_cheat_clear_nanite_panel_cheat_sentinel_smoke(void);
extern "C" int hud_cheat_execute_command_string_smoke(void);
extern "C" int ainet_free_all_smoke(void);
extern "C" int ainet_alloc_smoke(void);
extern "C" int ainet_find_node_by_index_smoke(void);
extern "C" int ainet_path_probe_fan_init_from_segment_smoke(void);
extern "C" int ainet_resolve_neighbor_links_and_probe_fans_smoke(void);
extern "C" int ainet_load_from_zrd_smoke(void);
extern "C" int ainet_load_all_from_zrd_smoke(void);
extern "C" int zvehicle_select_zrd_by_difficulty_smoke(void);
extern "C" int ainet_find_by_net_id_smoke(void);
extern "C" int ainet_find_nearest_node_smoke(void);
extern "C" int player_load_master_common_data_from_node_smoke(void);
extern "C" int player_get_save_state_list_head_smoke(void);
extern "C" int player_unbind_current_save_state_if_single_player_smoke(void);
extern "C" int player_bind_active_game_state_as_current_save_state_smoke(void);
extern "C" int player_async_command_callback_basic_smoke(void);
extern "C" int player_sync_local_pose_from_root_node_smoke(void);
extern "C" int player_get_aiv_zrd_path_smoke(void);
extern "C" int player_extract_vehicle_name_from_aiv_name_smoke(void);
extern "C" int player_clone_type6_node_from_template_and_rename_smoke(void);
extern "C" int player_create_from_names_at_pose_smoke(void);
extern "C" int player_init_mission_runtime_missing_aiv_smoke(void);
extern "C" int hud_sensor_tracker_init_mission_gameplay_systems_smoke(void);
extern "C" int player_cache_gun_hardpoints_and_detach_displays_smoke(void);
extern "C" int player_init_state_from_name_and_master_common_data_smoke(void);
extern "C" int player_build_ai_peer_rings_by_ai_net_id_smoke(void);
extern "C" int player_build_support_points_from_model_smoke(void);
extern "C" int player_build_collision_points_from_model_smoke(void);
extern "C" int player_bind_modal_state_from_master_modal_data_smoke(void);
extern "C" int player_sample_ground_and_align_root_to_surface_smoke(void);
extern "C" int player_init_spawn_state_from_primary_modal_data_smoke(void);
extern "C" int player_load_master_modal_data_from_node_smoke(void);
extern "C" int player_ai_discard_negative_branch_nodes_smoke(void);
extern "C" int player_tick_ai_mode2_top_level_smoke(void);
extern "C" int player_mgr_tick_all_players_smoke(void);
extern "C" int player_ai_enter_mode2_steering_pursuit_smoke(void);
extern "C" int player_ai_alert_attack_buddies_smoke(void);
extern "C" int player_ai_try_enter_mode2_attack_pursuit_los_smoke(void);
extern "C" int player_ai_rebuild_synthetic_path_to_node_if_far_smoke(void);
extern "C" int player_tick_ai_mode2_steering_substate_smoke(void);
extern "C" int player_update_ai_mode2_move_and_turn_toward_target_smoke(void);
extern "C" int player_update_ai_mode2_turn_toward_player_no_throttle_smoke(void);
extern "C" int player_update_ai_mode2_turn_in_place_toward_player_smoke(void);
extern "C" int player_solve_alt_gun_lead_target_point_smoke(void);
extern "C" int player_tick_ai_mode2_alt_gun_attack_window_smoke(void);
extern "C" int player_update_ai_mode2_move_and_turn_toward_offset_target_smoke(void);
extern "C" int player_update_ai_mode2_move_and_turn_toward_dynamic_offset_target_smoke(void);
extern "C" int player_tick_ai_mode2_offset_target_steering_smoke(void);
extern "C" int player_tick_ai_mode2_dynamic_offset_target_steering_smoke(void);
extern "C" int player_ai_restore_saved_top_level_state_smoke(void);
extern "C" int player_ai_finalize_mode2_state1_for_all_players_smoke(void);
extern "C" int player_ai_steer_toward_path_node_forward_smoke(void);
extern "C" int player_ai_steer_toward_path_node_reverse_smoke(void);
extern "C" int player_tick_ai_mode2_timed_path_steering_smoke(void);
extern "C" int player_find_alt_gun_controller_smoke(void);
extern "C" int player_is_alt_weapon_allowed_in_current_master_mode_smoke(void);
extern "C" int player_auto_switch_to_next_usable_alt_weapon_smoke(void);
extern "C" int player_update_gun_dispatch_requests_from_trigger_latches_smoke(void);
extern "C" int player_update_debug_overlay_hud_smoke(void);
extern "C" int player_alt_gun_fire_point_selection_smoke(void);
extern "C" int player_primary_gun_fire_point_selection_smoke(void);
extern "C" int player_alt_gun_ensure_aux_effect_active_smoke(void);
extern "C" int player_update_continuous_alt_gun_fire_controller_smoke(void);
extern "C" int player_alt_gun_projectile_dispatch_helpers_smoke(void);
extern "C" int player_process_alt_gun_fire_dispatch_request_smoke(void);
extern "C" int player_process_primary_gun_dispatch_request_smoke(void);
extern "C" int player_tick_alt_gun_runtime_state_smoke(void);
extern "C" int player_alt_gun_fire_slot_offset_smoke(void);
extern "C" int player_build_mission_save_data_smoke(void);
extern "C" int player_apply_mission_save_data_smoke(void);
extern "C" int player_zar_read_mission_save_data_section_smoke(void);
extern "C" int player_refresh_hud_from_state_smoke(void);
extern "C" int player_apply_status_meter_change_smoke(void);
extern "C" int player_apply_primary_weapon_switch_smoke(void);
extern "C" int player_apply_alt_weapon_switch_smoke(void);
extern "C" int player_load_weapon_banks_and_select_defaults_smoke(void);
extern "C" int player_free_alt_weapon_trail_runtime_states_smoke(void);
extern "C" int player_check_mission_weapon_availability_smoke(void);
extern "C" int player_reset_alt_gun_door_animation_state_smoke(void);
extern "C" int player_reset_alt_gun_runtime_state_smoke(void);
extern "C" int player_remove_all_deployed_mines_smoke(void);
extern "C" int player_handle_alt_weapon_bank_select_smoke(void);
extern "C" int player_handle_primary_weapon_variant_toggle_smoke(void);
extern "C" int player_reset_damage_state_and_timed_hit_status_smoke(void);
extern "C" int player_reset_damage_visuals_and_timed_status_smoke(void);
extern "C" int player_destroyed_state_reset_local_finalize_smoke(void);
extern "C" int player_destroyed_state_reset_finalize_callback_smoke(void);
extern "C" int player_destroyed_state_reset_callback_smoke(void);
extern "C" int player_clear_respawn_transition_flag_callback_smoke(void);
extern "C" int player_destroyed_state_respawn_callback_smoke(void);
extern "C" int player_enter_local_inactive_destroyed_lifecycle_smoke(void);
extern "C" int player_update_status_meter_smoke(void);
extern "C" int player_set_world_pose_and_restart_anchor_smoke(void);
extern "C" int player_capture_current_object_pose_as_restart_anchor_smoke(void);
extern "C" int player_reset_mouse_control_state_and_recenter_cursor_smoke(void);
extern "C" int player_register_gameplay_callbacks_and_ff_smoke(void);
extern "C" int player_toggle_steering_mode_and_reset_mouse_look_smoke(void);
extern "C" int player_reset_motion_transient_state_smoke(void);
extern "C" int player_is_mission_probe_type1_enabled_by_id_smoke(void);
extern "C" int player_update_bank_velocity_from_steer_input_smoke(void);
extern "C" int player_update_auto_turn_and_steer_from_target_smoke(void);
extern "C" int player_integrate_yaw_and_wrap_from_yaw_velocity_smoke(void);
extern "C" int player_rebuild_steer_basis_from_motion_basis_smoke(void);
extern "C" int player_rebuild_steer_basis_from_motion_axes_smoke(void);
extern "C" int player_set_auto_turn_target_dir_from_world_point_smoke(void);
extern "C" int player_tick_local_player_controls_smoke(void);
extern "C" int player_filter_camera_probe_blocking_hits_smoke(void);
extern "C" int player_find_nearest_third_person_camera_probe_point_smoke(void);
extern "C" int player_adjust_sub_camera_focus_for_obstruction_smoke(void);
extern "C" int player_adjust_third_person_camera_by_offset_probes_smoke(void);
extern "C" int player_adjust_third_person_camera_by_side_probes_smoke(void);
extern "C" int player_update_chase_camera_from_input_smoke(void);
extern "C" int player_update_top_down_camera_state_smoke(void);
extern "C" int player_update_first_person_camera_from_input_smoke(void);
extern "C" int player_update_camera_from_stored_target_toward_player_smoke(void);
extern "C" int player_restore_third_person_camera_from_obstruction_state_smoke(void);
extern "C" int player_update_camera_variant_from_anchor_smoke(void);
extern "C" int player_update_camera_variant_from_camera_pos_smoke(void);
extern "C" int player_update_camera_weather_fx_emitter_visibility_smoke(void);
extern "C" int player_tick_active_camera_state_smoke(void);
extern "C" int player_clear_pending_contact_queues_smoke(void);
extern "C" int player_pending_contact_select_preferred_smoke(void);
extern "C" int player_select_and_resolve_preferred_pending_collision_contact_smoke(void);
extern "C" int checkpoint_update_player_lap_progress_and_notify_net_smoke(void);
extern "C" int checkpoint_instantiate_named_objects_smoke(void);
extern "C" int player_classify_pending_contacts_for_segment_smoke(void);
extern "C" int player_collect_pending_contacts_for_segments_smoke(void);
extern "C" int player_ai_mode2_forward_probe_requires_auto_turn_smoke(void);
extern "C" int player_ai_choose_next_path_branch_index_smoke(void);
extern "C" int player_ai_advance_path_cursor_and_compute_target_vec_smoke(void);
extern "C" int player_tick_ai_mode2_path_follow_smoke(void);
extern "C" int player_pickup_contact_passes_collection_test_smoke(void);
extern "C" int player_process_pending_pickup_contacts_smoke(void);
extern "C" int player_build_pending_contact_queues_smoke(void);
extern "C" int player_process_pending_contact_queues_smoke(void);
extern "C" int player_collect_pending_collision_contacts_for_quad_probe_smoke(void);
extern "C" int player_try_resolve_pending_collision_probe_sweep_smoke(void);
extern "C" int player_resolve_pending_collision_contact_smoke(void);
extern "C" int player_prepare_pending_world_collision_response_smoke(void);
extern "C" int player_resolve_pending_world_collision_contact_smoke(void);
extern "C" int player_resolve_pending_player_collision_contact_smoke(void);
extern "C" int player_process_transfer_contact_queue_smoke(void);
extern "C" int player_apply_pitch_roll_velocity_impulse_from_direction_smoke(void);
extern "C" int player_record_recent_hit_feedback_smoke(void);
extern "C" int player_update_timed_hit_status_from_source_smoke(void);
extern "C" int player_hit_callback_record_context_and_timed_status_smoke(void);
extern "C" int player_hit_callback_record_net_context_and_timed_status_smoke(void);
extern "C" int player_enter_destroyed_state_smoke(void);
extern "C" int player_clear_destroyed_respawn_effect_handle_callback_smoke(void);
extern "C" int player_start_destroyed_state_vehicle_effect_smoke(void);
extern "C" int player_apply_damage_local_smoke(void);
extern "C" int player_tick_remote_network_player_smoke(void);
extern "C" int player_apply_pending_collision_probe_velocity_smoke(void);
extern "C" int player_vec3_fast_normalize_smoke(void);
extern "C" int player_constrain_to_unit_distance_from_smoke(void);
extern "C" int hud_sensor_tracker_parse_checkpoint_number_from_node_smoke(void);
extern "C" int player_start_modal_loop_sfx_handle_smoke(void);
extern "C" int player_start_master_type_loop_sfx_handle_smoke(void);
extern "C" int player_ensure_master_type_loop_sfx_handle_smoke(void);
extern "C" int player_stop_master_type_loop_sfx_handle_smoke(void);
extern "C" int player_stop_modal_loop_sfx_handle_smoke(void);
extern "C" int player_update_modal_loop_sfx_smoke(void);
extern "C" int player_select_modal_state_by_master_type_smoke(void);
extern "C" int player_master_type_transition_leaf_smoke(void);
extern "C" int player_apply_master_type_transition_smoke(void);
extern "C" int player_transition_to_master_type_track_smoke(void);
extern "C" int player_transition_to_master_type_amphib_smoke(void);
extern "C" int player_transition_to_master_type_sub_smoke(void);
extern "C" int player_transition_to_master_type_hover_smoke(void);
extern "C" int player_cache_disable_copter_snd_nodes_smoke(void);
extern "C" int player_reactivate_copter_snd_nodes_if_healthy_smoke(void);
extern "C" int player_start_slip_sfx_smoke(void);
extern "C" int player_stop_slip_sfx_smoke(void);
extern "C" int player_float_sign_smoke(void);
extern "C" int player_update_bank_and_turn_dynamics_smoke(void);
extern "C" int player_compute_turn_slip_delta_smoke(void);
extern "C" int player_update_sub_vertical_damping_smoke(void);
extern "C" int player_update_yaw_velocity_from_steer_input_smoke(void);
extern "C" int player_select_probe_sample_height_smoke(void);
extern "C" int player_build_environment_probe_result_smoke(void);
extern "C" int player_los_from_fx_offset_smoke(void);
extern "C" int player_apply_aim_pitch_to_direction_smoke(void);
extern "C" int player_apply_environment_probe_result_smoke(void);
extern "C" int player_surface_height_and_terrain_tilt_smoke(void);
extern "C" int player_compute_surface_from1_probe_smoke(void);
extern "C" int player_compute_triangle_normal_smoke(void);
extern "C" int player_compute_surface_from2_probes_smoke(void);
extern "C" int player_check_probe_sample_mask_overlap_smoke(void);
extern "C" int player_rebuild_above_ground_indices_smoke(void);
extern "C" int player_select_best_probes_by_dot_product_smoke(void);
extern "C" int player_compute_surface_from3_probes_smoke(void);
extern "C" int player_process_env_probe_results_smoke(void);
extern "C" int player_rebuild_orientation_from_normal_smoke(void);
extern "C" int player_rebuild_steer_basis_raw_from_ref_smoke(void);
extern "C" int player_rebuild_motion_basis_from_steer_basis_smoke(void);
extern "C" int player_apply_amphib_speed_oscillation_smoke(void);
extern "C" int player_find_third_probe_and_compute_normal_smoke(void);
extern "C" int player_accumulate_slope_forces_smoke(void);
extern "C" int player_update_vertical_velocity_and_transform_smoke(void);
extern "C" int player_update_post_move_environment_smoke(void);
extern "C" int player_update_master_type_track_smoke(void);
extern "C" int player_probe_modal_sample_heights_smoke(void);
extern "C" int player_update_master_type_hover_from_modal_probe_smoke(void);
extern "C" int player_update_master_type_hover_smoke(void);
extern "C" int player_update_master_type_amphib_from_modal_probe_smoke(void);
extern "C" int player_update_sub_mode_water_probe_state_smoke(void);
extern "C" int player_update_master_type_sub_smoke(void);
extern "C" int player_tick_master_type_and_force_feedback_smoke(void);
extern "C" int player_update_master_type_amphib_smoke(void);
extern "C" int player_update_master_type_basic_smoke(void);
extern "C" int player_update_third_person_camera_smoke(void);
extern "C" int player_apply_camera_state_and_zopt_set_camera_mode_smoke(void);
extern "C" int player_zar_write_mission_save_data_section_smoke(void);
extern "C" int player_restore_recorded_node_flags_smoke(void);
extern "C" int player_record_node_flags_for_restore_smoke(void);
extern "C" int zutil_save_game_state_free_owned_resources_smoke(void);
extern "C" int player_zar_register_sections_smoke(void);
extern "C" int player_zar_read_vehicle_list_section_smoke(void);
extern "C" int player_zar_write_vehicle_list_section_smoke(void);
extern "C" int player_write_mines_zar_section_smoke(void);
extern "C" int player_mines_zar_read_entry_or_reset_smoke(void);
extern "C" int player_destroy_save_game_state_smoke(void);
extern "C" int player_shutdown_mission_runtime_smoke(void);
extern "C" int zfmv_script_reset_smoke(void);
extern "C" int zfmv_script_init_null_path_smoke(void);
extern "C" int zfmv_script_cleanup_smoke(void);
extern "C" int zfmv_script_append_action_smoke(void);
extern "C" int zfmv_script_run_blocking_empty_smoke(void);
extern "C" int zfmv_script_begin_current_action_smoke(void);
extern "C" int zfmv_script_begin_at_time_smoke(void);
extern "C" int zfmv_script_update_smoke(void);
extern "C" int zfmv_script_update_at_time_smoke(void);
extern "C" int zfmv_script_begin_now_smoke(void);
extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void);
extern "C" int zfmv_action_image_constructor_scaled_smoke(void);
extern "C" int zfmv_action_fade_constructor_smoke(void);
extern "C" int zfmv_playback_init_smoke(void);
extern "C" int zfmv_playback_set_dest_rect_smoke(void);
extern "C" int zfmv_playback_destructor_smoke(void);
extern "C" int zfmv_playback_report_mci_error_smoke(void);
extern "C" int zfmv_stream_destructor_empty_smoke(void);
extern "C" int zfmv_stream_constructor_missing_file_smoke(void);
extern "C" int zfmv_stream_open_audio_missing_file_smoke(void);
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
extern "C" int zutil_store_int32_smoke(void);
extern "C" int zgame_options_init_registry_context_smoke(void);
extern "C" int zmodel_display_init_smoke(void);
extern "C" int zmodel_backface_elimination_tolerance_smoke(void);
extern "C" int zmodel_small_poly_reject_thresholds_smoke(void);
extern "C" int zmodel_set_vertex_shading_enabled_smoke(void);
extern "C" int zmodel_render_state_setters_smoke(void);
extern "C" int zmodel_const_tolerances_and_cross_smoke(void);
extern "C" int zdi_add_polygon_wrapper_smoke(void);
extern "C" int zmodel_damage_mask_uv_smoke(void);
extern "C" int zmodel_damage_mask_stamp_smoke(void);
extern "C" int zmodel_display_shutdown_smoke(void);
extern "C" int zmodel_material_pool_entry_smoke(void);
extern "C" int zmodel_matl_init_globals_smoke(void);
extern "C" int zmodel_matlbuffer_set_array_size_smoke(void);
extern "C" int zmodel_set_display_instance_pool_capacity_smoke(void);
extern "C" int zmodel_set_software_path_active_smoke(void);
extern "C" int zmodel_matlbuffer_release_texture_surfaces_smoke(void);
extern "C" int zmodel_material_defaults_and_find_smoke(void);
extern "C" int zgeometry_model_add_polygon_to_di_smoke(void);
extern "C" int zgeometry_model_polygon_uv_and_di_helpers_smoke(void);
extern "C" int zgeometry_vec3array_clip_leaf_helpers_smoke(void);
extern "C" int zgeometry_clip_polygon_create_copy_finalize_smoke(void);
extern "C" int zgeometry_clip_polygon_upsert_point_list_xy_smoke(void);
extern "C" int zgeometry_model_linear_buffer_and_bounds_overlap_smoke(void);
extern "C" int zgeometry_model_is_fully_inside_clip_polygon_xy_smoke(void);
extern "C" int zgeometry_model_process_clip_patch_node_smoke(void);
extern "C" int zgeometry_clip_polygon_process_node_polygon_set_xy_smoke(void);
extern "C" int zgeometry_model_clip_patch_smoke(void);
extern "C" int zdeclient_qsand_build_smoke(void);
extern "C" int zgeometry_polygon_convexify_and_triangulate_smoke(void);
extern "C" int zgeometry_triangulate_hole_and_orientation_smoke(void);
extern "C" int zgeometry_polygon_snap_points_xy_if_near_smoke(void);
extern "C" int zgeometry_clip_polygon_snap_points_near_node_model_xy_smoke(void);
extern "C" int zmodel_material_write_gamez_smoke(void);
extern "C" int zmodel_material_read_gamez_smoke(void);
extern "C" int zmodel_dipool_write_to_stream_smoke(void);
extern "C" int zmodel_dipool_read_from_stream_smoke(void);
extern "C" int zmodel_dipool_read_entry_by_index_from_stream_smoke(void);
extern "C" int zmodel_material_and_di_clone_smoke(void);
extern "C" int zmodel_material_update_cycle_if_needed_smoke(void);
extern "C" int zmodel_scrolling_texture_update_smoke(void);
extern "C" int zmodel_render_point_queue_entry_smoke(void);
extern "C" int zmodel_init_smoke(void);
extern "C" int zmodel_render_node_hardware_flat_smoke(void);
extern "C" int zmodel_light_fog_fade_smoke(void);
extern "C" int zmodel_light_build_light_weights_smoke(void);
extern "C" int zmodel_light_point_in_polygon_init_smoke(void);
extern "C" int zmodel_light_set_active_lights_smoke(void);
extern "C" int zmodel_light_build_attr0_depth_fade_smoke(void);
extern "C" int zmodel_light_build_attr1_falloff_smoke(void);
extern "C" int zclass_world_build_active_light_list_smoke(void);
extern "C" int zclass_world_apply_pending_fog_settings_smoke(void);
extern "C" int zclass_world_settings_section_callbacks_smoke(void);
extern "C" int zclass_shutdown_core_smoke(void);
extern "C" int zclass_gwnode_update_smoke(void);
extern "C" int zclass_init_smoke(void);
extern "C" int zgame_options_find_option_smoke(void);
extern "C" int zgame_options_runtime_config_copy_default_smoke(void);
extern "C" int zopt_lookup_named_value_as_int_smoke(void);
extern "C" int zopt_read_scalar_value_as_int_smoke(void);
extern "C" int zopt_evaluate_profile_metric_condition_smoke(void);
extern "C" int zopt_select_profile_value_for_system_smoke(void);
extern "C" int zgame_options_load_from_registry_missing_smoke(void);
extern "C" int zgame_options_save_game_options_smoke(void);
extern "C" int zgame_options_shutdown_registry_context_smoke(void);
extern "C" int zgame_options_load_game_options_minimal_smoke(void);
extern "C" int zopt_eval_int_compare_op_smoke(void);
extern "C" int zopt_fullscreen_accessors_smoke(void);
extern "C" int zopt_toggle_hud_type_for_current_hw_mode_smoke(void);
extern "C" int zopt_network_enabled_accessor_smoke(void);
extern "C" int hud_sensor_objective_slot_reset_smoke(void);
extern "C" int hud_sensor_tracker_unload_objectives_smoke(void);
extern "C" int hud_sensor_tracker_get_objective_briefing_strings_smoke(void);
extern "C" int hud_sensor_tracker_load_race_checkpoint_meta_smoke(void);
extern "C" int hud_sensor_tracker_set_runtime_timer_sec_and_goal_value_smoke(void);
extern "C" int hud_sensor_tracker_load_mission_core_resources_smoke(void);
extern "C" int hud_sensor_tracker_load_objectives_from_path_smoke(void);
extern "C" int hud_sensor_tracker_load_objectives_from_zrd_smoke(void);
extern "C" int hud_sensor_tracker_load_mission_weather_fx_smoke(void);
extern "C" int zopt_view_rect_target_side_effects_smoke(void);
extern "C" int zopt_section_accessor_smoke(void);
extern "C" int zclipalt_set_source_rect_smoke(void);
extern "C" int zclipalt_set_target_rect_smoke(void);
extern "C" int zclipalt_remap_point_xy_in_place_smoke(void);
extern "C" int zclass_cls_di_segment_batch_vs_polygon_smoke(void);
extern "C" int zclass_cls_di_segment_batch_vs_polygon_uv_smoke(void);
extern "C" int zclass_cls_di_filter_regions_polygon_damage_mask_uv_smoke(void);
extern "C" int zclass_cls_di_filter_regions_against_polygon_smoke(void);
extern "C" int zclass_cls_di_frustum_test_and_pick_smoke(void);
extern "C" int zclass_cls_di_point_query_chain_smoke(void);
extern "C" int zclass_cls_di_snap_probe_point_y_to_best_candidate_smoke(void);
extern "C" int zclass_cls_di_segment_batch_recursive_smoke(void);
extern "C" int zclass_cls_di_segment_grid_window_smoke(void);
extern "C" int zclass_cls_di_probe_hit_batches_for_segments_smoke(void);
extern "C" int zclass_cls_di_set_stop_after_first_hit_smoke(void);
extern "C" int zhud_element_constructor_smoke(void);
extern "C" int zhud_element_copy_constructor_smoke(void);
extern "C" int zhud_element_set_timer_smoke(void);
extern "C" int zhud_circle_constructor_and_hit_test_smoke(void);
extern "C" int zhud_composite_panel_vector_clear_smoke(void);
extern "C" int zhud_panel_layout_entry_copy_construct_smoke(void);
extern "C" int zhud_panel_layout_entry_copy_assign_smoke(void);
extern "C" int zhud_panel_layout_entry_copy_assign_range_smoke(void);
extern "C" int zhud_panel_layout_entry_destroy_range_smoke(void);
extern "C" int zhud_panel_span_clear_smoke(void);
extern "C" int zhud_panel_span_copy_init_smoke(void);
extern "C" int zhud_panel_span_copy_from_smoke(void);
extern "C" int zhud_panel_span_destroy_and_free_smoke(void);
extern "C" int zhud_panel_span_insert_n_smoke(void);
extern "C" int zhud_panel_span_vec_insert_n_smoke(void);
extern "C" int zhud_credits_panel_destructor_smoke(void);
extern "C" int zhud_credits_panel_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_panel_destructor_callback_smoke(void);
extern "C" int zhud_scrolling_text_destructor_smoke(void);
extern "C" int zhud_scrolling_text_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_credits_panel_constructor_smoke(void);
extern "C" int zhud_scrolling_text_load_from_zrd_smoke(void);
extern "C" int zhud_scrolling_text_update_smoke(void);
extern "C" int zhud_scrolling_text_on_activate_reset_owner_fade_smoke(void);
extern "C" int zhud_scrolling_text_update_scroll_positions_smoke(void);
extern "C" int zhud_credits_panel_update_fade_and_exit_smoke(void);
extern "C" int zhud_composite_panel_vector_insert_copies_smoke(void);
extern "C" int zhud_composite_panel_constructor_with_entry_count_smoke(void);
extern "C" int zhud_composite_panel_layout_entries_smoke(void);
extern "C" int zhud_composite_panel_resize_entry_count_smoke(void);
extern "C" int zhud_composite_panel_resize_vector_relayout_smoke(void);
extern "C" int zhud_composite_panel_entry_copy_smoke(void);
extern "C" int zhud_flash_panel_compute_blend_color_smoke(void);
extern "C" int zhud_layout_shutdown_stub_smoke(void);
extern "C" int zhud_layout_hw_release_images_smoke(void);
extern "C" int zhud_layout_hw_update_objective_dirty_rect_smoke(void);
extern "C" int zhud_element_clip_and_invalidate_smoke(void);
extern "C" int zhud_element_visible_smoke(void);
extern "C" int zhud_element_update_smoke(void);
extern "C" int zhud_element_position_mutators_smoke(void);
extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void);
extern "C" int zhud_background_bind_primitive_node_to_element_smoke(void);
extern "C" int zhud_container_child_list_smoke(void);
extern "C" int zhud_container_destructor_core_smoke(void);
extern "C" int zhud_widget_constructor_smoke(void);
extern "C" int zhud_widget_default_ctor_thunk_smoke(void);
extern "C" int zhud_widget_invalidate_rect_smoke(void);
extern "C" int hud_ui_widget_draw_smoke(void);
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
extern "C" int zhud_cmd_dialog_on_command_selection_changed_smoke(void);
extern "C" int zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke(void);
extern "C" int zhud_cmd_dialog_rebuild_command_binding_lists_smoke(void);
extern "C" int zhud_cmd_dialog_constructor_smoke(void);
extern "C" int zhud_cmd_dialog_state_on_try_become_current_smoke(void);
extern "C" int zhud_cmd_dialog_select_group_relative_smoke(void);
extern "C" int zhud_cmd_set_list_widget_on_activate_smoke(void);
extern "C" int zhud_cmd_dialog_callback_navigation_smoke(void);
extern "C" int zhud_cmd_key_a_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_key_b_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_joy_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_mouse_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_key_a_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_key_b_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_joy_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_mouse_button_on_clear_binding_smoke(void);
extern "C" int zhud_cmd_dialog_select_command_relative_smoke(void);
extern "C" int zhud_cmd_reset_button_on_activate_smoke(void);
extern "C" int zhud_cmd_dialog_apply_primary_key_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_apply_secondary_key_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_apply_joystick_button_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_apply_mouse_button_rebind_smoke(void);
extern "C" int zhud_cmd_dialog_update_capture_state_idle_smoke(void);
extern "C" int zhud_message_box_leaf_handlers_smoke(void);
extern "C" int zhud_message_box_constructor_fallback_smoke(void);
extern "C" int zhud_message_box_run_modal_smoke(void);
extern "C" int zhud_message_box_destructor_smoke(void);
extern "C" int zhud_message_box_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_background_container_focus_smoke(void);
extern "C" int zhud_background_update_input_focus_smoke(void);
extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void);
extern "C" int zhud_background_cursor_widget_rebuild_captured_image_smoke(void);
extern "C" int zhud_background_cursor_widget_set_image_borrowed_refresh_smoke(void);
extern "C" int zhud_background_cursor_widget_set_capture_enabled_smoke(void);
extern "C" int zhud_background_video_widget_destructor_smoke(void);
extern "C" int zhud_background_constructor_smoke(void);
extern "C" int zhud_background_bind_widget_by_name_smoke(void);
extern "C" int zhud_background_load_zrd_and_section_null_root_smoke(void);
extern "C" int zhud_background_load_from_zrd_missing_path_smoke(void);
extern "C" int hud_ui_cheat_code_dialog_constructor_smoke(void);
extern "C" int hud_ui_cheat_code_dialog_destructor_smoke(void);
extern "C" int hud_ui_cheat_code_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_background_free_loaded_tree_roots_smoke(void);
extern "C" int zhud_background_video_widget_set_color_key_smoke(void);
extern "C" int zhud_background_video_widget_set_media_path_missing_smoke(void);
extern "C" int zhud_play_powerup_sfx_smoke(void);
extern "C" int zhud_background_set_enabled_smoke(void);
extern "C" int zhud_dialog_controller_blit_owned_surface_smoke(void);
extern "C" int zhud_font_style_constructor_smoke(void);
extern "C" int zhud_numeric_text_input_base_constructor_smoke(void);
extern "C" int zhud_widget_release_and_destructor_core_smoke(void);
extern "C" int zhud_triplet_panel_constructor_smoke(void);
extern "C" int zhud_triplet_panel_set_visible_count_smoke(void);
extern "C" int zhud_triplet_panel_draw_smoke(void);
extern "C" int zhud_triplet_panel_shutdown_items_stub_smoke(void);
extern "C" int zhud_triplet_panel_destructor_core_smoke(void);
extern "C" int zhud_text_input_destructor_core_smoke(void);
extern "C" int zhud_text_input_constructor_and_alloc_smoke(void);
extern "C" int zhud_mgr_objective_block_destructor_smoke(void);
extern "C" int hud_ui_net_exit_destroy_global_smoke(void);
extern "C" int hud_ui_net_exit_show_tick_smoke(void);
extern "C" int hud_ui_net_exit_constructor_smoke(void);
extern "C" int hud_ui_net_exit_create_global_smoke(void);
extern "C" int hud_ui_aux_overlay_text_lines_smoke(void);
extern "C" int zhud_mgr_destroy_sensor_window_null_smoke(void);
extern "C" int zhud_mgr_disable_hud_smoke(void);
extern "C" int zhud_mgr_enable_hud_smoke(void);
extern "C" int zhud_mgr_toggle_hud_smoke(void);
extern "C" int zhud_mgr_switch_active_dialog_smoke(void);
extern "C" int zhud_mgr_set_float_timer_visible_smoke(void);
extern "C" int zhud_mgr_hide_tracked_progress_meter_if_owner_matches_smoke(void);
extern "C" int zhud_mgr_target_update_selected_progress_meter_smoke(void);
extern "C" int zhud_mgr_set_aux_overlay_visible_smoke(void);
extern "C" int zhud_mgr_apply_hud_mode_switch_smoke(void);
extern "C" int zhud_mgr_ensure_hud_loaded_minimal_smoke(void);
extern "C" int zhud_slot_destructors_smoke(void);
extern "C" int zhud_slot_draw_smoke(void);
extern "C" int zhud_stats_list_element_update_smoke(void);
extern "C" int zhud_counter_constructor_smoke(void);
extern "C" int zhud_counter_update_layout_position_smoke(void);
extern "C" int zhud_counter_apply_from_layout_node_smoke(void);
extern "C" int zhud_counter_release_state_images_smoke(void);
extern "C" int zhud_message_release_images_smoke(void);
extern "C" int zhud_message_set_value_if_owner_matches_smoke(void);
extern "C" int zhud_message_update_selected_weapon_display_smoke(void);
extern "C" int zhud_message_constructor_smoke(void);
extern "C" int zhud_message_rebuild_weapon_layout_smoke(void);
extern "C" int zhud_message_load_weapon_layout_from_node_smoke(void);
extern "C" int zhud_message_destructors_smoke(void);
extern "C" int zhud_shield_message_widget_destructor_smoke(void);
extern "C" int zhud_mgr_sensor_set_shield_message_ratio_smoke(void);
extern "C" int zhud_shield_message_widget_apply_layout_smoke(void);
extern "C" int zhud_bar_and_meter_constructor_smoke(void);
extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void);
extern "C" int zhud_polyline_draw_and_slider_update_smoke(void);
extern "C" int zhud_panel_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_util_free_field_ptr_smoke(void);
extern "C" int zhud_cmd_binding_entry_copy_range_smoke(void);
extern "C" int zhud_cmd_binding_destroy_range_smoke(void);
extern "C" int zhud_std_ptr_vector_clear_no_op_destroy_smoke(void);
extern "C" int zhud_cmd_command_list_destructor_smoke(void);
extern "C" int zhud_cmd_command_list_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_cmd_key_a_button_destructor_smoke(void);
extern "C" int zhud_cmd_key_b_button_destructor_smoke(void);
extern "C" int zhud_cmd_joy_button_destructor_smoke(void);
extern "C" int zhud_cmd_mouse_button_destructor_smoke(void);
extern "C" int zhud_cmd_bind_button_base_destructor_core_smoke(void);
extern "C" int zhud_cmd_dialog_destructor_smoke(void);
extern "C" int zhud_cmd_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_options_dialog_constructor_smoke(void);
extern "C" int zhud_options_dialog_destructor_core_smoke(void);
extern "C" int zhud_options_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_options_panel_lighting_init_from_options_smoke(void);
extern "C" int zhud_options_panel_lighting_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_perspective_init_from_options_smoke(void);
extern "C" int zhud_options_panel_perspective_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_full_hud_init_from_options_smoke(void);
extern "C" int zhud_options_panel_object_detail_init_from_options_smoke(void);
extern "C" int zhud_options_panel_object_detail_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_texture_memory_init_from_options_smoke(void);
extern "C" int zhud_options_panel_texture_memory_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_effects_init_from_options_smoke(void);
extern "C" int zhud_options_panel_effects_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_active_init_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_active_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_music_enable_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_music_enable_on_activate_smoke(void);
extern "C" int zhud_options_panel_music_volume_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_music_volume_on_activate_smoke(void);
extern "C" int zhud_options_panel_resolution_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_resolution_on_activate_smoke(void);
extern "C" int zhud_options_panel_sound_quality_init_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_quality_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_volume_sync_from_options_smoke(void);
extern "C" int zhud_options_panel_sound_volume_on_activate_smoke(void);
extern "C" int zhud_cmd_dialog_state_lifecycle_smoke(void);
extern "C" int zhud_cmd_dialog_state_queue_enter_smoke(void);
extern "C" int zhud_cmd_dialog_state_on_deactivate_smoke(void);
extern "C" int zhud_panel_destructor_thunk_smoke(void);
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
extern "C" int zhud_objective_refresh_counter_text_smoke(void);
extern "C" int zhud_objective_update_meter_xpoints_smoke(void);
extern "C" int zhud_objective_tick_meter_fill_animation_smoke(void);
extern "C" int zhud_layout_node_read_rect_offset_and_size_smoke(void);
extern "C" int zhud_layout_node_read_rect_smoke(void);
extern "C" int zhud_layout_node_read_int3_smoke(void);
extern "C" int zhud_layout_node_apply_text_label_smoke(void);
extern "C" int zhud_layout_node_apply_image_widget_smoke(void);
extern "C" int zhud_layout_base_load_type_i_from_zar_root_smoke(void);
extern "C" int zhud_layout_hw_load_type_ii_from_zar_root_smoke(void);
extern "C" int zhud_timer_panel_float_constructor_smoke(void);
extern "C" int zhud_triplet_constructor_smoke(void);
extern "C" int zhud_triplet_interpolate_layout_smoke(void);
extern "C" int zhud_triplet_destructor_core_smoke(void);
extern "C" int zhud_triplet_is_local_player_first_entry_smoke(void);
extern "C" int zhud_nanite_panel_init_layout_smoke(void);
extern "C" int zhud_triplet_scoreboard_entry_update_smoke(void);
extern "C" int zhud_stats_list_destructors_smoke(void);
extern "C" int zhud_string_menu_destructor_core_smoke(void);
extern "C" int zhud_bar_set_point_xy_smoke(void);
extern "C" int zhud_layout_node_apply_meter_quad_smoke(void);
extern "C" int zhud_layout_node_apply_corner_text_quad_smoke(void);
extern "C" int zhud_text_stack_constructors_smoke(void);
extern "C" int zhud_text_stack_set_font_all_smoke(void);
extern "C" int zhud_text_stack_push_line_smoke(void);
extern "C" int zhud_text_stack_clear_and_enable_smoke(void);
extern "C" int zhud_text_stack_clear_and_disable_smoke(void);
extern "C" int zhud_text_stack_destructor_core_smoke(void);
extern "C" int zhud_timed_task_remove_from_active_list_smoke(void);
extern "C" int zhud_timed_task_run_immediate_action_smoke(void);
extern "C" int zhud_timed_task_tick_active_list_smoke(void);
extern "C" int zhud_mgr_update_frame_smoke(void);
extern "C" int zhud_loading_checkpoint_init_table_smoke(void);
extern "C" int zhud_loading_checkpoint_advance_and_log_smoke(void);
extern "C" int zhud_sensor_viewport_rect_smoke(void);
extern "C" int zhud_sensor_get_fx_rect_smoke(void);
extern "C" int zhud_layout_apply_viewport_rect_smoke(void);
extern "C" int zhud_objective_update_smoke(void);
extern "C" int zhud_objective_begin_smoke(void);
extern "C" int zhud_objective_start_hide_smoke(void);
extern "C" int zhud_objective_show_smoke(void);
extern "C" int hud_sensor_tracker_show_objective_pickup_info_smoke(void);
extern "C" int hud_sensor_tracker_objective_panel_visible_smoke(void);
extern "C" int hud_sensor_tracker_reset_hud_for_mission_start_smoke(void);
extern "C" int hud_sensor_tracker_update_map_scale_lerp_smoke(void);
extern "C" int hud_sensor_tracker_project_world_points_smoke(void);
extern "C" int hud_line_clip_current_bounds_smoke(void);
extern "C" int hud_recti_clip_segment_helpers_smoke(void);
extern "C" int hud_sensor_tracker_draw_diamond_marker_smoke(void);
extern "C" int hud_sensor_tracker_save_marker_leaf_smoke(void);
extern "C" int hud_sensor_tracker_save_state_marker_smoke(void);
extern "C" int hud_sensor_tracker_update_smoke(void);
extern "C" int hud_sensor_map_node_draw_projected_path_smoke(void);
extern "C" int hud_sensor_map_node_draw_on_tracker_smoke(void);
extern "C" int hud_weather_fx_constructor_smoke(void);
extern "C" int hud_weather_fx_derived_constructors_smoke(void);
extern "C" int zhud_mgr_viewport_activation_smoke(void);
extern "C" int zhud_mgr_project_point_to_normalized_clamped_smoke(void);
extern "C" int zhud_mgr_update_target_reticle_smoke(void);
extern "C" int zhud_mgr_sensor_place_track_counter_widget_smoke(void);
extern "C" int zhud_mgr_sensor_place_track_marker_smoke(void);
extern "C" int zhud_mgr_sensor_update_markers_and_progress_smoke(void);
extern "C" int zhud_mgr_init_layouts_reentry_smoke(void);
extern "C" int zhud_mgr_shutdown_resources_smoke(void);
extern "C" int zhud_sensor_track_list_add_smoke(void);
extern "C" int zclass_node_propagate_transform_dirty_smoke(void);
extern "C" int player_add_scaled_hud_counter_smoke(void);
extern "C" int zutil_save_game_state_list_smoke(void);
extern "C" int zclass_object3d_visible_and_color_smoke(void);
extern "C" int zclass_object3d_alpha_scale_and_lit_smoke(void);
extern "C" int zclass_object3d_transform_getters_smoke(void);
extern "C" int zclass_object3d_transform_setters_smoke(void);
extern "C" int zclass_object3d_reset_transform_dirty_smoke(void);
extern "C" int zclass_model_ref_lerp_queue_reset_smoke(void);
extern "C" int zclass_model_ref_lerp_queue_add_smoke(void);
extern "C" int zclass_model_ref_lerp_queue_update_smoke(void);
extern "C" int zclass_type_list_alloc_and_insert_smoke(void);
extern "C" int zclass_animate_update_smoke(void);
extern "C" int zclass_sequence_new_add_child_smoke(void);
extern "C" int zclass_sequence_update_smoke(void);
extern "C" int zclass_typelist_update_bucket_smoke(void);
extern "C" int zclass_typelist_update_all_buckets_smoke(void);
extern "C" int zclass_typelist_update_sequences_smoke(void);
extern "C" int zclass_typelist_update_animations_smoke(void);
extern "C" int zclass_gwnode_update_all_smoke(void);
extern "C" int zclass_object3d_set_matrix_smoke(void);
extern "C" int zclass_node_free_and_deferred_work_smoke(void);
extern "C" int zclass_alloc_node_from_free_list_smoke(void);
extern "C" int zclass_node_flag16_flag17_smoke(void);
extern "C" int zclass_node_metadata_accessors_smoke(void);
extern "C" int zclass_copy_node_display_instance_smoke(void);
extern "C" int zclass_copy_node_base_data_smoke(void);
extern "C" int zclass_copy_node_unimplemented_stubs_smoke(void);
extern "C" int zclass_copy_camera_node_smoke(void);
extern "C" int zclass_copy_object3d_and_lod_smoke(void);
extern "C" int zclass_copy_node_dispatch_and_wrappers_smoke(void);
extern "C" int zclass_node_action_callback_smoke(void);
extern "C" int zclass_node_priority_smoke(void);
extern "C" int zclass_node_pick_flag_accessors_smoke(void);
extern "C" int zclass_node_extra_flag_setters_smoke(void);
extern "C" int zclass_node_vertex_alpha_and_root_smoke(void);
extern "C" int zclass_node_world_child_smoke(void);
extern "C" int zclass_gwnode_update_tree_smoke(void);
extern "C" int zclass_gwnode_build_node_to_ancestor_matrix_smoke(void);
extern "C" int zclass_gwnode_get_world_position_smoke(void);
extern "C" int zclass_world_set_origin_smoke(void);
extern "C" int zclass_world_set_size_smoke(void);
extern "C" int zclass_world_partition_tolerance_smoke(void);
extern "C" int zclass_world_max_dec_features_smoke(void);
extern "C" int zclass_world_virtual_partition_statics_smoke(void);
extern "C" int zclass_world_add_child_at_grid_smoke(void);
extern "C" int zclass_world_remove_light_sound_smoke(void);
extern "C" int zclass_child_generic_link_smoke(void);
extern "C" int zclass_child_generic_remove_smoke(void);
extern "C" int zclass_remove_wrapper_matrix_smoke(void);
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
extern "C" int gamez_read_retail_m1_zbd_smoke(void);
extern "C" int gamez_reload_display_instances_smoke(void);
extern "C" int zclass_try_free_node_smoke(void);
extern "C" int zdi_ref_and_pool_free_smoke(void);
extern "C" int zdi_reset_current_variant_smoke(void);
extern "C" int zdi_set_current_variant_smoke(void);
extern "C" int zdi_entry_material_helpers_smoke(void);
extern "C" int zmodel_set_di_texture_world_per_meter_smoke(void);
extern "C" int zclass_set_display_instance_smoke(void);
extern "C" int zclass_remove_dispatch_smoke(void);
extern "C" int zclass_destroy_node_recursive_display_smoke(void);
extern "C" int zclass_find_node_recursive_by_name_smoke(void);
extern "C" int zclass_object3d_delete_node_smoke(void);
extern "C" int zclass_world_new_smoke(void);
extern "C" int zclass_world_free_virtual_area_partitions_smoke(void);
extern "C" int zclass_world_set_virtual_area_partition_smoke(void);
extern "C" int zclass_world_get_area_partition_at_grid_smoke(void);
extern "C" int zclass_world_to_grid_coords_clamped_smoke(void);
extern "C" int zclass_camera_convex_hull_xz_smoke(void);
extern "C" int zclass_cls_di_region_filter_mesh_faces_smoke(void);
extern "C" int zclass_camera_build_frustum_grid_tiles_from_params_smoke(void);
extern "C" int zclass_camera_build_frustum_grid_tiles_smoke(void);
extern "C" int zclass_render_traverse_dispatch_smoke(void);
extern "C" int zclass_world_animate_delete_node_smoke(void);
extern "C" int zclass_sound_leaf_smoke(void);
extern "C" int zclass_object3d_init_smoke(void);
extern "C" int zclass_window_new_smoke(void);
extern "C" int zclass_display_init_smoke(void);
extern "C" int zclass_lod_leaf_smoke(void);
extern "C" int zclass_light_new_smoke(void);
extern "C" int zclass_damage_handler_smoke(void);
extern "C" int zclass_camera_view_distance_smoke(void);
extern "C" int zclass_camera_sync_view_context_positions_smoke(void);
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
extern "C" int zinput_keyboard_dik_ascii_smoke(void);
extern "C" int zinput_mouse_client_size_center_smoke(void);
extern "C" int zinput_mouse_apply_and_recenter_cursor_smoke(void);
extern "C" int zinput_mouse_coop_level_flags_smoke(void);
extern "C" int zinput_mouse_button_transition_state_smoke(void);
extern "C" int zinput_mouse_apply_accumulated_delta_smoke(void);
extern "C" int zinput_mouse_keyboard_small_accessors_smoke(void);
extern "C" int zinput_bindmap_name_tables_smoke(void);
extern "C" int zinput_bindmap_context_smoke(void);
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
extern "C" int znetwork_local_identity_smoke(void);
extern "C" int znetwork_destroy_cached_local_player_smoke(void);
extern "C" int znetwork_dplay_report_error_smoke(void);
extern "C" int znetwork_dplay_close_release_smoke(void);
extern "C" int znetwork_dispatch_handler_list_smoke(void);
extern "C" int znetwork_register_packet_handler_smoke(void);
extern "C" int znetwork_dispatch_packet_to_handlers_smoke(void);
extern "C" int znetwork_player_record_accessors_smoke(void);
extern "C" int znetwork_remove_player_record_by_key_smoke(void);
extern "C" int znetwork_alloc_free_player_color_index_smoke(void);
extern "C" int znetwork_host_send_player_color_assignments_packet_smoke(void);
extern "C" int znetwork_dplay_pump_incoming_messages_smoke(void);
extern "C" int znetwork_dplay_receive_pending_messages_smoke(void);
extern "C" int znetwork_dplay_enum_players_smoke(void);
extern "C" int znetwork_packet_send_wrappers_smoke(void);
extern "C" int pickup_send_pkt11_delta_smoke(void);
extern "C" int pickup_send_pkt11_create_delta_smoke(void);
extern "C" int pickup_reconcile_spawn_lists_smoke(void);
extern "C" int pickup_send_pkt12_airdrop_spawn_chute_relay_smoke(void);
extern "C" int znetwork_session_status_fields_smoke(void);
extern "C" int znetwork_shutdown_session_runtime_smoke(void);
extern "C" int zreader_named_int_lookup_smoke(void);
extern "C" int zreader_get_named_node_smoke(void);
extern "C" int zreader_named_string_float_lookup_smoke(void);
extern "C" int zreader_global_string_prefix_index_smoke(void);
extern "C" int zrndr_global_string_table_load_dynamic_entries_smoke(void);
extern "C" int zreader_load_node_from_archive_smoke(void);
extern "C" int zreader_file_exists_and_list_create_smoke(void);
extern "C" int zreader_archive_list_and_search_paths_smoke(void);
extern "C" int zreader_zrdr_free_search_path_list_smoke(void);
extern "C" int zreader_prealloc_and_pop_front_smoke(void);
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
extern "C" int zmath_matrix_stack_and_direction_smoke(void);
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
extern "C" int recoil_app_get_message_map_smoke(void);
extern "C" int recoil_app_accessor_and_skip_wait_smoke(void);
extern "C" int recoil_app_activate_existing_instance_absent_smoke(void);
extern "C" int recoil_app_pre_translate_message_smoke(void);
extern "C" int recoil_app_init_std_log_files_smoke(void);
extern "C" int recoil_app_fatal_error_and_exit_smoke(void);
extern "C" int crt_atexit_import_provider_smoke(void);
extern "C" int recoil_app_get_current_state_smoke(void);
extern "C" int recoil_app_state_queue_block_init_from_cursor_smoke(void);
extern "C" int recoil_app_state_queue_grow_chunk_base_list_smoke(void);
extern "C" int recoil_app_queue_switch_current_state_smoke(void);
extern "C" int hud_sensor_queue_mission_fmv_state_for_mission_id_smoke(void);
extern "C" int hud_sensor_save_and_queue_mission_state_smoke(void);
extern "C" int recoil_app_queue_push_state_smoke(void);
extern "C" int hud_ui_callback_queue_cheat_code_state_smoke(void);
extern "C" int recoil_app_queue_exit_current_state_smoke(void);
extern "C" int recoil_app_run_current_state_quit_smoke(void);
extern "C" int recoil_app_run_queue_transitions_smoke(void);
extern "C" int hud_ui_save_game_primary_action_button_on_activate_smoke(void);
extern "C" int hud_ui_load_game_dialog_on_primary_action_smoke(void);
extern "C" int hud_ui_load_game_dialog_process_dialog_result_smoke(void);
extern "C" int hud_ui_load_game_primary_action_button_on_activate_smoke(void);
extern "C" int hud_ui_save_load_process_dialog_result_smoke(void);
extern "C" int hud_ui_save_game_dialog_init_layout_smoke(void);
extern "C" int hud_ui_load_game_dialog_constructor_smoke(void);
extern "C" int hud_ui_load_game_dialog_destructor_smoke(void);
extern "C" int hud_ui_load_game_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_state_save_load_transition_on_try_become_current_smoke(void);
extern "C" int recoil_state_save_load_transition_on_update_should_quit_smoke(void);
extern "C" int recoil_state_save_load_transition_lifecycle_smoke(void);
extern "C" int hud_ui_main_menu_dialog_save_load_checks_smoke(void);
extern "C" int recoil_state_save_load_transition_queue_dialogs_smoke(void);
extern "C" int recoil_app_start_engine_and_queue_startup_state_smoke(void);
extern "C" int recoil_app_load_zbd_and_start_engine_smoke(void);
extern "C" int recoil_app_load_zbd_and_setup_sensor_tracker_smoke(void);
extern "C" int recoil_app_initialize_display_failure_smoke(void);
extern "C" int recoil_app_on_idle_or_dispatch_smoke(void);
extern "C" int recoil_app_mfc_ole_module_constructor_smoke(void);
extern "C" int recoil_app_mfc_ole_module_destructor_smoke(void);
extern "C" int recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_app_play_state_constructor_smoke(void);
extern "C" int recoil_app_play_state_on_wnd_activate_smoke(void);
extern "C" int recoil_app_play_state_tick_and_render_frame_quit_smoke(void);
extern "C" int recoil_app_play_state_on_update_should_quit_transition_smoke(void);
extern "C" int recoil_app_play_state_on_resume_cd_disabled_smoke(void);
extern "C" int recoil_app_play_state_on_deactivate_skip_gameplay_smoke(void);
extern "C" int recoil_app_fmv_state_constructor_smoke(void);
extern "C" int recoil_app_fmv_state_on_idle_or_dispatch_smoke(void);
extern "C" int recoil_app_intro_fmv_on_try_become_current_smoke(void);
extern "C" int recoil_app_intro_fmv_on_update_should_quit_smoke(void);
extern "C" int recoil_app_intro_fmv_on_deactivate_smoke(void);
extern "C" int recoil_app_main_menu_prep_on_try_become_current_smoke(void);
extern "C" int recoil_app_main_menu_prep_on_update_should_quit_smoke(void);
extern "C" int recoil_app_attract_fmv_on_try_become_current_smoke(void);
extern "C" int recoil_app_attract_fmv_on_update_should_quit_smoke(void);
extern "C" int recoil_app_attract_fmv_on_deactivate_smoke(void);
extern "C" int recoil_app_constructor_destructor_smoke(void);
extern "C" int recoil_app_istate_destructor_smoke(void);
extern "C" int recoil_app_fmv_state_destructor_smoke(void);
extern "C" int recoil_app_mission_fmv_state_destructor_smoke(void);
extern "C" int recoil_app_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_state_base_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_state_credits_destructor_smoke(void);
extern "C" int recoil_state_credits_constructor_smoke(void);
extern "C" int recoil_state_credits_on_wnd_activate_smoke(void);
extern "C" int recoil_state_credits_on_try_become_current_smoke(void);
extern "C" int recoil_state_credits_on_deactivate_smoke(void);
extern "C" int recoil_state_credits_queue_push_smoke(void);
extern "C" int recoil_state_confirm_quit_destructor_smoke(void);
extern "C" int recoil_state_confirm_quit_queue_enter_smoke(void);
extern "C" int hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke(void);
extern "C" int hud_ui_credits_quit_button_on_activate_smoke(void);
extern "C" int hud_ui_cheat_text_input_on_activate_smoke(void);
extern "C" int hud_ui_callback_queue_exit_current_state_smoke(void);
extern "C" int hud_ui_background_confirm_quit_lifecycle_smoke(void);
extern "C" int recoil_state_confirm_quit_on_try_become_current_smoke(void);
extern "C" int recoil_state_confirm_quit_static_init_smoke(void);
extern "C" int recoil_state_cheat_code_constructor_smoke(void);
extern "C" int recoil_state_cheat_code_destructor_smoke(void);
extern "C" int recoil_state_cheat_code_scalar_deleting_destructor_smoke(void);
extern "C" int recoil_state_cheat_code_static_init_thunks_smoke(void);
extern "C" int recoil_state_cheat_code_on_try_become_current_smoke(void);
extern "C" int recoil_state_cheat_code_on_deactivate_smoke(void);
extern "C" int hud_ui_confirm_quit_ok_button_on_activate_smoke(void);
extern "C" int recoil_state_main_menu_transition_constructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_destructor_smoke(void);
extern "C" int recoil_state_main_menu_transition_scalar_deleting_destructor_smoke(void);
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
extern "C" int zsnd_backend_init_directsound_provider_smoke(void);
extern "C" int zsnd_backend_init_a3d_provider_smoke(void);
extern "C" int zsnd_cached_directsound_get_caps_smoke(void);
extern "C" int zsnd_options_cpu_and_cached_directsound_smoke(void);
extern "C" int zsnd_cd_reset_track_state_smoke(void);
extern "C" int zsnd_cd_is_stereo_aux_enabled_smoke(void);
extern "C" int zsnd_cd_get_volume_smoke(void);
extern "C" int zsnd_cd_set_volume_smoke(void);
extern "C" int zsnd_cd_not_ready_playback_smoke(void);
extern "C" int zsnd_cd_playback_mci_commands_smoke(void);
extern "C" int zsnd_cd_on_mci_notify_loop_smoke(void);
extern "C" int zsnd_cd_init_ready_guard_smoke(void);
extern "C" int zsnd_cd_get_track_count_ready_guard_smoke(void);
extern "C" int zsnd_cd_shutdown_track_list_smoke(void);
extern "C" int zsnd_sample_set_get_sample_at_smoke(void);
extern "C" int zsnd_sample_set_registry_add_entry_smoke(void);
extern "C" int zsnd_find_sample_by_name_smoke(void);
extern "C" int zsnd_gain_scale_to_directsound_attenuation_smoke(void);
extern "C" int zsnd_is_muted_smoke(void);
extern "C" int zsnd_sample_play_simple_smoke(void);
extern "C" int zsnd_global_volume_and_flag_helpers_smoke(void);
extern "C" int zsnd_sample_acquire_play_handle_smoke(void);
extern "C" int zsnd_system_named_sets_syntax_smoke(void);
extern "C" int zsnd_system_legacy_sets_syntax_smoke(void);
extern "C" int zsnd_system_init_gate_and_missing_config_smoke(void);
extern "C" int zsnd_preinitialize_runtime_state_smoke(void);
extern "C" int zsnd_group_load_config_block_smoke(void);
extern "C" int zsnd_group_load_and_queue_smoke(void);
extern "C" int zsnd_create_queued_streaming_sample_smoke(void);
extern "C" int zsnd_stream_mgr_ensure_init_smoke(void);
extern "C" int zsnd_stream_request_queue_smoke(void);
extern "C" int zsnd_stream_mgr_shutdown_lists_smoke(void);
extern "C" int zsnd_backend_shutdown_release_smoke(void);
extern "C" int zsnd_play_handle_stop_if_active_smoke(void);
extern "C" int zsnd_sample_stop_active_voices_if_playing_smoke(void);
extern "C" int zsnd_snapshot_create_from_active_samples_smoke(void);
extern "C" int zsnd_apply_mute_state_to_active_voices_smoke(void);
extern "C" int zsnd_snapshot_restore_all_with_global_volume_delta_smoke(void);
extern "C" int zsnd_snapshot_destroy_smoke(void);
extern "C" int zsnd_snapshot_stop_all_if_playing_smoke(void);
extern "C" int zsnd_play_handle_update3d_a3d_smoke(void);
extern "C" int zsnd_update_listener_state_smoke(void);
extern "C" int zsnd_play_handle_update3d_directsound_smoke(void);
extern "C" int zsnd_play_handle_set_freq_scaled_smoke(void);
extern "C" int zsnd_play_handle_set_enable_scale_smoke(void);
extern "C" int zsnd_play_handle_try_disable_managed_smoke(void);
extern "C" int zsnd_sample_play_a3d_simple_direct_smoke(void);
extern "C" int zsnd_sample_play_a3d_worldpos_smoke(void);
extern "C" int zsnd_sample_destroy_owned_data_smoke(void);
extern "C" int zsnd_sample_destroy_smoke(void);
extern "C" int zsnd_sample_set_playback_event_handler_smoke(void);
extern "C" int zsnd_fade_entry_backend_and_dispatch_smoke(void);
extern "C" int zsnd_fade_active_list_tick_compacts_smoke(void);
extern "C" int zsnd_fade_list_cursor_helpers_smoke(void);
extern "C" int zsnd_fade_lists_stop_all_shutdown_smoke(void);
extern "C" int zsnd_tick_backend_markers_smoke(void);
extern "C" int zsnd_sample_set_init_by_name_empty_smoke(void);
extern "C" int zsnd_sample_set_init_loose_file_smoke(void);
extern "C" int zsnd_wave_data_load_parse_reset_smoke(void);
extern "C" int zsnd_wave_data_parse_chunks_smoke(void);
extern "C" int zsnd_wave_data_load_parse_edges_smoke(void);
extern "C" int zsnd_wave_data_archive_load_smoke(void);
extern "C" int zsnd_sample_set_load_samples_from_index_archive_smoke(void);
extern "C" int zsnd_sample_backend_buffer_lock_unlock_smoke(void);
extern "C" int zsnd_sample_init_from_wave_data_directsound_smoke(void);
extern "C" int zsnd_sample_init_from_wave_data_a3d_smoke(void);
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
extern "C" int zutil_zar_register_section_handler_smoke(void);
extern "C" int zutil_zbd_init_smoke(void);
extern "C" int zutil_zbd_destroy_global_manager_smoke(void);
extern "C" int zutil_zbd_section_handler_compare_sort_order_smoke(void);
extern "C" int zutil_zbd_section_handler_invoke_pre_load_smoke(void);
extern "C" int zutil_zbd_section_handler_invoke_data_ready_smoke(void);
extern "C" int zutil_zbd_manager_sort_section_handlers_smoke(void);
extern "C" int zutil_zbd_manager_load_entries_smoke(void);
extern "C" int zutil_zbd_load_entries_global_smoke(void);
extern "C" int zutil_zbd_manager_load_zar_file_smoke(void);
extern "C" int zutil_zar_load_file_global_smoke(void);
extern "C" int hud_sensor_register_mission_sections_smoke(void);
extern "C" int hud_sensor_mission_identity_smoke(void);
extern "C" int hud_sensor_map_node_basics_smoke(void);
extern "C" int hud_sensor_objective_marker_enable_color_smoke(void);
extern "C" int hud_sensor_find_first_incomplete_objective_smoke(void);
extern "C" int hud_sensor_map_bounds_and_save_state_smoke(void);
extern "C" int hud_sensor_map_remove_and_shutdown_smoke(void);
extern "C" int hud_sensor_map_overlay_toggle_smoke(void);
extern "C" int hud_sensor_tracker_load_map_paths_smoke(void);
extern "C" int hud_sensor_reset_mission_state_smoke(void);
extern "C" int hud_sensor_shutdown_mission_gameplay_systems_early_smoke(void);
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
extern "C" int zeffect_handle_light_anim_event_smoke(void);
extern "C" int zeffect_handle_fog_event_smoke(void);
extern "C" int zeffect_handle_camera_params_event_smoke(void);
extern "C" int zeffect_animate_camera_params_over_time_smoke(void);
extern "C" int zeffect_handle_rotation_event_smoke(void);
extern "C" int zeffect_handle_position_event_smoke(void);
extern "C" int zeffect_handle_activate_event_smoke(void);
extern "C" int zeffect_handle_node_anim_event_smoke(void);
extern "C" int zeffect_animate_node_over_time_smoke(void);
extern "C" int zeffect_handle_node_scale_event_smoke(void);
extern "C" int zeffect_handle_screen_fx_events_smoke(void);
extern "C" int zeffect_anim_keyframe_sample_smoke(void);
extern "C" int zeffect_parent_attach_detach_events_smoke(void);
extern "C" int zeffect_anim_activation_prereqs_smoke(void);
extern "C" int hud_sensor_run_start_anims_from_zrd_smoke(void);
extern "C" int zeffect_anim_activation_record_queue_smoke(void);
extern "C" int zeffect_anim_process_activation_record_smoke(void);
extern "C" int zeffect_handle_emitter_reset_event_smoke(void);
extern "C" int zeffect_handle_emitter_loop_event_smoke(void);
extern "C" int zeffect_handle_emitter_stop_event_smoke(void);
extern "C" int zeffect_surface_control_events_smoke(void);
extern "C" int zeffect_conditional_ref_pos_smoke(void);
extern "C" int zeffect_skip_conditional_chain_smoke(void);
extern "C" int zeffect_trace_upward_hit_smoke(void);
extern "C" int zeffect_set_variant_override_packed_ids_if_complete_smoke(void);
extern "C" int zeffect_handle_conditional_chain_event_smoke(void);
extern "C" int zeffect_marker_and_callback_event_smoke(void);
extern "C" int zeffect_handle_top_message_event_smoke(void);
extern "C" int zeffect_anim_set_on_state_done_callback_smoke(void);
extern "C" int zeffect_world_and_zbd_setters_smoke(void);
extern "C" int zeffect_anim_load_zbd_minimal_smoke(void);
extern "C" int zeffect_anim_load_and_instantiate_minimal_smoke(void);
extern "C" int zeffect_tick_reset_delay_callbacks_smoke(void);
extern "C" int zeffect_anim_debug_frame_tag_smoke(void);
extern "C" int zeffect_anim_find_next_async_entry_smoke(void);
extern "C" int zeffect_anim_get_root_node_or_null_smoke(void);
extern "C" int zeffect_anim_capture_node_states_smoke(void);
extern "C" int zeffect_anim_restore_node_states_smoke(void);
extern "C" int zeffect_anim_reset_for_node_smoke(void);
extern "C" int zeffect_anim_runtime_sequence_group_smoke(void);
extern "C" int zeffect_anim_init_shutdown_smoke(void);
extern "C" int zeffect_shutdown_all_smoke(void);
extern "C" int zeffect_find_template_index_by_name_smoke(void);
extern "C" int zeffect_find_node_user_data_recursive_smoke(void);
extern "C" int zeffect_clone_runtime_entry_from_template_smoke(void);
extern "C" int zeffect_acquire_runtime_entry_by_index_smoke(void);
extern "C" int zeffect_compute_distance_sq_to_listener_smoke(void);
extern "C" int zeffect_activate_runtime_entry_at_position_smoke(void);
extern "C" int zeffect_runtime_node_action_callback_smoke(void);
extern "C" int zeffect_spawn_runtime_instance_at_smoke(void);
extern "C" int zeffect_init_smoke(void);
extern "C" int zeffect_init_from_path_smoke(void);
extern "C" int zdeclient_set_camera_node_smoke(void);
extern "C" int zdeclient_load_material_from_texture_path_smoke(void);
extern "C" int zdeclient_load_config_resources_smoke(void);
extern "C" int zdeclient_crater_init_event_template_defaults_smoke(void);
extern "C" int zdeclient_qsand_copy_event_template_defaults_smoke(void);
extern "C" int zdeclient_write_feature_sections_to_zar_smoke(void);
extern "C" int zdeclient_shutdown_globals_smoke(void);
extern "C" int zdeclient_map_tree_iter_next_node_ref_smoke(void);
extern "C" int zdeclient_map_tree_iter_prev_node_ref_smoke(void);
extern "C" int zdeclient_map_tree_insert_at_smoke(void);
extern "C" int zdeclient_map_tree_find_or_insert_key_smoke(void);
extern "C" int zdeclient_submit_feature_geometry_smoke(void);
extern "C" int zdeclient_feature_entry_copy_fill_helpers_smoke(void);
extern "C" int zdeclient_append_feature_entry_smoke(void);
extern "C" int zdeclient_clear_feature_display_nodes_smoke(void);
extern "C" int zdeclient_feature_leaf_helpers_smoke(void);
extern "C" int zdeclient_feature_init_helpers_smoke(void);
extern "C" int zdeclient_create_feature_node_from_partition_smoke(void);
extern "C" int zdeclient_qsand_create_feature_smoke(void);
extern "C" int zdeclient_crater_build_and_create_feature_smoke(void);
extern "C" int zdeclient_crater_instance_event_smoke(void);
extern "C" int zdeclient_crater_instance_event_maybe_relay_smoke(void);
extern "C" int zdeclient_crater_net_relay_callback_smoke(void);
extern "C" int zdeclient_crater_execute_smoke(void);
extern "C" int zdeclient_qsand_net_relay_callback_smoke(void);
extern "C" int zdeclient_apply_feature_entry_reload_smoke(void);
extern "C" int zdeclient_dispatch_feature_event_templates_smoke(void);
extern "C" int zdeclient_qsand_instance_event_maybe_relay_smoke(void);
extern "C" int zgeometry_clip_patch_output_apply_node_di_pairs_smoke(void);
extern "C" int zloc_message_lookup_failure_smoke(void);
extern "C" int zloc_load_unload_messages_dll_smoke(void);
extern "C" int directdraw_enumerate_import_provider_smoke(void);
extern "C" int zvid_pack_color_rgb_smoke(void);
extern "C" int zvideo_draw_noise_rect_smoke(void);
extern "C" int zvideo_pixel_pack_setup_smoke(void);
extern "C" int zvideo_pixel_pack_getters_smoke(void);
extern "C" int zvideo_set_renderer_type_smoke(void);
extern "C" int zvideo_pending_dither_enable_smoke(void);
extern "C" int zvideo_pending_wireframe_state_smoke(void);
extern "C" int zvideo_dd3d_begin_scene_flush_pending_smoke(void);
extern "C" int zvideo_sw_render_frame_smoke(void);
extern "C" int zclass_camera_render_scene_smoke(void);
extern "C" int zclass_list_render_active_cameras_smoke(void);
extern "C" int zvideo_surface_accessors_smoke(void);
extern "C" int zvideo_fxpass3_local_queue_smoke(void);
extern "C" int zvideo_primary_surface_rect_scratch_smoke(void);
extern "C" int zvideo_run_postprocess_on_sw_buffer_smoke(void);
extern "C" int zvideo_run_postprocess_on_primary_buffer_smoke(void);
extern "C" int zvideo_frame_scratch_buffers_smoke(void);
extern "C" int zvideo_clear_dispatch_and_exchange_smoke(void);
extern "C" int zvideo_mode_geometry_and_set_video_mode_smoke(void);
extern "C" int zvideo_init_video_system_reentry_guard_smoke(void);
extern "C" int zvideo_dispatch_wrappers_smoke(void);
extern "C" int zvideo_bind_renderer_dispatch_smoke(void);
extern "C" int zvideo_adjust_surfaces_if_enabled_smoke(void);
extern "C" int zvideo_set_half_res_adjust_mode_smoke(void);
extern "C" int zvideo_handle_software_mode_hotkey_smoke(void);
extern "C" int zvideo_dd_report_error_smoke(void);
extern "C" int zvideo_dd_run_device_enumeration_smoke(void);
extern "C" int zvideo_dd_startup_enumerate_default_select_smoke(void);
extern "C" int zvideo_module_init_smoke(void);
extern "C" int zvid_hw_api_accessors_smoke(void);
extern "C" int zvid_cached_renderer_and_texture_counts_smoke(void);
extern "C" int zvid_texture_pack_load_state_getter_smoke(void);
extern "C" int zvid_texture_pack_load_state_setter_smoke(void);
extern "C" int zvid_option_accessors_smoke(void);
extern "C" int zvid_set_video_mode_index_smoke(void);
extern "C" int zvideo_buff_clip_coord_to_range_smoke(void);
extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void);
extern "C" int zvideo_buff_blt_source_to_primary_clipped_smoke(void);
extern "C" int zvideo_surface_state_lock_skip_smoke(void);
extern "C" int zvideo_dd_unlock_directdraw_surface_smoke(void);
extern "C" int zvideo_dd_lock_directdraw_surface_smoke(void);
extern "C" int zvideo_dd_lock_surface_state_smoke(void);
extern "C" int zvideo_dd_unlock_surface_state_smoke(void);
extern "C" int zvideo_capture_surface_to_image_smoke(void);
extern "C" int zvideo_image_lazy_create_backing_surface_guards_smoke(void);
extern "C" int zvideo_blt_sw_to_primary_rect_lazy_failure_smoke(void);
extern "C" int zvideo_flip_to_gdi_if_attached_null_smoke(void);
extern "C" int zvideo_clear_rect_skip_paths_smoke(void);
extern "C" int zvideo_palette_set_entries_non8bpp_smoke(void);
extern "C" int zvideo_apply_brightness_to_palette_entries_smoke(void);
extern "C" int zvideo_load_palette_file_and_apply_brightness_smoke(void);
extern "C" int zvideo_quad_batch_depth_and_rhw_smoke(void);
extern "C" int zvideo_set_active_view_context_smoke(void);
extern "C" int zvideo_frustum_test_sphere_clip_mask_smoke(void);
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
extern "C" int zvid_image_create_format_size_pixels_smoke(void);
extern "C" int zvideo_image_file_read_helpers_smoke(void);
extern "C" int zvideo_palette_remap_no_recipes_smoke(void);
extern "C" int zvideo_palette_remap_recipe_variants_smoke(void);
extern "C" int zvideo_texture_pack_load_image_smoke(void);
extern "C" int zimage_font_glyph_scan_smoke(void);
extern "C" int zimage_font_measure_string_smoke(void);
extern "C" int zimage_fonts_load_missing_smoke(void);
extern "C" int zimage_texdir_find_or_create_missing_smoke(void);
extern "C" int zimage_texdir_build_mip_chain_smoke(void);
extern "C" int zvid_texture_pack_ensure_builtin_smoke(void);
extern "C" int zimage_texdir_load_pending_entries_smoke(void);
extern "C" int zimage_texdir_load_pending_entries_renderer_smoke(void);
extern "C" int zclass_node_load_flag_bit8_material_images_and_texture_pack_smoke(void);
extern "C" int zimage_texdir_base_name_path_smoke(void);
extern "C" int zimage_texdir_variant_image_smoke(void);
extern "C" int zimage_texdir_find_by_name_smoke(void);
extern "C" int zimage_texdir_write_smoke(void);
extern "C" int zimage_init_option_fallback_smoke(void);
extern "C" int zimage_init_texture_directory_smoke(void);
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
extern "C" int zweapon_optcatalog_load_kill_verb_string_smoke(void);
extern "C" int zweapon_load_opt_catalog_from_path_smoke(void);
extern "C" int zweapon_zar_weapons_section_callbacks_smoke(void);
extern "C" int zweapon_optcatalog_shutdown_smoke(void);
extern "C" int zweapon_set_max_tether_altitude_smoke(void);
extern "C" int zweapon_optcatalog_blend_direction_toward_target_smoke(void);
extern "C" int zweapon_optcatalog_find_entry_by_name_smoke(void);
extern "C" int zweapon_optcatalog_find_entry_by_id_smoke(void);
extern "C" int zweapon_optcatalog_create_trail_segment_node_smoke(void);
extern "C" int zweapon_optcatalog_create_trail_runtime_state_smoke(void);
extern "C" int zweapon_optcatalog_mine_iterator_smoke(void);
extern "C" int zweapon_optcatalog_pending_spawn_override_smoke(void);
extern "C" int zweapon_optcatalog_load_fx_spec_from_reader_node_smoke(void);
extern "C" int zweapon_optcatalog_invoke_damage_feedback_and_hit_callback_smoke(void);
extern "C" int hitcontext_get_current_owner_smoke(void);
extern "C" int optcatalog_set_damage_context_smoke(void);
extern "C" int optcatalog_damage_feedback_leaf_helpers_smoke(void);
extern "C" int optcatalog_capture_hit_snapshot_and_invoke_damage_timer_callback_smoke(void);
extern "C" int zweapon_optcatalog_runtime_free_list_helpers_smoke(void);
extern "C" int zweapon_optcatalog_build_impact_hit_list_smoke(void);
extern "C" int zweapon_optcatalog_handle_impact_from_runtime_probe_smoke(void);
extern "C" int zweapon_optcatalog_process_runtime_instance_smoke(void);
extern "C" int zweapon_optcatalog_process_runtime_instances_smoke(void);
extern "C" int zweapon_optcatalog_remove_runtime_instance_smoke(void);
extern "C" int zweapon_optcatalog_can_spawn_through_ray_smoke(void);
extern "C" int zweapon_optcatalog_compute_aim_pitch_for_target_smoke(void);
extern "C" int zweapon_optcatalog_reflect_and_sort_impact_trace_list_smoke(void);
extern "C" int zweapon_optcatalog_compute_trail_impact_response_smoke(void);
extern "C" int zweapon_optcatalog_update_trail_segment_visual_smoke(void);
extern "C" int zweapon_optcatalog_emit_crater_impact_event_smoke(void);
extern "C" int zweapon_optcatalog_emit_qsand_impact_event_smoke(void);
extern "C" int zweapon_optcatalog_play_impact_sound_smoke(void);
extern "C" int zweapon_optcatalog_play_bounce_sound_smoke(void);
extern "C" int zweapon_optcatalog_handle_impact_event_smoke(void);
extern "C" int zweapon_optcatalog_handle_impact_event_from_runtime_state_smoke(void);
extern "C" int light_alloc_from_free_list_and_attach_smoke(void);
extern "C" int light_init_thermal_glow_pool_smoke(void);
extern "C" int player_timed_hit_status_smoke(void);
extern "C" int zweapon_optcatalog_warning_samples_smoke(void);
extern "C" int zweapon_optcatalog_activate_trail_runtime_state_smoke(void);
extern "C" int zweapon_optcatalog_deactivate_trail_runtime_state_smoke(void);
extern "C" int zvideo_restore_iconic_fullscreen_window_smoke(void);
extern "C" int zvideo_shutdown_video_system_smoke(void);
extern "C" int zsys_find_file_on_drive_type_negative_smoke(void);
extern "C" int zsys_runtime_probe_leaves_smoke(void);
extern "C" int zsys_cpuid_mmx_smoke(void);
extern "C" int zsys_cpu_leaf_helpers_smoke(void);
extern "C" int zsys_exit_process_with_cleanup_child_smoke(void);
extern "C" int zsys_exit_process_with_cleanup_smoke(void);
extern "C" int zerror_init_output_context_smoke(void);
extern "C" int zerror_emit_debug_buffer_smoke(void);
extern "C" int zcom_query_interface_from_interface_map_smoke(void);
extern "C" int zcom_connection_point_container_advise_smoke(void);
extern "C" int zcom_connection_point_container_unadvise_smoke(void);
extern "C" int pickup_type_table_free_opt_meta_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_shutdown_global_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_init_nodes_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_init_global_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_get_world_pos_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_can_spawn_with_clearance_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_spawn_pickup_type_and_relay_gates_smoke(void);
extern "C" int pickup_airdrop_spawn_ref_try_spawn_random_pickup_from_global_blocked_smoke(void);
extern "C" int pickup_spawn_list_has_entry_near_xz_smoke(void);
extern "C" int pickup_map_vtol_drop_group_variant_to_type_index_smoke(void);
extern "C" int pickup_select_next_vtol_spawn_type_index_smoke(void);
extern "C" int pickup_select_puppies_zrd_by_difficulty_smoke(void);
extern "C" int net_is_opt_entry_active_in_any_slot_smoke(void);
extern "C" int pickup_init_and_load_puppy_spawns_smoke(void);
extern "C" int pickup_register_existing_object_smoke(void);
extern "C" int pickup_spawn_with_airdrop_chute_smoke(void);
extern "C" int pickup_spawn_list_clear_smoke(void);
extern "C" int pickup_respawn_queue_clear_smoke(void);
extern "C" int pickup_archive_write_all_smoke(void);
extern "C" int pickup_archive_read_record_smoke(void);
extern "C" int pickup_init_smoke(void);
extern "C" int pickup_shutdown_smoke(void);
extern "C" int pickup_remove_object_smoke(void);
extern "C" int pickup_on_collected_no_anim_smoke(void);
extern "C" int pickup_leaf_helpers_smoke(void);
extern "C" int pickup_respawn_spawn_def_smoke(void);
extern "C" int pickup_respawn_queue_update_smoke(void);
extern "C" int pickup_find_droppable_type_for_current_weapon_smoke(void);
extern "C" int pickup_type_key_table_find_index_smoke(void);
extern "C" int pickup_type_meta_find_by_name_smoke(void);
extern "C" int pickup_spawn_from_parsed_zrd_entry_smoke(void);
extern "C" int pickup_handle_pkt11_spawn_delta_smoke(void);
extern "C" int pickup_handle_pkt12_airdrop_spawn_chute_relay_smoke(void);
extern "C" int pickup_remove_other_spawns_same_opt_entry_smoke(void);
extern "C" int pickup_resolve_owner_from_bvol_hit_smoke(void);
extern "C" int pickup_grant_ammo_or_weapon_smoke(void);
extern "C" int pickup_apply_effect_smoke(void);
extern "C" int time_reset_smoke(void);
extern "C" int time_tick_smoke(void);

#include <cstdio>
#include <cstring>

namespace {
struct SmokeTest {
    const char *name;
    int (*run)();
};

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

int main(int argc, char **argv) {
    const SmokeTest tests[] = {
        {"recoil_native_build_anchor", recoil_native_build_anchor},
        {"recoil_mfc42_provider_smoke", recoil_mfc42_provider_smoke},
        {"mfc_cstring_default_ctor_provider_smoke",
         mfc_cstring_default_ctor_provider_smoke},
        {"czrecoil_frame_build_window_title_smoke", czrecoil_frame_build_window_title_smoke},
        {"czframe_metadata_accessors_smoke", czframe_metadata_accessors_smoke},
        {"get_open_file_name_import_provider_smoke", get_open_file_name_import_provider_smoke},
        {"czrecoil_frame_set_menu_bar_visibility_smoke",
         czrecoil_frame_set_menu_bar_visibility_smoke},
        {"czrecoil_frame_configure_mode_feature_flags_smoke",
         czrecoil_frame_configure_mode_feature_flags_smoke},
        {"czrecoil_frame_video_mode_menu_handlers_smoke",
         czrecoil_frame_video_mode_menu_handlers_smoke},
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
        {"czrecoil_frame_on_size_smoke", czrecoil_frame_on_size_smoke},
        {"czgame_frame_is_window_valid_smoke", czgame_frame_is_window_valid_smoke},
        {"czgame_frame_build_window_title_smoke", czgame_frame_build_window_title_smoke},
        {"czgame_frame_constructor_smoke", czgame_frame_constructor_smoke},
        {"czgame_frame_create_object_smoke", czgame_frame_create_object_smoke},
        {"czgame_frame_destructor_smoke", czgame_frame_destructor_smoke},
        {"czgame_frame_on_close_smoke", czgame_frame_on_close_smoke},
        {"czgame_frame_on_create_smoke", czgame_frame_on_create_smoke},
        {"czgame_frame_on_destroy_smoke", czgame_frame_on_destroy_smoke},
        {"czgame_frame_on_activate_smoke", czgame_frame_on_activate_smoke},
        {"czgame_frame_on_paint_smoke", czgame_frame_on_paint_smoke},
        {"czrecoil_frame_constructor_smoke", czrecoil_frame_constructor_smoke},
        {"czrecoil_frame_create_object_smoke", czrecoil_frame_create_object_smoke},
        {"recoil_app_create_main_wnd_smoke", recoil_app_create_main_wnd_smoke},
        {"recoil_app_init_main_window_smoke", recoil_app_init_main_window_smoke},
        {"czrecoil_frame_destructor_smoke", czrecoil_frame_destructor_smoke},
        {"czrecoil_frame_on_menu_start_single_player_smoke",
         czrecoil_frame_on_menu_start_single_player_smoke},
        {"czrecoil_frame_on_open_file_dialog_smoke",
         czrecoil_frame_on_open_file_dialog_smoke},
        {"czrecoil_frame_on_menu_open_campaign_smoke",
         czrecoil_frame_on_menu_open_campaign_smoke},
        {"czrecoil_frame_on_menu_open_help_docs_smoke",
         czrecoil_frame_on_menu_open_help_docs_smoke},
        {"cabout_dlg_constructor_smoke", cabout_dlg_constructor_smoke},
        {"czrecoil_frame_on_menu_about_smoke", czrecoil_frame_on_menu_about_smoke},
        {"mfc_three_float_dialog_handlers_smoke", mfc_three_float_dialog_handlers_smoke},
        {"zstr_contains_case_insensitive_smoke", zstr_contains_case_insensitive_smoke},
        {"czrecoil_frame_start_mode_menu_handlers_smoke",
         czrecoil_frame_start_mode_menu_handlers_smoke},
        {"czrecoil_frame_select_hw_api_menu_handlers_smoke",
         czrecoil_frame_select_hw_api_menu_handlers_smoke},
        {"czrecoil_frame_toggle_archive_banks_smoke",
         czrecoil_frame_toggle_archive_banks_smoke},
        {"czrecoil_frame_toggle_texture_packs_smoke",
         czrecoil_frame_toggle_texture_packs_smoke},
        {"hud_ui_save_load_entry_is_newer_than_smoke",
         hud_ui_save_load_entry_is_newer_than_smoke},
        {"hud_ui_save_load_list_item_constructor_smoke",
         hud_ui_save_load_list_item_constructor_smoke},
        {"hud_ui_save_load_list_item_on_activate_smoke",
         hud_ui_save_load_list_item_on_activate_smoke},
        {"hud_ui_save_load_insert_entry_sorted_prefix_smoke",
         hud_ui_save_load_insert_entry_sorted_prefix_smoke},
        {"hud_ui_save_load_partition_entries_by_pivot_smoke",
         hud_ui_save_load_partition_entries_by_pivot_smoke},
        {"hud_ui_save_load_sort_entry_range_smoke", hud_ui_save_load_sort_entry_range_smoke},
        {"hud_ui_save_load_refresh_file_list_smoke", hud_ui_save_load_refresh_file_list_smoke},
        {"hud_ui_save_load_initialize_file_entries_smoke",
         hud_ui_save_load_initialize_file_entries_smoke},
        {"hud_ui_save_load_delete_save_file_smoke", hud_ui_save_load_delete_save_file_smoke},
        {"hud_ui_save_load_set_selected_entry_index_smoke",
         hud_ui_save_load_set_selected_entry_index_smoke},
        {"hud_ui_save_load_game_name_input_smoke",
         hud_ui_save_load_game_name_input_smoke},
        {"hud_ui_save_load_next_prev_buttons_smoke",
         hud_ui_save_load_next_prev_buttons_smoke},
        {"hud_ui_save_load_delete_button_on_activate_smoke",
         hud_ui_save_load_delete_button_on_activate_smoke},
        {"czgame_frame_on_app_idle_dispatch_message_smoke",
         czgame_frame_on_app_idle_dispatch_message_smoke},
        {"recoil_state_controls_queue_enter_smoke",
         recoil_state_controls_queue_enter_smoke},
        {"hud_ui_options_panel_overlay_owner_queue_enter_smoke",
         hud_ui_options_panel_overlay_owner_queue_enter_smoke},
        {"hud_ui_options_panel_overlay_owner_constructor_smoke",
         hud_ui_options_panel_overlay_owner_constructor_smoke},
        {"hud_ui_options_panel_overlay_owner_destructor_core_smoke",
         hud_ui_options_panel_overlay_owner_destructor_core_smoke},
        {"hud_ui_options_panel_overlay_owner_scalar_deleting_destructor_smoke",
         hud_ui_options_panel_overlay_owner_scalar_deleting_destructor_smoke},
        {"hud_ui_options_panel_overlay_owner_static_init_thunks_smoke",
         hud_ui_options_panel_overlay_owner_static_init_thunks_smoke},
        {"hud_ui_options_panel_overlay_owner_on_try_become_current_smoke",
         hud_ui_options_panel_overlay_owner_on_try_become_current_smoke},
        {"briefing_stop_and_shutdown_thread_smoke", briefing_stop_and_shutdown_thread_smoke},
        {"briefing_thread_main_one_iteration_smoke", briefing_thread_main_one_iteration_smoke},
        {"briefing_start_for_mission_smoke", briefing_start_for_mission_smoke},
        {"briefing_set_progress_and_sleep_smoke", briefing_set_progress_and_sleep_smoke},
        {"briefing_runtime_constructor_smoke", briefing_runtime_constructor_smoke},
        {"briefing_locator_panel_constructor_smoke", briefing_locator_panel_constructor_smoke},
        {"briefing_locator_panel_blit_dirty_rect_smoke",
         briefing_locator_panel_blit_dirty_rect_smoke},
        {"briefing_locator_panel_update_smoke", briefing_locator_panel_update_smoke},
        {"briefing_runtime_destructor_smoke", briefing_runtime_destructor_smoke},
        {"briefing_runtime_update_smoke", briefing_runtime_update_smoke},
        {"briefing_objective_picture_draw_noise_overlay_smoke",
         briefing_objective_picture_draw_noise_overlay_smoke},
        {"briefing_build_objective_actions_smoke", briefing_build_objective_actions_smoke},
        {"gamenet_list_reset_smoke", gamenet_list_reset_smoke},
        {"gamenet_player_row_append_smoke", gamenet_player_row_append_smoke},
        {"gamenet_unregister_gameplay_packet_handlers_smoke",
         gamenet_unregister_gameplay_packet_handlers_smoke},
        {"gamenet_register_gameplay_handlers_and_callbacks_smoke",
         gamenet_register_gameplay_handlers_and_callbacks_smoke},
        {"gamenet_chat_compose_key_callback_smoke", gamenet_chat_compose_key_callback_smoke},
        {"gamenet_begin_chat_compose_smoke", gamenet_begin_chat_compose_smoke},
        {"hud_ui_handle_hotkey_command_begin_chat_smoke",
         hud_ui_handle_hotkey_command_begin_chat_smoke},
        {"hud_timer_panel_net_state_clear_tail_flags_smoke",
         hud_timer_panel_net_state_clear_tail_flags_smoke},
        {"gamenet_find_player_row_and_status_bits_smoke",
         gamenet_find_player_row_and_status_bits_smoke},
        {"gamenet_update_remote_player_hud_widget_screen_pos_smoke",
         gamenet_update_remote_player_hud_widget_screen_pos_smoke},
        {"gamenet_get_local_player_color_index_smoke",
         gamenet_get_local_player_color_index_smoke},
        {"gamenet_get_nearest_other_player_distance_to_spawn_point_smoke",
         gamenet_get_nearest_other_player_distance_to_spawn_point_smoke},
        {"gamenet_respawn_player_color_indexed_spawn_smoke",
         gamenet_respawn_player_color_indexed_spawn_smoke},
        {"gamenet_reset_hud_timer_panel_net_state_smoke",
         gamenet_reset_hud_timer_panel_net_state_smoke},
        {"gamenet_wait_for_local_player_color_index_smoke",
         gamenet_wait_for_local_player_color_index_smoke},
        {"net_init_from_zrd_smoke", net_init_from_zrd_smoke},
        {"gamenet_host_update_session_status_fields_smoke",
         gamenet_host_update_session_status_fields_smoke},
        {"gamenet_timer_status_packet_smoke", gamenet_timer_status_packet_smoke},
        {"gamenet_timer_panel_state_packet_smoke",
         gamenet_timer_panel_state_packet_smoke},
        {"gamenet_tick_local_player_pkt06_and_timer_smoke",
         gamenet_tick_local_player_pkt06_and_timer_smoke},
        {"gamenet_scoreboard_snapshot_packet_smoke", gamenet_scoreboard_snapshot_packet_smoke},
        {"gamenet_lap_progress_packet_smoke", gamenet_lap_progress_packet_smoke},
        {"gamenet_chat_message_packet_smoke", gamenet_chat_message_packet_smoke},
        {"gamenet_show_player_kill_message_smoke", gamenet_show_player_kill_message_smoke},
        {"gamenet_player_kill_event_packet_smoke", gamenet_player_kill_event_packet_smoke},
        {"gamenet_reassign_player_colors_smoke", gamenet_reassign_player_colors_smoke},
        {"gamenet_player_row_apply_color_tint_smoke", gamenet_player_row_apply_color_tint_smoke},
        {"gamenet_apply_pkt06_player_state_snapshot_smoke",
         gamenet_apply_pkt06_player_state_snapshot_smoke},
        {"gamenet_handle_pkt06_player_state_snapshot_smoke",
         gamenet_handle_pkt06_player_state_snapshot_smoke},
        {"gamenet_spawn_remote_player_missing_template_smoke",
         gamenet_spawn_remote_player_missing_template_smoke},
        {"gamenet_handle_pkt07_alt_gun_dispatch_smoke",
         gamenet_handle_pkt07_alt_gun_dispatch_smoke},
        {"gamenet_send_pkt07_alt_gun_dispatch_smoke",
         gamenet_send_pkt07_alt_gun_dispatch_smoke},
        {"optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke",
         optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke},
        {"gamenet_alt_gun_dispatch_no_op_callback_smoke",
         gamenet_alt_gun_dispatch_no_op_callback_smoke},
        {"optcatalog_handle_pkt0a_remove_runtime_relay_smoke",
         optcatalog_handle_pkt0a_remove_runtime_relay_smoke},
        {"optcatalog_send_pkt0a_remove_runtime_relay_smoke",
         optcatalog_send_pkt0a_remove_runtime_relay_smoke},
        {"gamenet_host_send_pkt10_qsand_feature_smoke",
         gamenet_host_send_pkt10_qsand_feature_smoke},
        {"gamenet_send_pkt10_qsand_event_smoke", gamenet_send_pkt10_qsand_event_smoke},
        {"gamenet_host_send_pkt0f_crater_feature_smoke",
         gamenet_host_send_pkt0f_crater_feature_smoke},
        {"gamenet_send_pkt13_effect_anim_activation_record_smoke",
         gamenet_send_pkt13_effect_anim_activation_record_smoke},
        {"gamenet_handle_pkt13_effect_anim_activation_record_smoke",
         gamenet_handle_pkt13_effect_anim_activation_record_smoke},
        {"gamenet_send_all_pkt13_effect_anim_activation_records_smoke",
         gamenet_send_all_pkt13_effect_anim_activation_records_smoke},
        {"gamenet_player_row_destroy_embedded_panel_smoke",
         gamenet_player_row_destroy_embedded_panel_smoke},
        {"gamenet_reset_remote_players_and_spawn_lists_smoke",
         gamenet_reset_remote_players_and_spawn_lists_smoke},
        {"gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke",
         gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke},
        {"gamenet_handle_pkt03_remove_remote_player_smoke",
         gamenet_handle_pkt03_remove_remote_player_smoke},
        {"hud_low_meter_loop_sound_set_loop_active_smoke",
         hud_low_meter_loop_sound_set_loop_active_smoke},
        {"hud_cheat_clear_nanite_panel_cheat_sentinel_smoke",
         hud_cheat_clear_nanite_panel_cheat_sentinel_smoke},
        {"hud_cheat_execute_command_string_smoke", hud_cheat_execute_command_string_smoke},
        {"ainet_free_all_smoke", ainet_free_all_smoke},
        {"ainet_alloc_smoke", ainet_alloc_smoke},
        {"ainet_find_node_by_index_smoke", ainet_find_node_by_index_smoke},
        {"ainet_path_probe_fan_init_from_segment_smoke",
         ainet_path_probe_fan_init_from_segment_smoke},
        {"ainet_resolve_neighbor_links_and_probe_fans_smoke",
         ainet_resolve_neighbor_links_and_probe_fans_smoke},
        {"ainet_load_from_zrd_smoke", ainet_load_from_zrd_smoke},
        {"ainet_load_all_from_zrd_smoke", ainet_load_all_from_zrd_smoke},
        {"ainet_find_by_net_id_smoke", ainet_find_by_net_id_smoke},
        {"ainet_find_nearest_node_smoke", ainet_find_nearest_node_smoke},
        {"zvehicle_select_zrd_by_difficulty_smoke", zvehicle_select_zrd_by_difficulty_smoke},
        {"player_load_master_common_data_from_node_smoke",
         player_load_master_common_data_from_node_smoke},
        {"player_get_save_state_list_head_smoke", player_get_save_state_list_head_smoke},
        {"player_unbind_current_save_state_if_single_player_smoke",
         player_unbind_current_save_state_if_single_player_smoke},
        {"player_bind_active_game_state_as_current_save_state_smoke",
         player_bind_active_game_state_as_current_save_state_smoke},
        {"player_async_command_callback_basic_smoke", player_async_command_callback_basic_smoke},
        {"player_sync_local_pose_from_root_node_smoke",
         player_sync_local_pose_from_root_node_smoke},
        {"player_get_aiv_zrd_path_smoke", player_get_aiv_zrd_path_smoke},
        {"player_extract_vehicle_name_from_aiv_name_smoke",
         player_extract_vehicle_name_from_aiv_name_smoke},
        {"player_clone_type6_node_from_template_and_rename_smoke",
         player_clone_type6_node_from_template_and_rename_smoke},
        {"player_create_from_names_at_pose_smoke", player_create_from_names_at_pose_smoke},
        {"player_init_mission_runtime_missing_aiv_smoke",
         player_init_mission_runtime_missing_aiv_smoke},
        {"hud_sensor_tracker_init_mission_gameplay_systems_smoke",
         hud_sensor_tracker_init_mission_gameplay_systems_smoke},
        {"player_cache_gun_hardpoints_and_detach_displays_smoke",
         player_cache_gun_hardpoints_and_detach_displays_smoke},
        {"player_init_state_from_name_and_master_common_data_smoke",
         player_init_state_from_name_and_master_common_data_smoke},
        {"player_build_ai_peer_rings_by_ai_net_id_smoke",
         player_build_ai_peer_rings_by_ai_net_id_smoke},
        {"player_build_support_points_from_model_smoke",
         player_build_support_points_from_model_smoke},
        {"player_build_collision_points_from_model_smoke",
         player_build_collision_points_from_model_smoke},
        {"player_bind_modal_state_from_master_modal_data_smoke",
         player_bind_modal_state_from_master_modal_data_smoke},
        {"player_sample_ground_and_align_root_to_surface_smoke",
         player_sample_ground_and_align_root_to_surface_smoke},
        {"player_init_spawn_state_from_primary_modal_data_smoke",
         player_init_spawn_state_from_primary_modal_data_smoke},
        {"player_load_master_modal_data_from_node_smoke",
         player_load_master_modal_data_from_node_smoke},
        {"player_ai_discard_negative_branch_nodes_smoke",
         player_ai_discard_negative_branch_nodes_smoke},
        {"player_tick_ai_mode2_top_level_smoke", player_tick_ai_mode2_top_level_smoke},
        {"player_mgr_tick_all_players_smoke", player_mgr_tick_all_players_smoke},
        {"player_ai_enter_mode2_steering_pursuit_smoke",
         player_ai_enter_mode2_steering_pursuit_smoke},
        {"player_ai_alert_attack_buddies_smoke", player_ai_alert_attack_buddies_smoke},
        {"player_ai_try_enter_mode2_attack_pursuit_los_smoke",
         player_ai_try_enter_mode2_attack_pursuit_los_smoke},
        {"player_ai_rebuild_synthetic_path_to_node_if_far_smoke",
         player_ai_rebuild_synthetic_path_to_node_if_far_smoke},
        {"player_tick_ai_mode2_steering_substate_smoke",
         player_tick_ai_mode2_steering_substate_smoke},
        {"player_update_ai_mode2_move_and_turn_toward_target_smoke",
         player_update_ai_mode2_move_and_turn_toward_target_smoke},
        {"player_update_ai_mode2_turn_toward_player_no_throttle_smoke",
         player_update_ai_mode2_turn_toward_player_no_throttle_smoke},
        {"player_update_ai_mode2_turn_in_place_toward_player_smoke",
         player_update_ai_mode2_turn_in_place_toward_player_smoke},
        {"player_solve_alt_gun_lead_target_point_smoke",
         player_solve_alt_gun_lead_target_point_smoke},
        {"player_tick_ai_mode2_alt_gun_attack_window_smoke",
         player_tick_ai_mode2_alt_gun_attack_window_smoke},
        {"player_update_ai_mode2_move_and_turn_toward_offset_target_smoke",
         player_update_ai_mode2_move_and_turn_toward_offset_target_smoke},
        {"player_update_ai_mode2_move_and_turn_toward_dynamic_offset_target_smoke",
         player_update_ai_mode2_move_and_turn_toward_dynamic_offset_target_smoke},
        {"player_tick_ai_mode2_offset_target_steering_smoke",
         player_tick_ai_mode2_offset_target_steering_smoke},
        {"player_tick_ai_mode2_dynamic_offset_target_steering_smoke",
         player_tick_ai_mode2_dynamic_offset_target_steering_smoke},
        {"player_ai_restore_saved_top_level_state_smoke",
         player_ai_restore_saved_top_level_state_smoke},
        {"player_ai_finalize_mode2_state1_for_all_players_smoke",
         player_ai_finalize_mode2_state1_for_all_players_smoke},
        {"player_ai_steer_toward_path_node_forward_smoke",
         player_ai_steer_toward_path_node_forward_smoke},
        {"player_ai_steer_toward_path_node_reverse_smoke",
         player_ai_steer_toward_path_node_reverse_smoke},
        {"player_tick_ai_mode2_timed_path_steering_smoke",
         player_tick_ai_mode2_timed_path_steering_smoke},
        {"player_find_alt_gun_controller_smoke", player_find_alt_gun_controller_smoke},
        {"player_is_alt_weapon_allowed_in_current_master_mode_smoke",
         player_is_alt_weapon_allowed_in_current_master_mode_smoke},
        {"player_auto_switch_to_next_usable_alt_weapon_smoke",
         player_auto_switch_to_next_usable_alt_weapon_smoke},
        {"player_update_gun_dispatch_requests_from_trigger_latches_smoke",
         player_update_gun_dispatch_requests_from_trigger_latches_smoke},
        {"player_update_debug_overlay_hud_smoke", player_update_debug_overlay_hud_smoke},
        {"player_alt_gun_fire_point_selection_smoke", player_alt_gun_fire_point_selection_smoke},
        {"player_primary_gun_fire_point_selection_smoke",
         player_primary_gun_fire_point_selection_smoke},
        {"player_alt_gun_ensure_aux_effect_active_smoke",
         player_alt_gun_ensure_aux_effect_active_smoke},
        {"player_update_continuous_alt_gun_fire_controller_smoke",
         player_update_continuous_alt_gun_fire_controller_smoke},
        {"player_alt_gun_projectile_dispatch_helpers_smoke",
         player_alt_gun_projectile_dispatch_helpers_smoke},
        {"player_process_alt_gun_fire_dispatch_request_smoke",
         player_process_alt_gun_fire_dispatch_request_smoke},
        {"player_process_primary_gun_dispatch_request_smoke",
         player_process_primary_gun_dispatch_request_smoke},
        {"player_tick_alt_gun_runtime_state_smoke", player_tick_alt_gun_runtime_state_smoke},
        {"player_alt_gun_fire_slot_offset_smoke", player_alt_gun_fire_slot_offset_smoke},
        {"player_build_mission_save_data_smoke", player_build_mission_save_data_smoke},
        {"player_apply_mission_save_data_smoke", player_apply_mission_save_data_smoke},
        {"player_zar_read_mission_save_data_section_smoke",
         player_zar_read_mission_save_data_section_smoke},
        {"player_refresh_hud_from_state_smoke", player_refresh_hud_from_state_smoke},
        {"player_apply_status_meter_change_smoke", player_apply_status_meter_change_smoke},
        {"player_apply_primary_weapon_switch_smoke", player_apply_primary_weapon_switch_smoke},
        {"player_apply_alt_weapon_switch_smoke", player_apply_alt_weapon_switch_smoke},
        {"player_load_weapon_banks_and_select_defaults_smoke",
         player_load_weapon_banks_and_select_defaults_smoke},
        {"player_free_alt_weapon_trail_runtime_states_smoke",
         player_free_alt_weapon_trail_runtime_states_smoke},
        {"player_check_mission_weapon_availability_smoke",
         player_check_mission_weapon_availability_smoke},
        {"player_reset_alt_gun_door_animation_state_smoke",
         player_reset_alt_gun_door_animation_state_smoke},
        {"player_reset_alt_gun_runtime_state_smoke",
         player_reset_alt_gun_runtime_state_smoke},
        {"player_remove_all_deployed_mines_smoke", player_remove_all_deployed_mines_smoke},
        {"player_handle_alt_weapon_bank_select_smoke",
         player_handle_alt_weapon_bank_select_smoke},
        {"player_handle_primary_weapon_variant_toggle_smoke",
         player_handle_primary_weapon_variant_toggle_smoke},
        {"player_reset_damage_state_and_timed_hit_status_smoke",
         player_reset_damage_state_and_timed_hit_status_smoke},
        {"player_reset_damage_visuals_and_timed_status_smoke",
         player_reset_damage_visuals_and_timed_status_smoke},
        {"player_destroyed_state_reset_local_finalize_smoke",
         player_destroyed_state_reset_local_finalize_smoke},
        {"player_destroyed_state_reset_finalize_callback_smoke",
         player_destroyed_state_reset_finalize_callback_smoke},
        {"player_destroyed_state_reset_callback_smoke",
         player_destroyed_state_reset_callback_smoke},
        {"player_clear_respawn_transition_flag_callback_smoke",
         player_clear_respawn_transition_flag_callback_smoke},
        {"player_destroyed_state_respawn_callback_smoke",
         player_destroyed_state_respawn_callback_smoke},
        {"player_enter_local_inactive_destroyed_lifecycle_smoke",
         player_enter_local_inactive_destroyed_lifecycle_smoke},
        {"player_update_status_meter_smoke", player_update_status_meter_smoke},
        {"player_set_world_pose_and_restart_anchor_smoke",
         player_set_world_pose_and_restart_anchor_smoke},
        {"player_capture_current_object_pose_as_restart_anchor_smoke",
         player_capture_current_object_pose_as_restart_anchor_smoke},
        {"player_reset_mouse_control_state_and_recenter_cursor_smoke",
         player_reset_mouse_control_state_and_recenter_cursor_smoke},
        {"player_register_gameplay_callbacks_and_ff_smoke",
         player_register_gameplay_callbacks_and_ff_smoke},
        {"player_toggle_steering_mode_and_reset_mouse_look_smoke",
         player_toggle_steering_mode_and_reset_mouse_look_smoke},
        {"player_reset_motion_transient_state_smoke", player_reset_motion_transient_state_smoke},
        {"player_is_mission_probe_type1_enabled_by_id_smoke",
         player_is_mission_probe_type1_enabled_by_id_smoke},
        {"player_update_bank_velocity_from_steer_input_smoke",
         player_update_bank_velocity_from_steer_input_smoke},
        {"player_update_auto_turn_and_steer_from_target_smoke",
         player_update_auto_turn_and_steer_from_target_smoke},
        {"player_integrate_yaw_and_wrap_from_yaw_velocity_smoke",
         player_integrate_yaw_and_wrap_from_yaw_velocity_smoke},
        {"player_rebuild_steer_basis_from_motion_basis_smoke",
         player_rebuild_steer_basis_from_motion_basis_smoke},
        {"player_rebuild_steer_basis_from_motion_axes_smoke",
         player_rebuild_steer_basis_from_motion_axes_smoke},
        {"player_set_auto_turn_target_dir_from_world_point_smoke",
         player_set_auto_turn_target_dir_from_world_point_smoke},
        {"player_tick_local_player_controls_smoke", player_tick_local_player_controls_smoke},
        {"player_filter_camera_probe_blocking_hits_smoke",
         player_filter_camera_probe_blocking_hits_smoke},
        {"player_find_nearest_third_person_camera_probe_point_smoke",
         player_find_nearest_third_person_camera_probe_point_smoke},
        {"player_adjust_sub_camera_focus_for_obstruction_smoke",
         player_adjust_sub_camera_focus_for_obstruction_smoke},
        {"player_adjust_third_person_camera_by_offset_probes_smoke",
         player_adjust_third_person_camera_by_offset_probes_smoke},
        {"player_adjust_third_person_camera_by_side_probes_smoke",
         player_adjust_third_person_camera_by_side_probes_smoke},
        {"player_update_chase_camera_from_input_smoke",
         player_update_chase_camera_from_input_smoke},
        {"player_update_top_down_camera_state_smoke",
         player_update_top_down_camera_state_smoke},
        {"player_update_first_person_camera_from_input_smoke",
         player_update_first_person_camera_from_input_smoke},
        {"player_update_camera_from_stored_target_toward_player_smoke",
         player_update_camera_from_stored_target_toward_player_smoke},
        {"player_restore_third_person_camera_from_obstruction_state_smoke",
         player_restore_third_person_camera_from_obstruction_state_smoke},
        {"player_update_camera_variant_from_anchor_smoke",
         player_update_camera_variant_from_anchor_smoke},
        {"player_update_camera_variant_from_camera_pos_smoke",
         player_update_camera_variant_from_camera_pos_smoke},
        {"player_update_camera_weather_fx_emitter_visibility_smoke",
         player_update_camera_weather_fx_emitter_visibility_smoke},
        {"player_tick_active_camera_state_smoke", player_tick_active_camera_state_smoke},
        {"player_clear_pending_contact_queues_smoke", player_clear_pending_contact_queues_smoke},
        {"player_pending_contact_select_preferred_smoke",
         player_pending_contact_select_preferred_smoke},
        {"player_select_and_resolve_preferred_pending_collision_contact_smoke",
         player_select_and_resolve_preferred_pending_collision_contact_smoke},
        {"checkpoint_update_player_lap_progress_and_notify_net_smoke",
         checkpoint_update_player_lap_progress_and_notify_net_smoke},
        {"checkpoint_instantiate_named_objects_smoke", checkpoint_instantiate_named_objects_smoke},
        {"player_classify_pending_contacts_for_segment_smoke",
         player_classify_pending_contacts_for_segment_smoke},
        {"player_collect_pending_contacts_for_segments_smoke",
         player_collect_pending_contacts_for_segments_smoke},
        {"player_ai_mode2_forward_probe_requires_auto_turn_smoke",
         player_ai_mode2_forward_probe_requires_auto_turn_smoke},
        {"player_ai_choose_next_path_branch_index_smoke",
         player_ai_choose_next_path_branch_index_smoke},
        {"player_ai_advance_path_cursor_and_compute_target_vec_smoke",
         player_ai_advance_path_cursor_and_compute_target_vec_smoke},
        {"player_tick_ai_mode2_path_follow_smoke",
         player_tick_ai_mode2_path_follow_smoke},
        {"player_pickup_contact_passes_collection_test_smoke",
         player_pickup_contact_passes_collection_test_smoke},
        {"player_process_pending_pickup_contacts_smoke",
         player_process_pending_pickup_contacts_smoke},
        {"player_build_pending_contact_queues_smoke", player_build_pending_contact_queues_smoke},
        {"player_process_pending_contact_queues_smoke",
         player_process_pending_contact_queues_smoke},
        {"player_collect_pending_collision_contacts_for_quad_probe_smoke",
         player_collect_pending_collision_contacts_for_quad_probe_smoke},
        {"player_try_resolve_pending_collision_probe_sweep_smoke",
         player_try_resolve_pending_collision_probe_sweep_smoke},
        {"player_resolve_pending_collision_contact_smoke",
         player_resolve_pending_collision_contact_smoke},
        {"player_prepare_pending_world_collision_response_smoke",
         player_prepare_pending_world_collision_response_smoke},
        {"player_resolve_pending_world_collision_contact_smoke",
         player_resolve_pending_world_collision_contact_smoke},
        {"player_resolve_pending_player_collision_contact_smoke",
         player_resolve_pending_player_collision_contact_smoke},
        {"player_process_transfer_contact_queue_smoke",
         player_process_transfer_contact_queue_smoke},
        {"player_apply_pitch_roll_velocity_impulse_from_direction_smoke",
         player_apply_pitch_roll_velocity_impulse_from_direction_smoke},
        {"player_record_recent_hit_feedback_smoke", player_record_recent_hit_feedback_smoke},
        {"player_update_timed_hit_status_from_source_smoke",
         player_update_timed_hit_status_from_source_smoke},
        {"player_hit_callback_record_context_and_timed_status_smoke",
         player_hit_callback_record_context_and_timed_status_smoke},
        {"player_hit_callback_record_net_context_and_timed_status_smoke",
         player_hit_callback_record_net_context_and_timed_status_smoke},
        {"player_enter_destroyed_state_smoke", player_enter_destroyed_state_smoke},
        {"player_clear_destroyed_respawn_effect_handle_callback_smoke",
         player_clear_destroyed_respawn_effect_handle_callback_smoke},
        {"player_start_destroyed_state_vehicle_effect_smoke",
         player_start_destroyed_state_vehicle_effect_smoke},
        {"player_apply_damage_local_smoke", player_apply_damage_local_smoke},
        {"player_tick_remote_network_player_smoke", player_tick_remote_network_player_smoke},
        {"player_apply_pending_collision_probe_velocity_smoke",
         player_apply_pending_collision_probe_velocity_smoke},
        {"player_vec3_fast_normalize_smoke", player_vec3_fast_normalize_smoke},
        {"player_constrain_to_unit_distance_from_smoke",
         player_constrain_to_unit_distance_from_smoke},
        {"hud_sensor_tracker_parse_checkpoint_number_from_node_smoke",
         hud_sensor_tracker_parse_checkpoint_number_from_node_smoke},
        {"player_start_modal_loop_sfx_handle_smoke", player_start_modal_loop_sfx_handle_smoke},
        {"player_start_master_type_loop_sfx_handle_smoke",
         player_start_master_type_loop_sfx_handle_smoke},
        {"player_ensure_master_type_loop_sfx_handle_smoke",
         player_ensure_master_type_loop_sfx_handle_smoke},
        {"player_stop_master_type_loop_sfx_handle_smoke",
         player_stop_master_type_loop_sfx_handle_smoke},
        {"player_stop_modal_loop_sfx_handle_smoke", player_stop_modal_loop_sfx_handle_smoke},
        {"player_update_modal_loop_sfx_smoke", player_update_modal_loop_sfx_smoke},
        {"player_select_modal_state_by_master_type_smoke",
         player_select_modal_state_by_master_type_smoke},
        {"player_master_type_transition_leaf_smoke", player_master_type_transition_leaf_smoke},
        {"player_apply_master_type_transition_smoke", player_apply_master_type_transition_smoke},
        {"player_transition_to_master_type_track_smoke",
         player_transition_to_master_type_track_smoke},
        {"player_transition_to_master_type_amphib_smoke",
         player_transition_to_master_type_amphib_smoke},
        {"player_transition_to_master_type_sub_smoke",
         player_transition_to_master_type_sub_smoke},
        {"player_transition_to_master_type_hover_smoke",
         player_transition_to_master_type_hover_smoke},
        {"player_cache_disable_copter_snd_nodes_smoke",
         player_cache_disable_copter_snd_nodes_smoke},
        {"player_reactivate_copter_snd_nodes_if_healthy_smoke",
         player_reactivate_copter_snd_nodes_if_healthy_smoke},
        {"player_start_slip_sfx_smoke", player_start_slip_sfx_smoke},
        {"player_stop_slip_sfx_smoke", player_stop_slip_sfx_smoke},
        {"player_float_sign_smoke", player_float_sign_smoke},
        {"player_update_bank_and_turn_dynamics_smoke",
         player_update_bank_and_turn_dynamics_smoke},
        {"player_compute_turn_slip_delta_smoke", player_compute_turn_slip_delta_smoke},
        {"player_update_sub_vertical_damping_smoke",
         player_update_sub_vertical_damping_smoke},
        {"player_update_yaw_velocity_from_steer_input_smoke",
         player_update_yaw_velocity_from_steer_input_smoke},
        {"player_select_probe_sample_height_smoke", player_select_probe_sample_height_smoke},
        {"player_build_environment_probe_result_smoke",
         player_build_environment_probe_result_smoke},
        {"player_los_from_fx_offset_smoke", player_los_from_fx_offset_smoke},
        {"player_apply_aim_pitch_to_direction_smoke",
         player_apply_aim_pitch_to_direction_smoke},
        {"player_apply_environment_probe_result_smoke",
         player_apply_environment_probe_result_smoke},
        {"player_surface_height_and_terrain_tilt_smoke",
         player_surface_height_and_terrain_tilt_smoke},
        {"player_compute_surface_from1_probe_smoke",
         player_compute_surface_from1_probe_smoke},
        {"player_compute_triangle_normal_smoke", player_compute_triangle_normal_smoke},
        {"player_compute_surface_from2_probes_smoke",
         player_compute_surface_from2_probes_smoke},
        {"player_check_probe_sample_mask_overlap_smoke",
         player_check_probe_sample_mask_overlap_smoke},
        {"player_rebuild_above_ground_indices_smoke",
         player_rebuild_above_ground_indices_smoke},
        {"player_select_best_probes_by_dot_product_smoke",
         player_select_best_probes_by_dot_product_smoke},
        {"player_compute_surface_from3_probes_smoke",
         player_compute_surface_from3_probes_smoke},
        {"player_process_env_probe_results_smoke", player_process_env_probe_results_smoke},
        {"player_rebuild_orientation_from_normal_smoke",
         player_rebuild_orientation_from_normal_smoke},
        {"player_rebuild_steer_basis_raw_from_ref_smoke",
         player_rebuild_steer_basis_raw_from_ref_smoke},
        {"player_rebuild_motion_basis_from_steer_basis_smoke",
         player_rebuild_motion_basis_from_steer_basis_smoke},
        {"player_apply_amphib_speed_oscillation_smoke",
         player_apply_amphib_speed_oscillation_smoke},
        {"player_find_third_probe_and_compute_normal_smoke",
         player_find_third_probe_and_compute_normal_smoke},
        {"player_accumulate_slope_forces_smoke", player_accumulate_slope_forces_smoke},
        {"player_update_vertical_velocity_and_transform_smoke",
         player_update_vertical_velocity_and_transform_smoke},
        {"player_update_post_move_environment_smoke",
         player_update_post_move_environment_smoke},
        {"player_update_master_type_track_smoke", player_update_master_type_track_smoke},
        {"player_probe_modal_sample_heights_smoke", player_probe_modal_sample_heights_smoke},
        {"player_update_master_type_hover_from_modal_probe_smoke",
         player_update_master_type_hover_from_modal_probe_smoke},
        {"player_update_master_type_hover_smoke", player_update_master_type_hover_smoke},
        {"player_update_master_type_amphib_from_modal_probe_smoke",
         player_update_master_type_amphib_from_modal_probe_smoke},
        {"player_update_sub_mode_water_probe_state_smoke",
         player_update_sub_mode_water_probe_state_smoke},
        {"player_update_master_type_sub_smoke", player_update_master_type_sub_smoke},
        {"player_tick_master_type_and_force_feedback_smoke",
         player_tick_master_type_and_force_feedback_smoke},
        {"player_update_master_type_amphib_smoke", player_update_master_type_amphib_smoke},
        {"player_update_master_type_basic_smoke", player_update_master_type_basic_smoke},
        {"player_update_third_person_camera_smoke", player_update_third_person_camera_smoke},
        {"player_apply_camera_state_and_zopt_set_camera_mode_smoke",
         player_apply_camera_state_and_zopt_set_camera_mode_smoke},
        {"player_zar_write_mission_save_data_section_smoke",
         player_zar_write_mission_save_data_section_smoke},
        {"player_restore_recorded_node_flags_smoke", player_restore_recorded_node_flags_smoke},
        {"player_record_node_flags_for_restore_smoke",
         player_record_node_flags_for_restore_smoke},
        {"zutil_save_game_state_free_owned_resources_smoke",
         zutil_save_game_state_free_owned_resources_smoke},
        {"player_zar_register_sections_smoke", player_zar_register_sections_smoke},
        {"player_zar_read_vehicle_list_section_smoke",
         player_zar_read_vehicle_list_section_smoke},
        {"player_zar_write_vehicle_list_section_smoke",
         player_zar_write_vehicle_list_section_smoke},
        {"player_write_mines_zar_section_smoke", player_write_mines_zar_section_smoke},
        {"player_mines_zar_read_entry_or_reset_smoke",
         player_mines_zar_read_entry_or_reset_smoke},
        {"player_destroy_save_game_state_smoke", player_destroy_save_game_state_smoke},
        {"player_shutdown_mission_runtime_smoke", player_shutdown_mission_runtime_smoke},
        {"zfmv_script_reset_smoke", zfmv_script_reset_smoke},
        {"zfmv_script_init_null_path_smoke", zfmv_script_init_null_path_smoke},
        {"zfmv_script_cleanup_smoke", zfmv_script_cleanup_smoke},
        {"zfmv_script_append_action_smoke", zfmv_script_append_action_smoke},
        {"zfmv_script_run_blocking_empty_smoke", zfmv_script_run_blocking_empty_smoke},
        {"zfmv_script_begin_current_action_smoke", zfmv_script_begin_current_action_smoke},
        {"zfmv_script_begin_at_time_smoke", zfmv_script_begin_at_time_smoke},
        {"zfmv_script_update_smoke", zfmv_script_update_smoke},
        {"zfmv_script_update_at_time_smoke", zfmv_script_update_at_time_smoke},
        {"zfmv_script_begin_now_smoke", zfmv_script_begin_now_smoke},
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
        {"zfmv_stream_open_audio_missing_file_smoke",
         zfmv_stream_open_audio_missing_file_smoke},
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
        {"zutil_store_int32_smoke", zutil_store_int32_smoke},
        {"zgame_options_init_registry_context_smoke", zgame_options_init_registry_context_smoke},
        {"zmodel_display_init_smoke", zmodel_display_init_smoke},
        {"zmodel_backface_elimination_tolerance_smoke",
         zmodel_backface_elimination_tolerance_smoke},
        {"zmodel_small_poly_reject_thresholds_smoke",
         zmodel_small_poly_reject_thresholds_smoke},
        {"zmodel_set_vertex_shading_enabled_smoke",
         zmodel_set_vertex_shading_enabled_smoke},
        {"zmodel_render_state_setters_smoke", zmodel_render_state_setters_smoke},
        {"zmodel_const_tolerances_and_cross_smoke", zmodel_const_tolerances_and_cross_smoke},
        {"zdi_add_polygon_wrapper_smoke", zdi_add_polygon_wrapper_smoke},
        {"zmodel_damage_mask_uv_smoke", zmodel_damage_mask_uv_smoke},
        {"zmodel_damage_mask_stamp_smoke", zmodel_damage_mask_stamp_smoke},
        {"zmodel_display_shutdown_smoke", zmodel_display_shutdown_smoke},
        {"zmodel_material_pool_entry_smoke", zmodel_material_pool_entry_smoke},
        {"zmodel_matl_init_globals_smoke", zmodel_matl_init_globals_smoke},
        {"zmodel_matlbuffer_set_array_size_smoke", zmodel_matlbuffer_set_array_size_smoke},
        {"zmodel_set_display_instance_pool_capacity_smoke",
         zmodel_set_display_instance_pool_capacity_smoke},
        {"zmodel_set_software_path_active_smoke", zmodel_set_software_path_active_smoke},
        {"zmodel_matlbuffer_release_texture_surfaces_smoke",
         zmodel_matlbuffer_release_texture_surfaces_smoke},
        {"zmodel_material_defaults_and_find_smoke", zmodel_material_defaults_and_find_smoke},
        {"zgeometry_model_add_polygon_to_di_smoke", zgeometry_model_add_polygon_to_di_smoke},
        {"zgeometry_model_polygon_uv_and_di_helpers_smoke",
         zgeometry_model_polygon_uv_and_di_helpers_smoke},
        {"zgeometry_vec3array_clip_leaf_helpers_smoke",
         zgeometry_vec3array_clip_leaf_helpers_smoke},
        {"zgeometry_clip_polygon_create_copy_finalize_smoke",
         zgeometry_clip_polygon_create_copy_finalize_smoke},
        {"zgeometry_clip_polygon_upsert_point_list_xy_smoke",
         zgeometry_clip_polygon_upsert_point_list_xy_smoke},
        {"zgeometry_model_linear_buffer_and_bounds_overlap_smoke",
         zgeometry_model_linear_buffer_and_bounds_overlap_smoke},
        {"zgeometry_model_is_fully_inside_clip_polygon_xy_smoke",
         zgeometry_model_is_fully_inside_clip_polygon_xy_smoke},
        {"zgeometry_model_process_clip_patch_node_smoke",
         zgeometry_model_process_clip_patch_node_smoke},
        {"zgeometry_clip_polygon_process_node_polygon_set_xy_smoke",
         zgeometry_clip_polygon_process_node_polygon_set_xy_smoke},
        {"zgeometry_model_clip_patch_smoke", zgeometry_model_clip_patch_smoke},
        {"zdeclient_qsand_build_smoke", zdeclient_qsand_build_smoke},
        {"zgeometry_polygon_convexify_and_triangulate_smoke",
         zgeometry_polygon_convexify_and_triangulate_smoke},
        {"zgeometry_triangulate_hole_and_orientation_smoke",
         zgeometry_triangulate_hole_and_orientation_smoke},
        {"zgeometry_polygon_snap_points_xy_if_near_smoke",
         zgeometry_polygon_snap_points_xy_if_near_smoke},
        {"zgeometry_clip_polygon_snap_points_near_node_model_xy_smoke",
         zgeometry_clip_polygon_snap_points_near_node_model_xy_smoke},
        {"zmodel_material_write_gamez_smoke", zmodel_material_write_gamez_smoke},
        {"zmodel_material_read_gamez_smoke", zmodel_material_read_gamez_smoke},
        {"zmodel_dipool_write_to_stream_smoke", zmodel_dipool_write_to_stream_smoke},
        {"zmodel_dipool_read_from_stream_smoke", zmodel_dipool_read_from_stream_smoke},
        {"zmodel_dipool_read_entry_by_index_from_stream_smoke",
         zmodel_dipool_read_entry_by_index_from_stream_smoke},
        {"zmodel_material_and_di_clone_smoke", zmodel_material_and_di_clone_smoke},
        {"zmodel_material_update_cycle_if_needed_smoke",
         zmodel_material_update_cycle_if_needed_smoke},
        {"zmodel_scrolling_texture_update_smoke", zmodel_scrolling_texture_update_smoke},
        {"zmodel_render_point_queue_entry_smoke", zmodel_render_point_queue_entry_smoke},
        {"zmodel_init_smoke", zmodel_init_smoke},
        {"zmodel_render_node_hardware_flat_smoke", zmodel_render_node_hardware_flat_smoke},
        {"zmodel_light_fog_fade_smoke", zmodel_light_fog_fade_smoke},
        {"zmodel_light_build_light_weights_smoke", zmodel_light_build_light_weights_smoke},
        {"zmodel_light_point_in_polygon_init_smoke", zmodel_light_point_in_polygon_init_smoke},
        {"zmodel_light_set_active_lights_smoke", zmodel_light_set_active_lights_smoke},
        {"zmodel_light_build_attr0_depth_fade_smoke", zmodel_light_build_attr0_depth_fade_smoke},
        {"zmodel_light_build_attr1_falloff_smoke", zmodel_light_build_attr1_falloff_smoke},
        {"zclass_world_build_active_light_list_smoke",
         zclass_world_build_active_light_list_smoke},
        {"zclass_world_apply_pending_fog_settings_smoke",
         zclass_world_apply_pending_fog_settings_smoke},
        {"zclass_world_settings_section_callbacks_smoke",
         zclass_world_settings_section_callbacks_smoke},
        {"zclass_shutdown_core_smoke", zclass_shutdown_core_smoke},
        {"zclass_gwnode_update_smoke", zclass_gwnode_update_smoke},
        {"zclass_init_smoke", zclass_init_smoke},
        {"zgame_options_find_option_smoke", zgame_options_find_option_smoke},
        {"zgame_options_runtime_config_copy_default_smoke",
         zgame_options_runtime_config_copy_default_smoke},
        {"zopt_lookup_named_value_as_int_smoke", zopt_lookup_named_value_as_int_smoke},
        {"zopt_read_scalar_value_as_int_smoke", zopt_read_scalar_value_as_int_smoke},
        {"zopt_evaluate_profile_metric_condition_smoke",
         zopt_evaluate_profile_metric_condition_smoke},
        {"zopt_select_profile_value_for_system_smoke",
         zopt_select_profile_value_for_system_smoke},
        {"zgame_options_load_from_registry_missing_smoke",
         zgame_options_load_from_registry_missing_smoke},
        {"zgame_options_save_game_options_smoke", zgame_options_save_game_options_smoke},
        {"zgame_options_shutdown_registry_context_smoke",
         zgame_options_shutdown_registry_context_smoke},
        {"zgame_options_load_game_options_minimal_smoke",
         zgame_options_load_game_options_minimal_smoke},
        {"zopt_eval_int_compare_op_smoke", zopt_eval_int_compare_op_smoke},
        {"zopt_fullscreen_accessors_smoke", zopt_fullscreen_accessors_smoke},
        {"zopt_toggle_hud_type_for_current_hw_mode_smoke",
         zopt_toggle_hud_type_for_current_hw_mode_smoke},
        {"zopt_network_enabled_accessor_smoke", zopt_network_enabled_accessor_smoke},
        {"hud_sensor_objective_slot_reset_smoke", hud_sensor_objective_slot_reset_smoke},
        {"hud_sensor_tracker_unload_objectives_smoke", hud_sensor_tracker_unload_objectives_smoke},
        {"hud_sensor_tracker_get_objective_briefing_strings_smoke",
         hud_sensor_tracker_get_objective_briefing_strings_smoke},
        {"hud_sensor_tracker_load_race_checkpoint_meta_smoke",
         hud_sensor_tracker_load_race_checkpoint_meta_smoke},
        {"hud_sensor_tracker_set_runtime_timer_sec_and_goal_value_smoke",
         hud_sensor_tracker_set_runtime_timer_sec_and_goal_value_smoke},
        {"hud_sensor_tracker_load_mission_core_resources_smoke",
         hud_sensor_tracker_load_mission_core_resources_smoke},
        {"hud_sensor_tracker_load_objectives_from_path_smoke",
         hud_sensor_tracker_load_objectives_from_path_smoke},
        {"hud_sensor_tracker_load_objectives_from_zrd_smoke",
         hud_sensor_tracker_load_objectives_from_zrd_smoke},
        {"hud_sensor_tracker_load_mission_weather_fx_smoke",
         hud_sensor_tracker_load_mission_weather_fx_smoke},
        {"zopt_view_rect_target_side_effects_smoke", zopt_view_rect_target_side_effects_smoke},
        {"zopt_section_accessor_smoke", zopt_section_accessor_smoke},
        {"zclipalt_set_source_rect_smoke", zclipalt_set_source_rect_smoke},
        {"zclipalt_set_target_rect_smoke", zclipalt_set_target_rect_smoke},
        {"zclipalt_remap_point_xy_in_place_smoke", zclipalt_remap_point_xy_in_place_smoke},
        {"zclass_cls_di_segment_batch_vs_polygon_smoke",
         zclass_cls_di_segment_batch_vs_polygon_smoke},
        {"zclass_cls_di_segment_batch_vs_polygon_uv_smoke",
         zclass_cls_di_segment_batch_vs_polygon_uv_smoke},
        {"zclass_cls_di_filter_regions_polygon_damage_mask_uv_smoke",
         zclass_cls_di_filter_regions_polygon_damage_mask_uv_smoke},
        {"zclass_cls_di_filter_regions_against_polygon_smoke",
         zclass_cls_di_filter_regions_against_polygon_smoke},
        {"zclass_cls_di_frustum_test_and_pick_smoke",
         zclass_cls_di_frustum_test_and_pick_smoke},
        {"zclass_cls_di_point_query_chain_smoke", zclass_cls_di_point_query_chain_smoke},
        {"zclass_cls_di_snap_probe_point_y_to_best_candidate_smoke",
         zclass_cls_di_snap_probe_point_y_to_best_candidate_smoke},
        {"zclass_cls_di_segment_batch_recursive_smoke",
         zclass_cls_di_segment_batch_recursive_smoke},
        {"zclass_cls_di_segment_grid_window_smoke",
         zclass_cls_di_segment_grid_window_smoke},
        {"zclass_cls_di_probe_hit_batches_for_segments_smoke",
         zclass_cls_di_probe_hit_batches_for_segments_smoke},
        {"zclass_cls_di_set_stop_after_first_hit_smoke",
         zclass_cls_di_set_stop_after_first_hit_smoke},
        {"zhud_element_constructor_smoke", zhud_element_constructor_smoke},
        {"zhud_element_copy_constructor_smoke", zhud_element_copy_constructor_smoke},
        {"zhud_element_set_timer_smoke", zhud_element_set_timer_smoke},
        {"zhud_circle_constructor_and_hit_test_smoke", zhud_circle_constructor_and_hit_test_smoke},
        {"zhud_composite_panel_vector_clear_smoke", zhud_composite_panel_vector_clear_smoke},
        {"zhud_panel_layout_entry_copy_construct_smoke",
         zhud_panel_layout_entry_copy_construct_smoke},
        {"zhud_panel_layout_entry_copy_assign_smoke",
         zhud_panel_layout_entry_copy_assign_smoke},
        {"zhud_panel_layout_entry_copy_assign_range_smoke",
         zhud_panel_layout_entry_copy_assign_range_smoke},
        {"zhud_panel_layout_entry_destroy_range_smoke",
         zhud_panel_layout_entry_destroy_range_smoke},
        {"zhud_panel_span_clear_smoke", zhud_panel_span_clear_smoke},
        {"zhud_panel_span_copy_init_smoke", zhud_panel_span_copy_init_smoke},
        {"zhud_panel_span_copy_from_smoke", zhud_panel_span_copy_from_smoke},
        {"zhud_panel_span_destroy_and_free_smoke", zhud_panel_span_destroy_and_free_smoke},
        {"zhud_panel_span_insert_n_smoke", zhud_panel_span_insert_n_smoke},
        {"zhud_panel_span_vec_insert_n_smoke", zhud_panel_span_vec_insert_n_smoke},
        {"zhud_credits_panel_destructor_smoke", zhud_credits_panel_destructor_smoke},
        {"zhud_credits_panel_scalar_deleting_destructor_smoke",
         zhud_credits_panel_scalar_deleting_destructor_smoke},
        {"zhud_panel_destructor_callback_smoke", zhud_panel_destructor_callback_smoke},
        {"zhud_scrolling_text_destructor_smoke", zhud_scrolling_text_destructor_smoke},
        {"zhud_scrolling_text_scalar_deleting_destructor_smoke",
         zhud_scrolling_text_scalar_deleting_destructor_smoke},
        {"zhud_credits_panel_constructor_smoke", zhud_credits_panel_constructor_smoke},
        {"zhud_scrolling_text_load_from_zrd_smoke", zhud_scrolling_text_load_from_zrd_smoke},
        {"zhud_scrolling_text_update_smoke", zhud_scrolling_text_update_smoke},
        {"zhud_scrolling_text_on_activate_reset_owner_fade_smoke",
         zhud_scrolling_text_on_activate_reset_owner_fade_smoke},
        {"zhud_scrolling_text_update_scroll_positions_smoke",
         zhud_scrolling_text_update_scroll_positions_smoke},
        {"zhud_credits_panel_update_fade_and_exit_smoke",
         zhud_credits_panel_update_fade_and_exit_smoke},
        {"zhud_composite_panel_vector_insert_copies_smoke",
         zhud_composite_panel_vector_insert_copies_smoke},
        {"zhud_composite_panel_constructor_with_entry_count_smoke",
         zhud_composite_panel_constructor_with_entry_count_smoke},
        {"zhud_composite_panel_layout_entries_smoke",
         zhud_composite_panel_layout_entries_smoke},
        {"zhud_composite_panel_resize_entry_count_smoke",
         zhud_composite_panel_resize_entry_count_smoke},
        {"zhud_composite_panel_resize_vector_relayout_smoke",
         zhud_composite_panel_resize_vector_relayout_smoke},
        {"zhud_composite_panel_entry_copy_smoke", zhud_composite_panel_entry_copy_smoke},
        {"zhud_flash_panel_compute_blend_color_smoke", zhud_flash_panel_compute_blend_color_smoke},
        {"zhud_layout_shutdown_stub_smoke", zhud_layout_shutdown_stub_smoke},
        {"zhud_layout_hw_release_images_smoke", zhud_layout_hw_release_images_smoke},
        {"zhud_layout_hw_update_objective_dirty_rect_smoke",
         zhud_layout_hw_update_objective_dirty_rect_smoke},
        {"zhud_element_clip_and_invalidate_smoke", zhud_element_clip_and_invalidate_smoke},
        {"zhud_element_visible_smoke", zhud_element_visible_smoke},
        {"zhud_element_update_smoke", zhud_element_update_smoke},
        {"zhud_element_position_mutators_smoke", zhud_element_position_mutators_smoke},
        {"zhud_primitive_bind_target_set_segment_endpoints_smoke",
         zhud_primitive_bind_target_set_segment_endpoints_smoke},
        {"zhud_background_bind_primitive_node_to_element_smoke",
         zhud_background_bind_primitive_node_to_element_smoke},
        {"zhud_container_child_list_smoke", zhud_container_child_list_smoke},
        {"zhud_container_destructor_core_smoke", zhud_container_destructor_core_smoke},
        {"zhud_widget_constructor_smoke", zhud_widget_constructor_smoke},
        {"zhud_widget_default_ctor_thunk_smoke", zhud_widget_default_ctor_thunk_smoke},
        {"zhud_widget_invalidate_rect_smoke", zhud_widget_invalidate_rect_smoke},
        {"hud_ui_widget_draw_smoke", hud_ui_widget_draw_smoke},
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
        {"zhud_cmd_dialog_on_command_selection_changed_smoke",
         zhud_cmd_dialog_on_command_selection_changed_smoke},
        {"zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke",
         zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke},
        {"zhud_cmd_dialog_rebuild_command_binding_lists_smoke",
         zhud_cmd_dialog_rebuild_command_binding_lists_smoke},
        {"zhud_cmd_dialog_constructor_smoke", zhud_cmd_dialog_constructor_smoke},
        {"zhud_cmd_dialog_state_on_try_become_current_smoke",
         zhud_cmd_dialog_state_on_try_become_current_smoke},
        {"zhud_cmd_dialog_select_group_relative_smoke",
         zhud_cmd_dialog_select_group_relative_smoke},
        {"zhud_cmd_set_list_widget_on_activate_smoke",
         zhud_cmd_set_list_widget_on_activate_smoke},
        {"zhud_cmd_dialog_callback_navigation_smoke",
         zhud_cmd_dialog_callback_navigation_smoke},
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
        {"zhud_cmd_dialog_select_command_relative_smoke",
         zhud_cmd_dialog_select_command_relative_smoke},
        {"zhud_cmd_reset_button_on_activate_smoke", zhud_cmd_reset_button_on_activate_smoke},
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
        {"zhud_message_box_leaf_handlers_smoke", zhud_message_box_leaf_handlers_smoke},
        {"zhud_message_box_constructor_fallback_smoke",
         zhud_message_box_constructor_fallback_smoke},
        {"zhud_message_box_run_modal_smoke", zhud_message_box_run_modal_smoke},
        {"zhud_message_box_destructor_smoke", zhud_message_box_destructor_smoke},
        {"zhud_message_box_scalar_deleting_destructor_smoke",
         zhud_message_box_scalar_deleting_destructor_smoke},
        {"zhud_background_container_focus_smoke", zhud_background_container_focus_smoke},
        {"zhud_background_update_input_focus_smoke", zhud_background_update_input_focus_smoke},
        {"zhud_background_cursor_widget_member_constructor_smoke",
         zhud_background_cursor_widget_member_constructor_smoke},
        {"zhud_background_cursor_widget_rebuild_captured_image_smoke",
         zhud_background_cursor_widget_rebuild_captured_image_smoke},
        {"zhud_background_cursor_widget_set_image_borrowed_refresh_smoke",
         zhud_background_cursor_widget_set_image_borrowed_refresh_smoke},
        {"zhud_background_cursor_widget_set_capture_enabled_smoke",
         zhud_background_cursor_widget_set_capture_enabled_smoke},
        {"zhud_background_video_widget_destructor_smoke",
         zhud_background_video_widget_destructor_smoke},
        {"zhud_background_constructor_smoke", zhud_background_constructor_smoke},
        {"zhud_background_bind_widget_by_name_smoke", zhud_background_bind_widget_by_name_smoke},
        {"zhud_background_load_zrd_and_section_null_root_smoke",
         zhud_background_load_zrd_and_section_null_root_smoke},
        {"zhud_background_load_from_zrd_missing_path_smoke",
         zhud_background_load_from_zrd_missing_path_smoke},
        {"hud_ui_cheat_code_dialog_constructor_smoke",
         hud_ui_cheat_code_dialog_constructor_smoke},
        {"hud_ui_cheat_code_dialog_destructor_smoke",
         hud_ui_cheat_code_dialog_destructor_smoke},
        {"hud_ui_cheat_code_dialog_scalar_deleting_destructor_smoke",
         hud_ui_cheat_code_dialog_scalar_deleting_destructor_smoke},
        {"zhud_background_free_loaded_tree_roots_smoke",
         zhud_background_free_loaded_tree_roots_smoke},
        {"zhud_background_video_widget_set_color_key_smoke",
         zhud_background_video_widget_set_color_key_smoke},
        {"zhud_background_video_widget_set_media_path_missing_smoke",
         zhud_background_video_widget_set_media_path_missing_smoke},
        {"zhud_play_powerup_sfx_smoke", zhud_play_powerup_sfx_smoke},
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
        {"zhud_triplet_panel_draw_smoke", zhud_triplet_panel_draw_smoke},
        {"zhud_triplet_panel_shutdown_items_stub_smoke",
         zhud_triplet_panel_shutdown_items_stub_smoke},
        {"zhud_triplet_panel_destructor_core_smoke", zhud_triplet_panel_destructor_core_smoke},
        {"zhud_text_input_destructor_core_smoke", zhud_text_input_destructor_core_smoke},
        {"zhud_text_input_constructor_and_alloc_smoke",
         zhud_text_input_constructor_and_alloc_smoke},
        {"zhud_mgr_objective_block_destructor_smoke", zhud_mgr_objective_block_destructor_smoke},
        {"hud_ui_net_exit_destroy_global_smoke", hud_ui_net_exit_destroy_global_smoke},
        {"hud_ui_net_exit_show_tick_smoke", hud_ui_net_exit_show_tick_smoke},
        {"hud_ui_net_exit_constructor_smoke", hud_ui_net_exit_constructor_smoke},
        {"hud_ui_net_exit_create_global_smoke", hud_ui_net_exit_create_global_smoke},
        {"hud_ui_aux_overlay_text_lines_smoke", hud_ui_aux_overlay_text_lines_smoke},
        {"zhud_mgr_destroy_sensor_window_null_smoke", zhud_mgr_destroy_sensor_window_null_smoke},
        {"zhud_mgr_disable_hud_smoke", zhud_mgr_disable_hud_smoke},
        {"zhud_mgr_enable_hud_smoke", zhud_mgr_enable_hud_smoke},
        {"zhud_mgr_toggle_hud_smoke", zhud_mgr_toggle_hud_smoke},
        {"zhud_mgr_switch_active_dialog_smoke", zhud_mgr_switch_active_dialog_smoke},
        {"zhud_mgr_set_float_timer_visible_smoke",
         zhud_mgr_set_float_timer_visible_smoke},
        {"zhud_mgr_hide_tracked_progress_meter_if_owner_matches_smoke",
         zhud_mgr_hide_tracked_progress_meter_if_owner_matches_smoke},
        {"zhud_mgr_target_update_selected_progress_meter_smoke",
         zhud_mgr_target_update_selected_progress_meter_smoke},
        {"zhud_mgr_set_aux_overlay_visible_smoke",
         zhud_mgr_set_aux_overlay_visible_smoke},
        {"zhud_mgr_apply_hud_mode_switch_smoke", zhud_mgr_apply_hud_mode_switch_smoke},
        {"zhud_mgr_ensure_hud_loaded_minimal_smoke",
         zhud_mgr_ensure_hud_loaded_minimal_smoke},
        {"zhud_slot_destructors_smoke", zhud_slot_destructors_smoke},
        {"zhud_slot_draw_smoke", zhud_slot_draw_smoke},
        {"zhud_stats_list_element_update_smoke", zhud_stats_list_element_update_smoke},
        {"zhud_counter_constructor_smoke", zhud_counter_constructor_smoke},
        {"zhud_counter_update_layout_position_smoke",
         zhud_counter_update_layout_position_smoke},
        {"zhud_counter_apply_from_layout_node_smoke",
         zhud_counter_apply_from_layout_node_smoke},
        {"zhud_counter_release_state_images_smoke", zhud_counter_release_state_images_smoke},
        {"zhud_message_release_images_smoke", zhud_message_release_images_smoke},
        {"zhud_message_set_value_if_owner_matches_smoke",
         zhud_message_set_value_if_owner_matches_smoke},
        {"zhud_message_update_selected_weapon_display_smoke",
         zhud_message_update_selected_weapon_display_smoke},
        {"zhud_message_constructor_smoke", zhud_message_constructor_smoke},
        {"zhud_message_rebuild_weapon_layout_smoke", zhud_message_rebuild_weapon_layout_smoke},
        {"zhud_message_load_weapon_layout_from_node_smoke",
         zhud_message_load_weapon_layout_from_node_smoke},
        {"zhud_message_destructors_smoke", zhud_message_destructors_smoke},
        {"zhud_shield_message_widget_destructor_smoke",
         zhud_shield_message_widget_destructor_smoke},
        {"zhud_mgr_sensor_set_shield_message_ratio_smoke",
         zhud_mgr_sensor_set_shield_message_ratio_smoke},
        {"zhud_shield_message_widget_apply_layout_smoke",
         zhud_shield_message_widget_apply_layout_smoke},
        {"zhud_bar_and_meter_constructor_smoke", zhud_bar_and_meter_constructor_smoke},
        {"zhud_polyline_and_slider_border_constructor_smoke",
         zhud_polyline_and_slider_border_constructor_smoke},
        {"zhud_polyline_draw_and_slider_update_smoke", zhud_polyline_draw_and_slider_update_smoke},
        {"zhud_panel_scalar_deleting_destructor_smoke",
         zhud_panel_scalar_deleting_destructor_smoke},
        {"zhud_util_free_field_ptr_smoke", zhud_util_free_field_ptr_smoke},
        {"zhud_cmd_binding_entry_copy_range_smoke",
         zhud_cmd_binding_entry_copy_range_smoke},
        {"zhud_cmd_binding_destroy_range_smoke",
         zhud_cmd_binding_destroy_range_smoke},
        {"zhud_std_ptr_vector_clear_no_op_destroy_smoke",
         zhud_std_ptr_vector_clear_no_op_destroy_smoke},
        {"zhud_cmd_command_list_destructor_smoke",
         zhud_cmd_command_list_destructor_smoke},
        {"zhud_cmd_command_list_scalar_deleting_destructor_smoke",
         zhud_cmd_command_list_scalar_deleting_destructor_smoke},
        {"zhud_cmd_key_a_button_destructor_smoke",
         zhud_cmd_key_a_button_destructor_smoke},
        {"zhud_cmd_key_b_button_destructor_smoke",
         zhud_cmd_key_b_button_destructor_smoke},
        {"zhud_cmd_joy_button_destructor_smoke",
         zhud_cmd_joy_button_destructor_smoke},
        {"zhud_cmd_mouse_button_destructor_smoke",
         zhud_cmd_mouse_button_destructor_smoke},
        {"zhud_cmd_bind_button_base_destructor_core_smoke",
         zhud_cmd_bind_button_base_destructor_core_smoke},
        {"zhud_cmd_dialog_destructor_smoke", zhud_cmd_dialog_destructor_smoke},
        {"zhud_cmd_dialog_scalar_deleting_destructor_smoke",
         zhud_cmd_dialog_scalar_deleting_destructor_smoke},
        {"zhud_options_dialog_constructor_smoke",
         zhud_options_dialog_constructor_smoke},
        {"zhud_options_dialog_destructor_core_smoke",
         zhud_options_dialog_destructor_core_smoke},
        {"zhud_options_dialog_scalar_deleting_destructor_smoke",
         zhud_options_dialog_scalar_deleting_destructor_smoke},
        {"zhud_options_panel_lighting_init_from_options_smoke",
         zhud_options_panel_lighting_init_from_options_smoke},
        {"zhud_options_panel_lighting_sync_from_options_smoke",
         zhud_options_panel_lighting_sync_from_options_smoke},
        {"zhud_options_panel_perspective_init_from_options_smoke",
         zhud_options_panel_perspective_init_from_options_smoke},
        {"zhud_options_panel_perspective_sync_from_options_smoke",
         zhud_options_panel_perspective_sync_from_options_smoke},
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
        {"zhud_options_panel_effects_sync_from_options_smoke",
         zhud_options_panel_effects_sync_from_options_smoke},
        {"zhud_options_panel_sound_active_init_from_options_smoke",
         zhud_options_panel_sound_active_init_from_options_smoke},
        {"zhud_options_panel_sound_active_sync_from_options_smoke",
         zhud_options_panel_sound_active_sync_from_options_smoke},
        {"zhud_options_panel_music_enable_sync_from_options_smoke",
         zhud_options_panel_music_enable_sync_from_options_smoke},
        {"zhud_options_panel_music_enable_on_activate_smoke",
         zhud_options_panel_music_enable_on_activate_smoke},
        {"zhud_options_panel_music_volume_sync_from_options_smoke",
         zhud_options_panel_music_volume_sync_from_options_smoke},
        {"zhud_options_panel_music_volume_on_activate_smoke",
         zhud_options_panel_music_volume_on_activate_smoke},
        {"zhud_options_panel_resolution_sync_from_options_smoke",
         zhud_options_panel_resolution_sync_from_options_smoke},
        {"zhud_options_panel_resolution_on_activate_smoke",
         zhud_options_panel_resolution_on_activate_smoke},
        {"zhud_options_panel_sound_quality_init_from_options_smoke",
         zhud_options_panel_sound_quality_init_from_options_smoke},
        {"zhud_options_panel_sound_quality_sync_from_options_smoke",
         zhud_options_panel_sound_quality_sync_from_options_smoke},
        {"zhud_options_panel_sound_volume_sync_from_options_smoke",
         zhud_options_panel_sound_volume_sync_from_options_smoke},
        {"zhud_options_panel_sound_volume_on_activate_smoke",
         zhud_options_panel_sound_volume_on_activate_smoke},
        {"zhud_cmd_dialog_state_lifecycle_smoke",
         zhud_cmd_dialog_state_lifecycle_smoke},
        {"zhud_cmd_dialog_state_queue_enter_smoke",
         zhud_cmd_dialog_state_queue_enter_smoke},
        {"zhud_cmd_dialog_state_on_deactivate_smoke",
         zhud_cmd_dialog_state_on_deactivate_smoke},
        {"zhud_panel_destructor_thunk_smoke", zhud_panel_destructor_thunk_smoke},
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
        {"zhud_objective_refresh_counter_text_smoke", zhud_objective_refresh_counter_text_smoke},
        {"zhud_objective_update_meter_xpoints_smoke",
         zhud_objective_update_meter_xpoints_smoke},
        {"zhud_objective_tick_meter_fill_animation_smoke",
         zhud_objective_tick_meter_fill_animation_smoke},
        {"zhud_layout_node_read_rect_offset_and_size_smoke",
         zhud_layout_node_read_rect_offset_and_size_smoke},
        {"zhud_layout_node_read_rect_smoke", zhud_layout_node_read_rect_smoke},
        {"zhud_layout_node_read_int3_smoke", zhud_layout_node_read_int3_smoke},
        {"zhud_layout_node_apply_text_label_smoke", zhud_layout_node_apply_text_label_smoke},
        {"zhud_layout_node_apply_image_widget_smoke",
         zhud_layout_node_apply_image_widget_smoke},
        {"zhud_layout_base_load_type_i_from_zar_root_smoke",
         zhud_layout_base_load_type_i_from_zar_root_smoke},
        {"zhud_layout_hw_load_type_ii_from_zar_root_smoke",
         zhud_layout_hw_load_type_ii_from_zar_root_smoke},
        {"zhud_timer_panel_float_constructor_smoke", zhud_timer_panel_float_constructor_smoke},
        {"zhud_triplet_constructor_smoke", zhud_triplet_constructor_smoke},
        {"zhud_triplet_interpolate_layout_smoke", zhud_triplet_interpolate_layout_smoke},
        {"zhud_triplet_destructor_core_smoke", zhud_triplet_destructor_core_smoke},
        {"zhud_triplet_is_local_player_first_entry_smoke",
         zhud_triplet_is_local_player_first_entry_smoke},
        {"zhud_nanite_panel_init_layout_smoke", zhud_nanite_panel_init_layout_smoke},
        {"zhud_triplet_scoreboard_entry_update_smoke", zhud_triplet_scoreboard_entry_update_smoke},
        {"zhud_stats_list_destructors_smoke", zhud_stats_list_destructors_smoke},
        {"zhud_string_menu_destructor_core_smoke", zhud_string_menu_destructor_core_smoke},
        {"zhud_bar_set_point_xy_smoke", zhud_bar_set_point_xy_smoke},
        {"zhud_layout_node_apply_meter_quad_smoke", zhud_layout_node_apply_meter_quad_smoke},
        {"zhud_layout_node_apply_corner_text_quad_smoke",
         zhud_layout_node_apply_corner_text_quad_smoke},
        {"zhud_text_stack_constructors_smoke", zhud_text_stack_constructors_smoke},
        {"zhud_text_stack_set_font_all_smoke", zhud_text_stack_set_font_all_smoke},
        {"zhud_text_stack_push_line_smoke", zhud_text_stack_push_line_smoke},
        {"zhud_text_stack_clear_and_enable_smoke", zhud_text_stack_clear_and_enable_smoke},
        {"zhud_text_stack_clear_and_disable_smoke", zhud_text_stack_clear_and_disable_smoke},
        {"zhud_text_stack_destructor_core_smoke", zhud_text_stack_destructor_core_smoke},
        {"zhud_timed_task_remove_from_active_list_smoke",
         zhud_timed_task_remove_from_active_list_smoke},
        {"zhud_timed_task_run_immediate_action_smoke",
         zhud_timed_task_run_immediate_action_smoke},
        {"zhud_timed_task_tick_active_list_smoke", zhud_timed_task_tick_active_list_smoke},
        {"zhud_mgr_update_frame_smoke", zhud_mgr_update_frame_smoke},
        {"zhud_loading_checkpoint_init_table_smoke", zhud_loading_checkpoint_init_table_smoke},
        {"zhud_loading_checkpoint_advance_and_log_smoke",
         zhud_loading_checkpoint_advance_and_log_smoke},
        {"zhud_sensor_viewport_rect_smoke", zhud_sensor_viewport_rect_smoke},
        {"zhud_sensor_get_fx_rect_smoke", zhud_sensor_get_fx_rect_smoke},
        {"zhud_layout_apply_viewport_rect_smoke", zhud_layout_apply_viewport_rect_smoke},
        {"zhud_objective_update_smoke", zhud_objective_update_smoke},
        {"zhud_objective_begin_smoke", zhud_objective_begin_smoke},
        {"zhud_objective_start_hide_smoke", zhud_objective_start_hide_smoke},
        {"zhud_objective_show_smoke", zhud_objective_show_smoke},
        {"hud_sensor_tracker_show_objective_pickup_info_smoke",
         hud_sensor_tracker_show_objective_pickup_info_smoke},
        {"hud_sensor_tracker_objective_panel_visible_smoke",
         hud_sensor_tracker_objective_panel_visible_smoke},
        {"hud_sensor_tracker_reset_hud_for_mission_start_smoke",
         hud_sensor_tracker_reset_hud_for_mission_start_smoke},
        {"hud_sensor_tracker_update_map_scale_lerp_smoke",
         hud_sensor_tracker_update_map_scale_lerp_smoke},
        {"hud_sensor_tracker_project_world_points_smoke",
         hud_sensor_tracker_project_world_points_smoke},
        {"hud_line_clip_current_bounds_smoke", hud_line_clip_current_bounds_smoke},
        {"hud_recti_clip_segment_helpers_smoke", hud_recti_clip_segment_helpers_smoke},
        {"hud_sensor_tracker_draw_diamond_marker_smoke",
         hud_sensor_tracker_draw_diamond_marker_smoke},
        {"hud_sensor_tracker_save_marker_leaf_smoke",
         hud_sensor_tracker_save_marker_leaf_smoke},
        {"hud_sensor_tracker_save_state_marker_smoke",
         hud_sensor_tracker_save_state_marker_smoke},
        {"hud_sensor_tracker_update_smoke", hud_sensor_tracker_update_smoke},
        {"hud_sensor_map_node_draw_projected_path_smoke",
         hud_sensor_map_node_draw_projected_path_smoke},
        {"hud_sensor_map_node_draw_on_tracker_smoke",
         hud_sensor_map_node_draw_on_tracker_smoke},
        {"hud_weather_fx_constructor_smoke", hud_weather_fx_constructor_smoke},
        {"hud_weather_fx_derived_constructors_smoke",
         hud_weather_fx_derived_constructors_smoke},
        {"zhud_mgr_viewport_activation_smoke", zhud_mgr_viewport_activation_smoke},
        {"zhud_mgr_project_point_to_normalized_clamped_smoke",
         zhud_mgr_project_point_to_normalized_clamped_smoke},
        {"zhud_mgr_update_target_reticle_smoke", zhud_mgr_update_target_reticle_smoke},
        {"zhud_mgr_sensor_place_track_counter_widget_smoke",
         zhud_mgr_sensor_place_track_counter_widget_smoke},
        {"zhud_mgr_sensor_place_track_marker_smoke", zhud_mgr_sensor_place_track_marker_smoke},
        {"zhud_mgr_sensor_update_markers_and_progress_smoke",
         zhud_mgr_sensor_update_markers_and_progress_smoke},
        {"zhud_mgr_init_layouts_reentry_smoke", zhud_mgr_init_layouts_reentry_smoke},
        {"zhud_mgr_shutdown_resources_smoke", zhud_mgr_shutdown_resources_smoke},
        {"zhud_sensor_track_list_add_smoke", zhud_sensor_track_list_add_smoke},
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
        {"zclass_model_ref_lerp_queue_add_smoke", zclass_model_ref_lerp_queue_add_smoke},
        {"zclass_model_ref_lerp_queue_update_smoke", zclass_model_ref_lerp_queue_update_smoke},
        {"zclass_type_list_alloc_and_insert_smoke", zclass_type_list_alloc_and_insert_smoke},
        {"zclass_animate_update_smoke", zclass_animate_update_smoke},
        {"zclass_sequence_new_add_child_smoke", zclass_sequence_new_add_child_smoke},
        {"zclass_sequence_update_smoke", zclass_sequence_update_smoke},
        {"zclass_typelist_update_bucket_smoke", zclass_typelist_update_bucket_smoke},
        {"zclass_typelist_update_all_buckets_smoke",
         zclass_typelist_update_all_buckets_smoke},
        {"zclass_typelist_update_sequences_smoke", zclass_typelist_update_sequences_smoke},
        {"zclass_typelist_update_animations_smoke", zclass_typelist_update_animations_smoke},
        {"zclass_gwnode_update_all_smoke", zclass_gwnode_update_all_smoke},
        {"zclass_object3d_set_matrix_smoke", zclass_object3d_set_matrix_smoke},
        {"zclass_node_free_and_deferred_work_smoke", zclass_node_free_and_deferred_work_smoke},
        {"zclass_alloc_node_from_free_list_smoke", zclass_alloc_node_from_free_list_smoke},
        {"zclass_node_flag16_flag17_smoke", zclass_node_flag16_flag17_smoke},
        {"zclass_node_metadata_accessors_smoke", zclass_node_metadata_accessors_smoke},
        {"zclass_copy_node_display_instance_smoke", zclass_copy_node_display_instance_smoke},
        {"zclass_copy_node_base_data_smoke", zclass_copy_node_base_data_smoke},
        {"zclass_copy_node_unimplemented_stubs_smoke", zclass_copy_node_unimplemented_stubs_smoke},
        {"zclass_copy_camera_node_smoke", zclass_copy_camera_node_smoke},
        {"zclass_copy_object3d_and_lod_smoke", zclass_copy_object3d_and_lod_smoke},
        {"zclass_copy_node_dispatch_and_wrappers_smoke",
         zclass_copy_node_dispatch_and_wrappers_smoke},
        {"zclass_node_action_callback_smoke", zclass_node_action_callback_smoke},
        {"zclass_node_priority_smoke", zclass_node_priority_smoke},
        {"zclass_node_pick_flag_accessors_smoke", zclass_node_pick_flag_accessors_smoke},
        {"zclass_node_extra_flag_setters_smoke", zclass_node_extra_flag_setters_smoke},
        {"zclass_node_vertex_alpha_and_root_smoke", zclass_node_vertex_alpha_and_root_smoke},
        {"zclass_node_world_child_smoke", zclass_node_world_child_smoke},
        {"zclass_gwnode_update_tree_smoke", zclass_gwnode_update_tree_smoke},
        {"zclass_gwnode_build_node_to_ancestor_matrix_smoke",
         zclass_gwnode_build_node_to_ancestor_matrix_smoke},
        {"zclass_gwnode_get_world_position_smoke", zclass_gwnode_get_world_position_smoke},
        {"zclass_world_set_origin_smoke", zclass_world_set_origin_smoke},
        {"zclass_world_set_size_smoke", zclass_world_set_size_smoke},
        {"zclass_world_partition_tolerance_smoke", zclass_world_partition_tolerance_smoke},
        {"zclass_world_max_dec_features_smoke", zclass_world_max_dec_features_smoke},
        {"zclass_world_virtual_partition_statics_smoke",
         zclass_world_virtual_partition_statics_smoke},
        {"zclass_world_add_child_at_grid_smoke", zclass_world_add_child_at_grid_smoke},
        {"zclass_world_remove_light_sound_smoke", zclass_world_remove_light_sound_smoke},
        {"zclass_child_generic_link_smoke", zclass_child_generic_link_smoke},
        {"zclass_child_generic_remove_smoke", zclass_child_generic_remove_smoke},
        {"zclass_remove_wrapper_matrix_smoke", zclass_remove_wrapper_matrix_smoke},
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
        {"gamez_read_retail_m1_zbd_smoke", gamez_read_retail_m1_zbd_smoke},
        {"gamez_reload_display_instances_smoke", gamez_reload_display_instances_smoke},
        {"zclass_try_free_node_smoke", zclass_try_free_node_smoke},
        {"zdi_ref_and_pool_free_smoke", zdi_ref_and_pool_free_smoke},
        {"zdi_reset_current_variant_smoke", zdi_reset_current_variant_smoke},
        {"zdi_set_current_variant_smoke", zdi_set_current_variant_smoke},
        {"zdi_entry_material_helpers_smoke", zdi_entry_material_helpers_smoke},
        {"zmodel_set_di_texture_world_per_meter_smoke",
         zmodel_set_di_texture_world_per_meter_smoke},
        {"zclass_set_display_instance_smoke", zclass_set_display_instance_smoke},
        {"zclass_remove_dispatch_smoke", zclass_remove_dispatch_smoke},
        {"zclass_destroy_node_recursive_display_smoke",
         zclass_destroy_node_recursive_display_smoke},
        {"zclass_find_node_recursive_by_name_smoke", zclass_find_node_recursive_by_name_smoke},
        {"zclass_object3d_delete_node_smoke", zclass_object3d_delete_node_smoke},
        {"zclass_world_new_smoke", zclass_world_new_smoke},
        {"zclass_world_free_virtual_area_partitions_smoke",
         zclass_world_free_virtual_area_partitions_smoke},
        {"zclass_world_set_virtual_area_partition_smoke",
         zclass_world_set_virtual_area_partition_smoke},
        {"zclass_world_get_area_partition_at_grid_smoke",
         zclass_world_get_area_partition_at_grid_smoke},
        {"zclass_world_to_grid_coords_clamped_smoke",
         zclass_world_to_grid_coords_clamped_smoke},
        {"zclass_camera_convex_hull_xz_smoke", zclass_camera_convex_hull_xz_smoke},
        {"zclass_cls_di_region_filter_mesh_faces_smoke",
         zclass_cls_di_region_filter_mesh_faces_smoke},
        {"zclass_camera_build_frustum_grid_tiles_from_params_smoke",
         zclass_camera_build_frustum_grid_tiles_from_params_smoke},
        {"zclass_camera_build_frustum_grid_tiles_smoke",
         zclass_camera_build_frustum_grid_tiles_smoke},
        {"zclass_render_traverse_dispatch_smoke", zclass_render_traverse_dispatch_smoke},
        {"zclass_world_animate_delete_node_smoke", zclass_world_animate_delete_node_smoke},
        {"zclass_sound_leaf_smoke", zclass_sound_leaf_smoke},
        {"zclass_object3d_init_smoke", zclass_object3d_init_smoke},
        {"zclass_window_new_smoke", zclass_window_new_smoke},
        {"zclass_display_init_smoke", zclass_display_init_smoke},
        {"zclass_lod_leaf_smoke", zclass_lod_leaf_smoke},
        {"zclass_light_new_smoke", zclass_light_new_smoke},
        {"zclass_damage_handler_smoke", zclass_damage_handler_smoke},
        {"zclass_camera_view_distance_smoke", zclass_camera_view_distance_smoke},
        {"zclass_camera_sync_view_context_positions_smoke",
         zclass_camera_sync_view_context_positions_smoke},
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
        {"znetwork_local_identity_smoke", znetwork_local_identity_smoke},
        {"znetwork_destroy_cached_local_player_smoke", znetwork_destroy_cached_local_player_smoke},
        {"znetwork_dplay_report_error_smoke", znetwork_dplay_report_error_smoke},
        {"znetwork_dplay_close_release_smoke", znetwork_dplay_close_release_smoke},
        {"znetwork_dispatch_handler_list_smoke", znetwork_dispatch_handler_list_smoke},
        {"znetwork_register_packet_handler_smoke", znetwork_register_packet_handler_smoke},
        {"znetwork_dispatch_packet_to_handlers_smoke", znetwork_dispatch_packet_to_handlers_smoke},
        {"znetwork_player_record_accessors_smoke", znetwork_player_record_accessors_smoke},
        {"znetwork_remove_player_record_by_key_smoke", znetwork_remove_player_record_by_key_smoke},
        {"znetwork_alloc_free_player_color_index_smoke",
         znetwork_alloc_free_player_color_index_smoke},
        {"znetwork_host_send_player_color_assignments_packet_smoke",
         znetwork_host_send_player_color_assignments_packet_smoke},
        {"znetwork_dplay_pump_incoming_messages_smoke",
         znetwork_dplay_pump_incoming_messages_smoke},
        {"znetwork_dplay_receive_pending_messages_smoke",
         znetwork_dplay_receive_pending_messages_smoke},
        {"znetwork_dplay_enum_players_smoke", znetwork_dplay_enum_players_smoke},
        {"znetwork_packet_send_wrappers_smoke", znetwork_packet_send_wrappers_smoke},
        {"pickup_send_pkt11_delta_smoke", pickup_send_pkt11_delta_smoke},
        {"pickup_send_pkt11_create_delta_smoke", pickup_send_pkt11_create_delta_smoke},
        {"pickup_reconcile_spawn_lists_smoke", pickup_reconcile_spawn_lists_smoke},
        {"pickup_send_pkt12_airdrop_spawn_chute_relay_smoke",
         pickup_send_pkt12_airdrop_spawn_chute_relay_smoke},
        {"znetwork_session_status_fields_smoke", znetwork_session_status_fields_smoke},
        {"znetwork_shutdown_session_runtime_smoke", znetwork_shutdown_session_runtime_smoke},
        {"zreader_named_int_lookup_smoke", zreader_named_int_lookup_smoke},
        {"zreader_get_named_node_smoke", zreader_get_named_node_smoke},
        {"zreader_named_string_float_lookup_smoke", zreader_named_string_float_lookup_smoke},
        {"zreader_global_string_prefix_index_smoke", zreader_global_string_prefix_index_smoke},
        {"zrndr_global_string_table_load_dynamic_entries_smoke",
         zrndr_global_string_table_load_dynamic_entries_smoke},
        {"zreader_load_node_from_archive_smoke", zreader_load_node_from_archive_smoke},
        {"zreader_file_exists_and_list_create_smoke", zreader_file_exists_and_list_create_smoke},
        {"zreader_archive_list_and_search_paths_smoke",
         zreader_archive_list_and_search_paths_smoke},
        {"zreader_zrdr_free_search_path_list_smoke",
         zreader_zrdr_free_search_path_list_smoke},
        {"zreader_prealloc_and_pop_front_smoke", zreader_prealloc_and_pop_front_smoke},
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
        {"recoil_app_get_message_map_smoke", recoil_app_get_message_map_smoke},
        {"recoil_app_accessor_and_skip_wait_smoke", recoil_app_accessor_and_skip_wait_smoke},
        {"recoil_app_activate_existing_instance_absent_smoke",
         recoil_app_activate_existing_instance_absent_smoke},
        {"recoil_app_pre_translate_message_smoke", recoil_app_pre_translate_message_smoke},
        {"recoil_app_init_std_log_files_smoke", recoil_app_init_std_log_files_smoke},
        {"recoil_app_fatal_error_and_exit_smoke", recoil_app_fatal_error_and_exit_smoke},
        {"crt_atexit_import_provider_smoke", crt_atexit_import_provider_smoke},
        {"recoil_app_get_current_state_smoke", recoil_app_get_current_state_smoke},
        {"recoil_app_state_queue_block_init_from_cursor_smoke",
         recoil_app_state_queue_block_init_from_cursor_smoke},
        {"recoil_app_state_queue_grow_chunk_base_list_smoke",
         recoil_app_state_queue_grow_chunk_base_list_smoke},
        {"recoil_app_queue_switch_current_state_smoke",
         recoil_app_queue_switch_current_state_smoke},
        {"hud_sensor_queue_mission_fmv_state_for_mission_id_smoke",
         hud_sensor_queue_mission_fmv_state_for_mission_id_smoke},
        {"hud_sensor_save_and_queue_mission_state_smoke",
         hud_sensor_save_and_queue_mission_state_smoke},
        {"recoil_app_queue_push_state_smoke", recoil_app_queue_push_state_smoke},
        {"hud_ui_callback_queue_cheat_code_state_smoke",
         hud_ui_callback_queue_cheat_code_state_smoke},
        {"recoil_app_queue_exit_current_state_smoke", recoil_app_queue_exit_current_state_smoke},
        {"recoil_app_run_current_state_quit_smoke", recoil_app_run_current_state_quit_smoke},
        {"recoil_app_run_queue_transitions_smoke", recoil_app_run_queue_transitions_smoke},
        {"hud_ui_save_game_primary_action_button_on_activate_smoke",
         hud_ui_save_game_primary_action_button_on_activate_smoke},
        {"hud_ui_load_game_dialog_on_primary_action_smoke",
         hud_ui_load_game_dialog_on_primary_action_smoke},
        {"hud_ui_load_game_dialog_process_dialog_result_smoke",
         hud_ui_load_game_dialog_process_dialog_result_smoke},
        {"hud_ui_load_game_primary_action_button_on_activate_smoke",
         hud_ui_load_game_primary_action_button_on_activate_smoke},
        {"hud_ui_save_load_process_dialog_result_smoke",
         hud_ui_save_load_process_dialog_result_smoke},
        {"hud_ui_save_game_dialog_init_layout_smoke",
         hud_ui_save_game_dialog_init_layout_smoke},
        {"hud_ui_load_game_dialog_constructor_smoke",
         hud_ui_load_game_dialog_constructor_smoke},
        {"hud_ui_load_game_dialog_destructor_smoke",
         hud_ui_load_game_dialog_destructor_smoke},
        {"hud_ui_load_game_dialog_scalar_deleting_destructor_smoke",
         hud_ui_load_game_dialog_scalar_deleting_destructor_smoke},
        {"recoil_state_save_load_transition_on_try_become_current_smoke",
         recoil_state_save_load_transition_on_try_become_current_smoke},
        {"recoil_state_save_load_transition_on_update_should_quit_smoke",
         recoil_state_save_load_transition_on_update_should_quit_smoke},
        {"recoil_state_save_load_transition_lifecycle_smoke",
         recoil_state_save_load_transition_lifecycle_smoke},
        {"hud_ui_main_menu_dialog_save_load_checks_smoke",
         hud_ui_main_menu_dialog_save_load_checks_smoke},
        {"recoil_state_save_load_transition_queue_dialogs_smoke",
         recoil_state_save_load_transition_queue_dialogs_smoke},
        {"recoil_app_start_engine_and_queue_startup_state_smoke",
         recoil_app_start_engine_and_queue_startup_state_smoke},
        {"recoil_app_load_zbd_and_start_engine_smoke",
         recoil_app_load_zbd_and_start_engine_smoke},
        {"recoil_app_load_zbd_and_setup_sensor_tracker_smoke",
         recoil_app_load_zbd_and_setup_sensor_tracker_smoke},
        {"recoil_app_initialize_display_failure_smoke",
         recoil_app_initialize_display_failure_smoke},
        {"recoil_app_on_idle_or_dispatch_smoke", recoil_app_on_idle_or_dispatch_smoke},
        {"recoil_app_mfc_ole_module_constructor_smoke",
         recoil_app_mfc_ole_module_constructor_smoke},
        {"recoil_app_mfc_ole_module_destructor_smoke", recoil_app_mfc_ole_module_destructor_smoke},
        {"recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke",
         recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke},
        {"recoil_app_play_state_constructor_smoke", recoil_app_play_state_constructor_smoke},
        {"recoil_app_play_state_on_wnd_activate_smoke",
         recoil_app_play_state_on_wnd_activate_smoke},
        {"recoil_app_play_state_tick_and_render_frame_quit_smoke",
         recoil_app_play_state_tick_and_render_frame_quit_smoke},
        {"recoil_app_play_state_on_update_should_quit_transition_smoke",
         recoil_app_play_state_on_update_should_quit_transition_smoke},
        {"recoil_app_play_state_on_resume_cd_disabled_smoke",
         recoil_app_play_state_on_resume_cd_disabled_smoke},
        {"recoil_app_play_state_on_deactivate_skip_gameplay_smoke",
         recoil_app_play_state_on_deactivate_skip_gameplay_smoke},
        {"recoil_app_fmv_state_constructor_smoke", recoil_app_fmv_state_constructor_smoke},
        {"recoil_app_fmv_state_on_idle_or_dispatch_smoke",
         recoil_app_fmv_state_on_idle_or_dispatch_smoke},
        {"recoil_app_intro_fmv_on_try_become_current_smoke",
         recoil_app_intro_fmv_on_try_become_current_smoke},
        {"recoil_app_intro_fmv_on_update_should_quit_smoke",
         recoil_app_intro_fmv_on_update_should_quit_smoke},
        {"recoil_app_intro_fmv_on_deactivate_smoke",
         recoil_app_intro_fmv_on_deactivate_smoke},
        {"recoil_app_main_menu_prep_on_try_become_current_smoke",
         recoil_app_main_menu_prep_on_try_become_current_smoke},
        {"recoil_app_main_menu_prep_on_update_should_quit_smoke",
         recoil_app_main_menu_prep_on_update_should_quit_smoke},
        {"recoil_app_attract_fmv_on_try_become_current_smoke",
         recoil_app_attract_fmv_on_try_become_current_smoke},
        {"recoil_app_attract_fmv_on_update_should_quit_smoke",
         recoil_app_attract_fmv_on_update_should_quit_smoke},
        {"recoil_app_attract_fmv_on_deactivate_smoke",
         recoil_app_attract_fmv_on_deactivate_smoke},
        {"recoil_app_constructor_destructor_smoke", recoil_app_constructor_destructor_smoke},
        {"recoil_app_istate_destructor_smoke", recoil_app_istate_destructor_smoke},
        {"recoil_app_fmv_state_destructor_smoke", recoil_app_fmv_state_destructor_smoke},
        {"recoil_app_mission_fmv_state_destructor_smoke",
         recoil_app_mission_fmv_state_destructor_smoke},
        {"recoil_app_scalar_deleting_destructor_smoke",
         recoil_app_scalar_deleting_destructor_smoke},
        {"recoil_state_base_scalar_deleting_destructor_smoke",
         recoil_state_base_scalar_deleting_destructor_smoke},
        {"recoil_state_credits_destructor_smoke", recoil_state_credits_destructor_smoke},
        {"recoil_state_credits_constructor_smoke", recoil_state_credits_constructor_smoke},
        {"recoil_state_credits_on_wnd_activate_smoke",
         recoil_state_credits_on_wnd_activate_smoke},
        {"recoil_state_credits_on_try_become_current_smoke",
         recoil_state_credits_on_try_become_current_smoke},
        {"recoil_state_credits_on_deactivate_smoke",
         recoil_state_credits_on_deactivate_smoke},
        {"recoil_state_credits_queue_push_smoke", recoil_state_credits_queue_push_smoke},
        {"recoil_state_confirm_quit_destructor_smoke",
         recoil_state_confirm_quit_destructor_smoke},
        {"recoil_state_confirm_quit_queue_enter_smoke",
         recoil_state_confirm_quit_queue_enter_smoke},
        {"hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke",
         hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke},
        {"hud_ui_credits_quit_button_on_activate_smoke",
         hud_ui_credits_quit_button_on_activate_smoke},
        {"hud_ui_cheat_text_input_on_activate_smoke",
         hud_ui_cheat_text_input_on_activate_smoke},
        {"hud_ui_callback_queue_exit_current_state_smoke",
         hud_ui_callback_queue_exit_current_state_smoke},
        {"hud_ui_background_confirm_quit_lifecycle_smoke",
         hud_ui_background_confirm_quit_lifecycle_smoke},
        {"recoil_state_confirm_quit_on_try_become_current_smoke",
         recoil_state_confirm_quit_on_try_become_current_smoke},
        {"recoil_state_confirm_quit_static_init_smoke",
         recoil_state_confirm_quit_static_init_smoke},
        {"recoil_state_cheat_code_constructor_smoke",
         recoil_state_cheat_code_constructor_smoke},
        {"recoil_state_cheat_code_destructor_smoke",
         recoil_state_cheat_code_destructor_smoke},
        {"recoil_state_cheat_code_scalar_deleting_destructor_smoke",
         recoil_state_cheat_code_scalar_deleting_destructor_smoke},
        {"recoil_state_cheat_code_static_init_thunks_smoke",
         recoil_state_cheat_code_static_init_thunks_smoke},
        {"recoil_state_cheat_code_on_try_become_current_smoke",
         recoil_state_cheat_code_on_try_become_current_smoke},
        {"recoil_state_cheat_code_on_deactivate_smoke",
         recoil_state_cheat_code_on_deactivate_smoke},
        {"hud_ui_confirm_quit_ok_button_on_activate_smoke",
         hud_ui_confirm_quit_ok_button_on_activate_smoke},
        {"recoil_state_main_menu_transition_constructor_smoke",
         recoil_state_main_menu_transition_constructor_smoke},
        {"recoil_state_main_menu_transition_destructor_smoke",
         recoil_state_main_menu_transition_destructor_smoke},
        {"recoil_state_main_menu_transition_scalar_deleting_destructor_smoke",
         recoil_state_main_menu_transition_scalar_deleting_destructor_smoke},
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
        {"zsnd_backend_init_directsound_provider_smoke",
         zsnd_backend_init_directsound_provider_smoke},
        {"zsnd_backend_init_a3d_provider_smoke", zsnd_backend_init_a3d_provider_smoke},
        {"zsnd_cached_directsound_get_caps_smoke", zsnd_cached_directsound_get_caps_smoke},
        {"zsnd_options_cpu_and_cached_directsound_smoke",
         zsnd_options_cpu_and_cached_directsound_smoke},
        {"zsnd_cd_reset_track_state_smoke", zsnd_cd_reset_track_state_smoke},
        {"zsnd_cd_is_stereo_aux_enabled_smoke", zsnd_cd_is_stereo_aux_enabled_smoke},
        {"zsnd_cd_get_volume_smoke", zsnd_cd_get_volume_smoke},
        {"zsnd_cd_set_volume_smoke", zsnd_cd_set_volume_smoke},
        {"zsnd_cd_not_ready_playback_smoke", zsnd_cd_not_ready_playback_smoke},
        {"zsnd_cd_playback_mci_commands_smoke", zsnd_cd_playback_mci_commands_smoke},
        {"zsnd_cd_on_mci_notify_loop_smoke", zsnd_cd_on_mci_notify_loop_smoke},
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
        {"zsnd_global_volume_and_flag_helpers_smoke",
         zsnd_global_volume_and_flag_helpers_smoke},
        {"zsnd_sample_acquire_play_handle_smoke", zsnd_sample_acquire_play_handle_smoke},
        {"zsnd_system_named_sets_syntax_smoke", zsnd_system_named_sets_syntax_smoke},
        {"zsnd_system_legacy_sets_syntax_smoke", zsnd_system_legacy_sets_syntax_smoke},
        {"zsnd_system_init_gate_and_missing_config_smoke",
         zsnd_system_init_gate_and_missing_config_smoke},
        {"zsnd_preinitialize_runtime_state_smoke", zsnd_preinitialize_runtime_state_smoke},
        {"zsnd_group_load_config_block_smoke", zsnd_group_load_config_block_smoke},
        {"zsnd_group_load_and_queue_smoke", zsnd_group_load_and_queue_smoke},
        {"zsnd_create_queued_streaming_sample_smoke",
         zsnd_create_queued_streaming_sample_smoke},
        {"zsnd_stream_mgr_ensure_init_smoke", zsnd_stream_mgr_ensure_init_smoke},
        {"zsnd_stream_request_queue_smoke", zsnd_stream_request_queue_smoke},
        {"zsnd_stream_mgr_shutdown_lists_smoke", zsnd_stream_mgr_shutdown_lists_smoke},
        {"zsnd_backend_shutdown_release_smoke", zsnd_backend_shutdown_release_smoke},
        {"zsnd_play_handle_stop_if_active_smoke", zsnd_play_handle_stop_if_active_smoke},
        {"zsnd_sample_stop_active_voices_if_playing_smoke",
         zsnd_sample_stop_active_voices_if_playing_smoke},
        {"zsnd_snapshot_create_from_active_samples_smoke",
         zsnd_snapshot_create_from_active_samples_smoke},
        {"zsnd_apply_mute_state_to_active_voices_smoke",
         zsnd_apply_mute_state_to_active_voices_smoke},
        {"zsnd_snapshot_restore_all_with_global_volume_delta_smoke",
         zsnd_snapshot_restore_all_with_global_volume_delta_smoke},
        {"zsnd_snapshot_destroy_smoke", zsnd_snapshot_destroy_smoke},
        {"zsnd_snapshot_stop_all_if_playing_smoke", zsnd_snapshot_stop_all_if_playing_smoke},
        {"zsnd_play_handle_update3d_a3d_smoke", zsnd_play_handle_update3d_a3d_smoke},
        {"zsnd_update_listener_state_smoke", zsnd_update_listener_state_smoke},
        {"zsnd_play_handle_update3d_directsound_smoke",
         zsnd_play_handle_update3d_directsound_smoke},
        {"zsnd_play_handle_set_freq_scaled_smoke", zsnd_play_handle_set_freq_scaled_smoke},
        {"zsnd_play_handle_set_enable_scale_smoke", zsnd_play_handle_set_enable_scale_smoke},
        {"zsnd_play_handle_try_disable_managed_smoke",
         zsnd_play_handle_try_disable_managed_smoke},
        {"zsnd_sample_play_a3d_simple_direct_smoke", zsnd_sample_play_a3d_simple_direct_smoke},
        {"zsnd_sample_play_a3d_worldpos_smoke", zsnd_sample_play_a3d_worldpos_smoke},
        {"zsnd_sample_destroy_owned_data_smoke", zsnd_sample_destroy_owned_data_smoke},
        {"zsnd_sample_destroy_smoke", zsnd_sample_destroy_smoke},
        {"zsnd_sample_set_playback_event_handler_smoke",
         zsnd_sample_set_playback_event_handler_smoke},
        {"zsnd_fade_entry_backend_and_dispatch_smoke", zsnd_fade_entry_backend_and_dispatch_smoke},
        {"zsnd_fade_active_list_tick_compacts_smoke", zsnd_fade_active_list_tick_compacts_smoke},
        {"zsnd_fade_list_cursor_helpers_smoke", zsnd_fade_list_cursor_helpers_smoke},
        {"zsnd_fade_lists_stop_all_shutdown_smoke", zsnd_fade_lists_stop_all_shutdown_smoke},
        {"zsnd_tick_backend_markers_smoke", zsnd_tick_backend_markers_smoke},
        {"zsnd_sample_set_init_by_name_empty_smoke", zsnd_sample_set_init_by_name_empty_smoke},
        {"zsnd_sample_set_init_loose_file_smoke", zsnd_sample_set_init_loose_file_smoke},
        {"zsnd_wave_data_load_parse_reset_smoke", zsnd_wave_data_load_parse_reset_smoke},
        {"zsnd_wave_data_parse_chunks_smoke", zsnd_wave_data_parse_chunks_smoke},
        {"zsnd_wave_data_load_parse_edges_smoke", zsnd_wave_data_load_parse_edges_smoke},
        {"zsnd_wave_data_archive_load_smoke", zsnd_wave_data_archive_load_smoke},
        {"zsnd_sample_set_load_samples_from_index_archive_smoke",
         zsnd_sample_set_load_samples_from_index_archive_smoke},
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
        {"zutil_zar_register_section_handler_smoke", zutil_zar_register_section_handler_smoke},
        {"zutil_zbd_init_smoke", zutil_zbd_init_smoke},
        {"zutil_zbd_destroy_global_manager_smoke", zutil_zbd_destroy_global_manager_smoke},
        {"zutil_zbd_section_handler_compare_sort_order_smoke",
         zutil_zbd_section_handler_compare_sort_order_smoke},
        {"zutil_zbd_section_handler_invoke_pre_load_smoke",
         zutil_zbd_section_handler_invoke_pre_load_smoke},
        {"zutil_zbd_section_handler_invoke_data_ready_smoke",
         zutil_zbd_section_handler_invoke_data_ready_smoke},
        {"zutil_zbd_manager_sort_section_handlers_smoke",
         zutil_zbd_manager_sort_section_handlers_smoke},
        {"zutil_zbd_manager_load_entries_smoke", zutil_zbd_manager_load_entries_smoke},
        {"zutil_zbd_load_entries_global_smoke", zutil_zbd_load_entries_global_smoke},
        {"zutil_zbd_manager_load_zar_file_smoke", zutil_zbd_manager_load_zar_file_smoke},
        {"zutil_zar_load_file_global_smoke", zutil_zar_load_file_global_smoke},
        {"hud_sensor_register_mission_sections_smoke", hud_sensor_register_mission_sections_smoke},
        {"hud_sensor_mission_identity_smoke", hud_sensor_mission_identity_smoke},
        {"hud_sensor_map_node_basics_smoke", hud_sensor_map_node_basics_smoke},
        {"hud_sensor_objective_marker_enable_color_smoke",
         hud_sensor_objective_marker_enable_color_smoke},
        {"hud_sensor_find_first_incomplete_objective_smoke",
         hud_sensor_find_first_incomplete_objective_smoke},
        {"hud_sensor_map_bounds_and_save_state_smoke", hud_sensor_map_bounds_and_save_state_smoke},
        {"hud_sensor_map_remove_and_shutdown_smoke", hud_sensor_map_remove_and_shutdown_smoke},
        {"hud_sensor_map_overlay_toggle_smoke", hud_sensor_map_overlay_toggle_smoke},
        {"hud_sensor_tracker_load_map_paths_smoke", hud_sensor_tracker_load_map_paths_smoke},
        {"hud_sensor_reset_mission_state_smoke", hud_sensor_reset_mission_state_smoke},
        {"hud_sensor_shutdown_mission_gameplay_systems_early_smoke",
         hud_sensor_shutdown_mission_gameplay_systems_early_smoke},
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
        {"zeffect_handle_light_anim_event_smoke", zeffect_handle_light_anim_event_smoke},
        {"zeffect_handle_fog_event_smoke", zeffect_handle_fog_event_smoke},
        {"zeffect_handle_camera_params_event_smoke", zeffect_handle_camera_params_event_smoke},
        {"zeffect_animate_camera_params_over_time_smoke",
         zeffect_animate_camera_params_over_time_smoke},
        {"zeffect_handle_rotation_event_smoke", zeffect_handle_rotation_event_smoke},
        {"zeffect_handle_position_event_smoke", zeffect_handle_position_event_smoke},
        {"zeffect_handle_activate_event_smoke", zeffect_handle_activate_event_smoke},
        {"zeffect_handle_node_anim_event_smoke", zeffect_handle_node_anim_event_smoke},
        {"zeffect_animate_node_over_time_smoke", zeffect_animate_node_over_time_smoke},
        {"zeffect_handle_node_scale_event_smoke", zeffect_handle_node_scale_event_smoke},
        {"zeffect_handle_screen_fx_events_smoke", zeffect_handle_screen_fx_events_smoke},
        {"zeffect_anim_keyframe_sample_smoke", zeffect_anim_keyframe_sample_smoke},
        {"zeffect_parent_attach_detach_events_smoke",
         zeffect_parent_attach_detach_events_smoke},
        {"zeffect_anim_activation_prereqs_smoke", zeffect_anim_activation_prereqs_smoke},
        {"hud_sensor_run_start_anims_from_zrd_smoke",
         hud_sensor_run_start_anims_from_zrd_smoke},
        {"zeffect_anim_activation_record_queue_smoke", zeffect_anim_activation_record_queue_smoke},
        {"zeffect_anim_process_activation_record_smoke",
         zeffect_anim_process_activation_record_smoke},
        {"zeffect_handle_emitter_reset_event_smoke", zeffect_handle_emitter_reset_event_smoke},
        {"zeffect_handle_emitter_loop_event_smoke", zeffect_handle_emitter_loop_event_smoke},
        {"zeffect_handle_emitter_stop_event_smoke", zeffect_handle_emitter_stop_event_smoke},
        {"zeffect_surface_control_events_smoke", zeffect_surface_control_events_smoke},
        {"zeffect_conditional_ref_pos_smoke", zeffect_conditional_ref_pos_smoke},
        {"zeffect_skip_conditional_chain_smoke", zeffect_skip_conditional_chain_smoke},
        {"zeffect_trace_upward_hit_smoke", zeffect_trace_upward_hit_smoke},
        {"zeffect_set_variant_override_packed_ids_if_complete_smoke",
         zeffect_set_variant_override_packed_ids_if_complete_smoke},
        {"zeffect_handle_conditional_chain_event_smoke",
         zeffect_handle_conditional_chain_event_smoke},
        {"zeffect_marker_and_callback_event_smoke", zeffect_marker_and_callback_event_smoke},
        {"zeffect_handle_top_message_event_smoke", zeffect_handle_top_message_event_smoke},
        {"zeffect_anim_set_on_state_done_callback_smoke",
         zeffect_anim_set_on_state_done_callback_smoke},
        {"zeffect_world_and_zbd_setters_smoke", zeffect_world_and_zbd_setters_smoke},
        {"zeffect_anim_load_zbd_minimal_smoke", zeffect_anim_load_zbd_minimal_smoke},
        {"zeffect_anim_load_and_instantiate_minimal_smoke",
         zeffect_anim_load_and_instantiate_minimal_smoke},
        {"zeffect_tick_reset_delay_callbacks_smoke",
         zeffect_tick_reset_delay_callbacks_smoke},
        {"zeffect_anim_debug_frame_tag_smoke", zeffect_anim_debug_frame_tag_smoke},
        {"zeffect_anim_find_next_async_entry_smoke", zeffect_anim_find_next_async_entry_smoke},
        {"zeffect_anim_get_root_node_or_null_smoke", zeffect_anim_get_root_node_or_null_smoke},
        {"zeffect_anim_capture_node_states_smoke", zeffect_anim_capture_node_states_smoke},
        {"zeffect_anim_restore_node_states_smoke", zeffect_anim_restore_node_states_smoke},
        {"zeffect_anim_reset_for_node_smoke", zeffect_anim_reset_for_node_smoke},
        {"zeffect_anim_runtime_sequence_group_smoke",
         zeffect_anim_runtime_sequence_group_smoke},
        {"zeffect_anim_init_shutdown_smoke", zeffect_anim_init_shutdown_smoke},
        {"zeffect_shutdown_all_smoke", zeffect_shutdown_all_smoke},
        {"zeffect_find_template_index_by_name_smoke", zeffect_find_template_index_by_name_smoke},
        {"zeffect_find_node_user_data_recursive_smoke",
         zeffect_find_node_user_data_recursive_smoke},
        {"zeffect_clone_runtime_entry_from_template_smoke",
         zeffect_clone_runtime_entry_from_template_smoke},
        {"zeffect_acquire_runtime_entry_by_index_smoke",
         zeffect_acquire_runtime_entry_by_index_smoke},
        {"zeffect_compute_distance_sq_to_listener_smoke",
         zeffect_compute_distance_sq_to_listener_smoke},
        {"zeffect_activate_runtime_entry_at_position_smoke",
         zeffect_activate_runtime_entry_at_position_smoke},
        {"zeffect_runtime_node_action_callback_smoke",
         zeffect_runtime_node_action_callback_smoke},
        {"zeffect_spawn_runtime_instance_at_smoke", zeffect_spawn_runtime_instance_at_smoke},
        {"zeffect_init_smoke", zeffect_init_smoke},
        {"zeffect_init_from_path_smoke", zeffect_init_from_path_smoke},
        {"zdeclient_set_camera_node_smoke", zdeclient_set_camera_node_smoke},
        {"zdeclient_load_material_from_texture_path_smoke",
         zdeclient_load_material_from_texture_path_smoke},
        {"zdeclient_load_config_resources_smoke", zdeclient_load_config_resources_smoke},
        {"zdeclient_crater_init_event_template_defaults_smoke",
         zdeclient_crater_init_event_template_defaults_smoke},
        {"zdeclient_qsand_copy_event_template_defaults_smoke",
         zdeclient_qsand_copy_event_template_defaults_smoke},
        {"zdeclient_write_feature_sections_to_zar_smoke",
         zdeclient_write_feature_sections_to_zar_smoke},
        {"zdeclient_shutdown_globals_smoke", zdeclient_shutdown_globals_smoke},
        {"zdeclient_map_tree_iter_next_node_ref_smoke",
         zdeclient_map_tree_iter_next_node_ref_smoke},
        {"zdeclient_map_tree_iter_prev_node_ref_smoke",
         zdeclient_map_tree_iter_prev_node_ref_smoke},
        {"zdeclient_map_tree_insert_at_smoke", zdeclient_map_tree_insert_at_smoke},
        {"zdeclient_map_tree_find_or_insert_key_smoke",
         zdeclient_map_tree_find_or_insert_key_smoke},
        {"zdeclient_submit_feature_geometry_smoke", zdeclient_submit_feature_geometry_smoke},
        {"zdeclient_feature_entry_copy_fill_helpers_smoke",
         zdeclient_feature_entry_copy_fill_helpers_smoke},
        {"zdeclient_append_feature_entry_smoke", zdeclient_append_feature_entry_smoke},
        {"zdeclient_clear_feature_display_nodes_smoke",
         zdeclient_clear_feature_display_nodes_smoke},
        {"zdeclient_feature_leaf_helpers_smoke", zdeclient_feature_leaf_helpers_smoke},
        {"zdeclient_feature_init_helpers_smoke", zdeclient_feature_init_helpers_smoke},
        {"zdeclient_create_feature_node_from_partition_smoke",
         zdeclient_create_feature_node_from_partition_smoke},
        {"zdeclient_qsand_create_feature_smoke", zdeclient_qsand_create_feature_smoke},
        {"zdeclient_crater_build_and_create_feature_smoke",
         zdeclient_crater_build_and_create_feature_smoke},
        {"zdeclient_crater_instance_event_smoke", zdeclient_crater_instance_event_smoke},
        {"zdeclient_crater_instance_event_maybe_relay_smoke",
         zdeclient_crater_instance_event_maybe_relay_smoke},
        {"zdeclient_crater_net_relay_callback_smoke",
         zdeclient_crater_net_relay_callback_smoke},
        {"zdeclient_crater_execute_smoke", zdeclient_crater_execute_smoke},
        {"zdeclient_qsand_net_relay_callback_smoke",
         zdeclient_qsand_net_relay_callback_smoke},
        {"zdeclient_apply_feature_entry_reload_smoke",
         zdeclient_apply_feature_entry_reload_smoke},
        {"zdeclient_dispatch_feature_event_templates_smoke",
         zdeclient_dispatch_feature_event_templates_smoke},
        {"zdeclient_qsand_instance_event_maybe_relay_smoke",
         zdeclient_qsand_instance_event_maybe_relay_smoke},
        {"zgeometry_clip_patch_output_apply_node_di_pairs_smoke",
         zgeometry_clip_patch_output_apply_node_di_pairs_smoke},
        {"zloc_message_lookup_failure_smoke", zloc_message_lookup_failure_smoke},
        {"zloc_load_unload_messages_dll_smoke", zloc_load_unload_messages_dll_smoke},
        {"directdraw_enumerate_import_provider_smoke",
         directdraw_enumerate_import_provider_smoke},
        {"zvid_pack_color_rgb_smoke", zvid_pack_color_rgb_smoke},
        {"zvideo_draw_noise_rect_smoke", zvideo_draw_noise_rect_smoke},
        {"zvideo_pixel_pack_setup_smoke", zvideo_pixel_pack_setup_smoke},
        {"zvideo_pixel_pack_getters_smoke", zvideo_pixel_pack_getters_smoke},
        {"zvideo_set_renderer_type_smoke", zvideo_set_renderer_type_smoke},
        {"zvideo_pending_dither_enable_smoke", zvideo_pending_dither_enable_smoke},
        {"zvideo_pending_wireframe_state_smoke", zvideo_pending_wireframe_state_smoke},
        {"zvideo_dd3d_begin_scene_flush_pending_smoke",
         zvideo_dd3d_begin_scene_flush_pending_smoke},
        {"zvideo_sw_render_frame_smoke", zvideo_sw_render_frame_smoke},
        {"zclass_camera_render_scene_smoke", zclass_camera_render_scene_smoke},
        {"zclass_list_render_active_cameras_smoke", zclass_list_render_active_cameras_smoke},
        {"zvideo_surface_accessors_smoke", zvideo_surface_accessors_smoke},
        {"zvideo_fxpass3_local_queue_smoke", zvideo_fxpass3_local_queue_smoke},
        {"zvideo_primary_surface_rect_scratch_smoke", zvideo_primary_surface_rect_scratch_smoke},
        {"zvideo_run_postprocess_on_sw_buffer_smoke",
         zvideo_run_postprocess_on_sw_buffer_smoke},
        {"zvideo_run_postprocess_on_primary_buffer_smoke",
         zvideo_run_postprocess_on_primary_buffer_smoke},
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
        {"zvideo_handle_software_mode_hotkey_smoke",
         zvideo_handle_software_mode_hotkey_smoke},
        {"zvideo_dd_report_error_smoke", zvideo_dd_report_error_smoke},
        {"zvideo_dd_run_device_enumeration_smoke", zvideo_dd_run_device_enumeration_smoke},
        {"zvideo_dd_startup_enumerate_default_select_smoke",
         zvideo_dd_startup_enumerate_default_select_smoke},
        {"zvideo_module_init_smoke", zvideo_module_init_smoke},
        {"zvid_hw_api_accessors_smoke", zvid_hw_api_accessors_smoke},
        {"zvid_cached_renderer_and_texture_counts_smoke",
         zvid_cached_renderer_and_texture_counts_smoke},
        {"zvid_texture_pack_load_state_getter_smoke",
         zvid_texture_pack_load_state_getter_smoke},
        {"zvid_texture_pack_load_state_setter_smoke",
         zvid_texture_pack_load_state_setter_smoke},
        {"zvid_option_accessors_smoke", zvid_option_accessors_smoke},
        {"zvid_set_video_mode_index_smoke", zvid_set_video_mode_index_smoke},
        {"zvideo_buff_clip_coord_to_range_smoke", zvideo_buff_clip_coord_to_range_smoke},
        {"zvideo_buff_copy_surface_rect_to_image_smoke",
         zvideo_buff_copy_surface_rect_to_image_smoke},
        {"zvideo_buff_blt_source_to_primary_clipped_smoke",
         zvideo_buff_blt_source_to_primary_clipped_smoke},
        {"zvideo_surface_state_lock_skip_smoke", zvideo_surface_state_lock_skip_smoke},
        {"zvideo_dd_unlock_directdraw_surface_smoke",
         zvideo_dd_unlock_directdraw_surface_smoke},
        {"zvideo_dd_lock_directdraw_surface_smoke", zvideo_dd_lock_directdraw_surface_smoke},
        {"zvideo_dd_lock_surface_state_smoke", zvideo_dd_lock_surface_state_smoke},
        {"zvideo_dd_unlock_surface_state_smoke", zvideo_dd_unlock_surface_state_smoke},
        {"zvideo_capture_surface_to_image_smoke", zvideo_capture_surface_to_image_smoke},
        {"zvideo_image_lazy_create_backing_surface_guards_smoke",
         zvideo_image_lazy_create_backing_surface_guards_smoke},
        {"zvideo_blt_sw_to_primary_rect_lazy_failure_smoke",
         zvideo_blt_sw_to_primary_rect_lazy_failure_smoke},
        {"zvideo_flip_to_gdi_if_attached_null_smoke", zvideo_flip_to_gdi_if_attached_null_smoke},
        {"zvideo_clear_rect_skip_paths_smoke", zvideo_clear_rect_skip_paths_smoke},
        {"zvideo_palette_set_entries_non8bpp_smoke", zvideo_palette_set_entries_non8bpp_smoke},
        {"zvideo_apply_brightness_to_palette_entries_smoke",
         zvideo_apply_brightness_to_palette_entries_smoke},
        {"zvideo_load_palette_file_and_apply_brightness_smoke",
         zvideo_load_palette_file_and_apply_brightness_smoke},
        {"zvideo_quad_batch_depth_and_rhw_smoke", zvideo_quad_batch_depth_and_rhw_smoke},
        {"zvideo_set_active_view_context_smoke", zvideo_set_active_view_context_smoke},
        {"zvideo_frustum_test_sphere_clip_mask_smoke",
         zvideo_frustum_test_sphere_clip_mask_smoke},
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
        {"zvid_image_create_format_size_pixels_smoke",
         zvid_image_create_format_size_pixels_smoke},
        {"zvideo_image_file_read_helpers_smoke", zvideo_image_file_read_helpers_smoke},
        {"zvideo_palette_remap_no_recipes_smoke", zvideo_palette_remap_no_recipes_smoke},
        {"zvideo_palette_remap_recipe_variants_smoke",
         zvideo_palette_remap_recipe_variants_smoke},
        {"zvideo_texture_pack_load_image_smoke", zvideo_texture_pack_load_image_smoke},
        {"zimage_font_glyph_scan_smoke", zimage_font_glyph_scan_smoke},
        {"zimage_font_measure_string_smoke", zimage_font_measure_string_smoke},
        {"zimage_fonts_load_missing_smoke", zimage_fonts_load_missing_smoke},
        {"zimage_texdir_find_or_create_missing_smoke", zimage_texdir_find_or_create_missing_smoke},
        {"zimage_texdir_build_mip_chain_smoke", zimage_texdir_build_mip_chain_smoke},
        {"zvid_texture_pack_ensure_builtin_smoke", zvid_texture_pack_ensure_builtin_smoke},
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
        {"zimage_init_mission_resources_smoke", zimage_init_mission_resources_smoke},
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
        {"zweapon_optcatalog_load_kill_verb_string_smoke",
         zweapon_optcatalog_load_kill_verb_string_smoke},
        {"zweapon_load_opt_catalog_from_path_smoke", zweapon_load_opt_catalog_from_path_smoke},
        {"zweapon_zar_weapons_section_callbacks_smoke",
         zweapon_zar_weapons_section_callbacks_smoke},
        {"zweapon_optcatalog_shutdown_smoke", zweapon_optcatalog_shutdown_smoke},
        {"zweapon_set_max_tether_altitude_smoke", zweapon_set_max_tether_altitude_smoke},
        {"zweapon_optcatalog_blend_direction_toward_target_smoke",
         zweapon_optcatalog_blend_direction_toward_target_smoke},
        {"zweapon_optcatalog_find_entry_by_name_smoke",
         zweapon_optcatalog_find_entry_by_name_smoke},
        {"zweapon_optcatalog_find_entry_by_id_smoke", zweapon_optcatalog_find_entry_by_id_smoke},
        {"zweapon_optcatalog_create_trail_segment_node_smoke",
         zweapon_optcatalog_create_trail_segment_node_smoke},
        {"zweapon_optcatalog_create_trail_runtime_state_smoke",
         zweapon_optcatalog_create_trail_runtime_state_smoke},
        {"zweapon_optcatalog_mine_iterator_smoke", zweapon_optcatalog_mine_iterator_smoke},
        {"zweapon_optcatalog_pending_spawn_override_smoke",
         zweapon_optcatalog_pending_spawn_override_smoke},
        {"zweapon_optcatalog_load_fx_spec_from_reader_node_smoke",
         zweapon_optcatalog_load_fx_spec_from_reader_node_smoke},
        {"zweapon_optcatalog_invoke_damage_feedback_and_hit_callback_smoke",
         zweapon_optcatalog_invoke_damage_feedback_and_hit_callback_smoke},
        {"hitcontext_get_current_owner_smoke", hitcontext_get_current_owner_smoke},
        {"optcatalog_set_damage_context_smoke", optcatalog_set_damage_context_smoke},
        {"optcatalog_damage_feedback_leaf_helpers_smoke",
         optcatalog_damage_feedback_leaf_helpers_smoke},
        {"optcatalog_capture_hit_snapshot_and_invoke_damage_timer_callback_smoke",
         optcatalog_capture_hit_snapshot_and_invoke_damage_timer_callback_smoke},
        {"zweapon_optcatalog_runtime_free_list_helpers_smoke",
         zweapon_optcatalog_runtime_free_list_helpers_smoke},
        {"zweapon_optcatalog_build_impact_hit_list_smoke",
         zweapon_optcatalog_build_impact_hit_list_smoke},
        {"zweapon_optcatalog_handle_impact_from_runtime_probe_smoke",
         zweapon_optcatalog_handle_impact_from_runtime_probe_smoke},
        {"zweapon_optcatalog_process_runtime_instance_smoke",
         zweapon_optcatalog_process_runtime_instance_smoke},
        {"zweapon_optcatalog_process_runtime_instances_smoke",
         zweapon_optcatalog_process_runtime_instances_smoke},
        {"zweapon_optcatalog_remove_runtime_instance_smoke",
         zweapon_optcatalog_remove_runtime_instance_smoke},
        {"zweapon_optcatalog_can_spawn_through_ray_smoke",
         zweapon_optcatalog_can_spawn_through_ray_smoke},
        {"zweapon_optcatalog_compute_aim_pitch_for_target_smoke",
         zweapon_optcatalog_compute_aim_pitch_for_target_smoke},
        {"zweapon_optcatalog_reflect_and_sort_impact_trace_list_smoke",
         zweapon_optcatalog_reflect_and_sort_impact_trace_list_smoke},
        {"zweapon_optcatalog_compute_trail_impact_response_smoke",
         zweapon_optcatalog_compute_trail_impact_response_smoke},
        {"zweapon_optcatalog_update_trail_segment_visual_smoke",
         zweapon_optcatalog_update_trail_segment_visual_smoke},
        {"zweapon_optcatalog_emit_crater_impact_event_smoke",
         zweapon_optcatalog_emit_crater_impact_event_smoke},
        {"zweapon_optcatalog_emit_qsand_impact_event_smoke",
         zweapon_optcatalog_emit_qsand_impact_event_smoke},
        {"zweapon_optcatalog_play_impact_sound_smoke",
         zweapon_optcatalog_play_impact_sound_smoke},
        {"zweapon_optcatalog_play_bounce_sound_smoke",
         zweapon_optcatalog_play_bounce_sound_smoke},
        {"zweapon_optcatalog_handle_impact_event_smoke",
         zweapon_optcatalog_handle_impact_event_smoke},
        {"zweapon_optcatalog_handle_impact_event_from_runtime_state_smoke",
         zweapon_optcatalog_handle_impact_event_from_runtime_state_smoke},
        {"light_alloc_from_free_list_and_attach_smoke",
         light_alloc_from_free_list_and_attach_smoke},
        {"light_init_thermal_glow_pool_smoke", light_init_thermal_glow_pool_smoke},
        {"player_timed_hit_status_smoke", player_timed_hit_status_smoke},
        {"zweapon_optcatalog_warning_samples_smoke", zweapon_optcatalog_warning_samples_smoke},
        {"zweapon_optcatalog_activate_trail_runtime_state_smoke",
         zweapon_optcatalog_activate_trail_runtime_state_smoke},
        {"zweapon_optcatalog_deactivate_trail_runtime_state_smoke",
         zweapon_optcatalog_deactivate_trail_runtime_state_smoke},
        {"zvideo_restore_iconic_fullscreen_window_smoke",
         zvideo_restore_iconic_fullscreen_window_smoke},
        {"zvideo_shutdown_video_system_smoke", zvideo_shutdown_video_system_smoke},
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
        {"zcom_query_interface_from_interface_map_smoke",
         zcom_query_interface_from_interface_map_smoke},
        {"zcom_connection_point_container_advise_smoke",
         zcom_connection_point_container_advise_smoke},
        {"zcom_connection_point_container_unadvise_smoke",
         zcom_connection_point_container_unadvise_smoke},
        {"pickup_type_table_free_opt_meta_smoke", pickup_type_table_free_opt_meta_smoke},
        {"pickup_airdrop_spawn_ref_shutdown_global_smoke",
         pickup_airdrop_spawn_ref_shutdown_global_smoke},
        {"pickup_airdrop_spawn_ref_init_nodes_smoke", pickup_airdrop_spawn_ref_init_nodes_smoke},
        {"pickup_airdrop_spawn_ref_init_global_smoke", pickup_airdrop_spawn_ref_init_global_smoke},
        {"pickup_airdrop_spawn_ref_get_world_pos_smoke",
         pickup_airdrop_spawn_ref_get_world_pos_smoke},
        {"pickup_airdrop_spawn_ref_can_spawn_with_clearance_smoke",
         pickup_airdrop_spawn_ref_can_spawn_with_clearance_smoke},
        {"pickup_airdrop_spawn_ref_spawn_pickup_type_and_relay_gates_smoke",
         pickup_airdrop_spawn_ref_spawn_pickup_type_and_relay_gates_smoke},
        {"pickup_airdrop_spawn_ref_try_spawn_random_pickup_from_global_blocked_smoke",
         pickup_airdrop_spawn_ref_try_spawn_random_pickup_from_global_blocked_smoke},
        {"pickup_spawn_list_has_entry_near_xz_smoke", pickup_spawn_list_has_entry_near_xz_smoke},
        {"pickup_map_vtol_drop_group_variant_to_type_index_smoke",
         pickup_map_vtol_drop_group_variant_to_type_index_smoke},
        {"pickup_select_next_vtol_spawn_type_index_smoke",
         pickup_select_next_vtol_spawn_type_index_smoke},
        {"pickup_select_puppies_zrd_by_difficulty_smoke",
         pickup_select_puppies_zrd_by_difficulty_smoke},
        {"net_is_opt_entry_active_in_any_slot_smoke",
         net_is_opt_entry_active_in_any_slot_smoke},
        {"pickup_init_and_load_puppy_spawns_smoke", pickup_init_and_load_puppy_spawns_smoke},
        {"pickup_register_existing_object_smoke", pickup_register_existing_object_smoke},
        {"pickup_spawn_with_airdrop_chute_smoke", pickup_spawn_with_airdrop_chute_smoke},
        {"pickup_spawn_list_clear_smoke", pickup_spawn_list_clear_smoke},
        {"pickup_respawn_queue_clear_smoke", pickup_respawn_queue_clear_smoke},
        {"pickup_archive_write_all_smoke", pickup_archive_write_all_smoke},
        {"pickup_archive_read_record_smoke", pickup_archive_read_record_smoke},
        {"pickup_init_smoke", pickup_init_smoke},
        {"pickup_shutdown_smoke", pickup_shutdown_smoke},
        {"pickup_remove_object_smoke", pickup_remove_object_smoke},
        {"pickup_on_collected_no_anim_smoke", pickup_on_collected_no_anim_smoke},
        {"pickup_leaf_helpers_smoke", pickup_leaf_helpers_smoke},
        {"pickup_respawn_spawn_def_smoke", pickup_respawn_spawn_def_smoke},
        {"pickup_respawn_queue_update_smoke", pickup_respawn_queue_update_smoke},
        {"pickup_find_droppable_type_for_current_weapon_smoke",
         pickup_find_droppable_type_for_current_weapon_smoke},
        {"pickup_type_key_table_find_index_smoke", pickup_type_key_table_find_index_smoke},
        {"pickup_type_meta_find_by_name_smoke", pickup_type_meta_find_by_name_smoke},
        {"pickup_spawn_from_parsed_zrd_entry_smoke", pickup_spawn_from_parsed_zrd_entry_smoke},
        {"pickup_handle_pkt11_spawn_delta_smoke", pickup_handle_pkt11_spawn_delta_smoke},
        {"pickup_handle_pkt12_airdrop_spawn_chute_relay_smoke",
         pickup_handle_pkt12_airdrop_spawn_chute_relay_smoke},
        {"pickup_remove_other_spawns_same_opt_entry_smoke",
         pickup_remove_other_spawns_same_opt_entry_smoke},
        {"pickup_resolve_owner_from_bvol_hit_smoke", pickup_resolve_owner_from_bvol_hit_smoke},
        {"pickup_grant_ammo_or_weapon_smoke", pickup_grant_ammo_or_weapon_smoke},
        {"pickup_apply_effect_smoke", pickup_apply_effect_smoke},
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
