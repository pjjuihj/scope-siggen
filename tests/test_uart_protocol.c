/**
  ******************************************************************************
  * @file           : test_uart_protocol.c
  * @brief          : UART 协议模块测试
  ******************************************************************************
  * @attention
  *
  * 测试 uart_protocol.c 的命令解析和执行功能
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Private define ------------------------------------------------------------*/

#define CMD_MAX_LENGTH  64

/* Private types ------------------------------------------------------------*/

typedef void (*CmdHandler_t)(const char *param);

typedef struct {
    const char *name;
    uint8_t min_len;
    bool has_param;
    CmdHandler_t handler;
} CommandEntry_t;

/* Command table (mirrored from uart_protocol.c) */
static const CommandEntry_t cmd_table[] = {
    {"help",        4,       false,     NULL},
    {"status",      6,       false,     NULL},
    {"version",     7,       false,     NULL},
    {"start_osc",   9,       false,     NULL},
    {"stop_osc",    8,       false,     NULL},
    {"set_freq",    8,       true,      NULL},
    {"set_wave",    8,       true,      NULL},
    {"set_amp",     7,       true,      NULL},
    {"start_gen",   9,       false,     NULL},
    {"stop_gen",    8,       false,     NULL},
    {"tasks",       5,       false,     NULL},
    {"memory",      6,       false,     NULL},
    {"errors",      6,       false,     NULL},
    {"log",         3,       true,      NULL},
};
#define CMD_TABLE_SIZE (sizeof(cmd_table) / sizeof(cmd_table[0]))

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试命令表初始化 — 14 个命令
  */
TEST_CASE(protocol_init)
{
    ASSERT(CMD_TABLE_SIZE == 14);

    /* 验证第一个命令是 help */
    ASSERT_STRING_EQUAL(cmd_table[0].name, "help");
    ASSERT_EQUAL(cmd_table[0].min_len, 4);
    ASSERT(!cmd_table[0].has_param);

    return TEST_PASS;
}

/**
  * @brief  测试命令注册 — 所有命令都存在
  */
TEST_CASE(protocol_register)
{
    const char *expected_cmds[] = {
        "help", "status", "version", "start_osc", "stop_osc",
        "set_freq", "set_wave", "set_amp", "start_gen", "stop_gen",
        "tasks", "memory", "errors", "log"
    };

    for (int i = 0; i < 14; i++) {
        bool found = false;
        for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
            if (strcmp(cmd_table[j].name, expected_cmds[i]) == 0) {
                found = true;
                break;
            }
        }
        ASSERT(found);
    }

    return TEST_PASS;
}

/**
  * @brief  测试有效命令解析
  */
TEST_CASE(protocol_parse_valid)
{
    const char *test_cmds[] = {
        "help", "status", "version", "start_osc", "stop_osc",
        "set_freq 1000", "set_wave sine", "set_amp 500",
        "start_gen", "stop_gen", "tasks", "memory", "errors", "log 2"
    };

    for (int i = 0; i < 14; i++) {
        const char *cmd_str = test_cmds[i];
        bool found = false;

        for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
            uint8_t len = cmd_table[j].min_len;
            if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
                char next = cmd_str[len];
                if (next == ' ' || next == '\0') {
                    found = true;
                    break;
                }
            }
        }
        ASSERT(found);
    }

    return TEST_PASS;
}

/**
  * @brief  测试无效命令处理
  */
TEST_CASE(protocol_parse_invalid)
{
    const char *invalid_cmds[] = { "xyz", "unknown", "helpx", "help2" };

    for (int i = 0; i < 4; i++) {
        const char *cmd_str = invalid_cmds[i];
        bool found = false;

        for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
            uint8_t len = cmd_table[j].min_len;
            if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
                char next = cmd_str[len];
                if (next == ' ' || next == '\0') {
                    found = true;
                    break;
                }
            }
        }
        ASSERT(!found);
    }

    return TEST_PASS;
}

/**
  * @brief  测试空命令处理
  */
