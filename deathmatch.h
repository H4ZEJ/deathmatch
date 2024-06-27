class CDeathMatchPvP : public singleton<CDeathMatchPvP>
{
public:
	typedef struct SPlayerData
	{
		DWORD	dwID;
		int		iMemberCount;
		DWORD		iKills;

		std::set<DWORD> set_pidJoiner;

		void Initialize();

		int GetAccumulatedJoinerCount();
		int GetCurJointerCount();

		void AppendMember(LPCHARACTER ch);
		void RemoveMember(LPCHARACTER ch);
		void AppendKills(int kills);
	} PlayerData;

	enum EDeathMatchStatus
	{
		DEATHMATCH_STATUS_CLOSED = 0,
		DEATHMATCH_STATUS_OPENED = 1,
	};

	typedef std::map<DWORD, PlayerData*> PlayerDataMap;
public:
	CDeathMatchPvP(void);
	virtual ~CDeathMatchPvP(void);

	PlayerData* GetTable(DWORD dwID);

	DWORD GetPlayerKills(DWORD dwID);
	DWORD GetMemberCount(DWORD dwID);

	void OnLogin(LPCHARACTER ch); //deathmatch

	void AppendPlayer(LPCHARACTER ch);
	void OnKills(LPCHARACTER ch, int kills);

	void SendScorePacket();

	void SetStatus(int iStatus, int iMap);
	int GetStatus();
	bool IsDeathMatchMap(int mapindex);
	bool IsDeathMatchActivate();

	void SetMemberLimitCount(int memberlimit);
	int GetMemberLimitCount();
	int GetTotalMemberCount() { return m_set_pkChr.size(); }
	DWORD GetWinnerPlayer();

	void Packet(const void* pv, int size);
	void Notice(const char* psz);
	void SendHome();

	void DelEvent();
	void Opened();
	void Closed();
	void ResetEvent();
	void Destroy();

protected:
	PlayerDataMap	m_PlayerDataMap;
	int m_iDeathMatchMapIndex;
	int m_iMemberLimitCount;
	int m_iActivateStatus;
	LPEVENT destroyEvent;
private:
		CHARACTER_SET m_set_pkChr;
};