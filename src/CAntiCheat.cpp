/*
	PROJECT		<>	SA:MP Anticheat Plug-in
	LICENSE		<>	See LICENSE in the top level directory.
	AUTHOR(S)	<>	MyU (myudev0@gmail.com)
	PURPOSE		<>  Providing datastructures for the internal SA:MP Server.


	Copyright (C) 2014 SA:MP Anticheat Plug-in.

	The Project is available on https://github.com/myudev/SAMPAC

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.

*/

#include "main.h"
#include <sampgdk/a_players.h>
#include "CAntiCheat.h"
#include "CPlayer.h"
#include <set>
#include <vector>
#include <math.h>
#include <stdarg.h> // va_*
#include <time.h>
#include <boost/unordered_map.hpp>

boost::unordered_map<PLAYERID, ePlayerData*> p_PlayerList;
bool bIsDetectionEnabled[MAX_DETECTIONS];

void CAntiCheat::Init() 
{ // Init stuff here. (prereserving array etc.)
}

void CAntiCheat::Tick()
{ // Invoked by ProcessTick
	logprintf("CAntiCheat::Tick()");
	int i_iIt = 0, i_It2 = 0; // Iterator (Base);
	int integers [ 2 ]; // no need for double allocation
	Vec3 tVec; // Temporary 3D Vector
	for (boost::unordered_map<PLAYERID, ePlayerData*>::iterator p = p_PlayerList.begin(); p != p_PlayerList.end(); ++p)
	{
		p->second->iState = CPlayer::GetState(p->second->iPlayerID) ; // Save State for Later processing.
		// Weapon Cheat
		if ( bIsDetectionEnabled[CHEAT_TYPE_WEAPON] ) {
			for(i_iIt = 0; i_iIt != 13; i_iIt++)
			{
				CPlayer::GetWeaponData(p->second->iPlayerID,i_iIt,&integers[0],&integers[1]);
				if(integers[1] > 0 && integers[0] != 0)
					if ( p->second->bHasWeapon[integers[0]] ) continue; else OnDetect(p->second, CHEAT_TYPE_WEAPON, "%d", integers[0]);
			}
		}

		// Money Cheat ( just the reset )
		if ( bIsDetectionEnabled[CHEAT_TYPE_MONEY] ) {
			if ( CPlayer::GetMoney(p->second->iPlayerID) != p->second->iPlayerMoney ) 
				CPlayer::SetMoney(p->second->iPlayerID, p->second->iPlayerMoney);
		}

		// Player Bugger
		if ( bIsDetectionEnabled[CHEAT_TYPE_PLAYERBUGGER] ) {
			if( p->second->iState == PLAYER_STATE_ONFOOT ) 
			{
				CPlayer::GetPosition( p->second->iPlayerID, &tVec.fX, &tVec.fY, &tVec.fZ );
		
				if( tVec.fX >= 99999.0 || tVec.fY >= 99999.0 || tVec.fZ >= 99999.0 || tVec.fX <= -99999.0 || tVec.fY <= -99999.0 || tVec.fZ <= -99999.0 ) {
					CPlayer::SetPosition( p->second->iPlayerID, p->second->vLastValidPos.fX, p->second->vLastValidPos.fY, p->second->vLastValidPos.fZ );
				}
				else
				{
					p->second->vLastValidPos.fX = tVec.fX;
					p->second->vLastValidPos.fY = tVec.fY;
					p->second->vLastValidPos.fZ = tVec.fZ;
				}
			}
		}

		// Spectate
		if ( bIsDetectionEnabled[CHEAT_TYPE_SPECTATE] ) {
			if ( p->second->iState == PLAYER_STATE_SPECTATING && !p->second->bHasPermissionToSpectate )
				OnDetect(p->second, CHEAT_TYPE_SPECTATE, "\0"); // Cheater !
		}
		
	}
}

char g_DetectBuffer [ 512 ];
void CAntiCheat::OnDetect(ePlayerData *pPlayer, eCheatType eCheatType, const char *fmt, ...) 
{
	int iAmxIndex = 0;

	va_list pVaArgs;
	va_start(pVaArgs, g_DetectBuffer);
	vsnprintf_s(g_DetectBuffer, 512-1, 512, fmt, pVaArgs);
	va_end(pVaArgs);
	
	cell tmp_addr, *tmp_physaddr;

	for (std::set<AMX*>::iterator a = pLAMXList.begin(); a != pLAMXList.end(); ++a)
	{
		if (!amx_FindPublic(*a, "SAMPAC_OnCheatDetect", &iAmxIndex ))
		{
			amx_PushString(*a, &tmp_addr, &tmp_physaddr, g_DetectBuffer, 0, 0);
			amx_Push(*a, static_cast<cell>(eCheatType));
			amx_Push(*a, static_cast<cell>(pPlayer->iPlayerID));
			amx_Exec(*a, NULL, iAmxIndex);
		}
	}  
}

bool CAntiCheat::AddPlayer(PLAYERID playerID)
{
	if ( !SAMP_IS_VALID_PLAYERID(playerID) )
		return false;

	logprintf("CAntiCheat::AddPlayer(%d)", playerID);
	boost::unordered_map<int, ePlayerData*>::iterator p = p_PlayerList.find(playerID);

	ePlayerData p_PlayerData;

	p_PlayerData.bHasPermissionToSpectate = false; // shall we ?
	p_PlayerData.bHasWeapon[46] = true; // ignore parachute for now later a advanced detection with vehicles
	p_PlayerData.iPlayerID = playerID;

	if (p == p_PlayerList.end())
	{
		p_PlayerList.insert(std::make_pair(playerID, &p_PlayerData));
	}
	return true;
}

