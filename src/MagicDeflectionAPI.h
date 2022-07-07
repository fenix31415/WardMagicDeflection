#pragma once

namespace MagicDeflectionAPI
{
	struct DeflectionData
	{
		RE::NiPoint3 P;
		ProjectileRot rot;
	};

	class API
	{
	public:
		virtual bool can_deflect(RE::Actor*, RE::Projectile*) { return false; };
		virtual bool can_deflect_ward(RE::Actor*, RE::Projectile*) { return false; };
		virtual void on_deflect(RE::Actor*, RE::Projectile*, DeflectionData&) {}
		virtual void on_deflect_ward(RE::Actor*, RE::Projectile*, DeflectionData&) {}
	};

	typedef void (*AddSubscriber_t)(std::unique_ptr<API> api);
}
