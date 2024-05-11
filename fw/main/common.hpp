#pragma once

extern "C" {
#include "esp_log.h"
}

//=================================
//===== scaleUsingLookupTable =====
//=================================
//scale/interpolate an input value to output value between several known points (two arrays)
//notes: 
//  - the lookup values must be in ascending order. 
//  - If the input value is lower/larger than smallest/largest value
//    the output is set to first/last element of output elements 
//    (be sure to add the min/max possible values)
template <typename T_IN, typename T_OUT>
T_OUT scaleUsingLookupTable(const T_IN lookupInput[], const T_OUT lookupOutput[], int count, T_IN input){
    static const char *TAG = "lookupTable";

    // check limit case (set to min/max)
	if (input <= lookupInput[0]) {
		ESP_LOGD(TAG, "lookup: %.2f is lower than lowest value -> returning min", (float)input);
		return lookupOutput[0];
	} else if (input >= lookupInput[count -1]) {
		ESP_LOGD(TAG, "lookup: %.2f is larger than largest value -> returning max", (float)input);
		return lookupOutput[count -1];
	}

	// find best matching range and
	// scale input linear to output in matched range
	for (int i = 1; i < count; ++i)
	{
		if (input <= lookupInput[i]) //best match
		{
			T_IN inputRange = lookupInput[i] - lookupInput[i - 1];
			T_IN inputOffset = input - lookupInput[i - 1];
			T_OUT outputRange = lookupOutput[i] - lookupOutput[i - 1];
			T_OUT outputOffset = lookupOutput[i - 1];
			T_OUT output = outputOffset + (inputOffset / inputRange) * outputRange;
			ESP_LOGD(TAG, "lookup: - input=%.3f => output=%.3f", (float)input, (float)output);
			ESP_LOGD(TAG, "lookup - matched range: %.2fV-%.2fV  => %.1f-%.1f", (float)lookupInput[i - 1], (float)lookupInput[i], (float)lookupOutput[i - 1], (float)lookupOutput[i]);
			return output;
		}
	}
	ESP_LOGE(TAG, "lookup - unknown range");
	return 0; //unknown range
}


//======================================
//=== scaleUsingLookupTable 2d-array ===
//======================================
//example array:
//const float lookupPSensor[][2] = {
//    {1234, 4.1},
//    {1234, 4.1}
//};
//int count = sizeof(lookupPSensor) / sizeof(lookupPSensor[0])
template <typename T>
T scaleUsingLookupTable(const T lookup2dArray[][2], int count, T input)
{
    //convert 2d array to 2 arrays
    T tmpIn[count];
    T tmpOut[count];
    for (int i = 0; i < count; i++)
    {
        tmpIn[i] = lookup2dArray[i][0];
        tmpOut[i] = lookup2dArray[i][1];
    }

    //call original function to lookup values
    return scaleUsingLookupTable<T, T>(tmpIn, tmpOut, count, input);
}


//==========================
//====== limitToRange ======
//==========================
// limit a value to a certain range
template <typename T>
T limitToRange(T value, T min, T max)
{
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}