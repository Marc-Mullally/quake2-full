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
// g_combat.c

#include "g_local.h"

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}
	
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;


	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	targ->enemy = attacker;

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
//		targ->svflags |= SVF_DEADMONSTER;	// now treat as a different content type
		if (!(targ->monsterinfo.aiflags & AI_GOOD_GUY))
		{
			level.killed_monsters++;
			if (coop->value && attacker->client)
				attacker->client->resp.score++;
			// medics won't heal monsters that they kill themselves
			if (strcmp(attacker->classname, "monster_medic") == 0)
				targ->owner = attacker;
		}
	}

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
		targ->touch = NULL;
		monster_death_use (targ);
	}

	targ->die (targ, inflictor, attacker, damage, point);
}


/*
================
SpawnDamage
================
*/
void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
//	gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int CheckPowerArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			power_armor_type;
	int			index;
	int			damagePerCell;
	int			pa_te_type;
	int			power;
	int			power_used;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (client)
	{
		power_armor_type = PowerArmorType (ent);
		if (power_armor_type != POWER_ARMOR_NONE)
		{
			index = ITEM_INDEX(FindItem("Cells"));
			power = client->pers.inventory[index];
		}
	}
	else if (ent->svflags & SVF_MONSTER)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = ent->monsterinfo.power_armor_power;
	}
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;
	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t		vec;
		float		dot;
		vec3_t		forward;

		// only works if damage point is in front
		AngleVectors (ent->s.angles, forward, NULL, NULL);
		VectorSubtract (point, ent->s.origin, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;
	if (!save)
		return 0;
	if (save > damage)
		save = damage;

	SpawnDamage (pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + 0.2;

	power_used = save / damagePerCell;

	if (client)
		client->pers.inventory[index] -= power_used;
	else
		ent->monsterinfo.power_armor_power -= power_used;
	return save;
}

static int CheckArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int te_sparks, int dflags)
{
	gclient_t	*client;
	int			save;
	int			index;
	gitem_t		*armor;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	index = ArmorIndex (ent);
	if (!index)
		return 0;

	armor = GetItemByIndex (index);

	if (dflags & DAMAGE_ENERGY)
		save = ceil(((gitem_armor_t *)armor->info)->energy_protection*damage);
	else
		save = ceil(((gitem_armor_t *)armor->info)->normal_protection*damage);
	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SpawnDamage (te_sparks, point, normal, save);

	return save;
}

void M_ReactToDamage (edict_t *targ, edict_t *attacker)
{
	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
		return;

	if (attacker == targ || attacker == targ->enemy)
		return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker->client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

	// we now know that we are not both good guys

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->client)
	{
		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy && targ->enemy->client)
		{
			if (visible(targ, targ->enemy))
			{
				targ->oldenemy = attacker;
				return;
			}
			targ->oldenemy = targ->enemy;
		}
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
		return;
	}

	// it's the same base (walk/swim/fly) type and a different classname and it's not a tank
	// (they spray too much), get mad at them
	if (((targ->flags & (FL_FLY|FL_SWIM)) == (attacker->flags & (FL_FLY|FL_SWIM))) &&
		 (strcmp (targ->classname, attacker->classname) != 0) &&
		 (strcmp(attacker->classname, "monster_tank") != 0) &&
		 (strcmp(attacker->classname, "monster_supertank") != 0) &&
		 (strcmp(attacker->classname, "monster_makron") != 0) &&
		 (strcmp(attacker->classname, "monster_jorg") != 0) )
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// if they *meant* to shoot us, then shoot back
	else if (attacker->enemy == targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker->enemy;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
}

qboolean CheckTeamDamage (edict_t *targ, edict_t *attacker)
{
		//FIXME make the next line real and uncomment this block
		// if ((ability to damage a teammate == OFF) && (targ's team == attacker's team))
	return false;
}

void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			psave;
	int			te_sparks;

	if (!targ->takedamage)
		return;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker) && ((deathmatch->value && ((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS))) || coop->value))
	{
		if (OnSameTeam (targ, attacker))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
				damage = 0;
			else
				mod |= MOD_FRIENDLY_FIRE;
		}
	}
	meansOfDeath = mod;

	// easy mode takes half damage
	if (skill->value == 0 && deathmatch->value == 0 && targ->client)
	{
		damage *= 0.5;
		if (!damage)
			damage = 1;
	}

	client = targ->client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

// bonus damage for suprising a monster
	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) && (!targ->enemy) && (targ->health > 0))
		damage *= 2;

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			if (targ->client  && attacker == targ)
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);

			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = damage;
		SpawnDamage (te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = damage;
	}

	psave = CheckPowerArmor (targ, point, normal, take, dflags);
	take -= psave;

	asave = CheckArmor (targ, point, normal, take, te_sparks, dflags);
	take -= asave;

	//treat cheat/powerup savings the same as armor
	asave += save;

	// team damage avoidance
	if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage (targ, attacker))
		return;

