#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "math.h"

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
    setAngle(config.minAllowedAngle);
}


//=========================
//== angleToCompareValue ==
//=========================
// convert angle in degrees to value for timer
inline uint32_t ServoMotor::angleToCompareValue(float angle) const
{
    if (mConfig.invertDirection)
        angle = mConfig.ratedAngle - angle;
    return angle * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / mConfig.ratedAngle + SERVO_MIN_PULSEWIDTH_US;
}


//=========================
//=== angle <-> percent ===
//=========================
// convert between absolute angle and percentage of configured allowed range (e.g. 10-90deg to 0-100%)
inline float ServoMotor::absAngleToRelPercent(float angle) const
{
    return 100 * (angle - mConfig.minAllowedAngle) / (mConfig.maxAllowedAngle - mConfig.minAllowedAngle);
}
inline float ServoMotor::relPercentToAbsAngle(float percent) const
{
    return percent * (mConfig.maxAllowedAngle - mConfig.minAllowedAngle) / 100 + mConfig.minAllowedAngle;
}

//return current position in percent
float ServoMotor::getPercent() const { return absAngleToRelPercent(mCurrentAngle); };

//======================
//======== init ========
//======================
void ServoMotor::init()
{
    ESP_LOGW(TAG, "Initializing servo motor...");
    // configure output that enables the power for servo motor
    ESP_LOGW(TAG, "Init enable power output");
    esp_rom_gpio_pad_select_gpio((gpio_num_t)mConfig.gpioEnablePower);
    gpio_set_direction((gpio_num_t)mConfig.gpioEnablePower, GPIO_MODE_OUTPUT);

    // configure timer for the PWM signal
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
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(mComparator, angleToCompareValue(mConfig.minAllowedAngle)));
    mCurrentAngle = mConfig.minAllowedAngle;

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
    if (newAngle > mConfig.maxAllowedAngle){
        ESP_LOGE(TAG, "Target '%f' exceeds max allowed angle -> limiting to %d", newAngle, mConfig.maxAllowedAngle);
        newAngle = mConfig.maxAllowedAngle;
    }
    if (newAngle < mConfig.minAllowedAngle){
        ESP_LOGE(TAG, "Target '%f' is below min allowed angle -> limiting to %d", newAngle, mConfig.minAllowedAngle);
        newAngle = mConfig.minAllowedAngle;
    }
    // move servo to new angle
    //FIXME: rework or remove below section (power management servo) 
    //  this was quick and dirty fix to prevent servo crash 
    //  at power down while testing
    //only change if not at pos already with 1 deg tolerance
    float angleDiff = fabs(mCurrentAngle - newAngle);
    if (angleDiff > 1){
    ESP_LOGI(TAG, "Setting new angle of rotation: %f (%.1f %% of allowed range)", newAngle, absAngleToRelPercent(newAngle));
    gpio_set_level((gpio_num_t)mConfig.gpioEnablePower, 1);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(mComparator, angleToCompareValue(newAngle)));
    // update stored angle
    // wait some time for target position reached
    #define TIME_MAX_TRAVEL_MS 2000
    #define MIN_ON_TIME 100
    vTaskDelay((angleDiff/mConfig.ratedAngle * TIME_MAX_TRAVEL_MS  + MIN_ON_TIME) / portTICK_PERIOD_MS);
    mCurrentAngle = newAngle;
    gpio_set_level((gpio_num_t)mConfig.gpioEnablePower, 0);
    }
}


//=========================
//===== setPercentage =====
//=========================
// move to percentage within allowed range 0=minAllowed, 100=maxAllowed
void ServoMotor::setPercentage(float percent){
    setAngle(relPercentToAbsAngle(percent));
}


//========================
//===== runTestDrive =====
//========================
void ServoMotor::runTestDrive()
{
    ESP_LOGW(TAG, "Starting test drive sequence");
    setAngle(mConfig.maxAllowedAngle);
    vTaskDelay(pdMS_TO_TICKS(1500));
    setAngle(mConfig.minAllowedAngle);
    vTaskDelay(pdMS_TO_TICKS(1500));
    setAngle(mConfig.maxAllowedAngle/2);
    vTaskDelay(pdMS_TO_TICKS(1000));
    setAngle(mConfig.minAllowedAngle);
    vTaskDelay(pdMS_TO_TICKS(1000));
}