#include <cstdio>
#include <fstream>
#include "../src/USCommand.h"

int main()
{
    USCommand cmd;
    cmd.reset();

    std::ifstream file("input.txt");
    if (file.is_open())
    {
        char ch;
        while (file.good())
        {
            ch = 0;
            file.get(ch);
            if (ch == 0) {
                break;
            }
            USC_Result res = cmd.parse(ch);
            //str.push_back(ch);

            switch(res) {
            case USC_OK:
                printf("'%s' | Device: %d, module: %d -> %d\n", 
                    cmd.data(), cmd.device(), cmd.module(), cmd.isResponse());
                cmd.reset();
                break;
            case USC_Continue:
                break;
            default:
                printf("'%s' (`%c`) | Err: %d, Device: %d, module: %d\n", 
                    cmd.data(), ch, (int)res, cmd.device(), cmd.module());
                cmd.reset();
                break;
            }
        }
        file.close();
    }

    return 0;
}