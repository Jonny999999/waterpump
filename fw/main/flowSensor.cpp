extern "C"
{
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_timer.h"
}
#include "flowSensor.hpp"


// tag for logging
static const char *TAG = "flowSensor";



// constructor
FlowSensor::FlowSensor(gpio_num_t gpioPulse, float pulsesPerLiter, uint32_t bufferSize)
{
    ESP_LOGW(TAG, "Creating flow sensor...");

    // copy config
    mGpioPulse = gpioPulse;
    mConfigPulsesPerLiter = pulsesPerLiter;
    mBufferSize = bufferSize;

    // create buffer initialized with 0
    mPulseCounts = new uint32_t[mBufferSize]();
    mTimestamps = new uint32_t[mBufferSize]();

    // init(); not running init here due to error when running gpio_install_isr_service(0)
}


//100l/min -> 20pulses/s => 1 period = 50000us
//2024.05.17: at 2bar fully open valve measuread period = ~25ms using scope -> decreasing value
#define MIN_PULSE_DURATION_US 10000 //period (rising edge to next rising edge)
void FlowSensor::countPulse()
{
    // get current timestamp in us
    int64_t now = esp_timer_get_time();

    // count pulse if last edge long enough ago
    // FIXME: this idea of removing noise only works when there are single noise spikes, if there is continous noise, nothing will be counted => needs testing
    if (now - mTimestampLastPulseUs > MIN_PULSE_DURATION_US) {
        // increment pulse count by 1
        mPulseCount++;
    }
    else
    {
        // track ignored pulses for debugging
        mIgnoredPulses++;
    }

    // update timestamp last pulse (in any case)
    mTimestampLastPulseUs = now;
}



// ISR function to count a pulse at rising edge interrupt
static void IRAM_ATTR isr_handler_countPulse(void *arg)
{
    FlowSensor *sensor = static_cast<FlowSensor *>(arg);
    sensor->countPulse();
}



void FlowSensor::init()
{
    if (mIsInitialized) {
        ESP_LOGE(TAG, "already initialized");
        return;
    }
    // configure gpio    
    ESP_LOGI(TAG, "configuring pin %d as input...", (int)mGpioPulse);
    gpio_set_direction(mGpioPulse, GPIO_MODE_INPUT);
    // Install ISR service
    ESP_LOGI(TAG, "installing ISR...");
    // gpio_install_isr_service(0);
    // FIXME this cant be run here (even when run later in the program)` E (633) gpio: esp_ipc_call_blocking failed (0x103) `
    // only works in main function, why?

    // attach ISR to the GPIO pin
    esp_err_t err = gpio_isr_handler_add(mGpioPulse, isr_handler_countPulse, (void *)this);
    gpio_set_intr_type(mGpioPulse, GPIO_INTR_POSEDGE);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "failed to register ISR! make sure to run `gpio_install_isr_service(0)` before .init()");
        return;
    }
    mIsInitialized = true;
}



void FlowSensor::reset(){
    ESP_LOGI(TAG, "resetting pulse count to 0");
    mPulseCount = 0;
    read(); // sets volume, flow to 0 and updates timestamp
}



// update and calculate stored volume and flow rate using pulses during time since last run
// TODO: what if read is called too quickly or too slow? -> add time limits to prevent unrealistic results?
#define MIN_READ_PERIOD_MS 100 // minimum time that has to pass between read (prevent inacurate flow readings)
void FlowSensor::read()
{
    // check initialized
    if (!mIsInitialized)
    {
        ESP_LOGE(TAG, "Flow sensor is not initialized! run .init() first! - not reading any values");
        return;
    }

    // prevent too fast flow update (inaccurate)
    uint32_t timeSinceLastRun = esp_log_timestamp() - mTimestampLastRead;
    if (timeSinceLastRun < MIN_READ_PERIOD_MS)
    {
        ESP_LOGE(TAG, ".read(): already read %ldms ago - updating volume only! (min gap is %dms)", timeSinceLastRun, MIN_READ_PERIOD_MS);
        // calculate absolute volume
        mVolume_liter = mPulseCount / mConfigPulsesPerLiter;
        return;
    }

    // update circular buffer
    mPulseCounts[mBufferIndex] = mPulseCount;
    mTimestamps[mBufferIndex] = esp_log_timestamp();

    // get relevant flow rate parameters from buffer
    uint32_t pulsesNow = mPulseCounts[mBufferIndex];                        // latest value
    uint32_t pulsesPast = mPulseCounts[(mBufferIndex + 1) % mBufferSize];   // oldest value in buffer
    uint32_t timestampNow = mTimestamps[mBufferIndex];                      // latest value
    uint32_t timestampPast = mTimestamps[(mBufferIndex + 1) % mBufferSize]; // oldest value in buffer
    uint32_t msPassed = timestampNow - timestampPast;

    mBufferIndex = (mBufferIndex + 1) % mBufferSize;

    // calculate flow rate
    mFlow_literPerSecond = ((double)(pulsesNow - pulsesPast) / mConfigPulsesPerLiter) / ((double)msPassed / 1000);

    // calculate absolute volume
    mVolume_liter = pulsesNow / mConfigPulsesPerLiter;

    // Log read stats
    ESP_LOGD(TAG, "read - timeSinceLastRead=%ld, newPulsesSinceLastRead=%ld, totalPulsesNow=%ld, absVolume=%.3f liter",
             timeSinceLastRun,
             pulsesNow - mPulseCounts[(mBufferIndex - 1 + mBufferSize) % mBufferSize], // count of previous run (second last)
             pulsesNow,
             mVolume_liter);
    // Log flow calculation
    ESP_LOGD(TAG, "flow - flowMeasureDuration=%ld ms, flowRecordedPulses=%ld, flow=%.6f liter-per-second",
             msPassed,
             pulsesNow - pulsesPast,
             mFlow_literPerSecond);

    // update variables for next run
    mTimestampLastRead = esp_log_timestamp();
}
