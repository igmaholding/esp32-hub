#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <string>


class EpromImage
{
    public:

        const uint8_t version = 1;
        const uint16_t signature = 0xe5d0;
        const uint8_t block_type_end = 0xff;

        EpromImage()
        {
        }

        bool read();
        bool write() const;

        bool diff(const EpromImage & other, std::vector<uint8_t> * added = NULL, std::vector<uint8_t> * removed = NULL,
                 std::vector<uint8_t> * changed = NULL) const;

        std::map<uint8_t, std::string> blocks;
};
