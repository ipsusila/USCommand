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

#define PARAM_END 0x00
#define PARAM_KEY 0x01
#define PARAM_VAL 0x02

enum {
    bBegin,
    bEnd,
    bDevice,
    bModule,
    bDesignation,
    bParamKey,
    bParamValue,
    bCRC
};

static inline int toInt(char *pb, char *pe) {
    char old = *pe;
    *pe = 0;
    int v = atoi(pb);
    *pe = old;

    return v;
}

static inline long toLong(char *pb, char *pe) {
    char old = *pe;
    *pe = 0;
    long v = atol(pb);
    *pe = old;

    return v;
}

static inline bool isValidKey(char c) {
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') || (c == '-' || c == '_' || c == '.'));
}

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

USParam::USParam() {
    clear();
}
void USParam::clear() {
    _key = nullptr;
    _value = nullptr;
    _next = nullptr;
    _end = nullptr;
}

bool USParam::parse() {
    if (_next == nullptr || _next >= _end) {
        return false;
    }
    _key = _next;
    _value = nullptr;

    char *p = _next;
    while (p != _end) {
        switch (*p) {
        case PARAM_END:
        case PARAM_KEY:
            *p = 0;
            _next = ++p;
            return true;
        case PARAM_VAL:
            _key = _next;
            _value = p+1;
            *p = 0;
            break;
        }
        p++;
    }
    return false;
}

bool USParam::valid() const {
    return _next != nullptr && _next < _end;
}

bool USParam::hasValue() const {
    return _value != nullptr;
}
char * USParam::key() {
    return _key;
}
char * USParam::value() {
    return _value;
}
int USParam::valueInt(int def) const {
    if (_value) {
        return atoi(_value);
    }
    return def;
}
long USParam::valueLong(long def) const {
    if (_value) {
        return atol(_value);
    }
    return def;
}
float USParam::valueFloat(float def) const {
    if (_value) {
        return atof(_value);
    }
    return def;
}


USCommand::USCommand() {
    clear();
}

void USCommand::clear(void) {
    _pc = '\0';
    _np = 0;
    _ni = 0;
    _state = bBegin;
    _device = USC_InvalidDevice;
    _module = 0;
    _record = false;
    _checksum = 0;
    _hasChecksum = false;
    _data[0] = 0;
    _data[1] = 0;
    for (uint8_t i = 0; i < USC_ElementsCount; i++) {
        _pos[i] = 0;
    }
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
    return _data[0] == '@';
}
char * USCommand::data(char prefix) {
    if (prefix != 0) {
        _data[0] = prefix;
    }
    return _data;
}
char * USCommand::beginResponse() {
    _data[0] = '@';
    return _data;
}
char USCommand::endResponse() const {
    return '$';
}

uint8_t USCommand::checksum(void) const {
    return _checksum;
}
bool USCommand::hasChecksum(void) const {
    return _hasChecksum;
}

char * USCommand::designation(void) {
    int p = _pos[USC_DesignationPos];
    if (p == 0) {
        return nullptr;
    }
    return _data+p;
}

bool USCommand::hasDesignation() const {
    return _pos[USC_DesignationPos] != 0;
}

bool USCommand::hasParam() const {
    return _param.valid();
}
USParam * USCommand::param() {
    return &_param;
}
char * USCommand::paramKey() {
    return _param._key;
}
char USCommand::paramKeyChar() const {
    if (_param._key) {
        return *(_param._key);
    }
    return 0;
}
bool USCommand::nextParam() {
    return _param.parse();
}

USC_Result USCommand::convertDevice(char c, uint8_t ns, USC_Result res) {
    if (_ni == 0) {
        _device = 0;
    }
    _ni++;

    // invalid address segment count
    if (_ni > 4) {
        return USC_Unexpected;
    } else if ((_np - _bp) > 4) {
        // more than 3 digits
        return USC_Unexpected;
    }

    // convert and assign address
    int v = toInt(&_data[_bp], &_data[_np]);
    if (v < 0 || v > 255) {
        return USC_Overflow;
    }
    _device <<= 4;
    _device |= 0x00ff & v;
    _state = ns;
    _bp = _np;

    return res;
}

USC_Result USCommand::convertModule(char c, uint8_t ns, USC_Result res) {
    if ((_np - _bp) > 6) {
        // more than 5 digits
        return USC_Unexpected;
    }

    // convert and assign address
    _module = (uint16_t)toLong(&_data[_bp], &_data[_np]);
    _state = ns;
    _bp = _np;

    return res;
}

USC_Result USCommand::parseBegin(char c) {
    switch(c) {
    case '!':
        _state = bDevice;
        _np = 0;
        _bp = 1;
        _ni = 0;
        _record = true;
        _data[_np++] = c;
        _pos[USC_DevicePos] = _np;
        _checksum ^= (uint8_t)c;
        return USC_Next;
    case '@':
        _state = bEnd;
        _np = 0;
        _data[_np++] = c;
        _data[_np] = 0;
        return USC_Next;
    default:
        if (isEmpty(c)) {
            return USC_Next;
        }
    }

    return USC_Unexpected;     
}
USC_Result USCommand::parseEnd(char c) {
    if (_pc == '\\') {
        char ec = unescape(c);
        if (ec != 0) {
            if (ec == '\\') {
                _pc = 0;
            } else {
                _pc = c;
            }
            return USC_Next;
        }
        return USC_Unexpected;
    } else if (c == '$') {
        _state = bBegin;
        _record = false;
        return USC_OK;
    }

    return USC_Next;
}

