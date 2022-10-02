#include "Hooks.h"

namespace Hooks
{
	namespace NotAChild
	{
		void Install()
		{
			//FF 90 F0 02 00 00	- call qword ptr [rax+2F0h]

			std::array targets{
#ifdef SKYRIM_AE
				std::make_pair(37229, 0x32),   // canKillMe.
				std::make_pair(37896, 0x8F),   // killImpl.
				std::make_pair(37690, 0xD1),   // applySkinBloodDecals.
				std::make_pair(44217, 0x165),  // triggerMines.
				std::make_pair(38627, 0x22F),  // applyBash.
				std::make_pair(44031, 0x1E8),  // recoverArrows.
				std::make_pair(54660, 0x97),   // damageActorValue_Script
#else
				std::make_pair(36247, 0x32),   // canKillMe
				std::make_pair(36872, 0x87),   // killImpl
				std::make_pair(36682, 0xE0),   // applySkinBloodDecals
				std::make_pair(43026, 0x164),  // triggerMines
				std::make_pair(37673, 0x220),  // applyBash
				std::make_pair(42856, 0x1D1),  // recoverArrows
				std::make_pair(53860, 0x97),   // damageActorValue_Script
#endif
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
					if (const auto actor = stl::adjust_pointer<RE::Character>(a_this, -0xA0); actor && actor->IsChild()) {
						result = false;
					}
				}
				return result;
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline constexpr std::size_t size = 0x04;
		};

		void Install()
		{
			stl::write_vfunc<RE::Character, 4, IsInvunerable>();
		    logger::info("Installed invunerability patch"sv);
		}
	}

	void Install()
	{
		NotAChild::Install();
		MakeVunerable::Install();
	}
}
