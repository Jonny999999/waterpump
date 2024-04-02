idf_component_register(
    SRCS 
        "app_main.c"
        "sensordeklaration.c"
        "mqtt.c"
        "digitalsensoren.c"
        "analogsensoren.c"
        "wasserpumpe.c"
    INCLUDE_DIRS 
        "."
    )

# colored build output (errors, warnings...)
idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)
