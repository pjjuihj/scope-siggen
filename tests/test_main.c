/**
  ******************************************************************************
  * @file           : test_main.c
  * @brief          : 测试主程序
  ******************************************************************************
  * @attention
  *
  * 测试主程序，运行所有测试用例
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 外部测试用例声明 */
extern TEST_CASE(ring_buffer_init);
extern TEST_CASE(ring_buffer_put_get);
extern TEST_CASE(ring_buffer_full);
extern TEST_CASE(ring_buffer_empty);
extern TEST_CASE(ring_buffer_peek);
extern TEST_CASE(ring_buffer_wrap);
extern TEST_CASE(ring_buffer_block);
extern TEST_CASE(ring_buffer_flush);
extern TEST_CASE(ring_buffer_init_null_rb);
extern TEST_CASE(ring_buffer_init_null_buffer);
extern TEST_CASE(ring_buffer_init_zero_size);
extern TEST_CASE(ring_buffer_put_null);
extern TEST_CASE(ring_buffer_get_null_rb);
extern TEST_CASE(ring_buffer_get_null_data);
extern TEST_CASE(ring_buffer_peek_null_rb);
extern TEST_CASE(ring_buffer_peek_null_data);
extern TEST_CASE(ring_buffer_peek_empty);
extern TEST_CASE(ring_buffer_fifo_order);
extern TEST_CASE(ring_buffer_count_free_consistency);
extern TEST_CASE(ring_buffer_wrap_fifo_order);
extern TEST_CASE(ring_buffer_putblock_partial);
extern TEST_CASE(ring_buffer_getblock_partial);
extern TEST_CASE(ring_buffer_flush_reuse);
extern TEST_CASE(ring_buffer_isfull_null);
extern TEST_CASE(ring_buffer_isempty_null);
extern TEST_CASE(ring_buffer_count_null);
extern TEST_CASE(ring_buffer_free_null);
extern TEST_CASE(ring_buffer_putblock_null_rb);
extern TEST_CASE(ring_buffer_putblock_null_data);
extern TEST_CASE(ring_buffer_putblock_zero_len);
extern TEST_CASE(ring_buffer_getblock_null_rb);
extern TEST_CASE(ring_buffer_getblock_null_data);
extern TEST_CASE(ring_buffer_getblock_zero_len);
extern TEST_CASE(ring_buffer_flush_null);

extern TEST_CASE(version_get_info);
extern TEST_CASE(version_string);
extern TEST_CASE(version_build_date);
extern TEST_CASE(version_build_time);
extern TEST_CASE(version_compare_equal);
extern TEST_CASE(version_compare_newer);
extern TEST_CASE(version_compare_older);
extern TEST_CASE(version_is_newer);
extern TEST_CASE(version_get_string);
extern TEST_CASE(version_get_timestamp);

/* UART TX 测试用例声明 */
extern TEST_CASE(tx_ring_init);
extern TEST_CASE(tx_ring_put_get);
extern TEST_CASE(tx_ring_full_discard);
extern TEST_CASE(tx_ring_count_free);
extern TEST_CASE(tx_ring_wrap);
extern TEST_CASE(tx_ring_fifo_order);
extern TEST_CASE(tx_ring_get_empty);
extern TEST_CASE(tx_ring_get_partial);
extern TEST_CASE(write_in_isr);
extern TEST_CASE(write_invalid_fd);
extern TEST_CASE(write_mutex_timeout);
extern TEST_CASE(write_buffer_full);
extern TEST_CASE(write_normal);
extern TEST_CASE(write_stderr);
extern TEST_CASE(send_task_basic);
extern TEST_CASE(send_task_dma_busy);
extern TEST_CASE(send_task_no_data);
extern TEST_CASE(send_task_heartbeat);
extern TEST_CASE(send_task_wrap);
extern TEST_CASE(send_task_drop_count);
extern TEST_CASE(dma_tc_callback);
extern TEST_CASE(dma_error_callback);
extern TEST_CASE(dma_error_reset);
extern TEST_CASE(full_flow_basic);
extern TEST_CASE(full_flow_dma_busy);
extern TEST_CASE(uart_tx_mark_ready);
extern TEST_CASE(write_before_mark_ready);
extern TEST_CASE(write_after_mark_ready);
extern TEST_CASE(write_stderr_before_mark_ready);
extern TEST_CASE(mark_ready_then_write);

