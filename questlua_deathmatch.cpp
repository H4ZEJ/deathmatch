#include "stdafx.h"
#include "questlua.h"
#include "questmanager.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "deathmatch.h"

namespace quest
{
	int deathmatch_start(lua_State* L)
	{
		int iStatus = (int)lua_tonumber(L, 1);
		int iMap = (int)lua_tonumber(L, 2);
		CDeathMatchPvP::instance().SetStatus(iStatus, iMap);
		return 0;
	}

	int deathmatch_get_status(lua_State* L)
	{
		lua_pushnumber(L, CDeathMatchPvP::instance().GetStatus());
		return 1;
	}

	int deathmatch_set_member_limit(lua_State* L)
	{
		int iMember = (int)lua_tonumber(L, 1);
		CDeathMatchPvP::instance().SetMemberLimitCount(iMember);
		return 0;
	}

	int deathmatch_get_member_limit(lua_State* L)
	{
		lua_pushnumber(L, CDeathMatchPvP::instance().GetMemberLimitCount());
		return 1;
	}

	int deathmatch_get_total_member_count(lua_State* L)
	{
		lua_pushnumber(L, CDeathMatchPvP::instance().GetTotalMemberCount());
		return 1;
	}

	int deathmatch_destroy(lua_State* L)
	{
		CDeathMatchPvP::instance().Destroy();
		return 0;
	}

	int deathmatch_get_winner_player(lua_State* L)
	{
		lua_pushnumber(L, CDeathMatchPvP::instance().GetWinnerPlayer());
		return 1;
	}

	void RegisterDeathMatchFunctionTable()
	{
		luaL_reg deathmatch_functions[] =
		{
			{ "start", deathmatch_start },
			{ "get_status", deathmatch_get_status },
			{ "set_member_limit", deathmatch_set_member_limit },
			{ "get_member_limit", deathmatch_get_member_limit },
			{ "get_total_member_count", deathmatch_get_total_member_count },
			{ "get_winner_player", deathmatch_get_winner_player },
			{ "destroy", deathmatch_destroy },
			{ NULL, NULL }
		};

		CQuestManager::instance().AddLuaFunctionTable("deathmatch", deathmatch_functions);
	}
}
