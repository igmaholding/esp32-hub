#ifdef INCLUDE_MAINSPROBE
#include <ArduinoJson.h>
#include <mainsProbe.h>
#include <autonom.h>
#include <gpio.h>
#include <binarySemaphore.h>
#include <mapTable.h>
#include <Wire.h>
#include <deque>
#include <epromImage.h>
#include <sstream>
#include <i2c_utils.h>
#include <PolyFitNasa.h>

extern GpioHandler gpioHandler;


static void _err_dup(const char *name, int value)
{
    ERROR("%s %d is duplicated / reused", name, value)
}

static void _err_cap(const char *name, int value)
{
    ERROR("%s %d, gpio doesn't have required capabilities", name, value)
}

static void _err_val(const char *name, int value)
{
    ERROR("%s %d incorrect", name, value)
}

bool MainsProbeConfig::is_valid() const
{
    GpioCheckpad checkpad;

    if (i2c.is_valid())
    {
        // only check I2c gpio if i2c is valid since it might not be needed and not defined
   
        String object_name = "i2c.scl.gpio";

        if (checkpad.get_usage(i2c.scl.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)i2c.scl.gpio);
            return false;
        }

        if (!checkpad.set_usage(i2c.scl.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)i2c.scl.gpio);
            return false;
        }

        object_name = "i2c.sda.gpio";

        if (checkpad.get_usage(i2c.sda.gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name.c_str(), (int)i2c.sda.gpio);
            return false;
        }

        if (!checkpad.set_usage(i2c.sda.gpio, GpioCheckpad::uDigitalOutput))
        {
            _err_cap(object_name.c_str(), (int)i2c.sda.gpio);
            return false;
        }
    }

    std::vector<uint8_t> used_i2c_addr;

    size_t i = 0;

    for (auto it = input_v.begin(); it != input_v.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("input_v[%d] is_valid() == false", (int) i)
            return false;
        }

        if (std::find(used_i2c_addr.begin(), used_i2c_addr.end(), it->addr) != used_i2c_addr.end())
        {
            ERROR("input_v[%d] duplicated i2c address %s", (int) i, i2c_addr_uint8_t_to_str(it->addr).c_str())
            return false;
        }

        used_i2c_addr.push_back(it->addr);
    }

    // the i2c addresses are shared by all, so continue with the same used_i2c_addr 

    i=0;

    for (auto it = input_a_high.begin(); it != input_a_high.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("input_a_high[%d] is_valid() == false", (int) i)
            return false;
        }

        if (std::find(used_i2c_addr.begin(), used_i2c_addr.end(), it->addr) != used_i2c_addr.end())
        {
            ERROR("input_a_high[%d] duplicated i2c address %s", (int) i, i2c_addr_uint8_t_to_str(it->addr).c_str())
            return false;
        }

        used_i2c_addr.push_back(it->addr);
    }

    i=0;

    for (auto it = input_a_low_channels.begin(); it != input_a_low_channels.end(); ++it, ++i)
    {
        if (it->is_valid() == false)
        {
            ERROR("input_a_low_channel %d is_valid() == false", (int) i)
            return false;
        }

        char object_name[64];
        sprintf(object_name, "input_a_low_channel[%d]", (int) i);

        if (checkpad.get_usage(it->gpio) != GpioCheckpad::uNone)
        {
            _err_dup(object_name, (int)it->gpio);
            return false;
        }

        if (!checkpad.set_usage(it->gpio, GpioCheckpad::uAnalogInput))
        {
            _err_cap(object_name, (int)it->gpio);
            return false;
        }
    }
    
    i=0;

    for (auto it = applets.begin(); it != applets.end(); ++it, ++i)
    {
        if (it->second.is_valid() == false)
        {
            ERROR("applet %s is_valid() == false", it->first.c_str())
            return false;
        }
        else
        {
        }
    }

    return true;
}


void MainsProbeConfig::from_json(const JsonVariant &json)
{
    //DEBUG("mains-probe config from_json")
    clear();

    if (json.containsKey("i2c"))
    {
        const JsonVariant &_json = json["i2c"];
        i2c.from_json(_json);
    }

    if (json.containsKey("input_v"))
    {
        //DEBUG("contains key input_v")
        const JsonVariant &_json = json["input_v"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("input_v is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                //DEBUG("input_v item")
                const JsonVariant & __json = *iterator;

                Ina3221Config ina3221_config;
                ina3221_config.from_json(__json);
                input_v.push_back(ina3221_config);
                //DEBUG("ina3221_config from_json, %s", ina3221_config.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("input_a_high"))
    {
        const JsonVariant &_json = json["input_a_high"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                Ina3221Config ina3221_config;
                ina3221_config.from_json(__json);
                input_a_high.push_back(ina3221_config);
                //DEBUG("ina3221_config from_json, %s", ina3221_config.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("input_a_low_channels"))
    {
        const JsonVariant &_json = json["input_a_low_channels"];

        if (_json.is<JsonArray>())
        {
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                InputChannel input_channel;
                input_channel.from_json(__json);
                input_a_low_channels.push_back(input_channel);
                //DEBUG("Channel from_json, %s", input_channel.as_string().c_str())

                ++iterator;
            }
        }
    }

    if (json.containsKey("applets"))
    {
        //DEBUG("contains key applets")
        const JsonVariant &_json = json["applets"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("applets is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                //DEBUG("analysing applets item")
                const JsonVariant & __json = *iterator;

                if (__json.containsKey("name"))
                {
                    String name = __json["name"]; 
                    Applet applet;
                    applet.from_json(__json);
                    applets.insert(std::make_pair(name,applet));
                    DEBUG("applet from_json: name=%s, %s", name.c_str(), applet.as_string().c_str())
                }
                ++iterator;
            }
        }
    }
}

void MainsProbeConfig::to_eprom(std::ostream &os) const
{
    os.write((const char *)&EPROM_VERSION, sizeof(EPROM_VERSION));

    i2c.to_eprom(os);

    uint8_t count = (uint8_t)input_v.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_v.begin(); it != input_v.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)input_a_high.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_a_high.begin(); it != input_a_high.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)input_a_low_channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = input_a_low_channels.begin(); it != input_a_low_channels.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = (uint8_t)applets.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = applets.begin(); it != applets.end(); ++it)
    {
        uint8_t len = it->first.length();
        os.write((const char *)&len, sizeof(len));
        os.write((const char *)it->first.c_str(), len);

        it->second.to_eprom(os);
    }
}

bool MainsProbeConfig::from_eprom(std::istream &is)
{
    uint8_t eprom_version = EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    if (eprom_version == EPROM_VERSION)
    {
        clear();

        i2c.from_eprom(is);

        uint8_t count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            Ina3221Config ina3221_config;
            ina3221_config.from_eprom(is);
            input_v.push_back(ina3221_config);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            Ina3221Config ina3221_config;
            ina3221_config.from_eprom(is);
            input_a_high.push_back(ina3221_config);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannel input_channel;
            input_channel.from_eprom(is);
            input_a_low_channels.push_back(input_channel);
        }

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            uint8_t len = 0;
            is.read((char *)&len, sizeof(len));

            if (len)
            {
                char buf[256];
                is.read(buf, len);
                buf[len] = 0;
                String name = buf;

                Applet applet;
                applet.from_eprom(is);
                applets[name] = applet;

            }
        }
        return is_valid() && !is.bad();
    }
    else
    {
        ERROR("Failed to read MainsProbeConfig from EPROM: version mismatch, expected %d, found %d", (int)EPROM_VERSION, (int)eprom_version)
        return false;
    }
}

void MainsProbeConfig::I2c::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("scl"))
    {
        //DEBUG("MultiConfig::I2c::from_json contains scl")
        const JsonVariant &_json = json["scl"];

        if (_json.containsKey("channel"))
        {
            //DEBUG("MultiConfig::I2c::from_json scl contains channel")
            const JsonVariant &__json = _json["channel"];
            scl.from_json(__json);
            //DEBUG("scl after from_json %s", scl.as_string().c_str())
        }
    }
    if (json.containsKey("sda"))
    {
        //DEBUG("MultiConfig::I2c::from_json contains sda")
        const JsonVariant &_json = json["sda"];

        if (_json.containsKey("channel"))
        {
            //DEBUG("MultiConfig::I2c::from_json sda contains channel")
            const JsonVariant &__json = _json["channel"];
            sda.from_json(__json);
            //DEBUG("sda after from_json %s", sda.as_string().c_str())
        }
    }
}

void MainsProbeConfig::I2c::to_eprom(std::ostream &os) const
{
    scl.to_eprom(os);
    sda.to_eprom(os);
}

