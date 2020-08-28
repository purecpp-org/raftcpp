#pragma once

#include <string>

namespace raftcpp {

class File {
public:
    static File Open(const std::string &file_name) {
        return File();
    }

    void CleanAndWrite(const std::string &context) {

    }

    std::string ReadAll() {
        return "";
    }



private:

};

}