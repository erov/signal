#include "intrusive_list.h"

namespace intrusive {
  base_list_element::base_list_element() : prev(), next() {}

  base_list_element::base_list_element(base_list_element const&) : prev(), next() {}

  base_list_element::base_list_element(base_list_element&& other)
      : base_list_element() {
    copy_base_list_element(std::move(other));
  }

  base_list_element& base_list_element::operator=(base_list_element const&) {
    prev = nullptr;
    next = nullptr;
    return *this;
  }

  base_list_element& base_list_element::operator=(base_list_element&& other) {
    copy_base_list_element(std::move(other));
    return *this;
  }

  void base_list_element::copy_base_list_element(base_list_element&& other) {
    prev = other.prev;
    next = other.next;
    if (other.prev != nullptr) {
      other.prev->next = this;
    }
    if (other.next != nullptr) {
      other.next->prev = this;
    }
    other.prev = nullptr;
    other.next = nullptr;
  }

  base_list_element::~base_list_element() {
    unlink();
  }

  void base_list_element::unlink() noexcept {
    if (prev != nullptr) {
      prev->next = next;
      next->prev = prev;
      next = nullptr;
      prev = nullptr;
    }
  }
}
