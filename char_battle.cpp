
#include "deathmatch.h"
// void CHARACTER::Dead(LPCHARACTER pkKiller, bool bImmediateDead)

// if (pkKiller && pkKiller->IsPC()) 
// if (pkKiller->m_pkChrTarget == this) // Fonksiyonundan sonra altına yazılacak


	if (pkKiller && pkKiller->IsPC() && this->IsPC())
	{
		CDeathMatchPvP::instance().OnKills(pkKiller, +1);
	}

