#include "ini.h"

// deps
#include <fmt/compile.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
// no w_char, only ASCII
#define SI_NO_CONVERSION 1
#include <SimpleIni.h>
// global
#include <charconv>
#include <string_view>

namespace ini {

void ReadBoolValue(CSimpleIniA &ini, const Record<bool> &record) {
  if (ini.KeyExists(record.section, record.key)) {
    *record.field = ini.GetBoolValue(record.section, record.key, false);
  } else {
    ini.SetBoolValue(record.section, record.key, *record.field);
  }
}

// link for benchmarks:
// https://www.zverovich.net/2020/06/13/fast-int-to-string-revisited.html
// std::from_chars: 3x faster than stoi/atoi and 50x than stringstream, no throw
// fmt::format_int: fastest method that exists
// if catch some errors, fix ini file
void SetValue(CSimpleIniA &ini, const RecordRanged<int> &record) {
  ini.SetValue(record.section, record.key,
               fmt::format_int(*record.field).c_str());
}

void SetValue(CSimpleIniA &ini, const RecordRanged<float> &record) {
  static std::string buffer(32, ' ');

  buffer.clear();
  fmt::format_to(std::back_inserter(buffer), FMT_COMPILE("{:.8f}"),
                 *record.field);
  ini.SetValue(record.section, record.key, buffer.c_str());
}

template <typename T>
void ReadRangedValue(CSimpleIniA &ini, const RecordRanged<T> &record) {
  const char *pv = ini.GetValue(record.section, record.key);
  if (pv == nullptr) {
    SetValue(ini, record);
    spdlog::error("{}: Value not found '{}'", __FUNCTION__, record.key);
    return;
  }

  T result;
  std::string_view sv{pv};
  auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
  if (ec == std::errc()) {
    if (record.InRange(result)) {
      SetValue(ini, record);
      spdlog::error("{}: Invalid value '{}' = {}", __FUNCTION__, record.key,
                    result);
      return;
    }

    *record.field = result;
    return;
  }

  // errors
  if (ec == std::errc::invalid_argument) {
    SetValue(ini, record);
    spdlog::error("{}: Invalid argument '{} = {}'", __FUNCTION__, record.key,
                  pv);
    return;
  }
  if (ec == std::errc::result_out_of_range) {
    SetValue(ini, record);
    spdlog::error("{}: Result out of range '{} = {}'", __FUNCTION__, record.key,
                  pv);
    return;
  }
}

void Load(const Description &desc, const std::string &filename) {
  CSimpleIniA ini;
  SI_Error rc = ini.LoadFile(filename.c_str());
  if (rc < 0) {
    spdlog::error("{}: Can't load '{}'", __FUNCTION__, filename);
    return;
  }

  CSimpleIniA::TNamesDepend sections;
  ini.GetAllSections(sections);
  CSimpleIniA::TNamesDepend keys;
  ini.GetAllKeys("section1", keys);

  for (const auto &record : desc.bools) {
    ReadBoolValue(ini, record);
  }
  for (const auto &record : desc.ints) {
    ReadRangedValue(ini, record);
  }
  for (const auto &record : desc.floats) {
    ReadRangedValue(ini, record);
  }

  rc = ini.SaveFile(filename.c_str());
  if (rc < 0) {
    spdlog::error("{}: Can't save '{}'", __FUNCTION__, filename);
  }
};

void Save(const Description &desc, const std::string &filename) {
  CSimpleIniA ini;
  SI_Error rc = ini.LoadFile(filename.c_str());
  if (rc < 0) {
    spdlog::error("{}: Can't load '{}'", __FUNCTION__, filename);
    return;
  }

  for (const auto &record : desc.bools) {
    ini.SetBoolValue(record.section, record.key, *record.field);
  }
  for (const auto &record : desc.ints) {
    SetValue(ini, record);
  }
  for (const auto &record : desc.floats) {
    SetValue(ini, record);
  }

  rc = ini.SaveFile(filename.c_str());
  if (rc < 0) {
    spdlog::error("{}: Can't save '{}'", __FUNCTION__, filename);
  }
}

}  // namespace ini