// do the damage
	if (take)
	{
		if ((targ->svflags & SVF_MONSTER) || (client))
			SpawnDamage (TE_BLOOD, point, normal, take);
		else
			SpawnDamage (te_sparks, point, normal, take);


		targ->health = targ->health - take;
			
		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
				targ->flags |= FL_NO_KNOCKBACK;
			Killed (targ, inflictor, attacker, take, point);
			return;
		}
	}

	if (targ->svflags & SVF_MONSTER)
	{
		M_ReactToDamage (targ, attacker);
		if (!(targ->monsterinfo.aiflags & AI_DUCKED) && (take))
		{
			targ->pain (targ, attacker, knockback, take);
			// nightmare mode monsters don't go into pain frames often
			if (skill->value == 3)
				targ->pain_debounce_time = level.time + 5;
		}
	}
	else if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain (targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

void CatchAttempt(edict_t* trainer, edict_t* target) {

	if (trainer->client->pers.partySize + 1 <= 6) {

		trainer->client->pers.party[trainer->client->pers.partySize] = target->pokemonStats;
		trainer->client->pers.partySize++;
			
		gi.centerprintf(trainer, "Caught a %s!\n", target->pokemonStats.nickname);
		
		G_FreeEdict(target);
		// Com_Printf(trainer->client->pers.party[trainer->client->pers.partySize].level);
	} else {
		gi.centerprintf(trainer, "Caught a %s! Sent to PC\n", target->pokemonStats.nickname);
		// ADD TO PC
		G_FreeEdict(target);
	}
}

void StartBattle(edict_t* trainer, edict_t* target) {
	
	// pokemonStruct* selectedPokemon = NULL;
	
	trainer->inBattle = true;
	target->inBattle = true;
	trainer->client->pers.menu = MAIN;
	trainer->client->pers.opponent = target;
	trainer->client->pers.selection = 0;
	trainer->client->pers.selected = -1;
	trainer->client->pers.opponent->moveSelected = -1;

	
	VectorAdd(trainer->s.origin, target->s.origin, trainer->client->pers.pokemonPosition);
	VectorScale(trainer->client->pers.pokemonPosition, 0.5, trainer->client->pers.pokemonPosition);
	trainer->client->pers.pokemonPosition[2] += 20;
	sendOut(trainer, target, -1);

	int clear[] = { 0,0,0,0,0,0,0,0 };
	memcpy(trainer->client->pers.party[trainer->client->pers.pokemonIndex].statStages, clear, sizeof(trainer->client->pers.party[trainer->client->pers.pokemonIndex].statStages));
	memcpy(target->pokemonStats.statStages, clear, sizeof(target->pokemonStats.statStages));

	
	trainer->client->pers.battleState = BATTLE_WAIT_ACTION;
	trainer->client->pers.opponent->alreadyMoved = false;
	trainer->client->pers.pokemon->alreadyMoved = false;

	target->health = target->pokemonStats.health;
	target->max_health = target->pokemonStats.stats[0];

	gi.centerprintf(trainer, "%s vs %s\n", trainer->classname, target->pokemonStats.nickname);

	updateChoices(trainer);
	UpdateBattleUI(trainer, target);
	
}

void EndBattle(edict_t* trainer, edict_t* target) {

	trainer->inBattle = false;
	
	/*
	trainer->client->pers.opponent = NULL;
	trainer->client->pers.pokemon = NULL;
	*/
	trainer->client->pers.selection = -1;
	trainer->client->pers.selected = -1;
	retrievePokemon(trainer);
	

	trainer->client->pers.battleState = BATTLE_WAIT_ACTION;
	trainer->client->pers.menu = MAIN;
	if (target) {
		target->inBattle = false;
		target->health = target->pokemonStats.health;
		target->moveSelected = -1;
		target->alreadyMoved = false;
	}
	trainer->client->pers.pokemon->alreadyMoved = false;

	UpdateBattleUI(trainer, target);
}



void UpdateBattleUI(edict_t* trainer, edict_t* target) {

	
	gi.configstring(CS_CHOICE1, trainer->client->pers.choices[0]);
	gi.configstring(CS_CHOICE2, trainer->client->pers.choices[1]);
	gi.configstring(CS_CHOICE3, trainer->client->pers.choices[2]);
	gi.configstring(CS_CHOICE4, trainer->client->pers.choices[3]);
	gi.configstring(CS_CHOICE5, trainer->client->pers.choices[4]);
	gi.configstring(CS_CHOICE6, trainer->client->pers.choices[5]);
	trainer->client->ps.stats[STAT_BATTLE_BOTTOMBOXES] = trainer->inBattle == true &&
														(trainer->client->pers.menu == BAG ||
														trainer->client->pers.menu == POKEMON ||
														trainer->client->pers.menu == HEALING ||
														trainer->client->pers.menu == STATUS ||
														trainer->client->pers.menu == BATTLEITEMS ||
														trainer->client->pers.menu == REVIVES ||
														trainer->client->pers.menu == POKEBALLS ||
														trainer->client->pers.menu == ESCAPE);
	
	trainer->client->ps.stats[STAT_BATTLE_CHOICE1_SELECTED] = trainer->client->pers.selection == 0;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE2_SELECTED] = trainer->client->pers.selection == 1;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE3_SELECTED] = trainer->client->pers.selection == 2;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE4_SELECTED] = trainer->client->pers.selection == 3;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE5_SELECTED] = trainer->client->pers.selection == 4;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE6_SELECTED] = trainer->client->pers.selection == 5;
	
	gi.configstring(CS_POKEMON_HP, createHPBar(trainer->client->pers.pokemon->health, trainer->client->pers.pokemon->max_health));
	gi.configstring(CS_OPPONENT_HP, createHPBar(target->health, target->max_health));
	
	gi.configstring(CS_POKEMON_NAME, trainer->client->pers.pokemon->pokemonStats.nickname);
	gi.configstring(CS_OPPONENT_NAME, target->pokemonStats.nickname);

	char* pokeLevel;
	snprintf(pokeLevel, 8, "LVL %i", trainer->client->pers.pokemon->pokemonStats.level);
	gi.configstring(CS_POKEMON_LEVEL, pokeLevel);

	snprintf(pokeLevel, 8, "LVL %i", target->pokemonStats.level);
	gi.configstring(CS_OPPONENT_LEVEL, pokeLevel);
	
	/*
	STAT_BATTLE_CHOICE1_SELECTED		7
	STAT_BATTLE_CHOICE2_SELECTED		8
	STAT_BATTLE_CHOICE3_SELECTED		10
	STAT_BATTLE_CHOICE4_SELECTED		11
	STAT_BATTLE_CHOICE5_SELECTED		16
	STAT_BATTLE_CHOICE6_SELECTED		17
	*/

	trainer->client->ps.stats[STAT_BATTLE_CHOICE1] = CS_CHOICE1;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE2] = CS_CHOICE2;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE3] = CS_CHOICE3;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE4] = CS_CHOICE4;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE5] = CS_CHOICE5;
	trainer->client->ps.stats[STAT_BATTLE_CHOICE6] = CS_CHOICE6;

	

	trainer->client->ps.stats[STAT_BATTLE_POKEMON_HP] = CS_POKEMON_HP;
	trainer->client->ps.stats[STAT_BATTLE_OPPONENT_HP] = CS_OPPONENT_HP;
	trainer->client->ps.stats[STAT_BATTLE_POKEMON_NAME] = CS_POKEMON_NAME;
	trainer->client->ps.stats[STAT_BATTLE_OPPONENT_NAME] = CS_OPPONENT_NAME;
	trainer->client->ps.stats[STAT_BATTLE_POKEMON_LEVEL] = CS_POKEMON_LEVEL;
	trainer->client->ps.stats[STAT_BATTLE_OPPONENT_LEVEL] = CS_OPPONENT_LEVEL;

	trainer->client->ps.stats[STAT_INBATTLE] = trainer->inBattle;

}

