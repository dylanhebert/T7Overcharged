#include <std_include.hpp>

#include <windows.h>
#include <iostream>
#include "game/game.hpp"
#include "havok/hks_api.hpp"
#include "loader/component_loader.hpp"

void AttachConsoleWindow() {
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::cerr.clear();
	std::cin.clear();
	std::cout << "Console attached to DLL." << std::endl;
}

extern "C"
{
	int __declspec(dllexport) init(lua::lua_State* L)
	{
		#ifdef _DEBUG
		//AttachConsoleWindow();
		#endif
		std::cout << "Initializing T7Overcharged..." << std::endl;

		game::minlog.WriteLine("T7Overchared initiating");

		const lua::luaL_Reg T7OverchargedLibrary[] =
		{
			{nullptr, nullptr},
		};
		hks::hksI_openlib(L, "T7Overcharged", T7OverchargedLibrary, 0, 1);

		if (!component_loader::post_start())
		{
			game::Com_Error_("", 0, 0x200u, "Error while loading T7Overcharged components");
			std::cerr << "Error while loading T7Overcharged components." << std::endl;
			game::minlog.WriteLine("Error while loading T7Overcharged components");
			return 0;
		}

		game::minlog.WriteLine("T7Overchared initiated");
		std::cout << "T7Overcharged initialized successfully." << std::endl;

		game::LoadDvarHashMap();
		return 1;
	}
}