bool MainsProbeConfig::I2c::from_eprom(std::istream &is)
{
    scl.from_eprom(is);
    sda.from_eprom(is);

    return is_valid() && !is.bad();
}

void MainsProbeConfig::InputChannelIna3221::from_json(const JsonVariant &json)
{
    //DEBUG("InputChannelIna3221::from_json")
    clear();    

    if (json.containsKey("channel"))
    {
        channel = (int) json["channel"];
        //DEBUG("contains channel, %d", (int) channel)
    }

    if (json.containsKey("default_poly_beta"))
    {
        const JsonVariant &_json = json["default_poly_beta"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("default_poly_beta is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;
                float value = __json;
                default_poly_beta.push_back(value);

                ++iterator;
            }
        }
    }
}

void MainsProbeConfig::InputChannelIna3221::to_eprom(std::ostream &os) const
{
    os.write((const char *)&channel, sizeof(channel));

    size_t count = default_poly_beta.size();
    os.write((const char *)&count, sizeof(count));

    for (size_t i=0;i<default_poly_beta.size(); ++i)
    {
        os.write((const char *)&default_poly_beta[i], sizeof(default_poly_beta[i]));
    }
}

bool MainsProbeConfig::InputChannelIna3221::from_eprom(std::istream &is)
{
    clear();
    is.read((char *)&channel, sizeof(channel));

    size_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0;i<count; ++i)
    {
        float value = 0;
        is.read((char *)&value, sizeof(value));
        default_poly_beta.push_back(value);
    }

    return is_valid() && !is.bad();
}

void MainsProbeConfig::Ina3221Config::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("addr"))
    {
        String addr_str = json["addr"];
        addr = i2c_addr_str_to_uint8_t(addr_str);
    }

    if (json.containsKey("channels"))
    {
        //DEBUG("contains key channels")
        const JsonVariant &_json = json["channels"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("channels is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;

                InputChannelIna3221 channel;
                channel.from_json(__json);
                channels.push_back(channel);
                //DEBUG("channel from_json, %s", channel.as_string().c_str())

                ++iterator;
            }
        }
    }
}

void MainsProbeConfig::Ina3221Config::to_eprom(std::ostream &os) const
{
    os.write((const char *)&addr, sizeof(addr));

    uint8_t count = (uint8_t)channels.size();
    os.write((const char *)&count, sizeof(count));

   for (auto it = channels.begin(); it != channels.end(); ++it)
    {
        it->to_eprom(os);
    }

}

bool MainsProbeConfig::Ina3221Config::from_eprom(std::istream &is)
{
    clear();

    is.read((char *)&addr, sizeof(addr));

    uint8_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0; i<count; ++i)
    {
        InputChannelIna3221 channel;
        channel.from_eprom(is);
        channels.push_back(channel);
    }
    return is_valid() && !is.bad();
}

void MainsProbeConfig::InputChannel::from_json(const JsonVariant &json)
{
    clear();    
    AnalogInputChannelConfig::from_json(json);

    if (json.containsKey("default_poly_beta"))
    {
        const JsonVariant &_json = json["default_poly_beta"];

        if (_json.is<JsonArray>())
        {
            //DEBUG("default_poly_beta is array")
            const JsonArray & jsonArray = _json.as<JsonArray>();
            auto iterator = jsonArray.begin();

            while(iterator != jsonArray.end())
            {
                const JsonVariant & __json = *iterator;
                float value = __json;
                default_poly_beta.push_back(value);

                ++iterator;
            }
        }
    }
}

void MainsProbeConfig::InputChannel::to_eprom(std::ostream &os) const
{
    AnalogInputChannelConfig::to_eprom(os);

    size_t count = default_poly_beta.size();
    os.write((const char *)&count, sizeof(count));

    for (size_t i=0;i<default_poly_beta.size(); ++i)
    {
        os.write((const char *)&default_poly_beta[i], sizeof(default_poly_beta[i]));
    }
}

bool MainsProbeConfig::InputChannel::from_eprom(std::istream &is)
{
    clear();
    AnalogInputChannelConfig::from_eprom(is);

    size_t count = 0;
    is.read((char *)&count, sizeof(count));

    for (size_t i=0;i<count; ++i)
    {
        float value = 0;
        is.read((char *)&value, sizeof(value));
        default_poly_beta.push_back(value);
    }

    return is_valid() && !is.bad();
}


void MainsProbeConfig::Applet::from_json(const JsonVariant &json)
{
    clear();

    if (json.containsKey("function"))
    {
        String function_str = json["function"];
        function = str_2_function(function_str.c_str());
    }
}

void MainsProbeConfig::Applet::to_eprom(std::ostream &os) const
{
    os.write((const char *)&function, sizeof(function));
}

bool MainsProbeConfig::Applet::from_eprom(std::istream &is)
{
    clear();

    is.read((char *)&function, sizeof(function));

    return is_valid() && !is.bad();
}


class MainsProbeHandler
{
public:

    static const int DATA_EPROM_VERSION = 1;

    class InputChannelData
    {
    public:
        // this class can handle mapping table from 1 item; in this case the other point is assumed to be [0,0] and
        // approximation of 1-st order is done
        //
        // otherwise the approximation order is amount of points in the mapping table-1, but highest MAX_POLY_ORDER
        //
        // this way even minimum calibration with one point will give a feasible result; the accuracy will increase
        // when adding new points both because of more input material and also because the order of polynom will increase

        static const uint8_t X_2_Y_MAP_MAX_SIZE = 5;
        static const int MAX_POLY_ORDER = 3;

        InputChannelData() 
        {
            clear();
        }

        void clear() 
        {
            addr = 0;    
            index = -1;  

            uncalibrate(); 
        }

        void to_eprom(std::ostream &os) const
        {
            os.write((const char *)&addr, sizeof(addr));
            os.write((const char *)&index, sizeof(index));
            os.write((const char *)x_2_y_map, sizeof(x_2_y_map));
        }

        bool from_eprom(std::istream &is)
        {
            clear();
            is.read((char *)&addr, sizeof(addr));
            is.read((char *)&index, sizeof(index));
            is.read((char *)x_2_y_map, sizeof(x_2_y_map));

            recalc_poly_beta();

            return !is.bad();
        }

        void to_json(JsonVariant & json)
        {
            if (addr == 0)
            {
                json["addr"] = "";
            }
            else
            {
                json["addr"] = String("0x") + String(addr, 16);
            }
            
            json["index"] = index;

            {json.createNestedArray("x_2_y_map");
            JsonArray jsonArray = json["x_2_y_map"];

            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                jsonArray.add(x_2_y_map[i][0]);
                jsonArray.add(x_2_y_map[i][1]);
            }}

            if (has_poly_beta())
            {
                json.createNestedArray("poly_beta");
                JsonArray jsonArray = json["poly_beta"];
    
                for (size_t i=0;i<MAX_POLY_ORDER+1;++i)
                {
                    jsonArray.add(poly_beta[i]);
                }    
            }
        }

        bool from_json(const JsonVariant & json)
        {
            uint8_t _addr = 0;

            if (json.containsKey("addr"))
            {
                const char * addr_str = json["addr"];
                _addr = i2c_addr_str_to_uint8_t(addr_str);
            }
            else
            {
                return false;
            }            

            int _index = -1;

            if (json.containsKey("index"))
            {
                _index = json["index"];
            }
            else
            {
                return false;
            }
            
            if (json.containsKey("x_2_y_map"))
            {
                addr = _addr;
                index = _index;

                uncalibrate();

                //DEBUG("contains key x_2_y_map")
                const JsonVariant &_json = json["x_2_y_map"];
    
                if (_json.is<JsonArray>())
                {
                    //DEBUG("x_2_y_map is array")
                    const JsonArray & jsonArray = _json.as<JsonArray>();
                    auto iterator = jsonArray.begin();
    
                    size_t i=0;

                    while(iterator != jsonArray.end() && i<X_2_Y_MAP_MAX_SIZE)
                    {
                        //DEBUG("input_v item")
                        const JsonVariant & __json = *iterator;
                        float x = __json;

                        ++iterator;

                        if (iterator != jsonArray.end())
                        {
                            const JsonVariant &___json = *iterator;
                            float y = ___json;
                        
                            if (x > 0)
                            {
                                x_2_y_map[i][0] = x;
                                x_2_y_map[i][1] = y;
                            }
                        }
        
                        ++iterator;
                        ++i;
                    }

                    sort_x_2_y_map();
                    recalc_poly_beta();
                }
            }
            else
            {
                return false;
            }

            return true;
        }

