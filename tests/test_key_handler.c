/**
  ******************************************************************************
  * @file           : test_key_handler.c
  * @brief          : 按键处理模块测试
  ******************************************************************************
  */

#include "test_framework.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Mock 按键状态 */
typedef enum {
    MOCK_KEY_NONE = 0,
    MOCK_KEY_ENTER,
    MOCK_KEY_UP,
    MOCK_KEY_DOWN,
    MOCK_KEY_LEFT,
    MOCK_KEY_RIGHT
} MockKeyCode_t;

/* Mock 变量 */
static MockKeyCode_t mock_current_key = MOCK_KEY_NONE;
static bool mock_initialized = false;
static bool mock_key_pressed = false;
static uint32_t mock_key_count = 0;

/* Mock 函数 */
static void Mock_KeyHandler_Init(void)
{
    mock_current_key = MOCK_KEY_NONE;
    mock_initialized = true;
    mock_key_pressed = false;
    mock_key_count = 0;
}

static MockKeyCode_t Mock_Key_Scan(void)
{
    return mock_current_key;
}

static bool Mock_Key_IsPressed(void)
{
    return mock_key_pressed;
}

static void Mock_Key_SetState(MockKeyCode_t key, bool pressed)
{
    mock_current_key = key;
    mock_key_pressed = pressed;
    if (pressed) {
        mock_key_count++;
    }
}

/* Test Cases */

/**
  * @brief  测试按键初始化
  */
TEST_CASE(key_handler_init)
{
    mock_initialized = false;
    mock_current_key = MOCK_KEY_ENTER;

    Mock_KeyHandler_Init();

    ASSERT_EQUAL(1, mock_initialized);
    ASSERT_EQUAL(MOCK_KEY_NONE, mock_current_key);
    ASSERT_EQUAL(0, mock_key_pressed);

    return TEST_PASS;
}

/**
  * @brief  测试按键扫描（无按键）
  */
TEST_CASE(key_scan_none)
{
    Mock_KeyHandler_Init();

    MockKeyCode_t key = Mock_Key_Scan();

    ASSERT_EQUAL(MOCK_KEY_NONE, key);
    ASSERT_EQUAL(0, Mock_Key_IsPressed());

    return TEST_PASS;
}

/**
  * @brief  测试按键扫描（按下 ENTER）
  */
TEST_CASE(key_scan_enter)
{
    Mock_KeyHandler_Init();
    Mock_Key_SetState(MOCK_KEY_ENTER, true);

    MockKeyCode_t key = Mock_Key_Scan();

    ASSERT_EQUAL(MOCK_KEY_ENTER, key);
    ASSERT_EQUAL(1, Mock_Key_IsPressed());

    return TEST_PASS;
}

/**
  * @brief  测试按键扫描（释放）
  */
TEST_CASE(key_scan_release)
{
    Mock_KeyHandler_Init();
    Mock_Key_SetState(MOCK_KEY_ENTER, true);
    Mock_Key_SetState(MOCK_KEY_NONE, false);

    MockKeyCode_t key = Mock_Key_Scan();

    ASSERT_EQUAL(MOCK_KEY_NONE, key);
    ASSERT_EQUAL(0, Mock_Key_IsPressed());

    return TEST_PASS;
}

/**
  * @brief  测试按键计数
  */
TEST_CASE(key_press_count)
{
    Mock_KeyHandler_Init();

    Mock_Key_SetState(MOCK_KEY_ENTER, true);
    Mock_Key_SetState(MOCK_KEY_NONE, false);
    Mock_Key_SetState(MOCK_KEY_UP, true);
    Mock_Key_SetState(MOCK_KEY_NONE, false);

    ASSERT_EQUAL(2, mock_key_count);

    return TEST_PASS;
}
