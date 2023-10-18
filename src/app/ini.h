#pragma once

// global
#include <string>
// local
#include "mi_types.h"

namespace ini {

template <typename T>
struct Record {
  const char *section;
  const char *key;
  T *field;
};

template <typename T>
struct RecordRanged : public Record<T> {
  T min;
  T max;
  bool InRange(T result) const { return (result < min || result > max); }
};

struct Description {
  MiVector<Record<bool>> bools;
  MiVector<RecordRanged<int>> ints;
  MiVector<RecordRanged<float>> floats;

  void Append(const Description &desc) noexcept {
    bools.reserve(bools.size() + desc.bools.size());
    ints.reserve(ints.size() + desc.ints.size());
    floats.reserve(floats.size() + desc.floats.size());

    bools.insert(bools.end(), desc.bools.begin(), desc.bools.end());
    ints.insert(ints.end(), desc.ints.begin(), desc.ints.end());
    floats.insert(floats.end(), desc.floats.begin(), desc.floats.end());
  }
};

void Load(const Description &desc, const std::string &filename);
void Save(const Description &desc, const std::string &filename);

}  // namespace ini