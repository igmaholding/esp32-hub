#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>

#include <logBuffer.h>
#include <trace.h>

#define LOG_BUFFER_SIZE 4096

String tm_2_str(const tm & _tm)
{
  char buf[128];
  sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d", _tm.tm_year+1900, _tm.tm_mon+1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
  return String(buf);
}


String time_t_2_str(time_t _time_t)
{
  tm _tm = {0};
  _tm = *localtime(&_time_t);
  return tm_2_str(_tm);
}


// memory aware ring buffer for log entries

class LogBuffer
{
   public:

        enum EntryType
        {
          etTrace = 0,
          etDebug = 1,
          etError = 2
        };

        static const char * entry_type_2_str(EntryType entry_type)
        {
          switch(entry_type)
          {
             case etTrace:
                  return "TRACE"; 
             case etDebug:
                  return "DEBUG"; 
             case etError:
                  return "ERROR";                   
          }
          return "";  
        }

        LogBuffer()
        {
          reset();
        }

        void reset()
        {
          Block block(buffer, sizeof(buffer));
          block.set_block_info();

          head = 0;
          tail = size_t(-1);

          data_lost = false;
        }

        void add_entry(const char *, LogBuffer::EntryType entry_type);

        void pop(JsonVariant & json);

   protected:

        struct BlockInfo
        {
          BlockInfo(const unsigned char * block_start, size_t block_size)
          {
            *this = *((const BlockInfo*)(block_start));
          }

          BlockInfo()
          {
            payload_size = 0;
            timestamp = 0;
            entry_type = etTrace;
          }

          BlockInfo(unsigned _payload_size, EntryType _entry_type, unsigned long _timestamp = 0)
          {
            payload_size = _payload_size;
            entry_type = _entry_type;
            timestamp = _timestamp;
          }

           size_t payload_size : 16;
           EntryType entry_type : 2;
           time_t timestamp; 
        };

        struct Block
        {
          Block(unsigned char * _block_start, size_t _block_size)
          {
            block_start = _block_start;
            block_size = _block_size;
          }

          Block(unsigned char * _block_start)
          {
            block_start = _block_start;
            const BlockInfo & block_info = *((const BlockInfo*)(_block_start));
            block_size = block_info.payload_size+sizeof(BlockInfo);
          }

          bool operator == (const Block & other) const 
          {
            return block_start == other.block_start && block_size == other.block_size;
          }

          Block split_off(size_t new_payload_size)
          {
            BlockInfo & block_info = get_block_info();

            if (new_payload_size >= block_info.payload_size)
            {
               return INVALID_BLOCK; 
            }
            else
            {
              size_t new_block_size = sizeof(BlockInfo) + new_payload_size;
              block_info.payload_size = new_payload_size;

              if (block_size - new_block_size <= sizeof(BlockInfo))
              {
                 return INVALID_BLOCK;
              }

              Block next_block((unsigned char*) (block_start + new_block_size), block_size - new_block_size);  
              next_block.set_block_info();

              block_size = new_block_size;

              return next_block;           
            }
          }

          void set_block_info(EntryType entry_type = etTrace, unsigned long timestamp = 0)
          {
            BlockInfo bi;
            
            if (block_size >= sizeof(bi)) 
            {
              bi.entry_type = entry_type;
              bi.timestamp = timestamp;

              bi.payload_size = block_size - sizeof(bi);

              *((BlockInfo*)(block_start)) = bi;
            }
          }
          
          BlockInfo & get_block_info()
          {
            return *((BlockInfo*)(block_start));
          }

          void set_payload(const unsigned char * payload)
          {
            BlockInfo & block_info = get_block_info();
            memcpy((unsigned char*) (block_start + sizeof(BlockInfo)), payload, block_info.payload_size);
          }
          
          unsigned char * get_payload()
          {
            return block_start + sizeof(BlockInfo);
          }

          unsigned char * block_start;
          size_t block_size;
          static const Block INVALID_BLOCK;
        };


