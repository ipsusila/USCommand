#include <cstdio>
#include <fstream>
#include "../src/USCommand.h"

uint8_t xorall(const char *data)
{
    uint8_t chk = 0;
    const char *p = data;
    while (*p != 0)
    {
        if (*p == '|')
        {
            chk ^= (uint8_t)*p;
            return chk;
        }
        chk ^= (uint8_t)*p;
        p++;
    }
    return chk;
}

std::string line;
void onCommand(bool bcast, uint16_t comp, const char *action, usc::Params &par) {
    printf("LINE: |\n%s\n|\n", line.c_str());
    printf("[CMD] bcast: %d, com: %d, action: %s, pars: %d\n", bcast, comp, action, par.count());
    line.clear();
}

void onError(usc::Result res, usc::Command &c) {
    printf("LINE: |\n%s\n|\n", line.c_str());
    printf("[ERR] err: %d, bcast: %d, com: %d, action: %s, pars: %d\n", 
        res, c.isBroadcast(), c.component(), c.action(), c.params().count());
    line.clear();
}

void testCallback() {
    usc::Command cmd(18);
    cmd.attachCallback(onCommand, onError);

    std::ifstream file("input.txt");
    if (file.is_open())
    {
        char ch;
        uint8_t chk;
        std::string str = "";
        while (file.good())
        {
            ch = 0;
            file.get(ch);
            if (ch == 0)
            {
                break;
            }
            line.push_back(ch);
            cmd.process(ch);
        }
        file.close();
    }
}

void testParse() {
    usc::Command cmd;

    const char *test = "!0.0.2.3:123|AA$";
    printf("Checksum: %s -> %X\n", test, xorall(test));

    std::ifstream file("input.txt");
    if (file.is_open())
    {
        char ch;
        uint8_t chk;
        std::string str = "";
        while (file.good())
        {
            ch = 0;
            file.get(ch);
            if (ch == 0)
            {
                break;
            }

            usc::Result res = cmd.process(ch);
            str.push_back(ch);

            char *pc;
            switch (res)
            {
            case usc::OK:
                printf("Data: %s\n", str.c_str());
                chk = xorall(cmd.data());
                printf("'%s' | Device: %d, component: %d -> %d, Par: %d, CHK=%X (%d) <> %X\n",
                       cmd.data(), cmd.device(), cmd.component(), cmd.isResponse(),
                       cmd.params().count(), cmd.checksum(), chk, chk);
                if (cmd.hasAction())
                {
                    printf("  Action: `%s`\n", cmd.action());
                }

                for (int i = 0; i < 2; i++)
                {
                    usc::Params &par = cmd.params().begin();
                    while (par.next())
                    {
                        const char *val = par.kv().hasValue() ? par.kv().value() : "";
                        printf("  Pars[%d]  (%d): `%s` > `%s`\n", i, par.count(), par.kv().key(), val);
                    }
                    par.begin();
                }

                printf("\n");
                str.clear();
                break;
            case usc::Next:
                break;
            default:
                printf("'%s' (`%c`) | Err: %d, Device: %d, component: %d, chk: %d\n",
                       cmd.data(), ch, (int)res, cmd.device(), cmd.component(), cmd.checksum());
                str.clear();
                break;
            }
        }
        file.close();
    }
}

int main()
{
    testCallback();
    printf("\n==========\n");
    testParse();
    return 0;
}