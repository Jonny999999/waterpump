#pragma once

extern "C" {
#include "esp_log.h"
}

//TODO optimize this function (create template?)

//=================================
//===== scaleUsingLookupTable =====
//=================================
//scale/inpolate an input value to output value between several known points (two arrays)
//notes: 
//  - the lookup values must be in ascending order. 
//  - If the input value is lower/larger than smallest/largest value.
//    output is set to first/last element of output elements 
//    (be sure to add the min/max possible values)
float scaleUsingLookupTable(const float lookupInput[], const float lookupOutput[], int count, float input){
    static const char *TAG = "lookupTable";

    // check limit case (set to min/max)
	if (input <= lookupInput[0]) {
		ESP_LOGD(TAG, "lookup: %.2f is lower than lowest value -> returning min", input);
		return lookupOutput[0];
	} else if (input >= lookupInput[count -1]) {
		ESP_LOGD(TAG, "lookup: %.2f is larger than largest value -> returning max", input);
		return lookupOutput[count -1];
	}

	// find best matching range and
	// scale input linear to output in matched range
	for (int i = 1; i < count; ++i)
	{
		if (input <= lookupInput[i]) //best match
		{
			float voltageRange = lookupInput[i] - lookupInput[i - 1];
			float voltageOffset = input - lookupInput[i - 1];
			float percentageRange = lookupOutput[i] - lookupOutput[i - 1];
			float percentageOffset = lookupOutput[i - 1];
			float output = percentageOffset + (voltageOffset / voltageRange) * percentageRange;
			ESP_LOGD(TAG, "lookup: - input=%.3f => output=%.3f", input, output);
			ESP_LOGD(TAG, "lookup - matched range: %.2fV-%.2fV  => %.1f-%.1f", lookupInput[i - 1], lookupInput[i], lookupOutput[i - 1], lookupOutput[i]);
			return output;
		}
	}
	ESP_LOGE(TAG, "lookup - unknown range");
	return 0.0; //unknown range


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
float scaleUsingLookupTable(const float lookup2dArray[][2], int count, float input)
{
    //convert 2d array to 2 arrays
    float tmpIn[count];
    float tmpOut[count];
    for (int i = 0; i < count; i++)
    {
        tmpIn[i] = lookup2dArray[i][0];
        tmpOut[i] = lookup2dArray[i][1];
    }

    //call original function to lookup values
    return scaleUsingLookupTable(tmpIn, tmpOut, count, input);
}