        unsigned char buffer[LOG_BUFFER_SIZE];  

        size_t head; // position of the next free block
        size_t tail; // position of the earliest available block

        bool data_lost;   
};

const LogBuffer::Block LogBuffer::Block::INVALID_BLOCK(NULL, 0);


void LogBuffer::add_entry(const char * text, LogBuffer::EntryType entry_type)
{
  unsigned payload_size = strlen(text);

  //Serial.printf("#enter add_entry: sizeof(BlockInfo)=%d, head=%d, tail=%d, payload_size=%d\n", (int) sizeof(BlockInfo), (int) head, (int) tail, (int) payload_size);

  if (head < sizeof(buffer) && sizeof(buffer)-head > sizeof(BlockInfo))
  {
    Block head_block(buffer+head, sizeof(buffer)-head);
    BlockInfo & head_block_info = head_block.get_block_info();

    if (head_block_info.payload_size < payload_size)
    {
      data_lost = true;
    }

    Block next_block = head_block.split_off(payload_size);

    if (next_block == Block::INVALID_BLOCK)
    {
      head = sizeof(buffer);
    }
    else
    {
      head = (size_t)(next_block.block_start - buffer);
    }

    if (tail == size_t(-1))
    {
      tail = 0;
    }

    time(&head_block_info.timestamp);
    head_block.set_payload((const unsigned char*)(text));

    //Serial.printf("#success new: head=%d, tail=%d\n", (int) head, (int) tail);

    if (!(next_block == Block::INVALID_BLOCK))
    {
      //BlockInfo & next_block_info = next_block.get_block_info();

      //Serial.printf("#split next_block: entry_type=%d, payload_size=%d\n", (int) next_block_info.entry_type, 
      //             (int) next_block_info.payload_size);
    }
  }
  else
  {
    //Serial.printf("#outofspace\n");
    data_lost = true;
  } 
}


void LogBuffer::pop(JsonVariant & json)
{  
  json["data_lost"] = data_lost;

  json.createNestedArray("entries");
  JsonArray jsonArrayEntries = json["entries"]; 

  static char text[512];  

  if (tail != size_t(-1))
  {
    size_t pp = tail;

    while(pp < head)
    {
      //JsonObject entry = jsonArrayEntries.createNestedObject(); 
      
      Block block(buffer+pp); 
      BlockInfo & block_info = block.get_block_info();

      size_t pos = 0;

      String timestamp_str = time_t_2_str(block_info.timestamp);

      memcpy(text + pos, timestamp_str.c_str(), timestamp_str.length());
      pos += timestamp_str.length();
      text[pos] = ' ';
      pos++;

      const char * entry_type_str = entry_type_2_str(block_info.entry_type);
      strcpy(text + pos, entry_type_str);
      pos += strlen(entry_type_str);
      text[pos] = ' ';
      pos++;

      size_t c = sizeof(text)-pos-1;

      if (c > block_info.payload_size)
      {
        c = block_info.payload_size;
      } 

      memcpy(text + pos, block.get_payload(), c);
      pos += c;

      text[pos] = 0;

      //entry["td"] = block_info.time_delta;
      //entry["t"] = text;
      jsonArrayEntries.add(text);

      pp += block.block_size;
    }

    reset();
  }
}


static LogBuffer logBuffer;


void writeLog(const char * str, LogBuffer::EntryType entry_type)
{
  Serial.write(LogBuffer::entry_type_2_str(entry_type));
  Serial.write(" ");
  Serial.write(str);
  Serial.println("");

  logBuffer.add_entry(str, entry_type);
}


void traceLog(const char * str)
{
  writeLog(str, LogBuffer::etTrace);
}


void debugLog(const char * str)
{
  writeLog(str, LogBuffer::etDebug);
}


void errorLog(const char * str)
{
  writeLog(str, LogBuffer::etError);
}


void popLog(JsonVariant & json)
{
  TRACE("popLog") 
  logBuffer.pop(json); 
}


