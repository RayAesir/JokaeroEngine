#include "string_parsing.h"

// global
#include <locale>

namespace parse {

const std::locale kLocale;
void ToLower(std::string &input) {
  for (auto &character : input) {
    character = std::tolower(character, kLocale);
  }
}

}  // namespace parse