#include "multiserial.h"

MultiSerial::MultiSerial() {}

void MultiSerial::addInterface(Stream* interface) {
    interfaces[interfaceCount] = interface;
    interfacesEnabled[interfaceCount] = true;
    interfaceCount++;
}

void MultiSerial::enableInterface(Stream* interface) {
    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfaces[i] == interface) {
            interfacesEnabled[i] = true;
        }
    }
}

void MultiSerial::disableInterface(Stream* interface) {
    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfaces[i] == interface) {
            interfacesEnabled[i] = false;
        }
    }
}

int MultiSerial::available() {
    int available = 0;

    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfacesEnabled[i]) {
            available += interfaces[i]->available();
        }
    }

    return available;
}

int MultiSerial::peek() {
    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfacesEnabled[i]) {
            if(interfaces[i]->available()) {
                return interfaces[i]->peek();
            }
        }
    }

    return -1;
}

int MultiSerial::read() {
    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfacesEnabled[i]) {
            if(interfaces[i]->available()) {
                return interfaces[i]->read();
            }
        }
    }

    return -1;
}

void MultiSerial::flush() {
    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfacesEnabled[i]) {
            interfaces[i]->flush();
        }
    }
}

size_t MultiSerial::write(uint8_t value) {
    size_t count = 0;

    for(uint8_t i = 0; i < interfaceCount; i++) {
        if(interfacesEnabled[i]) {
            count += interfaces[i]->write(value);
        }
    }

    return count;
}