/* 配置模块测试 */
extern TEST_CASE(config_load_defaults);
extern TEST_CASE(config_init_flash_empty);
extern TEST_CASE(config_init_flash_loaded);
extern TEST_CASE(config_save);
extern TEST_CASE(config_save_not_initialized);

/* 调试模块测试 */
extern TEST_CASE(debug_set_log_level);
extern TEST_CASE(debug_log_level_match);
extern TEST_CASE(debug_log_level_too_low);
extern TEST_CASE(debug_log_error);
extern TEST_CASE(debug_init);

/* 错误跟踪测试 */
extern TEST_CASE(error_tracker_init);
extern TEST_CASE(error_tracker_record_single);
extern TEST_CASE(error_tracker_record_multiple);
extern TEST_CASE(error_tracker_buffer_full);
extern TEST_CASE(error_tracker_clear);

/* 按键处理测试 */
extern TEST_CASE(key_handler_init);
extern TEST_CASE(key_scan_none);
extern TEST_CASE(key_scan_enter);
extern TEST_CASE(key_scan_release);
extern TEST_CASE(key_press_count);

/* UART 协议测试 */
extern TEST_CASE(protocol_init);
extern TEST_CASE(protocol_register);
extern TEST_CASE(protocol_parse_valid);
extern TEST_CASE(protocol_parse_invalid);
extern TEST_CASE(protocol_parse_empty);
extern TEST_CASE(protocol_execute_help);
extern TEST_CASE(protocol_execute_status);
extern TEST_CASE(protocol_execute_version);
extern TEST_CASE(protocol_unknown_cmd);
extern TEST_CASE(protocol_parse_with_param);
extern TEST_CASE(protocol_execute_result);

/* 信号发生器测试 */
extern TEST_CASE(siggen_voltage_to_dac);
extern TEST_CASE(siggen_buffer_size);
extern TEST_CASE(siggen_sine_wave);
extern TEST_CASE(siggen_square_wave);
extern TEST_CASE(siggen_triangle_wave);
extern TEST_CASE(siggen_sawtooth_wave);
extern TEST_CASE(siggen_dc_wave);
extern TEST_CASE(siggen_square_duty);
extern TEST_CASE(siggen_waveform_types);
extern TEST_CASE(siggen_config_struct);
extern TEST_CASE(siggen_status_struct);
extern TEST_CASE(siggen_amplitude_bounds);
extern TEST_CASE(siggen_frequency_param);
extern TEST_CASE(siggen_selftest);

/* 示波器测试 */
extern TEST_CASE(osc_voltage_conversion);
extern TEST_CASE(osc_frequency_1khz);
extern TEST_CASE(osc_frequency_100hz);
extern TEST_CASE(osc_frequency_dc);
extern TEST_CASE(osc_trigger_point);
extern TEST_CASE(osc_trigger_no_edge);
extern TEST_CASE(osc_config_struct);
extern TEST_CASE(osc_status_struct);
extern TEST_CASE(osc_buffer_size);
extern TEST_CASE(osc_sample_rate);
extern TEST_CASE(osc_selftest);

/* 显示模块测试 */
extern TEST_CASE(display_constants);
extern TEST_CASE(display_measurements_struct);
extern TEST_CASE(display_page_types);
extern TEST_CASE(display_format_hz);
extern TEST_CASE(display_format_khz);
extern TEST_CASE(display_format_mhz);
extern TEST_CASE(display_format_voltage);
extern TEST_CASE(display_measure_1khz);
extern TEST_CASE(display_measure_dc);
extern TEST_CASE(display_cursor_range);
extern TEST_CASE(display_timebase_zoom);

