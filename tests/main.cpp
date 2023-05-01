#include <cstdio>
#include <fstream>
#include "../src/USCommand.h"

int main()
{
    USCommand cmd;
    cmd.reset();

    std::string str;
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
            str.push_back(ch);

            switch(res) {
            case USC_OK:
                printf("%s | Device: %d, module: %d -> %d\n", 
                    str.c_str(), cmd.device(), cmd.module(), cmd.isResponse());
                cmd.reset();
                str = "";
                break;
            case USC_Continue:
                break;
            default:
                printf("%s (`%c`) | Result (error): %d, Device: %d, module: %d\n", 
                    str.c_str(), ch, (int)res, cmd.device(), cmd.module());
                cmd.reset();
                str = "";
                break;
            }
        }
        file.close();
    }

    return 0;
}