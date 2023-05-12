#include <stdlib.h>
#include <string.h>
#include "USCommand.h"

/**
 * Command format:
 * !nnn.nnn.nnn.nnn:nnnnn/xxx/yyy/zzz?abc=10&xyz=11|<CRC>$
 * Example:
 * !1:10/s$         -- start component 10
 * !1:10/e$         -- end component 10
 * !1:10/b?t=10$    -- begin component 10 with param `t` set to 10
 * !1:3/format$     -- format component 3
 * !1:3/w?0=1&1=2$  -- write to component 3 with addr 0 set to 1 and addr 1 set to 2
 */

#define PARAM_END 0x00
#define PARAM_KEY 0x01
#define PARAM_VAL 0x02

enum
{
    sBegin,
    sEnd,
    sError,
    sDevice,
    sComponent,
    sAction,
    sParamKey,
    sParamValue,
    sChecksum
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
    const char * Empty = "";

    KeyVal::KeyVal(const char *k, const char *v)
        : _key(k), _value(v)
    {
    }
    KeyVal::KeyVal(const KeyVal &kv)
    {
        _key = kv._key;
        _value = kv._value;
    }
    KeyVal &KeyVal::operator=(const KeyVal &kv)
    {
        _key = kv._key;
        _value = kv._value;
        return *this;
    }

    void KeyVal::clear()
    {
        _key = nullptr;
        _value = nullptr;
    }

    const char *KeyVal::key() const
    {
        return _key;
    }
    const char KeyVal::keyChar() const
    {
        return _key ? *_key : 0;
    }
    const char *KeyVal::value() const
    {
        return _value;
    }
    const char *KeyVal::safeValue() const {
        return _value ? _value : Empty;
    }
    bool KeyVal::hasValue() const
    {
        return _value != nullptr;
    }
    int KeyVal::valueInt(int def) const
    {
        return _value ? atoi(_value) : def;
    }
    long KeyVal::valueLong(long def) const
    {
        return _value ? atol(_value) : def;
    }
    float KeyVal::valueFloat(float def) const
    {
        return _value ? atof(_value) : def;
    }
    int KeyVal::valueInt(int def, int base) const {
        return _value ? (int)strtol(_value, NULL, base) : def;
    }
    long KeyVal::valueLong(long def, int base) const {
        return _value ? strtol(_value, NULL, base) : def;
    }
    bool KeyVal::copy(char *dest, int n) const {
        if (!_value || strlen(_value) < n) {
            return false;
        }
        for (int i = 0; i < n; i++) {
            dest[i] = _value[i];
        }
        dest[n] = 0;

        return true;
    }
    int KeyVal::copyn(char *dest, int n) const {
        int ns;
        if (_value || (ns = strlen(_value)) == 0) {
            return 0;
        }
        if (ns < n) {
            n = ns;
        }
        for (int i = 0; i < n; i++) {
            dest[i] = _value[i];
        }
        dest[n] = 0;

        return n;
    }

    uint8_t KeyVal::limitByte(uint8_t min, uint8_t max, uint8_t def, int base) const {
        long val = _value ? strtol(_value, NULL, base) : def;
        if (val < (long)min) {
            return min;
        } else if (val > (long)max) {
            return max;
        }
        return (uint8_t)val;
    }

    int KeyVal::limitInt(int min, int max, int def, int base) const {
        long val = _value ? strtol(_value, NULL, base) : def;
        if (val < (long)min) {
            return min;
        }
        if (val > (long)max) {
            return max;
        }
        return val;
    }
    long KeyVal::limitLong(long min, long max, long def, int base) const {
        long val = _value ? strtol(_value, NULL, base) : def;
        if (val < min) {
            return min;
        }
        if (val > max) {
            return max;
        }
        return val;
    }
    float KeyVal::limitFloat(float min, float max, float def) const {
        float val = _value ? atof(_value) : def;
        if (val < min) {
            return min;
        }
        if (val > max) {
            return max;
        }
        return val;
    }

    Params::Params()
    {
        clear();
    }

