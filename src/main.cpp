namespace NotAChild
{
	void Install()
	{
		//FF 90 F0 02 00 00	- call qword ptr [rax+2F0h]

		std::array targets{
			std::make_pair(37229, 0x32),   // canKillMe.
			std::make_pair(37896, 0x8F),   // killImpl.
			std::make_pair(37690, 0xD1),   // applySkinBloodDecals.
			std::make_pair(44217, 0x165),  // triggerMines.
			std::make_pair(38627, 0x22F),  // applyBash.
			std::make_pair(44031, 0x1E8),  // recoverArrows.
			std::make_pair(54660, 0x97),   // damageActorValue_Script
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

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Slayable Offspring SKSE");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
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
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	NotAChild::Install();
	MakeVunerable::Install();

	return true;
}