char* createHPBar(int health, int maxHealth) {
	static char HPBar[26];
	float hpRatio = (float) health / maxHealth;
	int filled = (int) ceilf(hpRatio * (sizeof(HPBar)-1));
	
	for (int i = 0; i < sizeof(HPBar); i++) {
		if (i < filled) {
			if (i % 2 == 0) {
				HPBar[i] = "[";
			} else {
				HPBar[i] = "]";
			}
		} else {
			HPBar[i] = " ";
		}
	}
	HPBar[25] = '\0';
	return HPBar;
}

void sendOut(edict_t* trainer, edict_t* target, int pokemonIndex) {
	
	if (pokemonIndex == -1) {
		for (int i = 0; i < trainer->client->pers.partySize; i++) {
			if (trainer->client->pers.party[i].classname != NULL && trainer->client->pers.party[i].health > 0) {
				pokemonIndex = i;
				break;
			}
		}

		if (pokemonIndex == -1) {
			Com_Printf("All Pokemon are fainted!\n");
			return;
		}

	}
	trainer->client->pers.pokemonIndex = pokemonIndex;
	
	edict_t* pokemonEntity = G_Spawn();
	
	pokemonEntity->classname = trainer->client->pers.party[pokemonIndex].classname;
	VectorCopy(trainer->client->pers.pokemonPosition, pokemonEntity->s.origin);

	// change rotation so that they are facing each other

	ED_CallSpawn(pokemonEntity);
	pokemonEntity->pokemonStats = trainer->client->pers.party[trainer->client->pers.pokemonIndex];

	pokemonEntity->health = pokemonEntity->pokemonStats.health;
	pokemonEntity->max_health = pokemonEntity->pokemonStats.stats[0];

	trainer->client->pers.pokemon = pokemonEntity;
	pokemonEntity->inBattle = true;
	trainer->client->pers.forcedSwitch = false;
	gi.centerprintf("%s sent out %s!", trainer->classname, trainer->client->pers.pokemon->pokemonStats.nickname);

	updateChoices(trainer);
	UpdateBattleUI(trainer, target);
}

void retrievePokemon(edict_t* trainer) {
	trainer->client->pers.party[trainer->client->pers.pokemonIndex] = trainer->client->pers.pokemon->pokemonStats;
	int clear[] = { 0,0,0,0,0,0,0,0 };
	memcpy(trainer->client->pers.party[trainer->client->pers.pokemonIndex].statStages, clear, sizeof(trainer->client->pers.party[trainer->client->pers.pokemonIndex].statStages));

	G_FreeEdict(trainer->client->pers.pokemon);
}