bool CAntiCheat::RemovePlayer(PLAYERID playerID)
{
	if ( !SAMP_IS_VALID_PLAYERID(playerID) )
		return false;

	boost::unordered_map<int, ePlayerData*>::iterator p = p_PlayerList.find(playerID);


	if (p != p_PlayerList.end())
	{
		delete p->second;
		p_PlayerList.erase(p);
	}
	return true;
}

ePlayerData* CAntiCheat::GetPlayerByID(PLAYERID playerID)
{
	boost::unordered_map<int, ePlayerData*>::iterator p = p_PlayerList.find(playerID);
	if (p != p_PlayerList.end())
	{
		return p->second;
	}
	return NULL;
}

void CAntiCheat::CarWarpCheck(PLAYERID playerID, NEWSTATE stateNEW)
{
	if( bIsDetectionEnabled[ CHEAT_TYPE_CARWARP ] )
	{		
		ePlayerData *player;
		if ( (player=GetPlayerByID(playerID)) == NULL ) return;

		if( CPlayer::GetVehicle( playerID ) != player->iCarWarpVehicleID )
        {
	        if( player->iCarWarpTimeStamp > time(NULL) )
	        {
				OnDetect(player, CHEAT_TYPE_CARWARP, "%d", player->iCarWarpVehicleID);
	            return;
	        }
			player->iCarWarpTimeStamp = ((int)time(NULL)) + 1;
			player->iCarWarpVehicleID = CPlayer::GetVehicle( playerID );
		}
	}
}

void CAntiCheat::RapidPickupSpam(PLAYERID playerID, PICKUPID pickupID)
{
	ePlayerData *player;
	if ( (player=GetPlayerByID(playerID)) == NULL ) return;
	if( bIsDetectionEnabled[ CHEAT_TYPE_PICKUP_SPAM ] )
	{
		if( pickupID != player->iLastPickupID )
		{
			int iTickCount = (int)GetTickCount( ); // Call it once, because swag
	        if( player->iPickupTimestamp > iTickCount )
	        {
				Vec3 curPos;
				CPlayer::GetPosition(player->iPlayerID, &curPos.fX,&curPos.fY,&curPos.fZ);
	        	// He entered very fast, bad boy!!! Let's see how far it is though
	        	float distance = player->vLastPickupPos.DistanceTo( &curPos );

	        	if( distance < 100.0 ) logprintf("[AC WARN] Player ID %d has entered a pickup near him really fast. (distance: %0.2fm, time: %d)", (int)playerID, distance, player->iPickupTimestamp - iTickCount );
	        	else 
	        	{
					OnDetect(player, CHEAT_TYPE_PICKUP_SPAM, "%d:%f:%d", pickupID, distance, player->iPickupTimestamp - iTickCount );
		            return;
	        	}
	        }
	        player->iPickupTimestamp = iTickCount + 1000;
			player->iLastPickupID = pickupID;		
		}		
	}
	CPlayer::GetPosition( player->iPlayerID, &player->vLastPickupPos.fX, &player->vLastPickupPos.fY, &player->vLastPickupPos.fZ );
}

cell AMX_NATIVE_CALL CAntiCheat::HookedGivePlayerWeapon( AMX* amx, cell* params )
{
	PLAYERID playerID = (PLAYERID)params[1];

	ePlayerData *player;
	if ( (player=GetPlayerByID(playerID)) == NULL ) return NULL;

	player->bHasWeapon[params[2]] = true;
	return sampgdk_GivePlayerWeapon(playerID,params[2],params[3]);
}

cell AMX_NATIVE_CALL CAntiCheat::HookedResetPlayerWeapons( AMX* amx, cell* params )
{
	PLAYERID playerID = (PLAYERID)params[1];
	ePlayerData *player;
	if ( (player=GetPlayerByID(playerID)) == NULL ) return NULL;

	sampgdk_ResetPlayerWeapons(playerID);

	for (int it = 0; it != MAX_WEAPS; it++ ) 
		player->bHasWeapon[it] = false;

	return true;
}

cell AMX_NATIVE_CALL CAntiCheat::HookedGivePlayerMoney( AMX* amx, cell* params )
{
	PLAYERID playerID = (PLAYERID)params[1];
	ePlayerData *player;
	if ( (player=GetPlayerByID(playerID)) == NULL ) return NULL;
	int iMoney = (int)params[2];
	player->iPlayerMoney += iMoney;

	return sampgdk_GivePlayerMoney(playerID,iMoney);
}

cell AMX_NATIVE_CALL CAntiCheat::HookedGetPlayerMoney( AMX* amx, cell* params )
{
	PLAYERID playerID = (PLAYERID)params[1];
	ePlayerData *player;
	if ( (player=GetPlayerByID(playerID)) == NULL ) return NULL;
	return player->iPlayerMoney;
}

cell AMX_NATIVE_CALL CAntiCheat::HookedResetPlayerMoney( AMX* amx, cell* params )
{
	PLAYERID playerID = (PLAYERID)params[1];
	CPlayer::SetMoney(playerID, 0);
	return true;
}

cell AMX_NATIVE_CALL CAntiCheat::HookedTogglePlayerSpectating( AMX* amx, cell* params )
{
	PLAYERID playerID = (PLAYERID)params[1];
	bool	 bToggle  = !!params[2];
	ePlayerData *player;
	if ( (player=GetPlayerByID(playerID)) == NULL ) return NULL;
	player->bHasPermissionToSpectate = bToggle;
	return true;
}