    void Params::clear()
    {
        _beg = nullptr;
        _end = nullptr;
        _next = nullptr;
        _count = 0;
        _pkey = nullptr;
        _pval = nullptr;
    }

    Params &Params::begin()
    {
        _kv.clear();
        _next = _beg;
        if (_pkey)
        {
            *_pkey = PARAM_KEY;
        }
        if (_pval)
        {
            *_pval = PARAM_VAL;
        }
        _pkey = nullptr;
        _pval = nullptr;

        return *this;
    }
    void Params::begin(char *beg)
    {
        _beg = beg;
        _end = beg;
        _next = beg;
        _count = 0;
    }
    void Params::add(char *end)
    {
        _count++;
        if (end)
        {
            _end = end;
        }
    }
    bool Params::next()
    {
        if (_next == nullptr || _next >= _end)
        {
            return false;
        }
        _kv._key = _next;
        _kv._value = nullptr;

        // restore marker
        if (_pkey)
        {
            *_pkey = PARAM_KEY;
        }
        if (_pval)
        {
            *_pval = PARAM_VAL;
        }
        _pkey = nullptr;
        _pval = nullptr;

        char *p = _next;
        while (p != _end)
        {
            switch (*p)
            {
            case PARAM_END:
            case PARAM_KEY:
                _pkey = p;
                *p = 0;
                _next = ++p;
                return true;
            case PARAM_VAL:
                _pval = p;
                _kv._key = _next;
                _kv._value = p + 1;
                *p = 0;
                break;
            }
            p++;
        }
        return false;
    }
    const KeyVal &Params::kv() const
    {
        return _kv;
    }
    int Params::count() const
    {
        return _count;
    }
    bool Params::empty() const
    {
        return _count == 0;
    }

    Command::Command(uint32_t dev)
    {
        cmdCb = nullptr;
        errCb = nullptr;
        _devAddr = dev;
        clear();
    }

    void Command::clear(void)
    {
        _pc = '\0';
        _np = 0;
        _ni = 0;
        _state = sBegin;
        _device = InvalidDevice;
        _component = 0;
        _capture = false;
        _checksum = 0;
        _hasChecksum = false;
        _data[0] = 0;
        _data[1] = 0;
        _action = nullptr;
        _params.clear();
    }

    uint32_t Command::device(void) const
    {
        return _device;
    }
    void Command::changeDeviceAddress(uint32_t addr) 
    {
        _devAddr = addr;
    }
    uint32_t Command::deviceAddress() const {
        return _devAddr;
    }

    uint16_t Command::component(void) const
    {
        return _component;
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
        return _action ? _action : Empty;
    }

    bool Command::hasAction() const
    {
        return _action != nullptr && *_action != 0;
    }

    Params &Command::params()
    {
        return _params;
    }

    void Command::attachCallback(CommandCb fnCmd, ErrorCb fnErr) {
        this->cmdCb = fnCmd;
        this->errCb = fnErr;
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

    Result Command::convertComponent(char c, uint8_t ns, Result res)
    {
        if ((_np - _bp) > 6)
        {
            // more than 5 digits
            return Unexpected;
        }

        // convert and assign address
        _component = (uint16_t)toLong(&_data[_bp], &_data[_np]);
        _state = ns;
        _bp = _np;

        return res;
    }

    Result Command::processBegin(char c)
    {
        switch (c)
        {
        case '!':
            _state = sDevice;
            _np = 0;
            _bp = 1;
            _ni = 0;
            _capture = true;
            _data[_np++] = c;
            _checksum ^= (uint8_t)c;
            return Next;
        case '@':
            _state = sEnd;
            _np = 0;
            _data[_np++] = c;
            _data[_np] = 0;
            return Next;
        }

        return Unexpected;
    }
    Result Command::processEnd(char c)
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
            _state = sBegin;
            _capture = false;
            return OK;
        }

