#include "esphome/core/log.h"
#include "samsung_ac.h"
#include "util.h"
#include "conversions.h"
#include <vector>
#include <set>
#include <algorithm>
#include "samsung_ac_device_custClim.h"

namespace esphome
{
  namespace samsung_ac
  {
    climate::ClimateTraits Samsung_AC_Climate::traits()
    {
      climate::ClimateTraits traits;

      traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
      traits.set_visual_temperature_step(1.0f);
      traits.set_visual_min_temperature(16.0f);
      traits.set_visual_max_temperature(30.0f);

      traits.set_supported_modes({
          climate::CLIMATE_MODE_OFF,
          climate::CLIMATE_MODE_AUTO,
          climate::CLIMATE_MODE_COOL,
          climate::CLIMATE_MODE_DRY,
          climate::CLIMATE_MODE_FAN_ONLY,
          climate::CLIMATE_MODE_HEAT});

      traits.set_supported_fan_modes({
          climate::CLIMATE_FAN_HIGH,
          climate::CLIMATE_FAN_MIDDLE,
          climate::CLIMATE_FAN_LOW,
          climate::CLIMATE_FAN_AUTO});

      traits.set_supported_swing_modes({
          climate::CLIMATE_SWING_OFF,
          climate::CLIMATE_SWING_HORIZONTAL,
          climate::CLIMATE_SWING_VERTICAL,
          climate::CLIMATE_SWING_BOTH});
      return traits;
    }


    void Samsung_AC_Climate::control(const climate::ClimateCall &call)
    {
      traits();

      ProtocolRequest request;

      auto targetTempOpt = call.get_target_temperature();
      if (targetTempOpt.has_value())
        request.target_temp = targetTempOpt.value();

      auto modeOpt = call.get_mode();
      if (modeOpt.has_value())
      {
        if (modeOpt.value() == climate::ClimateMode::CLIMATE_MODE_OFF)
        {
          request.power = false;
        }
        else
        {
          request.mode = climatemode_to_mode(modeOpt.value());
        }
      }

      auto fanmodeOpt = call.get_fan_mode();
      if (fanmodeOpt.has_value())
      {
        request.fan_mode = climatefanmode_to_fanmode(fanmodeOpt.value());
      }

      auto customFanmode = call.get_custom_fan_mode();
      const char *fanmode_str = customFanmode.c_str();
      if (fanmode_str != nullptr && fanmode_str[0] != '\0') {
        request.fan_mode = customfanmode_to_fanmode(fanmode_str);
      }

      auto presetOpt = call.get_preset();
      if (presetOpt.has_value())
      {
        set_alt_mode_by_name(request, preset_to_altmodename(presetOpt.value()));
      }

      auto customPreset = call.get_custom_preset();
      const char *preset_str = customPreset.c_str();
      if (preset_str != nullptr && preset_str[0] != '\0') {
        auto preset = altmodename_to_preset(preset_str);
        if (preset.has_value()) {
         request.alt_mode = (AltMode) *preset;
          }
      }

      auto swingModeOpt = call.get_swing_mode();
      if (swingModeOpt.has_value())
      {
        request.swing_mode = climateswingmode_to_swingmode(swingModeOpt.value());
      }

      device->publish_request(request);
    }


    void Samsung_AC_Climate::set_alt_mode_by_name(ProtocolRequest &request, const AltModeName &name)
    {
      auto supported = device->get_supported_alt_modes();
      auto mode = std::find_if(supported->begin(), supported->end(), [&name](const AltModeDesc &x)
                               { return x.name == name; });
      if (mode == supported->end())
      {
        ESP_LOGW(TAG, "Unsupported alt_mode %s", name.c_str());
        return;
      }
      request.alt_mode = mode->value;
    }

    void Samsung_AC_Device::getValueForCustomClimate(uint16_t address, long value) {
      for (auto& cc: custom_climates) {
        if (address == cc->modeAddr && cc->modeAddr) {
          cc->lastReadMode = value;
          cc->publishMode();
        } else if (address == cc->enable) {
          cc->lastEnabled = value;
          cc->publishMode();
        } else if (address == cc->presAddr &&  cc->presAddr) {
          cc->lastReadPres = value;
          cc->publishMode();
        } else if (address == cc->set) {
          float tempe = (float)value / 10.0;
          cc->target_temperature = tempe;
          cc->publish_state();
          ESP_LOGV(TAG, "CC changed setpoint, read %f for addr %x", tempe, address);
        } else if (address == cc->status) {
          float tempe = (float)value / 10.0;
          cc->current_temperature = tempe;
          cc->publish_state();
          ESP_LOGV(TAG, "CC changed status, read %f for addr %x", tempe, address);
        }
      }
    }

  } // namespace samsung_ac
} // namespace esphome