        void debug_x_2_y_map()
        {
            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                DEBUG("[%.2f, %.2f]", x_2_y_map[i][0], x_2_y_map[i][1])
            }
        }

        void debug_poly_beta()
        {
            for (size_t i=0;i<MAX_POLY_ORDER+1;++i)
            {
                DEBUG("[%d]=%.6f", (int) i, poly_beta[i]);
            }
        }

        void debug()
        {
            DEBUG("addr 0x%02x, index %d", (int) addr, (int) index)
            debug_x_2_y_map();
        }

        void clear_poly_beta()
        {
            _has_poly_beta = false;

            for (size_t i=0; i<sizeof(poly_beta)/sizeof(poly_beta[0]); ++i)
            {
                poly_beta[i] = 0;
            }

            _poly_order = 0;
        }

        void uncalibrate()
        {
            uncalibrate(& (x_2_y_map[0][0]));
            clear_poly_beta();
        }

        static void uncalibrate(float * _x_2_y_map)
        {
            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE*2;++i)
            {
                _x_2_y_map[i] = 0;    
            }
        }

        void add_calibration_point(float x, float y)
        {
            if (x <= 0)
            {
                return;
            }

            // first try to find a point with the same x, if found - replace

            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                if (x_2_y_map[i][0] == x)
                {
                    x_2_y_map[i][1] = y;
                    recalc_poly_beta();
                    return;
                }
            }

            // then try to find an empty cell 

            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                if (x_2_y_map[i][0] == 0)
                {
                    x_2_y_map[i][0] = x;
                    x_2_y_map[i][1] = y;
                    sort_x_2_y_map();
                    recalc_poly_beta();
                    return;
                }
            }

            // if no empty cells left - replace a cell with the closest x value

            float delta_x = 0;
            size_t delta_i = -1;

            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                // do not need to check for x_2_y_map[i][0]!=0, the above should exclude it

                float new_delta_x = x>x_2_y_map[i][0] ? x-x_2_y_map[i][0] : x_2_y_map[i][0]-x; 

                if (delta_i == -1 || new_delta_x < delta_x)
                {
                    delta_x = new_delta_x;
                    delta_i = i;
                }
            }

            if (delta_i != -1)  // just in case, shouldn't be
            {
                x_2_y_map[delta_i][0] = x;
                x_2_y_map[delta_i][1] = y;
                // sort shouldn't be needed as well?
            }
            recalc_poly_beta();
        }

        void sort_x_2_y_map()
        {
            float new_x_to_y_map[X_2_Y_MAP_MAX_SIZE][2];
            uncalibrate(& (new_x_to_y_map[0][0]));  

            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                float min_x = 0;
                size_t min_j = -1;

                for (size_t j=0;j<X_2_Y_MAP_MAX_SIZE;++j)
                {
                    if (x_2_y_map[j][0] > 0)
                    {
                        if (min_j == -1) 
                        {
                            min_x = x_2_y_map[j][0]; 
                            min_j = j;
                        }
                        else
                        {
                            if (x_2_y_map[j][0] < min_x)
                            {
                                min_x = x_2_y_map[j][0]; 
                                min_j = j;
                            }
                        }
                    }
                }

                if (min_j == -1)
                {
                    break;
                }

                new_x_to_y_map[i][0] = x_2_y_map[min_j][0];
                new_x_to_y_map[i][1] = x_2_y_map[min_j][1];
                x_2_y_map[min_j][0] = 0;
            }

            memcpy(x_2_y_map, new_x_to_y_map, sizeof(x_2_y_map));
        }

        size_t get_num_calibration_points() const
        {
            size_t c = 0;
            
            for (size_t i=0;i<X_2_Y_MAP_MAX_SIZE;++i)
            {
                if (x_2_y_map[i][0] > 0)
                {
                    ++c;
                }
            }

            return c;
        }

        void recalc_poly_beta()
        {
            clear_poly_beta();

            size_t n = get_num_calibration_points(); 

            if (n == 0)
            {
                return;
            }

            _poly_order = n-1;

            if (_poly_order > MAX_POLY_ORDER)
            {
                _poly_order = MAX_POLY_ORDER;
            }

            double x[X_2_Y_MAP_MAX_SIZE];
            double y[X_2_Y_MAP_MAX_SIZE];

            if (_poly_order == 0)
            {
                x[0] = 0;
                y[0] = 0;    
                x[1] = x_2_y_map[1][0];
                y[1] = x_2_y_map[1][1];
            }
            else
            {
                for (size_t i=0;i<n; ++i)
                {
                    x[i] = x_2_y_map[i][0];
                    y[i] = x_2_y_map[i][1];
                }
            }
    
            PolyFitEasy(x, y, n, _poly_order, poly_beta);
            _has_poly_beta = true;
        }

        bool has_poly_beta() const 
        {
            return _has_poly_beta;
        }

        float poly_calc(float x) const 
        {
            if (x == 0.)
            {
                return 0.;
            }

            if (_has_poly_beta == true)
            {
                return (float) calculatePoly(x, poly_beta, _poly_order);
            }
            return 0.;
        }

        static float poly_calc(float x, const std::vector<float> & _poly_beta) 
        {
            if (x == 0.)
            {
                return 0.;
            }

            if (_poly_beta.size() >= 2)  // it has to be at least linear function, 0 coefficient first
            {
                double * __poly_beta = new double[_poly_beta.size()];

                for (size_t i=0; i<_poly_beta.size(); ++i)
                {
                    __poly_beta[i] = _poly_beta[i];
                }
                
                float r = (float) calculatePoly(x, __poly_beta, _poly_beta.size()-1);
                delete __poly_beta;
                return r;
            }
            return 0.;
        }

        uint8_t addr;  // not used for input_a_low
        int index;     // index for input_a_low is induced by the order in the array, for others it is specified channel number

        // should be kept sorted all the time, x == 0 ? unused cell
        // all algorithms are valid for x > 0 !!!

        float x_2_y_map[X_2_Y_MAP_MAX_SIZE][2];  

        double poly_beta[MAX_POLY_ORDER+1];
        bool _has_poly_beta;
        int _poly_order;
    };

    class AppletHandler
    {
    public:

        AppletHandler() 
        {
        }

        ~AppletHandler() 
        {
        }

        void init(const MainsProbeConfig::Applet & _config)
        {   
            config = _config;
        }

        MainsProbeConfig::Applet config;
    };

    MainsProbeHandler()
    {
        _data_needs_save = false;

        _is_active = false;
        _is_finished = true;

        // * input gain is what the measured value is multiplied with to get the final result

        const float input_v_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1
        };

        const float input_a_high_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1
        };

        const float input_a_low_gain_function[] =
        {
            1, 1,
            2, 1,
            3, 1
        };

        input_v_gain_map_table.init_table(input_v_gain_function, (sizeof(input_v_gain_function)/sizeof(float))/2);
        input_a_high_gain_map_table.init_table(input_a_high_gain_function, (sizeof(input_a_high_gain_function)/sizeof(float))/2);
        input_a_low_gain_map_table.init_table(input_a_low_gain_function, (sizeof(input_a_low_gain_function)/sizeof(float))/2);

        two_wire = &Wire;
    }

    ~MainsProbeHandler()
    {
        for (auto it=applet_handlers.begin(); it!=applet_handlers.end(); ++it)
        {
            delete it->second;
        }
        applet_handlers.clear();

        for (auto it=input_v_ina3221.begin(); it!=input_v_ina3221.end(); it++)
        {
            delete it->second;
        }
        
        input_v_ina3221.clear();

        for (auto it=input_a_high_ina3221.begin(); it!=input_a_high_ina3221.end(); it++)
        {
            delete it->second;
        }
        
        input_v_ina3221.clear();
    }

    bool is_active() const { return _is_active; }

    void start(const MainsProbeConfig &config);
    void stop();
    void reconfigure(const MainsProbeConfig &config);

    MainsProbeStatus get_status()
    {
        MainsProbeStatus _status;

        Lock lock(semaphore);
        _status = status;

        return _status;
    }

    String calibrate_v(uint8_t addr, size_t channel, float value);
    String calibrate_a_high(uint8_t addr, size_t channel, float value);
    String calibrate_a_low(size_t channel, float value);
    String uncalibrate_v(uint8_t addr, size_t channel);
    String uncalibrate_a_high(uint8_t addr, size_t channel);
    String uncalibrate_a_low(size_t channel);
    String input_v(uint8_t addr, size_t channel, float & value, bool no_trace = false, float * raw_voltage = NULL);
    String input_a_high(uint8_t addr, size_t channel, float & value, bool no_trace = false, float * raw_voltage = NULL);
    String input_a_low(size_t channel, float & value, bool no_trace = false, float * raw_voltage = NULL);
    
    void get_calibration_data(JsonVariant &);
    String import_calibration_data(const JsonVariant & json);

    bool does_data_need_save();
    void data_saved();

    void data_to_eprom(std::ostream & os);
    bool data_from_eprom(std::istream & is);

    bool read_data();
    void save_data();

