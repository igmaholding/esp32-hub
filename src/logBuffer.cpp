#include <Arduino.h>
#include <logBuffer.h>
#include <trace.h>

#define LOG_BUFFER_SIZE 4096

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

        const char * entryType2str(EntryType entryType)
        {
          switch(entryType)
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
          BlockHeader bh;
          bh.payloadSize = sizeof(buffer) - sizeof(bh);

          *((BlockHeader *)(buffer)) = bh;
          dataLost = false;
          head = 0;
          tail = 0;
        }

        void addEntry(const char *, LogBuffer::EntryType entryType);

   protected:

        struct BlockHeader
        {
          BlockHeader()
          {
            isUsed = false;
            payloadSize = 0;
            timestamp = 0;
            entryType = etTrace;
          }

          BlockHeader(unsigned _payloadSize, EntryType _entryType)
          {
            isUsed = true;
            payloadSize = _payloadSize;
            entryType = _entryType;
          }

           bool isUsed;
           unsigned payloadSize;
           unsigned long timestamp;
           EntryType entryType;
        };

        unsigned char buffer[LOG_BUFFER_SIZE];  
        unsigned head;
        unsigned tail;
        bool dataLost;   
};


static LogBuffer logBuffer;


void LogBuffer::addEntry(const char * text, LogBuffer::EntryType entryType)
{
  unsigned payloadSize = strlen(text);

  BlockHeader bh(payloadSize, entryType);



}


static void _push_line(const char * str, const char * prefix)
{

}


void writeLog(const char * str, const char * prefix = NULL)
{
  if (prefix) 
  {
    Serial.write(prefix);
  }
  
  Serial.write(str);
  Serial.println("");
  _push_line(str, prefix);
}


void traceLog(const char * str)
{
  writeLog(str, "TRACE ");
}


void debugLog(const char * str)
{
  writeLog(str, "DEBUG ");
}


void errorLog(const char * str)
{
  writeLog(str, "ERROR ");
}


String popLog()
{
  TRACE("popLog")  

  String log;

  return log;
}


