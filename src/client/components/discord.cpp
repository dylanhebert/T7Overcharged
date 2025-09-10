#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "scheduler.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"
#include "havok/hks_api.hpp"
#include "havok/lua_api.hpp"

#include <utils/string.hpp>

#include <discord_rpc.h>

namespace discord
{
	DiscordRichPresence discord_presence;

	int roundsPlayed;
	int playerScore;
	int enemyScore;
	bool isIngame = false;
	const char* playerWeapon;
	int playerKills;

	void update_discord()
	{
		//std::cout << "Attempting to update Discord RPC" << std::endl;

		Discord_RunCallbacks();

		if (!isIngame)
		{
			discord_presence.details = "Zombies";
			discord_presence.state = "Lobby";
			roundsPlayed = 0;
			playerScore = 0;
			enemyScore = 0;
			playerWeapon = "None";
			playerKills = 0;

			discord_presence.startTimestamp = 0;

			discord_presence.largeImageKey = "deadhigh_splatter";
			discord_presence.largeImageText = "Dead High";
			discord_presence.smallImageKey = "bo3_logo_transparent";
			discord_presence.smallImageText = "Call of Duty: Black Ops III - Zombies";
		}
		else
		{
			static std::string detailsBuffer;
			detailsBuffer = "Round " + std::to_string(roundsPlayed) + " - " + playerWeapon;
			discord_presence.details = detailsBuffer.c_str();
			//discord_presence.state = utils::string::va("Round %d", roundsPlayed);

			if (!discord_presence.startTimestamp)
			{
				discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
			}

			discord_presence.largeImageKey = "deadhigh_splatter";
			discord_presence.largeImageText = "Dead High";
			discord_presence.smallImageKey = "bo3_logo_transparent";
			discord_presence.smallImageText = "Call of Duty: Black Ops III - Zombies";
		}

		discord_presence.partySize = game::LobbySession_GetClientCount(0, game::LobbyClientType::LOBBY_CLIENT_TYPE_ALL);
		discord_presence.partyMax = dvars::com_maxclients->current.integer;
		
		if (discord_presence.partySize == 1) 
		{
			discord_presence.state = "Playing Solo";
		}
		else 
		{
			discord_presence.state = "Playing Co-op";
		}

		// Persistent buttons
		discord_presence.button1_url = "https://deadhighstats.com";
		discord_presence.button1_label = "Dead High Website";
		discord_presence.button2_url = "https://discord.gg/jeqXWhzh5J";
		discord_presence.button2_label = "Join the Discord";

		//std::cout << "Updating Discord RPC - state: " << discord_presence.state << std::endl;
		Discord_UpdatePresence(&discord_presence);
		//std::cout << "Updated Discord RPC - state: " << discord_presence.state << std::endl;
	}

	int enable(lua::lua_State* s);

	int set_rounds_played(lua::lua_State* s)
	{
		roundsPlayed = lua::lua_tonumber(s, 1);
		return 1;
	}

	int set_playerscore(lua::lua_State* s)
	{
		playerScore = lua::lua_tonumber(s, 1);
		return 1;
	}

	int set_enemyscore(lua::lua_State* s)
	{
		enemyScore = lua::lua_tonumber(s, 1);
		return 1;
	}

	int set_playerweapon(lua::lua_State* s)
	{
		playerWeapon = lua::lua_tostring(s, 1);
		return 1;
	}

	int set_playerkills(lua::lua_State* s)
	{
		playerKills = lua::lua_tonumber(s, 1);
		return 1;
	}

	class component final : public component_interface
	{
	public:
		static void start_discord_rpc(const char* applicationId)
		{
			if (initialized_)
				return;

			DiscordEventHandlers handlers;
			ZeroMemory(&handlers, sizeof(handlers));
			handlers.ready = ready;
			handlers.errored = errored;
			handlers.disconnected = errored;
			handlers.joinGame = nullptr;
			handlers.spectateGame = nullptr;
			handlers.joinRequest = nullptr;

			Discord_Initialize(applicationId, &handlers, 1, nullptr);
			update_discord();
			scheduler::loop(update_discord, scheduler::pipeline::async, 6s);

			initialized_ = true;
			std::cout << "Initialized Discord RPC - ID: " << applicationId << std::endl;
		}

		void lua_start() override
		{
			const lua::luaL_Reg HotReloadLibrary[] =
			{
				{"Enable", enable},
				{"SetRoundsPlayed", set_rounds_played},
				{"SetPlayerScore", set_playerscore},
				{"SetEnemyScore", set_enemyscore},
				{"SetPlayerWeapon", set_playerweapon},
				{"SetPlayerKills", set_playerkills},
				{nullptr, nullptr},
			};
			hks::hksI_openlib(game::UI_luaVM, "DiscordRPC", HotReloadLibrary, 0, 1);
		}

		void start_hooks() override
		{
			isIngame = true;
			std::string raw_lua =
				"LUI.roots.UIRoot0:subscribeToGlobalModel(0, 'GameScore', 'roundsPlayed', function(model) "
				"local roundsPlayed = Engine.GetModelValue(model); "
				"if roundsPlayed then "
				"DiscordRPC.SetRoundsPlayed(roundsPlayed - 1); "
				"end; "
				"end); "
				"LUI.roots.UIRoot0:subscribeToGlobalModel(0, 'GameScore', 'playerScore', function(model) "
				"local playerScore = Engine.GetModelValue(model); "
				"if playerScore and not Engine.IsVisibilityBitSet( 0, Enum.UIVisibilityBit.BIT_IN_KILLCAM ) then "
				"DiscordRPC.SetPlayerScore(playerScore); "
				"end; "
				"end); "
				"LUI.roots.UIRoot0:subscribeToGlobalModel(0, 'GameScore', 'enemyScore', function(model) "
				"local enemyScore = Engine.GetModelValue(model); "
				"if enemyScore and not Engine.IsVisibilityBitSet( 0, Enum.UIVisibilityBit.BIT_IN_KILLCAM ) then "
				"DiscordRPC.SetEnemyScore(enemyScore); "
				"end; "
				"end); "
				"LUI.roots.UIRoot0:subscribeToGlobalModel(0, 'CurrentWeapon', 'weaponName', function(model) "
				"local playerWeapon = Engine.GetModelValue(model); "
				"if playerWeapon and not Engine.IsVisibilityBitSet( 0, Enum.UIVisibilityBit.BIT_IN_KILLCAM ) then "
				"DiscordRPC.SetPlayerWeapon(playerWeapon); "
				"end; "
				"end); ";
			hks::execute_raw_lua(raw_lua, "DiscordScoreModels");
		}

		void destroy_hooks() override
		{
			isIngame = false;
		}

	private:
		static inline bool initialized_ = false;

		static void ready(const DiscordUser*)
		{
			ZeroMemory(&discord_presence, sizeof(discord_presence));

			discord_presence.instance = 1;

			Discord_UpdatePresence(&discord_presence);
		}

		static void errored(const int error_code, const char* message)
		{
			std::cout << "Discord RPC Error: (" << error_code << ") " << message << std::endl;
			printf("Discord: (%i) %s", error_code, message);
		}
	};


	int enable(lua::lua_State* s)
	{
		auto applicationId = lua::lua_tostring(s, 1);
		discord::component::start_discord_rpc(applicationId);
		return 1;
	}
}

REGISTER_COMPONENT(discord::component)