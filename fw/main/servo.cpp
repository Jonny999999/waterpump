#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "servo.hpp"


// parameters from datasheet: 
// note: seem to be very common values - same for most stepper motors
#define SERVO_MIN_PULSEWIDTH_US 500  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2500 // Maximum pulse width in microsecond
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 20000          // 20000 ticks, 20ms


static const char *TAG = "servo";


//=======================
//===== Constructor =====
//=======================
ServoMotor::ServoMotor(servoConfig_t config){
    mConfig = config;
    init();
    setAngle(0);
}


//=========================
//== angleToCompareValue ==
//=========================
// convert angle in degrees to value for timer
inline uint32_t ServoMotor::angleToCompareValue(float angle)
{
    if (mConfig.invertDirection)
        angle = mConfig.maxAngle - angle;
    return (angle - mConfig.minAngle) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (mConfig.maxAngle - mConfig.minAngle) + SERVO_MIN_PULSEWIDTH_US;
}


//======================
//======== init ========
//======================
void ServoMotor::init()
{
    ESP_LOGW(TAG, "Initializing servo motor...");
    ESP_LOGI(TAG, "Create timer and operator");
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = SERVO_TIMEBASE_PERIOD,
        .intr_priority = 0,
        .flags {},
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {};
    operator_config.group_id = 0; // operator must be in the same group to the timer

    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_comparator_config_t comparator_config = {};
    comparator_config.flags.update_cmp_on_tez = true;

    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &mComparator));

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {};
    generator_config.gen_gpio_num = mConfig.gpioPwmSignal;

    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // set the initial compare value, so that the servo will spin to min pos
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(mComparator, angleToCompareValue(mConfig.minAngle)));
    mCurrentAngle = mConfig.minAngle;

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, mComparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}


//========================
//======= setAngle =======
//========================
void ServoMotor::setAngle(float newAngle)
{
    // limit angle to configured range
    if (newAngle > mConfig.maxAngle){
        ESP_LOGE(TAG, "Target '%f' exceeds max angle -> limiting to %d", newAngle, mConfig.maxAngle);
        newAngle = mConfig.maxAngle;
    }
    if (newAngle < mConfig.minAngle){
        ESP_LOGE(TAG, "Target '%f' is below min angle -> limiting to %d", newAngle, mConfig.minAngle);
        newAngle = mConfig.minAngle;
    }
    // move servo to new angle
    ESP_LOGI(TAG, "Setting new angle of rotation: %f", newAngle);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(mComparator, angleToCompareValue(newAngle)));
    // update stored angle
    mCurrentAngle = newAngle;
}


//========================
//===== runTestDrive =====
//========================
void ServoMotor::runTestDrive()
{
    ESP_LOGW(TAG, "Starting test drive sequence");
    setAngle(mConfig.maxAngle);
    vTaskDelay(pdMS_TO_TICKS(1500));
    setAngle(mConfig.minAngle);
    vTaskDelay(pdMS_TO_TICKS(1500));
    setAngle(mConfig.maxAngle/2);
    vTaskDelay(pdMS_TO_TICKS(1000));
    setAngle(mConfig.minAngle);
    vTaskDelay(pdMS_TO_TICKS(1000));
}