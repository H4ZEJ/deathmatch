#include "stdafx.h"
#include "char.h"
#include "utils.h"
#include "log.h"
#include "db.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "char_manager.h"
#include "questmanager.h"
#include "start_position.h"
#include "sectree_manager.h"
#include "deathmatch_map.h"

void CDeathMatchPvP::OnLogin(LPCHARACTER ch)
{
	if (ch->GetMapIndex() == DEATHMATCH_MAP_INDEX && !ch->IsGM())
	{
		if (GetStatus() == DEATHMATCH_STATUS_CLOSED)
			ch->GoHome();
		
	}
}

/*** Unclassed Functions ***/
bool SortPlayers(CDeathMatchPvP::PlayerData i, CDeathMatchPvP::PlayerData j)
{
	return (i.iKills > j.iKills);
}
/*** End Of Unclassed Functions ***/

/*** Team Data Begin ***/
int CDeathMatchPvP::SPlayerData::GetAccumulatedJoinerCount()
{
	return set_pidJoiner.size();
}

int CDeathMatchPvP::SPlayerData::GetCurJointerCount()
{
	return iMemberCount;
}

void CDeathMatchPvP::SPlayerData::AppendMember(LPCHARACTER ch)
{
	set_pidJoiner.insert(ch->GetPlayerID());
	++iMemberCount;
}

void CDeathMatchPvP::SPlayerData::RemoveMember(LPCHARACTER ch)
{
	--iMemberCount;
}

void CDeathMatchPvP::SPlayerData::AppendKills(int kills)
{
	iKills += kills;
}

void CDeathMatchPvP::SPlayerData::Initialize()
{
	dwID = 0;
	iMemberCount = 0;
	iKills = 0;
	set_pidJoiner.clear();
}
/*** Team Data End ***/

/*** Public Team Data Functions ***/
CDeathMatchPvP::PlayerData* CDeathMatchPvP::GetTable(DWORD dwID)
{
	PlayerDataMap::iterator itor = m_PlayerDataMap.find(dwID);

	if (itor == m_PlayerDataMap.end())
		return NULL;

	return itor->second;
}

DWORD CDeathMatchPvP::GetPlayerKills(DWORD dwID)
{
	CDeathMatchPvP::PlayerData* c_pTable = GetTable(dwID);
	if (!c_pTable)
		return 0;

	return c_pTable->iKills;
}

DWORD CDeathMatchPvP::GetMemberCount(DWORD dwID)
{
	CDeathMatchPvP::PlayerData* c_pTable = GetTable(dwID);
	if (!c_pTable)
		return 0;

	return c_pTable->iMemberCount;
}
/*** End Of Public Team Data Functions ***/

/*** Public Append/Remove ***/
void CDeathMatchPvP::IncMember(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!ch->IsPC())
		return;

	PlayerData* pPlayer = GetTable(ch->GetPlayerID());
	if (!pPlayer)
	{
		AppendPlayer(ch);
	}

	pPlayer = GetTable(ch->GetPlayerID());
	if (pPlayer)
	{
		pPlayer->AppendMember(ch);
		sys_log(0, "CDeathMatchPvP::DeathMatch Append Member: %s %d", ch->GetName(), ch->GetPlayerID());
	}

	m_set_pkChr.insert(ch);
}

void CDeathMatchPvP::DecMember(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!ch->IsPC())
		return;

	PlayerData* pPlayer = GetTable(ch->GetPlayerID());

	if (pPlayer)
	{
		pPlayer->RemoveMember(ch);
		sys_log(0, "CDeathMatchPvP::DeathMatch Remove Member: %s %d", ch->GetName(), ch->GetPlayerID());
	}

	m_set_pkChr.erase(ch);
}

void CDeathMatchPvP::AppendPlayer(LPCHARACTER ch)
{
	PlayerData* pPlayerData = new PlayerData;
	pPlayerData->dwID = ch->GetPlayerID();
	pPlayerData->iMemberCount = 0;
	pPlayerData->iKills = 0;
	m_PlayerDataMap.insert(PlayerDataMap::value_type(ch->GetPlayerID(), pPlayerData));
}