protected:

    void configure_i2c();
    void configure_channels();
    void configure_applets();

    String calibrate_ina3221(MainsProbeConfig::InputWhat iw, uint8_t addr, size_t channel, float value);
    String input_ina3221(MainsProbeConfig::InputWhat iw, uint8_t addr, size_t channel, float & value, bool no_trace = false, float * raw_voltage = NULL);

    unsigned analog_read(uint8_t gpio);
    unsigned analog_read_cycle_avg(uint8_t gpio, size_t * _sample_count = NULL);

    static void task(void *parameter);

    BinarySemaphore semaphore;
    MainsProbeConfig config;
    MainsProbeStatus status;

    std::vector<InputChannelData> input_v_channel_data;
    std::vector<InputChannelData> input_a_high_channel_data;
    std::vector<InputChannelData> input_a_low_channel_data;
 
    std::map<String, AppletHandler*> applet_handlers;

    bool _data_needs_save;

    bool _is_active;
    bool _is_finished;

    MapTable input_v_gain_map_table;
    MapTable input_a_high_gain_map_table;
    MapTable input_a_low_gain_map_table;

    std::map<uint8_t, Ina3221*> input_v_ina3221;
    std::map<uint8_t, Ina3221*> input_a_high_ina3221;
    
    TwoWire * two_wire;
};

static MainsProbeHandler handler;

void MainsProbeHandler::start(const MainsProbeConfig &_config)
{
    TRACE("starting mains-probe handler")
    //Serial.write("DIRECT: starting mains-probe handler");

    if (_is_active)
    {
        ERROR("mains-probe handler already running")
        return; // already running
    }

    while(_is_finished == false)
    {
        delay(100);
    }

    config = _config;

    configure_i2c();
    configure_channels();
    configure_applets();

    read_data();

    _is_active = true;
    _is_finished = false;

    TRACE("starting mains-probe handler task")

    xTaskCreate(
        task,                // Function that should be called
        "mains_probe_task", // Name of the task (for debugging)
        4096,                // Stack size (bytes)
        this,                // Parameter to pass
        1,                   // Task priority
        NULL                 // Task handle
    );
}

void MainsProbeHandler::stop()
{
    TRACE("stopping mains_probe handler")

    if (_is_active)
    {
    }

    _is_active = false;

    while(_is_finished == false)
    {
        delay(100);
    }
}

void MainsProbeHandler::reconfigure(const MainsProbeConfig &_config)
{
    Lock lock(semaphore);

    if (!(config == _config))
    {
        TRACE("mains-probe task: config changed")

        bool should_configure_i2c = false;

        if (!(config.i2c == _config.i2c))
        {
            TRACE("mains-probe task: i2c config changed")
            should_configure_i2c = true;
        }
    
        bool should_configure_channels = (config.input_v == _config.input_v && 
                                          config.input_a_high == _config.input_a_high && 
                                          config.input_a_low_channels == _config.input_a_low_channels) ? false : true;

        bool should_configure_applets = config.applets == _config.applets  ? false : true;

        config = _config;

        if (should_configure_i2c == true)
        {
            configure_i2c();
        }
    
        if (should_configure_channels)
        {
            configure_channels();
        }

        if (should_configure_applets)
        {
            configure_applets();
        }
    }
}

void MainsProbeHandler::task(void *parameter)
{
    MainsProbeHandler *_this = (MainsProbeHandler *)parameter;

    TRACE("mains-probe task: started")

    const size_t SAVE_DATA_INTERVAL = 10; // seconds
    unsigned long last_save_data_millis = millis();

    const size_t INA3221_READ_INTERVAL = 10; // seconds
    unsigned long last_ina3221_read_millis = millis();

    uint32_t last_i2c_scan_millis = 0;
    const uint32_t I2C_SCAN_INTERVAL_MILLIS = 10000; // 10 minutes (seconds)

    while (_this->_is_active)
    {
        unsigned long now_millis = millis();

        if (now_millis < last_save_data_millis || 
            (now_millis-last_save_data_millis)/1000 >= SAVE_DATA_INTERVAL)
        {
            // automatically refresh input values in status

            last_save_data_millis = now_millis;

            if (_this->does_data_need_save())
            {
                _this->save_data();
                _this->data_saved();
            }
        }

        if (now_millis < last_ina3221_read_millis || 
            (now_millis-last_ina3221_read_millis)/1000 >= INA3221_READ_INTERVAL)
        {
            last_ina3221_read_millis = now_millis;

            Lock lock(_this->semaphore);

            for (auto it=_this->config.input_v.begin(); it!=_this->config.input_v.end(); it++)
            {
                for (auto jt=it->channels.begin(); jt!=it->channels.end(); jt++)
                {
                    float raw_voltage = 0;
                    float v = 0;

                    _this->input_v(it->addr, jt->channel, v, true, & raw_voltage);
                    DEBUG("input_v[addr=0x%02x][%d] raw=%f V calc=%f V", (int) it->addr, jt->channel, raw_voltage, v)
                }
            }

            for (auto it=_this->config.input_a_high.begin(); it!=_this->config.input_a_high.end(); it++)
            {
                for (auto jt=it->channels.begin(); jt!=it->channels.end(); jt++)
                {
                    float raw_voltage = 0;
                    float v = 0;

                    _this->input_a_high(it->addr, jt->channel, v, true, & raw_voltage);
                    DEBUG("input_a_high[addr=0x%02x][%d] raw=%f V calc=%f A", (int) it->addr, jt->channel, raw_voltage, v)
                }
            }
            
            size_t i=0;
            
            for (auto it=_this->config.input_a_low_channels.begin(); it!=_this->config.input_a_low_channels.end(); it++, ++i)
            {
                float raw_voltage = 0;
                float v = 0;

                _this->input_a_low(i, v, true, & raw_voltage);
                DEBUG("input_a_low[%d] raw=%f mV calc=%f A", (int) i, raw_voltage, v)
            }
        }

        if (last_i2c_scan_millis > now_millis || (now_millis-last_i2c_scan_millis) >= I2C_SCAN_INTERVAL_MILLIS)
        {
            last_i2c_scan_millis = now_millis;
            i2c_scan(*(_this->two_wire));  // TODO: move to a separate task?
        }

        delay(1000);
    }

    _this->_is_finished = true;

    TRACE("mains-probe task: terminated")
    vTaskDelete(NULL);
}


static String esp_err_2_string(esp_err_t esp_r)
{
    char buf[128];
    esp_err_to_name_r(esp_r, buf, sizeof(buf));
    return String("ERR=") + String((int) esp_r) + ", " + buf;
}


void MainsProbeHandler::configure_i2c()
{
    TRACE("configure_i2c")
    
    if (two_wire)
    {
        two_wire->end();
        two_wire->begin(config.i2c.sda.gpio, config.i2c.scl.gpio);
        i2c_scan(*two_wire);
    }

    TRACE("configure_i2c done")
}


