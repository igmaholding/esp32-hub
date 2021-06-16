#include <Arduino.h>

class BinarySemaphore
{
    public:

        BinarySemaphore()
        {
            handle = xSemaphoreCreateBinary();
            give(); // it is created in empty state and must be "given" first
        }

        ~BinarySemaphore()
        {
            vSemaphoreDelete(handle);
        }

        void take()
        {
            xSemaphoreTake(handle, portMAX_DELAY);
        }

        void give()
        {
            xSemaphoreGive(handle);
        }

        void take_isr()
        {
            BaseType_t dummy;
            xSemaphoreTakeFromISR(handle, & dummy);
        }

        void give_isr()
        {
            BaseType_t dummy;
            xSemaphoreGiveFromISR(handle, & dummy);
        }

    protected:

        SemaphoreHandle_t  handle; 

};

class Lock
{
    public:

        Lock(BinarySemaphore & _binary_semaphore)  
        {
            binary_semaphore = & _binary_semaphore; 
            binary_semaphore->take();
        }

        ~Lock()  
        {
            binary_semaphore->give();
        }

    protected:
        BinarySemaphore * binary_semaphore;
};


class LockISR
{
    public:

        LockISR(BinarySemaphore & _binary_semaphore)  
        {
            binary_semaphore = & _binary_semaphore; 
            binary_semaphore->take_isr();
        }

        ~LockISR()  
        {
            binary_semaphore->give_isr();
        }

    protected:
        BinarySemaphore * binary_semaphore;
};


