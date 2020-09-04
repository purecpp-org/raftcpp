#pragma once

#include <fstream>
#include <string>
using namespace std;

namespace raftcpp {

class File {
public:
    File(fstream file_id, std::string file_name) 
	    : file_id_(std::move(file_id)), file_name_(file_name) {}

    static File Open(const std::string &file_name);

    void CleanAndWrite(const std::string &context);

    std::string ReadAll();

private:
    fstream file_id_;
    std::string file_name_;
};

}  // namespace raftcpp