/* 应用初始化测试 */
extern TEST_CASE(appinit_boot_stages);
extern TEST_CASE(appinit_stage_strings);
extern TEST_CASE(appinit_simulate_init);
extern TEST_CASE(appinit_retry_constants);
extern TEST_CASE(appinit_simulate_shutdown);

/* 上位机通信测试 */
extern TEST_CASE(uc_state_enum);
extern TEST_CASE(uc_prefix_constants);
extern TEST_CASE(uc_hex_single);
extern TEST_CASE(uc_hex_multiple);
extern TEST_CASE(uc_hex_zero);
extern TEST_CASE(uc_format_freq);
extern TEST_CASE(uc_format_voltage);
extern TEST_CASE(uc_format_status);
extern TEST_CASE(uc_format_ok);
extern TEST_CASE(uc_format_error);
extern TEST_CASE(uc_format_info);
extern TEST_CASE(uc_packet_size_limit);
extern TEST_CASE(uc_stream_interval_range);

/* 代码审查测试 */
extern TEST_CASE(code_reviewer_check_types);
extern TEST_CASE(code_reviewer_check_results);
extern TEST_CASE(code_reviewer_report_struct);
extern TEST_CASE(code_reviewer_score_all_pass);
extern TEST_CASE(code_reviewer_score_with_warnings);
extern TEST_CASE(code_reviewer_score_with_fails);
extern TEST_CASE(code_reviewer_score_with_errors);
extern TEST_CASE(code_reviewer_score_mixed);
extern TEST_CASE(code_reviewer_score_min_zero);
extern TEST_CASE(code_reviewer_add_item);
extern TEST_CASE(code_reviewer_add_multiple_items);
extern TEST_CASE(code_reviewer_item_overflow);

/* 单元测试模块测试 */
extern TEST_CASE(unit_test_function_exists);
extern TEST_CASE(unit_test_send_message);
extern TEST_CASE(unit_test_message_format);
extern TEST_CASE(unit_test_null_param);

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

/* 环形缓冲测试套件 */
TEST_SUITE_BEGIN(ring_buffer_suite)
    TEST_CASE_ENTRY(ring_buffer_init)
    TEST_CASE_ENTRY(ring_buffer_put_get)
    TEST_CASE_ENTRY(ring_buffer_full)
    TEST_CASE_ENTRY(ring_buffer_empty)
    TEST_CASE_ENTRY(ring_buffer_peek)
    TEST_CASE_ENTRY(ring_buffer_wrap)
    TEST_CASE_ENTRY(ring_buffer_block)
    TEST_CASE_ENTRY(ring_buffer_flush)
    /* NULL 参数校验 */
    TEST_CASE_ENTRY(ring_buffer_init_null_rb)
    TEST_CASE_ENTRY(ring_buffer_init_null_buffer)
    TEST_CASE_ENTRY(ring_buffer_init_zero_size)
    TEST_CASE_ENTRY(ring_buffer_put_null)
    TEST_CASE_ENTRY(ring_buffer_get_null_rb)
    TEST_CASE_ENTRY(ring_buffer_get_null_data)
    TEST_CASE_ENTRY(ring_buffer_peek_null_rb)
    TEST_CASE_ENTRY(ring_buffer_peek_null_data)
    TEST_CASE_ENTRY(ring_buffer_peek_empty)
    /* FIFO 顺序与一致性 */
    TEST_CASE_ENTRY(ring_buffer_fifo_order)
    TEST_CASE_ENTRY(ring_buffer_count_free_consistency)
    TEST_CASE_ENTRY(ring_buffer_wrap_fifo_order)
    /* 部分操作 */
    TEST_CASE_ENTRY(ring_buffer_putblock_partial)
    TEST_CASE_ENTRY(ring_buffer_getblock_partial)
    /* Flush 后重用 */
    TEST_CASE_ENTRY(ring_buffer_flush_reuse)
    /* 边界 NULL 安全 */
    TEST_CASE_ENTRY(ring_buffer_isfull_null)
    TEST_CASE_ENTRY(ring_buffer_isempty_null)
    TEST_CASE_ENTRY(ring_buffer_count_null)
    TEST_CASE_ENTRY(ring_buffer_free_null)
    TEST_CASE_ENTRY(ring_buffer_putblock_null_rb)
    TEST_CASE_ENTRY(ring_buffer_putblock_null_data)
    TEST_CASE_ENTRY(ring_buffer_putblock_zero_len)
    TEST_CASE_ENTRY(ring_buffer_getblock_null_rb)
    TEST_CASE_ENTRY(ring_buffer_getblock_null_data)
    TEST_CASE_ENTRY(ring_buffer_getblock_zero_len)
    TEST_CASE_ENTRY(ring_buffer_flush_null)