void calculateStat(pokemonStruct* pokemonStats, int i) {
	float natureMultiplier = 1.0f;

	if (i == pokemonStats->nature[0]) {
		natureMultiplier += .1f;
	}

	if (i == pokemonStats->nature[1]) {
		natureMultiplier -= .1f;
	}

	if (i == 0) {
		pokemonStats->stats[i] = floor(0.01 * (2 * pokemonStats->baseStats[i] + pokemonStats->IVStats[i] + floor(0.25 * pokemonStats->EVStats[i])) * pokemonStats->level) + pokemonStats->level + 10;
	} else {
		pokemonStats->stats[i] = (floor(0.01 * (2 * pokemonStats->baseStats[i] + pokemonStats->IVStats[i] + floor(0.25 * pokemonStats->EVStats[i])) * pokemonStats->level) + 5) * natureMultiplier;
	}
	

}

void updateChoices(edict_t* ent) {
	char* displayName;
	switch (ent->client->pers.menu) {

	case MAIN:
		ent->client->pers.choices[0] = "FIGHT";
		ent->client->pers.choices[1] = "BAG";
		ent->client->pers.choices[2] = "POKEMON";
		ent->client->pers.choices[3] = "RUN";
		ent->client->pers.choices[4] = "";
		ent->client->pers.choices[5] = "";

		return;
	case FIGHT:

		// displays pokemon for now but implement when moves are made
		for (int i = 0; i < 4; i++) {
			if (ent->client->pers.pokemon->pokemonStats.moveSet[i].name != '\0') {
				ent->client->pers.choices[i] = ent->client->pers.pokemon->pokemonStats.moveSet[i].name;
			} else {
				ent->client->pers.choices[i] = "";
			}
			
		}

		return;
	case BAG:

		ent->client->pers.choices[0] = "HEALING";
		ent->client->pers.choices[1] = "STATUS";
		ent->client->pers.choices[2] = "BATTLE";
		ent->client->pers.choices[3] = "REVIVES";
		ent->client->pers.choices[4] = "POKEBALLS";
		ent->client->pers.choices[5] = "ESCAPE";

		return;

	case POKEMON:
		
		for (int i = 0; i < ent->client->pers.partySize; i++) {
			displayName = ent->client->pers.party[i].nickname;
			if (ent->client->pers.party[i].health <= 0) {
				strcat(displayName, " {FNT}");
			}
			ent->client->pers.choices[i] = displayName;
		}

		for (int i = 5; i >= ent->client->pers.partySize; i--) {
			ent->client->pers.choices[i] = "";
		}

		return;

	case HEALING:
	case STATUS:
	case BATTLEITEMS:
	case REVIVES:
	case POKEBALLS:
	case ESCAPE:
		for (int i = 0; i < 6; i++) {
			if (ent->client->pers.currentBag[i]) {
				ent->client->pers.choices[i] = ent->client->pers.currentBag[i]->name;
			}
			else {
				ent->client->pers.choices[i] = "";
			}
		}		
		return;
	}
}

float TypeChart[18][18] = {
	// NOR FIR WAT GRA ELE ICE FIG POI GRO FLY PSY BUG ROC GHO DRA DAR STL FAI
	{  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, .5,  0,  1,  1, .5,  1}, // Normal
	{  1, .5, .5,  2,  1,  2,  1,  1,  1,  1,  1,  2, .5,  1, .5,  1,  2,  1}, // Fire
	{  1,  2, .5, .5,  1,  1,  1,  1,  2,  1,  1,  1,  2,  1, .5,  1,  1,  1}, // Water
	{  1, .5,  2, .5,  1,  1,  1, .5,  2, .5,  1, .5,  2,  1, .5,  1, .5,  1}, // Grass
	{  1,  1,  2, .5, .5,  1,  1,  1,  0,  2,  1,  1,  1,  1, .5,  1,  1,  1}, // Electric
	{  1, .5, .5,  2,  1, .5,  1,  1,  2,  2,  1,  1,  1,  1,  2,  1, .5,  1}, // Ice
	{  2,  1,  1,  1,  1,  2,  1, .5,  1, .5, .5, .5,  2,  0,  1,  2,  2, .5}, // Fighting
	{  1,  1,  1,  2,  1,  1,  1, .5, .5,  1,  1,  1, .5, .5,  1,  1,  0,  2}, // Poison
	{  1,  2,  1, .5,  2,  1,  1,  2,  1,  0,  1, .5,  2,  1,  1,  1,  2,  1}, // Ground
	{  1,  1,  1,  2, .5,  1,  2,  1,  1,  1,  1,  2, .5,  1,  1,  1, .5,  1}, // Flying
	{  1,  1,  1,  1,  1,  1,  2,  2,  1,  1, .5,  1,  1,  1,  1,  0, .5,  1}, // Psychic
	{  1, .5,  1,  2,  1,  1, .5, .5,  1, .5,  2,  1,  1, .5,  1,  2, .5, .5}, // Bug
	{  1,  2,  1,  1,  1,  2, .5,  1, .5,  2,  1,  2,  1,  1,  1,  1, .5,  1}, // Rock
	{  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  1,  1,  2,  1, .5,  1,  1}, // Ghost
	{  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  1, .5,  0}, // Dragon
	{  1,  1,  1,  1,  1,  1, .5,  1,  1,  1,  2,  1,  1,  2,  1, .5,  1, .5}, // Dark
	{  1, .5, .5,  1, .5,  2,  1,  1,  1,  1,  1,  1,  2,  1,  1,  1, .5,  2}, // Steel
	{  1, .5,  1,  1,  1,  1,  2, .5,  1,  1,  1,  1,  1,  1,  2,  2, .5,  1} // Fairy
};

