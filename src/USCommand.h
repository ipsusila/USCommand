#ifndef _USCOMMAND_H_
#define _USCOMMAND_H_

#include <stdint.h>

#define USC_BROADCAST_ADDR  0
#define USC_DEFAULT_MODULE  0
#define USC_BUFSIZE 128

enum USC_Identifier {
    USC_Broadcast = 0x00,
    USC_InvalidDevice = UINT32_MAX,
    USC_InvalidModule = UINT16_MAX
};

enum USC_Result {
    USC_OK = 0x00,
    USC_Continue,
    USC_Invalid,
    USC_Unexpected,
    USC_Overflow
};

enum USC_Pos {
    USC_DevicePos = 0,
    USC_ModulePos,
    USC_DesignationPos,
    USC_DesignationEndPos,
    USC_ParamPos,
    USC_ParamEndPos,
    USC_CRCPos,
    USC_ElementsCount
};

class USCommand {
public:
    USCommand();

    USC_Result parse(char c);
    uint32_t device(void) const;
    uint16_t module(void) const;
    bool isBroadcast(void) const;
    bool isResponse(void) const;
    void reset(void);
    const char * data(void) const;
    uint8_t checksum(void) const;
    bool hasChecksum(void) const;
    char * designationBegin(void);
    char * designationEnd(void);
    char * designation(void);
    char * paramBegin(void);
    char * paramEnd(void);
    char * param(void);

protected:
    USC_Result parseBegin(char c);
    USC_Result parseEnd(char c);
    USC_Result parseDevice(char c);
    USC_Result parseModule(char c);
    USC_Result parseDesignation(char c);
    USC_Result parseParamKey(char c);
    USC_Result parseParamValue(char c);
    USC_Result parseCRC(char c);
    USC_Result convertDevice(char c, uint8_t ns, USC_Result res = USC_Continue);
    USC_Result convertModule(char c, uint8_t ns, USC_Result res = USC_Continue);

private:
    uint32_t _device;
    uint16_t _module;
    uint8_t _state;
    uint8_t _checksum;
    bool _hasChecksum;
    
    char _pc;
    int _np, _bp;
    uint8_t _ni;
    bool _record;

    char _data[USC_BUFSIZE+1];
    int _pos[USC_ElementsCount];
};

#endif