void MainsProbeHandler::configure_channels()
{
    TRACE("configure channels")
    Lock lock(semaphore);

    // V
    
    status.input_v_channels.clear();
    std::vector<InputChannelData> new_input_v_channel_data;
    std::map<uint8_t, Ina3221*> new_input_v_ina3221;

    for (auto it=config.input_v.begin(); it!=config.input_v.end(); it++)
    {
        for (auto jt=it->channels.begin(); jt!=it->channels.end(); jt++)
        {
            // handle status: just add a new ; it will be updated with a new read value soon
         
            MainsProbeStatus::Channel channel_status;
            channel_status.addr = it->addr;
            channel_status.index = jt->channel;

            status.input_v_channels.push_back(channel_status);

            // handle input channel data: try to find existing data with the same addr+index
            // if found - copy to new array, otherwise create
            // this way we can keep calibration data e.g. if a new INA3221 is added (provided the old one
            // didnt change the address)

            bool found = false;

            for (auto xt=input_v_channel_data.begin(); xt!=input_v_channel_data.end(); xt++)
            {
                if (xt->addr == it->addr && xt->index == jt->channel)
                {
                    new_input_v_channel_data.push_back(*xt);
                    found = true;
                    break;
                }
            }

            if (found == false)
            {
                InputChannelData input_channel_data;
                input_channel_data.addr = it->addr; 
                input_channel_data.index = jt->channel; 
                new_input_v_channel_data.push_back(input_channel_data);
            }

            // handle ina3221 instance; move existing one if found with the same addr

            auto zt = new_input_v_ina3221.find(it->addr);

            if (zt == new_input_v_ina3221.end())
            {
                auto yt = input_v_ina3221.find(it->addr);

                if (yt == input_v_ina3221.end())
                {
                    // create new

                    Ina3221 * ina3221 = new Ina3221(two_wire);
                    ina3221->set_addr(it->addr);
                    ina3221->setup();

                    new_input_v_ina3221.insert(std::make_pair(it->addr, ina3221));
                }
                else
                {
                    new_input_v_ina3221.insert(std::make_pair(it->addr, yt->second));
                    input_v_ina3221[it->addr] = NULL; // remember not to delete
                }
            }
            // else just already added with another channel
        }
    }

    input_v_channel_data = new_input_v_channel_data;

    // delete ina3221 instances that has not been reused

    for (auto it=input_v_ina3221.begin(); it!=input_v_ina3221.end(); it++)
    {
        if (it->second != NULL)
        {
            delete it->second;
        }
    }

    input_v_ina3221 = new_input_v_ina3221;

    // A-HIGH
    
    status.input_a_high_channels.clear();
    std::vector<InputChannelData> new_input_a_high_channel_data;
    std::map<uint8_t, Ina3221*> new_input_a_high_ina3221;

    for (auto it=config.input_a_high.begin(); it!=config.input_a_high.end(); it++)
    {
        for (auto jt=it->channels.begin(); jt!=it->channels.end(); jt++)
        {
            // handle status: just add a new ; it will be updated with a new read value soon
         
            MainsProbeStatus::Channel channel_status;
            channel_status.addr = it->addr;
            channel_status.index = jt->channel;

            status.input_a_high_channels.push_back(channel_status);

            // handle input channel data: try to find existing data with the same addr+index
            // if found - copy to new array, otherwise create
            // this way we can keep calibration data e.g. if a new INA3221 is added (provided the old one
            // didnt change the address)

            bool found = false;

            for (auto xt=input_a_high_channel_data.begin(); xt!=input_a_high_channel_data.end(); xt++)
            {
                if (xt->addr == it->addr && xt->index == jt->channel)
                {
                    new_input_a_high_channel_data.push_back(*xt);
                    found = true;
                    break;
                }
            }

            if (found == false)
            {
                InputChannelData input_channel_data;
                input_channel_data.addr = it->addr; 
                input_channel_data.index = jt->channel; 
                new_input_a_high_channel_data.push_back(input_channel_data);
            }

            // handle ina3221 instance; move existing one if found with the same addr

            auto zt = new_input_a_high_ina3221.find(it->addr);

            if (zt == new_input_a_high_ina3221.end())
            {
                auto yt = input_a_high_ina3221.find(it->addr);

                if (yt == input_a_high_ina3221.end())
                {
                    // create new

                    Ina3221 * ina3221 = new Ina3221(two_wire);
                    ina3221->set_addr(it->addr);
                    ina3221->setup();

                    new_input_a_high_ina3221.insert(std::make_pair(it->addr, ina3221));
                }
                else
                {
                    new_input_a_high_ina3221.insert(std::make_pair(it->addr, yt->second));
                    input_a_high_ina3221[it->addr] = NULL; // remember not to delete
                }
            }
            // else just already added with another channel
        }
    }

    input_a_high_channel_data = new_input_a_high_channel_data;

    // delete ina3221 instances that has not been reused

    for (auto it=input_a_high_ina3221.begin(); it!=input_a_high_ina3221.end(); it++)
    {
        if (it->second != NULL)
        {
            delete it->second;
        }
    }

    input_a_high_ina3221 = new_input_a_high_ina3221;

    // A-LOW

    input_a_low_channel_data.resize(config.input_a_low_channels.size());

    MainsProbeStatus::Channel input_a_low_channel_p;
    status.input_a_low_channels.resize(config.input_a_low_channels.size(), input_a_low_channel_p);

    size_t i=0;

    for (auto it=config.input_a_low_channels.begin(); it!=config.input_a_low_channels.end(); ++it, ++i)
    {
        TRACE("configure input_a_low_channels[%d] gpio=%d, atten=%d", (int)i, (int)it->gpio, (int)it->atten)
        analogSetPinAttenuation(it->gpio, (adc_attenuation_t)it->atten);
    }

    _data_needs_save = true;

    TRACE("configure channels done")
}

void MainsProbeHandler::configure_applets()
{    
    TRACE("configure_applets")
    Lock lock(semaphore);
    
    for (auto it=applet_handlers.begin(); it!=applet_handlers.end(); ++it)
    {
        delete it->second;
    }
    applet_handlers.clear();

    status.applets.clear();

    for (auto it=config.applets.begin(); it!=config.applets.end(); ++it)
    {
        TRACE("configure applet %s", it->first.c_str())
        AppletHandler * _applet_handler = new AppletHandler();
        _applet_handler->init(it->second);
        applet_handlers.insert(std::make_pair(it->first, _applet_handler));

        MainsProbeStatus::Applet _applet_status;
        _applet_status.function = it->second.function_2_str(it->second.function);
        status.applets.insert(std::make_pair(it->first, _applet_status));
    }

    TRACE("configure_applets done")
}

unsigned MainsProbeHandler::analog_read(uint8_t gpio)
{
    adcAttachPin(gpio);
    return analogRead(gpio);
}

unsigned MainsProbeHandler::analog_read_cycle_avg(uint8_t gpio, size_t * _sample_count)
{
    // we have to take several samples preferably during one cycle of a pulsation sine (we assume some pulsation)
    // we take the frequency to be 50 Hz but we don't know how long time reading takes
    // thus we read as many as we can during 1/frequency time (limit though)

    const size_t MAX_SAMPLE_COUNT=100;

    unsigned long now_millis = millis();
    const unsigned long SAMPLE_MILLIS = 20; // for 50 Hz

    unsigned long sample_average = 0;
    size_t sample_count = 0;

    for (size_t i=0; i<MAX_SAMPLE_COUNT; ++i)
    {
        sample_average += analog_read(gpio);
        sample_count++;

        unsigned long new_millis = millis();

        if (new_millis < now_millis || (new_millis - now_millis >= SAMPLE_MILLIS))
        {
            break;
        }

        delay(0);
    }

    sample_average /= sample_count;

    if (_sample_count != NULL)
    {
        *_sample_count = sample_count;
    }

    return (unsigned) sample_average;
}

