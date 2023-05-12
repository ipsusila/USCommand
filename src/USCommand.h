#ifndef _USCOMMAND_H_
#define _USCOMMAND_H_

#include <stdint.h>

#define USC_BROADCAST_ADDR 0
#define USC_DEFAULT_COMPONENT 0

#ifndef USC_BUFSIZE
#define USC_BUFSIZE 128
#endif

namespace usc
{
    enum Identifier
    {
        Broadcast = 0x00,
        InvalidDevice = UINT32_MAX,
        InvalidComponent = UINT16_MAX
    };

    enum Result
    {
        OK = 0x00,
        Next,
        Invalid,
        Unexpected,
        Overflow
    };

    class Command;
    class Params;

    // callback type
    typedef void (*CommandCb)(bool, uint16_t, const char *, Params &);
    typedef void (*ErrorCb)(Result, Command &);

    class KeyVal
    {
        friend class Params;

    public:
        KeyVal(const char *k = nullptr, const char *v = nullptr);
        KeyVal(const KeyVal &kv);
        KeyVal &operator=(const KeyVal &kv);

        const char *key() const;
        const char keyChar() const;
        const char *value() const;
        const char *safeValue() const;
        bool hasValue() const;
        int valueInt(int def = 0) const;
        long valueLong(long def = 0) const;
        float valueFloat(float def = 0) const;
        int valueInt(int def, int base) const;
        long valueLong(long def, int base) const;
        uint8_t valueByte(uint8_t min = 0, uint8_t max = 255, uint8_t def = 0, int base = 10) const;
        int valueInt(int min, int max, int def, int base = 10) const;
        long valueLong(long min, long max, long def, int base = 10) const;
        float valueFloat(float min, float max, float def) const;
        bool copy(char *dest, int n) const;
        int copyn(char *dest, int n) const;

    private:
        const char *_key;
        const char *_value;

        void clear();
    };

    class Params
    {
        friend class Command;

    public:
        Params();

        Params &begin();
        bool next();
        const KeyVal &kv() const;
        int count() const;
        bool empty() const;

    private:
        char *_beg;
        char *_end;
        char *_next;
        int _count;
        char *_pkey;
        char *_pval;
        KeyVal _kv;

        void clear();
        void begin(char *beg);
        void add(char *end = nullptr);
    };

    class Command
    {
    public:
        Command(uint32_t addr = 0);

        Result process(char c);
        bool isBroadcast(void) const;
        bool isResponse(void) const;
        void clear(void);
        char *data(char prefix = 0);
        bool hasChecksum(void) const;
        uint8_t checksum(void) const;
        uint32_t device(void) const;
        uint16_t component(void) const;
        bool hasAction(void) const;
        const char *action(void) const;
        char *beginResponse(void);
        char *beginResponseCheksum(uint8_t &chk);
        char endResponse(void) const;
        Params &params(void);
        void attachCallback(CommandCb fnCmd = nullptr, ErrorCb fnErr = nullptr);
        void changeDeviceAddress(uint32_t addr);
        uint32_t deviceAddress() const;

    protected:
        Result processBegin(char c);
        Result processEnd(char c);
        Result processDevice(char c);
        Result processComponent(char c);
        Result processAction(char c);
        Result processParamKey(char c);
        Result processParamValue(char c);
        Result processChecksum(char c);
        Result convertDevice(char c, uint8_t ns, Result res = Next);
        Result convertComponent(char c, uint8_t ns, Result res = Next);
        Result doProcess(char c);

    private:
        uint32_t _device;
        uint16_t _component;
        uint8_t _state;
        uint8_t _checksum;
        bool _hasChecksum;
        Params _params;

        char _pc;
        int _np, _bp;
        uint8_t _ni;
        bool _capture;

        char *_action;
        char _data[USC_BUFSIZE + 1];

        uint32_t _devAddr;
        ErrorCb errCb;
        CommandCb cmdCb;
    };
};

#endif