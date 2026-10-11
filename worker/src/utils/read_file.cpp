#include "utils/read_file.h"

namespace monitor {

bool ReadFile::ReadLine(std::vector<std::string>* args) {
  std::string line;
  std::getline(ifs_, line);

  // 原实现把 EOF 或空行都视为本次读取失败
  if (ifs_.eof() || line.empty()) {
    return false;
  }

  // 将一整行转换成字符串流，再按空白字符拆成多个字段
  std::istringstream line_ss(line);
  while (!line_ss.eof()) {
    std::string word;
    line_ss >> word;
    args->push_back(word);
  }

  return true;
}

}  // namespace monitor