String MainsProbeHandler::calibrate_ina3221(MainsProbeConfig::InputWhat iw, uint8_t addr, size_t channel, float value)
{
    const char * iw_text1 = NULL;
    std::map<uint8_t, Ina3221*> * ina3221_map = NULL;
    std::vector<InputChannelData> * channel_data_vector = NULL;
    
    switch(iw)
    {
        case MainsProbeConfig::iwV:

            iw_text1 = "input V";
            ina3221_map = & input_v_ina3221;
            channel_data_vector = & input_v_channel_data;
            break;

            case MainsProbeConfig::iwAHigh:

            iw_text1 = "input A-High";
            ina3221_map = & input_a_high_ina3221;
            channel_data_vector = & input_a_high_channel_data;
            break;

        default:
            return "calibrate_ina3221 internal error, InputWhat";    
        }
    

    TRACE("calibrating %s addr 0x%2x channel %d, value %f", iw_text1, (int) addr, (int) channel, value)
    String r;

    Lock lock(semaphore);

    if (addr == 0 && ina3221_map->size() == 1)
    {
        addr = (*ina3221_map)[0]->get_addr();
    }

    if (value > 0)
    {
        for (auto it=channel_data_vector->begin(); it!=channel_data_vector->end(); ++it)
        {
            if (it->addr == addr && it->index == channel)
            {
                auto jt = ina3221_map->find(addr);

                if (jt != ina3221_map->end())
                {
                    float x = jt->second->read_voltage(channel);
                    it->add_calibration_point(x, value);

                    TRACE("Added calibration point for %s addr 0x%02x channel %d: x=%.2f, y=%.2f", iw_text1, (int) addr, (int) channel, x, value)
                    
                    it->debug_x_2_y_map();

                    _data_needs_save = true;
                }
                else
                {
                    r = String("no INA3221 with addr 0x") + String(addr, 16);
                }

                break;
            }
        }
    }
    else
    {
        r = "value should be > 0";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String MainsProbeHandler::calibrate_v(uint8_t addr, size_t channel, float value)
{
    return calibrate_ina3221(MainsProbeConfig::iwV, addr, channel, value);
}

String MainsProbeHandler::calibrate_a_high(uint8_t addr, size_t channel, float value)
{
    return calibrate_ina3221(MainsProbeConfig::iwAHigh, addr, channel, value);
}

String MainsProbeHandler::calibrate_a_low(size_t channel, float value)
{
    TRACE("calibrating input A-Low channel %d, value %f", (int) channel, value)
    String r;

    Lock lock(semaphore);

    if (value > 0)
    {
        if (channel < input_a_low_channel_data.size())
        {
            uint8_t gpio = config.input_a_low_channels[channel].gpio;
            uint8_t atten = config.input_a_low_channels[channel].atten;
        
            size_t sample_count = 0;
            unsigned sample_average = analog_read_cycle_avg(gpio, & sample_count);
        
            DEBUG("analog read gpio %d sample average %lu based on %d samples", (int) gpio, sample_average, (int) sample_count)
        
            float x = MainsProbeConfig::InputChannel::adc_value_2_mv(sample_average, atten);
        
            input_a_low_channel_data[channel].add_calibration_point(x, value);

            TRACE("Added calibration point for input A-Low channel %d: x=%.2f, y=%.2f", (int) channel, x, value)
            
            input_a_low_channel_data[channel].debug_x_2_y_map();

            _data_needs_save = true;
}
        else
        {
            r = String("input A-Low channel number ") + String((int) channel) + " is invalid";
        }
    }
    else
    {
        r = "value should be > 0";
    }

    if (r.length())
    {
        ERROR(r.c_str())
    }

    return r;
}

String MainsProbeHandler::uncalibrate_v(uint8_t addr, size_t channel)
{
    TRACE("uncalibrating input V addr 0x%02x channel %d", (int) addr, (int) channel)
    String r;

    Lock lock(semaphore);

    for (auto it=input_v_channel_data.begin(); it!=input_v_channel_data.end(); ++it)
    {
        if (it->addr == addr && it->index == channel)
        {
            it->uncalibrate();
            return r;
        }
    }

    r = String("input V with addr 0x") + String(addr, 16) + " channel " + String((int) channel) + " is invalid";
    ERROR(r.c_str())

    return r;
}

String MainsProbeHandler::uncalibrate_a_high(uint8_t addr, size_t channel)
{
    TRACE("uncalibrating input A-HIGH addr 0x%02x channel %d", (int) addr, (int) channel)
    String r;

    Lock lock(semaphore);

    for (auto it=input_a_high_channel_data.begin(); it!=input_a_high_channel_data.end(); ++it)
    {
        if (it->addr == addr && it->index == channel)
        {
            it->uncalibrate();
            return r;
        }
    }

    r = String("input A-HIGH with addr 0x") + String(addr, 16) + " channel " + String((int) channel) + " is invalid";
    ERROR(r.c_str())

    return r;
}

String MainsProbeHandler::uncalibrate_a_low(size_t channel)
{
    TRACE("uncalibrating input A-LOW channel %d", (int) channel)
    String r;

    Lock lock(semaphore);

    if (channel < input_a_low_channel_data.size())
    {
        input_a_low_channel_data[channel].uncalibrate();
    }
    else
    {
        r = String("input A-LOW channel number ") + String((int) channel) + " is invalid";
        ERROR(r.c_str())
    }

    return r;
}

String MainsProbeHandler::input_ina3221(MainsProbeConfig::InputWhat iw, uint8_t addr, size_t channel, float & value, bool no_trace, float * _raw_voltage)
{
    const char * iw_text1 = NULL;
    const char * iw_text2 = NULL;
    std::map<uint8_t, Ina3221*> * ina3221_map = NULL;
    std::vector<InputChannelData> * channel_data_vector = NULL;
    std::vector<MainsProbeConfig::Ina3221Config> * ina3221_config_vector = NULL;
    std::vector<MainsProbeStatus::Channel> * channel_status_vector = NULL;

    switch(iw)
    {
        case MainsProbeConfig::iwV:

            iw_text1 = "input V";
            iw_text2 = "input_v";
            ina3221_map = & input_v_ina3221;
            channel_data_vector = & input_v_channel_data;
            ina3221_config_vector = & config.input_v;
            channel_status_vector = & status.input_v_channels;
            break;

            case MainsProbeConfig::iwAHigh:

            iw_text1 = "input A-High";
            iw_text2 = "input_a_high";
            ina3221_map = & input_a_high_ina3221;
            channel_data_vector = & input_a_high_channel_data;
            ina3221_config_vector = & config.input_a_high;
            channel_status_vector = & status.input_a_high_channels;
            break;

        default:
            return "input_ina3221 internal error, InputWhat";    
        }

    if (no_trace == false)
    {
        TRACE("getting %s for addr 0x%02x, channel %d", iw_text1, (int) addr, (int) channel)
    }

    value = 0;
    String r;

    Lock lock(semaphore);

    float raw_voltage = 0;

    auto ina_it=ina3221_map->find(addr);
        
    if (ina_it != ina3221_map->end())
    {
        if (channel >= 0 && channel < Ina3221::NUM_PORTS)
        {
            raw_voltage = ina_it->second->read_voltage(channel);
    
            if (_raw_voltage != NULL)
            {
                *_raw_voltage = raw_voltage;
            }
            
            if (no_trace == false)
            {
                DEBUG("%s_raw[addr=0x%02x][%d]=%f V", iw_text2, (int) addr, (int) channel, raw_voltage)
            }
        }
        else
        {
            r = String(iw_text1) + ", channel index " + String(channel) + " is out of range";
        }
    }
    else
    {
        r = String(iw_text1) + ", address 0x" + String((int) addr, 16) + " is not configured";
    }

    if (r.length() == 0)
    {
        bool found_and_set = false;

        /*
        if (raw_voltage == 0)
        {
            value = 0;
            found_and_set = true;
        }*/

        // first try poly calc from channel data (will work if has enough calibration points)

        if (found_and_set == false)  
        {
            for (auto it=channel_data_vector->begin(); it!=channel_data_vector->end(); ++it)
            {
                if (it->addr == addr && it->index == channel)
                {
                    if (it->has_poly_beta() == true)
                    {
                        value = (float) it->poly_calc(raw_voltage);
                        
                        if (no_trace == false)
                        {
                            DEBUG("%s, value for [addr=0x%02x][%d] calculated via calibration data %f", iw_text1, (int) addr, (int) channel, value)
                            //DEBUG("poly_beta [addr=0x%02x][%d]:", (int) addr, (int) channel)
                            //it->debug_poly_beta();
                        }

                        found_and_set = true;
                        break;
                    }
                }
            }
        }

        // otherwise - try default_poly_beta from config
        
        if (found_and_set == false)  
        {
            for (auto it=ina3221_config_vector->begin(); it!=ina3221_config_vector->end(); ++it)
            {
                if (it->addr == addr)
                {
                    for (auto jt=it->channels.begin(); jt!=it->channels.end(); ++jt)
                    {
                        if (jt->channel == channel)
                        {
                            if (jt->default_poly_beta.size() > 1) // at least a linear function
                            {
                                value = InputChannelData::poly_calc(raw_voltage,jt->default_poly_beta);

                                if (no_trace == false)
                                {
                                    DEBUG("%s, value for [addr=0x%02x][%d] calculated via default_poly_beta %f", iw_text1, (int) addr, (int) channel, value)
                                }

                                found_and_set = true;
                            }
                        }
                    }
                }
            }

            if (found_and_set == false)
            {
                r = String(iw_text1) + " for address 0x" + String((int) addr, 16) + " channel " + String(channel) + " cannot be evaluated";
            }
        }
    }

    // update in status

    for (auto it=channel_status_vector->begin(); it!=channel_status_vector->end(); ++it)
    {
        if (it->addr == addr && it->index == channel)
        {
            it->value = value;
            break;
        }
    }

    if (no_trace == false && r.length() > 0)
    {
        ERROR(r.c_str())
    }
    
    return r;
}

String MainsProbeHandler::input_v(uint8_t addr, size_t channel, float & value, bool no_trace, float * _raw_voltage)
{
    return input_ina3221(MainsProbeConfig::iwV, addr, channel, value, no_trace, _raw_voltage);
}

String MainsProbeHandler::input_a_high(uint8_t addr, size_t channel, float & value, bool no_trace, float * _raw_voltage)
{
    return input_ina3221(MainsProbeConfig::iwAHigh, addr, channel, value, no_trace, _raw_voltage);
}

String MainsProbeHandler::input_a_low(size_t channel, float & value, bool no_trace, float * _raw_voltage)
{
    if (no_trace == false)
    {
        TRACE("getting input A-Low of channel %d", (int) channel)
    }

    String r;

    Lock lock(semaphore);

    if (channel < input_a_low_channel_data.size())
    {
        uint8_t gpio = config.input_a_low_channels[channel].gpio;
        uint8_t atten = config.input_a_low_channels[channel].atten;

        size_t sample_count = 0;
        unsigned sample_average = analog_read_cycle_avg(gpio, & sample_count);

        if (no_trace == false)
        {
            DEBUG("analog read gpio %d sample average %lu based on %d samples", (int) gpio, sample_average, (int) sample_count)
        }

        float _value = MainsProbeConfig::InputChannel::adc_value_2_mv(sample_average, atten);

        if (_raw_voltage != NULL)
        {
            *_raw_voltage = _value;
        }

        if (no_trace == false)
        {
            DEBUG("analog read mv %f", _value)
        }
        
        if (r.length() == 0)
        {
            bool found_and_set = false;
    
            /*
            if (raw_voltage == 0)
            {
                value = 0;
                found_and_set = true;
            }*/
    
            // first try poly calc from channel data (will work if has enough calibration points)
    
            if (found_and_set == false)  
            {
                if (input_a_low_channel_data[channel].has_poly_beta() == true)
                {
                    value = (float) input_a_low_channel_data[channel].poly_calc(_value);
                            
                    if (no_trace == false)
                    {
                        DEBUG("input A-Low, value for [%d] calculated via calibration data %f", (int) channel, value)
                        //DEBUG("poly_beta [%d]:", (int) channel)
                        //input_a_low_channel_data[channel].debug_poly_beta();
                    }

                    found_and_set = true;
                }
            }
    
            // otherwise - try default_poly_beta from config
            
            if (found_and_set == false)  
            {
                if (config.input_a_low_channels[channel].default_poly_beta.size() > 1) // at least a linear function
                {
                    value = InputChannelData::poly_calc(_value,config.input_a_low_channels[channel].default_poly_beta);

                    if (no_trace == false)
                    {
                        DEBUG("input A-Low, value for [%d] calculated via default_poly_beta %f", (int) channel, value)
                    }

                    found_and_set = true;
                }
    
                if (found_and_set == false)
                {
                    r = String("input A-Low for channel ") + String(channel) + " cannot be evaluated";
                }
            }
        }
    
        // update in status            
        status.input_a_low_channels[channel].value = value;
    }
    else
    {
        r = String("input A-Low channel number ") + String((int) channel) + " is invalid";
    }

    if (no_trace == false && r.length() > 0)
    {
        ERROR(r.c_str())
    }

    return r;
}

void MainsProbeHandler::get_calibration_data(JsonVariant & json)
{
    json.createNestedObject("mainsProbeCalibrationData");
    JsonVariant jsonVariant = json["mainsProbeCalibrationData"];

    {jsonVariant.createNestedArray("input_v");
    JsonArray jsonArray = jsonVariant["input_v"];

    for (auto it = input_v_channel_data.begin(); it != input_v_channel_data.end(); ++it)
    {
        DynamicJsonDocument doc(1024);
        JsonVariant item = doc.to<JsonVariant>();
        it->to_json(item);
        jsonArray.add(item);
    }}

    {jsonVariant.createNestedArray("input_a_high");
    JsonArray jsonArray = jsonVariant["input_a_high"];

    for (auto it = input_a_high_channel_data.begin(); it != input_a_high_channel_data.end(); ++it)
    {
        DynamicJsonDocument doc(1024);
        JsonVariant item = doc.to<JsonVariant>();
        it->to_json(item);
        jsonArray.add(item);
    }}
    

    {jsonVariant.createNestedArray("input_a_low");
    JsonArray jsonArray = jsonVariant["input_a_low"];

    for (auto it = input_a_low_channel_data.begin(); it != input_a_low_channel_data.end(); ++it)
    {
        DynamicJsonDocument doc(1024);
        JsonVariant item = doc.to<JsonVariant>();
        it->to_json(item);
        jsonArray.add(item);
    }}
}

String MainsProbeHandler::import_calibration_data(const JsonVariant & json)
{
    DEBUG("MainsProbeHandler::import_calibration_data")
    String r;

    Lock lock(semaphore);

    if (json.containsKey("mainsProbeCalibrationData"))
    {
        DEBUG("contains key mainsProbeCalibrationData")
        JsonVariant jsonVariant = json["mainsProbeCalibrationData"];

        const char * input_type_str[] = 
        {
            "input_v", "input_a_high", "input_a_low"
        };

        std::vector<InputChannelData>* input_vector[] = 
        {
            & input_v_channel_data, & input_a_high_channel_data, & input_a_low_channel_data
        };

        for (size_t k=0; k<sizeof(input_type_str)/sizeof(input_type_str[0]); ++k)
        {
            if (jsonVariant.containsKey(input_type_str[k]))
            {
                DEBUG("contains key %s", input_type_str[k])
                const JsonVariant &_json = jsonVariant[input_type_str[k]];

                if (_json.is<JsonArray>())
                {
                    DEBUG("%s is array", input_type_str[k])
                    const JsonArray & jsonArray = _json.as<JsonArray>();
                    auto iterator = jsonArray.begin();

                    while(iterator != jsonArray.end())
                    {
                        //DEBUG("%s item", input_type_str[k])
                        const JsonVariant & __json = *iterator;

                        InputChannelData input_channel_data;

                        if (input_channel_data.from_json(__json) == true)
                        {
                            bool found = false;

                            for (auto it=input_vector[k]->begin(); it!=input_vector[k]->end(); ++it)
                            {
                                if (it->addr == input_channel_data.addr && it->index == input_channel_data.index)
                                {
                                    found = true;
                                    *it = input_channel_data;

                                    DEBUG("Imported %s calibration data item", input_type_str[k])
                                    it->debug();
                                    break;
                                }
                            }

                            if (found == false)
                            {
                                TRACE("%s calibration_data item being imported is not found in current data and will be skipped, addr 0x%02x, index %d", 
                                    input_type_str[k], (int) input_channel_data.addr, (int) input_channel_data.index)
                            }
                        }

                        ++iterator;
                    }
                }
            }
        }

        _data_needs_save = true;
        // TODO: recalculate interpolation coefficients
    }

    return r;
}

bool MainsProbeHandler::does_data_need_save() 
{
    return _data_needs_save;
}

void MainsProbeHandler::data_saved() 
{
    _data_needs_save = false;
}

void MainsProbeHandler::data_to_eprom(std::ostream &os) 
{
    Lock lock(semaphore);

    DEBUG("MainsProbeHandler data_to_eprom")

    uint8_t eprom_version = (uint8_t)DATA_EPROM_VERSION;
    os.write((const char *)&eprom_version, sizeof(eprom_version));

    uint8_t count = input_v_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_v_channel_data.begin(); it!=input_v_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }
    
    count = input_a_high_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_a_high_channel_data.begin(); it!=input_a_high_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }

    count = input_a_low_channel_data.size();
    os.write((const char *)&count, sizeof(count));

    for (auto it=input_a_low_channel_data.begin(); it!=input_a_low_channel_data.end(); ++it)
    {
        it->to_eprom(os);
    }

}

