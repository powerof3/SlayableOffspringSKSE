#include "version.h"

bool applyBloodDecals;
bool triggerMines;
bool applyBash;
bool recoverArrows;

class IsChild
{
public:
	static void Patch()
	{
		//FF 90 F0 02 00 00	- call qword ptr [rax+2F0h]

		std::array targets{
			std::make_tuple(true, 36247, 0x32),				 // isValidMurderTarget
			std::make_tuple(true, 36872, 0x87),				 // killImpl
			std::make_tuple(applyBloodDecals, 36682, 0xE0),	 // applySkinBloodDecals
			std::make_tuple(triggerMines, 43026, 0x164),	 // triggerMines
			std::make_tuple(applyBash, 37673, 0x220),		 // applyBash
			std::make_tuple(recoverArrows, 42856, 0x1D1),	 // recoverArrows
			std::make_tuple(true, 53860, 0x97),				 // damageActorValue_Script
		};

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				call(qword[rax + 0x830]);  //unused vfunc? that returns false for actor classes
			}
		};

		Patch patch;
		patch.ready();

		for (const auto& [toggle, id, offset] : targets) {
			if (toggle) {
				REL::Relocation<std::uintptr_t> target{ REL::ID(id) };
				REL::safe_write(target.address() + offset, stl::span{ patch.getCode(), patch.getSize() });
			}
		}
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


void LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_SlayableOffspring.ini";

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	ini.LoadFile(path);

	applyBloodDecals = ini.GetBoolValue("Settings", "Apply Blood Decals", true);
	ini.SetBoolValue("Settings", "Apply Blood Decals", applyBloodDecals, ";Children will bleed on hit.", true);

	triggerMines = ini.GetBoolValue("Settings", "Trigger Runes", true);
	ini.SetBoolValue("Settings", "Trigger Runes", triggerMines, ";Children will be able to trip runes.", true);

	applyBash = ini.GetBoolValue("Settings", "Apply Bash Perks", true);
	ini.SetBoolValue("Settings", "Trigger Runes", applyBash, ";Allows bash perk entry to work on children.", true);

	recoverArrows = ini.GetBoolValue("Settings", "Recover Arrows", true);
	ini.SetBoolValue("Settings", "Recover Arrows", recoverArrows, ";Player can recover arrows from corpses.", true);

	ini.SaveFile(path);
}


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

		logger::info("Slayable Offspring SKSE {}", SOS_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Slayable Offspring SKSE";
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
		logger::info("Slayable Offspring SKSE loaded");

		SKSE::Init(a_skse);

		LoadSettings();

		IsChild::Patch();
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
