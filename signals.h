#pragma once
#include <functional>
#include "intrusive_list.h"

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  using slot_t = std::function<void(Args...)>;

  struct connection_tag;
  struct connection : intrusive::list_element<connection_tag> {

    connection() = default;

    connection(connection const&) = delete;

    connection(connection&& other) noexcept {
      switch_with(std::move(other));
    }

    connection& operator=(connection const&) = delete;

    connection& operator=(connection&& rhs) noexcept {
      if (this != &rhs) {
        disconnect();
        switch_with(std::move(rhs));
      }
      return *this;
    }

    ~connection() {
      disconnect();
    }

    void disconnect() noexcept {
      if (sig != nullptr) {
        for (iteration_token* token = sig->top_token; token != nullptr; token = token->next) {
          if (&*token->iter == this) {
            ++token->iter;
          }
        }
        remove();
      }
    }

    friend struct signal;

  private:
    connection(signal* sig, slot_t&& slot) noexcept : sig(sig), slot(std::move(slot)) {
      sig->cons.push_back(*this);
    }

    void remove() noexcept {
      sig = nullptr;
      slot = {};
      this->unlink();
    }

    void switch_with(connection&& rhs) noexcept {
      sig = rhs.sig;
      slot = std::move(rhs.slot);
      if (sig != nullptr) {
        sig->cons.insert(++sig->cons.wrap(rhs), *this);
        rhs.disconnect();
      }
    }

    signal* sig{nullptr};
    slot_t slot;
  };

  using connection_t = intrusive::list<connection, connection_tag>;

  signal() = default;

  signal(signal const&) = delete;

  signal& operator=(signal const&) = delete;

  ~signal() {
    for (iteration_token* token = top_token; token != nullptr; token = token->next) {
      token->sig = nullptr;
    }
    while (!cons.empty()) {
      cons.back().remove();
    }
  }

  connection connect(slot_t slot) noexcept {
    return connection(this, std::move(slot));
  }

  void operator()(Args... args) const {
    iteration_token token(this);
    while (token.sig != nullptr && token.iter != cons.end()) {
      typename connection_t::const_iterator current = token.iter;
      ++token.iter;
      current->slot(args...);
    }
  }

private:
  struct iteration_token {
    explicit iteration_token(signal const* sig) : sig(sig), iter(sig->cons.begin()), next(sig->top_token) {
      sig->top_token = this;
    }

    ~iteration_token() {
      if (sig != nullptr) {
        sig->top_token = next;
      }
    }

    const signal* sig;
    typename connection_t::const_iterator iter;
    iteration_token* next;
  };

  connection_t cons;
  mutable iteration_token* top_token{nullptr};
};

} // namespace signals
