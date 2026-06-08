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
extern "C" int zsnd_stream_request_stop_if_active_smoke(void);
extern "C" int zsnd_play_handle_stop_if_active_smoke(void);
extern "C" int zsnd_report_error_helpers_smoke(void);
extern "C" int zsnd_stream_request_queue_smoke(void);
extern "C" int zreader_named_int_lookup_smoke(void);
extern "C" int zreader_get_named_node_smoke(void);
extern "C" int zreader_named_string_float_lookup_smoke(void);
extern "C" int zreader_global_string_prefix_index_smoke(void);
extern "C" int zrndr_global_string_table_load_dynamic_entries_smoke(void);
extern "C" int zreader_load_node_from_archive_smoke(void);
extern "C" int zreader_file_exists_and_list_create_smoke(void);
extern "C" int hud_ui_save_load_entry_is_newer_than_smoke(void);
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
extern "C" int zhud_element_invalidate_smoke(void);
extern "C" int zhud_element_clip_and_invalidate_smoke(void);
extern "C" int zhud_element_constructor_smoke(void);
extern "C" int zhud_element_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_element_destructor_smoke(void);
extern "C" int zhud_element_draw_dispatch_smoke(void);
extern "C" int zhud_widget_constructor_smoke(void);
extern "C" int zhud_slot_draw_smoke(void);
extern "C" int zhud_layout_hw_update_objective_dirty_rect_smoke(void);
extern "C" int zhud_counter_constructor_smoke(void);
extern "C" int zhud_bar_and_meter_constructor_smoke(void);
extern "C" int zhud_widget_release_image_if_owned_smoke(void);
extern "C" int zhud_widget_destructor_core_smoke(void);
extern "C" int zhud_widget_set_image_by_path_owned_smoke(void);
extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void);
extern "C" int zhud_background_cursor_widget_rebuild_captured_image_smoke(void);
extern "C" int zhud_background_cursor_widget_set_image_borrowed_refresh_smoke(void);
extern "C" int zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke(void);
extern "C" int zhud_background_video_widget_constructor_smoke(void);
extern "C" int zhud_background_video_widget_destructor_smoke(void);
extern "C" int zhud_background_constructor_smoke(void);
extern "C" int zhud_text_label_constructor_and_extents_smoke(void);
extern "C" int zhud_panel_constructor_default_smoke(void);
extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void);
extern "C" int zhud_container_child_list_smoke(void);
extern "C" int zhud_zrd_widget_constructor_smoke(void);
extern "C" int zhud_options_dialog_constructor_smoke(void);
extern "C" int zhud_credits_panel_constructor_smoke(void);
extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void);
extern "C" int zhud_cmd_dialog_on_command_selection_changed_smoke(void);
extern "C" int zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke(void);
extern "C" int zhud_cmd_reset_button_on_activate_smoke(void);
extern "C" int zhud_cmd_key_a_button_on_begin_capture_smoke(void);
extern "C" int zhud_cmd_dialog_rebuild_command_binding_lists_smoke(void);
extern "C" int zhud_cmd_dialog_select_group_relative_smoke(void);
extern "C" int zhud_cmd_dialog_select_command_relative_smoke(void);
extern "C" int zhud_cmd_dialog_callback_navigation_smoke(void);
extern "C" int zhud_cmd_dialog_constructor_smoke(void);
extern "C" int zhud_cmd_dialog_destructor_smoke(void);
extern "C" int zhud_cmd_dialog_scalar_deleting_destructor_smoke(void);
extern "C" int zhud_text_input_constructor_smoke(void);
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
extern "C" int zclass_type_list_alloc_and_insert_smoke(void);
extern "C" int zclass_node_action_callback_smoke(void);
extern "C" int zclass_node_priority_smoke(void);
extern "C" int zloc_message_lookup_failure_smoke(void);
extern "C" int zloc_load_unload_messages_dll_smoke(void);
extern "C" int zimage_font_glyph_scan_smoke(void);
extern "C" int zimage_font_measure_string_smoke(void);
extern "C" int zimage_font_blit_string_smoke(void);
extern "C" int zimage_fonts_load_missing_smoke(void);
extern "C" int zvid_pack_color_rgb_smoke(void);
extern "C" int zvid_image_create_format_size_pixels_smoke(void);
extern "C" int zvideo_image_set_pixels_smoke(void);
extern "C" int zvideo_capture_surface_to_image_smoke(void);
extern "C" int zvideo_fx_set_surface_state_smoke(void);
extern "C" int zvideo_image_alpha_clear_smoke(void);
extern "C" int zvideo_buff_clip_coord_to_range_smoke(void);
extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void);
extern "C" int zvideo_texture_pack_load_image_smoke(void);
extern "C" int zvideo_dd_report_error_smoke(void);
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
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdio>
#include <cstring>

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
        {"zsnd_stream_request_stop_if_active_smoke", zsnd_stream_request_stop_if_active_smoke},
        {"zsnd_play_handle_stop_if_active_smoke", zsnd_play_handle_stop_if_active_smoke},
        {"zsnd_report_error_helpers_smoke", zsnd_report_error_helpers_smoke},
        {"zsnd_stream_request_queue_smoke", zsnd_stream_request_queue_smoke},
        {"zreader_named_int_lookup_smoke", zreader_named_int_lookup_smoke},
        {"zreader_get_named_node_smoke", zreader_get_named_node_smoke},
        {"zreader_named_string_float_lookup_smoke", zreader_named_string_float_lookup_smoke},
        {"zreader_global_string_prefix_index_smoke", zreader_global_string_prefix_index_smoke},
        {"zrndr_global_string_table_load_dynamic_entries_smoke",
         zrndr_global_string_table_load_dynamic_entries_smoke},
        {"zreader_load_node_from_archive_smoke", zreader_load_node_from_archive_smoke},
        {"zreader_file_exists_and_list_create_smoke", zreader_file_exists_and_list_create_smoke},
        {"hud_ui_save_load_entry_is_newer_than_smoke",
         hud_ui_save_load_entry_is_newer_than_smoke},
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
        {"zhud_element_invalidate_smoke", zhud_element_invalidate_smoke},
        {"zhud_element_clip_and_invalidate_smoke",
         zhud_element_clip_and_invalidate_smoke},
        {"zhud_element_constructor_smoke", zhud_element_constructor_smoke},
        {"zhud_element_scalar_deleting_destructor_smoke",
         zhud_element_scalar_deleting_destructor_smoke},
        {"zhud_element_destructor_smoke", zhud_element_destructor_smoke},
        {"zhud_element_draw_dispatch_smoke", zhud_element_draw_dispatch_smoke},
        {"zhud_widget_constructor_smoke", zhud_widget_constructor_smoke},
        {"zhud_slot_draw_smoke", zhud_slot_draw_smoke},
        {"zhud_layout_hw_update_objective_dirty_rect_smoke",
         zhud_layout_hw_update_objective_dirty_rect_smoke},
        {"zhud_counter_constructor_smoke", zhud_counter_constructor_smoke},
        {"zhud_bar_and_meter_constructor_smoke", zhud_bar_and_meter_constructor_smoke},
        {"zhud_widget_release_image_if_owned_smoke",
         zhud_widget_release_image_if_owned_smoke},
        {"zhud_widget_destructor_core_smoke",
         zhud_widget_destructor_core_smoke},
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
        {"zhud_text_label_constructor_and_extents_smoke",
         zhud_text_label_constructor_and_extents_smoke},
        {"zhud_panel_constructor_default_smoke", zhud_panel_constructor_default_smoke},
        {"zhud_primitive_bind_target_set_segment_endpoints_smoke",
         zhud_primitive_bind_target_set_segment_endpoints_smoke},
        {"zhud_container_child_list_smoke", zhud_container_child_list_smoke},
        {"zhud_zrd_widget_constructor_smoke", zhud_zrd_widget_constructor_smoke},
        {"zhud_options_dialog_constructor_smoke", zhud_options_dialog_constructor_smoke},
        {"zhud_credits_panel_constructor_smoke", zhud_credits_panel_constructor_smoke},
        {"zhud_cmd_bind_button_base_constructor_smoke",
         zhud_cmd_bind_button_base_constructor_smoke},
        {"zhud_cmd_dialog_on_command_selection_changed_smoke",
         zhud_cmd_dialog_on_command_selection_changed_smoke},
        {"zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke",
         zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke},
        {"zhud_cmd_reset_button_on_activate_smoke",
         zhud_cmd_reset_button_on_activate_smoke},
        {"zhud_cmd_key_a_button_on_begin_capture_smoke",
         zhud_cmd_key_a_button_on_begin_capture_smoke},
        {"zhud_cmd_dialog_rebuild_command_binding_lists_smoke",
         zhud_cmd_dialog_rebuild_command_binding_lists_smoke},
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
        {"zhud_text_input_constructor_smoke", zhud_text_input_constructor_smoke},
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
        {"zclass_type_list_alloc_and_insert_smoke",
         zclass_type_list_alloc_and_insert_smoke},
        {"zclass_node_action_callback_smoke", zclass_node_action_callback_smoke},
        {"zclass_node_priority_smoke", zclass_node_priority_smoke},
        {"zloc_message_lookup_failure_smoke", zloc_message_lookup_failure_smoke},
        {"zloc_load_unload_messages_dll_smoke", zloc_load_unload_messages_dll_smoke},
        {"zimage_font_glyph_scan_smoke", zimage_font_glyph_scan_smoke},
        {"zimage_font_measure_string_smoke", zimage_font_measure_string_smoke},
        {"zimage_font_blit_string_smoke", zimage_font_blit_string_smoke},
        {"zimage_fonts_load_missing_smoke", zimage_fonts_load_missing_smoke},
        {"zvid_pack_color_rgb_smoke", zvid_pack_color_rgb_smoke},
        {"zvid_image_create_format_size_pixels_smoke",
         zvid_image_create_format_size_pixels_smoke},
        {"zvideo_image_set_pixels_smoke", zvideo_image_set_pixels_smoke},
        {"zvideo_capture_surface_to_image_smoke", zvideo_capture_surface_to_image_smoke},
        {"zvideo_fx_set_surface_state_smoke", zvideo_fx_set_surface_state_smoke},
        {"zvideo_image_alpha_clear_smoke", zvideo_image_alpha_clear_smoke},
        {"zvideo_buff_clip_coord_to_range_smoke", zvideo_buff_clip_coord_to_range_smoke},
        {"zvideo_buff_copy_surface_rect_to_image_smoke",
         zvideo_buff_copy_surface_rect_to_image_smoke},
        {"zvideo_texture_pack_load_image_smoke", zvideo_texture_pack_load_image_smoke},
        {"zvideo_dd_report_error_smoke", zvideo_dd_report_error_smoke},
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
