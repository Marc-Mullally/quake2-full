/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		sprintf(st, "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}



void Cmd_Select(edict_t* ent) {
	if (ent->client->pers.battleState != BATTLE_PERFORM_ACTION) {
		if (ent->inBattle) {

			ent->client->pers.selected = ent->client->pers.selection;

		}
		else if (ent->client->pers.partySize != 0) {
			ent->client->pers.catchMode = !ent->client->pers.catchMode;
			if (ent->client->pers.catchMode) {
				Com_Printf("Catch Mode\n");
			}
			else if (ent->client->pers.partySize != 0) {
				Com_Printf("Battle Mode\n");
			}
		}
		else {
			Com_Printf("Catch a Pokemon to switch to Battle Mode\n");
		}
	}
}


void Cmd_SelectChange(edict_t* ent, char* direction) {
	if (ent->inBattle && ent->client->pers.battleState != BATTLE_PERFORM_ACTION) {
		int rows = 2;
		int cols = 2;
		if (ent->client->pers.menu == BAG || ent->client->pers.menu == POKEMON || ent->client->pers.menu == HEALING || ent->client->pers.menu == STATUS || ent->client->pers.menu == BATTLEITEMS || ent->client->pers.menu == REVIVES || ent->client->pers.menu == POKEBALLS || ent->client->pers.menu == ESCAPE) {
			rows = 3;
		}

		int row = ent->client->pers.selection / cols;
		int col = ent->client->pers.selection % cols;

		if (strcmp(direction, "up") == 0) {
			row = (row - 1 + rows) % rows; 
		} else if (strcmp(direction, "down") == 0) {
			row = (row + 1) % rows;
		} else if (strcmp(direction, "left") == 0) {
			col = (col - 1 + cols) % cols;
		} else if (strcmp(direction, "right") == 0) {
			col = (col + 1) % cols;
		}

		ent->client->pers.selection = row * cols + col;
		UpdateBattleUI(ent, ent->client->pers.opponent);
	} 
}



void Cmd_SelectBack(edict_t* ent) {
	
	if (ent->inBattle && ent->client->pers.battleState != BATTLE_PERFORM_ACTION && !ent->client->pers.forcedSwitch) {
		ent->client->pers.selection = 0;
		if (ent->client->pers.menu == HEALING ||
			ent->client->pers.menu == STATUS  ||
			ent->client->pers.menu == BATTLEITEMS ||
			ent->client->pers.menu == REVIVES ||
			ent->client->pers.menu == POKEBALLS ||
			ent->client->pers.menu == ESCAPE) {

			ent->client->pers.menu = BAG;
		} else {

		// add later when we got deeper into bag and items in it
			ent->client->pers.menu = MAIN;

		}
		updateChoices(ent);
		UpdateBattleUI(ent, ent->client->pers.opponent);
	}
}

void Cmd_Spawn_f(edict_t* ent) {
	if (gi.argc() < 2) {
		Com_Printf("No class name provided\n");
		return;
	}
	char* classname = gi.argv(1);

	edict_t* spawned = G_Spawn();

	vec3_t forward, right, up, spawnOrigin;

	AngleVectors(ent->client->v_angle, forward, right, up);
	VectorMA(ent->s.origin, 100, forward, spawnOrigin);
	VectorCopy(spawnOrigin, spawned->s.origin);
	spawned->s.origin[2] += 20;
	spawned->classname = classname;
	ED_CallSpawn(spawned);
}

void Cmd_ExpMultiplier_f(edict_t* ent) {
	if (gi.argc() < 2) {
		Com_Printf("No multiplier provided\n");
		return;
	}
	char* argument = gi.argv(1);
	ent->client->pers.expMultiplier = atof(argument);
	Com_Printf("Exp Multiplier set to %f", atof(argument));
	
}

void Cmd_partyRemove_f(edict_t* ent) {
	if (gi.argc() < 2) {
		Com_Printf("No index provided\n");
		return;
	}

	int index = atoi(gi.argv(1));

	if (index < 0 || index >= ent->client->pers.partySize) {
		Com_Printf("Invalid index %i\n", index);
		return;
	}

	for (int i = index; i < ent->client->pers.partySize - 1; i++) {
		ent->client->pers.party[i] = ent->client->pers.party[i + 1];
	}


	memset(&ent->client->pers.party[ent->client->pers.partySize - 1], 0, sizeof(ent->client->pers.party[0]));

	ent->client->pers.partySize--;

	Com_Printf("Removed Pokemon at index %i.\nParty:\n", index, ent->client->pers.partySize);

	for (int i = 0; i < ent->client->pers.partySize; i++) {
		Com_Printf("%s\n", ent->client->pers.party[i].nickname);
	}

	if (ent->client->pers.partySize < 1) {
		ent->client->pers.catchMode = true;
		Com_Printf("Catch Mode\n");
	}
}

void Cmd_givePokemonItem_f(edict_t* ent) {
	if (gi.argc() < 3) {
		Com_Printf("Not enough arguments\n");
		return;
	}

	char* itemName = gi.argv(1);
	int	  amount = atoi(gi.argv(2));

	for (int i = 0; i < sizeof(ent->client->pers.pokeBag)/sizeof(ent->client->pers.pokeBag[0]); i++) {
		if (ent->client->pers.pokeBag[i].name && strcmp(ent->client->pers.pokeBag[i].name, itemName) == 0) {
			ent->client->pers.pokeBag[i].amount += amount;
			Com_Printf("Given %i %s \n Total: %i %ss", amount, itemName, ent->client->pers.pokeBag[i].amount, ent->client->pers.pokeBag[i].name);
			return;
		}
	}

	Com_Printf("Can't find %s", itemName);
}

void Cmd_setLevel_f(edict_t* ent) {
	if (gi.argc() < 3) {
		Com_Printf("Not enough arguments\n");
		return;
	}

	int index = atoi(gi.argv(1));
	int	lvl = atoi(gi.argv(2));
	ent->client->pers.party[index].level = lvl;
	
	for (int i = 0; i < 6;i++) {
		calculateStat(&(ent->client->pers.party[index]), i);
	}
	
	Com_Printf("%s set to Level %i\n", ent->client->pers.party[index].nickname, lvl);
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);

	// GRICKUS!!! cmds
	else if (Q_stricmp(cmd, "select") == 0) 
		Cmd_Select(ent);
	else if (Q_stricmp(cmd, "selectUp") == 0)
		Cmd_SelectChange(ent, "up");
	else if (Q_stricmp(cmd, "selectDown") == 0)
		Cmd_SelectChange(ent, "down");
	else if (Q_stricmp(cmd, "selectLeft") == 0)
		Cmd_SelectChange(ent, "left");
	else if (Q_stricmp(cmd, "selectRight") == 0)
		Cmd_SelectChange(ent, "right");
	else if (Q_stricmp(cmd, "selectBack") == 0)
		Cmd_SelectBack(ent);
	else if (Q_stricmp(cmd, "helpMenu") == 0) {
		ent->client->ps.stats[STAT_HELPMENU] = !ent->client->ps.stats[STAT_HELPMENU];
		if (ent->client->ps.stats[STAT_HELPMENU]) {
			gi.configstring(CS_STATUSBAR, "if 19"
				"	xl 190 yt 110"
				"	picn helpScreenBox "
				"	xl 210 yt 125"
				"	string \" --------How to Play--------\""
				"	xl 210 yt 135"
				"	string \"1. Click H for help screen\""
				"	xl 210 yt 145"
				"	string \"2. After spawning, equip your \""
				"	xl 210 yt 155"
				"	string \"grenades (G)\""
				"	xl 210 yt 165"
				"	string \"3. Throw your grenades at\""
				"	xl 210 yt 175"
				"	string \"monsters to capture and \""
				"	xl 210 yt 185"
				"	string \"battle \""

				"	xl 210 yt 200"
				"	string \"----------Controls----------\""
				"	xl 210 yt 210"
				"	string \"H  -  Toggle help screen\""
				"	xl 210 yt 220"
				"	string \"Enter - (Out of battle) Switch \""
				"	xl 210 yt 230"
				"	string \"between Catch and Battle Mode\""
				"	xl 210 yt 240"
				"	string \"(In Battle) Select option\""
				"	xl 210 yt 250"
				"	string \"Arrow Keys - \""
				"	xl 210 yt 260"
				"	string \"(In Battle) Switch selection\""
				"	xl 210 yt 270"
				"	string \"Backspace - \""
				"	xl 210 yt 280"
				"	string \"(In Battle)  Go back in menu\""
				"endif ");
		}
		else {
			gi.configstring(CS_STATUSBAR, "yb	-24 "

				// health
				"xv	0 "
				"hnum "
				"xv	50 "
				"pic 0 "


				// GRICKUS!!!  UI
				/*
				"if 19"
				"	xl 200 yt 125"
				"	picn helpScreen "
				"endif "
				*/

				"if 18 "

				"	xl 25 yt 25"
				"	picn pokemonbox "
				"	xl 35 yt 35"
				"	stat_string 30 "
				"	yt 55"
				"	stat_string 28 "
				"	yt 35 xl 185"
				"	stat_string 9"

				"	xl 400 yt 235"
				"	picn pokemonbox"
				"	xl 410 yt 245"
				"	stat_string 29 "
				"	yt 265"
				"	stat_string 27 "
				"	yt 245 xl 560"
				"	stat_string 31 "

				"	yb	-100 "
				"	xv	325 "
				"	picn	blackbox "
				"	yb	-92 "
				"	xv	330 "
				"	stat_string 21 "

				"	yb	-100 "
				"	xv	400 "
				"	picn	blackbox "
				"	yb	-92 "
				"	xv	405 "
				"	stat_string 22 "

				"	yb	-75 "
				"	xv	325 "
				"	picn	blackbox "
				"	yb	-67 "
				"	xv	330 "
				"	stat_string 23 "

				"	yb	-75 "
				"	xv	400 "
				"	picn	blackbox "
				"	yb	-67 "
				"	xv	405 "
				"	stat_string 24 "

				"endif "

				"if 7 "
				"	yb	-100 "
				"	xv	325 "
				"	picn	blackboxselection "
				"	yb	-92 "
				"	xv	330 "
				"	stat_string 21 "
				"endif "

				"if 8 "
				"	yb	-100 "
				"	xv	400 "
				"	picn	blackboxselection "
				"	yb	-92 "
				"	xv	405 "
				"	stat_string 22 "
				"endif "

				"if 10 "
				"	yb	-75 "
				"	xv	325 "
				"	picn	blackboxselection "
				"	yb	-67 "
				"	xv	330 "
				"	stat_string 23 "
				"endif "

				"if 11 "
				"	yb	-75 "
				"	xv	400 "
				"	picn	blackboxselection "
				"	yb	-67 "
				"	xv	405 "
				"	stat_string 24 "
				"endif "

				"if 20 "
				"	yb	-50 "
				"	xv	325 "
				"	picn	blackbox "
				"	yb	-42 "
				"	xv	330 "
				"	stat_string 25 "

				"	yb	-50 "
				"	xv	400 "
				"	picn	blackbox "
				"	yb	-42 "
				"	xv	405 "
				"	stat_string 26 "
				"endif "

				"if 16 "
				"	yb	-50 "
				"	xv	325 "
				"	picn	blackboxselection "
				"	yb	-42 "
				"	xv	330 "
				"	stat_string 25 "
				"endif "

				"if 17 "
				"	yb	-50 "
				"	xv	400 "
				"	picn	blackboxselection "
				"	yb	-42 "
				"	xv	405 "
				"	stat_string 26 "
				"endif "

				"	yb -24"

				// end here

				// ammo
				"if 2 "
				"	xv	100 "
				"	anum "
				"	xv	150 "
				"	pic 2 "
				"endif "

				// armor
				/*
				"if 4 "
				"	xv	200 "
				"	rnum "
				"	xv	250 "
				"	pic 4 "
				"endif "

				// selected item
				"if 6 "
				"	xv	296 "
				"	pic 6 "
				"endif "
				*/
				"yb	-50 ");
		}
		
	}
	else if (Q_stricmp(cmd, "spawn") == 0) {
		Cmd_Spawn_f(ent);
	}
	else if (Q_stricmp(cmd, "expmultiplier") == 0) {
		Cmd_ExpMultiplier_f(ent);
	}
	else if (Q_stricmp(cmd, "partyremove") == 0) {
		Cmd_partyRemove_f(ent);
	}
	else if (Q_stricmp(cmd, "givepokemonitem") == 0) {
		Cmd_givePokemonItem_f(ent);
	}
	else if (Q_stricmp(cmd, "setlevel") == 0) {
		Cmd_setLevel_f(ent);
	}
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
