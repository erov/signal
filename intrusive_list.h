#pragma once
#include <cstdlib>
#include <type_traits>
#include <iterator>
#include <cassert>

namespace intrusive {
  struct default_tag;

  struct base_list_element {
    base_list_element();

    base_list_element(base_list_element const&);
    base_list_element(base_list_element&&);
    base_list_element& operator=(base_list_element const&);
    base_list_element& operator=(base_list_element&&);

    ~base_list_element();

    void unlink() noexcept;

    template<typename T_, typename Tag_>
    friend struct list;

  private:
    void copy_base_list_element(base_list_element&& other);
    base_list_element* prev{nullptr};
    base_list_element* next{nullptr};
  };


  template <typename Tag = default_tag>
  struct list_element : base_list_element {};


  template <typename T, typename Tag = default_tag>
  struct list {

  private:
    template <typename Type>
    struct base_iterator;

  public:
    using node = list_element<Tag>;
    using iterator = base_iterator<T>;
    using const_iterator = base_iterator<T const>;

    static_assert(std::is_convertible_v<T&, list_element<Tag>&>,
          "value type is not convertible to list_element");

    list() noexcept : fake() {
      make_loop();
    }

    list(list const&) = delete;

    list(list&& other) noexcept : list() {
      copy_list(std::move(other));
    }

    ~list() {
      clear();
    }

    list& operator=(list const&) = delete;

    list& operator=(list&& other) noexcept {
      clear();
      copy_list(std::move(other));
      return *this;
    }

    void clear() noexcept {
      while (!empty()) {
        pop_front();
      }
    }

    void push_back(T& value) noexcept {
      insert(end(), value);
    }

    void pop_back() noexcept {
      erase(std::prev(end()));
    }

    T& back() noexcept {
      return *(std::prev(end()));
    }

    T const& back() const noexcept {
      return *(std::prev(end()));
    }

    void push_front(T& value) noexcept {
      insert(begin(), value);
    }

    void pop_front() noexcept {
      erase(begin());
    }

    T& front() noexcept {
      return *begin();
    }

    T const& front() const noexcept {
      return *begin();
    }

    bool empty() const noexcept {
      return &fake == fake.next;
    }


    iterator begin() noexcept {
      return iterator(fake.next);
    }

    const_iterator begin() const noexcept {
      return const_iterator(const_cast<base_list_element*>(fake.next));
    }

    iterator end() noexcept {
      return iterator(&fake);
    }

    const_iterator end() const noexcept {
      return const_iterator(const_cast<node*>(&fake));
    }

    iterator insert(const_iterator pos, T& value) noexcept {
      node* value_node = &static_cast<node&>(value);
      if (pos.data != value_node) {
        value_node->unlink();
        link(static_cast<node*>(pos.data->prev), value_node);
        link(value_node, pos.data);
      }
      return iterator(value_node);
    }

    iterator erase(const_iterator pos) noexcept {
      iterator iter = iterator(pos.data->next);
      pos.data->unlink();
      return iter;
    }

    void splice(const_iterator pos, list&, const_iterator first, const_iterator last) noexcept {
      if (first != last) {
        node* other_left = static_cast<node*>(first.data->prev);
        node* other_right = last.data;
        link(static_cast<node*>(pos.data->prev), first.data);
        link(static_cast<node*>(last.data->prev), pos.data);
        link(other_left, other_right);
      }
    }

    const_iterator wrap(T& value) const noexcept {
      return const_iterator(&static_cast<node&>(value));
    }

  private:
    void copy_list(list&& other) {
      if (!other.empty()) {
        other.fake.prev->next = &fake;
        other.fake.next->prev = &fake;
        fake.next = other.fake.next;
        fake.prev = other.fake.prev;
        other.make_loop();
      }
    }

    void make_loop() {
      fake.next = &fake;
      fake.prev = &fake;
    }

    void link(node* first, node* second) {
      first->next = second;
      second->prev = first;
    }

    node fake;

    template <typename Type>
    struct base_iterator {
      using iterator_category = std::bidirectional_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = Type;
      using pointer = Type*;
      using reference = Type&;

      base_iterator() = default;

      template<typename Other, typename = std::enable_if_t<std::is_const_v<Type> && !std::is_const_v<Other>>>
      base_iterator(base_iterator<Other> const &other) : data(other.data) {}

      base_iterator& operator++() {
        data = static_cast<node*>(data->next);
        return *this;
      }

      base_iterator operator++(int) {
        base_iterator old(*this);
        ++(*this);
        return old;
      }

      base_iterator& operator--() {
        data = static_cast<node*>(data->prev);
        return *this;
      }

      base_iterator operator--(int) {
        base_iterator old(*this);
        --(*this);
        return old;
      }

      reference operator*() const {
        return static_cast<reference>(*data);
      }

      pointer operator->() const {
        return static_cast<pointer>(data);
      }

      friend bool operator==(base_iterator const &a, base_iterator const &b) {
        return a.data == b.data;
      }

      friend bool operator!=(base_iterator const &a, base_iterator const &b) {
        return a.data != b.data;
      }

      template<typename T_, typename Tag_>
      friend struct list;

    private:
      base_iterator(node* data) : data(data) {}
      base_iterator(base_list_element* data) : data(static_cast<node*>(data)) {}
      node* data;
    };
  };
}