        return Next;
    }

    Result Command::processDevice(char c)
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
            return convertDevice(c, sDevice);
        case ':':
            // set default component address
            _component = USC_DEFAULT_COMPONENT;
            return convertDevice(c, sComponent);
        case '/':
            _action = _data + _np;
            return convertDevice(c, sAction);
        case '|':
            _bp = _np;
            _data[_np - 1] = 0;
            return convertDevice(c, sChecksum);
        case '$':
            _data[_np - 1] = 0;
            return convertDevice(c, sBegin, OK);
        }

        return Unexpected;
    }
    Result Command::processComponent(char c)
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
            return convertComponent(c, sAction);
        case '|':
            _bp = _np;
            _data[_np - 1] = 0;
            return convertComponent(c, sChecksum);
        case '$':
            _data[_np - 1] = 0;
            return convertComponent(c, sBegin, OK);
        }

        return Unexpected;
    }
    Result Command::processAction(char c)
    {
        // Check if c is valid
        if (isValidKey(c) || (c == '/' && _pc != '/'))
        {
            return Next;
        }

        switch (c)
        {
        case '?':
            _state = sParamKey;
            _params.begin(_data + _np);
            _data[_np - 1] = 0;
            return Next;
        case '|':
            _state = sChecksum;
            _bp = _np;
            _data[_np - 1] = 0;
            return Next;
        case '$':
            _state = sBegin;
            _data[_np - 1] = 0;
            return OK;
        }
        return Unexpected;
    }
    Result Command::processParamKey(char c)
    {
        // Check if c is valid
        if (isValidKey(c))
        {
            return Next;
        }
        switch (c)
        {
        case '=':
            _state = sParamValue;
            _data[_np - 1] = PARAM_VAL;
            _params.add();
            return Next;
        case '|':
            _state = sChecksum;
            _bp = _np;
            _data[_np - 1] = PARAM_END;
            _params.add(_data + _np);
            return Next;
        case '$':
            _state = sBegin;
            _data[_np - 1] = PARAM_END;
            _params.add(_data + _np);
            return OK;
        }
        return Unexpected;
    }
    Result Command::processParamValue(char c)
    {
        switch (c)
        {
        case '&':
            _state = sParamKey;
            _data[_np - 1] = PARAM_KEY;
            return Next;
        case '|':
            _state = sChecksum;
            _bp = _np;
            _data[_np - 1] = PARAM_END;
            _params._end = _data + _np;
            return Next;
        case '$':
            _state = sBegin;
            _data[_np - 1] = PARAM_END;
            _params._end = _data + _np;
            return OK;
        }
        return Next;
    }
    Result Command::processChecksum(char c)
    {
        // TODO: efficient calculation method
        if (isDigit(c))
        {
            return Next;
        }
        else if (c == '$')
        {
            _state = sBegin;
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

    Result Command::doProcess(char c)
    {
        if (_state == sBegin) {
            if (isEmpty(c)) {
                return Next;
            } else if (c == '!' || c == '@') {
                clear();
            }
        }
        if (_capture)
        {
            if (_state != sChecksum && _state != sEnd)
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
                if (_state != sParamValue)
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
        case sBegin:
            res = processBegin(c);
            break;
        case sEnd:
            res = processEnd(c);
            break;
        case sDevice:
            res = processDevice(c);
            break;
        case sComponent:
            res = processComponent(c);
            break;
        case sAction:
            res = processAction(c);
            break;
        case sParamKey:
            res = processParamKey(c);
            break;
        case sParamValue:
            res = processParamValue(c);
            break;
        case sChecksum:
            res = processChecksum(c);
            break;
        default:
            res = Unexpected;
            break;
        }

        // save character as previous
        _pc = c;

        return res;
    }

    Result Command::process(char c) {
        if (_state == sError) {
            clear();
        }

        bool call;
        Result res = doProcess(c);
        switch (res) {
        case OK:
            // match address
            call = !isResponse() && (isBroadcast() || _device == _devAddr) && cmdCb;
            if (call) {
                _params.begin();
                cmdCb(isBroadcast(), _component, action(), _params);
            }
            break;
        case Next:
            break;
        default:
            if (errCb) {
                _params.begin();
                errCb(res, *this);
            }
            _state = sError;
            break;
        }
        return res;
    }
}