TEST_SUITE_END(ring_buffer_suite)

/* 版本管理测试套件 */
TEST_SUITE_BEGIN(version_suite)
    TEST_CASE_ENTRY(version_get_info)
    TEST_CASE_ENTRY(version_string)
    TEST_CASE_ENTRY(version_build_date)
    TEST_CASE_ENTRY(version_build_time)
    TEST_CASE_ENTRY(version_compare_equal)
    TEST_CASE_ENTRY(version_compare_newer)
    TEST_CASE_ENTRY(version_compare_older)
    TEST_CASE_ENTRY(version_is_newer)
    TEST_CASE_ENTRY(version_get_string)
    TEST_CASE_ENTRY(version_get_timestamp)
TEST_SUITE_END(version_suite)

/* UART TX 测试套件 */
TEST_SUITE_BEGIN(uart_tx_suite)
    /* Phase 1: Ring Buffer */
    TEST_CASE_ENTRY(tx_ring_init)
    TEST_CASE_ENTRY(tx_ring_put_get)
    TEST_CASE_ENTRY(tx_ring_full_discard)
    TEST_CASE_ENTRY(tx_ring_count_free)
    TEST_CASE_ENTRY(tx_ring_wrap)
    TEST_CASE_ENTRY(tx_ring_fifo_order)
    TEST_CASE_ENTRY(tx_ring_get_empty)
    TEST_CASE_ENTRY(tx_ring_get_partial)
    /* Phase 2: _write() 逻辑 */
    TEST_CASE_ENTRY(write_in_isr)
    TEST_CASE_ENTRY(write_invalid_fd)
    TEST_CASE_ENTRY(write_mutex_timeout)
    TEST_CASE_ENTRY(write_buffer_full)
    TEST_CASE_ENTRY(write_normal)
    TEST_CASE_ENTRY(write_stderr)
    /* Phase 3: 发送任务逻辑 */
    TEST_CASE_ENTRY(send_task_basic)
    TEST_CASE_ENTRY(send_task_dma_busy)
    TEST_CASE_ENTRY(send_task_no_data)
    TEST_CASE_ENTRY(send_task_heartbeat)
    TEST_CASE_ENTRY(send_task_wrap)
    TEST_CASE_ENTRY(send_task_drop_count)
    /* Phase 4: DMA 回调 */
    TEST_CASE_ENTRY(dma_tc_callback)
    TEST_CASE_ENTRY(dma_error_callback)
    TEST_CASE_ENTRY(dma_error_reset)
    TEST_CASE_ENTRY(full_flow_basic)
    TEST_CASE_ENTRY(full_flow_dma_busy)
    /* Phase 5: MarkReady 测试 */
    TEST_CASE_ENTRY(uart_tx_mark_ready)
    TEST_CASE_ENTRY(write_before_mark_ready)
    TEST_CASE_ENTRY(write_after_mark_ready)
    TEST_CASE_ENTRY(write_stderr_before_mark_ready)
    TEST_CASE_ENTRY(mark_ready_then_write)