char* statStageChanges(pokemonStruct* pokemon, int stageChanges[]) {
	char statusOutput[360];
	statusOutput[0] = '\0';
	char* stat[] = { "health", "attack", "defense", "special attack", "special defense", "speed", "accuracy", "evasion"};
	char addon[360];
	Com_Printf("%s:\n", pokemon->nickname);
	for (int i = 0; i < 8; i++) {
		
		if (stageChanges[i] == 1) {
			snprintf(addon, sizeof(addon), "%s's %s rose!\n", pokemon->nickname, stat[i]);
		} else if (stageChanges[i] == 2) {
			snprintf(addon, sizeof(addon), "%s's %s rose sharply!\n", pokemon->nickname, stat[i]);
		} else if (stageChanges[i] >= 3) {
			snprintf(addon, sizeof(addon), "%s's %s rose drastically!\n", pokemon->nickname, stat[i]);
		} else if (stageChanges[i] == -1) {
			snprintf(addon, sizeof(addon), "%s's %s fell!\n", pokemon->nickname, stat[i]);
		} else if (stageChanges[i] == -2) {
			snprintf(addon, sizeof(addon), "%s's %s harshly fell!\n", pokemon->nickname, stat[i]);
		} else if (stageChanges[i] <= -3) {
			snprintf(addon, sizeof(addon), "%s's %s severely fell!\n", pokemon->nickname, stat[i]);
		} else {
			snprintf(addon, sizeof(addon), "");
		}
		strcat(statusOutput, addon);

		pokemon->statStages[i] += stageChanges[i];

		snprintf(addon, sizeof(addon), "");
		if (pokemon->statStages[i] > 6) {
			snprintf(addon, sizeof(addon), "%s's %s won't go any higher!\n", pokemon->nickname, stat[i]);
			pokemon->statStages[i] = 6;
		} else if (pokemon->statStages[i] < -6) {
			snprintf(addon, sizeof(addon), "%s's %s won't go any lower!\n", pokemon->nickname, stat[i]);
			pokemon->statStages[i] = -6;
		}
		strcat(statusOutput, addon);
		if (i != 0) { Com_Printf("%s: %i\n", stat[i], pokemon->statStages[i]); }
	}
	Com_Printf("\n");
	return statusOutput;
	/*
	<Pokémon>'s <stat> rose!
	<Pokémon>'s <stat> rose sharply!
	<Pokémon>'s <stat> rose drastically!
	<Pokémon>'s <stat> won't go any higher!

	<Pokémon>'s <stat> fell!
	<Pokémon>'s <stat> harshly fell!
	<Pokémon>'s <stat> severely fell!
	<Pokémon>'s <stat> won't go any lower!

	*/

	
}