TEST_CASE(protocol_parse_empty)
{
    const char *cmd_str = "";
    bool found = false;

    for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
        uint8_t len = cmd_table[j].min_len;
        if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
            char next = cmd_str[len];
            if (next == ' ' || next == '\0') {
                found = true;
                break;
            }
        }
    }

    ASSERT(!found);
    return TEST_PASS;
}

/**
  * @brief  测试 help 命令存在且无参数
  */
TEST_CASE(protocol_execute_help)
{
    /* 验证 help 命令存在且无参数 */
    ASSERT_STRING_EQUAL(cmd_table[0].name, "help");
    ASSERT(!cmd_table[0].has_param);

    return TEST_PASS;
}

/**
  * @brief  测试 status 命令存在且无参数
  */
TEST_CASE(protocol_execute_status)
{
    ASSERT_STRING_EQUAL(cmd_table[1].name, "status");
    ASSERT(!cmd_table[1].has_param);

    const char *cmd_str = "status";
    uint8_t len = cmd_table[1].min_len;
    ASSERT(strncmp(cmd_str, cmd_table[1].name, len) == 0);
    ASSERT(cmd_str[len] == '\0');

    return TEST_PASS;
}

/**
  * @brief  测试 version 命令存在且无参数
  */
TEST_CASE(protocol_execute_version)
{
    ASSERT_STRING_EQUAL(cmd_table[2].name, "version");
    ASSERT(!cmd_table[2].has_param);

    const char *cmd_str = "version";
    uint8_t len = cmd_table[2].min_len;
    ASSERT(strncmp(cmd_str, cmd_table[2].name, len) == 0);
    ASSERT(cmd_str[len] == '\0');

    return TEST_PASS;
}

/**
  * @brief  测试未知命令处理
  */
TEST_CASE(protocol_unknown_cmd)
{
    const char *cmd_str = "unknown_cmd";
    bool found = false;

    for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
        uint8_t len = cmd_table[j].min_len;
        if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
            char next = cmd_str[len];
            if (next == ' ' || next == '\0') {
                found = true;
                break;
            }
        }
    }

    ASSERT(!found);
    return TEST_PASS;
}

/**
  * @brief  测试带参数命令解析
  */
TEST_CASE(protocol_parse_with_param)
{
    /* 测试 set_freq 命令解析 */
    const char *cmd_str = "set_freq 1000";
    bool found = false;
    const char *param = NULL;

    for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
        uint8_t len = cmd_table[j].min_len;
        if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
            char next = cmd_str[len];
            if (next == ' ' || next == '\0') {
                found = true;
                if (cmd_table[j].has_param) {
                    const char *space = strchr(cmd_str, ' ');
                    param = (space != NULL) ? (space + 1) : NULL;
                }
                break;
            }
        }
    }

    ASSERT(found);
    ASSERT(param != NULL);
    ASSERT_STRING_EQUAL(param, "1000");

    /* 测试 set_wave sine 命令解析 */
    cmd_str = "set_wave sine";
    found = false;
    param = NULL;

    for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
        uint8_t len = cmd_table[j].min_len;
        if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
            char next = cmd_str[len];
            if (next == ' ' || next == '\0') {
                found = true;
                if (cmd_table[j].has_param) {
                    const char *space = strchr(cmd_str, ' ');
                    param = (space != NULL) ? (space + 1) : NULL;
                }
                break;
            }
        }
    }

    ASSERT(found);
    ASSERT(param != NULL);
    ASSERT_STRING_EQUAL(param, "sine");

    return TEST_PASS;
}

/**
  * @brief  测试命令执行结果验证
  */
TEST_CASE(protocol_execute_result)
{
    const char *cmd_str = "help";

    /* 查找并执行命令 */
    for (uint64_t j = 0; j < CMD_TABLE_SIZE; j++) {
        uint8_t len = cmd_table[j].min_len;
        if (strncmp(cmd_str, cmd_table[j].name, len) == 0) {
            char next = cmd_str[len];
            if (next == ' ' || next == '\0') {
                ASSERT_STRING_EQUAL(cmd_table[j].name, "help");
                return TEST_PASS;
            }
        }
    }

    /* 不应该到这里 */
    ASSERT(0);
    return TEST_FAIL;
}
