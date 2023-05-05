#include <cstdio>
#include <fstream>
#include "../src/USCommand.h"

uint8_t xorall(const char *data) {
    uint8_t chk = 0;
    const char *p = data;
    while (*p != 0) {
        if (*p == '|') {
            chk ^= (uint8_t)*p;
            return chk;
        }
        chk ^= (uint8_t)*p;
        p++;
    }
    return chk;
}

int main()
{
    usc::Command cmd;
    cmd.clear();

    const char *test = "!0.0.2.3:123|AA$";
    printf("Checksum: %s -> %X\n", test, xorall(test));

    std::ifstream file("input.txt");
    if (file.is_open())
    {
        char ch;
        uint8_t chk;
        while (file.good())
        {
            ch = 0;
            file.get(ch);
            if (ch == 0) {
                break;
            }
            

            usc::Result res = cmd.parse(ch);
            //str.push_back(ch);

            char *pc;
            usc::Param *pp;
            switch(res) {
            case usc::OK:
                chk = xorall(cmd.data());
                printf("'%s' | Device: %d, module: %d -> %d, CHK=%X (%d) <> %X\n", 
                    cmd.data(), cmd.device(), cmd.module(), cmd.isResponse(), cmd.checksum(), chk, chk);
                if (cmd.hasDesignation()) {
                    printf("  Designation: `%s`\n", cmd.designation());
                }
                while (cmd.nextParam()) {
                    pp = cmd.param();
                    if (pp && pp->key()) {
                        printf("  Param: `%s`", pp->key());
                    }
                    if (pp && pp->hasValue()) {
                        printf(">`%s`\n", pp->value());
                    }
                }
                printf("\n");
                cmd.clear();
                break;
            case usc::Next:
                break;
            default:
                printf("'%s' (`%c`) | Err: %d, Device: %d, module: %d, chk: %d\n", 
                    cmd.data(), ch, (int)res, cmd.device(), cmd.module(), cmd.checksum());
                cmd.clear();
                break;
            }
        }
        file.close();
    }

    return 0;
}