void doMove(edict_t* trainer, edict_t* pokemon, edict_t* opponent, int moveNumber) {
	/*
	char*			name;
	int				moveType; // 1 = Physical, 3 = Special, -1 = Status
	int				damage;
	int				priority;
	float			accuracy;
	pokemonType		type;
	int				levelRequirement;
	int				selfStateChange[8]; // 1-6 is pokemon stats, 7 and 8 are evasion and accuracy (0 isnt used but there to align)
	int				enemyStateChange[8];
	float			statusEffectChance;
	status_effect	statusEffect;

	*/
	if (pokemon != NULL) {
		pokemonMove move = pokemon->pokemonStats.moveSet[moveNumber];
		char* output[360];
		snprintf(output, sizeof(output), "");
		if (pokemon->pokemonStats.statusEffect == ASLEEP) {
			if (rollNumber() >= 1.0f / 3.0f) {
				snprintf(output, sizeof(output), "%s woke up!\n", pokemon->pokemonStats.nickname);
				pokemon->pokemonStats.statusEffect = NONE;
			}
			else {
				gi.centerprintf("%s is fast asleep.", pokemon->pokemonStats.nickname);
				return;
			}
		}

		if (pokemon->pokemonStats.statusEffect == PARALYZED) {
			if (rollNumber() >= .25f) {
				gi.centerprintf("%s is paralyzed! It can't move!", pokemon->pokemonStats.nickname);
				return;

			}
		}

		if (pokemon->pokemonStats.statusEffect == FROZEN) {
			if (rollNumber() >= .20) {
				snprintf(output, sizeof(output), "%s thawed out!\n", pokemon->pokemonStats.nickname);
				pokemon->pokemonStats.statusEffect = NONE;
			} else {
				gi.centerprintf("%s is frozen solid.", pokemon->pokemonStats.nickname);
				return;
			}
		}

		char* usedMove[360];
		snprintf(usedMove, sizeof(usedMove), "%s used %s!\n", pokemon->pokemonStats.nickname, move.name);
		strcat(output, usedMove);
		float modifiedAccuracy;
		if (move.accuracy != -1) {
			float adjustedStages = 1.0f;
			int accuracyStages = pokemon->pokemonStats.statStages[6] - opponent->pokemonStats.statStages[7];
			if (accuracyStages >= 0) {
				adjustedStages = (3.0f + accuracyStages) / 3.0f;
			}
			else {
				adjustedStages = 3.0f / (3.0f - accuracyStages);
			}
			modifiedAccuracy = move.accuracy * adjustedStages;
		}
		else {
			modifiedAccuracy = 1.0f;
		}

		if (rollNumber() > modifiedAccuracy) {
			char* missed[360];
			snprintf(missed, sizeof(missed), "%s's attack missed!", pokemon->pokemonStats.nickname);
			strcat(output, missed);
			gi.centerprintf(trainer, output);
			return;
		}

		float typeEffectiveness = 1.0f;
		int attackerNum = (int)move.type;
		for (int i = 0; i < 2; i++) {
			if (opponent->pokemonStats.type[i] != MONOTYPE) {
				typeEffectiveness *= TypeChart[attackerNum][(int)opponent->pokemonStats.type[i]];
			}
		}

		if (typeEffectiveness != 0) {

			if (move.moveType != -1) {

				qboolean crit = false;
				float critMultiplier = 1.0f;
				if (rollNumber() <= 1.0f / 24.0f) {
					crit = true;
					critMultiplier *= 1.5;

				}

				float randomFactor = ((rand() % (100 - 85 + 1)) + 85) / 100.0f;
				float STABMultiplier = 1.0f;
				if (move.type == pokemon->pokemonStats.type[0] || move.type == pokemon->pokemonStats.type[1]) {
					STABMultiplier = 1.5f;
				}

				float burnMultiplier = 1.0f;
				if (pokemon->pokemonStats.statusEffect == BURNED && move.moveType == 1) {
					burnMultiplier = 0.5f;
				}

				int pokeLevel = pokemon->pokemonStats.level;

				float attackStatMultiplier = 1.0f;
				float defenseStatMultiplier = 1.0f;

				if (pokemon->pokemonStats.statStages[move.moveType] >= 0) {
					attackStatMultiplier = (2.0f + pokemon->pokemonStats.statStages[move.moveType]) / 2.0f;
				}
				else if (!crit) {
					attackStatMultiplier = 2.0f / (2.0f - pokemon->pokemonStats.statStages[move.moveType]);
				}

				if (opponent->pokemonStats.statStages[move.moveType + 1] >= 0 && !crit) {
					defenseStatMultiplier = (2.0f + opponent->pokemonStats.statStages[move.moveType + 1]) / 2.0f;
				}
				else {
					defenseStatMultiplier = 2.0f / (2.0f - opponent->pokemonStats.statStages[move.moveType + 1]);
				}

				float effectiveAttack = attackStatMultiplier * pokemon->pokemonStats.stats[move.moveType];
				float effectiveDefense = defenseStatMultiplier * opponent->pokemonStats.stats[move.moveType + 1];

				int damage = (((2 * pokeLevel / 5 + 2) * move.power * (effectiveAttack / effectiveDefense)) / 50 + 2) * critMultiplier * randomFactor * STABMultiplier * typeEffectiveness * burnMultiplier;
				opponent->health -= damage;
				opponent->pokemonStats.health -= damage;


				Com_Printf("Health : %i/%i\n", opponent->health, opponent->max_health);
				// Com_Printf("%f\n", typeEffectiveness);
				if (typeEffectiveness > 1.0f) {
					strcat(output, "It's super effective!\n");
				}
				else if (typeEffectiveness < 1.0f) {
					strcat(output, "It's not very effective...\n");
				}

				if (crit) {
					strcat(output, "A critical hit!\n");
				}

			}

			if (rollNumber() <= move.statusEffectChance && opponent->pokemonStats.statusEffect == NONE) {
				opponent->pokemonStats.statusEffect = move.statusEffect;
				char* statusChange[360];
				snprintf(statusChange, sizeof(statusChange), "");
				switch (opponent->pokemonStats.statusEffect) {
				case ASLEEP: snprintf(statusChange, sizeof(statusChange), "%s fell asleep!\n", opponent->pokemonStats.nickname); break;
				case BURNED: snprintf(statusChange, sizeof(statusChange), "%s was burned!\n", opponent->pokemonStats.nickname); break;
				case FROZEN: snprintf(statusChange, sizeof(statusChange), "%s was frozen!\n", opponent->pokemonStats.nickname); break;
				case PARALYZED: snprintf(statusChange, sizeof(statusChange), "%s was paralyzed!\n", opponent->pokemonStats.nickname); break;
				case POISONED: snprintf(statusChange, sizeof(statusChange), "%s was poisoned!\n", opponent->pokemonStats.nickname); break;
				case NONE: break;
				}
				strcat(output, statusChange);
			}

		}
		else {
			char* noEffect[360];
			snprintf(noEffect, sizeof(noEffect), "It doesn't affect %s...\n", opponent->pokemonStats.nickname);
			strcat(output, noEffect);

		}
		if (opponent->pokemonStats.statusEffect == FROZEN && move.moveType == FIRE) {
			char* thawed[360];
			snprintf(thawed, sizeof(thawed), "%s thawed out!\n", opponent->pokemonStats.nickname);
			strcat(output, thawed);
		}
		if (rollNumber() <= move.stateChangeChance) {
			strcat(output, statStageChanges(&pokemon->pokemonStats, move.selfStateChange));
			strcat(output, statStageChanges(&opponent->pokemonStats, move.enemyStateChange));
		}

		char statusMsg[360];
		snprintf(statusMsg, sizeof(statusMsg), "");
		if (pokemon->pokemonStats.statusEffect == BURNED) {
			pokemon->pokemonStats.health -= (1.0f / 16.0f) * pokemon->pokemonStats.stats[0];
			snprintf(statusMsg, sizeof(statusMsg), "%s was hurt by its burn\n", pokemon->pokemonStats.nickname);
		}
		if (pokemon->pokemonStats.statusEffect == POISONED) {
			pokemon->pokemonStats.health -= (1.0f / 8.0f) * pokemon->pokemonStats.stats[0];
			snprintf(statusMsg, sizeof(statusMsg), "%s was hurt by its poison\n", pokemon->pokemonStats.nickname);
		}
		strcat(output, statusMsg);
		pokemon->alreadyMoved = true;
		gi.centerprintf(trainer, output);
	}

}