void CDeathMatchPvP::OnKills(LPCHARACTER ch, int kills)
{
	PlayerData* pPlayer = GetTable(ch->GetPlayerID());

	if (pPlayer)
	{
		pPlayer->AppendKills(kills);
	}

	SendScorePacket();
}
/*** End Of Public Append Remove ***/

/*** Callable Funcs ***/
DWORD CDeathMatchPvP::GetWinnerPlayer()
{
	std::vector<CDeathMatchPvP::PlayerData> sendLists;

	for (PlayerDataMap::iterator itor = m_PlayerDataMap.begin(); itor != m_PlayerDataMap.end(); ++itor)
		sendLists.push_back(*itor->second);

	std::stable_sort(sendLists.begin(), sendLists.end(), SortPlayers);

	std::vector<CDeathMatchPvP::PlayerData>::iterator ch;
	int list = 0;
	for (ch = sendLists.begin(); ch != sendLists.end(); ++ch)
	{
		++list;
		DWORD dwID = ch->dwID;
		if (list == 1)
		{
			return dwID;
		}
	}

	return 0;
}
/*** End Of Callable Funcs ***/

/*** Status And Map ***/
void CDeathMatchPvP::SetStatus(int iStatus, int iMap)
{
	m_iDeathMatchMapIndex = iMap; // set mapindex
	m_iStatus = iStatus; // set status
}

int CDeathMatchPvP::GetStatus()
{
	return m_iStatus;
}

bool CDeathMatchPvP::IsDeathMatchMap(int mapindex)
{
	return m_iDeathMatchMapIndex == mapindex ? true : false;
}

bool CDeathMatchPvP::IsDeathMatchActivate()
{
	return m_iStatus == DEATHMATCH_STATUS_OPENED ? true : false;
}
/*** End Of Status And Map ***/

/*** Member Limit Count ***/
void CDeathMatchPvP::SetMemberLimitCount(int memberlimit)
{
	m_iMemberLimitCount = memberlimit;
}

int CDeathMatchPvP::GetMemberLimitCount()
{
	return m_iMemberLimitCount;
}
/*** End Of Member Limit Count ***/

/*** Packets Begin ***/
namespace
{
	struct FPacket
	{
		FPacket(const void* p, int size) : m_pvData(p), m_iSize(size)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			ch->GetDesc()->Packet(m_pvData, m_iSize);
		}

		const void* m_pvData;
		int m_iSize;
	};

	struct FNotice
	{
		FNotice(const char* psz) : m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char* m_psz;
	};

	struct FGoToVillage
	{
		void operator() (LPCHARACTER ch)
		{
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	};
};

