// all used mqtt topics are collected in this file for better overview and easy adjustments
// note: mostly used in mqtt.cpp


//==============================
//====== Published topics ======
//==============================
// publish status
#define MQTT_TOPIC__PRESSURE_BAR    "waterpump/pressure_bar"
#define MQTT_TOPIC__PRESSURE_DIFF   "waterpump/valve/pidStats/pressureDiff"
#define MQTT_TOPIC__TARGET_PRESSURE "waterpump/valve/pidStats/targetPressure"
#define MQTT_TOPIC__MOTOR_LEVEL     "waterpump/status/motorLevel"
#define MQTT_TOPIC__VALVE_PER       "waterpump/valve/valvePer"
#define MQTT_TOPIC__P_VALUE         "waterpump/valve/pidStats/p"
#define MQTT_TOPIC__I_VALUE         "waterpump/valve/pidStats/i"
#define MQTT_TOPIC__D_VALUE         "waterpump/valve/pidStats/d"
#define MQTT_TOPIC__FLOW_LITER_PER_MINUTE "waterpump/flowRate_literPerMinute"
#define MQTT_TOPIC__VOLUME_LITER    "waterpump/volume_liter"

// publish current control settings
#define MQTT_TOPIC__KP              "waterpump/valve/getSettings/Kp"
#define MQTT_TOPIC__KI              "waterpump/valve/getSettings/Ki"
#define MQTT_TOPIC__KD              "waterpump/valve/getSettings/Kd"
#define MQTT_TOPIC__OFFSET          "waterpump/valve/getSettings/offset"
#define MQTT_TOPIC__ACCEPTABLE_DIFF "waterpump/valve/getSettings/acceptableDiff"



//===============================
//====== subscribed topics ======
//===============================
// receive configuration
#define MQTT_TOPIC__SET_KP "waterpump/valve/setKp"
#define MQTT_TOPIC__SET_KI "waterpump/valve/setKi"
#define MQTT_TOPIC__SET_KD "waterpump/valve/setKd"
#define MQTT_TOPIC__SET_OFFSET  "waterpump/valve/setOffset"
#define MQTT_TOPIC__SET_ACCEPTABLE_DIFF "waterpump/valve/setAcceptableDiff"

// control pump
#define MQTT_TOPIC__RESET "waterpump/valve/reset"
#define MQTT_TOPIC__START "waterpump/start"
#define MQTT_TOPIC__STOP "waterpump/stop"
#define MQTT_TOPIC__SET_TARGET_PRESSURE "waterpump/setTargetPressure"

// data requests
#define MQTT_TOPIC__REQUEST_PARAMMETERS "waterpump/requestParameters"
