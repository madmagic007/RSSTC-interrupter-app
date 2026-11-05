
#include "PulseGenerator.h"
#include "driver/mcpwm_prelude.h"
#include <Arduino.h>

uint32_t PulseGenerator::periodToTicks(uint32_t bps) const {
    return TIMER_RESOLUTION / bps;
}

uint32_t PulseGenerator::pulseWidthToTicks(uint32_t onTimeUs) const {
    return onTimeUs;
}

bool PulseGenerator::begin(int pin, uint32_t bps, uint32_t onTimeUs,
                           uint32_t requestedMaxBps,
                           uint32_t requestedMaxOnTimeUs) {
    if (initialized || pin < 0) {
        return false;
    }

    if (requestedMaxBps < MIN_BPS || requestedMaxBps > 1000 ||
        requestedMaxOnTimeUs > 200 ||
        bps < MIN_BPS || bps > requestedMaxBps ||
        onTimeUs > requestedMaxOnTimeUs) {
        return false;
    }

    const uint32_t periodTicks = periodToTicks(bps);
    const uint32_t pulseWidthTicks = pulseWidthToTicks(onTimeUs);

    if (periodTicks == 0 || periodTicks > MAX_TIMER_PERIOD_TICKS ||
        pulseWidthTicks >= periodTicks) {
        return false;
    }

    mcpwm_timer_handle_t timer = nullptr;
    mcpwm_oper_handle_t oper = nullptr;
    mcpwm_cmpr_handle_t comparator = nullptr;
    mcpwm_gen_handle_t generator = nullptr;

    mcpwm_timer_config_t timerConfig = {};
    timerConfig.group_id = 0;
    timerConfig.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timerConfig.resolution_hz = TIMER_RESOLUTION;
    timerConfig.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
    timerConfig.period_ticks = periodTicks;
    timerConfig.flags.update_period_on_empty = true;

    if (mcpwm_new_timer(&timerConfig, &timer) != ESP_OK) {
        return false;
    }

    mcpwm_operator_config_t operatorConfig = {};
    operatorConfig.group_id = 0;

    if (mcpwm_new_operator(&operatorConfig, &oper) != ESP_OK) {
        return false;
    }

    if (mcpwm_operator_connect_timer(oper, timer) != ESP_OK) {
        return false;
    }

    mcpwm_comparator_config_t comparatorConfig = {};
    comparatorConfig.flags.update_cmp_on_tez = true;

    if (mcpwm_new_comparator(oper, &comparatorConfig, &comparator) != ESP_OK) {
        return false;
    }

    mcpwm_generator_config_t generatorConfig = {};
    generatorConfig.gen_gpio_num = pin;

    if (mcpwm_new_generator(oper, &generatorConfig, &generator) != ESP_OK) {
        return false;
    }

    const mcpwm_generator_action_t startAction =
        pulseWidthTicks == 0 ? MCPWM_GEN_ACTION_LOW : MCPWM_GEN_ACTION_HIGH;
    if (mcpwm_generator_set_action_on_timer_event(
        generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            startAction
        )
    ) != ESP_OK) {
        return false;
    }

    if (mcpwm_generator_set_action_on_compare_event(
        generator,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            comparator,
            MCPWM_GEN_ACTION_LOW
        )
    ) != ESP_OK) {
        return false;
    }

    if (mcpwm_comparator_set_compare_value(comparator, pulseWidthTicks) != ESP_OK) {
        return false;
    }

    if (mcpwm_timer_enable(timer) != ESP_OK) {
        return false;
    }

    if (onTimeUs != 0 &&
        mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP) != ESP_OK) {
        return false;
    }

    timerHandle = timer;
    operatorHandle = oper;
    comparatorHandle = comparator;
    generatorHandle = generator;

    outputPin = pin;
    currentBps = bps;
    currentOnTimeUs = onTimeUs;
    maxBps = requestedMaxBps;
    maxOnTimeUs = requestedMaxOnTimeUs;
    initialized = true;
    outputEnabled = onTimeUs != 0;

    return true;
}

bool PulseGenerator::enable() {
    if (!initialized || currentOnTimeUs == 0) {
        return false;
    }

    auto generator = static_cast<mcpwm_gen_handle_t>(generatorHandle);
    if (mcpwm_generator_set_action_on_timer_event(
        generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_HIGH
        )
    ) != ESP_OK) {
        return false;
    }

    if (outputEnabled) {
        return true;
    }

    auto timer = static_cast<mcpwm_timer_handle_t>(timerHandle);
    if (mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP) != ESP_OK) {
        return false;
    }

    outputEnabled = true;
    return true;
}

bool PulseGenerator::disable() {
    if (!initialized) {
        return false;
    }

    if (!outputEnabled) {
        return true;
    }

    auto generator = static_cast<mcpwm_gen_handle_t>(generatorHandle);
    if (mcpwm_generator_set_action_on_timer_event(
        generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_LOW
        )
    ) != ESP_OK) {
        return false;
    }

    auto timer = static_cast<mcpwm_timer_handle_t>(timerHandle);
    if (mcpwm_timer_start_stop(timer, MCPWM_TIMER_STOP_EMPTY) != ESP_OK) {
        return false;
    }

    outputEnabled = false;
    return true;
}

bool PulseGenerator::set(uint32_t bps, uint32_t onTimeUs) {
    if (!initialized) {
        return false;
    }

    if (bps < MIN_BPS || bps > maxBps || onTimeUs > maxOnTimeUs) {
        return false;
    }

    const uint32_t periodTicks = periodToTicks(bps);
    const uint32_t pulseWidthTicks = pulseWidthToTicks(onTimeUs);

    if (periodTicks == 0 || periodTicks > MAX_TIMER_PERIOD_TICKS ||
        pulseWidthTicks >= periodTicks) {
        return false;
    }

    auto timer = static_cast<mcpwm_timer_handle_t>(timerHandle);
    auto comparator = static_cast<mcpwm_cmpr_handle_t>(comparatorHandle);
    auto generator = static_cast<mcpwm_gen_handle_t>(generatorHandle);

    if (mcpwm_timer_set_period(timer, periodTicks) != ESP_OK) {
        return false;
    }

    if (mcpwm_comparator_set_compare_value(comparator, pulseWidthTicks) != ESP_OK) {
        return false;
    }

    const mcpwm_generator_action_t startAction =
        pulseWidthTicks == 0 ? MCPWM_GEN_ACTION_LOW : MCPWM_GEN_ACTION_HIGH;
    if (mcpwm_generator_set_action_on_timer_event(
        generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            startAction
        )
    ) != ESP_OK) {
        return false;
    }

    currentBps = bps;
    currentOnTimeUs = onTimeUs;

    return onTimeUs == 0 ? disable() : enable();
}

bool PulseGenerator::setBps(uint32_t bps) {
    return set(bps, currentOnTimeUs);
}

bool PulseGenerator::setOnTimeUs(uint32_t onTimeUs) {
    return set(currentBps, onTimeUs);
}