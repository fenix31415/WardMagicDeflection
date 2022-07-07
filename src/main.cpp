extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef DEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef DEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
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

#include "Hooks.h"
#include "FenixProjectilesAPI.h"
#include "MagicDeflectionAPI.h"

using namespace MagicDeflectionAPI;

class WardDeflection : public API
{
	static bool is_dual_casting_ward(RE::Character* a)
	{
		for (int i = 0; i < RE::Actor::SlotTypes::kTotal; i++) {
			if (auto mcaster = a->magicCasters[i]; mcaster && mcaster->currentSpell) {
				auto spel = mcaster->currentSpell;
				if (auto mgef = FenixUtils::getAVEffectSetting(spel); mgef && a->IsCasting(spel) && mcaster->GetIsDualCasting()) {
					if (mgef->data.castingType == RE::MagicSystem::CastingType::kConcentration &&
						mgef->GetArchetype() == RE::EffectSetting::Archetype::kAccumulateMagnitude &&
						mgef->data.primaryAV == RE::ActorValue::kWardPower)
						return true;
				}
			}
		}
		return false;
	}

	static bool is_casting_ward(RE::Character* a)
	{
		for (int i = 0; i < RE::Actor::SlotTypes::kTotal; i++) {
			if (auto mcaster = a->magicCasters[i]; mcaster && mcaster->currentSpell) {
				auto spel = mcaster->currentSpell;
				if (auto mgef = FenixUtils::getAVEffectSetting(spel); mgef && a->IsCasting(spel)) {
					if (mgef->data.castingType == RE::MagicSystem::CastingType::kConcentration &&
						mgef->GetArchetype() == RE::EffectSetting::Archetype::kAccumulateMagnitude &&
						mgef->data.primaryAV == RE::ActorValue::kWardPower)
						return true;
				}
			}
		}
		return false;
	}


	bool can_deflect_ward(RE::Actor* target, RE::Projectile* proj) override
	{
		if (!target || !target->As<RE::Character>() ||
			proj->spell->GetCastingType() != RE::MagicSystem::CastingType::kFireAndForget ||
			(!proj->IsMissileProjectile() && !proj->IsBeamProjectile()))
			return false;

		auto a = target->As<RE::Character>();
		if (a->IsPlayerRef()) {
			if (is_dual_casting_ward(a)) {
				return FenixUtils::random(a->GetActorValue(RE::ActorValue::kWardPower) * 0.01f);
			}
		} else {
			if (is_casting_ward(a)) {
				return FenixUtils::random(0.2f);
			}
		}
		return false;
	};

	void on_deflect_ward(RE::Actor* victim, RE::Projectile* proj, DeflectionData& ddata) override
	{
		auto bone = victim->Get3D();
		if (!bone)
			return;

		auto n = bone->world.rotate * RE::NiPoint3{ 0, 1, 0 };
		auto d = proj->velocity;

		auto v = d - n * (2 * d.Dot(n));
		v.Unitize();

		ddata.rot = { 0, atan2(v.x, v.y) };
	}
};

void addSubscriber()
{
	if (auto pluginHandle = GetModuleHandleA("MagicDeflectionAPI.dll")) {
		if (auto AddSubscriber = (AddSubscriber_t)GetProcAddress(pluginHandle, "MagicDeflectionAPI_AddSubscriber")) {
			AddSubscriber(std::make_unique<WardDeflection>());
		}
	}
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		PlayerCharacterHook::Hook();
		addSubscriber();

		break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	auto g_messaging = reinterpret_cast<SKSE::MessagingInterface*>(a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));
	if (!g_messaging) {
		logger::critical("Failed to load messaging interface! This error is fatal, plugin will not load.");
		return false;
	}

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(1 << 10);

	g_messaging->RegisterListener("SKSE", SKSEMessageHandler);

	logger::info("loaded");
	return true;
}
