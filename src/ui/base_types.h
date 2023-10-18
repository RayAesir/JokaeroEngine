#pragma once

// global
#include <string>
// local
#include "mi_types.h"

namespace ui {

// ListBox or ComboBox, etc.
template <typename T>
struct Selectable {
  struct Record : public T {
    std::string str;
  };
  std::string label;
  MiVector<Record> list;
  int current{0};

  template <typename... Args>
  void EmplaceBack(Args&&... args) noexcept {
    list.emplace_back(Record{std::forward<Args>(args)...});
  }

  const T& Selected() const { return list[current]; }
  T& Selected() { return list[current]; }

  void Reset() {
    current = 0;
    list.clear();
  }
};

template <typename R>
struct Table {
  template <typename... Args>
  void AddRow(Args&&... args) noexcept {
    selected = data.size();
    data.emplace_back(std::forward<Args>(args)...);
  }

  template <typename V>
  void DeleteRow(V(R::*field), V value) {
    std::erase_if(
        data, [&value, &field](const R& row) { return row.*field == value; });
  }

  const R& GetSelected() const { return data[selected]; }

  R& SetSelected() { return data[selected]; }

  // return deleted row, and update selected index
  R DeleteSelected() {
    R row = GetSelected();
    data.erase(data.begin() + selected);
    selected = selected ? selected -= 1 : 0;
    return row;
  }

  size_t selected{0};
  MiVector<R> data;
};

}  // namespace ui
