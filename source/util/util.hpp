/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Useful C++ classes/bindings for util data and pointers */

#pragma once

#include <cstdarg>
#include <cstring>
#include <utility>
#include "util/bmem.h"

/* RAII wrappers */

template<typename T>
class BPtr {
	T* ptr;

	BPtr(BPtr const&) = delete;

	BPtr& operator=(BPtr const&) = delete;

	public:
	inline BPtr(T* p = nullptr) : ptr(p) {}
	inline BPtr(BPtr&& other)
	{
		*this = std::move(other);
	}
	inline ~BPtr()
	{
		bfree(ptr);
	}

	inline T* operator=(T* p)
	{
		bfree(ptr);
		ptr = p;
		return p;
	}

	inline BPtr& operator=(BPtr&& other)
	{
		ptr       = other.ptr;
		other.ptr = nullptr;
		return *this;
	}

	inline operator T*()
	{
		return ptr;
	}
	inline T** operator&()
	{
		bfree(ptr);
		ptr = nullptr;
		return &ptr;
	}

	inline bool operator!()
	{
		return ptr == NULL;
	}
	inline bool operator==(T p)
	{
		return ptr == p;
	}
	inline bool operator!=(T p)
	{
		return ptr != p;
	}

	inline T* Get() const
	{
		return ptr;
	}
};
