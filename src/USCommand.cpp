#include <stdlib.h>
#include "USCommand.h"

/**
 * Command format:
 * !nnn.nnn.nnn.nnn:nnnnn/xxx/yyy/zzz?abc=10&xyz=11|<CRC>$
 * Example:
 * !1:10/s$         -- start module 10
 * !1:10/e$         -- end module 10
 * !1:10/b?t=10$    -- begin module 10 with param `t` set to 10
 * !1:3/format$     -- format module 3
 * !1:3/w?0=1&1=2$  -- write to module 3 with addr 0 set to 1 and addr 1 set to 2
*/


/**
 * internal state
 * 1. bStart
 * 2. bEnd
 * 3. bDevice
 * 4. eDevice
 * 5. bModule
 * 6. eModule
 * 7. bDesignation
 * 8. bParamKey
 * 9. bParamValue
 * 10. bCRC
*/
enum {
    bBegin,
    bEnd,
    bDevice,
    //eDevice,
    bModule,
    //eModule,
    bDesignation,
    bParamkey,
    bParamValue,
    bCRC
};

static inline bool isEmpty(char c) {
    switch(c) {
    case ' ':
    case '\r':
    case '\n':
    case '\t':
        return true;
    }

    return false;
}
static inline bool isDigit(char c) {
    switch(c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    }
    return false;
}
/*
static inline bool isHex(char c) {
    switch(c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'A':
    case 'b':
    case 'B':
    case 'c':
    case 'C':
    case 'd':
    case 'D':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
        return true;
    }
    return false;
}
*/
static inline char unescape(char c) {
    switch(c) {
    case 'r':
        return '\r';
    case 'n':
        return '\n';
    case 't':
        return '\t';
    case 'b':
        return '\b';
    case '\\':
    case '&':
    case '$':
    case '=':
    case '|':
        return c;
    }
    return 0;
}

USCommand::USCommand() {
    reset();
}

void USCommand::reset() {
    _pc = '\0';
    _dp = 0;
    _ni = 0;
    _begin = 0;
    _state = bBegin;
    _device = USC_InvalidDevice;
    _module = 0;
}

uint32_t USCommand::device(void) const {
    return _device;
}

uint16_t USCommand::module(void) const {
    return _module;
}

bool USCommand::isBroadcast(void) const {
    return _device == USC_BROADCAST_ADDR;
}
bool USCommand::isResponse(void) const {
    return _begin == '@';
}

USC_Result USCommand::parseBegin(char c) {
    switch(c) {
    case '!':
        _state = bDevice;
        _dp = 0;
        _ni = 0;
        _begin = c;
        return USC_Continue;
    case '@':
        _state = bEnd;
        _begin = c;
        return USC_Continue;
    default:
        if (isEmpty(c)) {
            return USC_Continue;
        }
    }

    return USC_Unexpected;     
}
USC_Result USCommand::parseEnd(char c) {
    if (_pc == '\\') {
        if (unescape(c) != 0) {
            return USC_Continue;
        }
        return USC_Unexpected;
    } else if (c == '$') {
        _state = bBegin;
        return USC_OK;
    }

    return USC_Continue;
}
USC_Result USCommand::convertDevice(char c, uint8_t ns, USC_Result res) {
    // invalid address segment count
    if (_ni >= 4) {
        return USC_Unexpected;
    } else if (_ni == 0) {
        _device = 0;
    }

    // convert and assign address
    int v = atoi(_data);
    _device <<= 4;
    _device |= 0x00ff & v;
    _ni++;
    _state = ns;
    _dp = 0;

    return res;
}

USC_Result USCommand::convertModule(char c, uint8_t ns, USC_Result res) {
    // convert and assign address
    _module = (uint16_t)atol(_data);
    _state = ns;
    _dp = 0;

    return res;
}


USC_Result USCommand::parseDevice(char c) {
    // check for overflow
    if (_dp >= USC_BUFSIZE) {
        return USC_Overflow;
    } else if (isDigit(c)) {
        // no more than 3 digits / segment
        if (_dp >= 3) {
            return USC_Unexpected;
        }
        _data[_dp++] = c;
        return USC_Continue;
    }

    // terminate buffer
    _data[_dp] = 0;

    switch(c) {
    case '-':
    case '_':
    case '.':
        return convertDevice(c, bDevice);
    case ':':
        return convertDevice(c, bModule);
    case '/':
        return convertDevice(c, bDesignation);
    case '|':
        return convertDevice(c, bCRC);
    case '$':
        return convertDevice(c, bBegin, USC_OK);
    }

    return USC_Unexpected;
}
USC_Result USCommand::parseModule(char c) {
    // check for overflow
    if (_dp >= USC_BUFSIZE) {
        return USC_Overflow;
    } else if (isDigit(c)) {
        // ensure initialized
        if (_pc == ':') {
            _dp = 0;
            _module = 0;
        }

        // no more than 5 digits (16-bits)
        if (_dp >= 5) {
            return USC_Unexpected;
        }
        _data[_dp++] = c;
        return USC_Continue;
    }

    // terminate buffer
    _data[_dp] = 0;

    switch(c) {
    case '/':
        return convertModule(c, bDesignation);
    case '|':
        return convertModule(c, bCRC);
    case '$':
        return convertModule(c, bBegin, USC_OK);
    }

    return USC_Unexpected;
}
USC_Result USCommand::parseDesignation(char c) {
    if (_dp >= USC_BUFSIZE) {
        return USC_Overflow;
    }
}
USC_Result USCommand::parseParam(char c) {
    if (_dp >= USC_BUFSIZE) {
        return USC_Overflow;
    }
}
USC_Result USCommand::parseCRC(char c) {
    if (_dp >= USC_BUFSIZE) {
        return USC_Overflow;
    }
}

USC_Result USCommand::parse(char c) {
    // process
    USC_Result res;
    switch (_state){
    case bBegin:
        res = parseBegin(c);
        break;
    case bEnd:
        res = parseEnd(c);
        break;
    case bDevice:
        res = parseDevice(c);
        break;
    case bModule:
        res = parseModule(c);
        break;
    case bDesignation:
        res = parseDesignation(c);
        break;
    case bParamkey:
    case bParamValue:
        res = parseParam(c);
        break;
    case bCRC:
        res = parseCRC(c);
        break;
    default:
        res = USC_Unexpected;
        break;
    }

    // save character as previous
    _pc = c;

    return res;
}