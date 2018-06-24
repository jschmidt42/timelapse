#pragma once

#include "foundation/assert.h"
#include "foundation/memory.h"

template <class T>
int num_digits(T number)
{
    int digits = 0;
    if (number < 0) digits = 1; // remove this line if '-' counts as a digit
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}

size_t string_occurence(const char* str, size_t len, char c);

namespace generics {

    template<typename T> T min(T a, T b) { return (((a) < (b)) ? (a) : (b)); }
    template<typename T> T max(T a, T b) { return (((a) > (b)) ? (a) : (b)); }

    template<typename T>
    class vector
    {
    public:
        size_t Size;
        size_t Capacity;
        T* Data;

        typedef T value_type;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;

        typedef value_type* reverse_iterator;
        typedef const value_type* const_reverse_iterator;

        vector()
            : Size(0)
            , Capacity(0)
            , Data(nullptr)
        {
        }

        ~vector() { if (Data) memory_deallocate(Data); }

        vector(vector<T>&& src) noexcept
            : Size(src.Size)
            , Capacity(src.capacity())
            , Data(src.Data)
        {
            src.Size = src.Capacity = 0;
            src.Data = nullptr;
        }

        vector(const vector<T>& src)
            : Size(0)
            , Capacity(0)
            , Data(nullptr)
        {
            operator=(src);
        }

        vector& operator=(vector<T>&& src) noexcept
        {
            Size = src.Size;
            Capacity = src.Capacity;
            Data = src.Data;
            
            src.Size = src.Capacity = 0;
            src.Data = nullptr;

            return *this;
        }

        vector& operator=(const vector<T>& src)
        {
            clear();
            resize(src.Size);
            memcpy(Data, src.Data, (size_t)Size * sizeof(value_type));
            return *this;
        }

        bool empty() const { return Size == 0; }
        size_t size() const { return Size; }
        size_t capacity() const { return Capacity; }

        value_type& operator[](size_t i)
        {
            FOUNDATION_ASSERT(i < Size);
            return Data[i];
        }

        const value_type& operator[](size_t i) const
        {
            FOUNDATION_ASSERT(i < Size);
            return Data[i];
        }

        void clear()
        {
            if (Data)
            {
                Size = Capacity = 0;
                memory_deallocate(Data);
                Data = NULL;
            }
        }

        iterator begin() { return Data; }
        const_iterator begin() const { return Data; }
        iterator end() { return Data + Size; }
        const_iterator end() const { return Data + Size; }

        value_type& front()
        {
            FOUNDATION_ASSERT(Size > 0);
            return Data[0];
        }

        const value_type& front() const
        {
            FOUNDATION_ASSERT(Size > 0);
            return Data[0];
        }

        value_type& back()
        {
            FOUNDATION_ASSERT(Size > 0);
            return Data[Size - 1];
        }

        const value_type& back() const
        {
            FOUNDATION_ASSERT(Size > 0);
            return Data[Size - 1];
        }

        void swap(vector<value_type>& rhs) noexcept
        {
            size_t rhs_size = rhs.Size;
            rhs.Size = Size;
            Size = rhs_size;
            size_t rhs_cap = rhs.Capacity;
            rhs.Capacity = Capacity;
            Capacity = rhs_cap;
            value_type* rhs_data = rhs.Data;
            rhs.Data = Data;
            Data = rhs_data;
        }

        size_t _grow_capacity(size_t sz) const
        {
            size_t new_capacity = Capacity ? (Capacity + Capacity / 2) : 8;
            return new_capacity > sz ? new_capacity : sz;
        }

        void resize(size_t new_size)
        {
            if (new_size > Capacity) reserve(_grow_capacity(new_size));
            Size = new_size;
        }

        void resize(size_t new_size, const value_type& v)
        {
            if (new_size > Capacity) reserve(_grow_capacity(new_size));
            if (new_size > Size) for (size_t n = Size; n < new_size; n++) memcpy(&Data[n], &v, sizeof(v));
            Size = new_size;
        }

        void reserve(size_t new_capacity)
        {
            if (new_capacity <= Capacity)
                return;
            value_type* new_data = (value_type*)memory_allocate(0, (size_t)new_capacity * sizeof(value_type), 0, 0);
            if (Data)
            {
                memcpy(new_data, Data, (size_t)Size * sizeof(value_type));
                memory_deallocate(Data);
            }
            Data = new_data;
            Capacity = new_capacity;
        }

        // NB: It is forbidden to call push_back/push_front/insert with a reference pointing inside the ImVector data itself! e.g. v.push_back(v[10]) is forbidden.
        void push_back(const value_type& v)
        {
            if (Size == Capacity) reserve(_grow_capacity(Size + 1));
            memcpy(&Data[Size], &v, sizeof(v));
            Size++;
        }

        void pop_back()
        {
            FOUNDATION_ASSERT(Size > 0);
            Size--;
        }

        void push_front(const value_type& v)
        {
            if (Size == 0) push_back(v);
            else insert(Data, v);
        }

        iterator erase(const_iterator it)
        {
            IM_ASSERT(it >= Data && it < Data + Size);
            const ptrdiff_t off = it - Data;
            memmove(Data + off, Data + off + 1, ((size_t)Size - (size_t)off - 1) * sizeof(value_type));
            Size--;
            return Data + off;
        }

        iterator erase(const_iterator it, const_iterator it_last)
        {
            IM_ASSERT(it >= Data && it < Data + Size && it_last > it && it_last <= Data + Size);
            const ptrdiff_t count = it_last - it;
            const ptrdiff_t off = it - Data;
            memmove(Data + off, Data + off + count, ((size_t)Size - (size_t)off - count) * sizeof(value_type));
            Size -= (size_t)count;
            return Data + off;
        }

        iterator erase_unsorted(const_iterator it)
        {
            IM_ASSERT(it >= Data && it < Data + Size);
            const ptrdiff_t off = it - Data;
            if (it < Data + Size - 1) memcpy(Data + off, Data + Size - 1, sizeof(value_type));
            Size--;
            return Data + off;
        }

        iterator insert(const_iterator it, const value_type& v)
        {
            IM_ASSERT(it >= Data && it <= Data + Size);
            const ptrdiff_t off = it - Data;
            if (Size == Capacity) reserve(_grow_capacity(Size + 1));
            if (off < (size_t)Size) memmove(Data + off + 1, Data + off, ((size_t)Size - (size_t)off) * sizeof(value_type));
            memcpy(&Data[off], &v, sizeof(v));
            Size++;
            return Data + off;
        }

        bool contains(const value_type& v) const
        {
            const T* data = Data;
            const T* data_end = Data + Size;
            while (data < data_end) if (*data++ == v) return true;
            return false;
        }
    };
}