TEST_SUITE_END(uart_tx_suite)

/* 配置模块测试套件 */
TEST_SUITE_BEGIN(config_suite)
    TEST_CASE_ENTRY(config_load_defaults)
    TEST_CASE_ENTRY(config_init_flash_empty)
    TEST_CASE_ENTRY(config_init_flash_loaded)
    TEST_CASE_ENTRY(config_save)
    TEST_CASE_ENTRY(config_save_not_initialized)
TEST_SUITE_END(config_suite)

/* 调试模块测试套件 */
TEST_SUITE_BEGIN(debug_suite)
    TEST_CASE_ENTRY(debug_set_log_level)
    TEST_CASE_ENTRY(debug_log_level_match)
    TEST_CASE_ENTRY(debug_log_level_too_low)
    TEST_CASE_ENTRY(debug_log_error)
    TEST_CASE_ENTRY(debug_init)
TEST_SUITE_END(debug_suite)

/* 错误跟踪测试套件 */
TEST_SUITE_BEGIN(error_tracker_suite)
    TEST_CASE_ENTRY(error_tracker_init)
    TEST_CASE_ENTRY(error_tracker_record_single)
    TEST_CASE_ENTRY(error_tracker_record_multiple)
    TEST_CASE_ENTRY(error_tracker_buffer_full)
    TEST_CASE_ENTRY(error_tracker_clear)
TEST_SUITE_END(error_tracker_suite)

/* 按键处理测试套件 */
TEST_SUITE_BEGIN(key_handler_suite)
    TEST_CASE_ENTRY(key_handler_init)
    TEST_CASE_ENTRY(key_scan_none)
    TEST_CASE_ENTRY(key_scan_enter)
    TEST_CASE_ENTRY(key_scan_release)
    TEST_CASE_ENTRY(key_press_count)
TEST_SUITE_END(key_handler_suite)

/* UART 协议测试套件 */
TEST_SUITE_BEGIN(uart_protocol_suite)
    TEST_CASE_ENTRY(protocol_init)
    TEST_CASE_ENTRY(protocol_register)
    TEST_CASE_ENTRY(protocol_parse_valid)
    TEST_CASE_ENTRY(protocol_parse_invalid)
    TEST_CASE_ENTRY(protocol_parse_empty)
    TEST_CASE_ENTRY(protocol_execute_help)
    TEST_CASE_ENTRY(protocol_execute_status)
    TEST_CASE_ENTRY(protocol_execute_version)
    TEST_CASE_ENTRY(protocol_unknown_cmd)
    TEST_CASE_ENTRY(protocol_parse_with_param)
    TEST_CASE_ENTRY(protocol_execute_result)
TEST_SUITE_END(uart_protocol_suite)

/* 信号发生器测试套件 */
TEST_SUITE_BEGIN(signal_gen_suite)
    TEST_CASE_ENTRY(siggen_voltage_to_dac)
    TEST_CASE_ENTRY(siggen_buffer_size)
    TEST_CASE_ENTRY(siggen_sine_wave)
    TEST_CASE_ENTRY(siggen_square_wave)
    TEST_CASE_ENTRY(siggen_triangle_wave)
    TEST_CASE_ENTRY(siggen_sawtooth_wave)
    TEST_CASE_ENTRY(siggen_dc_wave)
    TEST_CASE_ENTRY(siggen_square_duty)
    TEST_CASE_ENTRY(siggen_waveform_types)
    TEST_CASE_ENTRY(siggen_config_struct)
    TEST_CASE_ENTRY(siggen_status_struct)
    TEST_CASE_ENTRY(siggen_amplitude_bounds)
    TEST_CASE_ENTRY(siggen_frequency_param)
    TEST_CASE_ENTRY(siggen_selftest)