USC_Result USCommand::parseDevice(char c) {
    // wait terminated
    if (isDigit(c)) {
        return USC_Next;
    }

    switch(c) {
    case '-':
    case '_':
    case '.':
        return convertDevice(c, bDevice);
    case ':':
        // set default module address
        _module = USC_DEFAULT_MODULE;
        _pos[USC_ModulePos] = _np;
        return convertDevice(c, bModule);
    case '/':
        _pos[USC_DesignationPos] = _np;
        return convertDevice(c, bDesignation);
    case '|':
        _pos[USC_CRCPos] = _np;
        _bp = _np;
        _data[_np-1] = 0;
        return convertDevice(c, bCRC);
    case '$':
        _data[_np-1] = 0;
        return convertDevice(c, bBegin, USC_OK);
    }

    return USC_Unexpected;
}
USC_Result USCommand::parseModule(char c) {
    // check for overflow
    if (isDigit(c)) {
        return USC_Next;
    }

    switch(c) {
    case '/':
        _pos[USC_DesignationPos] = _np;
        return convertModule(c, bDesignation);
    case '|':
        _pos[USC_CRCPos] = _np;
        _bp = _np;
        _data[_np-1] = 0;
        return convertModule(c, bCRC);
    case '$':
        _data[_np-1] = 0;
        return convertModule(c, bBegin, USC_OK);
    }

    return USC_Unexpected;
}
USC_Result USCommand::parseDesignation(char c) {
    // Check if c is valid
    if (isValidKey(c) || (c == '/' && _pc != '/')) {
        return USC_Next;
    }

    switch(c) {
    case '?':
        _pos[USC_DesignationEndPos] = _np - 1;
        _pos[USC_ParamPos] = _np;
        _state = bParamKey;
        _param._next = &_data[_np];
        _data[_np-1] = 0;
        return USC_Next;
    case '|':
        _pos[USC_DesignationEndPos] = _np - 1;
        _pos[USC_CRCPos] = _np;
        _state = bCRC;
        _bp = _np;
        _data[_np-1] = 0;
        return USC_Next;
    case '$':
        _pos[USC_DesignationEndPos] = _np - 1;
        _state = bBegin;
        _data[_np-1] = 0;
        return USC_OK;
    }
    return USC_Unexpected;
}
USC_Result USCommand::parseParamKey(char c) {
    // Check if c is valid
    if (isValidKey(c)) {
        return USC_Next;
    }
    switch(c) {
    case '=':
        _state = bParamValue;
        _data[_np-1] = PARAM_VAL;
        return USC_Next;
    case '|':
        _pos[USC_ParamEndPos] = _np - 1;
        _pos[USC_CRCPos] = _np;
        _state = bCRC;
        _bp = _np;
        _data[_np-1] = PARAM_END;
        _param._end = &_data[_np];
        return USC_Next;
    case '$':
        _pos[USC_ParamEndPos] = _np - 1;
        _state = bBegin;
        _data[_np-1] = PARAM_END;
        _param._end = &_data[_np];
        return USC_OK;
    }
    return USC_Unexpected;
}
USC_Result USCommand::parseParamValue(char c) {
    switch(c) {
    case '&':
        _state = bParamKey;
        _data[_np-1] = PARAM_KEY;
        return USC_Next;
    case '|':
        _pos[USC_ParamEndPos] = _np - 1;
        _pos[USC_CRCPos] = _np;
        _state = bCRC;
        _bp = _np;
        _data[_np-1] = PARAM_END;
        _param._end = &_data[_np];
        return USC_Next;
    case '$':
        _pos[USC_ParamEndPos] = _np - 1;
        _state = bBegin;
        _data[_np-1] = PARAM_END;
        _param._end = &_data[_np];
        return USC_OK;
    }
    return USC_Next;
}
USC_Result USCommand::parseCRC(char c) {
    // TODO: efficient calculation method
    if (isDigit(c)) {
        return USC_Next;
    } else if (c == '$') {
        _state = bBegin;
        _hasChecksum = true;

        // compare checksum
        uint8_t chk = (uint8_t)toInt(&_data[_bp], &_data[_np]);
        if (chk != _checksum) {
            return USC_Invalid;
        }
        return USC_OK;
    }
    return USC_Unexpected;
}

USC_Result USCommand::parse(char c) {
    if (_record) {
        if (_state != bCRC && _state != bEnd) {
            _checksum ^= (uint8_t)c;
        }
        // Save to buffer
        if (_np >= USC_BUFSIZE) {
            return USC_Overflow;
        } else if (_pc == '\\') {
            if (_state != bParamValue) {
                // escape char only allowed in param value
                return USC_Unexpected;
            }
            char ec = unescape(c);
            if (ec != 0) {
                _data[_np-1] = ec;
                _data[_np] = 0;
                if (ec == '\\') {
                    _pc = 0;
                } else {
                    _pc = c;
                }
                return USC_Next;
            }
            return USC_Invalid;
        }
        _data[_np++] = c;
        _data[_np] = 0;
    }

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
    case bParamKey:
        res = parseParamKey(c);
        break;
    case bParamValue:
        res = parseParamValue(c);
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
