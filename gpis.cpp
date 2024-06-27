#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
using namespace std;


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


class session
{
public:

    session()
    {
        cout << "Session initialized" << endl;
    }

    void checkForChanges()
    {
        string result = exec("git diff");
        cout << result <<endl;

    }
};

int main(int argc, char **argv)
{
    session sess;
    sess.checkForChanges();
    for (int i = 0; i < argc; i++)
    {
        printf("%s \n", argv[i]);
    }
}