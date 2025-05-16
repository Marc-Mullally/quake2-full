
----------
How to Install
----------

1. Download the "finalproject" folder from GitHub (https://github.com/Marc-Mullally/quake2-full/tree/finalproject)
2. Insert the "finalproject" folder into your Quake 2 folder
3. Right-click quake2.exe
4. Click "Create shortcut"
5. Right-click the new "Quake4.exe - Shortcut" and left-click "Properties"
4. Insert "+set game finalproject" at the end of the "Target:" textbox
   (Do not delete what's already in the textbox, just put a space and insert the command after it)
5. Click "Apply"
6. Done! Double clicking or running the shortcut will automatically boot Quake 4 with the mod

----------
How to Play
----------
1. Click H for the help screen
2. After spawning, equip your grenades (G)
3. Throw your grenades at monsters to capture 
4. Press Enter to switch between Catch and Battle Mode

----------
Controls
----------

H		- Toggle help screen
Enter   	- (Out of battle) Switch between Catch and Battle Mode, (In Battle) Select option
←,↑,→,↓ 	- (In Battle) Switch selection in battle
Backspace 	- (In Battle) Go back in menu

----------
How to Test
----------
1. Use the spawn command to spawn any capturable monsters and catch them. Use them in battle to test them
2. Use the expmultiplier command to speed up leveling
3. Use "give grenades" to get more throwable pokeballs

----------
Commands
----------
spawn <classname> 			(Spawns a monsters)
expmultiplier <float> 			(Multiplies exp gain)
partyremove <int>			(Removes monster at the index from the party)
givepokemonitem <itemName> <amount>	(Gives pokemon items)
setlevel <index> <level>		(Set level of a pokemon. Warning: it does not do evolve or move learning checks so do one level before what your aiming for)

----------
List of Capturable Monsters	
----------
(Grouped monsters evolve from top to bottom at level 16 and 36) (Pokemons learn moves at levels 1,3,5,7,9,17,19,21,23,25,37,39,41, and 45)

monster_soldier			FIRE
monster_berserk			FIRE  	| FIGHTING	
monster_gladiator		FIRE  	| FIGHTING

monster_infantry		STEEL
monster_gunner			STEEL 	| ELECTRIC	
monster_tank			STEEL 	| ELECTRIC

monster_parasite 		BUG			
monster_medic 			BUG			
monster_mutant 			BUG 	| DARK		

monster_flipper 		WATER 	| GROUND

monster_flyer 			FLYING	|
monster_hover 			FLYING	| ICE
monster_boss2 			FLYING	| ICE

monster_brain 			PSYCHIC
monster_boss3_stand 		PSYCHIC	
monster_jorg 			PSYCHIC | DRAGON


----------
List of Pokemon Items
----------

POTION  
SUPER  
HYPER  
MAX  
FULL  
ANTIDOTE  
BURN HEAL  
ICE HEAL  
AWAKENING  
PARALYZE HEAL  
FULL HEAL  
X ATTACK  
X DEF  
X SP.ATK  
X SP.DEF  
X SPEED  
X ACC  
REVIVES  
MAX REV.  
POKE BALL  
GREAT BALL  
ULTRA BALL  
MASTER BALL  
POKEDOLL  
F. TAIL  
POKETOY  