qboolean RunAttempt(edict_t* trainer) {
	int pokemonSpeed = trainer->client->pers.pokemon->pokemonStats.stats[5];
	int opponentSpeed = trainer->client->pers.opponent->pokemonStats.stats[5];
	trainer->client->pers.runAttempts++;
	float runChance = (floorf((pokemonSpeed * 32) / (opponentSpeed / 4)) + 30 * trainer->client->pers.runAttempts) / 256;
	float rngNumber = rollNumber();
	trainer->client->pers.pokemon->alreadyMoved = true;
	if (rngNumber <= runChance) {
		gi.centerprintf(trainer, "You got away safely!");
		EndBattle(trainer, trainer->client->pers.opponent);
		return true;
	}
	else {
		gi.centerprintf(trainer, "You couldn't get away!");
		return false;
	}
}

void useItem(edict_t* trainer, pokemonStruct* pokemon, pokemonItem* item) {

	/*
	char*			name;
	int				healthAmount;
	pokemonItemType	itemType;
	status_effect	statusClear;
	int				statStages[8];
	float			revivePercent;
	float			catchRate;
	int				amount;

	HEALING_ITEM,
	STATUS_CLEAR_ITEM,
	BATTLE_ITEM,
	REVIVES_ITEM,
	POKEBALLS_ITEM,
	ESCAPE_ITEM
	*/

	char* output[360];
	snprintf(output, sizeof(output), "%s used %s\n", trainer->classname, item->name);

	if (item->amount > 0) {
		
		if (item->healthAmount > 0 && pokemon->health > 0) {
			char healthMsg[360];
			snprintf(healthMsg, sizeof(healthMsg), "%s had its HP restored.\n", pokemon->nickname);
			strcat(output, healthMsg);
			pokemon->health += item->healthAmount;
			if (pokemon->health > pokemon->stats[0]) {
				pokemon->health = pokemon->stats[0];
			}
			trainer->client->pers.pokemon->health = trainer->client->pers.pokemon->pokemonStats.health;
		}
		
		if (pokemon->statusEffect != NONE) {
			if (item->statusClear == ALL || pokemon->statusEffect == item->statusClear) {
				char statusClear[360];
				snprintf(statusClear, sizeof(statusClear), "%s%s\n", pokemon->nickname, statusToString(pokemon->statusEffect));
				strcat(output, statusClear);
				pokemon->statusEffect = NONE;
			}

		}

		if (item->itemType == ESCAPE_ITEM) {
			strcat(output, "You got away safely!\n");
			EndBattle(trainer, trainer->client->pers.opponent);
		}


		if (item->revivePercent > 0.0f && pokemon->health < 0) {
			char reviveMsg[360];
			snprintf(reviveMsg, sizeof(reviveMsg), "%s was revived!\n", pokemon->nickname);
			strcat(output, reviveMsg);
			pokemon->health *= item->revivePercent;
		}

		

		if (item->itemType == POKEBALLS_ITEM && item->catchRate >= 1.0f) {
			char catchMsg[360];
			float roll = rollNumber();

			if (roll <= catchChance(trainer->client->pers.opponent, item)) {
				snprintf(catchMsg, sizeof(catchMsg), "Caught a %s!\n", trainer->client->pers.opponent->pokemonStats.nickname);
				EndBattle(trainer, trainer->client->pers.opponent);
				CatchAttempt(trainer, trainer->client->pers.opponent);
			} else {
				snprintf(catchMsg, sizeof(catchMsg), "%s broke free!\n", trainer->client->pers.opponent->pokemonStats.nickname);
				
			}
			strcat(output, catchMsg);
		}
		
		item->amount--;
	}
	trainer->client->pers.pokemon->alreadyMoved = true;
	gi.centerprintf(trainer, output);
}

float catchChance(edict_t* pokemon, pokemonItem* item) {
	float bonusStatus;
	switch (pokemon->pokemonStats.statusEffect) {
	case ASLEEP: 
	case FROZEN: bonusStatus = 2.0f; break;
	case PARALYZED:
	case POISONED:
	case BURNED: bonusStatus = 1.5f; break;
	default: bonusStatus = 1.0f; break;
	}

	if (!pokemon->max_health || !pokemon->health) { return 0.0; }
	float maxhealth = (float)pokemon->max_health;
	float health = (float)pokemon->health;
	float ballRate = item->catchRate;

	return ((3.0f * maxhealth - 2.0f * health) / (3.0f * maxhealth)) * ballRate * bonusStatus;
}

