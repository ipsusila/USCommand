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
        std::string str = "";
        while (file.good())
        {
            ch = 0;
            file.get(ch);
            if (ch == 0)
            {
                break;
            }

            usc::Result res = cmd.parse(ch);
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
                cmd.clear();
                str.clear();
                break;
            case usc::Next:
                break;
            default:
                printf("'%s' (`%c`) | Err: %d, Device: %d, component: %d, chk: %d\n",
                       cmd.data(), ch, (int)res, cmd.device(), cmd.component(), cmd.checksum());
                cmd.clear();
                str.clear();
                break;
            }
        }
        file.close();
    }

    return 0;
}