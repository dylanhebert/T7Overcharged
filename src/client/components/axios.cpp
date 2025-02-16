#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "havok/hks_api.hpp"
#include "havok/lua_api.hpp"
#include "game/game.hpp"
#include <thread>
#include <future>
#include <functional>

namespace axios
{
    // GET
    int get(lua::lua_State* s)
    {
        auto url = lua::lua_tostring(s, 1);
        std::cout << "Requesting URL: " << url << std::endl;

        try
        {
            http::Request request{ url };
            const auto response = request.send("GET");

            std::cout << "Response Status: " << std::to_string(response.status.code) << std::endl;

            const auto responseBody = std::string{ response.body.begin(), response.body.end() };

            if (responseBody.empty()) {
                std::cout << "Response body is empty" << std::endl;
            }
            else {
                std::cout << "Response Body: " << responseBody << std::endl;
            }

            lua::lua_pushstring(s, responseBody.c_str());
        }
        catch (const http::ResponseError& e)
        {
            std::cout << "Request FAILED: " << e.what() << std::endl;
            lua::lua_pushstring(s, "FAILED\n");
        }
        catch (const std::exception& e)
        {
            std::cout << "Unexpected error: " << e.what() << std::endl;
            lua::lua_pushstring(s, "ERROR\n1");
        }
        catch (...)
        {
            std::cout << "Unknown Exception Caught!" << std::endl;
            lua::lua_pushstring(s, "ERROR\n2");
        }

        return 1;
    }

    // POST - SENDS THE RESPONSE TO LUA
    int post_and_response(lua::lua_State* s)
    {
        auto url = lua::lua_tostring(s, 1);
        auto body = lua::lua_tostring(s, 2);

        try
        {
            http::Request request{ url };
            const auto response = request.send("POST", body, {
                {"Content-Type", "application/json"}
                });

            std::cout << "Response Status: " << std::to_string(response.status.code) << std::endl;

            const auto responseBody = std::string{ response.body.begin(), response.body.end() };

            if (responseBody.empty()) {
                std::cout << "Response body is empty" << std::endl;
            }
            else {
                std::cout << "Response Body: " << responseBody << std::endl;
            }

            lua::lua_pushstring(s, responseBody.c_str());
        }
        catch (const http::ResponseError& e)
        {
            std::cout << "Request FAILED: " << e.what() << std::endl;
            lua::lua_pushstring(s, "FAILED\n");
        }
        catch (const std::exception& e)
        {
            std::cout << "Unexpected error: " << e.what() << std::endl;
            lua::lua_pushstring(s, "ERROR\n1");
        }
        catch (...)
        {
            std::cout << "Unknown Exception Caught!" << std::endl;
            lua::lua_pushstring(s, "ERROR\n2");
        }

        return 1;
    }

    // POST - DOESNT HAVE CALLBACK
    int post(lua::lua_State* s)
    {
        auto url = lua::lua_tostring(s, 1);
        auto body = lua::lua_tostring(s, 2);

        // Start asynchronous HTTP POST request
        std::thread([s, url, body]()
        {
            try 
            {
                http::Request request{ url };
                const auto response = request.send("POST", body, {
                    {"Content-Type", "application/json"}
                    });

                game::minlog.WriteLine("post SUCCESS");

            }
            catch (const http::ResponseError& e)
            {
                game::minlog.WriteLine("post FAILED");
            }
            catch (const std::exception& e)
            {
                game::minlog.WriteLine("post ERROR1");
            }
            catch (...)
            {
                game::minlog.WriteLine("post ERROR2");
            }

        }).detach();

        // Return immediate confirmation to Lua
        lua::lua_pushstring(s, "POST request started");
        return 1;
    }

	class component final : public component_interface
	{
	public:
		void lua_start() override
		{
			const lua::luaL_Reg AxiosLibrary[] =
			{
				{"Get", get},
				{"Post", post},
                {"PostAndResponse", post_and_response},
				{nullptr, nullptr},
			};
			hks::hksI_openlib(game::UI_luaVM, "Axios", AxiosLibrary, 0, 1);
		}
	};
}

REGISTER_COMPONENT(axios::component)