#pragma once

class PlayerCharacterHook
{
public:
	static void Hook()
	{
#ifdef DEBUG
		_Update = REL::Relocation<uintptr_t>(REL::ID(RE::VTABLE_PlayerCharacter[0])).write_vfunc(0xad, Update);
#endif
	}

private:
	static void Update(RE::PlayerCharacter* a, float delta)
	{
		_Update(a, delta);

#ifdef DEBUG
		DebugAPI_IMPL::DebugAPI::Update();
#endif
	}

	static inline REL::Relocation<decltype(Update)> _Update;
};
