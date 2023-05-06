#ifndef _USCOMMAND_H_
#define _USCOMMAND_H_

#include <stdint.h>

#define USC_BROADCAST_ADDR 0
#define USC_DEFAULT_MODULE 0

#ifndef USC_BUFSIZE
#define USC_BUFSIZE 128
#endif

namespace usc
{
    enum Identifier
    {
        Broadcast = 0x00,
        InvalidDevice = UINT32_MAX,
        InvalidModule = UINT16_MAX
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

    class KeyVal {
    public:
        KeyVal(const char *k = nullptr, const char *v = nullptr);
        KeyVal(const KeyVal &kv);
        KeyVal &operator=(const KeyVal &kv);

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

    class Param
    {
        friend class Command;

    public:
        Param();

        int count() const;
        bool valid() const;
        bool hasValue() const;
        char *key();
        char keyChar() const;
        char *value();
        int valueInt(int def = 0) const;
        long valueLong(long def = 0) const;
        float valueFloat(float def = 0) const;
        KeyVal kv() const;

    private:
        char *_key;
        char *_value;
        char *_next;
        char *_end;
        int _count;

        void clear(void);
        bool parse();
    };


    class Command
    {
    public:
        Command();

        Result parse(char c);
        bool isBroadcast(void) const;
        bool isResponse(void) const;
        void clear(void);
        char *data(char prefix = 0);
        bool hasChecksum(void) const;
        uint8_t checksum(void) const;
        uint32_t device(void) const;
        uint16_t module(void) const;
        bool hasAction(void) const;
        const char *action(void) const;
        bool hasParam(void) const;
        Param &param(void);
        bool nextParam(void);
        char *beginResponse(void);
        char *beginResponseCheksum(uint8_t &chk);
        char endResponse(void) const;

    protected:
        Result parseBegin(char c);
        Result parseEnd(char c);
        Result parseDevice(char c);
        Result parseModule(char c);
        Result parseAction(char c);
        Result parseParamKey(char c);
        Result parseParamValue(char c);
        Result parseCRC(char c);
        Result convertDevice(char c, uint8_t ns, Result res = Next);
        Result convertModule(char c, uint8_t ns, Result res = Next);

    private:
        uint32_t _device;
        uint16_t _module;
        uint8_t _state;
        uint8_t _checksum;
        bool _hasChecksum;
        Param _param;

        char _pc;
        int _np, _bp;
        uint8_t _ni;
        bool _capture;

        char *_action;
        char _data[USC_BUFSIZE + 1];
    };
};

#endif