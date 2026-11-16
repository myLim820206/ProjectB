// 가짜 언리얼 헤더 (Tests/syntax.sh). 내용은 FakeUE.h 에 모여 있다
#pragma once
#include "FakeUE.h"

class IModuleInterface
{
public:
	virtual ~IModuleInterface() = default;
	virtual void StartupModule() {}
	virtual void ShutdownModule() {}
};
class FDefaultGameModuleImpl : public IModuleInterface {};
#define IMPLEMENT_MODULE(ModuleImplClass, ModuleName) static_assert(std::is_base_of_v<IModuleInterface, ModuleImplClass>, #ModuleName);
#define IMPLEMENT_GAME_MODULE(ModuleImplClass, ModuleName) IMPLEMENT_MODULE(ModuleImplClass, ModuleName)
#define IMPLEMENT_PRIMARY_GAME_MODULE(ModuleImplClass, ModuleName, GameName) IMPLEMENT_MODULE(ModuleImplClass, ModuleName)