qboolean isEndOfBattle(edict_t* trainer, edict_t* opponent) {
	char output[360];
	snprintf(output, sizeof(output), "");
	char faintMsg[360];

	if (opponent->health <= 0) {
		snprintf(faintMsg, sizeof(faintMsg), "%s fainted\n", opponent->pokemonStats.nickname);
		pokemonStruct opponentStruct = opponent->pokemonStats;
		G_FreeEdict(opponent);
		strcat(output, faintMsg);
		rewardPokemon(trainer, &trainer->client->pers.pokemon->pokemonStats, opponentStruct, output);
		if (trainer->client->pers.learningMove == NULL) { EndBattle(trainer, opponent); }
		return true;
	}

	trainer->client->pers.pokemon->health = trainer->client->pers.pokemon->pokemonStats.health;
	if (trainer->client->pers.pokemon->health <= 0) {
		snprintf(faintMsg, sizeof(faintMsg), "%s fainted\n", opponent->pokemonStats.nickname);
		if (!isTeamDead(trainer)) {
			trainer->client->pers.forcedSwitch = true;
			trainer->client->pers.menu = POKEMON;
			trainer->client->pers.selected = -1;
			trainer->client->pers.selection = 0;
			trainer->client->pers.battleState = BATTLE_WAIT_ACTION;
			updateChoices(trainer);
			UpdateBattleUI(trainer, trainer->client->pers.opponent);
		}
		else {
			EndBattle(trainer, opponent);
			Killed(trainer, trainer, trainer, 9999, MOD_SUICIDE);
			strcat(faintMsg, "You blacked out...");
			strcat(output, faintMsg);
			gi.centerprintf(trainer, output);
			return true;
		}
		strcat(output, faintMsg);
	}

	gi.centerprintf(trainer, output);
	return false;
}
qboolean isTeamDead(edict_t* trainer) {
	for (int i = 0; i < 6; i++) {
		if (trainer->client->pers.party[i].classname != NULL && trainer->client->pers.party[i].health > 0 && i != trainer->client->pers.pokemonIndex) {
			return false;
		}
	}
	return true;
}

void rewardPokemon(edict_t* trainer, pokemonStruct* pokemon, pokemonStruct opponent, char* output) {
	int expNeededToLvl = (int) ((4.0f / 5.0f) * pow((float)pokemon->level, 3));
	char* rewardMsg[360];

	float b = 200;
	float L = (float) opponent.level;
	float LP = (float) pokemon->level;
	int experienceGained = (int)((b * L / 5.0f) * pow((2.0f * L + 10.0f) / (L + LP + 10.0f), 2.5f) + 1.0f);
	pokemon->experience += 3*experienceGained;
	snprintf(rewardMsg, sizeof(rewardMsg), "%s gained %i experience\n", pokemon->nickname, experienceGained);
	strcat(output, rewardMsg);

	if (pokemon->experience >= expNeededToLvl) {
		pokemon->level++;
		pokemon->experience = 0;
		snprintf(rewardMsg, sizeof(rewardMsg), "%s grew to LV. %i\n", pokemon->nickname, pokemon->level);
		strcat(output, rewardMsg);

		for (int i = 0; i < 6; i++) {
			calculateStat(pokemon, i);
		}

		char* learnMsg[360];
		for (int i = 0; i < sizeof(pokemon->learnableMoves) / sizeof(pokemon->learnableMoves[0]); i++) {
			if (pokemon->learnableMoves[i].levelRequirement == pokemon->level) {
				for (int j = 0; j < 4; j++) {
					if (pokemon->moveSet[j].name == NULL) {
						snprintf(learnMsg, sizeof(learnMsg), "%s learned %s\n", pokemon->nickname, pokemon->learnableMoves[j].name);
						strcat(output, learnMsg);
						pokemon->moveSet[j] = pokemon->learnableMoves[j];
						gi.centerprintf(trainer, output);
						return;
					}
				}

				snprintf(learnMsg, sizeof(learnMsg), "%s wants to learn %s, but it already knows 4 moves\nChoose a move to forget!", pokemon->nickname, pokemon->learnableMoves[i].name);
				strcat(output, learnMsg);
				trainer->client->pers.menu = FIGHT;
				trainer->client->pers.learningMove = &pokemon->learnableMoves[i];
				trainer->client->pers.selected = -1;
				trainer->client->pers.selection = 0;
				trainer->client->pers.battleState = BATTLE_WAIT_ACTION;
				gi.centerprintf(trainer, output);
				updateChoices(trainer);
				UpdateBattleUI(trainer, trainer->client->pers.opponent);
			}
		}
	}

	
	gi.centerprintf(trainer, output);
}

char* statusToString(status_effect statusEffect) {
	char* statusString;
	switch (statusEffect) {
	case ASLEEP:	statusString = " woke up!";					break;
	case BURNED:	statusString = "'s burn was cured!";		break;
	case FROZEN:	statusString = " thawed out!";				break;
	case PARALYZED:	statusString = "'s paralysis was cured!";	break;
	case POISONED:	statusString = "'s poison was cured!";		break;
	default:		statusString = "";							break;
	}

	return statusString;
}

float rollNumber() {
	return (float)rand() / (float)RAND_MAX;
}