bool MainsProbeHandler::data_from_eprom(std::istream &is)
{
    uint8_t eprom_version = DATA_EPROM_VERSION;

    is.read((char *)&eprom_version, sizeof(eprom_version));

    DEBUG("MainsProbeHandler data_from_eprom")

    if (eprom_version == DATA_EPROM_VERSION)
    {
        DEBUG("Version match")

        Lock lock(semaphore);

        // V
        
        std::vector<InputChannelData> new_input_v_channel_data;

        uint8_t count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input V channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_v_channel_data.push_back(input_channel_data_item);
        }

        if (is.bad())
        {
            ERROR("error reading input V channel data count from eprom. corrupt?")
            return false;
        }

        input_v_channel_data.clear();

        for (auto it=config.input_v.begin(); it!=config.input_v.end(); ++it)
        {
            for (auto jt=it->channels.begin(); jt!=it->channels.end(); ++jt)
            {
                bool found = false;
                for (auto xt=new_input_v_channel_data.begin(); xt!=new_input_v_channel_data.end(); ++xt)
                {
                    if (xt->addr == it->addr && xt->index == jt->channel)
                    {
                        // TODO feed data 
                        input_v_channel_data.push_back(*xt);
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    InputChannelData input_channel_data_item;
                    input_channel_data_item.addr = it->addr;
                    input_channel_data_item.index = jt->channel;
                    input_v_channel_data.push_back(input_channel_data_item);
                }
            }
        }

        // A-HIGH

        std::vector<InputChannelData> new_input_a_high_channel_data;

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input A-HIGH channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_a_high_channel_data.push_back(input_channel_data_item);
        }

        if (is.bad())
        {
            ERROR("error reading input A-HIGH channel data count from eprom. corrupt?")
            return false;
        }

        input_a_high_channel_data.clear();

        for (auto it=config.input_a_high.begin(); it!=config.input_a_high.end(); ++it)
        {
            for (auto jt=it->channels.begin(); jt!=it->channels.end(); ++jt)
            {
                bool found = false;
                for (auto xt=new_input_a_high_channel_data.begin(); xt!=new_input_a_high_channel_data.end(); ++xt)
                {
                    if (xt->addr == it->addr && xt->index == jt->channel)
                    {
                        // TODO feed data 
                        input_a_high_channel_data.push_back(*xt);
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    InputChannelData input_channel_data_item;
                    input_channel_data_item.addr = it->addr;
                    input_channel_data_item.index = jt->channel;
                    input_a_high_channel_data.push_back(input_channel_data_item);
                }
            }
        }

        // A-LOW

        std::vector<InputChannelData> new_input_a_low_channel_data;

        count = 0;
        is.read((char *)&count, sizeof(count));

        for (size_t i=0; i<count; ++i)
        {
            InputChannelData input_channel_data_item;
            
            if (input_channel_data_item.from_eprom(is) == false)
            {
                ERROR("error reading input A-Low channel data %d from eprom. corrupt?", (int) i)
                return false;
            }
            
            new_input_a_low_channel_data.push_back(input_channel_data_item);
        }

        if (is.bad())
        {
            ERROR("error reading input A-LOW channel data count from eprom. corrupt?")
            return false;
        }

        for (size_t i=0; i<config.input_a_low_channels.size(); ++i)
        {
            if (i < new_input_a_low_channel_data.size())
            {
                input_a_low_channel_data[i] = new_input_a_low_channel_data[i];
            }
            else
            {
                InputChannelData input_channel_data;
                input_channel_data.index = i;
                input_a_low_channel_data.push_back(input_channel_data);
            }
            
            //status.input_a_low_channels[i].calibration_coefficient =  input_a_low_channel_data[i].calibration_coefficient; 
        }

        _data_needs_save = false;  // output(..) sets this to true, but we just read the values from eprom 
    }
    else
    {
        ERROR("Failed to read mains-probe data from EPROM: version mismatch, expected %d, found %d", (int)DATA_EPROM_VERSION, (int)eprom_version)
        return false;
    }

    return !is.bad();
}


