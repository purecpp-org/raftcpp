#include <iostream>
#include <gflags/gflags.h>
static bool ValidatePort(const char* flagname, gflags::int32 value)
{
    if (value > 0 && value < 32768)
    {
        return true;
    }
    return false;
}
DEFINE_int32(port, 8080, "What port to listen on");
DEFINE_validator(port, &ValidatePort);
DEFINE_bool(debug, false, "Turn on the debug mode");
int main(int argc, char *argv[])
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << "port = " << FLAGS_port << std::endl;
    std::cout << "debug = " << std::boolalpha << FLAGS_debug << std::endl;
    return 0;
}