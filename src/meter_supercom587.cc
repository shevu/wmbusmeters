/*
 Copyright (C) 2017-2019 Fredrik Öhrström

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"dvparser.h"
#include"meters.h"
#include"meters_common_implementation.h"
#include"wmbus.h"
#include"wmbus_utils.h"
#include"util.h"

using namespace std;

struct MeterSupercom587 : public virtual WaterMeter, public virtual MeterCommonImplementation {
    MeterSupercom587(WMBus *bus, string& name, string& id, string& key);

    // Total water counted through the meter
    double totalWaterConsumption(Unit u);
    bool  hasTotalWaterConsumption();

private:
    void processContent(Telegram *t);

    double total_water_consumption_m3_ {};
};

unique_ptr<WaterMeter> createSupercom587(WMBus *bus, string& name, string& id, string& key)
{
    return unique_ptr<WaterMeter>(new MeterSupercom587(bus,name,id,key));
}

MeterSupercom587::MeterSupercom587(WMBus *bus, string& name, string& id, string& key) :
    MeterCommonImplementation(bus, name, id, key, MeterType::SUPERCOM587, MANUFACTURER_SON, LinkMode::T1)
{
    setEncryptionMode(EncryptionMode::AES_CBC);

    addMedia(0x06);
    addMedia(0x07);

    setExpectedVersion(0x3c);

    addPrint("total", Quantity::Volume,
             [&](Unit u){ return totalWaterConsumption(u); },
             "The total water consumption recorded by this meter.",
             true, true);

    MeterCommonImplementation::bus()->onTelegram(calll(this,handleTelegram,Telegram*));
}

void MeterSupercom587::processContent(Telegram *t)
{
    map<string,pair<int,DVEntry>> values;
    parseDV(t, t->content, t->content.begin(), t->content.size(), &values);

    int offset;
    string key;

    if(findKey(ValueInformation::Volume, 0, &key, &values)) {
        extractDVdouble(&values, key, &offset, &total_water_consumption_m3_);
        t->addMoreExplanation(offset, " total consumption (%f m3)", total_water_consumption_m3_);
    }
}

double MeterSupercom587::totalWaterConsumption(Unit u)
{
    assertQuantity(u, Quantity::Volume);
    return convert(total_water_consumption_m3_, Unit::M3, u);
}

bool MeterSupercom587::hasTotalWaterConsumption()
{
    return true;
}