bool MainsProbeHandler::read_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);

    TRACE("MainsProbeHandler reading data from EEPROM")

    if (dataVolume.read() == true)
    {
        for (auto it = dataVolume.blocks.begin(); it != dataVolume.blocks.end(); ++it)
        {
            if(it->first == ftMainsProbe)
            {
                const char * function_type_str = function_type_2_str((FunctionType) it->first);
                TRACE("Found block type for function %s", function_type_str)

                std::istringstream is(it->second);

                if (data_from_eprom(is) == true)
                {
                    TRACE("MainsProbe data read success")

                    for (auto it=input_v_channel_data.begin(); it!=input_v_channel_data.end();++it)
                    {
                        DEBUG("input_v_channel data addr 0x%2x index %d", (int) it->addr, (int) it->index)
                        it->debug_x_2_y_map();
                    }

                    for (auto it=input_a_high_channel_data.begin(); it!=input_a_high_channel_data.end();++it)
                    {
                        DEBUG("input_a_high_channel data addr 0x%2x index %d", (int) it->addr, (int) it->index)
                        it->debug_x_2_y_map();
                    }

                    for (auto it=input_a_low_channel_data.begin(); it!=input_a_low_channel_data.end();++it)
                    {
                        DEBUG("input_a_low_channel data addr 0x%2x index %d", (int) it->addr, (int) it->index)
                        it->debug_x_2_y_map();
                    }

                    return true;
                }
                else
                {
                    TRACE("MainsProbe data read failure")
                }
            }
        }
    }
    else
    {
        ERROR("Cannot read eprom image (data)")
    }

    return false;
}

void MainsProbeHandler::save_data() 
{
    Lock lock(AutonomDataVolumeSemaphore);
    EpromImage dataVolume(AUTONOM_DATA_VOLUME);
    dataVolume.read();

    std::ostringstream os;

    TRACE("Saving mains-probe data to EEPROM")
    data_to_eprom(os);

    std::string buffer = os.str();
    TRACE("block size %d", (int) os.tellp())
    
    if (dataVolume.blocks.find((uint8_t) ftMainsProbe) == dataVolume.blocks.end())
    {
        dataVolume.blocks.insert({(uint8_t) ftMainsProbe, buffer});
    }
    else
    {
        if (dataVolume.blocks[(uint8_t) ftMainsProbe] == buffer)
        {
            TRACE("Data identical, skip saving")
            return;
        }
        else
        {
            dataVolume.blocks[(uint8_t) ftMainsProbe] = buffer;
        }
    }
    
    if (dataVolume.write())
    {
        TRACE("MainsProbe data save success")
    }
    else
    {
        TRACE("MainsProbe data save failure")
    }
}

void start_mains_probe_task(const MainsProbeConfig &config)
{
    if (handler.is_active())
    {
        ERROR("Attempt to start mains-probe task while it is running, redirecting to reconfigure")
        reconfigure_mains_probe(config);
    }
    else
    {
        handler.start(config);
    }
}

void stop_mains_probe_task()
{
    handler.stop();
}

MainsProbeStatus get_mains_probe_status()
{
    return handler.get_status();
}

void reconfigure_mains_probe(const MainsProbeConfig &_config)
{
    handler.reconfigure(_config);
}


bool __is_number_or_empty(const String & value)
{
    for (size_t i=0; i<value.length(); ++i)
    {
        if (!(isdigit(value[i]) || value[i] == '.'))
        {
            return false;
        } 
    }
    return true;
}

String mains_probe_calibrate_v(const String & addr_str, const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (!value_str.isEmpty())
            {
                if (__is_number_or_empty(value_str))
                {
                    float value = (float)  value_str.toFloat();
                    return handler.calibrate_v(i2c_addr_str_to_uint8_t(addr_str), channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_v(i2c_addr_str_to_uint8_t(addr_str), channel);
            }
        }
    }
    
    return "Parameter error";
}

String mains_probe_calibrate_a_high(const String & addr_str, const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (!value_str.isEmpty())
            {
                if (__is_number_or_empty(value_str))
                {
                    float value = (float)  value_str.toFloat();
                    return handler.calibrate_a_high(i2c_addr_str_to_uint8_t(addr_str), channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_a_high(i2c_addr_str_to_uint8_t(addr_str), channel);
            }
        }
    }
    
    return "Parameter error";
}

String mains_probe_calibrate_a_low(const String & channel_str, const String & value_str)
{
    bool param_ok = true;

    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();

            if (!value_str.isEmpty())
            {
                if (__is_number_or_empty(value_str))
                {
                    float value = (float)  value_str.toFloat();
                    return handler.calibrate_a_low(channel, value);
                }
            }
            else
            {
                return handler.uncalibrate_a_low(channel);
            }
        }
    }
    
    return "Parameter error";
}

String mains_probe_input_v(const String & addr_str, const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input_v(i2c_addr_str_to_uint8_t(addr_str), channel, value);

            if (r.isEmpty() == true)
            {
                char buf[64];
                sprintf(buf, "%.2f", value);
                value_str = buf;
            }
            
            return r;
        }
    }
    
    return "Parameter error";
}

String mains_probe_input_a_high(const String & addr_str, const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input_a_high(i2c_addr_str_to_uint8_t(addr_str), channel, value);

            if (r.isEmpty() == true)
            {
                char buf[64];
                sprintf(buf, "%.2f", value);
                value_str = buf;
            }
            
            return r;
        }
    }
    
    return "Parameter error";
}

String mains_probe_input_a_low(const String & channel_str, String & value_str)
{
    if (!channel_str.isEmpty())
    {
        if (__is_number_or_empty(channel_str))
        {
            size_t channel = (size_t)  channel_str.toInt();
            float value = -1;
        
            String r = handler.input_a_low(channel, value);

            if (r.isEmpty() == true)
            {
                char buf[64];
                sprintf(buf, "%.2f", value);
                value_str = buf;
            }
            
            return r;
        }
    }
    
    return "Parameter error";
}

void mains_probe_get_calibration_data(JsonVariant & json_variant)
{
    handler.get_calibration_data(json_variant);
}

String mains_probe_import_calibration_data(const JsonVariant & json)
{
    return handler.import_calibration_data(json);
}

#endif // INCLUDE_MAINSPROBE
