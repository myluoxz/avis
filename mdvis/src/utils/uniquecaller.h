#pragma once
#include "Engine.h"

namespace std
{
    template<typename T, size_t N>
    struct hash<array<T, N> >
    {
        typedef array<T, N> argument_type;
        typedef size_t result_type;

        result_type operator()(const argument_type& a) const
        {
            hash<T> hasher;
            result_type h = 0;
            for (result_type i = 0; i < N; ++i)
            {
                h = h * 31 + hasher(a[i]);
            }
            return h;
        }
    };
}
typedef std::array<uintptr_t, UI_MAX_EDIT_TEXT_FRAMES> traceframes;

class UniqueCaller {
public:
	traceframes frames;
	ushort id;

	bool operator== (const UniqueCaller& rhs);
};

class UniqueCallerList {
public:
	std::unordered_map<traceframes, ushort> frames;

	UniqueCaller current, last;

	void Preloop();
	bool Add();
	void Set(), Clear();
};