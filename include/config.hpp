#pragma once
#include <yaml-cpp/yaml.h>

struct config {

    struct {
        double execution_time;
    } common;

    struct {
        double period_ns;
        double duty_cycle;
        double start_delay_ns;
    } clock;

};

class config_loader {
public:
    config cfg;

    void load_yaml (const std::string& path = "./config.yaml") {
        YAML::Node config = YAML::LoadFile(path);

        // --- common
        cfg.common.execution_time = config["common"]["execution_time"].as<double>();

        // --- clock
        cfg.clock.period_ns      = config["clock"]["period_ns"].as<double>();
        cfg.clock.duty_cycle     = config["clock"]["duty_cycle"].as<double>();
        cfg.clock.start_delay_ns = config["clock"]["start_delay_ns"].as<double>();
    }
};