#include <Arduino.h>
#include <ArduinoJson.h>
#include <eeprom.h>

#include <trace.h>
#include <epromImage.h>


bool EpromImage::read()
{
    TRACE("read EPROM image")

    blocks.clear();

    size_t pos = 0;

    uint16_t _signature = 0xffff;
    
    if (EEPROM.readBytes(pos, & _signature, sizeof(_signature)) != sizeof(_signature))
    {
        TRACE("unexpected end of EEPROM")
        return false;
    }
    
    pos += sizeof(_signature);

    if (_signature == signature)
    {
        TRACE("signature found")

        uint8_t _version = 0;
        
        if (EEPROM.readBytes(pos, & _version, sizeof(_version)) != sizeof(_version))
        {
            TRACE("unexpected end of EEPROM")
            return false;
        }

        pos += sizeof(_version);

        if (_version == version)
        {
            TRACE("version match")

            uint8_t block_type = block_type_end;

            while(true)
            {
                if (EEPROM.readBytes(pos, & block_type, sizeof(block_type)) != sizeof(block_type))
                {
                    TRACE("unexpected end of EEPROM")
                    return false;
                }

                pos += sizeof(block_type);

                if (block_type == block_type_end)
                {
                    TRACE("found block_type_end, finished")
                    break;
                }

                uint8_t count = 0;

                if (EEPROM.readBytes(pos, & count, sizeof(count)) != sizeof(count))
                {
                    TRACE("unexpected end of EEPROM")
                    return false;
                }

                pos += sizeof(count);

                TRACE("reading block at pos %d, block_type %d, block_size %d", (int) (pos-sizeof(block_type)-sizeof(count)), (int) block_type, (int) count)

                uint8_t buf[256];

                if (count > sizeof(buf))
                {   
                    TRACE("block is lager than buffer (%d), will be truncated", (int) sizeof(buf))
                    count = (uint8_t) sizeof(buf);
                }

                if (EEPROM.readBytes(pos, buf, count) != count)
                {
                    TRACE("unexpected end of EEPROM")
                    return false;
                }

                pos += count;

                blocks.insert({block_type,std::string((const char*) buf, count)});
            }
        }
        else
        {
            TRACE("version mismatch")
            return false;
        }
    }
    else
    {
        TRACE("no signature, EPROM is empty?")
        return false;
    }

    return true;
}


bool EpromImage::write() const
{
    TRACE("write EPROM image")

    size_t pos = 0;

    if (EEPROM.writeBytes(pos, & signature, sizeof(signature)) != sizeof(signature))
    {
        TRACE("unexpected end of EEPROM")
        return false;
    }
    
    pos += sizeof(signature);

    if (EEPROM.writeBytes(pos, & version, sizeof(version)) != sizeof(version))
    {
        TRACE("unexpected end of EEPROM")
        return false;
    }

    pos += sizeof(version);

    auto it = blocks.begin();

    while(it != blocks.end())
    {
        uint8_t block_type = it->first;
        uint8_t count = it->second.length();

        TRACE("writing block at pos %d, block_type %d, block_size %d", (int) (pos), (int) block_type, (int) count)

        if (EEPROM.writeBytes(pos, & block_type, sizeof(block_type)) != sizeof(block_type))
        {
            TRACE("unexpected end of EEPROM")
            return false;
        }

        pos += sizeof(block_type);

        if (EEPROM.writeBytes(pos, & count, sizeof(count)) != sizeof(count))
        {
            TRACE("unexpected end of EEPROM")
            return false;
        }

        pos += sizeof(count);

        if (EEPROM.writeBytes(pos, it->second.c_str(), count) != count)
        {
            TRACE("unexpected end of EEPROM")
            return false;
        }

        pos += count;
        it++;
    }

    TRACE("writing block_type_end at pos %d", (int) (pos))
    if (EEPROM.writeBytes(pos, & block_type_end, sizeof(block_type_end)) != sizeof(block_type_end))
    {
        TRACE("unexpected end of EEPROM")
        return false;
    }

    TRACE("EEPROM write commit")
    EEPROM.commit();
    return true;
}


bool EpromImage::diff(const EpromImage & other, std::vector<uint8_t> * added, std::vector<uint8_t> * removed,
                      std::vector<uint8_t> * changed) const
{
    bool r = false;

    // first ensure that us and the other contains the same set of keys

    for (auto it=blocks.begin(); it != blocks.end(); ++it)
    {
        if (other.blocks.find(it->first) == other.blocks.end())
        {
            if (added != NULL)
            {
                added ->push_back((uint8_t) it->first);
            }
            r = true;
        }
    }

    for (auto it=other.blocks.begin(); it != other.blocks.end(); ++it)
    {
        if (blocks.find(it->first) == blocks.end())
        {
            if (removed != NULL)
            {
                removed->push_back((uint8_t) it->first);
            }
            r = true;            
        }
    }

    // then compare contents of blocks present in both

    for (auto it=blocks.begin(); it != blocks.end(); ++it)
    {
        auto other_it = other.blocks.find(it->first);
        
        if (other_it != other.blocks.end())
        {
            if (it->second != other_it->second)
            {
                if (changed != NULL)
                {
                    changed->push_back((uint8_t) it->first);
                }
                r = true;
            }
        }
    }

    return r;
}

