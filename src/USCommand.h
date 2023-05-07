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

        void clear();
        const char *key() const;
        const char keyChar() const;
        const char *value() const;
        bool hasValue() const;
        int valueInt(int def = 0) const;
        long valueLong(long def = 0) const;
        float valueFloat(float def = 0) const;

    private:
        const char *_key;
        const char *_value;
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
        Command();

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
        void registerCallback(uint32_t dev, CommandCb fnCmd = nullptr, ErrorCb fnErr = nullptr);

    protected:
        Result parseBegin(char c);
        Result parseEnd(char c);
        Result parseDevice(char c);
        Result parseComponent(char c);
        Result parseAction(char c);
        Result parseParamKey(char c);
        Result parseParamValue(char c);
        Result parseCRC(char c);
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

        uint32_t _devCb;
        ErrorCb errCb;
        CommandCb cmdCb;
    };
};

#endif