void CDeathMatchPvP::Notice(const char* psz)
{
	FNotice f(psz);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CDeathMatchPvP::Packet(const void* p, int size)
{
	FPacket f(p, size);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CDeathMatchPvP::SendHome()
{
	FGoToVillage f;
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CDeathMatchPvP::SendScorePacket()
{
	if (!IsDeathMatchActivate())
		return;

	std::vector<CDeathMatchPvP::PlayerData> sendLists;
	for (PlayerDataMap::iterator itor = m_PlayerDataMap.begin(); itor != m_PlayerDataMap.end(); ++itor)
		sendLists.push_back(*itor->second);

	std::stable_sort(sendLists.begin(), sendLists.end(), SortPlayers);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(GetPlayerID());

	// send
	std::vector<CDeathMatchPvP::PlayerData>::iterator ch;
	int list = 0;
	for (ch = sendLists.begin(); ch != sendLists.end(); ++ch)
	{
		DWORD dwID = ch->dwID;

		TPacketGCDeathMatch p;
		p.header = HEADER_GC_DeathMatch;
		p.dwPlayerID = dwID;
		p.dwKills = GetPlayerKills(dwID);
		p.dwMemberCount = GetMemberCount(dwID);
		p.dwLimit = GetMemberLimitCount();
		p.iList = list++;

		Packet(&p, sizeof(p));
	}
}
/*** End Of Packets Begin ***/

/*** Event Funcs ***/

EVENTINFO(deathmatch_info)
{
	DWORD map_index;

	deathmatch_info()
		: map_index(0)
	{
	}
};

EVENTFUNC(deathmatch_destroy_event)
{
	deathmatch_info* info = dynamic_cast<deathmatch_info*>(event->info);

	if (info == NULL)
	{
		sys_err("deathmatch_destroy_event> <Factor> NULL pointer");
		return 0;
	}

	CDeathMatchPvP::instance().DelEvent();
	CDeathMatchPvP::instance().ResetEvent();
	CDeathMatchPvP::instance().SendHome();
	return 0;
}

void CDeathMatchPvP::DelEvent()
{
	destroyEvent = NULL;
}
/*** End Of Event Funcs ***/

void CDeathMatchPvP::Opened()
{
	SetStatus(CDeathMatchPvP::DEATHMATCH_STATUS_OPENED, m_iDeathMatchMapIndex);

	if (iStatus == DEATHMATCH_STATUS_OPENED)
	{
		std::unique_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery("SELECT king,id FROM player.player"));

		if (pMsg->Get()->uiNumRows != 0)
		{
			MYSQL_ROW row;
			while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int king = 0;
				int GetPlayerID = 0;
				str_to_number(king, row[0]);
				str_to_number(GetPlayerID, row[1]);
				if (king != 0)
				{
					LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(GetPlayerID());
					if (ch)
					{
						ch->RequestPlayerKing(false);
					}
				}
			}
		}
	}
	sys_log(0, "CDeathMatchPvP:: DeathMatch has start.");

	deathmatch_info* info = AllocEventInfo<deathmatch_info>();
	info->map_index = m_iDeathMatchMapIndex;
	destroyEvent = event_create(deathmatch_destroy_event, info, PASSES_PER_SEC(1200)); //20 dakika sonra event biter
}

void CDeathMatchPvP::Closed()
{
	SetStatus(CDeathMatchPvP::DEATHMATCH_STATUS_CLOSED, m_iDeathMatchMapIndex);

	if (iStatus == DEATHMATCH_STATUS_CLOSED)
	{
		//buraya sýralamada 1. olan eklenecek.
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(GetPlayerID());
		if (ch)
		{
			char szNotice[512 + 1];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("DEATHMATCHNOTICE%s"), g->GetName());
			BroadcastNotice(szNotice);
			ch->RequestPlayerKing(true);
		}
	}

	ResetEvent();
}

void CDeathMatchPvP::ResetEvent()
{
	for (PlayerDataMap::iterator itor = m_PlayerDataMap.begin(); itor != m_PlayerDataMap.end(); ++itor)
	{
		delete itor->second;
	}
	m_PlayerDataMap.clear();
	m_iDeathMatchMapIndex = 0;
	m_iStatus = 0;
	m_iMemberLimitCount = 500;
	CDeathMatchPvP::SendHome(); // for lua
	event_cancel(&destroyEvent);
	m_set_pkChr.clear();
}

void CDeathMatchPvP::Destroy()
{
	for (PlayerDataMap::iterator itor = m_PlayerDataMap.begin(); itor != m_PlayerDataMap.end(); ++itor)
	{
		delete itor->second;
	}
	m_PlayerDataMap.clear();
	m_iDeathMatchMapIndex = 0;
	m_iStatus = 0;
	m_iMemberLimitCount = 0;
	destroyEvent = NULL;
	m_set_pkChr.clear();
}

CDeathMatchPvP::CDeathMatchPvP()
{
	ResetEvent();
	m_iMemberLimitCount = 500; // default 500
}

CDeathMatchPvP::~CDeathMatchPvP()
{
	Destroy();
}

