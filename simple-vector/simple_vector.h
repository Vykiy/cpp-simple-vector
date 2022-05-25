#pragma once

#include "array_ptr.h"

#include <cassert>
#include <stdexcept>
#include <initializer_list>
#include <array>

using namespace std;

class ReserveProxyObj {
public:
  ReserveProxyObj() = default;
  explicit ReserveProxyObj (size_t capacity_to_reserve) {
    capacity_to_reserve_ = capacity_to_reserve;
  }
  size_t GetSize() const {
    return capacity_to_reserve_;
  }
private:
  size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
  ReserveProxyObj tmp(capacity_to_reserve);
  return tmp;
}

template <typename Type>
class SimpleVector {
public:
  using Iterator = Type*;
  using ConstIterator = const Type*;

  SimpleVector() noexcept = default;

  // Создаёт пустой вектор c резервированием размера
  explicit SimpleVector(ReserveProxyObj proxysize_) {
    Reserve(proxysize_.GetSize());
  }

  // Создаёт вектор из size элементов, инициализированных значением по умолчанию
  explicit SimpleVector(size_t size) : size_(size), capacity_(size) {
    SimpleVector tmp(size, Type{});
    this->swap(tmp);
  }

  // Создаёт вектор из size элементов, инициализированных значением value
  SimpleVector(size_t size, const Type& value) : size_(size), capacity_(size) {
    ArrayPtr<Type> newArray(size);
    std::fill(newArray.Get(), newArray.Get() + size, value);
    items_.swap(newArray);
  }

  // Создаёт вектор из std::initializer_list
  SimpleVector(std::initializer_list<Type> init) : size_(init.size()), capacity_(size_) {
    ArrayPtr<Type> newArray(size_);
    std::copy(init.begin(), init.end(), newArray.Get());
    items_.swap(newArray);
  }

  SimpleVector(const SimpleVector& other) {
    SimpleVector tmp(other.GetSize());
    std::copy(other.begin(), other.end(), tmp.begin());
    this->swap(tmp);
  }

  SimpleVector(SimpleVector&& other)  noexcept {
    this->swap(other);
  }

  SimpleVector& operator=(const SimpleVector& rhs) {
    if (&rhs != this) {
        SimpleVector<Type> tmp(rhs.GetSize());
        std::copy(rhs.begin(), rhs.end(), tmp.begin());
        this->swap(tmp);
      }
    return *this;
  }

  SimpleVector& operator=(SimpleVector&& rhs)  noexcept {
    this->swap(rhs);
      return *this;
  }

  // Резервирует емкость вектора заданного размера
  void Reserve(size_t newcapacity_)
  {
    if (newcapacity_  > capacity_)
      {
        Type t{};
        SimpleVector <Type> tmp(newcapacity_, t);
        copy(this->begin(), this->end(), tmp.begin());
        items_.swap(tmp.items_);
        capacity_ = tmp.capacity_;
      }
  }

  // Добавляет элемент в конец вектора
  // При нехватке места увеличивает вдвое вместимость вектора
  void PushBack(const Type& item) {
    Type tmp(item);
    PushBack(std::move(tmp));
  }

  void PushBack(Type&& item) {
    if (size_ < capacity_) {
        *end() = std::move(item);
        ++size_;
      }
    else {
        if (capacity_ == 0) {
            ++capacity_;
          }
        ArrayPtr<Type> newArray(capacity_ * 2);
        std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), newArray.Get());
        newArray[size_++] = std::move(item);
        items_.swap(newArray);
        capacity_ *= 2;
      }
  }

  // Вставляет значение value в позицию pos.
  // Возвращает итератор на вставленное значение
  // Если перед вставкой значения вектор был заполнен полностью,
  // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
  Iterator Insert(ConstIterator pos, const Type& value) {
    return Insert(pos, std::move(value));
  }

  Iterator Insert(ConstIterator pos, Type&& value) {
    assert(pos >= begin() && pos <= end());
    if (pos == this->end()) {
        PushBack(std::move(value));
        return this->end()-1;
      }
    auto t = pos - this->begin();
    ArrayPtr<Type> newArray(capacity_);
    Type* ptr = newArray.Get();
    std::copy(std::make_move_iterator(this->begin()), std::make_move_iterator(Iterator(pos)), ptr);
    ptr[t] = std::move(value);

    std::copy(std::make_move_iterator(Iterator(pos)), std::make_move_iterator(this->end()), &ptr[t + 1]);
    items_.swap(newArray);
    ++size_;
    return Iterator(pos);
  }

  // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
  void PopBack() noexcept {
    if (!IsEmpty())
      --size_;
  }

  // Удаляет элемент вектора в указанной позиции
  Iterator Erase(ConstIterator pos) {
    assert(pos >= begin() && pos < end());
    move(Iterator(pos) + 1, end(), Iterator(pos));
    --size_;
    return Iterator(pos);
  }

  // Обменивает значение с другим вектором
  void swap(SimpleVector& other) noexcept {
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    items_.swap(other.items_);
  }

  // Возвращает количество элементов в массиве
  size_t GetSize() const noexcept {
    return size_;
  }

  // Возвращает вместимость массива
  size_t GetCapacity() const noexcept {
    return capacity_;
  }

  // Сообщает, пустой ли массив
  bool IsEmpty() const noexcept {
    return !size_;
  }

  // Возвращает ссылку на элемент с индексом index
  Type& operator[](size_t index) noexcept {
    assert(index < size_);
    return items_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  const Type& operator[](size_t index) const noexcept {
    assert(index < size_);
    return const_cast<const Type&>(items_[index]);
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  Type& At(size_t index) {
    if (index >= size_) {
        throw std::out_of_range("Out of range");
      }
    return items_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  const Type& At(size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("Out of range");
      }
    return const_cast<const Type&>(items_[index]);
  }

  // Обнуляет размер массива, не изменяя его вместимость
  void Clear() noexcept {
    size_ = 0;
  }

  // Изменяет размер массива.
  // При увеличении размера новые элементы получают значение по умолчанию для типа Type
  void Resize(size_t newsize_) {
    if (size_ >= newsize_)
      size_ = newsize_;
    else if (capacity_ >= newsize_) {
        auto it = begin() + size_;
        while ( it < it + (newsize_ - size_)) {
            *it++ = std::move(Type{});
          }
        size_ = newsize_;
      }
    else {
        ArrayPtr<Type> newArray(newsize_);
        std::move(begin(), end(), newArray.Get());
        auto it = newArray.Get() + size_;
        while ( it < newArray.Get() + newsize_) {
            *it++ = std::move(Type{});
          }
        items_.swap(newArray);
        capacity_ = size_ = newsize_;
      }
  }

  // Возвращает итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator begin() noexcept {
    return items_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator end() noexcept {
    return items_.Get() + size_;
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator begin() const noexcept {
    return cbegin();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator end() const noexcept {
    return cend();
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cbegin() const noexcept {
    return const_cast<ConstIterator>(items_.Get());
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cend() const noexcept {
    return const_cast<ConstIterator>(items_.Get() + size_);
  }

private:
  ArrayPtr<Type> items_;
  size_t size_ = 0;
  size_t capacity_ = 0;
};


template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  // Заглушка. Напишите тело самостоятельно
  return !(lhs != rhs);
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return (lhs < rhs) || (lhs > rhs) ;
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return !(lhs < rhs);
}