TEST_SUITE_END(signal_gen_suite)

/* 示波器测试套件 */
TEST_SUITE_BEGIN(oscilloscope_suite)
    TEST_CASE_ENTRY(osc_voltage_conversion)
    TEST_CASE_ENTRY(osc_frequency_1khz)
    TEST_CASE_ENTRY(osc_frequency_100hz)
    TEST_CASE_ENTRY(osc_frequency_dc)
    TEST_CASE_ENTRY(osc_trigger_point)
    TEST_CASE_ENTRY(osc_trigger_no_edge)
    TEST_CASE_ENTRY(osc_config_struct)
    TEST_CASE_ENTRY(osc_status_struct)
    TEST_CASE_ENTRY(osc_buffer_size)
    TEST_CASE_ENTRY(osc_sample_rate)
    TEST_CASE_ENTRY(osc_selftest)
TEST_SUITE_END(oscilloscope_suite)

/* 显示模块测试套件 */
TEST_SUITE_BEGIN(display_suite)
    TEST_CASE_ENTRY(display_constants)
    TEST_CASE_ENTRY(display_measurements_struct)
    TEST_CASE_ENTRY(display_page_types)
    TEST_CASE_ENTRY(display_format_hz)
    TEST_CASE_ENTRY(display_format_khz)
    TEST_CASE_ENTRY(display_format_mhz)
    TEST_CASE_ENTRY(display_format_voltage)
    TEST_CASE_ENTRY(display_measure_1khz)
    TEST_CASE_ENTRY(display_measure_dc)
    TEST_CASE_ENTRY(display_cursor_range)
    TEST_CASE_ENTRY(display_timebase_zoom)
TEST_SUITE_END(display_suite)

/* 应用初始化测试套件 */
TEST_SUITE_BEGIN(app_init_suite)
    TEST_CASE_ENTRY(appinit_boot_stages)
    TEST_CASE_ENTRY(appinit_stage_strings)
    TEST_CASE_ENTRY(appinit_simulate_init)
    TEST_CASE_ENTRY(appinit_retry_constants)
    TEST_CASE_ENTRY(appinit_simulate_shutdown)
TEST_SUITE_END(app_init_suite)

/* 上位机通信测试套件 */
TEST_SUITE_BEGIN(upper_computer_suite)
    TEST_CASE_ENTRY(uc_state_enum)
    TEST_CASE_ENTRY(uc_prefix_constants)
    TEST_CASE_ENTRY(uc_hex_single)
    TEST_CASE_ENTRY(uc_hex_multiple)
    TEST_CASE_ENTRY(uc_hex_zero)
    TEST_CASE_ENTRY(uc_format_freq)
    TEST_CASE_ENTRY(uc_format_voltage)
    TEST_CASE_ENTRY(uc_format_status)
    TEST_CASE_ENTRY(uc_format_ok)
    TEST_CASE_ENTRY(uc_format_error)
    TEST_CASE_ENTRY(uc_format_info)
    TEST_CASE_ENTRY(uc_packet_size_limit)
    TEST_CASE_ENTRY(uc_stream_interval_range)
TEST_SUITE_END(upper_computer_suite)

/* 代码审查测试套件 */
TEST_SUITE_BEGIN(code_reviewer_suite)
    TEST_CASE_ENTRY(code_reviewer_check_types)
    TEST_CASE_ENTRY(code_reviewer_check_results)
    TEST_CASE_ENTRY(code_reviewer_report_struct)
    TEST_CASE_ENTRY(code_reviewer_score_all_pass)
    TEST_CASE_ENTRY(code_reviewer_score_with_warnings)
    TEST_CASE_ENTRY(code_reviewer_score_with_fails)
    TEST_CASE_ENTRY(code_reviewer_score_with_errors)
    TEST_CASE_ENTRY(code_reviewer_score_mixed)
    TEST_CASE_ENTRY(code_reviewer_score_min_zero)
    TEST_CASE_ENTRY(code_reviewer_add_item)
    TEST_CASE_ENTRY(code_reviewer_add_multiple_items)
    TEST_CASE_ENTRY(code_reviewer_item_overflow)
