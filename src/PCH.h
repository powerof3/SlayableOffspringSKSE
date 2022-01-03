#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

namespace logger = SKSE::log;
using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}
}

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
