#include "common/file.h"

namespace raftcpp {

File File::Open(const std::string &file_name) {
    fstream file_id;
    file_id.open(file_name.c_str(), ios::in | ios::out | ios::binary | ios::trunc);
    File tmp(std::move(file_id), file_name);
    return tmp;
}

void File::CleanAndWrite(const std::string &context) {
    if (file_id_.is_open()) {
        file_id_.close();
    }
    file_id_.open(file_name_.c_str(), ios::in | ios::out | ios::binary | ios::trunc);
    file_id_.write(context.c_str(), context.size());
}

std::string File::ReadAll() {
    size_t file_len = 0;
    if (file_id_) {
	file_id_.seekg(0, ios::end);
        file_len = file_id_.tellg();
        file_id_.seekg(0, ios::beg);
        std::string res;
        res.resize(file_len);
        file_id_.read(const_cast<char *>(res.c_str()), file_len);
        file_id_.close();
        return res;
    }
    return std::string();
}

}  // namespace raftcpp
