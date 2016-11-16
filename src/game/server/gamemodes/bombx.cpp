/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/* BombX by Sphenocorona */
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <time.h>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <string>

#include <game/server/entities/character.h>
#include <game/server/entities/laser.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "bombx.h"

CGameControllerBOMBX::CGameControllerBOMBX(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "BombX";
	g_Config.m_SvTournamentMode = 0;

	srand(time(0));

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}
void CGameControllerBOMBX::DoWarmup(int Seconds)
{
	IGameController::DoWarmup(Seconds);
	g_Config.m_SvSpectatorSlots = 0;
	for(int j = 0; j < MAX_CLIENTS; j++) {
		if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS &&
				GameServer()->m_apPlayers[j]->m_PreferredTeam != TEAM_SPECTATORS){
			GameServer()->m_apPlayers[j]->SetTeam(0, 0);
		}
	}
}

void CGameControllerBOMBX::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();

	// Allow players to join during the warmup period.
	if(m_Warmup){
		g_Config.m_SvSpectatorSlots = 0;
		GameServer()->SetBID(-1);
	}
	else // However, players cannot join during the actual game.
	{
		EnumerateLivePlayers();
		// prevent new players from joining mid-game
		g_Config.m_SvSpectatorSlots = min(MAX_CLIENTS-m_LivePlayers, MAX_CLIENTS-1);

		// if the bomb exists and has been selected, make their fuse burn
		if (GameServer()->GetBIDs() >= 0){
			if(GameServer()->GetFuse() > 0) {
				GameServer()->Fuse();
				GameServer()->HammerBackTick();
			} else {
				if(GameServer()->m_apPlayers[GameServer()->GetBIDs()] && GameServer()->m_apPlayers[GameServer()->GetBIDs()]->GetTeam() != TEAM_SPECTATORS) {
					GameServer()->m_apPlayers[GameServer()->GetBIDs()]->GetCharacter()->Die(GameServer()->GetBIDs(), WEAPON_GRENADE);
				}
			}
		} else { // otherwise, (so when bomb id is -1) we will randomly choose a bomb.
			if (GameServer()->GetBIDs() == -1) {
				int BombChoice = (rand() % m_LivePlayers);
				if(GameServer()->m_apPlayers[m_LiveIDs[BombChoice]] && GameServer()->m_apPlayers[m_LiveIDs[BombChoice]]->GetTeam() != TEAM_SPECTATORS) {
					GameServer()->SetBID(m_LiveIDs[BombChoice]);
					GameServer()->SetFuse(g_Config.m_SvBombFuse*Server()->TickSpeed());
				}
			}
		}

		// Make the bomb player look special.
		if (GameServer()->GetBIDs() >= 0){

			// Change the player skin to be bomb skin.
			str_copy(GameServer()->m_apPlayers[GameServer()->GetBIDs()]->m_TeeInfos.m_SkinName, "bomb",
					sizeof(GameServer()->m_apPlayers[GameServer()->GetBIDs()]->m_TeeInfos.m_SkinName));

			GameServer()->m_apPlayers[GameServer()->GetBIDs()]->m_TeeInfos.m_ColorBody = 16776960;
			GameServer()->m_apPlayers[GameServer()->GetBIDs()]->m_TeeInfos.m_UseCustomColor = 1;
		}

		for(int j = 0; j < MAX_CLIENTS; j++) {
			if(GameServer()->m_apPlayers[j] && j != GameServer()->GetBIDs()) {

				// If the player looks like a bomb, fix that...
				if (strncmp("bomb", GameServer()->m_apPlayers[j]->m_TeeInfos.m_SkinName, 4) == 0)
				{
					str_copy(GameServer()->m_apPlayers[j]->m_TeeInfos.m_SkinName,
							GameServer()->m_apPlayers[j]->m_OriginalSkinName, sizeof(GameServer()->m_apPlayers[j]->m_TeeInfos.m_SkinName));
				}
				if (GameServer()->m_apPlayers[j]->m_StunTick > 0) {
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_ColorBody = 8978178;
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_UseCustomColor = 1;
				} else {
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_ColorBody = 16119285;
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_UseCustomColor = 1;
				}
			}
		}
	}
}

void CGameControllerBOMBX::EnumerateLivePlayers()
{
	m_LivePlayers = 0;
	m_ActivePlayers = 0;
	std::fill_n(m_LiveIDs, 16, -1); // fill array with -1's instead of 0's since 0 is a valid player ID
	for(int j = 0; j < MAX_CLIENTS; j++)
	{
		if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() != TEAM_SPECTATORS)
		{
			m_LiveIDs[m_LivePlayers] = j;
			m_LivePlayers++;
			m_ActivePlayers++;
		}
		else if (GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->m_PreferredTeam != TEAM_SPECTATORS)
		{
			m_ActivePlayers++;
		}
	}
}

int CGameControllerBOMBX::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);
	if(!m_Warmup){
		pVictim->GetPlayer()->SetTeamDirect(TEAM_SPECTATORS);
		if (GameServer()->GetBIDs() >= 0){
			if (pVictim->GetPlayer()->GetCID() == GameServer()->GetBIDs() &&
					GameServer()->m_apPlayers[GameServer()->GetBIDs()]) {
				GameServer()->SetBID(-1); // Set the current bomb to undefined, so that the bomb will be reset.
				// Bomb EXPLODE!!!!
				pVictim->MakeDeathGrenade(pKiller);
				pVictim->GetPlayer()->m_Score++; // Neutralize score loss due to blowing up
			}
			else if (Weapon == WEAPON_WORLD || pVictim->GetPlayer()->GetCID() == pKiller->GetCID())
				pVictim->GetPlayer()->m_Score++; // Neutralize score loss due to fall/suicide
		}
	}
	else
	{
		if (pKiller != pVictim->GetPlayer())
			pKiller->m_Score--; // Neutralize score gains during warmups
		if (Weapon == WEAPON_WORLD || pVictim->GetPlayer()->GetCID() == pKiller->GetCID())
					pVictim->GetPlayer()->m_Score++; // Neutralize score loss due to fall/suicide
	}
//	MakeWarningLasers(pKiller);
	return 0;
}

void CGameControllerBOMBX::OnCharacterSpawn(CCharacter *pChr)
{
	// give start equipment
	pChr->IncreaseHealth(10);
	pChr->IncreaseArmor(10);

	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
	pChr->GiveWeapon(WEAPON_RIFLE, -1);
}

void CGameControllerBOMBX::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		EnumerateLivePlayers();
		if (m_LivePlayers <= 1 ||
				(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60)) {
			if (m_LivePlayers > 0 && GameServer()->m_apPlayers[m_LiveIDs[0]] &&
					GameServer()->m_apPlayers[m_LiveIDs[0]] == GameServer()->m_apPlayers[m_LiveIDs[0]] && m_ActivePlayers > 1)
			{
				GameServer()->m_apPlayers[m_LiveIDs[0]]->m_Score++; // Reward last surviving player.
			}
			DoWarmup(g_Config.m_SvWarmup);
		}
	}
}

void CGameControllerBOMBX::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}

void CGameControllerBOMBX::MakeWarningLasers(class CPlayer *pBomb)
{
	for(int i = 0; i < GameServer()->Collision()->GetWidth(); i++)
	{
		for(int j = 0; j < GameServer()->Collision()->GetHeight(); j++)
		{
			if(GameServer()->Collision()->GetCollisionAt(i*32,j*32)/8 >= m_LivePlayers - 1)
			{
				pBomb->GetCharacter()->MakeLaserDot(i,j);
			}
		}
	}
}
