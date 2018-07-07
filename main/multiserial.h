#pragma once

#include <Arduino.h>

class MultiSerial : public Stream
{
    public:
        MultiSerial();

        void addInterface(Stream*);
        void enableInterface(Stream* interface);
        void disableInterface(Stream* interface);

        virtual int available();
        virtual int peek();
        virtual int read();
        virtual void flush();

        virtual size_t write(uint8_t);
        inline size_t write(unsigned long n) {return write((uint8_t)n);};
        inline size_t write(long n) {return write((uint8_t)n);};
        inline size_t write(unsigned int n) {return write((uint8_t)n);};
        inline size_t write(int n) {return write((uint8_t)n);};
        using Print::write;
        operator bool() {return true;};

    private:
        unsigned long baud;
        uint8_t mode;

        bool interfacesEnabled[5];
        Stream* interfaces[5];
        uint8_t interfaceCount = 0;
};
