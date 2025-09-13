#pragma once

#include "core.h"

#include <forward_list>

namespace Bratwurst
{

// zero-abstraction cost stack allocated vector like container
template<typename T, size_t N>
class StaticVector
{
public:
    constexpr StaticVector() : m_size(0) {}

    constexpr void push_back(const T& value) noexcept
    {
        ASSERT(m_size < N, "StaticVector out of capacity!");
        m_data[m_size++] = value;
    }

    constexpr void push_back(T&& value) noexcept
    {
        ASSERT(m_size < N, "StaticVector out of capacity!");
        m_data[m_size++] = std::move(value);
    }

    template<typename... Args>
    constexpr T& emplace_back(Args&&... args) noexcept
    {
        ASSERT(m_size < N, "StaticVector out of capacity!");
        m_data[m_size++] = T(std::forward<Args>(args)...);
        return m_data[m_size - 1];
    }

    constexpr void pop_back() noexcept
    {
        ASSERT(m_size > 0, "StaticVector empty!");
        --m_size;
    }

    constexpr T& back() noexcept
    {
        ASSERT(m_size > 0, "StaticVector empty!");
        return m_data[m_size - 1];
    }

    constexpr const T& back() const noexcept
    {
        ASSERT(m_size > 0, "StaticVector empty!");
        return m_data[m_size - 1];
    }

    constexpr T& operator[](size_t i) noexcept
    {
        ASSERT(i < m_size, "Invalid index");
        return m_data[i];
    }

    constexpr const T& operator[](size_t i) const noexcept
    {
        ASSERT(i < m_size, "Invalid index");
        return m_data[i];
    }

    constexpr void remove(size_t i) noexcept
    {
        ASSERT(i < m_size, "Invalid index");
        m_data[i] = m_data[--m_size];
    }

    constexpr size_t size() const noexcept { return m_size; }
    constexpr size_t capacity() const noexcept { return N; }
    constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr T* begin() noexcept { return m_data; }
    constexpr T* end() noexcept { return m_data + m_size; }
    constexpr const T* begin() const noexcept { return m_data; }
    constexpr const T* end() const noexcept { return m_data + m_size; }

private:
    size_t m_size;
    T m_data[N];
};

}