TEST_SUITE_END(code_reviewer_suite)

/* 单元测试模块测试套件 */
TEST_SUITE_BEGIN(unit_test_suite)
    TEST_CASE_ENTRY(unit_test_function_exists)
    TEST_CASE_ENTRY(unit_test_send_message)
    TEST_CASE_ENTRY(unit_test_message_format)
    TEST_CASE_ENTRY(unit_test_null_param)
TEST_SUITE_END(unit_test_suite)

/**
  * @brief  测试主函数
  * @retval 测试结果
  */
int main(void)
{
    printf("\n=== SCOPE-SIGGEN Test Suite ===\n");
    printf("Version: %s\n", VERSION_STRING);
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("==============================\n");

    TestResult_t result = TEST_PASS;

    /* 运行环形缓冲测试 */
    result |= Test_RunSuite(&ring_buffer_suite);

    /* 运行版本管理测试 */
    result |= Test_RunSuite(&version_suite);

    /* 运行 UART TX 测试 */
    result |= Test_RunSuite(&uart_tx_suite);

    /* 运行配置模块测试 */
    result |= Test_RunSuite(&config_suite);

    /* 运行调试模块测试 */
    result |= Test_RunSuite(&debug_suite);

    /* 运行错误跟踪测试 */
    result |= Test_RunSuite(&error_tracker_suite);

    /* 运行按键处理测试 */
    result |= Test_RunSuite(&key_handler_suite);

    /* 运行 UART 协议测试 */
    result |= Test_RunSuite(&uart_protocol_suite);

    /* 运行信号发生器测试 */
    result |= Test_RunSuite(&signal_gen_suite);

    /* 运行示波器测试 */
    result |= Test_RunSuite(&oscilloscope_suite);

    /* 运行显示模块测试 */
    result |= Test_RunSuite(&display_suite);

    /* 运行应用初始化测试 */
    result |= Test_RunSuite(&app_init_suite);

    /* 运行上位机通信测试 */
    result |= Test_RunSuite(&upper_computer_suite);

    /* 运行代码审查测试 */
    result |= Test_RunSuite(&code_reviewer_suite);

    /* 运行单元测试模块测试 */
    result |= Test_RunSuite(&unit_test_suite);

    /* 打印总报告 */
    printf("\n=== Overall Test Report ===\n");
    printf("Ring Buffer: %s\n",
           (ring_buffer_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Version: %s\n",
           (version_suite.failed == 0) ? "PASS" : "FAIL");
    printf("UART TX: %s\n",
           (uart_tx_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Config: %s\n",
           (config_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Debug: %s\n",
           (debug_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Error Tracker: %s\n",
           (error_tracker_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Key Handler: %s\n",
           (key_handler_suite.failed == 0) ? "PASS" : "FAIL");
    printf("UART Protocol: %s\n",
           (uart_protocol_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Signal Gen: %s\n",
           (signal_gen_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Oscilloscope: %s\n",
           (oscilloscope_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Display: %s\n",
           (display_suite.failed == 0) ? "PASS" : "FAIL");
    printf("App Init: %s\n",
           (app_init_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Upper Computer: %s\n",
           (upper_computer_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Code Reviewer: %s\n",
           (code_reviewer_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Unit Test: %s\n",
           (unit_test_suite.failed == 0) ? "PASS" : "FAIL");
    printf("===========================\n");

    if (result == TEST_PASS) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }

    return (result == TEST_PASS) ? 0 : 1;
}

/* Private function implementations ------------------------------------------*/
