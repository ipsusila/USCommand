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

enum
{
    bBegin,
    bEnd,
    bDevice,
    bModule,
    bAction,
    bParamKey,
    bParamValue,
    bCRC
};

static inline int toInt(char *pb, char *pe)
{
    char old = *pe;
    *pe = 0;
    int v = atoi(pb);
    *pe = old;

    return v;
}

static inline long toLong(char *pb, char *pe)
{
    char old = *pe;
    *pe = 0;
    long v = atol(pb);
    *pe = old;

    return v;
}

static inline bool isValidKey(char c)
{
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || (c == '-' || c == '_' || c == '.'));
}

static inline bool isEmpty(char c)
{
    switch (c)
    {
    case ' ':
    case '\r':
    case '\n':
    case '\t':
        return true;
    }

    return false;
}
static inline bool isDigit(char c)
{
    switch (c)
    {
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
static inline char unescape(char c)
{
    switch (c)
    {
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

// Begin implementation
namespace usc
{

    KeyVal::KeyVal(const char *k, const char *v)
        : _key(k), _value(v) {
    }
    KeyVal::KeyVal(const KeyVal &kv) {
        _key = kv._key;
        _value = kv._value;
    }
    KeyVal &KeyVal::operator=(const KeyVal &kv) {
        _key = kv._key;
        _value = kv._value;
        return *this;
    }

    const char *KeyVal::key() const {
        return _key;
    }
    const char KeyVal::keyChar() const {
        return _key ? *_key : 0;
    }
    const char *KeyVal::value() const {
        return _value;
    }
    bool KeyVal::hasValue() const {
        return _value != nullptr;
    }
    int KeyVal::valueInt(int def) const {
        return _value ? atoi(_value) : def;
    }
    long KeyVal::valueLong(long def) const {
        return _value ? atol(_value) : def;
    }
    float KeyVal::valueFloat(float def) const {
        return _value ? atof(_value) : def;
    }

    Param::Param()
    {
        clear();
    }
    void Param::clear()
    {
        _key = nullptr;
        _value = nullptr;
        _next = nullptr;
        _end = nullptr;
        _count = 0;
    }

    bool Param::parse()
    {
        if (_next == nullptr || _next >= _end)
        {
            return false;
        }
        _key = _next;
        _value = nullptr;

        char *p = _next;
        while (p != _end)
        {
            switch (*p)
            {
            case PARAM_END:
            case PARAM_KEY:
                *p = 0;
                _next = ++p;
                return true;
            case PARAM_VAL:
                _key = _next;
                _value = p + 1;
                *p = 0;
                break;
            }
            p++;
        }
        return false;
    }

    int Param::count() const {
        return _count;
    }

    bool Param::valid() const
    {
        return _next != nullptr && _next < _end;
    }

    bool Param::hasValue() const
    {
        return _value != nullptr;
    }
    char *Param::key()
    {
        return _key;
    }
    char Param::keyChar() const {
        return _key ? *_key : 0;
    }
    char *Param::value()
    {
        return _value;
    }
    int Param::valueInt(int def) const
    {
        return _value ? atoi(_value) : def;
    }
    long Param::valueLong(long def) const
    {
        return _value ? atol(_value) : def;
    }
    float Param::valueFloat(float def) const
    {
        return _value ? atof(_value) : def;
    }
    KeyVal Param::kv() const {
        return KeyVal(_key, _value);
    }


    Command::Command()
    {
        clear();
    }

    void Command::clear(void)
    {
        _pc = '\0';
        _np = 0;
        _ni = 0;
        _state = bBegin;
        _device = InvalidDevice;
        _module = 0;
        _capture = false;
        _checksum = 0;
        _hasChecksum = false;
        _data[0] = 0;
        _data[1] = 0;
        _action = nullptr;
        _param.clear();
    }

    uint32_t Command::device(void) const
    {
        return _device;
    }

    uint16_t Command::module(void) const
    {
        return _module;
    }

    bool Command::isBroadcast(void) const
    {
        return _device == USC_BROADCAST_ADDR;
    }
    bool Command::isResponse(void) const
    {
        return _data[0] == '@';
    }
    char *Command::data(char prefix)
    {
        if (prefix != 0)
        {
            _data[0] = prefix;
        }
        return _data;
    }
    char *Command::beginResponse()
    {
        _data[0] = '@';
        return _data;
    }
    char Command::endResponse() const
    {
        return '$';
    }
    char *Command::beginResponseCheksum(uint8_t &chk)
    {
        chk = 0;
        char *resp = beginResponse();
        char *p = resp;
        while (*p != 0)
        {
            chk ^= *p;
            p++;
        }
        return resp;
    }

    uint8_t Command::checksum(void) const
    {
        return _checksum;
    }
    bool Command::hasChecksum(void) const
    {
        return _hasChecksum;
    }

    const char *Command::action(void) const
    {
        return _action;
    }

    bool Command::hasAction() const
    {
        return _action != nullptr && *_action != 0;
    }

    bool Command::hasParam() const
    {
        return _param.valid();
    }
    Param &Command::param()
    {
        return _param;
    }
    bool Command::nextParam()
    {
        return _param.parse();
    }

    Result Command::convertDevice(char c, uint8_t ns, Result res)
    {
        if (_ni == 0)
        {
            _device = 0;
        }
        _ni++;

        // invalid address segment count
        if (_ni > 4)
        {
            return Unexpected;
        }
        else if ((_np - _bp) > 4)
        {
            // more than 3 digits
            return Unexpected;
        }

        // convert and assign address
        int v = toInt(&_data[_bp], &_data[_np]);
        if (v < 0 || v > 255)
        {
            return Overflow;
        }
        _device <<= 4;
        _device |= 0x00ff & v;
        _state = ns;
        _bp = _np;

        return res;
    }

    Result Command::convertModule(char c, uint8_t ns, Result res)
    {
        if ((_np - _bp) > 6)
        {
            // more than 5 digits
            return Unexpected;
        }

        // convert and assign address
        _module = (uint16_t)toLong(&_data[_bp], &_data[_np]);
        _state = ns;
        _bp = _np;

        return res;
    }

    Result Command::parseBegin(char c)
    {
        switch (c)
        {
        case '!':
            _state = bDevice;
            _np = 0;
            _bp = 1;
            _ni = 0;
            _capture = true;
            _data[_np++] = c;
            _checksum ^= (uint8_t)c;
            return Next;
        case '@':
            _state = bEnd;
            _np = 0;
            _data[_np++] = c;
            _data[_np] = 0;
            return Next;
        default:
            if (isEmpty(c))
            {
                return Next;
            }
        }

        return Unexpected;
    }
    Result Command::parseEnd(char c)
    {
        if (_pc == '\\')
        {
            char ec = unescape(c);
            if (ec != 0)
            {
                if (ec == '\\')
                {
                    _pc = 0;
                }
                else
                {
                    _pc = c;
                }
                return Next;
            }
            return Unexpected;
        }
        else if (c == '$')
        {
            _state = bBegin;
            _capture = false;
            return OK;
        }

        return Next;
    }

    Result Command::parseDevice(char c)
    {
        // wait terminated
        if (isDigit(c))
        {
            return Next;
        }

        switch (c)
        {
        case '-':
        case '_':
        case '.':
            return convertDevice(c, bDevice);
        case ':':
            // set default module address
            _module = USC_DEFAULT_MODULE;
            return convertDevice(c, bModule);
        case '/':
            _action = _data + _np;
            return convertDevice(c, bAction);
        case '|':
            _bp = _np;
            _data[_np - 1] = 0;
            return convertDevice(c, bCRC);
        case '$':
            _data[_np - 1] = 0;
            return convertDevice(c, bBegin, OK);
        }

        return Unexpected;
    }
    Result Command::parseModule(char c)
    {
        // check for overflow
        if (isDigit(c))
        {
            return Next;
        }

        switch (c)
        {
        case '/':
            _action = _data + _np;
            return convertModule(c, bAction);
        case '|':
            _bp = _np;
            _data[_np - 1] = 0;
            return convertModule(c, bCRC);
        case '$':
            _data[_np - 1] = 0;
            return convertModule(c, bBegin, OK);
        }

        return Unexpected;
    }
    Result Command::parseAction(char c)
    {
        // Check if c is valid
        if (isValidKey(c) || (c == '/' && _pc != '/'))
        {
            return Next;
        }

        switch (c)
        {
        case '?':
            _state = bParamKey;
            _param._next = &_data[_np];
            _data[_np - 1] = 0;
            return Next;
        case '|':
            _state = bCRC;
            _bp = _np;
            _data[_np - 1] = 0;
            return Next;
        case '$':
            _state = bBegin;
            _data[_np - 1] = 0;
            return OK;
        }
        return Unexpected;
    }
    Result Command::parseParamKey(char c)
    {
        // Check if c is valid
        if (isValidKey(c))
        {
            return Next;
        }
        switch (c)
        {
        case '=':
            _state = bParamValue;
            _data[_np - 1] = PARAM_VAL;
            _param._count++;
            return Next;
        case '|':
            _state = bCRC;
            _bp = _np;
            _data[_np - 1] = PARAM_END;
            _param._end = &_data[_np];
            _param._count++;
            return Next;
        case '$':
            _state = bBegin;
            _data[_np - 1] = PARAM_END;
            _param._end = &_data[_np];
            _param._count++;
            return OK;
        }
        return Unexpected;
    }
    Result Command::parseParamValue(char c)
    {
        switch (c)
        {
        case '&':
            _state = bParamKey;
            _data[_np - 1] = PARAM_KEY;
            return Next;
        case '|':
            _state = bCRC;
            _bp = _np;
            _data[_np - 1] = PARAM_END;
            _param._end = &_data[_np];
            return Next;
        case '$':
            _state = bBegin;
            _data[_np - 1] = PARAM_END;
            _param._end = &_data[_np];
            return OK;
        }
        return Next;
    }
    Result Command::parseCRC(char c)
    {
        // TODO: efficient calculation method
        if (isDigit(c))
        {
            return Next;
        }
        else if (c == '$')
        {
            _state = bBegin;
            _hasChecksum = true;

            // compare checksum
            uint8_t chk = (uint8_t)toInt(&_data[_bp], &_data[_np]);
            if (chk != _checksum)
            {
                return Invalid;
            }
            return OK;
        }
        return Unexpected;
    }

    Result Command::parse(char c)
    {
        if (_capture)
        {
            if (_state != bCRC && _state != bEnd)
            {
                _checksum ^= (uint8_t)c;
            }
            // Save to buffer
            if (_np >= USC_BUFSIZE)
            {
                return Overflow;
            }
            else if (_pc == '\\')
            {
                if (_state != bParamValue)
                {
                    // escape char only allowed in param value
                    return Unexpected;
                }
                char ec = unescape(c);
                if (ec != 0)
                {
                    _data[_np - 1] = ec;
                    _data[_np] = 0;
                    if (ec == '\\')
                    {
                        _pc = 0;
                    }
                    else
                    {
                        _pc = c;
                    }
                    return Next;
                }
                return Invalid;
            }
            _data[_np++] = c;
            _data[_np] = 0;
        }

        // process
        Result res;
        switch (_state)
        {
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
        case bAction:
            res = parseAction(c);
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
            res = Unexpected;
            break;
        }

        // save character as previous
        _pc = c;

        return res;
    }
}
