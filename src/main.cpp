namespace NotAChild
{
	void Install()
	{
		//FF 90 F0 02 00 00	- call qword ptr [rax+2F0h]

		std::array targets{
			std::make_pair(36247, 0x32),   // canKillMe
			std::make_pair(36872, 0x87),   // killImpl
			std::make_pair(36682, 0xE0),   // applySkinBloodDecals
			std::make_pair(43026, 0x164),  // triggerMines
			std::make_pair(37673, 0x220),  // applyBash
			std::make_pair(42856, 0x1D1),  // recoverArrows
			std::make_pair(53860, 0x97),   // damageActorValue_Script
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

		for (const auto& [id, offset] : targets) {
			REL::Relocation<std::uintptr_t> target{ REL::ID(id) };
			REL::safe_write(target.address() + offset, std::span{ patch.getCode(), patch.getSize() });
		}
	}
}

namespace MakeVunerable
{
	struct IsInvunerable
	{
		static bool thunk(RE::MagicTarget* a_this)
		{
			auto result = func(a_this);
			if (result && a_this) {
				if (const auto actor = stl::adjust_pointer<RE::Character>(a_this, -0x98); actor && actor->IsChild()) {
					result = false;
				}
			}
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static inline constexpr std::size_t size = 0x04;
	};

	inline void Install()
	{
		stl::write_vfunc<RE::Character, 4, IsInvunerable>();
		logger::info("Installed invunerability patch"sv);
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Slayable Offsprings SKSE";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded");

	SKSE::Init(a_skse);

	NotAChild::Install();
	MakeVunerable::Install();

	return true;
}
