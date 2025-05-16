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
/*
==============================================================================

boss3

==============================================================================
*/

#include "g_local.h"
#include "m_boss32.h"

void Use_Boss3 (edict_t *ent, edict_t *other, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BOSSTPORT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	G_FreeEdict (ent);
}

void Think_Boss3Stand (edict_t *ent)
{
	if (ent->s.frame == FRAME_stand260)
		ent->s.frame = FRAME_stand201;
	else
		ent->s.frame++;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED monster_boss3_stand (1 .5 0) (-32 -32 0) (32 32 90)

Just stands and cycles in one place until targeted, then teleports away.
*/
void SP_monster_boss3_stand (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/boss3/rider/tris.md2";
	self->s.modelindex = gi.modelindex (self->model);
	self->s.frame = FRAME_stand201;

	gi.soundindex ("misc/bigtele.wav");

	VectorSet (self->mins, -32, -32, 0);
	VectorSet (self->maxs, 32, 32, 90);

	self->use = Use_Boss3;
	self->think = Think_Boss3Stand;
	self->nextthink = level.time + FRAMETIME;

	// pokemon stats
	self->pokemonStats.classname = self->classname;
	self->pokemonStats.experience = 0;
	self->pokemonStats.statusEffect = NONE;
	self->pokemonStats.nickname = malloc(strlen(self->classname + 8) + 1);
	strcpy(self->pokemonStats.nickname, self->classname + 8);

	// change

	self->pokemonStats.level = 16 + (rand() % 5);
	self->pokemonStats.type[0] = PSYCHIC;
	self->pokemonStats.type[1] = MONOTYPE;

	// 400 stat total
	self->pokemonStats.baseStats[0] = 75;
	self->pokemonStats.baseStats[1] = 25;
	self->pokemonStats.baseStats[2] = 60;
	self->pokemonStats.baseStats[3] = 105;
	self->pokemonStats.baseStats[4] = 85;
	self->pokemonStats.baseStats[5] = 50;

	pokemonMove learnableMoves[] = { {.name = "Amnesia",	.moveType = -1,   .power = 0, .priority = 0, .accuracy = -1.0f, .type = PSYCHIC,  .levelRequirement = 1, .selfStateChange = {0,0,0,0,2,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 1.0f},
									{.name = "Hypnosis",	.moveType = -1,   .power = 0,  .priority = 0, .accuracy = 0.6f, .type = PSYCHIC,  .levelRequirement = 3, .selfStateChange = {0,0,0,0,0,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0},.statusEffectChance = 1.0f, .statusEffect = ASLEEP,  .stateChangeChance = 0.0f},
									{.name = "Psybeam",		.moveType = 3,   .power = 65,  .priority = 0, .accuracy = 1.0f, .type = PSYCHIC, .levelRequirement = 5, .selfStateChange = {0,0,0,0,0,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 0.0f},
									{.name = "Calm Mind",	.moveType = -1, .power = 40,  .priority = 0, .accuracy = 1.0f, .type = PSYCHIC,   .levelRequirement = 7, .selfStateChange = {0,0,0,1,1,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,.stateChangeChance = 1.0f},
									{.name = "Psychic",		.moveType = 3,   .power = 90,  .priority = 0, .accuracy = 1.0f, .type = PSYCHIC, .levelRequirement = 9, .selfStateChange = {0,0,0,0,0,0,0,0},  .enemyStateChange = {0,0,0,0,-1,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 0.1f},
	
									{.name = "Kinesis",		.moveType = -1,   .power = 0, .priority = 0, .accuracy = 0.8f, .type = PSYCHIC,  .levelRequirement = 17, .selfStateChange = {0,0,0,0,0,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,-1,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 1.0f},
									{.name = "Cosmic Power",.moveType = -1,   .power = 0,  .priority = 0, .accuracy = -1.0f, .type = PSYCHIC,  .levelRequirement = 19, .selfStateChange = {0,0,1,0,1,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0},.statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 1.0f},
									{.name = "Eerie Spell",	.moveType = 3,   .power = 80,  .priority = 0, .accuracy = 1.0f, .type = PSYCHIC, .levelRequirement = 21, .selfStateChange = {0,0,0,0,0,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 0.0f},
									{.name = "Psyshield Bash",.moveType = -1, .power = 70,  .priority = 0, .accuracy = 0.9f, .type = PSYCHIC,   .levelRequirement = 23, .selfStateChange = {0,0,1,0,0,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,.stateChangeChance = 1.0f},
									{.name = "Psycho Boost", .moveType = 3,  .power = 140,  .priority = 0, .accuracy = 0.9f, .type = PSYCHIC, .levelRequirement = 25, .selfStateChange = {0,0,0,-2,0,0,0,0},  .enemyStateChange = {0,0,0,0,0,0,0,0}, .statusEffectChance = 0.0f, .statusEffect = NONE,  .stateChangeChance = 1.0f} };

	int EVYield[] = { 0,0,0,0,2,0 };

	self->pokemonStats.evolveLevel = 36;
	self->pokemonStats.evolveTo = "monster_jorg";

	// change end

	memcpy(self->pokemonStats.learnableMoves, learnableMoves, sizeof(learnableMoves));
	memcpy(self->pokemonStats.EVYield, EVYield, sizeof(EVYield));

	self->pokemonStats.nature[0] = 1 + (rand() % 5);
	self->pokemonStats.nature[1] = 1 + (rand() % 5);

	for (int i = 0; i < 6; i++) {
		self->pokemonStats.EVStats[i] = 0;
		self->pokemonStats.IVStats[i] = (rand() % 32);
		calculateStat(&(self->pokemonStats), i);
	}

	self->pokemonStats.health = self->pokemonStats.stats[0];
	self->max_health = self->pokemonStats.stats[0];
	self->health = self->pokemonStats.stats[0];

	int moveCount = 0;

	for (int i = (sizeof(self->pokemonStats.learnableMoves) / sizeof(self->pokemonStats.learnableMoves[0])) - 1; i >= 0; i--) {
		if (self->pokemonStats.level >= self->pokemonStats.learnableMoves[i].levelRequirement && moveCount < 4 && self->pokemonStats.learnableMoves[i].name != NULL) {
			self->pokemonStats.moveSet[moveCount] = self->pokemonStats.learnableMoves[i];
			moveCount++;
			if (moveCount == 4) break;
		}
	}

	int start = 0, end = moveCount - 1;

	// reverse the order of moves cus looks dumb the other way
	int i = 0;
	int j = moveCount - 1;
	while (i < j) {
		pokemonMove temp = self->pokemonStats.moveSet[i];
		self->pokemonStats.moveSet[i] = self->pokemonStats.moveSet[j];
		self->pokemonStats.moveSet[j] = temp;
		i++;
		j--;

	}

	// stats end

	gi.linkentity (self);


}
