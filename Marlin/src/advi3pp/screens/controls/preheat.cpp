/**
 * ADVi3++ Firmware For Wanhao Duplicator i3 Plus (based on Marlin 2)
 *
 * Copyright (C) 2017-2020 Sebastien Andrivet [https://github.com/andrivet/]
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../../parameters.h"
#include "preheat.h"
#include "../../core/core.h"
#include "../../core/dgus.h"
#include "../../core/status.h"
#include "../print/temperatures.h"

namespace ADVi3pp {

Preheat preheat;

//! Handle Preheat actions.
//! @param key_value    Sub-action to handle
//! @return             True if the action was handled
bool Preheat::do_dispatch(KeyValue key_value)
{
    if(Parent::do_dispatch(key_value))
        return true;

    switch(key_value)
    {
        case KeyValue::PresetPrevious:  previous_command(); break;
        case KeyValue::PresetNext:      next_command(); break;
        case KeyValue::Cooldown:        cooldown_command(); break;
        default:                        return false;
    }

    return true;
}

//! Send the presets t the LCD Panel
void Preheat::send_presets()
{
    Log::log() << F("Preheat page") << Log::endl();
    WriteRamDataRequest frame{Variable::Value0};
    frame << Uint16(ExtUI::getMaterialPresetHotendTemp_celsius(index_))
          << Uint16(ExtUI::getMaterialPresetBedTemp_celsius(index_))
          << Uint16(ExtUI::getMaterialPresetFanSpeed_percent(index_));
    frame.send();

    ADVString<8> preset;
    preset << index_ + 1 << F(" / ") << NB_PRESETS;
    frame.reset(Variable::ShortText0);
    frame << preset;
    frame.send();
}

//! Retrieve presets values from the LCD Panel
void Preheat::retrieve_presets()
{
    ReadRamData frame{Variable::Value0, 3};
    if(!frame.send_and_receive())
    {
        Log::error() << F("Error receiving presets") << Log::endl();
        return;
    }

    Uint16 hotend, bed, fan;
    frame >> hotend >> bed >> fan;

    ExtUI::setMaterialPreset(index_, hotend.word, bed.word, fan.word);
}

//! Prepare the page before being displayed and return the right Page value
//! @return The index of the page to display
Page Preheat::do_prepare_page()
{
    if(!core.ensure_not_printing())
        return Page::None;
    send_presets();
    return Page::Preheat;
}

//! Handle Previous command
void Preheat::previous_command()
{
    if(index_ <= 0)
        return;
    retrieve_presets();
    --index_;
    send_presets();
}

//! Handle Next command
void Preheat::next_command()
{
    if(index_ >= NB_PRESETS - 1)
        return;
    retrieve_presets();
    ++index_;
    send_presets();
}

//! Handles the Save (Continue) command
void Preheat::do_save_command()
{
    retrieve_presets();

    ExtUI::setTargetTemp_celsius(ExtUI::getMaterialPresetHotendTemp_celsius(index_), ExtUI::E0);
    ExtUI::setTargetTemp_celsius(ExtUI::getMaterialPresetBedTemp_celsius(index_), ExtUI::BED);
    ExtUI::setTargetFan_percent(ExtUI::getMaterialPresetFanSpeed_percent(index_), ExtUI::FAN0);

    settings.save();
    status.set(F("Preheat..."));
    temperatures.show(ShowOptions::None);
}

//! Cooldown the bed and the nozzle, turn off the fan
void Preheat::cooldown_command()
{
    if(!core.ensure_not_printing())
        return;

    Log::log() << F("Cooldown") << Log::endl();
    status.set(F("Cooldown"));
    ExtUI::finishAndDisableHeaters();
}

}