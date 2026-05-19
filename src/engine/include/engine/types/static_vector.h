#pragma once

#include <utility>
#include <engine/core/core.h>

namespace Bratwurst
{

    // zero-abstraction cost stack allocated vector like container
    template<typename T, size_t N>
    class StaticVector
    {
    public:
        constexpr StaticVector();

        template<typename... Args>
        constexpr T& emplace(Args&&... args);
        constexpr void push(const T& value);
        constexpr void push(T&& value);
        
        constexpr void remove(size_t i);
        constexpr void pop();
		constexpr void clear() { m_size = 0; }

        constexpr T& back();
        constexpr const T& back() const;
        constexpr T& get(size_t i);
        constexpr const T& get(size_t i) const;

        constexpr T& operator[](size_t i);
        constexpr const T& operator[](size_t i) const;

        constexpr size_t size() const { return m_size; }
        constexpr size_t capacity() const { return N; }
        constexpr bool empty() const { return m_size == 0; }

        constexpr T* begin() { return m_data; }
        constexpr T* end() { return m_data + m_size; }
        constexpr const T* begin() const { return m_data; }
        constexpr const T* end() const { return m_data + m_size; }

    private:
        size_t m_size;
        T m_data[N];
    };

    template<typename T, size_t N>
    constexpr StaticVector<T, N>::StaticVector()
        : m_size(0)
    {
    }

    template<typename T, size_t N>
    constexpr void StaticVector<T, N>::push(const T& value)
    {
        ASSERT_MSG(m_size < N, "StaticVector out of capacity!");
        m_data[m_size++] = value;
    }

    template<typename T, size_t N>
    constexpr void StaticVector<T, N>::push(T&& value)
    {
        ASSERT_MSG(m_size < N, "StaticVector out of capacity!");
        m_data[m_size++] = std::move(value);
    }

    template<typename T, size_t N>
    template<typename... Args>
    constexpr T& StaticVector<T, N>::emplace(Args&&... args)
    {
        ASSERT_MSG(m_size < N, "StaticVector out of capacity!");
        m_data[m_size++] = T(std::forward<Args>(args)...);
        return m_data[m_size - 1];
    }

    template<typename T, size_t N>
    constexpr void StaticVector<T, N>::pop()
    {
        ASSERT_MSG(m_size > 0, "StaticVector empty!");
        --m_size;
    }

    template<typename T, size_t N>
    constexpr T& StaticVector<T, N>::back()
    {
        ASSERT_MSG(m_size > 0, "StaticVector empty!");
        return m_data[m_size - 1];
    }

    template<typename T, size_t N>
    constexpr const T& StaticVector<T, N>::back() const
    {
        ASSERT_MSG(m_size > 0, "StaticVector empty!");
        return m_data[m_size - 1];
    }

    template<typename T, size_t N>
    inline constexpr T& StaticVector<T, N>::get(size_t i)
    {
        ASSERT_MSG(i < m_size, "index out of bounds!");
        return m_data[i];
    }

    template<typename T, size_t N>
    constexpr const T& StaticVector<T, N>::get(size_t i) const
    {
        ASSERT_MSG(i < m_size, "index out of bounds!");
        return m_data[i];
    }

    template<typename T, size_t N>
    constexpr T& StaticVector<T, N>::operator[](size_t i)
    {
        ASSERT_MSG(i < m_size, "index out of bounds!");
        return get(i);
    }

    template<typename T, size_t N>
    constexpr const T& StaticVector<T, N>::operator[](size_t i) const
    {
        ASSERT_MSG(i < m_size, "index out of bounds!");
        return get(i);
    }

    template<typename T, size_t N>
    constexpr void StaticVector<T, N>::remove(size_t i)
    {
        ASSERT_MSG(i < m_size, "Invalid index");
        m_data[i] = m_data[--m_size];
    }
}