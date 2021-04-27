#include "version.h"


class ChildCheckPatch
{
public:
	static void MineTrigger()
	{
		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				call(qword[rax + 0x830]);
			}
		};

		Patch patch;
		patch.ready();

		REL::Relocation<std::uintptr_t> target{ REL::ID(43026) }; 
		REL::safe_write(target.address() + 0x164, stl::span{ patch.getCode(), patch.getSize() });
	}

	static void IsValidMurderTarget()
	{
		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				call(qword[rax + 0x830]);
			}
		};

		Patch patch;
		patch.ready();

		REL::Relocation<std::uintptr_t> target{ REL::ID(36247) };
		REL::safe_write(target.address() + 0x32, stl::span{ patch.getCode(), patch.getSize() });
	}

	static void KillImpl() 
	{
		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				call(qword[rax + 0x830]);
			}
		};

		Patch patch;
		patch.ready();

		REL::Relocation<std::uintptr_t> target{ REL::ID(36872) };
		REL::safe_write(target.address() + 0x87, stl::span{ patch.getCode(), patch.getSize() });
	}
};


class Invunerable
{
public:
	static void Patch()
	{
		REL::Relocation<std::uintptr_t> vtbl{ REL::ID(261401) };
		_IsInvunerable = vtbl.write_vfunc(0x04, IsInvunerable);

		logger::info("patching IsInvunerable");
	}


	static bool IsInvunerable(RE::MagicTarget* a_this)
	{
		auto result = _IsInvunerable(a_this);
		if (result && a_this) {
			auto actor = adjust_pointer<RE::Character>(a_this, -0x98);
			if (actor && actor->IsChild()) {
				result = false;
			}
		}
		return result;
	}
	using IsInvunerable_t = decltype(&RE::Actor::IsInvulnerable);  // 04
	static inline REL::Relocation<IsInvunerable_t> _IsInvunerable;
};


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	try {
		auto path = logger::log_directory().value() / "po3_SlayableOffsprings.log";
		auto log = spdlog::basic_logger_mt("global log", path.string(), true);
		log->flush_on(spdlog::level::info);

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
		log->set_level(spdlog::level::info);

#endif
		spdlog::set_default_logger(log);
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");

		logger::info("Slayable Offsprings SKSE {}", SOS_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Slayable Offsprings SKSE";
		a_info->version = SOS_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_1_5_39) {
			logger::critical("Unsupported runtime version {}", ver.string());
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		logger::info("Slayable Offsprings SKSE loaded");

		SKSE::Init(a_skse);

		ChildCheckPatch::KillImpl();
		ChildCheckPatch::IsValidMurderTarget();
		Invunerable::Patch();

	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}
