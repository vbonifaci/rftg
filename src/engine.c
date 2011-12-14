/*
 * Race for the Galaxy AI
 * 
 * Copyright (C) 2009-2011 Keldon Jones
 *
 * Source file modified by B. Nordli, December 2011.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "rftg.h"

/*
 * Expansion level names.
 */
char *exp_names[MAX_EXPANSION + 1] =
{
	"Base game only",
	"The Gathering Storm",
	"Rebel vs Imperium",
	"The Brink of War",
	NULL
};

/*
 * Textual representation for number of players.
 */
char *player_labels[MAX_PLAYER] =
{
	"Two players",
	"Three players",
	"Four players",
	"Five players",
	"Six players",
	NULL
};

/*
 * Textual representation for card locations.
 */
char *location_names[MAX_WHERE] =
{
	"Deck",
	"Discard",
	"Hand",
	"Active",
	"Good",
	"Saved",
	"Revealed",
	"Removed",
};

/*
 * Textual representation for variants.
 */
char *variant_labels[MAX_VARIANT + 1] =
{
	"Normal",
	"Preset start hands",
	"Takeover scenario",
	"Drafting",
	"Separate decks",
	NULL,
};

void dump_hand(game *g, int who)
{
	card *c_ptr;
	int i;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards not owned */
		if (c_ptr->owner != who) continue;

		/* Skip cards in wrong area */
		if (c_ptr->where != WHERE_HAND) continue;

		printf("%s\n", c_ptr->d_ptr->name);
	}
}
void dump_hand_new(game *g, int who)
{
	int x;

	/* Start at head */
	x = g->p[who].head[WHERE_HAND];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Print name */
		printf("%s\n", g->deck[x].d_ptr->name);
	}
}

void dump_active(game *g, int who)
{
	card *c_ptr;
	int i;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards not owned */
		if (c_ptr->owner != who) continue;

		/* Skip cards in wrong area */
		if (c_ptr->where != WHERE_ACTIVE) continue;

		printf("%s\n", c_ptr->d_ptr->name);
	}
}
void dump_active_new(game *g, int who)
{
	int x;

	/* Start at head */
	x = g->p[who].head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Print name */
		printf("%s\n", g->deck[x].d_ptr->name);
	}
}

/*
 * Return a random number using the given argument as a seed.
 *
 * Algorithm from rand() manpage.
 */
int simple_rand(unsigned int *seed)
{
	*seed = *seed * 1103515245 + 12345;
	return ((unsigned)(*seed/65536) % 32768);
}

/*
 * Return whether goals are enabled in this game.
 */
int goals_enabled(game *g)
{
	return g->expanded && !g->goal_disabled;
}

/*
 * Return whether takeovers are enabled in this game.
 */
int takeovers_enabled(game *g)
{
	return g->expanded >= EXPANSION_RVI && !g->takeover_disabled;
}

/*
 * Return whether the prestige mechanic is enabled in this game.
 */
int prestige_enabled(game *g)
{
	return g->expanded == EXPANSION_BOW;
}

/*
 * The path to the executing program.
 */
char program_path[1024];

/*
 * Store the program path based on the command line arguments.
 */
void set_program_path(int argc, char **argv)
{
	/* Make a copy of the program path */
	strcpy(program_path, argv[0]);

	/* Search for forward slash in program path */
	if (strrchr(program_path, '/'))
	{
		/* Chop string */
		*strrchr(program_path, '/') = '\0';
	}

	/* Search for backward slash in program path */
	else if (strrchr(program_path, '\\'))
	{
		/* Chop string */
		*strrchr(program_path, '\\') = '\0';
	}

	/* No slashes found */
	else
	{
		/* Use current folder */
		strcpy(program_path, ".");
	}
}

/*
 * Look for a file in any of the default locations and open it for reading.
 */
FILE *open_file(char *name)
{
	FILE *fff;
	char fn[1024];

	/* Look in installed location */
	sprintf(fn, RFTGDIR "/%s", name);

	/* Open card database */
	fff = fopen(fn, "r");

	/* Return if successful */
	if (fff) return fff;

	/* Try reading from current directory instead */
	fff = fopen(name, "r");

	/* Return if successful */
	if (fff) return fff;

	/* Look in program location */
	sprintf(fn, "%s/%s", program_path, name);

	/* Return the file, or give up */
	return fopen(fn, "r");
}

/*
 * Return whether separate decks are enabled in this game.
 */
int separate_decks(game *g)
{
	return g->variant == VARIANT_DRAFTING || g->variant == VARIANT_SEPARATE;
}

/*
 * Return the minimum expansion, given the variant.
 */
int min_expansion(int variant)
{
	/* Check variant */
	switch (variant)
	{
		/* Takeover variant introduced in Rebel vs Imperium */
		case VARIANT_TAKEOVER:
			return EXPANSION_RVI;

		/* Drafting variant introduced in The Gathering Storm */
		case VARIANT_DRAFTING:
			return EXPANSION_TGS;

		/* No limits in other variants */
		default:
			return EXPANSION_BASE;
	}
}

/*
 * Return the maximum expansion, given the variant.
 */
int max_expansion(int variant)
{
	/* Preset hands not supported above The Gathering Storm */
	return variant == VARIANT_PRESET ? EXPANSION_TGS : MAX_EXPANSION - 1;
}

/*
 * Return the maximum number of players, given the expansion and the variant.
 */
int max_players(int expansion, int variant)
{
	int max;

	/* Check variant */
	switch (variant)
	{
		/* Takeover variant always with two players */
		case VARIANT_TAKEOVER:
			return 2;

		/* Drafting variant */
		case VARIANT_DRAFTING:
			max = expansion * 2 + 1;
			break;

		/* Other variants */
		default:
			max = expansion + 4;
			break;
	}

	/* Cap at maximum six players */
	if (max > 6) max = 6;
	return max;
}


/*
 * Return the number of cards in the draw deck.
 */
int count_draw(game *g, int who)
{
	card *c_ptr;
	int i, n = 0;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Count cards in draw deck */
		if (c_ptr->owner == who && c_ptr->where == WHERE_DECK) n++;
	}

	/* Return count */
	return n;
}

/*
 * Return the number of card's in the given player's hand or active area.
 */
int count_player_area(game *g, int who, int where)
{
	int x, n = 0;

	/* Get first card of area chosen */
	x = g->p[who].head[where];

	/* Loop until end of list */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Count card */
		n++;
	}

	/* Return count */
	return n;
}

/*
 * Return true if the player has a given card design active.
 */
static int player_has(game *g, int who, design *d_ptr)
{
	int x;

	/* Get first active card */
	x = g->p[who].head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Check for matching type */
		if (g->deck[x].d_ptr == d_ptr) return 1;
	}

	/* Assume not */
	return 0;
}

/*
 * Return the number of active cards with the given flags.
 *
 * We check the card's location as of the start of the phase.
 */
int count_active_flags(game *g, int who, int flags)
{
	int x, count = 0;

	/* Start at first active card */
	x = g->p[who].start_head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].start_next)
	{
		/* Check for correct flags */
		if ((g->deck[x].d_ptr->flags & flags) == flags) count++;
	}

	/* Return count */
	return count;
}

/*
 * Check if a player has selected the given action.
 */
int player_chose(game *g, int who, int act)
{
	player *p_ptr;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Checking for prestige action */
	if (act & ACT_PRESTIGE)
	{
		/* Player must have selected action with prestige */
		return (p_ptr->action[0] == act) || (p_ptr->action[1] == act);
	}
	else
	{
		/* Check for either with or without prestige */
		return ((p_ptr->action[0] & ACT_MASK) == act ||
		        (p_ptr->action[1] & ACT_MASK) == act);
	}
}

/*
 * Check whether a player has the prestige tile and prestige on it.
 */
int prestige_on_tile(game *g, int who)
{
	player *p_ptr;
	int i;

	/* Get player pointer */
	p_ptr = &g->p[who];

	if (p_ptr->prestige == 0 || !p_ptr->prestige_turn)
	{
		/* Player has not earned prestige this turn */
		return 0;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Check if another player has more or equal prestige */
		if (i != who && g->p[i].prestige >= p_ptr->prestige)
		{
			/* Not prestige leader */
			return 0;
		}
	}

	/* Player is prestige leader and has prestige on the tile */
	return 1;
}

/*
 * Refresh the draw deck.
 */
void refresh_draw(game *g, int who)
{
	char msg[1024];
	card *c_ptr;
	int i;

	/* Check for common decks */
	if (!separate_decks(g))
	{
		/* Draw deck is global */
		who = -1;
	}

	/* Message */
	if (!g->simulation)
	{
		/* Check for common decks */
		if (!separate_decks(g))
		{
			/* Send message */
			message_add_formatted(g, "Refreshing draw deck.\n", FORMAT_EM);
		}

		/* Variant game with separate decks */
		else if (who != -1)
		{
			/* Format message */
			sprintf(msg, "%s refreshes his draw deck.\n", g->p[who].name);

			/* Send message*/
			message_add_formatted(g, msg, FORMAT_EM);
		}
	}

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards with wrong owner */
		if (c_ptr->owner != who) continue;

		/* Skip cards not in discard pile */
		if (c_ptr->where != WHERE_DISCARD) continue;

		/* Move card to draw deck */
		c_ptr->where = WHERE_DECK;

		/* Card's location is no longer known to anyone */
		c_ptr->known = 0;
	}
}

/*
 * Return a card index from the draw deck.
 * If emptied is set on the, the deck needs to be refreshed.
 */
int random_draw(game *g, int who, int *emptied)
{
	card *c_ptr = NULL;
	int i, n;

	/* Check for common decks */
	if (!separate_decks(g))
	{
		/* Draw deck is global */
		who = -1;
	}

	/* Count draw deck size */
	n = count_draw(g, who);

	/* Check for no cards */
	if (!n)
	{
		/* Refresh draw deck */
		refresh_draw(g, who);

		/* Recount */
		n = count_draw(g, who);

		/* Check for still no cards */
		if (!n)
		{
			/* No card to return */
			return -1;
		}
	}

	/* Choose randomly */
	n = game_rand(g, who) % n;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards with wrong owner */
		if (c_ptr->owner != who) continue;

		/* Skip cards not in draw deck */
		if (c_ptr->where != WHERE_DECK) continue;

		/* Check for chosen card */
		if (!(n--)) break;
	}

	/* Clear chosen card's location */
	move_card(g, i, c_ptr->owner, -1);

	/* Count draw deck size */
	n = count_draw(g, who);

	/* Check for just-emptied draw pile */
	if (emptied) *emptied = !count_draw(g, who);

	/* Return chosen card */
	return i;
}

/*
 * Return the first card pointer from the draw deck.
 *
 * We use this in simulated games for choosing cards to be used as goods
 * and other non-essential tasks.  We don't want to use the random number
 * generator in these cases.
 */
static int first_draw(game *g, int who, int *emptied)
{
	card *c_ptr = NULL;
	int i;

	/* Check for no cards */
	if (!count_draw(g, who))
	{
		/* Refresh draw deck */
		refresh_draw(g, who);

		/* Check for still no cards */
		if (!count_draw(g, who))
		{
			/* No card to return */
			return -1;
		}
	}

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards not in draw deck */
		if (c_ptr->where != WHERE_DECK) continue;

		/* Stop at first valid card */
		break;
	}

	/* Clear chosen card's location */
	move_card(g, i, c_ptr->owner, -1);

	/* Check for just-emptied draw pile */
	if (emptied) *emptied = !count_draw(g, who);

	/* Return chosen card */
	return i;
}

/*
 * Move a card, keeping track of linked lists.
 *
 * This MUST be called when a card is moved to or from a player in
 * these locations: WHERE_HAND, WHERE_ACTIVE, WHERE_GOOD, WHERE_SAVED
 */
void move_card(game *g, int which, int owner, int where)
{
	player *p_ptr;
	card *c_ptr;
	int x;

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Check for current owner and tracked lists */
	if (c_ptr->owner != -1 &&
	    (c_ptr->where == WHERE_HAND || c_ptr->where == WHERE_ACTIVE ||
	     c_ptr->where == WHERE_GOOD || c_ptr->where == WHERE_SAVED))
	{
		/* Get pointer of current owner */
		p_ptr = &g->p[c_ptr->owner];

		/* Find card in list */
		x = p_ptr->head[c_ptr->where];

		/* Check for beginning of list */
		if (x == which)
		{
			/* Adjust list forward */
			p_ptr->head[c_ptr->where] = c_ptr->next;
			c_ptr->next = -1;
		}
		else
		{
			/* Loop until moved card is found */
			while (g->deck[x].next != which)
			{
				/* Move forward */
				x = g->deck[x].next;
			}

			/* Remove moved card from list */
			g->deck[x].next = c_ptr->next;
			c_ptr->next = -1;
		}
	}

	/* Check for new owner and tracked lists */
	if (owner != -1 &&
	    (where == WHERE_HAND || where == WHERE_ACTIVE ||
	     where == WHERE_GOOD || where == WHERE_SAVED))
	{
		/* Get player pointer of new owner */
		p_ptr = &g->p[owner];

		/* Add card to beginning of list */
		c_ptr->next = p_ptr->head[where];
		p_ptr->head[where] = which;
	}

	/* Adjust location */
	c_ptr->owner = owner;
	c_ptr->where = where;
}

/*
 * Move a card's start of phase location, keeping track of linked lists.
 *
 * This should only be called when doing funky manipulation of fake game
 * states.
 */
void move_start(game *g, int which, int owner, int where)
{
	player *p_ptr;
	card *c_ptr;
	int x;

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Check for current owner and tracked lists */
	if (c_ptr->start_owner != -1 &&
	    (c_ptr->start_where == WHERE_HAND || c_ptr->start_where == WHERE_ACTIVE ||
	     c_ptr->start_where == WHERE_GOOD || c_ptr->start_where == WHERE_SAVED))
	{
		/* Get pointer of current owner */
		p_ptr = &g->p[c_ptr->start_owner];

		/* Find card in list */
		x = p_ptr->start_head[c_ptr->start_where];

		/* Check for beginning of list */
		if (x == which)
		{
			/* Adjust list forward */
			p_ptr->start_head[c_ptr->start_where] =
			                                      c_ptr->start_next;
			c_ptr->start_next = -1;
		}
		else
		{
			/* Loop until moved card is found */
			while (g->deck[x].start_next != which)
			{
				/* Move forward */
				x = g->deck[x].start_next;
			}

			/* Remove moved card from list */
			g->deck[x].start_next = c_ptr->start_next;
			c_ptr->start_next = -1;
		}
	}

	/* Check for new owner and tracked lists */
	if (owner != -1 &&
	    (where == WHERE_HAND || where == WHERE_ACTIVE ||
	     where == WHERE_GOOD || where == WHERE_SAVED))
	{
		/* Get player pointer of new owner */
		p_ptr = &g->p[owner];

		/* Add card to beginning of list */
		c_ptr->start_next = p_ptr->start_head[where];
		p_ptr->start_head[where] = which;
	}

	/* Adjust location */
	c_ptr->start_owner = owner;
	c_ptr->start_where = where;
}

/*
 * Draw a card from the deck.
 */
void draw_card(game *g, int who, char *reason)
{
	player *p_ptr;
	card *c_ptr;
	char msg[1024];
	int which, emptied;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Check for simulated game */
	if (g->simulation)
	{
		/* Count fake cards */
		p_ptr->fake_hand++;

		/* Track total number of fake cards seen */
		p_ptr->total_fake++;

		/* Get card from draw pile */
		which = random_draw(g, who, &emptied);

		/* Check for failure */
		if (which == -1) return;

		/* Refresh the deck if it became empty */
		if (emptied) refresh_draw(g, who);

		/* Get card pointer */
		c_ptr = &g->deck[which];

		/* Move card to discard to simulate deck cycling */
		c_ptr->where = WHERE_DISCARD;

		/* Done */
		return;
	}

	/* Choose random card */
	which = random_draw(g, who, &emptied);

	/* Check for failure */
	if (which == -1) return;

	/* Move card to player's hand */
	move_card(g, which, who, WHERE_HAND);

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Card's location is known to player */
	c_ptr->known |= 1 << who;

	/* Check for real game and reason */
	if (!g->simulation)
	{
		if (reason)
		{
			/* Format message */
			sprintf(msg, "%s receives 1 card from %s.\n",
			        p_ptr->name, reason);

			/* Add message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);
		}

		/* Check for private message */
		if (g->p[who].control->private_message)
		{
			/* Format draw message */
			sprintf(msg, "%s draws %s.\n", p_ptr->name, c_ptr->d_ptr->name);

			/* Add message */
			g->p[who].control->private_message(g, who, msg, FORMAT_DRAW);
		}
	}

	/* Refresh the deck if it became empty */
	if (emptied) refresh_draw(g, who);
}

/*
 * Draw a number of cards, as in draw_card() above.
 */
void draw_cards(game *g, int who, int num, char *reason)
{
	int i;
	char msg[1024];

	/* Check for real game and reason */
	if (!g->simulation && reason)
	{
		/* Format message */
		sprintf(msg, "%s receives %d card%s from %s.\n",
		        g->p[who].name, num, PLURAL(num), reason);

		/* Add message */
		message_add_formatted(g, msg, FORMAT_VERBOSE);
	}

	/* Draw required number */
	for (i = 0; i < num; i++) draw_card(g, who, NULL);
}

/*
 * Give player VPs from the pool.
 */
static void gain_vps(game *g, int who, int num, char *reason)
{
	char msg[1024];
	player *p_ptr = &g->p[who];

	/* Award VPs */
	p_ptr->vp += num;

	/* Remove from pool */
	g->vp_pool -= num;

	/* Check for simulated game and reason */
	if (!g->simulation && reason)
	{
		sprintf(msg, "%s receives %d VP%s from %s.\n",
			    g->p[who].name, num, PLURAL(num), reason);

		/* Add message */
		message_add_formatted(g, msg, FORMAT_VERBOSE);
	}
}

/*
 * Discard a card, keeping owner if variant game.
 */
void discard_card(game *g, int who, int which)
{
	/* Check for common decks */
	if (!separate_decks(g))
	{
		/* Move card to discard and clear owner */
		move_card(g, which, -1, WHERE_DISCARD);
	}
	else
	{
		/* Move card to discard and keep owner */
		move_card(g, which, who, WHERE_DISCARD);
	}
}

/*
 * Give a player some prestige.
 */
static void gain_prestige(game *g, int who, int num, char *reason)
{
	player *p_ptr;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Add to prestige */
	p_ptr->prestige += num;

	/* Check for real game and reason */
	if (!g->simulation && reason)
	{
		/* Format message */
		sprintf(msg, "%s receives %d prestige from %s.\n",
		        p_ptr->name, num, reason);

		/* Add message */
		message_add_formatted(g, msg, FORMAT_VERBOSE);
	}

	/* Mark prestige earned this turn */
	p_ptr->prestige_turn = 1;
}

/*
 * Spend some of a player's prestige.
 */
static void spend_prestige(game *g, int who, int num)
{
	player *p_ptr;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Decrease prestige */
	p_ptr->prestige -= num;

	/* Check goal losses */
	check_goal_loss(g, who, GOAL_MOST_PRESTIGE);
}

/*
 * Check prestige lead at end of each phase.
 */
static void check_prestige(game *g)
{
	player *p_ptr;
	int i, max = 0, num = 0;

	/* Do nothing if prestige is disabled */
	if (!prestige_enabled(g)) return;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Track most prestige */
		if (p_ptr->prestige > max) max = p_ptr->prestige;
	}

	/* Do nothing if no one has any prestige */
	if (max == 0) return;

	/* Count number of players tied for most */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for tied for most */
		if (p_ptr->prestige == max) num++;
	}

	/* Check for single player with most */
	for (i = 0; i < g->num_players; i++)
	{
		/* Check for less than most */
		if (p_ptr->prestige < max) p_ptr->prestige_turn = 0;

		/* Check for tie for most */
		if (num > 1) p_ptr->prestige_turn = 0;
	}
}

/*
 * Handle start of round prestige bonuses.
 */
void start_prestige(game *g)
{
	player *p_ptr;
	char msg[1024];
	int i, max = 0, num = 0, card_bonus = -1;

	/* Do nothing if prestige is disabled */
	if (!prestige_enabled(g)) return;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Track most prestige */
		if (p_ptr->prestige > max) max = p_ptr->prestige;
	}

	/* Do nothing if no one has any prestige */
	if (max == 0) return;

	/* Count number of players tied for most */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for tied for most */
		if (p_ptr->prestige == max) num++;
	}

	/* Reward VP to those with most prestige */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for most (or tied) */
		if (p_ptr->prestige == max)
		{
			/* Award VP */
			p_ptr->vp++;

			/* Remove from pool */
			g->vp_pool--;

			/* Start message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s earns VP", p_ptr->name);
			}

			/* Check for sole most, and earned this turn */
			if (num == 1 && p_ptr->prestige_turn)
			{
				/* Remember card bonus */
				card_bonus = i;

				/* Message */
				if (!g->simulation)
				{
					/* Add to message */
					strcat(msg, " and card");
				}
			}

			/* Finish message */
			if (!g->simulation)
			{
				/* Complete message */
				strcat(msg, " for Prestige Leader.\n");

				/* Send message */
				message_add_formatted(g, msg, FORMAT_PRESTIGE);
			}
		}

		/* Check if a player got a card */
		if (card_bonus == i)
		{
			/* Draw a card as well */
			draw_card(g, card_bonus, NULL);
		}

		/* Clear prestige earned this turn mark */
		p_ptr->prestige_turn = 0;
	}

	/* Clear temp flags on card just drawn */
	clear_temp(g);

	/* Check intermediate goals */
	check_goals(g);
}

/*
 * Clear temp flags on all cards and player structures.
 */
void clear_temp(game *g)
{
	player *p_ptr;
	card *c_ptr;
	int i, j;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Copy current location */
		c_ptr->start_owner = c_ptr->owner;
		c_ptr->start_where = c_ptr->where;

		/* Copy next card */
		c_ptr->start_next = c_ptr->next;

		/* Clear unpaid flag */
		c_ptr->unpaid = 0;

		/* Clear produced flag */
		c_ptr->produced = 0;

		/* Clear used flags */
		c_ptr->used = 0;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Clear bonus used flag */
		p_ptr->phase_bonus_used = 0;

		/* Clear bonus military */
		p_ptr->bonus_military = 0;

		/* Clear bonus settle cost reduction */
		p_ptr->bonus_reduce = 0;

		/* Loop over location heads */
		for (j = 0; j < MAX_WHERE; j++)
		{
			/* Copy start of list */
			p_ptr->start_head[j] = p_ptr->head[j];
		}
	}
}

/*
 * Find the next choice in a log after the current position.
 */
int next_choice(int* log, int pos)
{
	/* Step over the type and the return value */
	pos += 2;

	/* Step over the list size and the list itself */
	pos += *(log + pos) + 1;

	/* Step over the special size and the special list itself */
	pos += *(log + pos) + 1;

	/* Return the start of the next choice */
	return pos;
}

/*
 * Look for debug choices in the log and execute the choices.
 */
static void perform_debug_moves(game *g, int who)
{
	player *p_ptr;
	int *l_ptr;
	char msg[1024];
	int c, owner, where;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get current position in log */
	l_ptr = &p_ptr->choice_log[p_ptr->choice_pos];

	/* Loop for debug choices */
	while (p_ptr->choice_pos < p_ptr->choice_size)
	{
		/* Read next choice type */
		switch (*l_ptr)
		{
			/* Move a card in debug mode */
			case CHOICE_D_MOVE:

				/* Advance pointer */
				l_ptr++;

				/* Get card index value */
				c = *l_ptr++;

				/* Advance pointer to start of list */
				l_ptr++;

				/* Get new owner */
				owner = *l_ptr++;

				/* Get new location */
				where = *l_ptr++;

				/* Advance pointer to next choice */
				l_ptr++;

				/* Format message */
				sprintf(msg, "%s moved %s to (%s, %s).\n", g->p[who].name,
				        g->deck[c].d_ptr->name,
				        owner == -1 ? "None" : g->p[owner].name,
				        location_names[where]);

				/* Add message */
				message_add_formatted(g, msg, FORMAT_DEBUG);

				/* Move card */
				move_card(g, c, owner, where);

				/* Always place debug cards first in order */
				g->deck[c].order = -1;

				/* Check for new location */
				switch (where)
				{
					case WHERE_DISCARD:
					case WHERE_ACTIVE:

						/* Make card known to everyone */
						g->deck[c].known = ~0;
						break;

					case WHERE_HAND:
					case WHERE_SAVED:

						/* Make card known to player */
						g->deck[c].known = 1 << owner;
						break;

					default:

						/* Card's location unknown to everybody */
						g->deck[c].known = 0;
						break;
				}

				break;

			/* Shuffle the deck */
			case CHOICE_D_SHUFFLE:

				/* Ignore all data in choice */
				l_ptr += 4;

				/* Check for needing to shuffle the global deck */
				if (!separate_decks(g) || g->cur_action == ACT_DRAFTING)
				{
					/* Format message */
					sprintf(msg, "%s shuffles the draw deck.\n",
					        g->p[who].name);

					/* Add message */
					message_add_formatted(g, msg, FORMAT_DEBUG);

					/* Move to next random number */
					game_rand(g, -1);
				}

				/* Check for needing to shuffle the private deck */
				if (separate_decks(g))
				{
					/* Format message */
					sprintf(msg, "%s shuffles his draw deck.\n",
					        g->p[who].name);

					/* Add message */
					message_add_formatted(g, msg, FORMAT_DEBUG);

					/* Move to next random number */
					game_rand(g, who);
				}
				break;

			/* Take a card */
			case CHOICE_D_TAKE_CARD:

				/* Ignore all data in choice */
				l_ptr += 4;

				/* Don't do anything if currently drafting */
				if (g->cur_action == ACT_DRAFTING) break;

				/* Format message */
				sprintf(msg, "%s takes a card.\n", g->p[who].name);

				/* Add message */
				message_add_formatted(g, msg, FORMAT_DEBUG);

				/* Shuffle the deck to avoid peeking */
				game_rand(g, who);

				/* Give player a card */
				draw_card(g, who, NULL);
				break;

			/* Take a card */
			case CHOICE_D_TAKE_VP:

				/* Ignore all data in choice */
				l_ptr += 4;

				/* Format message */
				sprintf(msg, "%s takes a VP.\n", g->p[who].name);

				/* Add message */
				message_add_formatted(g, msg, FORMAT_DEBUG);

				/* Give player a VP */
				gain_vps(g, who, 1, NULL);
				break;

			/* Take a card */
			case CHOICE_D_TAKE_PRESTIGE:

				/* Ignore all data in choice */
				l_ptr += 4;

				/* Do nothing if prestige is disabled */
				if (!prestige_enabled(g)) break;

				/* Format message */
				sprintf(msg, "%s takes a prestige.\n", g->p[who].name);

				/* Add message */
				message_add_formatted(g, msg, FORMAT_DEBUG);

				/* Give player a prestige */
				gain_prestige(g, who, 1, NULL);
				break;

			/* Rotate players */
			case CHOICE_D_ROTATE:

				/* Ignore all data in choice */
				l_ptr += 4;

				/* Don't do anything if game has not started */
				if (g->cur_action <= ACT_GAME_START) break;

				/* Format message */
				sprintf(msg, "%s changes the first player.\n", g->p[who].name);

				/* Add message */
				message_add_formatted(g, msg, FORMAT_DEBUG);

				/* Remember to rotate players one step */
				++g->debug_rotate;
				break;

			/* No more debug choices */
			default:
				return;
		}

		/* Set log position to current */
		p_ptr->choice_pos = l_ptr - p_ptr->choice_log;

		/* Update unread position */
		p_ptr->choice_unread_pos = p_ptr->choice_pos;

		/* Set game debugged flag */
		g->debug_game = 1;
	}
}

/*
 * Extract an answer to a pending choice from the player's choice log.
 */
static int extract_choice(game *g, int who, int type, int list[], int *nl,
                          int special[], int *ns)
{
	player *p_ptr;
	int *l_ptr;
	int rv, i, num;
	char message[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Check for no data in log */
	if (p_ptr->choice_pos >= p_ptr->choice_size)
	{
		/* Error */
		display_error("No data available in choice log!\n");
		exit(1);
	}

	/* Look for and execute any debug choices in the log */
	perform_debug_moves(g, who);

	/* Get current position in log */
	l_ptr = &p_ptr->choice_log[p_ptr->choice_pos];

	/* Check for correct type of answer */
	if (*l_ptr != type)
	{
		/* Error */
		sprintf(message, "Expected %d in choice log for player %d, found %d!\n",
		        type, who, *l_ptr);
		display_error(message);
		exit(1);
	}

	/* Advance pointer */
	l_ptr++;

	/* Copy return value */
	rv = *l_ptr++;

	/* Get number of items returned */
	num = *l_ptr++;

	/* Check for number of items available */
	if (nl)
	{
		/* Copy number of items in list */
		*nl = num;

		/* Copy list items */
		for (i = 0; i < num; i++)
		{
			/* Copy item */
			list[i] = *l_ptr++;
		}
	}
	else
	{
		/* Ensure no items were returned */
		if (num)
		{
			/* Error */
			display_error("Log has items but nowhere to copy them!\n");
			exit(1);
		}
	}

	/* Get number of special items returned */
	num = *l_ptr++;

	/* Check for number of special items available */
	if (ns)
	{
		/* Copy number of special items */
		*ns = num;

		/* Copy special items */
		for (i = 0; i < num; i++)
		{
			/* Copy item */
			special[i] = *l_ptr++;
		}
	}
	else
	{
		/* Ensure no items were returned */
		if (num)
		{
			/* Error */
			display_error("Log has specials but nowhere to copy them!\n");
			exit(1);
		}
	}

	/* Set log position to current */
	p_ptr->choice_pos = l_ptr - p_ptr->choice_log;

	/* Update unread position */
	p_ptr->choice_unread_pos = p_ptr->choice_pos;

	/* Return logged answer */
	return rv;
}

/*
 * Ask a player to make a decision.
 *
 * If we are replaying a game session, we may already have the decision
 * saved in the player's choice log.  In that case, pull the decision
 * from the log and return it.
 *
 * In this function we always wait for an answer from the player before
 * returning.
 */
static int ask_player(game *g, int who, int type, int list[], int *nl,
                      int special[], int *ns, int arg1, int arg2, int arg3)
{
	player *p_ptr;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Look for and execute any debug choices in the log */
	perform_debug_moves(g, who);

	/* Check for (still) unconsumed choices in log */
	if (p_ptr->choice_pos < p_ptr->choice_size)
	{
		/* Return logged answer */
		return extract_choice(g, who, type, list, nl, special, ns);
	}

	/* Ask player for answer */
	p_ptr->control->make_choice(g, who, type, list, nl, special, ns,
	                            arg1, arg2, arg3);

	/* Check for aborted game */
	if (g->game_over) return -1;

	/* Check for need to wait for answer */
	if (p_ptr->control->wait_answer)
	{
		/* Wait for answer to become available */
		p_ptr->control->wait_answer(g, who);
	}

	/* Return logged answer */
	return extract_choice(g, who, type, list, nl, special, ns);
}

/*
 * Ask a player to make a decision.
 *
 * In this function we will return immediately after asking.  The answer
 * can later be retrieved using extract_choice() above.
 */
static void send_choice(game *g, int who, int type, int list[], int *nl,
                        int special[], int *ns, int arg1, int arg2, int arg3)
{
	player *p_ptr;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Look for and execute any debug choices in the log */
	perform_debug_moves(g, who);

	/* Check for unconsumed choices in log */
	if (p_ptr->choice_pos < p_ptr->choice_size)
	{
		/* Update unread pos */
		p_ptr->choice_unread_pos = next_choice(p_ptr->choice_log,
		                                       p_ptr->choice_pos);
		return;
	}

	/* Ask player for answer */
	p_ptr->control->make_choice(g, who, type, list, nl, special, ns,
	                            arg1, arg2, arg3);

	/* Set the unread position of the log */
	p_ptr->choice_unread_pos = p_ptr->choice_size;
}

/*
 * Wait for all players to have responses ready to the most recent question.
 */
static void wait_for_all(game *g)
{
	player *p_ptr;
	int i;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for need to wait for answer */
		if (p_ptr->control->wait_answer)
		{
			/* Wait for answer to become available */
			p_ptr->control->wait_answer(g, i);
		}
	}
}

/*
 * Return a list of cards in the given player's given area.
 */
static int get_player_area(game *g, int who, int list[MAX_DECK], int where)
{
	int x, n = 0;

	/* Start at beginning of list */
	x = g->p[who].head[where];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Add card to list */
		list[n++] = x;
	}

	/* Return number found */
	return n;
}

/*
 * Return a list of cards holding the given type of good.
 */
int get_goods(game *g, int who, int goods[], int type)
{
	card *c_ptr;
	int x, n = 0;

	/* Start at first active card */
	x = g->p[who].head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Check for uncovered card */
		if (c_ptr->covered == -1) continue;

		/* Skip cards with wrong good type */
		if (c_ptr->d_ptr->good_type != GOOD_ANY &&
		    c_ptr->d_ptr->good_type != type) continue;

		/* Skip cards that are newly-placed */
		if (c_ptr->unpaid) continue;

		/* Add card to list */
		if (goods) goods[n] = x;

		/* Increase number of goods */
		++n;
	}

	/* Return number found */
	return n;
}

/*
 * Called when player has chosen which cards to discard.
 */
void discard_callback(game *g, int who, int list[], int num)
{
	char msg[1024];
	player *p_ptr;
	int i;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Loop over choices */
	for (i = 0; i < num; i++)
	{
		/* Move card to discard */
		discard_card(g, who, list[i]);

		/* Message */
		if (!g->simulation && g->p[who].control->private_message)
		{
			/* Format message */
			sprintf(msg, "%s discards %s.\n",
			        p_ptr->name,
			        g->deck[list[i]].d_ptr->name);

			/* Send message */
			g->p[who].control->private_message(g, who, msg, FORMAT_DISCARD);
		}
	}
}

/*
 * Ask a player to discard the given number of cards.
 */
void player_discard(game *g, int who, int discard)
{
	player *p_ptr;
	int list[MAX_DECK];
	int i, n = 0;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Loop over fake cards */
	for (i = 0; i < p_ptr->fake_hand - p_ptr->fake_discards; i++)
	{
		/* Add fake card to list */
		list[n++] = -1;
	}

	/* Ask player to choose cards to discard */
	ask_player(g, who, CHOICE_DISCARD, list, &n, NULL, NULL, discard, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Discard chosen cards */
	discard_callback(g, who, list, n);
}

/*
 * Return locations of powers for a given player for the given phase.
 */
int get_powers(game *g, int who, int phase, power_where *w_list)
{
	card *c_ptr;
	power *o_ptr;
	int x, i, n = 0;

	/* Get first active card */
	x = g->p[who].start_head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].start_next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Loop over card's powers */
		for (i = 0; i < c_ptr->d_ptr->num_power; i++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[i];

			/* Skip used powers */
			if (c_ptr->used & (1 << i)) continue;

			/* Skip incorrect phase */
			if (o_ptr->phase != phase) continue;

			/* Check for settle phase and discard power */
			if (o_ptr->phase == PHASE_SETTLE &&
			    (o_ptr->code & P3_DISCARD) &&
			    c_ptr->where == WHERE_DISCARD) continue;

			/* Copy power location */
			w_list[n].c_idx = x;
			w_list[n].o_idx = i;

			/* Copy power pointer */
			w_list[n++].o_ptr = o_ptr;
		}
	}

	/* Return length of list */
	return n;
}

/*
 * Add a good to a played card.
 */
void add_good(game *g, card *c_ptr)
{
	int which, emptied = 0;

	/* Check for simulated game */
	if (g->simulation)
	{
		/* Use first available card */
		which = first_draw(g, c_ptr->owner, &emptied);
	}
	else
	{
		/* Get random card to use as good */
		which = random_draw(g, c_ptr->owner, &emptied);
	}

	/* Check for failure */
	if (which == -1) return;

	/* Refresh the deck if it became empty */
	if (emptied) refresh_draw(g, c_ptr->owner);

	/* Move card to owner */
	move_card(g, which, c_ptr->owner, WHERE_GOOD);

	/* Mark covered card */
	c_ptr->covered = which;
}

/*
 * Names of Search categories.
 */
char *search_name[MAX_SEARCH] =
{
	"development providing +1 or +2 Military",
	"military windfall with 1 or 2 defense",
	"peaceful windfall with 1 or 2 cost",
	"world with Chromosome symbol",
	"world producing or coming with Alien good",
	"card consuming two or more goods",
	"military world with 5 or more defense",
	"6-cost development giving ? VPs",
	"card with takeover power"
};

/*
 * Check if a card matches a Search category.
 */
int search_match(game *g, int which, int category)
{
	card *c_ptr;
	design *d_ptr;
	power *o_ptr;
	int i;

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Get card design */
	d_ptr = c_ptr->d_ptr;

	/* Switch on category */
	switch (category)
	{
		/* Development that provides 1 or 2 military */
		case SEARCH_DEV_MILITARY:

			/* Check for non-development */
			if (d_ptr->type != TYPE_DEVELOPMENT) return 0;

			/* Loop over powers */
			for (i = 0; i < d_ptr->num_power; i++)
			{
				/* Get power pointer */
				o_ptr = &d_ptr->powers[i];

				/* Skip non-Settle powers */
				if (o_ptr->phase != PHASE_SETTLE) continue;

				/* Check for extra military */
				if (o_ptr->code == P3_EXTRA_MILITARY)
				{
					/* Check for correct amount */
					if (o_ptr->value == 1 ||
					    o_ptr->value == 2) return 1;
				}
			}

			/* Assume no match */
			return 0;

		/* 1 or 2 defense military windfall */
		case SEARCH_MILITARY_WINDFALL:

			/* Check for non-world */
			if (d_ptr->type != TYPE_WORLD) return 0;

			/* Check for non-windfall */
			if (!(d_ptr->flags & FLAG_WINDFALL)) return 0;

			/* Check for non-military */
			if (!(d_ptr->flags & FLAG_MILITARY)) return 0;

			/* Check for wrong defense */
			if (d_ptr->cost < 1 || d_ptr->cost > 2) return 0;

			/* Match */
			return 1;

		/* 1 or 2 cost non-military windfall */
		case SEARCH_PEACEFUL_WINDFALL:

			/* Check for non-world */
			if (d_ptr->type != TYPE_WORLD) return 0;

			/* Check for non-windfall */
			if (!(d_ptr->flags & FLAG_WINDFALL)) return 0;

			/* Check for military */
			if (d_ptr->flags & FLAG_MILITARY) return 0;

			/* Check for wrong cost */
			if (d_ptr->cost < 1 || d_ptr->cost > 2) return 0;

			/* Match */
			return 1;

		/* World with chromosome symbol */
		case SEARCH_CHROMO_WORLD:

			/* Check for non-world */
			if (d_ptr->type != TYPE_WORLD) return 0;

			/* Check for chromosome symbol */
			return d_ptr->flags & FLAG_CHROMO;

		/* World that produces Alien good */
		case SEARCH_ALIEN_WORLD:

			/* Check for non-world */
			if (d_ptr->type != TYPE_WORLD) return 0;

			/* Check for alien (or any) good type */
			return d_ptr->good_type == GOOD_ALIEN ||
			       d_ptr->good_type == GOOD_ANY;

		/* Card that can consume multiple goods */
		case SEARCH_CONSUME_TWO:

			/* Loop over powers */
			for (i = 0; i < d_ptr->num_power; i++)
			{
				/* Get power pointer */
				o_ptr = &d_ptr->powers[i];

				/* Skip non-Consume powers */
				if (o_ptr->phase != PHASE_CONSUME) continue;

				/* Check for consuming multiple times */
				if ((o_ptr->times > 1 ||
				     o_ptr->code & P4_CONSUME_TWO) &&
				    (o_ptr->code & (P4_CONSUME_ANY |
				                    P4_CONSUME_NOVELTY |
				                    P4_CONSUME_RARE |
				                    P4_CONSUME_GENE |
				                    P4_CONSUME_ALIEN)))
				{
					/* Match */
					return 1;
				}

				/* Check for other consume multiple powers */
				if (o_ptr->code & (P4_CONSUME_ALL |
				                   P4_CONSUME_3_DIFF |
				                   P4_CONSUME_N_DIFF))
				{
					/* Match */
					return 1;
				}
			}

			/* Assume no match */
			return 0;

		/* Military world with 5 or more defense */
		case SEARCH_MILITARY_5:

			/* Check for non-world */
			if (d_ptr->type != TYPE_WORLD) return 0;

			/* Check for non-military */
			if (!(d_ptr->flags & FLAG_MILITARY)) return 0;

			/* Check for low defense */
			if (d_ptr->cost < 5) return 0;

			/* Match */
			return 1;

		/* Six-cost development (with variable points) */
		case SEARCH_6_DEV:

			/* Check for non-development */
			if (d_ptr->type != TYPE_DEVELOPMENT) return 0;

			/* Check for wrong cost */
			if (d_ptr->cost != 6) return 0;

			/* Ensure variable points */
			if (!d_ptr->num_vp_bonus) return 0;

			/* Match */
			return 1;

		/* Card with takeover power */
		case SEARCH_TAKEOVER:

			/* Loop over powers */
			for (i = 0; i < d_ptr->num_power; i++)
			{
				/* Get power pointer */
				o_ptr = &d_ptr->powers[i];

				/* Skip non-Settle powers */
				if (o_ptr->phase != PHASE_SETTLE) continue;

				/* Check for takeover power */
				if (o_ptr->code & (P3_TAKEOVER_REBEL |
				                   P3_TAKEOVER_IMPERIUM |
				                   P3_TAKEOVER_MILITARY |
				                   P3_TAKEOVER_PRESTIGE |
				                   P3_TAKEOVER_DEFENSE |
				                   P3_PREVENT_TAKEOVER))
				{
					/* Match */
					return 1;
				}
			}

			/* Assume no match */
			return 0;
	}

	/* XXX */
	return 0;
}

/*
 * Handle the Search Phase.
 */
void phase_search(game *g)
{
	player *p_ptr;
	card *c_ptr;
	char msg[1024];
	int category, which, emptied;
	int i, j, second, third, match, keep;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip players who did not choose Search */
		if (!player_chose(g, i, ACT_SEARCH)) continue;

		/* Check for simulated game */
		if (g->simulation)
		{
			/* Just give player a card */
			draw_card(g, i, NULL);
			continue;
		}

		/* Ask player for a search category */
		category = ask_player(g, i, CHOICE_SEARCH_TYPE, NULL, NULL,
		                      NULL, NULL, 0, 0, 0);

		/* Check for aborted game */
		if (g->game_over) return;

		/* Clear second/third chance flag */
		second = third = 0;

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s searches for %s.\n", p_ptr->name,
			        search_name[category]);

			/* Send message */
			message_add(g, msg);
		}

		/* Loop until match is found or search failed */
		while (1)
		{
			/* Draw a card */
			which = random_draw(g, i, &emptied);

			/* Check for failure */
			if (which == -1)
			{
				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "Search fails for %s.\n",
					        p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}

				/* Restore prestige/search action to player */
				p_ptr->prestige_action_used = 0;

				/* Done */
				break;
			}

			/* Get card pointer */
			c_ptr = &g->deck[which];

			/* Move card to limbo */
			c_ptr->where = WHERE_ASIDE;

			/* Location is known to everyone */
			c_ptr->known = ~0;

			/* Check for match */
			match = search_match(g, which, category);

			/* Message */
			if (!g->simulation)
			{
				/* Check for match */
				if (match)
				{
					/* Format message */
					sprintf(msg, "%s reveals %s (match).\n",
							p_ptr->name, c_ptr->d_ptr->name);

					/* Send formatted message */
					message_add_formatted(g, msg, FORMAT_PRESTIGE);
				}
				else
				{
					/* Format message */
					sprintf(msg, "%s reveals %s (no match).\n",
							p_ptr->name, c_ptr->d_ptr->name);

					/* Send message */
					message_add(g, msg);
				}
			}

			/* Refresh the deck if it became empty */
			if (emptied) refresh_draw(g, i);

			/* Keep looking if no match */
			if (!match) continue;

			/* XXX Check for any good type and Alien category */
			if (second && c_ptr->d_ptr->good_type == GOOD_ANY &&
			    category == SEARCH_ALIEN_WORLD)
			{
				/* Clear second chance flag */
				second = 0;

				/* Set third chance */
				third = 1;
			}

			/* Check for choice to keep */
			if (!second)
			{
				/* Ask player to keep/decline */
				keep = ask_player(g, i, CHOICE_SEARCH_KEEP,
				                  NULL, NULL, NULL, NULL,
				                  which, category, 0);

				/* Check for aborted game */
				if (g->game_over) return;

				/* Check for declined */
				if (!keep)
				{
					/* Message */
					if (!g->simulation)
					{
						/* Format message */
						sprintf(msg,
						        "%s declines %s.\n",
							p_ptr->name,
						        c_ptr->d_ptr->name);

						/* Send message */
						message_add(g, msg);
					}

					/* Must keep next card */
					second = 1;

					/* XXX Check for any good type */
					if (!third &&
					  c_ptr->d_ptr->good_type == GOOD_ANY &&
					  category == SEARCH_ALIEN_WORLD)
					{
						/* Clear second chance flag */
						second = 0;
					}

					/* Keep looking */
					continue;
				}
			}

			/* Give card to player */
			move_card(g, which, i, WHERE_HAND);

			/* Card is known to player */
			c_ptr->known = 1 << i;

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s takes %s.\n", p_ptr->name,
				        c_ptr->d_ptr->name);

				/* Add message */
				message_add(g, msg);
			}

			/* Done */
			break;
		}

		/* Loop over cards */
		for (j = 0; j < g->deck_size; j++)
		{
			/* Get card pointer */
			c_ptr = &g->deck[j];

			/* Check for card set aside */
			if (c_ptr->where == WHERE_ASIDE)
			{
				/* Move to discard */
				discard_card(g, i, j);
			}
		}
	}

	/* Clear any temp flags on cards */
	clear_temp(g);
}

/*
 * Handle the Explore Phase.
 */
void phase_explore(game *g)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int i, j, x, n, draw, keep, drawn[MAX_PLAYER], kept[MAX_PLAYER];
	int discard_any = 0, any[MAX_PLAYER];
	int list[MAX_DECK], num;
	char msg[1024];

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Get list of explore powers */
		n = get_powers(g, i, PHASE_EXPLORE, w_list);

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Skip powers that aren't "discard for prestige" */
			if (!(o_ptr->code & P1_DISCARD_PRESTIGE)) continue;

			/* Get list of cards in hand */
			num = get_player_area(g, i, list, WHERE_HAND);

			/* Check for no cards to discard */
			if (!num) continue;

			/* Ask player to discard one */
			ask_player(g, i, CHOICE_DISCARD_PRESTIGE, list, &num,
			           NULL, NULL, 0, 0, 0);

			/* Check for aborted game */
			if (g->game_over) return;

			/* Check for no discard made */
			if (!num) continue;

			/* Discard chosen card */
			discard_callback(g, i, list, num);

			/* Gain prestige */
			gain_prestige(g, i, o_ptr->value, NULL);

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s discards to gain prestige from %s.\n",
				        g->p[i].name, g->deck[w_list[j].c_idx].d_ptr->name);

				/* Send message */
				message_add(g, msg);
			}
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Assume we draw 2 cards */
		draw = 2;

		/* Check for chosen "+5 draw" explore */
		if (player_chose(g, i, ACT_EXPLORE_5_0)) draw += 5;

		/* Check for chosen "+1 draw" explore */
		if (player_chose(g, i, ACT_EXPLORE_1_1)) draw += 1;

		/* Check for prestige explore */
		if (player_chose(g, i, ACT_PRESTIGE | ACT_EXPLORE_5_0) ||
		    player_chose(g, i, ACT_PRESTIGE | ACT_EXPLORE_1_1))
		{
			/* Draw 6 additional */
			draw += 6;
		}

		/* Get list of explore powers */
		n = get_powers(g, i, PHASE_EXPLORE, w_list);

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Skip powers that do not have extra draw */
			if (!(o_ptr->code & P1_DRAW)) continue;

			/* Add value */
			draw += o_ptr->value;
		}

		/* Draw cards */
		draw_cards(g, i, draw, NULL);

		/* Remember cards drawn */
		drawn[i] = draw;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Assume we keep one card */
		keep = 1;

		/* Assume player has no "discard any" power */
		discard_any = 0;

		/* Check for chosen "+1 keep" explore */
		if (player_chose(g, i, ACT_EXPLORE_1_1)) keep += 1;

		/* Check for prestige explore */
		if (player_chose(g, i, ACT_PRESTIGE | ACT_EXPLORE_5_0) ||
		    player_chose(g, i, ACT_PRESTIGE | ACT_EXPLORE_1_1))
		{
			/* Keep one extra */
			keep += 1;

			/* Discard any cards */
			discard_any = 1;
		}

		/* Get list of explore powers */
		n = get_powers(g, i, PHASE_EXPLORE, w_list);

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Skip powers that do not have extra keep */
			if (!(o_ptr->code & P1_KEEP)) continue;

			/* Add value */
			keep += o_ptr->value;
		}

		/* Keep track of cards kept */
		kept[i] = keep;

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Check for "discard any" power */
			if (o_ptr->code & P1_DISCARD_ANY) discard_any = 1;
		}

		/* Keep track of discard any power */
		any[i] = discard_any;

		/* Check for no need to discard any */
		if (kept[i] == drawn[i]) continue;

		/* Clear list of cards */
		num = 0;

		/* Start at beginning of hand */
		x = p_ptr->head[WHERE_HAND];

		/* Loop over cards */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip cards already in hand unless discarding any */
			if (c_ptr->start_where == WHERE_HAND &&
			    c_ptr->start_owner == i && !discard_any)
				continue;

			/* Add card to list */
			list[num++] = x;
		}

		/* Ask player to discard */
		send_choice(g, i, CHOICE_DISCARD, list, &num, NULL, NULL,
		            drawn[i] - kept[i], 0, 0);

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Wait for all players to finish choosing */
	wait_for_all(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for need to discard */
		if (kept[i] != drawn[i])
		{
			/* Get cards to discard */
			extract_choice(g, i, CHOICE_DISCARD, list, &num,
			               NULL, NULL);

			/* Discard chosen cards */
			discard_callback(g, i, list, num);
		}

		/* Message */
		if (!g->simulation)
		{
			/* Check for discarding any */
			if (any[i])
			{
				/* Format message */
				sprintf(msg, "%s draws %d and discards %d.\n",
				        p_ptr->name, drawn[i],
				        drawn[i] - kept[i]);
			}
			else
			{
				/* Format message */
				sprintf(msg, "%s draws %d and keeps %d.\n",
				        p_ptr->name, drawn[i], kept[i]);
			}

			/* Add message */
			message_add(g, msg);
		}

		/* Check for our simulated game */
		if (g->simulation && g->sim_who == i &&
		    p_ptr->control->explore_sample &&
		    (player_chose(g, i, ACT_EXPLORE_5_0) ||
		     player_chose(g, i, ACT_EXPLORE_1_1)))
		{
			/* Place "sample" cards in hand */
			p_ptr->control->explore_sample(g, i, drawn[i], kept[i],
			                               any[i]);
		}
	}

	/* Check intermediate goals */
	check_goals(g);

	/* Check prestige leader */
	check_prestige(g);

	/* Clear leftover temp flags */
	clear_temp(g);
}

/*
 * Place a card on the table for the given player.
 */
void place_card(game *g, int who, int which)
{
	player *p_ptr;
	card *c_ptr;
	char reason[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card being placed */
	c_ptr = &g->deck[which];

	/* Move card to player */
	move_card(g, which, who, WHERE_ACTIVE);

	/* Location is known to all */
	c_ptr->known = ~0;

	/* Count order played */
	c_ptr->order = p_ptr->table_order++;

	/* Add a good to windfall worlds */
	if (c_ptr->d_ptr->flags & FLAG_WINDFALL) add_good(g, c_ptr);

	/* Check for prestige enabled */
	if (prestige_enabled(g))
	{
		/* Check for prestige from card */
		if (c_ptr->d_ptr->flags & FLAG_PRESTIGE)
		{
			/* Format reason */
			sprintf(reason, "placing %s", c_ptr->d_ptr->name);

			/* Add prestige to player */
			gain_prestige(g, who, 1, reason);
		}
	}

	/* Card is as-yet unpaid for */
	c_ptr->unpaid = 1;
}

/*
 * Computes the current cost for a development.
 * Assumes the current phase of the game is develop.
 */
int devel_cost(game *g, int who, int which)
{
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int i, n, cost;

	/* Get pointer to card being played */
	c_ptr = &g->deck[which];

	/* Start with card cost */
	cost = c_ptr->d_ptr->cost;

	/* Check for develop action chosen */
	if (player_chose(g, who, g->cur_action)) cost -= 1;

	/* Check for prestige develop */
	if (player_chose(g, who, ACT_PRESTIGE | g->cur_action)) cost -= 2;

	/* Get list of develop powers */
	n = get_powers(g, who, PHASE_DEVELOP, w_list);

	/* Loop over develop powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for reduce power */
		if (o_ptr->code & P2_REDUCE) cost -= o_ptr->value;
	}

	/* Cost cannot be less than zero */
	if (cost < 0) cost = 0;

	/* Return the computed cost */
	return cost;
}

/*
 * Called when a player has chosen how to pay for a development.
 *
 * We return 0 if the payment would not succeed.
 */
int devel_callback(game *g, int who, int which, int list[], int num,
                   int special[], int num_special)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int i, j, n;
	int g_list[MAX_DECK], num_goods = 0;
	int cost, consume = 0;
	int consume_special[2], num_consume_special;
	char msg[1024];

	/* Get cost */
	cost = devel_cost(g, who, which);

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Loop over special cards used */
	for (i = 0; i < num_special; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[special[i]];

		/* Loop over card's powers */
		for (j = 0; j < c_ptr->d_ptr->num_power; j++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[j];

			/* Skip non-Develop power */
			if (o_ptr->phase != PHASE_DEVELOP) continue;

			/* Check for discard to reduce cost power */
			if (o_ptr->code & P2_DISCARD_REDUCE)
			{
				/* Discard card */
				discard_card(g, who, special[i]);

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards %s.\n",
					        p_ptr->name,
					        c_ptr->d_ptr->name);

					/* Send message */
					message_add(g, msg);
				}

				/* Check goal losses */
				check_goal_loss(g, who, GOAL_MOST_CONSUME);

				/* Reduce cost by value */
				cost -= o_ptr->value;
			}

			/* Check for consume good to reduce cost */
			if (o_ptr->code & P2_CONSUME_RARE)
			{
				/* Ask player to consume a good later */
				consume = 1;

				/* Reduce cost by value */
				cost -= o_ptr->value;

				/* Track location of consume power */
				consume_special[0] = special[i];
				consume_special[1] = j;
				num_consume_special = 2;

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards a Rare good to "
					        "reduce cost.\n", p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}
			}
		}
	}

	/* Get pointer to card being played */
	c_ptr = &g->deck[p_ptr->placing];

	/* Do not reduce below zero */
	if (cost < 0) cost = 0;

	/* Check for incorrect payment */
	if (cost != num) return 0;

	/* Check for good to consume */
	if (consume)
	{
		/* Get good list */
		num_goods = get_goods(g, who, g_list, GOOD_RARE);

		/* Check for multiple choices */
		if (num_goods > 1)
		{
			/* Ask player to consume good */
			ask_player(g, who, CHOICE_GOOD, g_list, &num_goods,
				   consume_special, &num_consume_special,
			           1, 1, 0);

			/* Check for aborted game */
			if (g->game_over) return 0;
		}

		/* Consume chosen good */
		good_chosen(g, who, consume_special[0], consume_special[1],
		            g_list, 1);
	}

	/* Message */
	if (!g->simulation)
	{
		/* Private message */
		if (g->p[who].control->private_message)
		{
			/* Loop over choices */
			for (i = 0; i < num; i++)
			{
				/* Format message */
				sprintf(msg, "%s discards %s.\n", p_ptr->name,
				        g->deck[list[i]].d_ptr->name);

				/* Send message */
				g->p[who].control->private_message(g, who, msg, FORMAT_DISCARD);
			}
		}

		/* Format message */
		sprintf(msg, "%s pays %d for %s.\n", p_ptr->name, num,
		        c_ptr->d_ptr->name);

		/* Send message */
		message_add(g, msg);
	}

	/* Loop over cards chosen as payment */
	for (i = 0; i < num; i++)
	{
		/* Check for fake card passed in as payment */
		if (list[i] < 0)
		{
			/* Add to count */
			p_ptr->fake_discards++;
			continue;
		}

		/* Move to discard */
		discard_card(g, who, list[i]);
	}

	/* Get list of develop powers */
	n = get_powers(g, who, PHASE_DEVELOP, w_list);

	/* Loop over develop powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for "save cost" power */
		if (o_ptr->code & P2_SAVE_COST)
		{
			/* Do not ask in simulated game */
			if (g->simulation) continue;

			/* Check for no cards spent */
			if (num == 0) continue;

			/* Check for more than one card to choose */
			if (num > 1)
			{
				/* Ask player which to save */
				ask_player(g, who, CHOICE_SAVE, list, &num,
				           NULL, NULL, 0, 0, 0);

				/* Check for aborted game */
				if (g->game_over) return 0;
			}

			/* Move saved card */
			move_card(g, list[0], who, WHERE_SAVED);

			/* Format message */
			sprintf(msg, "%s saves 1 card under Galactic Scavengers.\n",
			        p_ptr->name);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);

			/* Private message */
			if (g->p[who].control->private_message)
			{
				/* Format message */
				sprintf(msg, "%s saves %s.\n", p_ptr->name,
				        g->deck[list[0]].d_ptr->name);

				/* Send message */
				g->p[who].control->private_message(g, who, msg,
				                                   FORMAT_DISCARD);
			}
		}
	}

	/* Card is now paid for */
	g->deck[which].unpaid = 0;

	/* Payment is good */
	return 1;
}

/*
 * Ask a player to discard to pay for a development card played.
 */
static void pay_devel(game *g, int who, int cost)
{
	player *p_ptr;
	power_where w_list[100];
	power *o_ptr;
	char msg[1024];
	int list[MAX_DECK], special[MAX_DECK], g_list[MAX_DECK];
	int i, n = 0, num_special = 0, num_goods = 0;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get develop phase powers */
	n = get_powers(g, who, PHASE_DEVELOP, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for discard to reduce cost */
		if (o_ptr->code & P2_DISCARD_REDUCE)
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for consume good to reduce cost */
		if (o_ptr->code & P2_CONSUME_RARE)
		{
			/* Get list of cards with Rare goods */
			num_goods = get_goods(g, who, g_list, GOOD_RARE);

			/* Add card to special if goods available */
			if (num_goods)
			{
				/* Add to special list */
				special[num_special++] = w_list[i].c_idx;
			}
		}
	}

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Add fake cards to list */
	for (i = 0; i < p_ptr->fake_hand - p_ptr->fake_discards; i++)
	{
		/* Add one fake card */
		list[n++] = -1;
	}

	/* Do not ask for payment if not needed or allowed */
	if (cost == 0 && !num_special)
	{
		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s pays 0 for %s.\n",
			        p_ptr->name, g->deck[p_ptr->placing].d_ptr->name);

			/* Send message */
			message_add(g, msg);
		}

		return;
	}

	/* Ask player to decide how to pay */
	ask_player(g, who, CHOICE_PAYMENT, list, &n, special, &num_special,
	           p_ptr->placing, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Apply payment */
	devel_callback(g, who, p_ptr->placing, list, n, special, num_special);
}

/*
 * The second half of the Develop Phase -- paying for chosen developments.
 */
void develop_action(game *g, int who, int placing)
{
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	char* name;
	int i, n, cost;

	/* Get card placed */
	c_ptr = &g->deck[placing];

	/* Get cost */
	cost = c_ptr->d_ptr->cost;

	/* Get list of develop powers */
	n = get_powers(g, who, PHASE_DEVELOP, w_list);

	/* Look for reduce cost powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for reduce cost */
		if (o_ptr->code & P2_REDUCE) cost -= o_ptr->value;
	}

	/* Check for develop action chosen */
	if (player_chose(g, who, g->cur_action)) cost--;

	/* Check for prestige develop */
	if (player_chose(g, who, ACT_PRESTIGE | g->cur_action)) cost -= 2;

	/* Do not reduce cost below zero */
	if (cost < 0) cost = 0;

	/* Have player pay cost */
	pay_devel(g, who, cost);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Get name of card with power */
		name = g->deck[w_list[i].c_idx].d_ptr->name;

		/* Check for "draw after developing" power */
		if (o_ptr->code & P2_DRAW_AFTER)
		{
			/* Draw cards */
			draw_cards(g, who, o_ptr->value, name);
		}

		/* Check for "earn prestige after Rebel" power */
		if (o_ptr->code & P2_PRESTIGE_REBEL)
		{
			/* Check for Rebel flag on played card */
			if (c_ptr->d_ptr->flags & FLAG_REBEL)
			{
				/* Reward prestige */
				gain_prestige(g, who, o_ptr->value, name);
			}
		}

		/* Check for "earn prestige after 6 dev" power */
		if (o_ptr->code & P2_PRESTIGE_SIX)
		{
			/* Check for six-cost development */
			if (c_ptr->d_ptr->cost == 6)
			{
				/* Reward prestige */
				gain_prestige(g, who, o_ptr->value, name);
			}
		}

		/* Check for "earn prestige" power */
		if (o_ptr->code & P2_PRESTIGE)
		{
			/* Reward prestige */
			gain_prestige(g, who, o_ptr->value, name);
		}
	}
}

/*
 * Handle the Develop Phase.
 *
 * Ask each player what they would like to develop.
 */
void phase_develop(game *g)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	char *name;
	char msg[1024];
	int list[MAX_DECK];
	int g_list[MAX_DECK], num_goods;
	int i, j, x, n, reduce, max, explore;
	int asked[MAX_PLAYER];

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current turn */
		g->turn = i;

		/* Get list of develop powers */
		n = get_powers(g, i, PHASE_DEVELOP, w_list);

		/* Clear count of explore powers */
		explore = 0;

		/* Look for beginning of phase powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Get name of card with power */
			name = g->deck[w_list[j].c_idx].d_ptr->name;

			/* Check for draw */
			if (o_ptr->code & P2_DRAW)
			{
				/* Draw cards */
				draw_cards(g, i, o_ptr->value, name);

				/* Done */
				continue;
			}

			/* Check for "explore" */
			if (o_ptr->code & P2_EXPLORE)
			{
				/* Draw cards */
				draw_cards(g, i, o_ptr->value, name);

				/* Discard one later */
				explore++;

				/* Done */
				continue;
			}
		}

		/* Check for unfinished "explore" power */
		if (explore)
		{
			/* Ask player to discard one per explore */
			player_discard(g, i, explore);

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s discards %d card%s.\n", g->p[i].name,
				        explore, PLURAL(explore));

				/* Send message */
				message_add_formatted(g, msg, FORMAT_VERBOSE);
			}

			/* Check for aborted game */
			if (g->game_over) return;
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current turn */
		g->turn = i;

		/* Get list of develop powers */
		n = get_powers(g, i, PHASE_DEVELOP, w_list);

		/* Clear placing selection */
		p_ptr->placing = -1;

		/* Assume player not asked */
		asked[i] = 0;

		/* Assume no reduction in cost */
		reduce = 0;

		/* Check for develop action chosen */
		if (player_chose(g, i, g->cur_action)) reduce += 1;

		/* Check for prestige develop */
		if (player_chose(g, i, ACT_PRESTIGE | g->cur_action))
		{
			/* Cost is reduced by 2 more */
			reduce += 2;
		}

		/* Look for cost reduction powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Skip non-reduce powers */
			if (!(o_ptr->code & P2_REDUCE)) continue;

			/* Total reduction */
			reduce += o_ptr->value;
		}

		/* Look for optional reduce powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Check for discard to reduce */
			if (o_ptr->code & P2_DISCARD_REDUCE)
			{
				/* Assume power can be used */
				reduce += o_ptr->value;
			}

			/* Check for consume Rare to reduce */
			if (o_ptr->code & P2_CONSUME_RARE)
			{
				/* Get Rare goods */
				num_goods = get_goods(g, i, g_list, GOOD_RARE);

				/* Apply reduction if possible */
				if (num_goods) reduce += o_ptr->value;
			}
		}

		/* Compute maximum cost */
		max = count_player_area(g, i, WHERE_HAND) + p_ptr->fake_hand -
		      p_ptr->fake_discards + reduce - 1;

		/* Check for simulated opponent */
		if (g->simulation && g->sim_who != i)
		{
			/* Check for no cards available */
			if (max <= 0) continue;

			/* Give no choices */
			n = 0;

			/* Ask AI to simulate opponent's choice */
			send_choice(g, i, CHOICE_PLACE, list, &n, NULL, NULL,
				    PHASE_DEVELOP, -1, max);

			/* Player was asked */
			asked[i] = 1;

			/* Next player */
			continue;
		}

		/* No cards in list */
		n = 0;

		/* Start at first card in hand */
		x = p_ptr->head[WHERE_HAND];

		/* Loop over cards in hand */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip non-developments */
			if (c_ptr->d_ptr->type != TYPE_DEVELOPMENT) continue;

			/* Skip too-expensive cards */
			if (c_ptr->d_ptr->cost > max) continue;

			/* Skip duplicate card designs */
			if (player_has(g, i, c_ptr->d_ptr)) continue;

			/* Add card to list */
			list[n++] = x;
		}

		/* Check for no choices */
		if (!n) continue;

		/* Ask player to choose */
		send_choice(g, i, CHOICE_PLACE, list, &n, NULL, NULL,
		            PHASE_DEVELOP, -1, 0);

		/* Player was asked to place */
		asked[i] = 1;

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Wait for all players to finish choosing */
	wait_for_all(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip players who were not asked */
		if (!asked[i])
		{
			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s does not place a development.\n",
				        p_ptr->name);

				/* Send message */
				message_add_formatted(g, msg, FORMAT_VERBOSE);
			}

			continue;
		}

		/* Get player's answer about placement */
		p_ptr->placing = extract_choice(g, i, CHOICE_PLACE, list, &n,
		                                NULL, NULL);

		/* Skip players who are not placing anything */
		if (p_ptr->placing == -1)
		{
			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s does not place a development.\n",
				        p_ptr->name);

				/* Send message */
				message_add_formatted(g, msg, FORMAT_VERBOSE);
			}

			continue;
		}

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s places %s.\n", p_ptr->name,
			        g->deck[p_ptr->placing].d_ptr->name);

			/* Send message */
			message_add(g, msg);
		}

		/* Place card */
		place_card(g, i, p_ptr->placing);
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip players who are not placing anything */
		if (p_ptr->placing == -1) continue;

		/* Check for prepare function */
		if (p_ptr->control->prepare_phase)
		{
			/* Ask player to prepare answers for payment */
			p_ptr->control->prepare_phase(g, i, PHASE_DEVELOP,
			                              p_ptr->placing);
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip players who are not placing anything */
		if (p_ptr->placing == -1) continue;

		/* Handle choice */
		develop_action(g, i, p_ptr->placing);

		/* Clear placing choice */
		p_ptr->placing = -1;

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Clear any temp flags on cards */
	clear_temp(g);

	/* Check intermediate goals */
	check_goals(g);

	/* Check prestige leader */
	check_prestige(g);
}

/*
 * Return player's military strength against the given world.
 * Includes cost of world if defending.
 */
int strength_against(game *g, int who, int world, int attack, int defend)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int i, n;
	int military, good;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card pointer */
	c_ptr = &g->deck[world];

	/* Get world's good type */
	good = c_ptr->d_ptr->good_type;

	/* Get Settle phase powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Count non-specific military */
	military = total_military(g, who);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for specific extra military */
		if (o_ptr->code & P3_EXTRA_MILITARY)
		{
			/* Check for specific good required */
			if (((o_ptr->code & P3_NOVELTY) && good == GOOD_NOVELTY) ||
			    ((o_ptr->code & P3_RARE) && good == GOOD_RARE) ||
			    ((o_ptr->code & P3_GENE) && good == GOOD_GENE) ||
			    ((o_ptr->code & P3_ALIEN) && good == GOOD_ALIEN))
			{
				/* Add value */
				military += o_ptr->value;
				continue;
			}

			/* Check for against rebels */
			if ((o_ptr->code & P3_AGAINST_REBEL) &&
			    (c_ptr->d_ptr->flags & FLAG_REBEL))
			{
				/* Add value */
				military += o_ptr->value;
				continue;
			}
		}

		/* Check for takeover defense */
		if (defend && (o_ptr->code & P3_TAKEOVER_DEFENSE))
		{
			/* Add defense for military worlds */
			military += count_active_flags(g, who,
			                               FLAG_MILITARY);

			/* Add extra defense for Rebel military worlds */
			military += count_active_flags(g, who,
			                          (FLAG_REBEL | FLAG_MILITARY));
		}
	}

	/* Check for attack used */
	if (attack >= 0)
	{
		/* Get attack card */
		c_ptr = &g->deck[attack];

		/* Loop over powers */
		for (i = 0; i < c_ptr->d_ptr->num_power; ++i)
		{
			/* Get power */
			o_ptr = &c_ptr->d_ptr->powers[i];

			/* Skip non-Settle powers */
			if (o_ptr->phase != PHASE_SETTLE) continue;

			/* Check for "takeover imperium" power */
			if (o_ptr->code & P3_TAKEOVER_IMPERIUM)
			{
				/* Add bonus military */
				military += o_ptr->value *
				         count_active_flags(g, who, FLAG_REBEL |
				                                    FLAG_MILITARY);
			}
		}
	}

	/* Check for defense */
	if (defend)
	{
		/* Add cost of world */
		military += c_ptr->d_ptr->cost;
	}

	/* Add in bonus temporary military strength */
	military += p_ptr->bonus_military;

	/* Return total military for this world */
	return military;
}

/*
 * Return true if the given player can settle the given world.
 */
int settle_legal(game *g, int who, int world, int mil_bonus, int mil_only)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int goods[MAX_DECK];
	int gene_used = 0;
	int i, n, cost, defense, military, conquer, good, pay_military;
	int pay_cost, pay_discount;
	int conquer_peaceful, conquer_bonus;
	int hand_military, hand_size;
	int takeover;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card pointer */
	c_ptr = &g->deck[world];

	/* Check for takeover attempt */
	takeover = (c_ptr->owner != who);

	/* Get settle phase powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Get initial cost/defense */
	cost = defense = c_ptr->d_ptr->cost;

	/* Check for military world */
	conquer = c_ptr->d_ptr->flags & FLAG_MILITARY;

	/* Get good type of world to be settled (if any) */
	good = c_ptr->d_ptr->good_type;

	/* Start with basic military strength */
	military = total_military(g, who);

	/* Assume we cannot pay for military worlds */
	pay_military = 0;

	/* Assume no discounts while paying for military */
	pay_cost = pay_discount = 0;

	/* Assume we cannot conquer peaceful worlds */
	conquer_peaceful = conquer_bonus = 0;

	/* Assume we cannot use cards in hand for military strength */
	hand_military = 0;

	/* Compute effective hand size (minus one for card played) */
	hand_size = count_player_area(g, who, WHERE_HAND) + p_ptr->fake_hand -
	            p_ptr->fake_discards - 1;

	/* We don't play a card from hand for takeovers */
	if (takeover) hand_size++;

	/* No card is legal if we have none */
	if (!takeover && hand_size < 0) return 0;

	/* Check for prestige settle */
	if (player_chose(g, who, ACT_PRESTIGE | g->cur_action)) cost -= 3;

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for reduce cost power */
		if (o_ptr->code & P3_REDUCE)
		{
			/* Check for specific good required */
			if (((o_ptr->code & P3_NOVELTY)&&good != GOOD_NOVELTY)||
			    ((o_ptr->code & P3_RARE) && good != GOOD_RARE) ||
			    ((o_ptr->code & P3_GENE) && good != GOOD_GENE) ||
			    ((o_ptr->code & P3_ALIEN) && good != GOOD_ALIEN))
			{
				/* Skip power */
				continue;
			}

			/* Check for consumption required */
			if (o_ptr->code & P3_CONSUME_GENE)
			{
				/* Mark one good as used */
				gene_used++;

				/* Check for sufficent genes goods avaliable */
				if (get_goods(g, who, goods, GOOD_GENE) <
				       gene_used)
				{
					/* Skip power */
					continue;
				}
			}

			/* Reduce cost */
			cost -= o_ptr->value;
		}

		/* Check for reduce cost to zero */
		if (o_ptr->code & P3_REDUCE_ZERO)
		{
			/* Alien production worlds cannot be affected */
			if (good == GOOD_ALIEN) continue;

			/* World can be settled without cost */
			cost = 0;
		}

		/* Check for extra military */
		if (o_ptr->code & P3_EXTRA_MILITARY)
		{
			/* Check for specific good required */
			if (((o_ptr->code & P3_NOVELTY)&&good == GOOD_NOVELTY)||
			    ((o_ptr->code & P3_RARE) && good == GOOD_RARE) ||
			    ((o_ptr->code & P3_GENE) && good == GOOD_GENE) ||
			    ((o_ptr->code & P3_ALIEN) && good == GOOD_ALIEN))
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}

			/* Check for consumption required */
			if ((o_ptr->code & P3_CONSUME_RARE) &&
			    get_goods(g, who, goods, GOOD_RARE) > 0)
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}

			/* Check for prestige required */
			if ((o_ptr->code & P3_CONSUME_PRESTIGE) &&
			    p_ptr->prestige > 0)
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}

			/* Check for against rebels */
			if ((o_ptr->code & P3_AGAINST_REBEL) &&
			    (c_ptr->d_ptr->flags & FLAG_REBEL))
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}

			/* Check for discard needed */
			if (o_ptr->code & P3_DISCARD)
			{
				/* Assume card will be used */
				military += o_ptr->value;
				continue;
			}
		}

		/* Check for able to pay for military worlds */
		if (o_ptr->code & P3_PAY_MILITARY)
		{
			/* Check for alien flag */
			if (o_ptr->code & P3_ALIEN)
			{
				/* Can only pay for alien world */
				if (good != GOOD_ALIEN) continue;
			}
			else
			{
				/* Cannot pay for alien production world */
				if (good == GOOD_ALIEN) continue;
			}

			/* Check for against rebels */
			if ((o_ptr->code & P3_AGAINST_REBEL) &&
			    !(c_ptr->d_ptr->flags & FLAG_REBEL))
			{
				/* Skip power */
				continue;
			}

			/* Check for against chromo */
			if ((o_ptr->code & P3_AGAINST_CHROMO) &&
			    !(c_ptr->d_ptr->flags & FLAG_CHROMO))
			{
				/* Skip power */
				continue;
			}

			/* Mark ability */
			pay_military = 1;

			/* Check for bigger discount */
			if (o_ptr->value > pay_cost)
			{
				/* Remember best cost */
				pay_cost = o_ptr->value;
			}
		}

		/* Check for discount when using pay for military */
		if (o_ptr->code & P3_PAY_DISCOUNT)
		{
			/* Track discount */
			pay_discount += o_ptr->value;
		}

		/* Check for ability to use cards from hand for military */
		if (o_ptr->code & P3_MILITARY_HAND)
		{
			/* Use ability to utmost */
			hand_military += o_ptr->value;

			/* Limit to handsize */
			if (hand_military > hand_size)
				hand_military = hand_size;
		}

		/* Check for ability to conquer peaceful world */
		if (o_ptr->code & P3_CONQUER_SETTLE)
		{
			/* Mark ability as available */
			conquer_peaceful = 1;

			/* Track best bonus */
			if (o_ptr->value > conquer_bonus)
			{
				/* Remember best bonus */
				conquer_bonus = o_ptr->value;
			}
		}
	}

	/* Apply bonus military accrued earlier in the phase */
	military += p_ptr->bonus_military;

	/* Apply bonus military from takeover power, if any */
	if (takeover) military += mil_bonus;

	/* Compute owner's military strength for defense on takeovers */
	if (takeover) defense = strength_against(g, c_ptr->owner, world, -1, 1);

	/* Check for military world and sufficient strength */
	if (conquer && military + hand_military >= defense) return 1;

	/* Check for peaceful world and ability to conquer */
	if (!conquer && conquer_peaceful &&
	    military + hand_military + conquer_bonus >= defense)
	{
		/* Can be played */
		return 1;
	}

	/* Takeovers must be military */
	if (takeover) return 0;

	/* When checking for military worlds only, cannot pay instead */
	if (mil_only) pay_military = 0;

	/* Check for military and inability to pay */
	if (conquer && !pay_military) return 0;

	/* Paying for military world may grant discount */
	if (conquer && pay_military) cost -= pay_cost + pay_discount;

	/* Cannot pay for worlds when only military worlds available */
	if (mil_only) return 0;

	/* Check for sufficient cards */
	if (hand_size >= cost)
	{
		/* Can afford */
		return 1;
	}

	/* Cannot afford */
	return 0;
}

/*
 * Called when player has chosen how to pay the world they are placing.
 *
 * We return 0 if the payment would not succeed.  We also return 0 in
 * cases where too many cards were discarded, to prevent stupid AI play.
 */
int settle_callback(game *g, int who, int which, int list[], int num,
                    int special[], int num_special, int mil_only)
{
	player *p_ptr;
	card *c_ptr, *t_ptr;
	power_where w_list[100];
	power *o_ptr;
	char *name;
	int conquer, pay_military = 0, military, cost, good;
	int hand_military = 0, conquer_peaceful = 0;
	int discard_zero = 0, takeover = 0;
	int consume_reduce = 0, consume_military = 0;
	int consume_special[2], num_consume_special;
	int g_list[MAX_DECK], num_goods;
	int goods_needed[6];
	int i, j, n;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get pointer to card being played */
	t_ptr = &g->deck[which];

	/* Check for takeover attempt */
	takeover = (t_ptr->owner != who);

	/* Get card cost */
	cost = t_ptr->d_ptr->cost;

	/* Get card's good type */
	good = t_ptr->d_ptr->good_type;

	/* Check for military world */
	conquer = t_ptr->d_ptr->flags & FLAG_MILITARY;

	/* Count basic military strength */
	military = total_military(g, who);

	/* Add bonuses from earlier in the phase */
	military += p_ptr->bonus_military;

	/* Reduce cost by bonus reductions from earlier in the phase */
	cost -= p_ptr->bonus_reduce;

	/* Clear number of goods needed */
	for (i = 0; i < 6; i++) goods_needed[i] = 0;

	/* Get all active settle powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Loop over special cards used */
	for (i = 0; i < num_special; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[special[i]];

		/* Loop over card's powers */
		for (j = 0; j < c_ptr->d_ptr->num_power; j++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[j];

			/* Get name of card with power */
			name = c_ptr->d_ptr->name;

			/* Skip non-settle phase power */
			if (o_ptr->phase != PHASE_SETTLE) continue;

			/* Check for pay for military ability */
			if (o_ptr->code & P3_PAY_MILITARY)
			{
				/* Don't use multiple pay abilities at once */
				if (pay_military) return 0;

				/* Check for correct alien-ness */
				if (((o_ptr->code & P3_ALIEN) &&
				      good == GOOD_ALIEN) ||
				    (!(o_ptr->code & P3_ALIEN) &&
				      good != GOOD_ALIEN))
				{
					/* Mark ability */
					pay_military = 1;

					/* Reduce cost */
					cost -= o_ptr->value;

					/* Do not reduce cost below zero */
					if (cost < 0) cost = 0;

					/* Message */
					if (!g->simulation)
					{
						/* Format message */
						sprintf(msg,
						        "%s uses %s.\n",
						        p_ptr->name,
						        c_ptr->d_ptr->name);

						/* Send message */
						message_add(g, msg);
					}
				}
			}

			/* Check for discard to use power */
			if (o_ptr->code & P3_DISCARD)
			{
				/* Discard card */
				discard_card(g, who, special[i]);

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards %s.\n",
					        p_ptr->name,
					        c_ptr->d_ptr->name);

					/* Send message */
					message_add(g, msg);
				}

				/* Check for cost reduction to zero */
				if (o_ptr->code & P3_REDUCE_ZERO)
				{
					/* Mark use of card */
					discard_zero = 1;

					/* Check for non-alien */
					if (good != GOOD_ALIEN)
					{
						/* Reduce cost to zero */
						cost = 0;
					}
				}

				/* Check for extra military gained */
				if (o_ptr->code & P3_EXTRA_MILITARY)
				{
					/* Add extra military */
					military += o_ptr->value;

					/* Remember bonus for later */
					p_ptr->bonus_military += o_ptr->value;
				}

				/* Check for conquer peaceful world */
				if (o_ptr->code & P3_CONQUER_SETTLE)
				{
					/* Add extra military */
					military += o_ptr->value;

					/* Make world conquerable */
					conquer = 1;

					/* Mark ability as used */
					conquer_peaceful = 1;

					/* Check for prestige award */
					if (o_ptr->code & P3_PRESTIGE &&
					    !takeover)
					{
						/* Award prestige */
						gain_prestige(g, who, 2, name);
					}
				}

				/* Check goal losses */
				check_goal_loss(g, who, GOAL_MOST_DEVEL);

				/* Use no more powers */
				break;
			}

			/* Check for hand cards for military */
			if (o_ptr->code & P3_MILITARY_HAND)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Assume cards are for military strength */
				hand_military += o_ptr->value;
			}

			/* Check for consume to reduce cost */
			if (o_ptr->code & P3_CONSUME_GENE)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Remember power is used */
				consume_reduce++;

				/* Reduce cost */
				cost -= o_ptr->value;

				/* Do not reduce cost below zero */
				if (cost < 0) cost = 0;

				/* Need one more gene good */
				goods_needed[GOOD_GENE]++;

				/* Remember discount for later settles */
				p_ptr->bonus_reduce += o_ptr->value;

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards a Genes good to "
					        "reduce cost.\n", p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}
			}

			/* Check for consume to increase military */
			if (o_ptr->code & P3_CONSUME_RARE)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Ask for goods to consume later */
				consume_military++;

				/* Add extra military */
				military += o_ptr->value;

				/* Remember bonus for later */
				p_ptr->bonus_military += o_ptr->value;

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards a Rare good for "
					        "extra military.\n", p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}

				/* Need one more rare good */
				goods_needed[GOOD_RARE]++;
			}

			/* Check for prestige to increase military */
			if (o_ptr->code & P3_CONSUME_PRESTIGE)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Spend prestige */
				spend_prestige(g, who, 1);

				/* Add extra military */
				military += o_ptr->value;

				/* Remember bonus for later */
				p_ptr->bonus_military += o_ptr->value;

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s spends prestige for "
					        "extra military.\n",
					        p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}
			}
		}
	}

	/* Check for using military from hand */
	if (hand_military > 0)
	{
		/* Check for too many cards given */
		if (num > hand_military) return 0;

		/* Reduce hand military strength to cards given */
		hand_military = num;

		/* Remember bonus military for later */
		p_ptr->bonus_military += num;

		/* Military from hand is incompatible with pay for military */
		if (pay_military) return 0;
	}

	/* Must use "conquer peaceful" if only military worlds can be settled */
	if (mil_only && !conquer_peaceful && !conquer) return 0;

	/* Must use "conquer peaceful" if takeover and non-military target */
	if (takeover && !conquer_peaceful && !conquer) return 0;

	/* Cannot use "conquer peaceful" and "pay for military" together */
	if (conquer_peaceful && pay_military) return 0;

	/* Check for illegal use of "discard to reduce cost to zero" */
	if (discard_zero && conquer && !pay_military) return 0;

	/* Check for sufficient goods */
	for (i = GOOD_NOVELTY; i <= GOOD_ALIEN; i++)
	{
		/* Check for none needed of this type */
		if (!goods_needed[i]) continue;

		/* Check for insufficient goods */
		if (get_goods(g, who, g_list, i) < goods_needed[i]) return 0;
	}

	/* Check for prestige settle */
	if (player_chose(g, who, ACT_PRESTIGE | g->cur_action)) cost -= 3;

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Get name of card with power */
		name = g->deck[w_list[i].c_idx].d_ptr->name;

		/* Check for reduce cost power */
		if (o_ptr->code & P3_REDUCE)
		{
			/* Check for specific good required */
			if (((o_ptr->code & P3_NOVELTY)&&good != GOOD_NOVELTY)||
			    ((o_ptr->code & P3_RARE) && good != GOOD_RARE) ||
			    ((o_ptr->code & P3_GENE) && good != GOOD_GENE) ||
			    ((o_ptr->code & P3_ALIEN) && good != GOOD_ALIEN))
			{
				/* Skip power */
				continue;
			}

			/* Skip powers that require good consumption */
			if (o_ptr->code & P3_CONSUME_GENE) continue;

			/* Reduce cost */
			cost -= o_ptr->value;
		}

		/* Check for extra military */
		if (o_ptr->code & P3_EXTRA_MILITARY)
		{
			/* Skip powers that require good consumption */
			if (o_ptr->code & P3_CONSUME_RARE) continue;

			/* Check for specific good required */
			if (((o_ptr->code & P3_NOVELTY) && good == GOOD_NOVELTY) ||
			    ((o_ptr->code & P3_RARE) && good == GOOD_RARE) ||
			    ((o_ptr->code & P3_GENE) && good == GOOD_GENE) ||
			    ((o_ptr->code & P3_ALIEN) && good == GOOD_ALIEN))
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}

			/* Check for against rebels */
			if ((o_ptr->code & P3_AGAINST_REBEL) &&
			    (t_ptr->d_ptr->flags & FLAG_REBEL))
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}
		}

		/* Discount when using pay for military */
		if (pay_military && (o_ptr->code & P3_PAY_DISCOUNT))
		{
			/* Apply extra discount */
			cost -= o_ptr->value;
		}

		/* Gain prestige when using pay for military */
		if (pay_military && (o_ptr->code & P3_PAY_PRESTIGE))
		{
			/* Gain prestige */
			gain_prestige(g, who, o_ptr->value, name);
		}
	}

	/* Do not reduce cost below zero */
	if (cost < 0) cost = 0;

	/* Check for insufficient military strength (except for takeovers) */
	if (!takeover && conquer && !pay_military &&
	    military + hand_military < t_ptr->d_ptr->cost)
	{
		/* Illegal payment */
		return 0;
	}

#if 0
	/* Check for hand military used and too much strength */
	if (hand_military > 0 && military + hand_military > c_ptr->d_ptr->cost)
	{
		/* Too much payment */
		return 0;
	}
#endif

	/* Check for insufficient payment */
	if ((!conquer || pay_military) && cost > num)
	{
		/* Insufficient payment */
		return 0;
	}

	/* Disallow overpayment */
	if ((!conquer || pay_military) && cost < num)
	{
		/* Too much payment */
		return 0;
	}

	/* Disallow normal paying for military */
	if (conquer && !pay_military && num > 0 && hand_military == 0)
	{
		/* Too much payment */
		return 0;
	}

	/* Disallow consume goods to reduce cost when conquering */
	if (conquer && !pay_military && consume_reduce > 0)
	{
		/* Illegal payment */
		return 0;
	}

	/* Disallow consuming goods for both cost reduction and military */
	if (consume_reduce && consume_military)
	{
		/* Illegal payment */
		return 0;
	}

	/* Loop over consume powers used */
	for (i = 0; i < num_special; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[special[i]];

		/* Loop over card's powers */
		for (j = 0; j < c_ptr->d_ptr->num_power; j++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[j];

			/* Skip non-settle phase power */
			if (o_ptr->phase != PHASE_SETTLE) continue;

			/* Check for needing gene good */
			if (o_ptr->code & P3_CONSUME_GENE)
			{
				/* Get good list */
				num_goods = get_goods(g, who, g_list,
				                      GOOD_GENE);
			}

			/* Check for needing rare good */
			else if (o_ptr->code & P3_CONSUME_RARE)
			{
				/* Get good list */
				num_goods = get_goods(g, who, g_list,
				                      GOOD_RARE);
			}

			else
			{
				/* Not applicable power */
				continue;
			}

			/* Set power location */
			consume_special[0] = special[i];
			consume_special[1] = j;
			num_consume_special = 2;

			/* Check for multiple goods */
			if (num_goods > 1)
			{
				/* Ask player to choose good to discard */
				ask_player(g, who, CHOICE_GOOD, g_list,
				           &num_goods, consume_special,
				           &num_consume_special, 1, 1, 0);

				/* Check for aborted game */
				if (g->game_over) return 0;
			}

			/* Discard chosen good */
			good_chosen(g, who, special[i], j, g_list, num_goods);
		}
	}

	/* Loop over cards chosen as payment */
	for (i = 0; i < num; i++)
	{
		/* Check for fake card passed in as payment */
		if (list[i] < 0)
		{
			/* Add to count */
			p_ptr->fake_discards++;
			continue;
		}

		/* Private message */
		if (!g->simulation && g->p[who].control->private_message)
		{
			/* Format message */
			sprintf(msg, "%s discards %s.\n", p_ptr->name,
			        g->deck[list[i]].d_ptr->name);

			/* Send message */
			g->p[who].control->private_message(g, who, msg, FORMAT_DISCARD);
		}

		/* Discard */
		discard_card(g, who, list[i]);
	}

	/* Message */
	if (!g->simulation)
	{
		/* Check for takeover attempt and payment for extra military */
		if (takeover && hand_military > 0)
		{
			/* Format message */
			sprintf(msg, "%s pays %d for extra military.\n",
			        p_ptr->name, num);
		}

		/* Otherwise no message for takeover attempt */
		else if (takeover)
		{
			/* No message */
			strcpy(msg, "");
		}

		/* Check for conquer with help from hand */
		else if (conquer && !pay_military && hand_military > 0)
		{
			/* Format message */
			sprintf(msg, "%s pays %d to conquer %s.\n", p_ptr->name,
			        num, t_ptr->d_ptr->name);
		}

		/* Check for normal conquer */
		else if (conquer && !pay_military)
		{
			/* Format message */
			sprintf(msg, "%s conquers %s.\n", p_ptr->name,
			        t_ptr->d_ptr->name);
		}

		/* Check for payment */
		else
		{
			/* Check for extra military activated */
			if (hand_military)
			{
				/* Format message */
				sprintf(msg, "%s pays %d for extra military.\n",
				        p_ptr->name, hand_military);

				/* Send message */
				message_add(g, msg);
			}

			/* Format message */
			sprintf(msg, "%s pays %d for %s.\n", p_ptr->name, num,
			        t_ptr->d_ptr->name);
		}

		/* Send message */
		if (strlen(msg)) message_add(g, msg);
	}

	/* Loop over settle powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for "save cost" power */
		if (o_ptr->code & P3_SAVE_COST)
		{
			/* Do not ask in simulated game */
			if (g->simulation) continue;

			/* Cards used to increase military are ineligible */
			if (hand_military > 0) continue;

			/* Check for no cards spent */
			if (num == 0) continue;

			/* Check for more than one card to choose */
			if (num > 1)
			{
				/* Ask player which to save */
				ask_player(g, who, CHOICE_SAVE, list, &num,
				           NULL, NULL, 0, 0, 0);

				/* Check for aborted game */
				if (g->game_over) return 0;
			}

			/* Format message */
			sprintf(msg, "%s saves 1 card under Galactic Scavengers.\n",
			        p_ptr->name);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);

			/* Move card to saved area */
			move_card(g, list[0], who, WHERE_SAVED);

			/* Private message */
			if (g->p[who].control->private_message)
			{
				/* Format message */
				sprintf(msg, "%s saves %s.\n", p_ptr->name,
				        g->deck[list[0]].d_ptr->name);

				/* Send message */
				g->p[who].control->private_message(g, who, msg,
				                                   FORMAT_DISCARD);
			}
		}
	}

	/* Card is now paid for */
	g->deck[which].unpaid = 0;

	/* Payment is good */
	return 1;
}

/*
 * Ask a player to pay to settle a world.
 *
 * This may require using cards with a one-off discard ability to lower
 * cost or increase military, etc.
 */
static void pay_settle(game *g, int who, int world, int mil_only)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int list[MAX_DECK], special[MAX_DECK], g_list[MAX_DECK];
	int conquer, good, military, cost, takeover;
	uint64_t flags;
	int n = 0, num_special = 0;
	int i;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get pointer to world we are playing */
	c_ptr = &g->deck[world];

	/* Set flag if this is an intended takeover */
	takeover = (c_ptr->owner != who);

	/* Set flag if world is conquerable */
	conquer = (c_ptr->d_ptr->flags & FLAG_MILITARY) > 0;

	/* Get good type of world to be settled (if any) */
	good = c_ptr->d_ptr->good_type;

	/* Get cost or defense of world */
	cost = c_ptr->d_ptr->cost;

	/* Get flags for world to be settled */
	flags = c_ptr->d_ptr->flags;

	/* Count basic military strength */
	military = total_military(g, who);

	/* Get settle phase powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for extra military */
		if (o_ptr->code & P3_EXTRA_MILITARY)
		{
			/* Check for specific good required */
			if (((o_ptr->code & P3_NOVELTY)&&good == GOOD_NOVELTY)||
			    ((o_ptr->code & P3_RARE) && good == GOOD_RARE) ||
			    ((o_ptr->code & P3_GENE) && good == GOOD_GENE) ||
			    ((o_ptr->code & P3_ALIEN) && good == GOOD_ALIEN))
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}

			/* Check for against rebels */
			if ((o_ptr->code & P3_AGAINST_REBEL) &&
			    (c_ptr->d_ptr->flags & FLAG_REBEL))
			{
				/* Add value to military */
				military += o_ptr->value;
				continue;
			}
		}

		/* Check for discard to reduce cost */
		if (!takeover && (good != GOOD_ALIEN) &&
		    (o_ptr->code & P3_REDUCE_ZERO))
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for discard for extra military */
		if (o_ptr->code == (P3_DISCARD | P3_EXTRA_MILITARY))
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for pay for military ability */
		if (!takeover && !mil_only && conquer &&
		    (o_ptr->code & P3_PAY_MILITARY))
		{
			/* Check for against Rebels only */
			if (o_ptr->code & P3_AGAINST_REBEL)
			{
				/* Check for non-Rebel world */
				if (!(flags & FLAG_REBEL))
				{
					/* Skip power */
					continue;
				}
			}

			/* Check for against Chromo only */
			if (o_ptr->code & P3_AGAINST_CHROMO)
			{
				/* Check for non-Chromo world */
				if (!(flags & FLAG_CHROMO))
				{
					/* Skip power */
					continue;
				}
			}

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for military from hand ability */
		if (o_ptr->code & P3_MILITARY_HAND)
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for conquer peaceful world */
		if (!conquer && (o_ptr->code & P3_CONQUER_SETTLE))
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for consume gene */
		if ((o_ptr->code & P3_CONSUME_GENE) &&
		    get_goods(g, who, g_list, GOOD_GENE) > 0)
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for consume rare */
		if ((o_ptr->code & P3_CONSUME_RARE) &&
		    get_goods(g, who, g_list, GOOD_RARE) > 0)
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for prestige for military */
		if ((o_ptr->code & P3_CONSUME_PRESTIGE) &&
		    p_ptr->prestige > 0)
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}
	}

	/* Check for no need to pay */
	if (!takeover && conquer && military >= cost && !num_special)
	{
		/* Get pointer to world we are playing */
		c_ptr = &g->deck[p_ptr->placing];

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s conquers %s.\n", p_ptr->name,
			        c_ptr->d_ptr->name);

			/* Send message */
			message_add(g, msg);
		}

		/* Done */
		return;
	}

	/* Check for no ability to pay */
	if (takeover && !num_special)
	{
		/* Done until takeover resolution */
		return;
	}

	/* Clear list */
	n = 0;

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Add fake cards to list */
	for (i = 0; i < p_ptr->fake_hand - p_ptr->fake_discards; i++)
	{
		/* Add a fake card to list */
		list[n++] = -1;
	}

	/* Have player decide how to pay */
	ask_player(g, who, CHOICE_PAYMENT, list, &n, special, &num_special,
	           world, mil_only, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Apply payment */
	settle_callback(g, who, world, list, n, special, num_special, mil_only);
}

/*
 * Called to verify that a takeover power can be used against the given
 * world.
 *
 * Players can call this to make sure they are using the correct power
 * for an opponent.
 */
int takeover_callback(game *g, int special, int world)
{
	card *c_ptr;
	power *o_ptr;
	char msg[1024];
	int i, owner, rebel;

	/* Get world being targetted */
	c_ptr = &g->deck[world];

	/* Get owner of disputed card */
	owner = c_ptr->owner;

	/* Check for target world having rebel flag */
	rebel = c_ptr->d_ptr->flags & FLAG_REBEL;

	/* Get special card */
	c_ptr = &g->deck[special];

	/* Loop over powers */
	for (i = 0; i < c_ptr->d_ptr->num_power; i++)
	{
		/* Get power pointer */
		o_ptr = &c_ptr->d_ptr->powers[i];

		/* Skip non-Settle powers */
		if (o_ptr->phase != PHASE_SETTLE) continue;

		/* Skip non-takeover powers */
		if (!(o_ptr->code & (P3_TAKEOVER_REBEL |
		                     P3_TAKEOVER_IMPERIUM |
		                     P3_TAKEOVER_MILITARY |
		                     P3_TAKEOVER_PRESTIGE)))
		{
			/* Skip power */
			continue;
		}

		/* Check for discard flag */
		if (o_ptr->code & P3_DISCARD)
		{
			/* Discard card */
			discard_card(g, c_ptr->owner, special);
		}
		else
		{
			/* Mark power as used */
			c_ptr->used |= 1 << i;
		}

		/* Check for takeover rebel power */
		if (o_ptr->code & P3_TAKEOVER_REBEL)
		{
			/* May only takeover Rebel worlds */
			return rebel;
		}

		/* Check for takeover imperium power */
		if (o_ptr->code & P3_TAKEOVER_IMPERIUM)
		{
			/* Check for owner having imperium cards */
			return count_active_flags(g, owner, FLAG_IMPERIUM);
		}

		/* Check for takeover military power */
		if (o_ptr->code & P3_TAKEOVER_MILITARY)
		{
			/* Check for owner having positive military */
			return total_military(g, owner) > 0;
		}

		/* Check for takeover using prestige */
		if (o_ptr->code & P3_TAKEOVER_PRESTIGE)
		{
			/* Spend prestige */
			spend_prestige(g, c_ptr->owner, 1);

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s spends 1 prestige on %s.\n",
				        g->p[c_ptr->owner].name, c_ptr->d_ptr->name);

				/* Send message */
				message_add(g, msg);
			}

			/* Always successful */
			return 1;
		}
	}

	/* XXX */
	return 0;
}

/*
 * Check if a player can takeover opponent's cards, and if desired,
 * ask player which card to declare an attempt on.
 */
int settle_check_takeover(game *g, int who, card *extra, int ask)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int i, x, n, list[MAX_DECK];
	int special[MAX_DECK], num_special = 0;
	int takeover_rebel = 0, takeover_imperium = 0, takeover_military = 0;
	int takeover_prestige = 0, conquer_peaceful = 0;
	int rebel_vuln, all_vuln, target, bonus = 0;
	char msg[1024];

	/* Don't ask opponents in simulated game */
	if (g->simulation && g->sim_who != who) return 0;

	/* Don't ask if takeovers disabled */
	if (g->takeover_disabled) return 0;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get settle powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for "takeover rebel" power */
		if (o_ptr->code & P3_TAKEOVER_REBEL)
		{
			/* Mark power */
			takeover_rebel = 1;

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for "takeover imperium" power */
		if (o_ptr->code & P3_TAKEOVER_IMPERIUM)
		{
			/* Mark power */
			takeover_imperium = 1;

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for "takeover military" power */
		if (o_ptr->code & P3_TAKEOVER_MILITARY)
		{
			/* Mark power */
			takeover_military = 1;

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for "takeover using prestige" power */
		if (o_ptr->code & P3_TAKEOVER_PRESTIGE &&
		    p_ptr->prestige > 0)
		{
			/* Mark power */
			takeover_prestige = 1;

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;
		}

		/* Check for usable "conquer peaceful" power */
		if (o_ptr->code & P3_CONQUER_SETTLE &&
		    !(o_ptr->code & P3_NO_TAKEOVER))
		{
			/* Peaceful worlds can be made military */
			conquer_peaceful = 1;
		}
	}

	/* Check for no takeover powers */
	if (!num_special)
	{
		/* Nothing to do */
		return 0;
	}

	/* Clear list of target planets */
	n = 0;

	/* Loop over opponents */
	for (i = 0; i < g->num_players; i++)
	{
		/* Skip active player */
		if (i == who) continue;

		/* Assume player is not vulnerable */
		rebel_vuln = all_vuln = 0;

		/* Assume no military bonus */
		bonus = 0;

		/* Check for Rebel military world */
		if (takeover_rebel &&
		    count_active_flags(g, i, FLAG_REBEL | FLAG_MILITARY))
		{
			/* Player's Rebel worlds are vulnerable */
			rebel_vuln = 1;
		}

		/* Check for Imperium card */
		if (takeover_imperium &&
		    count_active_flags(g, i, FLAG_IMPERIUM))
		{
			/* Player is vulnerable */
			all_vuln = 1;

			/* Get bonus to military */
			bonus = 2 * count_active_flags(g, who, FLAG_REBEL |
			                                       FLAG_MILITARY);
		}

		/* Check for military power */
		if (takeover_military &&
		    total_military(g, i) > 0)
		{
			/* Player is vulnerable */
			all_vuln = 1;
		}

		/* Check for prestige takeover */
		if (takeover_prestige)
		{
			/* Player is vulnerable */
			all_vuln = 1;
		}

		/* Skip players who are not vulnerable */
		if (!rebel_vuln && !all_vuln) continue;

		/* Start at opponent's first active card */
		x = g->p[i].head[WHERE_ACTIVE];

		/* Loop over active cards */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip just-played cards */
			if (c_ptr->start_where != WHERE_ACTIVE) continue;

			/* Skip developments */
			if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

			/* Skip non-military worlds unless convertable */
			if (!(c_ptr->d_ptr->flags & FLAG_MILITARY) &&
			    !conquer_peaceful) continue;

			/* Skip non-Rebel worlds unless completely vulnerable */
			if (!all_vuln && !(c_ptr->d_ptr->flags & FLAG_REBEL))
				continue;

			/* Check for sufficient military strength */
			if (!settle_legal(g, who, x, bonus, 0)) continue;

			/* Add target world to list */
			list[n++] = x;
		}
	}

	/* Check for no legal choices */
	if (!n) return 0;

	/* Return if not asking */
	if (!ask) return 1;

	/* Ask player which world to attempt to takeover */
	target = ask_player(g, who, CHOICE_TAKEOVER, list, &n,
	                    special, &num_special, 0, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Check for no choice made */
	if (target == -1) return 0;

	/* Remember takeover for later */
	g->takeover_target[g->num_takeover] = target;
	g->takeover_who[g->num_takeover] = who;
	g->takeover_power[g->num_takeover] = special[0];
	g->takeover_defeated[g->num_takeover] = 0;

	/* One more takeover to resolve */
	g->num_takeover++;

	/* Get takeover card being used */
	c_ptr = &g->deck[special[0]];

	/* Message */
	if (!g->simulation)
	{
		/* Check for card used for extra placement */
		if (extra)
		{
			/* Format message */
			sprintf(msg, "%s uses %s to attempt to take over a world.\n",
			        p_ptr->name, extra->d_ptr->name);

			/* Add message */
			message_add(g, msg);
		}

		/* Format message */
		sprintf(msg, "%s uses %s to attempt takeover of %s.\n",
		        p_ptr->name, c_ptr->d_ptr->name,
		        g->deck[target].d_ptr->name);

		/* Send message */
		message_add_formatted(g, msg, FORMAT_TAKEOVER);
	}

	/* Discard card used, if needed */
	takeover_callback(g, special[0], target);

	/* Attempt declared */
	return 1;
}

/*
 * Return true if a given world can be replaced by the other.
 */
static int upgrade_legal(game *g, int replacement, int old)
{
	card *c_ptr, *b_ptr;

	/* Get card pointers */
	c_ptr = &g->deck[old];
	b_ptr = &g->deck[replacement];

	/* Ensure both cards are worlds */
	if (c_ptr->d_ptr->type != TYPE_WORLD) return 0;
	if (b_ptr->d_ptr->type != TYPE_WORLD) return 0;

	/* Ensure both cards are non-military */
	if (c_ptr->d_ptr->flags & FLAG_MILITARY) return 0;
	if (b_ptr->d_ptr->flags & FLAG_MILITARY) return 0;

	/* Check for illegal types */
	if (c_ptr->d_ptr->good_type != GOOD_ANY &&
	    b_ptr->d_ptr->good_type != GOOD_ANY &&
	    (c_ptr->d_ptr->good_type != b_ptr->d_ptr->good_type)) return 0;

	/* Worlds without goods can't match "any" */
	if ((!c_ptr->d_ptr->good_type ||
	     !b_ptr->d_ptr->good_type) &&
	    (c_ptr->d_ptr->good_type != b_ptr->d_ptr->good_type)) return 0;

	/* Check for card in hand too cheap */
	if (b_ptr->d_ptr->cost < c_ptr->d_ptr->cost) return 0;

	/* Check for card in hand too expensive */
	if (b_ptr->d_ptr->cost > c_ptr->d_ptr->cost + 3) return 0;

	/* Upgrade is legal */
	return 1;
}

/*
 * A player has chosen one world to replace another.
 *
 * We return 0 if the upgrade is illegal.
 */
int upgrade_chosen(game *g, int who, int replacement, int old)
{
	player *p_ptr;
	card *c_ptr, *b_ptr;
	char msg[1024];
	int i;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card pointers */
	c_ptr = &g->deck[old];
	b_ptr = &g->deck[replacement];

	/* Check for illegal replacement */
	if (!upgrade_legal(g, replacement, old)) return 0;

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s uses Terraforming Engineers to replace %s with %s.\n",
		        p_ptr->name, c_ptr->d_ptr->name, b_ptr->d_ptr->name);

		/* Send message */
		message_add(g, msg);
	}

	/* Check for good on old card */
	if (c_ptr->covered != -1)
	{
		/* Move good to discard */
		discard_card(g, who, c_ptr->covered);

		/* Clear covered flag */
		c_ptr->covered = -1;
	}

	/* Check for cards saved underneath world */
	if (c_ptr->d_ptr->flags & FLAG_START_SAVE)
	{
		/* Loop over cards in deck */
		for (i = 0; i < g->deck_size; i++)
		{
			/* Check for saved card */
			if (g->deck[i].where == WHERE_SAVED)
			{
				/* Move to discard */
				discard_card(g, who, i);
			}
		}
	}

	/* Discard old card */
	discard_card(g, who, old);

	/* Place new card */
	place_card(g, who, replacement);

	/* Award prestige for upgrade */
	gain_prestige(g, who, 1, "Terraforming Engineers");

	/* Success */
	return 1;
}

/*
 * Ask player to replace a world with another.
 */
static int upgrade_world(game *g, int who)
{
	card *c_ptr, *b_ptr;
	int i, x, y;
	int special[MAX_DECK], num_special = 0;
	int list[MAX_DECK], n = 0;

	/* Start at first card in hand */
	x = g->p[who].head[WHERE_HAND];

	/* Loop over cards in hand */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip non-worlds */
		if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

		/* Skip military worlds */
		if (c_ptr->d_ptr->flags & FLAG_MILITARY) continue;

		/* Start at first active card */
		y = g->p[who].head[WHERE_ACTIVE];

		/* Loop over active cards */
		for ( ; y != -1; y = g->deck[y].next)
		{
			/* Get card pointer */
			b_ptr = &g->deck[y];

			/* Skip newly-placed worlds */
			if (b_ptr->start_where != WHERE_ACTIVE) continue;

			/* Skip non-worlds */
			if (b_ptr->d_ptr->type != TYPE_WORLD) continue;

			/* Skip military worlds */
			if (b_ptr->d_ptr->flags & FLAG_MILITARY) continue;

			/* Check for legal upgrade */
			if (upgrade_legal(g, x, y))
			{
				/* Look for world to be replaced in list */
				for (i = 0; i < num_special; i++)
				{
					/* Check for match */
					if (special[i] == y) break;
				}

				/* Check for no match */
				if (i == num_special)
				{
					/* Add world to special list */
					special[num_special++] = y;
				}

				/* Look for replacement in list */
				for (i = 0; i < n; i++)
				{
					/* Check for match */
					if (list[i] == x) break;
				}

				/* Check for no match */
				if (i == n)
				{
					/* Add replacement to list */
					list[n++] = x;
				}
			}
		}
	}

	/* Check for no cards to upgrade to */
	if (!n) return 0;

	/* Ask player for an upgrade choice */
	ask_player(g, who, CHOICE_UPGRADE, list, &n, special, &num_special,
	           0, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Check for no choice made */
	if (!n) return 0;

	/* Upgrade world */
	upgrade_chosen(g, who, list[0], special[0]);

	/* Upgrade successful */
	return 1;
}

/*
 * Award player bonus cards for successfully placing a world.
 */
static void settle_bonus(game *g, int who, int world, int takeover,
                         int simulated)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	char *name;
	char msg[1024];
	int i, n, explore = 0;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get pointer to card placed */
	c_ptr = &g->deck[world];

	/* Get settle phase powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Check for chosen settle action */
	if (player_chose(g, who, g->cur_action) && !p_ptr->phase_bonus_used)
	{
		/* Draw card */
		draw_card(g, who, "his Settle bonus");

		/* Mark bonus as used */
		p_ptr->phase_bonus_used = 1;
	}

	/* Loop over pre-existing powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Get name of card with power */
		name = g->deck[w_list[i].c_idx].d_ptr->name;

		/* Check for draw power */
		if (o_ptr->code & P3_DRAW_AFTER)
		{
			/* Draw cards */
			draw_cards(g, who, o_ptr->value, name);
		}

		/* Check for prestige after rebel power */
		if (o_ptr->code & P3_PRESTIGE_REBEL)
		{
			/* Check for rebel military world placed */
			if ((c_ptr->d_ptr->flags & FLAG_REBEL) &&
			    (c_ptr->d_ptr->flags & FLAG_MILITARY))
			{
				/* Award prestige */
				gain_prestige(g, who, o_ptr->value, name);
			}
		}

		/* Check for prestige after production world */
		if (o_ptr->code & P3_PRODUCE_PRESTIGE)
		{
			/* Check for production world */
			if (c_ptr->d_ptr->good_type > 0 &&
			    !(c_ptr->d_ptr->flags & FLAG_WINDFALL))
			{
				/* Award prestige */
				gain_prestige(g, who, o_ptr->value, name);
			}
		}

		/* Check for "explore" after placement */
		if (o_ptr->code & P3_EXPLORE_AFTER)
		{
			/* Draw given number of cards */
			draw_cards(g, who, o_ptr->value, name);

			/* Count cards to discard later */
			explore++;
		}

		/* Check for "auto-production" */
		if (!takeover && (o_ptr->code & P3_AUTO_PRODUCE))
		{
			/* Check for production world placed */
			if (c_ptr->d_ptr->good_type > 0 &&
			    !(c_ptr->d_ptr->flags & FLAG_WINDFALL))
			{
				/* Add good to world */
				add_good(g, c_ptr);
			}
		}
	}

	/* Check for need to discard */
	if (explore && !simulated)
	{
		/* Have player discard */
		player_discard(g, who, explore);

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s discards %d card%s.\n", g->p[who].name,
			        explore, PLURAL(explore));

			/* Send message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);
		}
	}
}

/*
 * The second half of the Settle Phase -- paying for chosen worlds.
 */
void settle_action(game *g, int who, int world)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100], *w_ptr;
	power *o_ptr;
	int i, x, n, list[MAX_DECK];
	int place_again = -1, place_military = -1, upgrade = 0;
	int takeover = 0;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Check for not placing anything */
	if (world == -1)
	{
		/* Check for declared takeover */
		if (g->num_takeover &&
		    g->takeover_who[g->num_takeover - 1] == who)
		{
			/* Get most recent takeover target */
			world = g->takeover_target[g->num_takeover - 1];

			/* Set takeover flag */
			takeover = 1;
		}
	}

	/* Have user pay for card (in some way) */
	if (world != -1) pay_settle(g, who, world, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Check for placed world */
	if (world != -1 && !takeover)
	{
		/* Award bonuses for settling */
		settle_bonus(g, who, world, 0, 0);

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Get settle powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power location pointer */
		w_ptr = &w_list[i];

		/* Get power pointer */
		o_ptr = w_ptr->o_ptr;

		/* Check for place second world power */
		if (world != -1 && (o_ptr->code & P3_PLACE_TWO))
		{
			/* Ask player to place another world */
			place_again = w_ptr->c_idx;

			/* Mark power as used */
			g->deck[w_ptr->c_idx].used |= 1 << w_ptr->o_idx;
		}

		/* Check for place military world power */
		if (world != -1 && (o_ptr->code & P3_PLACE_MILITARY))
		{
			/* Ask player to place another */
			place_military = w_ptr->c_idx;

			/* Mark power as used */
			g->deck[w_ptr->c_idx].used |= 1 << w_ptr->o_idx;
		}

		/* Check for upgrade world power */
		if (o_ptr->code & P3_UPGRADE_WORLD)
		{
			/* Ask player about upgrade */
			upgrade = 1;

			/* Mark power as used */
			g->deck[w_ptr->c_idx].used |= 1 << w_ptr->o_idx;
		}
	}

	/* Check for card played to place again */
	if (place_again != -1)
	{
		/* Clear placing selection */
		p_ptr->placing = -1;

		/* Assume no cards to play */
		n = 0;

		/* Start at first card in hand */
		x = g->p[who].head[WHERE_HAND];

		/* Loop over cards in hand */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip developments */
			if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

			/* Skip cards that cannot be settled */
			if (!settle_legal(g, who, x, 0, 0)) continue;

			/* Add card to list */
			list[n++] = x;
		}

		/* Ask player about placement if choices available */
		if (n)
		{
			/* Ask player to choose */
			p_ptr->placing = ask_player(g, who, CHOICE_PLACE, list,
			                            &n, NULL, NULL,
			                            PHASE_SETTLE, place_again,
			                            0);

			/* Check for aborted game */
			if (g->game_over) return;
		}

		/* Get card used to place world */
		c_ptr = &g->deck[place_again];

		/* Check for no choice */
		if (p_ptr->placing == -1)
		{
			/* Ask for takeover declaration if possible */
			if (settle_check_takeover(g, who, c_ptr, 1))
			{
				/* Act on declaration */
				settle_action(g, who, -1);
			}

			/* Check for aborted game */
			if (g->game_over) return;
		}
		else
		{
			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s uses %s to place an additional world.\n",
				        p_ptr->name, c_ptr->d_ptr->name);

				/* Add message */
				message_add(g, msg);
			}

			/* Place card */
			place_card(g, who, p_ptr->placing);

			/* Act on settle */
			settle_action(g, who, p_ptr->placing);

			/* Check for aborted game */
			if (g->game_over) return;
		}
	}

	/* Check for card played to place military */
	if (place_military != -1 &&
	    g->deck[place_military].where != WHERE_DISCARD)
	{
		/* Clear placing selection */
		p_ptr->placing = -1;

		/* Assume no cards to play */
		n = 0;

		/* Start at first card in hand */
		x = g->p[who].head[WHERE_HAND];

		/* Loop over cards in hand */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip developments */
			if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

			/* Skip cards that cannot be settled */
			if (!settle_legal(g, who, x, 0, 1)) continue;

			/* Add card to list */
			list[n++] = x;
		}

		/* Ask player about placement if choices available */
		if (n)
		{
			/* Ask player to choose */
			p_ptr->placing = ask_player(g, who, CHOICE_PLACE, list,
			                            &n, NULL, NULL,
			                            PHASE_SETTLE,
			                            place_military, 1);

			/* Check for aborted game */
			if (g->game_over) return;
		}

		/* Check for choice made */
		if (p_ptr->placing != -1)
		{
			/* Place card */
			place_card(g, who, p_ptr->placing);

			/* Get card used to place world */
			c_ptr = &g->deck[place_military];

			/* Discard */
			discard_card(g, who, place_military);

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s discards %s to place an additional world.\n",
				        p_ptr->name, c_ptr->d_ptr->name);

				/* Send message */
				message_add(g, msg);
			}

			/* Check goal losses */
			check_goal_loss(g, who, GOAL_MOST_DEVEL);

			/* Have user pay for card (if necessary) */
			pay_settle(g, who, p_ptr->placing, 1);

			/* Check for aborted game */
			if (g->game_over) return;

			/* Award bonuses for settling */
			settle_bonus(g, who, p_ptr->placing, 0, 0);

			/* Check for aborted game */
			if (g->game_over) return;
		}
	}

	/* Check for upgrade power available */
	if (upgrade)
	{
		/* Ask player to upgrade a world */
		upgrade_world(g, who);
	}
}

/*
 * Called when player has chosen a method of defense.
 *
 * We return:
 *   0 if the method is illegal
 *   1 if the method is legal but insufficient to stop the takeover
 *   2 if the method is legal and stops the takeover
 */
int defend_callback(game *g, int who, int deficit, int list[], int num,
                    int special[], int num_special)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr;
	int military = 0, hand_military = 0;
	int num_goods, g_list[MAX_DECK];
	int consume_special[2], num_consume_special;
	int i, j;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Loop over special cards used */
	for (i = 0; i < num_special; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[special[i]];

		/* Loop over card's powers */
		for (j = 0; j < c_ptr->d_ptr->num_power; j++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[j];

			/* Skip non-Settle power */
			if (o_ptr->phase != PHASE_SETTLE) continue;

			/* Check for discard for extra military */
			if (o_ptr->code == (P3_DISCARD | P3_EXTRA_MILITARY))
			{
				/* Discard card */
				discard_card(g, who, special[i]);

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards %s for extra military.\n",
					        p_ptr->name,
					        c_ptr->d_ptr->name);

					/* Send message */
					message_add(g, msg);
				}

				/* Check goal losses */
				check_goal_loss(g, who, GOAL_MOST_DEVEL);

				/* Add extra military */
				military += o_ptr->value;

				/* Remember bonus for later */
				p_ptr->bonus_military += o_ptr->value;
			}

			/* Check for hand cards for military */
			if (o_ptr->code & P3_MILITARY_HAND)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Assume cards are for military strength */
				hand_military += o_ptr->value;
			}

			/* Check for consume to increase military */
			if (o_ptr->code & P3_CONSUME_RARE)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Add extra military */
				military += o_ptr->value;

				/* Remember bonus for later */
				p_ptr->bonus_military += o_ptr->value;

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s discards a Rare good for "
					        "extra military.\n", p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}
			}

			/* Check for prestige to increase military */
			if (o_ptr->code & P3_CONSUME_PRESTIGE)
			{
				/* Mark power as used */
				c_ptr->used |= 1 << j;

				/* Spend prestige */
				spend_prestige(g, who, 1);

				/* Add extra military */
				military += o_ptr->value;

				/* Remember bonus for later */
				p_ptr->bonus_military += o_ptr->value;

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s spends prestige for "
					        "extra military.\n",
					        p_ptr->name);

					/* Send message */
					message_add(g, msg);
				}
			}
		}
	}

	/* Check for too many cards passed */
	if (num > hand_military) return 0;

	/* Use cards passed as military strength */
	p_ptr->bonus_military += num;
	military += num;

	/* Message */
	if (!g->simulation && num > 0)
	{
		/* Private message */
		if (g->p[who].control->private_message)
		{
			/* Loop over choices */
			for (i = 0; i < num; i++)
			{
				/* Format message */
				sprintf(msg, "%s discards %s.\n", p_ptr->name,
				        g->deck[list[i]].d_ptr->name);

				/* Send message */
				g->p[who].control->private_message(g, who, msg, FORMAT_DISCARD);
			}
		}

		/* Format message */
		sprintf(msg, "%s pays %d for extra military.\n",
			p_ptr->name, num);

		/* Send message */
		message_add(g, msg);
	}

	/* Check for simulation */
	if (g->simulation)
	{
		/* Simulate payment */
		p_ptr->fake_discards += num;
	}
	else
	{
		/* Discard cards given */
		for (i = 0; i < num; i++)
		{
			/* Discard card */
			discard_card(g, who, list[i]);
		}
	}

	/* Loop over consume powers used */
	for (i = 0; i < num_special; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[special[i]];

		/* Loop over card's powers */
		for (j = 0; j < c_ptr->d_ptr->num_power; j++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[j];

			/* Skip non-settle phase power */
			if (o_ptr->phase != PHASE_SETTLE) continue;

			/* Check for needing rare good */
			if (o_ptr->code & P3_CONSUME_RARE)
			{
				/* Get good list */
				num_goods = get_goods(g, who, g_list,
				                      GOOD_RARE);
			}
			else
			{
				/* Not applicable power */
				continue;
			}

			/* Set power location */
			consume_special[0] = special[i];
			consume_special[1] = j;
			num_consume_special = 2;

			/* Check for multiple goods */
			if (num_goods > 1)
			{
				/* Ask player to choose good to discard */
				ask_player(g, who, CHOICE_GOOD, g_list,
				           &num_goods, consume_special,
				           &num_consume_special, 1, 1, 0);

				/* Check for aborted game */
				if (g->game_over) return 0;
			}

			/* Discard chosen good */
			good_chosen(g, who, special[i], j, g_list, num_goods);
		}
	}

	/* Check for sufficient strength */
	if (military > deficit) return 2;

	/* Legal but insufficient */
	return 1;
}

/*
 * Ask current owner for extra defense to spend in defense of a world.
 */
static void defend_takeover(game *g, int who, int world, int attacker,
                            int deficit)
{
	player *p_ptr;
	power_where w_list[100];
	power *o_ptr;
	int list[MAX_DECK], special[MAX_DECK];
	int goods[MAX_DECK];
	int n = 0, num_special = 0;
	int max = 0, hand_military = 0, hand_size;
	int i;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get settle powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for discard for extra military */
		if (o_ptr->code == (P3_DISCARD | P3_EXTRA_MILITARY))
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;

			/* Add value to maximum military */
			max += o_ptr->value;
		}

		/* Check for military from hand ability */
		if (o_ptr->code & P3_MILITARY_HAND)
		{
			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;

			/* Track amount we can spend */
			hand_military += o_ptr->value;
		}

		/* Check for consume prestige for military */
		if (o_ptr->code & P3_CONSUME_PRESTIGE)
		{
			/* Check for no prestige to spend */
			if (p_ptr->prestige == 0) continue;

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;

			/* Add value to maximum military */
			max += o_ptr->value;
		}

		/* Check for consume Rare good for military */
		if (o_ptr->code & P3_CONSUME_RARE)
		{
			/* Check for sufficient Rare goods */
			if (!get_goods(g, who, goods, GOOD_RARE))
			{
				/* Skip power */
				continue;
			}

			/* Add to special list */
			special[num_special++] = w_list[i].c_idx;

			/* Add value to maximum military */
			max += o_ptr->value;
		}
	}

	/* Compute effective hand size */
	hand_size = count_player_area(g, who, WHERE_HAND) + p_ptr->fake_hand -
	            p_ptr->fake_discards;

	/* Reduce amount of military from hand we can use to hand size */
	if (hand_military > hand_size) hand_military = hand_size;

	/* Add maximum hand military */
	max += hand_military;

	/* Check for no way to successfully defend */
	if (max <= deficit) return;

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Add fake cards to list */
	for (i = 0; i < p_ptr->fake_hand - p_ptr->fake_discards; i++)
	{
		/* Add a fake card to list */
		list[n++] = -1;
	}

	/* Check for no "military from hand" abilities */
	if (!hand_military) n = 0;

	/* Have player decide how to defend */
	ask_player(g, who, CHOICE_DEFEND, list, &n, special, &num_special,
	           world, attacker, deficit);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Apply choice */
	defend_callback(g, who, deficit, list, n, special, num_special);
}

/*
 * Resolve a takeover.
 */
int resolve_takeover(game *g, int who, int world, int special,
                     int defeated, int simulated)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr = NULL;
	char *name;
	int defense;
	int prestige = 0;
	char prestige_reason[1024];
	char msg[1024];
	int attack, defender;
	int i;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card pointer to special power used for takeover */
	c_ptr = &g->deck[special];

	/* Loop over powers */
	for (i = 0; i < c_ptr->d_ptr->num_power; i++)
	{
		/* Get power pointer */
		o_ptr = &c_ptr->d_ptr->powers[i];

		/* Get name of card with power */
		name = c_ptr->d_ptr->name;

		/* Skip non-Settle powers */
		if (o_ptr->phase != PHASE_SETTLE) continue;

		/* Check for "takeover imperium" power */
		if (o_ptr->code & P3_TAKEOVER_IMPERIUM)
		{
			/* Found takeover power */
			break;
		}

		/* Check for "takeover with prestige" power */
		if (o_ptr->code & P3_TAKEOVER_PRESTIGE)
		{
			/* Remember prestige award */
			prestige += o_ptr->value;

			/* Remember card name */
			strcpy(prestige_reason, name);

			break;
		}

		/* Stop at other takeover powers */
		if (o_ptr->code & (P3_TAKEOVER_REBEL | P3_TAKEOVER_MILITARY))
		{
			/* Destruction awards prestige */
			if (o_ptr->code & P3_DESTROY)
			{
				/* Add prestige */
				prestige += 2;

				/* Remember card name */
				strcpy(prestige_reason, name);
			}

			/* Found takeover power */
			break;
		}
	}

	/* Get card pointer */
	c_ptr = &g->deck[world];

	/* Get defender */
	defender = c_ptr->owner;

	/* Check for card moved since takeover declared */
	if (c_ptr->where != c_ptr->start_where ||
	    defender != c_ptr->start_owner)
	{
		/* Takeover fails */
		defeated = 1;

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "Takeover of %s is defeated because the world has been moved.\n",
			              c_ptr->d_ptr->name);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_TAKEOVER);
		}
	}

	/* Compute current owner's defense */
	if (!defeated) defense = strength_against(g, defender, world, -1, 1);

	/* Compute total attack strength */
	if (!defeated) attack = strength_against(g, who, world, special, 0);

	/* Message */
	if (!g->simulation && !defeated)
	{
		/* Format attack message */
		sprintf(msg, "%s attacks %s with %d military.\n",
		             g->p[who].name,
		             c_ptr->d_ptr->name,
		             attack);

		/* Send attack message */
		message_add(g, msg);
	}

	/* Check for successful takeover */
	if (!defeated && attack >= defense && !simulated)
	{
		/* Ask defender for any extra defense to spend */
		defend_takeover(g, defender, world, who, attack - defense);

		/* Check for aborted game */
		if (g->game_over) return 0;
	}

	/* Recompute defense */
	if (!defeated) defense = strength_against(g, defender, world, -1, 1);

	/* Check for non-military target */
	if (!(c_ptr->d_ptr->flags & FLAG_MILITARY))
	{
		/* Check for previously awarded prestige */
		if (prestige)
		{
			/* XXX Append card name */
			strcat(prestige_reason, " and Imperium Invasion Fleet");
		}
		else
		{
			/* XXX Remember card name */
			strcpy(prestige_reason, "Imperium Invasion Fleet");
		}

		/* XXX Increase prestige award */
		prestige += 2;
	}

	/* Message */
	if (!g->simulation && !defeated)
	{
		/* Format defense message */
		sprintf(msg, "%s defends %s with %d military.\n",
		        g->p[defender].name,
		        c_ptr->d_ptr->name,
		        defense);

		/* Send defense message */
		message_add(g, msg);
	}

	/* Check for insufficient attack strength */
	if (defeated || attack < defense)
	{
		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s fails to takeover %s.\n", p_ptr->name,
			        c_ptr->d_ptr->name);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_TAKEOVER);
		}

		/* Failure */
		return 0;
	}

	/* Award prestige for success */
	if (prestige) gain_prestige(g, who, prestige, prestige_reason);

	/* Check for destruction instead of normal takeover */
	if (o_ptr->code & P3_DESTROY)
	{
		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s destroys %s.\n", p_ptr->name,
				c_ptr->d_ptr->name);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_TAKEOVER);
		}

		/* Discard card */
		discard_card(g, defender, world);

		/* Award settle bonus */
		settle_bonus(g, who, world, 1, simulated);

		/* Check for aborted game */
		if (g->game_over) return 0;

		/* Check for cards saved underneath world */
		if (c_ptr->d_ptr->flags & FLAG_START_SAVE)
		{
			/* Loop over cards in deck */
			for (i = 0; i < g->deck_size; i++)
			{
				/* Check for saved card */
				if (g->deck[i].where == WHERE_SAVED)
				{
					/* Move to discard */
					discard_card(g, defender, i);
				}
			}
		}

		/* Check for good on world */
		if (c_ptr->covered != -1)
		{
			/* Discard good as well */
			discard_card(g, defender, c_ptr->covered);

			/* Remove covered from destroyed card */
			c_ptr->covered = -1;
		}

		/* Success */
		return 1;
	}

	/* Transfer ownership */
	move_card(g, world, who, WHERE_ACTIVE);

	/* Set new card order */
	c_ptr->order = p_ptr->table_order++;

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s takes over %s.\n", p_ptr->name,
			c_ptr->d_ptr->name);

		/* Send message */
		message_add_formatted(g, msg, FORMAT_TAKEOVER);
	}

	/* Check for good on world */
	if (c_ptr->covered != -1)
	{
		/* Transfer good as well */
		move_card(g, c_ptr->covered, who, WHERE_GOOD);
	}

	/* Check for cards saved underneath world */
	if (c_ptr->d_ptr->flags & FLAG_START_SAVE)
	{
		/* Loop over cards in deck */
		for (i = 0; i < g->deck_size; i++)
		{
			/* Check for saved card */
			if (g->deck[i].where == WHERE_SAVED)
			{
				/* Move to new owner */
				move_card(g, i, who, WHERE_SAVED);
			}
		}
	}

	/* Award settle bonus */
	settle_bonus(g, who, world, 1, simulated);

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Successful takeover */
	return 1;
}

/*
 * Resolve all pending takeovers.
 */
void resolve_takeovers(game *g)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int i, j, k, n;
	int list[MAX_TAKEOVER], special[MAX_TAKEOVER], num;
	char msg[1024];

	/* Check for no takeovers to resolve */
	if (!g->num_takeover) return;

	/* Copy takeover targets and powers */
	for (i = 0; i < g->num_takeover; i++)
	{
		/* Copy target and power */
		list[i] = g->takeover_target[i];
		special[i] = g->takeover_power[i];
	}

	/* Copy number of takeovers */
	num = g->num_takeover;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip power if player has no prestige */
		if (!p_ptr->prestige) continue;

		/* Get settle powers */
		n = get_powers(g, i, PHASE_SETTLE, w_list);

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Get card pointer */
			c_ptr = &g->deck[w_list[j].c_idx];

			/* Skip powers that don't prevent takeovers */
			if (!(o_ptr->code & P3_PREVENT_TAKEOVER)) continue;

			/* Mark power as used */
			c_ptr->used |= 1 << w_list[j].o_idx;

			/* Ask player which takeover (if any) to defeat */
			ask_player(g, i, CHOICE_TAKEOVER_PREVENT,
				   list, &num, special, &num, 0, 0, 0);

			/* Check for aborted game */
			if (g->game_over) return;

			/* Check for no answer */
			if (!num) continue;

			/* Spend prestige */
			spend_prestige(g, i, 1);

			/* Look for target world chosen */
			for (k = 0; k < g->num_takeover; k++)
			{
				/* Check for match */
				if (list[0] == g->takeover_target[k])
				{
					/* Mark takeover as defeated */
					g->takeover_defeated[k] = 1;
				}
			}

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s spends prestige to defeat "
				        "takeover of %s.\n",
				        p_ptr->name, g->deck[list[0]].d_ptr->name);

				/* Send message */
				message_add_formatted(g, msg, FORMAT_TAKEOVER);
			}
		}
	}

	/* Loop over takeovers */
	for (i = 0; i < g->num_takeover; i++)
	{
		/* Resolve takeover */
		if (!resolve_takeover(g, g->takeover_who[i],
		                         g->takeover_target[i],
		                         g->takeover_power[i],
		                         g->takeover_defeated[i],
		                         0))
		{
			/* Fail future declarations if one fails */
			for (j = i + 1; j < g->num_takeover; j++)
			{
				/* Check for same player */
				if (g->takeover_who[i] == g->takeover_who[j])
				{
					/* Defeat takeover */
					g->takeover_defeated[j] = 1;

					/* Message */
					if (!g->simulation)
					{
						/* Format message */
						sprintf(msg, "Takeover of %s is defeated because "
						        "takeover of %s failed.\n",
						        g->deck[list[j]].d_ptr->name,
						        g->deck[list[i]].d_ptr->name);

						/* Send message */
						message_add_formatted(g, msg, FORMAT_TAKEOVER);
					}
				}
			}
		}
	}

	/* Clear takeovers */
	g->num_takeover = 0;
}

/*
 * Handle the Settle Phase.
 */
void phase_settle(game *g)
{
	player *p_ptr;
	card *c_ptr;
	char msg[1024];
	int list[MAX_DECK];
	int i, x, n;
	int asked[MAX_PLAYER];

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current player */
		g->turn = i;

		/* Clear placing selection */
		p_ptr->placing = -1;

		/* Assume not asked */
		asked[i] = 0;

		/* Check for prestige settle */
		if (player_chose(g, i, ACT_PRESTIGE | g->cur_action))
		{
			/* Add bonus military for this phase */
			p_ptr->bonus_military += 2;
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Ask simulated opponents */
		if (g->simulation && g->sim_who != i)
		{
			/* Give no choices */
			n = 0;

			/* Ask AI to simulate opponent's choice */
			send_choice(g, i, CHOICE_PLACE, list, &n, NULL, NULL,
			            PHASE_SETTLE, -1, 0);

			/* Player was asked */
			asked[i] = 1;

			/* Next player */
			continue;
		}

		/* Assume no cards to play */
		n = 0;

		/* Start at first card in hand */
		x = g->p[i].head[WHERE_HAND];

		/* Loop over cards in hand */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip developments */
			if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

			/* Skip cards that cannot be settled */
			if (!settle_legal(g, i, x, 0, 0)) continue;

			/* Add card to list */
			list[n++] = x;
		}

		/* Check for no choices */
		if (!n) continue;

		/* Ask player to choose */
		send_choice(g, i, CHOICE_PLACE, list, &n, NULL, NULL,
		            PHASE_SETTLE, -1, 0);

		/* Player was asked to place */
		asked[i] = 1;

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Wait for all responses */
	wait_for_all(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip players who were not asked */
		if (!asked[i])
		{
			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s does not place a world.\n",
				        p_ptr->name);

				/* Send message */
				message_add_formatted(g, msg, FORMAT_VERBOSE);
			}

			continue;
		}

		/* Get player's world to place */
		p_ptr->placing = extract_choice(g, i, CHOICE_PLACE, list, &n,
		                                NULL, NULL);

		/* Skip players who are not placing anything */
		if (p_ptr->placing == -1)
		{
			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s does not place a world.\n",
				        p_ptr->name);

				/* Send message */
				message_add_formatted(g, msg, FORMAT_VERBOSE);
			}

			continue;
		}

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s places %s.\n", p_ptr->name,
			        g->deck[p_ptr->placing].d_ptr->name);

			/* Send message */
			message_add(g, msg);
		}

		/* Place card */
		place_card(g, i, p_ptr->placing);
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for prepare function */
		if (p_ptr->control->prepare_phase)
		{
			/* Ask player to prepare answers for payment */
			p_ptr->control->prepare_phase(g, i, PHASE_SETTLE,
			                              p_ptr->placing);
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current player */
		g->turn = i;

		/* Check for no placement choice */
		if (p_ptr->placing == -1)
		{
			/* Ask player for takeover choice instead */
			settle_check_takeover(g, i, NULL, 1);

			/* Check for aborted game */
			if (g->game_over) return;
		}

		/* Handle choice */
		settle_action(g, i, p_ptr->placing);

		/* Check for aborted game */
		if (g->game_over) return;

		/* Clear placing choice */
		p_ptr->placing = -1;
	}

	/* Resolve takeovers */
	resolve_takeovers(g);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Clear any temp flags on cards */
	clear_temp(g);

	/* Check intermediate goals */
	check_goals(g);

	/* Check prestige leader */
	check_prestige(g);
}

/*
 * Pass a payment callback either to the develop or settle callback.
 */
int payment_callback(game *g, int who, int which, int list[], int num,
                     int special[], int num_special, int mil_only)
{
	card *c_ptr;

	/* Get pointer of card being played */
	c_ptr = &g->deck[which];

	/* Check for development */
	if (c_ptr->d_ptr->type == TYPE_DEVELOPMENT)
	{
		/* Use development callback */
		return devel_callback(g, who, which, list, num, special,
		                      num_special);
	}
	else
	{
		/* Use settle callback */
		return settle_callback(g, who, which, list, num, special,
		                       num_special, mil_only);
	}
}

int trade_value(game *g, int who, card *c_ptr, int type, int no_bonus)
{
	power_where w_list[100];
	power *o_ptr;
	int i, n, value;

	/* Value is equal to type */
	value = type;

	/* Get consume phase powers (including trade powers) */
	n = get_powers(g, who, PHASE_CONSUME, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for any type bonus */
		if (!no_bonus && (o_ptr->code & P4_TRADE_ANY))
		{
			/* Add bonus */
			value += o_ptr->value;
		}

		/* Check for matching specific type bonus */
		if (!no_bonus &&
		    ((type == GOOD_NOVELTY &&(o_ptr->code & P4_TRADE_NOVELTY))||
		    (type == GOOD_RARE && (o_ptr->code & P4_TRADE_RARE)) ||
		    (type == GOOD_GENE && (o_ptr->code & P4_TRADE_GENE)) ||
		    (type == GOOD_ALIEN && (o_ptr->code & P4_TRADE_ALIEN))))
		{
			/* Add bonus */
			value += o_ptr->value;
		}

		/* Check for Chromosome bonus and gene good */
		if (!no_bonus && (type == GOOD_GENE) &&
		    (o_ptr->code & P4_TRADE_BONUS_CHROMO))
		{
			/* Increase value */
			value += count_active_flags(g, who, FLAG_CHROMO);
		}
	}

	/* Loop over powers on card holding good */
	for (i = 0; i < c_ptr->d_ptr->num_power; i++)
	{
		/* Get power pointer */
		o_ptr = &c_ptr->d_ptr->powers[i];

		/* Skip non-consume power */
		if (o_ptr->phase != PHASE_CONSUME) continue;

		/* Check for "trade this" power */
		if (!no_bonus && (o_ptr->code & P4_TRADE_THIS))
		{
			/* Add bonus */
			value += o_ptr->value;
		}
	}

	/* Check for prestige trade chosen */
	if (player_chose(g, who, ACT_PRESTIGE | ACT_CONSUME_TRADE)) value += 3;

	/* Return computed value */
	return value;
}


/*
 * Called when player has chosen which good to trade.
 */
void trade_chosen(game *g, int who, int which, int no_bonus)
{
	player *p_ptr;
	card *c_ptr;
	int type, value;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Move good card to discard */
	discard_card(g, who, c_ptr->covered);

	/* Uncover production card */
	c_ptr->covered = -1;

	/* Get good type */
	type = c_ptr->d_ptr->good_type;

	/* Check for "any" type */
	if (type == GOOD_ANY)
	{
		/* Ask player what kind this is */
		type = ask_player(g, who, CHOICE_OORT_KIND, NULL, NULL, NULL,
		                  NULL, 0, 0, 0);

		/* Check for aborted game */
		if (g->game_over) return;

		/* Set kind */
		g->oort_kind = type;

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s changes Alien Oort Cloud Refinery's "
			        "kind to %s.\n", p_ptr->name, good_printable[type]);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);
		}

		/* Check goal loss */
		check_goal_loss(g, who, GOAL_MOST_BLUE_BROWN);

		/* Reset kind to any */
		g->oort_kind = GOOD_ANY;
	}

	/* Compute trade value */
	value = trade_value(g, who, c_ptr, type, no_bonus);

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s trades good from %s for %d.\n", p_ptr->name,
		        c_ptr->d_ptr->name, value);

		/* Send message */
		message_add(g, msg);
	}

	/* Draw cards */
	draw_cards(g, who, value, NULL);

	/* Count reward */
	p_ptr->phase_cards += value;
}

/*
 * Handle a Trade action.
 *
 * This can occur when choosing the Consume-Trade role, or via some special
 * Consume phase powers.
 */
void trade_action(game *g, int who, int no_bonus, int phase_bonus)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr;
	int list[MAX_DECK], n = 0;
	int i, x, trade;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip cards without a good */
		if (c_ptr->covered == -1) continue;

		/* Assume good is available for trade */
		trade = 1;

		/* Loop over card powers */
		for (i = 0; i < c_ptr->d_ptr->num_power; i++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[i];

			/* Skip non-consume powers */
			if (o_ptr->phase != PHASE_CONSUME) continue;

			/* Check for "no trade" power */
			if (o_ptr->code & P4_NO_TRADE) trade = 0;
		}

		/* Check for unavailable good */
		if (!trade && phase_bonus) continue;

		/* Add card to list */
		list[n++] = x;
	}

	/* Check for no goods to trade */
	if (!n) return;

	/* Ask player to choose good to trade */
	ask_player(g, who, CHOICE_TRADE, list, &n, NULL, NULL, no_bonus, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Trade good */
	trade_chosen(g, who, list[0], no_bonus);
}

/*
 * Summarize rewards for a player and send message to game log.
 */
static void log_rewards(game *g, int who, int cards, int vps, int prestige,
                        char *adj, char *reason, char *tag)
{
	player *p_ptr = &g->p[who];
	char msg[1024], text[1024];

	/* Check for earned rewards */
	if (cards || vps || prestige)
	{
		/* Begin message */
		sprintf(msg, "%s receives ", p_ptr->name);

		/* Check for cards received */
		if (cards)
		{
			/* Create card string */
			sprintf(text, "%d card%s ", cards, PLURAL(cards));

			/* Add text to message */
			strcat(msg, text);

			/* Check for more rewards */
			if (vps || prestige)
			{
				/* Add conjunction */
				strcat(msg, "and ");
			}
		}

		/* Check for VP received */
		if (vps)
		{
			/* Create VP string */
			sprintf(text, "%d VP%s ", vps, PLURAL(vps));

			/* Add text to message */
			strcat(msg, text);

			/* Check for more rewards */
			if (prestige)
			{
				/* Add conjunction */
				strcat(msg, "and ");
			}
		}

		/* Check for prestige received */
		if (prestige)
		{
			/* Create prestige string */
			sprintf(text, "%d prestige ", prestige);

			/* Add text to message */
			strcat(msg, text);
		}

		/* Add conclusion */
		sprintf(text, "%s %s.\n", adj, reason);

		/* Add text to message */
		strcat(msg, text);

		/* Send message */
		message_add_formatted(g, msg, tag);
	}
}

/*
 * Called when a player has chosen goods to consume.
 */
int good_chosen(game *g, int who, int c_idx, int o_idx, int g_list[], int num)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr;
	char *name;
	int i, types[6], num_types, times, vp_mult, vps, cards, prestige;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get pointer to card holding power used */
	c_ptr = &g->deck[c_idx];

	/* Get power pointer */
	o_ptr = &c_ptr->d_ptr->powers[o_idx];

	/* Get name of card with power */
	name = g->deck[c_idx].d_ptr->name;

	/* Check for consume phase power used */
	if (o_ptr->phase == PHASE_CONSUME)
	{
		/* Check for needing two goods */
		if (o_ptr->code & P4_CONSUME_TWO)
		{
			/* Check for not two */
			if (num != 2) return 0;
		}

		/* Check for needing three */
		else if (o_ptr->code & P4_CONSUME_3_DIFF)
		{
			/* Check for not zero or three */
			if (num != 0 && num != 3) return 0;
		}

		/* Check for needing all goods */
		else if (o_ptr->code & P4_CONSUME_ALL)
		{
			/* XXX Check for all goods */
		}

		/* Check for needing 1-4 goods */
		else if (o_ptr->code & P4_CONSUME_N_DIFF)
		{
		}

		/* Otherwise check for too many */
		else if (num > o_ptr->times) return 0;

		/* Check for three different types needed */
		if (o_ptr->code & P4_CONSUME_3_DIFF)
		{
			/* Clear type counts */
			for (i = 0; i < 6; i++) types[i] = 0;

			/* Assume zero types */
			num_types = 0;

			/* Loop over goods */
			for (i = 0; i < num; i++)
			{
				/* Get card pointer */
				c_ptr = &g->deck[g_list[i]];

				/* Count good type */
				types[c_ptr->d_ptr->good_type]++;
			}

			/* Count good types */
			for (i = 0; i < 6; i++)
			{
				/* Check for type given */
				if (types[i]) num_types++;
			}

			/* Check for not three */
			if (num_types != 3) return 0;
		}

		/* Check for different types needed */
		if (o_ptr->code & P4_CONSUME_N_DIFF)
		{
			/* Clear type counts */
			for (i = 0; i < 6; i++) types[i] = 0;

			/* Assume zero types */
			num_types = 0;

			/* Loop over goods */
			for (i = 0; i < num; i++)
			{
				/* Get card pointer */
				c_ptr = &g->deck[g_list[i]];

				/* Count good type */
				types[c_ptr->d_ptr->good_type]++;
			}

			/* Count good types */
			for (i = 0; i < 6; i++)
			{
				/* Check for type given */
				if (types[i]) num_types++;
			}

			/* Check for duplicate types */
			if (num_types < num) return 0;
		}
	}

	/* Consume goods */
	for (i = 0; i < num; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[g_list[i]];

		if (c_ptr->covered == -1)
		{
			display_error("Passed card without good!\n");
			exit(1);
		}

		/* Move good card to discard */
		discard_card(g, who, c_ptr->covered);

		/* Uncover production card */
		c_ptr->covered = -1;

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s consumes good from %s using %s.\n",
			        p_ptr->name, c_ptr->d_ptr->name, name);

			/* Send message */
			message_add(g, msg);
		}
	}

	/* No rewards from non-consume phase powers */
	if (o_ptr->phase != PHASE_CONSUME) return 1;

	/* Compute number of times award is given */
	times = o_ptr->times;

	/* Check for fewer goods given than max */
	if (num < times) times = num;

	/* When consuming all, give award of one less than goods given */
	if (o_ptr->code & P4_CONSUME_ALL) times = num - 1;

	/* When consuming different types, give award once per good */
	if (o_ptr->code & P4_CONSUME_N_DIFF) times = num;

	/* Reset counters */
	cards = vps = prestige = 0;

	/* Check for VP award */
	if (o_ptr->code & P4_GET_VP)
	{
		/* Base multiplier */
		vp_mult = 1;

		/* Check for double VP action */
		if (player_chose(g, who, ACT_CONSUME_X2) ||
		    player_chose(g, who, ACT_CONSUME_TRADE | ACT_PRESTIGE))
		{
			/* Multiplier is two */
			vp_mult = 2;
		}

		/* Check for triple VP action */
		if (player_chose(g, who, ACT_PRESTIGE | ACT_CONSUME_X2))
		{
			/* Multiplier is three */
			vp_mult = 3;
		}

		/* Remember VPs */
		vps = o_ptr->value * times * vp_mult;
	}

	/* Check for card award */
	if (o_ptr->code & P4_GET_CARD)
	{
		/* Remember cards */
		cards += o_ptr->value * times;
	}

	/* XXX Check for multiple card award */
	if (o_ptr->code & P4_GET_2_CARD)
	{
		/* Remember cards */
		cards += o_ptr->value * 2 * times;
	}

	/* Check for prestige award */
	if (o_ptr->code & P4_GET_PRESTIGE)
	{
		/* Remember prestige */
		prestige = o_ptr->value * times;
	}

	/* Check for simulated game */
	if (!g->simulation)
	{
		/* Log rewards */
		log_rewards(g, who, cards, vps, prestige,
		            "from", name, FORMAT_VERBOSE);
	}

	/* Check for any VPs awarded */
	if (vps > 0)
	{
		/* Award VPs */
		gain_vps(g, who, vps, NULL);

		/* Count reward */
		p_ptr->phase_vp += vps;
	}

	/* Check for any cards awarded */
	if (cards > 0)
	{
		/* Award cards */
		draw_cards(g, who, cards, NULL);

		/* Count reward */
		p_ptr->phase_cards += cards;
	}

	/* Check for prestige earned */
	if (prestige > 0)
	{
		/* Award prestige */
		gain_prestige(g, who, prestige, NULL);

		/* Count reward */
		p_ptr->phase_prestige += prestige;
	}

	/* Success */
	return 1;
}

/*
 * Ask player to choose a number and check for a match.
 */
static void draw_lucky(game *g, int who)
{
	player *p_ptr;
	card *c_ptr;
	char msg[1024];
	int cost, which, emptied;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Ask player to choose lucky number */
	cost = ask_player(g, who, CHOICE_LUCKY, NULL, NULL, NULL, NULL,
	                  0, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Draw top card */
	which = random_draw(g, who, &emptied);

	/* Check for failure */
	if (which == -1) return;

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s guesses %d.\n", p_ptr->name, cost);

		/* Add message */
		message_add(g, msg);

		/* Format message */
		sprintf(msg, "%s draws %s (cost %d).\n", p_ptr->name,
		                                         c_ptr->d_ptr->name,
		                                         c_ptr->d_ptr->cost);

		/* Add message */
		message_add(g, msg);
	}

	/* Refresh the deck if it became empty */
	if (emptied) refresh_draw(g, who);

	/* Check for correct guess */
	if (cost == c_ptr->d_ptr->cost)
	{
		/* Move card to player */
		move_card(g, which, who, WHERE_HAND);

		/* Make card known to player */
		c_ptr->known = 1 << who;
	}
	else
	{
		/* Move card to discard */
		discard_card(g, who, which);

		/* Make card known to everyone */
		c_ptr->known = ~0;
	}
}

/*
 * Ask player to choose a card to ante, and draw cards and possibly reward
 * one to the player.
 */
static void ante_card(game *g, int who)
{
	player *p_ptr;
	card *c_ptr;
	int drawn[MAX_DECK];
	char msg[1024];
	int list[MAX_DECK], n = 0;
	int i, x, chosen, emptied;
	int cost, success = 0;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Start at first card */
	x = p_ptr->head[WHERE_HAND];

	/* Loop over player's cards in hand */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip cards that are too cheap */
		if (c_ptr->d_ptr->cost < 1) continue;

		/* Skip cards that are too expensive */
		if (c_ptr->d_ptr->cost > 6) continue;

		/* Add card to list */
		list[n++] = x;
	}

	/* Check for no cards available to ante */
	if (!n) return;

	/* Ask player to choose ante */
	chosen = ask_player(g, who, CHOICE_ANTE, list, &n, NULL, NULL, 0, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Check for no card chosen */
	if (chosen < 0) return;

	/* Get card pointer */
	c_ptr = &g->deck[chosen];

	/* Get card cost */
	cost = c_ptr->d_ptr->cost;

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s antes %s.\n", p_ptr->name, c_ptr->d_ptr->name);

		/* Add message */
		message_add(g, msg);
	}

	/* Draw cards */
	for (i = 0; i < cost; i++)
	{
		/* Take a card */
		drawn[i] = random_draw(g, who, &emptied);

		/* Check for failure */
		if (drawn[i] == -1) return;

		/* Check for more expensive than ante */
		if (g->deck[drawn[i]].d_ptr->cost > cost) success = 1;

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s draws %s.\n", p_ptr->name,
			        g->deck[drawn[i]].d_ptr->name);

			/* Add message */
			message_add(g, msg);
		}

		/* Refresh the deck if it became empty */
		if (emptied) refresh_draw(g, who);
	}

	/* Check for failure */
	if (!success)
	{
		/* Discard ante */
		discard_card(g, who, chosen);

		/* Location is known to all */
		g->deck[chosen].known = ~0;

		/* Loop over drawn cards */
		for (i = 0; i < cost; i++)
		{
			/* Discard drawn card */
			discard_card(g, who, drawn[i]);

			/* Location is known to all */
			g->deck[drawn[i]].known = ~0;
		}

		/* Done */
		return;
	}

	/* Clear list */
	n = 0;

	/* Loop over cards drawn */
	for (i = 0; i < cost; i++)
	{
		/* Add drawn card to list */
		list[n++] = drawn[i];
	}

	/* Ask player which card to keep */
	chosen = ask_player(g, who, CHOICE_KEEP, list, &n, NULL, NULL, 0, 0, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s keeps %s.\n", p_ptr->name,
			g->deck[chosen].d_ptr->name);

		/* Add message */
		message_add(g, msg);
	}

	/* Loop over cards drawn */
	for (i = 0; i < cost; i++)
	{
		/* Check for chosen card */
		if (drawn[i] == chosen)
		{
			/* Give card to player */
			move_card(g, chosen, who, WHERE_HAND);

			/* Make card known to player */
			g->deck[drawn[i]].known = 1 << who;
		}
		else
		{
			/* Discard card */
			discard_card(g, who, drawn[i]);

			/* Location is known to all */
			g->deck[drawn[i]].known = ~0;
		}
	}
}

/*
 * Called when player has chosen cards in hand to consume.
 */
int consume_hand_chosen(game *g, int who, int c_idx, int o_idx,
                        int list[], int n)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr, prestige_bonus;
	int i, vps, cards, prestige;
	char msg[1024], *power_name;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* XXX Check for prestige trade power */
	if (c_idx < 0)
	{
		/* Make fake power */
		prestige_bonus.phase = PHASE_CONSUME;
		prestige_bonus.code = P4_DISCARD_HAND | P4_GET_VP;
		prestige_bonus.value = 1;
		prestige_bonus.times = 2;

		/* Use bonus name */
		power_name = "his Prestige Trade bonus";

		/* Use fake power */
		o_ptr = &prestige_bonus;
	}
	else
	{
		/* Get pointer to card holding power */
		c_ptr = &g->deck[c_idx];

		/* Use card name */
		power_name = c_ptr->d_ptr->name;

		/* Get pointer to power used */
		o_ptr = &c_ptr->d_ptr->powers[o_idx];
	}

	/* Check for two cards needed */
	if (o_ptr->code & P4_CONSUME_TWO)
	{
		/* Check for not zero or two cards given */
		if (n != 0 && n != 2) return 0;
	}
	else
	{
		/* Check for too many discards */
		if (n > o_ptr->times) return 0;
	}

	/* Message */
	if (!g->simulation)
	{
		/* Private message */
		if (g->p[who].control->private_message)
		{
			/* Loop over choices */
			for (i = 0; i < n; i++)
			{
				/* Format message */
				sprintf(msg, "%s discards %s.\n", p_ptr->name,
				        g->deck[list[i]].d_ptr->name);

				/* Send message */
				g->p[who].control->private_message(g, who, msg, FORMAT_DISCARD);
			}
		}

		/* Format message */
		sprintf(msg, "%s consumes %d card%s from hand using %s.\n",
		        p_ptr->name, n, PLURAL(n), power_name);

		/* Send message */
		message_add(g, msg);
	}

	/* Reset counters */
	vps = cards = prestige = 0;

	/* Loop over choices */
	for (i = 0; i < n; i++)
	{
		/* Move card to discard */
		discard_card(g, who, list[i]);

		/* Check for reward per two cards */
		if ((o_ptr->code & P4_CONSUME_TWO) && (i % 2 != 0)) continue;

		/* Give VP rewards */
		if (o_ptr->code & P4_GET_VP)
		{
			/* Remember VPs */
			vps += o_ptr->value;
		}

		/* Give card rewards */
		if (o_ptr->code & P4_GET_CARD)
		{
			/* Remember cards */
			cards += o_ptr->value;
		}

		/* Give prestige rewards */
		if (o_ptr->code & P4_GET_PRESTIGE)
		{
			/* Remember prestige */
			prestige += o_ptr->value;
		}
	}

	/* Check for simulated game */
	if (!g->simulation)
	{
		/* Log rewards */
		log_rewards(g, who, cards, vps, prestige,
		            "from", power_name, FORMAT_VERBOSE);
	}

	/* Check for any VPs awarded */
	if (vps > 0)
	{
		/* Award VPs */
		gain_vps(g, who, vps, NULL);

		/* Count reward */
		p_ptr->phase_vp += vps;
	}

	/* Check for any cards awarded */
	if (cards > 0)
	{
		/* Draw cards */
		draw_cards(g, who, cards, NULL);

		/* Count reward */
		p_ptr->phase_cards += cards;
	}

	/* Check for prestige earned */
	if (prestige > 0)
	{
		/* Award prestige */
		gain_prestige(g, who, prestige, NULL);

		/* Count reward */
		p_ptr->phase_prestige += prestige;
	}

	/* Success */
	return 1;
}

/*
 * Ask player to discard cards from hand for VPs or cards.
 */
static void consume_discard(game *g, int who, int c_idx, int o_idx)
{
	int list[MAX_DECK], n = 0;

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Ask player to choose discards */
	ask_player(g, who, CHOICE_CONSUME_HAND, list, &n, NULL, NULL,
	           c_idx, o_idx, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Consume */
	consume_hand_chosen(g, who, c_idx, o_idx, list, n);
}

/*
 * Called when a player has chosen to consume prestige for a reward.
 */
void consume_prestige_chosen(game *g, int who, int c_idx, int o_idx)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr;
	int vp_mult, vps, cards;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card holding power */
	c_ptr = &g->deck[c_idx];

	/* Get power pointer */
	o_ptr = &c_ptr->d_ptr->powers[o_idx];

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s consumes prestige using %s.\n",
		        p_ptr->name, c_ptr->d_ptr->name);

		/* Send message */
		message_add(g, msg);
	}

	/* Spend prestige */
	spend_prestige(g, who, 1);

	/* Reset counters */
	cards = vps = 0;

	/* Give card reward */
	if (o_ptr->code & P4_GET_CARD)
	{
		/* Remember cards */
		cards += o_ptr->value;
	}

	/* Give VP rewards */
	if (o_ptr->code & P4_GET_VP)
	{
		/* Base reward */
		vps = o_ptr->value;

		/* Base multiplier */
		vp_mult = 1;

		/* Check for double VP action */
		if (player_chose(g, who, ACT_CONSUME_X2) ||
		    player_chose(g, who, ACT_CONSUME_TRADE | ACT_PRESTIGE))
		{
			/* Multiplier is two */
			vp_mult = 2;
		}

		/* Check for triple VP action */
		if (player_chose(g, who, ACT_PRESTIGE | ACT_CONSUME_X2))
		{
			/* Multiplier is three */
			vp_mult = 3;
		}

		/* Save VPs */
		vps *= vp_mult;
	}

	/* Check for simulated game */
	if (!g->simulation)
	{
		/* Log rewards */
		log_rewards(g, who, cards, vps, 0,
		            "from", c_ptr->d_ptr->name, FORMAT_VERBOSE);
	}

	/* Check for any cards awarded */
	if (cards > 0)
	{
		/* Award cards */
		draw_cards(g, who, o_ptr->value, NULL);

		/* Remember reward */
		p_ptr->phase_cards += o_ptr->value;
	}

	/* Check for any VPs awarded */
	if (vps > 0)
	{
		/* Award VPs */
		gain_vps(g, who, vps, NULL);

		/* Count reward */
		p_ptr->phase_vp += vps;
	}
}

/*
 * Called when a player has chosen a consume power.
 */
void consume_chosen(game *g, int who, int c_idx, int o_idx)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr;
	char *name;
	int i, x, min, max, vp, vp_mult;
	int types[6], num_types = 0;
	int good, g_list[MAX_DECK], num_goods = 0;
	int special[MAX_DECK], num_special = 2;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* XXX Check for prestige-trade chosen */
	if (c_idx < 0)
	{
		/* Consume cards from hand */
		consume_discard(g, who, -1, -1);

		/* Power used */
		p_ptr->phase_bonus_used = 1;

		/* Done */
		return;
	}

	/* Get card of chosen power */
	c_ptr = &g->deck[c_idx];

	/* Get name of card with power */
	name = c_ptr->d_ptr->name;

	/* Mark power as used */
	c_ptr->used |= 1 << o_idx;

	/* Get pointer to power */
	o_ptr = &c_ptr->d_ptr->powers[o_idx];

	/* Check for trade action power */
	if (o_ptr->code & P4_TRADE_ACTION)
	{
		/* Perform trade action */
		trade_action(g, who, o_ptr->code & P4_TRADE_NO_BONUS, 0);

		/* Done */
		return;
	}

	/* Check for draw a card */
	if (o_ptr->code & P4_DRAW)
	{
		/* Draw cards */
		draw_cards(g, who, o_ptr->value, name);

		/* Count reward */
		p_ptr->phase_cards += o_ptr->value;

		/* Done */
		return;
	}

	/* Check for "draw if lucky" */
	if (o_ptr->code & P4_DRAW_LUCKY)
	{
		/* Perform lucky draw */
		draw_lucky(g, who);

		/* Done */
		return;
	}

	/* Check for "ante card for card" */
	if (o_ptr->code & P4_ANTE_CARD)
	{
		/* Ask player to ante */
		ante_card(g, who);

		/* Done */
		return;
	}

	/* Check for "VP" */
	if (o_ptr->code & P4_VP)
	{
		/* Base VP */
		vp = o_ptr->value;

		/* Base multiplier */
		vp_mult = 1;

		/* Check for double VP action */
		if (player_chose(g, who, ACT_CONSUME_X2) ||
		    player_chose(g, who, ACT_CONSUME_TRADE | ACT_PRESTIGE))
		{
			/* Multiplier is two */
			vp_mult = 2;
		}

		/* Check for triple VP action */
		if (player_chose(g, who, ACT_PRESTIGE | ACT_CONSUME_X2))
		{
			/* Multiplier is three */
			vp_mult = 3;
		}

		/* Award VPs */
		gain_vps(g, who, vp * vp_mult, name);

		/* Count reward */
		p_ptr->phase_vp += vp * vp_mult;

		/* Done */
		return;
	}

	/* Check for discard from hand */
	if (o_ptr->code & P4_DISCARD_HAND)
	{
		/* Choose discards for points/cards */
		consume_discard(g, who, c_idx, o_idx);

		/* Done */
		return;
	}

	/* Check for consume prestige */
	if (o_ptr->code & P4_CONSUME_PRESTIGE)
	{
		/* Consume prestige for reward */
		consume_prestige_chosen(g, who, c_idx, o_idx);

		/* Done */
		return;
	}

	/* Clear good type counts */
	for (i = 0; i < 6; i++) types[i] = 0;

	/* Start at first active card */
	x = g->p[who].head[WHERE_ACTIVE];

	/* Loop over cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip cards without goods */
		if (c_ptr->covered == -1) continue;

		/* Get good type */
		good = c_ptr->d_ptr->good_type;

		/* Count good type */
		types[good]++;

		/* Check for specific good type needed */
		if (good != GOOD_ANY &&
		   (((o_ptr->code & P4_CONSUME_NOVELTY)&&good != GOOD_NOVELTY)||
		    ((o_ptr->code & P4_CONSUME_RARE) && good != GOOD_RARE) ||
		    ((o_ptr->code & P4_CONSUME_GENE) && good != GOOD_GENE) ||
		    ((o_ptr->code & P4_CONSUME_ALIEN) && good != GOOD_ALIEN)))
		{
			/* Skip good */
			continue;
		}

		/* Check for specific good needed */
		if ((o_ptr->code & P4_CONSUME_THIS) && c_idx != x)
		{
			/* Skip good */
			continue;
		}

		/* Add good (world) to list */
		g_list[num_goods++] = x;
	}

	/* Count number of types */
	for (i = 0; i < 6; i++) if (types[i]) num_types++;

	/* Compute number of goods needed */
	if (o_ptr->code & P4_CONSUME_TWO)
	{
		/* Exactly two goods needed */
		min = max = 2;
	}
	else if (o_ptr->code & P4_CONSUME_3_DIFF)
	{
		/* Exactly three goods needed */
		min = max = 3;

		/* Check for "any" type and exactly 2 others */
		if (types[GOOD_ANY] && num_types == 3)
		{
			/* Power is optional */
			min = 0;
		}
	}
	else if (o_ptr->code & P4_CONSUME_N_DIFF)
	{
		/* One of each type needed */
		min = max = num_types;

		/* Do not consume more than 4 */
		if (num_types > 4) min = max = 4;

		/* Check for "any" type available */
		if (types[GOOD_ANY])
		{
			/* Reduce minimum (if not already 1) */
			if (min > 1) min--;
		}
	}
	else if (o_ptr->code & P4_CONSUME_ALL)
	{
		/* All goods needed */
		min = max = num_goods;
	}
	else
	{
		/* Use power a number of times */
		min = max = o_ptr->times;
	}

	/* Check for fewer goods available */
	if (min > num_goods)
	{
		/* Use only what is available */
		min = num_goods;
	}

	/* Check for fewer goods available */
	if (max > num_goods)
	{
		/* Use only what is available */
		max = num_goods;
	}

	/* Check for "any" type good available and specific consume power */
	if (types[GOOD_ANY] &&
	    !(o_ptr->code & P4_CONSUME_TWO) &&
	    (o_ptr->code & (P4_CONSUME_NOVELTY | P4_CONSUME_RARE |
	                    P4_CONSUME_GENE | P4_CONSUME_ALIEN)))
	{
		/* Check for less than maximum number */
		if (num_goods <= o_ptr->times)
		{
			/* Reduce minimum number needed */
			min--;
		}
	}

	/* XXX Put power card/index in special array */
	special[0] = c_idx;
	special[1] = o_idx;

	/* Ask player which good(s) to consume */
	ask_player(g, who, CHOICE_GOOD, g_list, &num_goods,
	           special, &num_special, min, max, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Consume chosen good(s) */
	good_chosen(g, who, c_idx, o_idx, g_list, num_goods);
}

/*
 * Ask the player to use a consume power.
 *
 * We return 0 if there are no powers to be used, and 1 otherwise.
 */
int consume_action(game *g, int who)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100], *w_ptr;
	power *o_ptr;
	int cidx[MAX_DECK], oidx[MAX_DECK];
	int goods = 0, types[6], num_types = 0;
	int i, x, need, n, num = 0;
	int optional = 1;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Clear good type counts */
	for (i = 0; i < 6; i++) types[i] = 0;

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Look for available goods */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip cards without goods */
		if (c_ptr->covered == -1) continue;

		/* Count good */
		goods++;

		/* Count good type */
		types[c_ptr->d_ptr->good_type]++;
	}

	/* Count number of types */
	for (i = 0; i < 6; i++) if (types[i]) num_types++;

	/* Get consume powers */
	n = get_powers(g, who, PHASE_CONSUME, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power location pointer */
		w_ptr = &w_list[i];

		/* Get power pointer */
		o_ptr = w_ptr->o_ptr;

		/* Get card pointer */
		c_ptr = &g->deck[w_ptr->c_idx];

		/* Assume only one good needed */
		need = 1;

		/* Check for need two */
		if (o_ptr->code & P4_CONSUME_TWO) need = 2;

		/* Check for good on this world needed */
		if ((o_ptr->code & P4_CONSUME_THIS) &&
		    (c_ptr->covered == -1)) continue;

		/* Check for regular consume powers */
		if (((o_ptr->code & P4_CONSUME_ANY) && goods >= need) ||
		    ((o_ptr->code & P4_CONSUME_NOVELTY) &&
		       types[GOOD_NOVELTY] + types[GOOD_ANY] >= need) ||
		    ((o_ptr->code & P4_CONSUME_RARE) &&
		       types[GOOD_RARE] + types[GOOD_ANY] >= need) ||
		    ((o_ptr->code & P4_CONSUME_GENE) &&
		       types[GOOD_GENE] + types[GOOD_ANY] >= need) ||
		    ((o_ptr->code & P4_CONSUME_ALIEN) &&
		       types[GOOD_ALIEN] + types[GOOD_ANY] >= need))
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;

			/* Not optional */
			optional = 0;
		}

		/* Check for consume 3 types */
		if ((o_ptr->code & P4_CONSUME_3_DIFF) && num_types >= 3)
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;

			/* Not optional */
			optional = 0;
		}

		/* Check for consume different types */
		if ((o_ptr->code & P4_CONSUME_N_DIFF) && goods > 0)
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;

			/* Not optional */
			optional = 0;
		}

		/* Check for consume all */
		if ((o_ptr->code & P4_CONSUME_ALL) && goods > 0)
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;

			/* Not optional */
			optional = 0;
		}

		/* Check for consume prestige */
		if ((o_ptr->code & P4_CONSUME_PRESTIGE) && p_ptr->prestige > 0)
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;
		}

		/* Check for discard from hand */
		if ((o_ptr->code & P4_DISCARD_HAND) &&
		    count_player_area(g, who, WHERE_HAND) +
		    p_ptr->fake_hand - p_ptr->fake_discards >= need)
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;
		}

		/* Check for trade action power */
		if ((o_ptr->code & P4_TRADE_ACTION) && goods > 0)
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;

			/* Not optional */
			optional = 0;
		}

		/* Check for other powers */
		if (o_ptr->code & (P4_DRAW | P4_DRAW_LUCKY | P4_VP |
				   P4_ANTE_CARD))
		{
			/* Add power to list */
			cidx[num] = w_ptr->c_idx;
			oidx[num] = w_ptr->o_idx;
			num++;

			/* Not optional */
			optional = 0;
		}
	}

	/* Check for extra power given by Prestige Consume-Trade */
	if (player_chose(g, who, ACT_PRESTIGE | ACT_CONSUME_TRADE) &&
	    !p_ptr->phase_bonus_used &&
	    (count_player_area(g, who, WHERE_HAND) +
	     p_ptr->fake_hand - p_ptr->fake_discards >= 1))
	{
		/* Add power to list */
		cidx[num] = -1;
		oidx[num] = -1;
		num++;
	}

	/* Check for no usable powers */
	if (!num) return 0;

	/* Check for more than one power to use */
	if (num > 1 || (num == 1 && optional))
	{
		/* Ask player which power to use */
		ask_player(g, who, CHOICE_CONSUME, cidx, &num, oidx, &num,
		           optional, 0, 0);
	}

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Check for no power selected */
	if (optional && num == 0) return 0;

	/* Use chosen power */
	consume_chosen(g, who, cidx[0], oidx[0]);

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Successfully used power */
	return 1;
}

/*
 * Perform the Consume Phase for one player.
 */
void consume_player(game *g, int who)
{
	player *p_ptr = &g->p[who];

	/* Set turn */
	g->turn = who;

	/* Clear count of earned rewards */
	p_ptr->phase_cards = p_ptr->phase_vp = 0;
	p_ptr->phase_prestige = 0;

	/* Check for consume-trade action chosen */
	if (player_chose(g, who, ACT_CONSUME_TRADE))
	{
		/* First trade a good */
		trade_action(g, who, 0, 1);

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Use consume powers until none are usable, or game is aborted */
	while (consume_action(g, who));
}

/*
 * Handle the Consume Phase.
 */
void phase_consume(game *g)
{
	player *p_ptr;
	int i;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for prepare function */
		if (p_ptr->control->prepare_phase)
		{
			/* Ask player to prepare answers for consume phase */
			p_ptr->control->prepare_phase(g, i, PHASE_CONSUME, 0);
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Perform trade (if needed) then consume actions */
		consume_player(g, i);

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Check for simulated game */
	if (!g->simulation)
	{
		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Log rewards */
			log_rewards(g, i, p_ptr->phase_cards,
			            p_ptr->phase_vp, p_ptr->phase_prestige,
			            "for", "Consume phase", NULL);
		}
	}

	/* Clear any temp flags on cards */
	clear_temp(g);

	/* Check intermediate goals */
	check_goals(g);

	/* Check prestige leader */
	check_prestige(g);
}

/*
 * Produce a good on a world.
 */
void produce_world(game *g, int who, int which, int c_idx, int o_idx)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr, produce_bonus;
	char *name;
	int i;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Add good to card */
	add_good(g, c_ptr);

	/* Mark world as producing */
	c_ptr->produced = c_ptr->d_ptr->good_type;

	/* Check for "any" kind world */
	if (c_ptr->d_ptr->good_type == GOOD_ANY)
	{
		/* Check for no card providing produce power */
		if (c_idx < 0)
		{
			/* Create fake produce power */
			produce_bonus.phase = PHASE_PRODUCE;
			produce_bonus.code = P5_WINDFALL_ANY;
			o_ptr = &produce_bonus;
		}
		else
		{
			/* Get power used */
			o_ptr = &g->deck[c_idx].d_ptr->powers[o_idx];
		}

		/* Check for specific kind power used */
		if (o_ptr->code & P5_WINDFALL_NOVELTY)
			c_ptr->produced = GOOD_NOVELTY;
		if (o_ptr->code & P5_WINDFALL_RARE)
			c_ptr->produced = GOOD_RARE;
		if (o_ptr->code & P5_WINDFALL_GENE)
			c_ptr->produced = GOOD_GENE;
		if (o_ptr->code & P5_WINDFALL_ALIEN)
			c_ptr->produced = GOOD_ALIEN;

		/* Check for any kind available */
		if (o_ptr->code & P5_WINDFALL_ANY)
		{
			/* Ask player for kind of good produced */
			c_ptr->produced = ask_player(g, who, CHOICE_OORT_KIND,
			                             NULL, NULL, NULL, NULL,
			                             0, 0, 0);

			/* Check for aborted game */
			if (g->game_over) return;

			/* Set kind */
			g->oort_kind = c_ptr->produced;

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s changes Alien Oort Cloud Refinery's "
				        "kind to %s.\n",
				        p_ptr->name, good_printable[c_ptr->produced]);

				/* Send message */
				message_add_formatted(g, msg, FORMAT_VERBOSE);
			}

			/* Check goal loss */
			check_goal_loss(g, who, GOAL_MOST_BLUE_BROWN);

			/* Reset kind to any */
			g->oort_kind = GOOD_ANY;
		}
	}

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "%s produces on %s.\n", p_ptr->name,
		        c_ptr->d_ptr->name);

		/* Send message */
		message_add(g, msg);
	}

	/* Loop over card's powers */
	for (i = 0; i < c_ptr->d_ptr->num_power; i++)
	{
		/* Get power pointer */
		o_ptr = &c_ptr->d_ptr->powers[i];

		/* Get name of card with power */
		name = c_ptr->d_ptr->name;

		/* Skip non-produce powers */
		if (o_ptr->phase != PHASE_PRODUCE) continue;

		/* Check for "draw if produced here" power */
		if (o_ptr->code & P5_DRAW_IF)
		{
			/* Draw cards */
			draw_cards(g, who, o_ptr->value, name);

			/* Count reward */
			p_ptr->phase_cards += o_ptr->value;
		}

		/* Check for "prestige if produced here" power */
		if (o_ptr->code & P5_PRESTIGE_IF)
		{
			/* Gain prestige */
			gain_prestige(g, who, o_ptr->value, name);

			/* Count reward */
			p_ptr->phase_prestige += o_ptr->value;
		}
	}
}

/*
 * Called when a player has chosen a card to discard in order to produce
 * on a world.
 */
void discard_produce_chosen(game *g, int who, int world, int discard,
                            int c_idx, int o_idx)
{
	player *p_ptr;
	card *c_ptr;
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get card to discard */
	c_ptr = &g->deck[discard];

	/* Move card to discard */
	discard_card(g, who, discard);

	/* Message */
	if (!g->simulation)
	{
		/* Private message */
		if (g->p[who].control->private_message)
		{
			/* Format message */
			sprintf(msg, "%s discards %s.\n", p_ptr->name, c_ptr->d_ptr->name);

			/* Send message */
			g->p[who].control->private_message(g, who, msg, FORMAT_DISCARD);
		}

		/* Format message */
		sprintf(msg, "%s discards to produce.\n", p_ptr->name);

		/* Send message */
		message_add(g, msg);
	}

	/* Produce on world */
	produce_world(g, who, world, c_idx, o_idx);
}

/*
 * Ask the player to discard a card in order to produce on this world.
 */
static void discard_produce(game *g, int who, int world, int c_idx, int o_idx)
{
	int list[MAX_DECK], n = 0;
	int special[MAX_DECK], num_special = 0;

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Put world in special list */
	special[num_special++] = world;

	/* Ask player to choose discard */
	ask_player(g, who, CHOICE_DISCARD_PRODUCE, list, &n,
	           special, &num_special, c_idx, o_idx, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Check for discard chosen */
	if (n > 0) discard_produce_chosen(g, who, world, list[0], c_idx, o_idx);
}

/*
 * Ask the player to discard a card in order to produce on a windfall.
 */
static void discard_windfall(game *g, int who, int c_idx, int o_idx)
{
	card *c_ptr;
	power *o_ptr;
	int i;
	int list[MAX_DECK], n = 0;
	int special[MAX_DECK], num_special = 0;
	int good = 0;

	/* Get card holding power used */
	c_ptr = &g->deck[c_idx];

	/* Get power pointer */
	o_ptr = &c_ptr->d_ptr->powers[o_idx];

	/* Check for good type restriction */
	if (o_ptr->code & P5_WINDFALL_NOVELTY) good = GOOD_NOVELTY;
	if (o_ptr->code & P5_WINDFALL_RARE) good = GOOD_RARE;
	if (o_ptr->code & P5_WINDFALL_GENE) good = GOOD_GENE;
	if (o_ptr->code & P5_WINDFALL_ALIEN) good = GOOD_ALIEN;

	/* Loop over active cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip unowned cards */
		if (c_ptr->owner != who) continue;

		/* Skip inactive cards */
		if (c_ptr->where != WHERE_ACTIVE) continue;

		/* Skip non-windfall worlds */
		if (!(c_ptr->d_ptr->flags & FLAG_WINDFALL)) continue;

		/* Skip worlds that do not produce goods */
		if (!c_ptr->d_ptr->good_type) continue;

		/* Skip worlds of incorrect type */
		if (good && c_ptr->d_ptr->good_type != good &&
		    c_ptr->d_ptr->good_type != GOOD_ANY) continue;

		/* Skip worlds with goods already */
		if (c_ptr->covered != -1) continue;

		/* Add world to special list */
		special[num_special++] = i;
	}

	/* Get cards in hand */
	n = get_player_area(g, who, list, WHERE_HAND);

	/* Check for no cards to discard */
	if (!n) return;

	/* Ask player to choose discard */
	ask_player(g, who, CHOICE_DISCARD_PRODUCE, list, &n,
	           special, &num_special, c_idx, o_idx, 0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Check for discard chosen */
	if (n > 0) discard_produce_chosen(g, who, special[0], list[0],
	                                  c_idx, o_idx);
}

/*
 * Produce a good on an empty windfall world.
 */
static void produce_windfall(game *g, int who, int c_idx, int o_idx)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr = NULL;
	int list[MAX_DECK], n = 0;
	int good = 0;
	int x;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Check for card power used to produce */
	if (c_idx >= 0)
	{
		/* Get card holding power used */
		c_ptr = &g->deck[c_idx];

		/* Get power pointer */
		o_ptr = &c_ptr->d_ptr->powers[o_idx];
	}

	/* Check for power passed */
	if (o_ptr)
	{
		/* Check for good type restriction */
		if (o_ptr->code & P5_WINDFALL_NOVELTY) good = GOOD_NOVELTY;
		if (o_ptr->code & P5_WINDFALL_RARE) good = GOOD_RARE;
		if (o_ptr->code & P5_WINDFALL_GENE) good = GOOD_GENE;
		if (o_ptr->code & P5_WINDFALL_ALIEN) good = GOOD_ALIEN;
	}

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Loop over active cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip non-windfall worlds */
		if (!(c_ptr->d_ptr->flags & FLAG_WINDFALL)) continue;

		/* Skip worlds that do not produce goods */
		if (!c_ptr->d_ptr->good_type) continue;

		/* Skip worlds of incorrect type */
		if (good && c_ptr->d_ptr->good_type != good &&
		    c_ptr->d_ptr->good_type != GOOD_ANY) continue;

		/* Skip worlds with goods already */
		if (c_ptr->covered != -1) continue;

		/* Check for "not this world" modifier on power */
		if (o_ptr && (o_ptr->code & P5_NOT_THIS) && x == c_idx)
		{
			/* Skip world */
			continue;
		}

		/* Add to list */
		list[n++] = x;
	}

	/* Do nothing if no worlds available */
	if (!n) return;

	/* Produce automatically if only one world available */
	if (n == 1)
	{
		/* Produce */
		produce_world(g, who, list[0], c_idx, o_idx);

		/* Done */
		return;
	}

	/* Ask player to choose world to produce */
	ask_player(g, who, CHOICE_WINDFALL, list, &n, NULL, NULL, c_idx, o_idx,
	           0);

	/* Check for aborted game */
	if (g->game_over) return;

	/* Produce on chosen world */
	produce_world(g, who, list[0], c_idx, o_idx);
}

/*
 * Called when a player has chosen a produce power.
 */
void produce_chosen(game *g, int who, int c_idx, int o_idx)
{
	player *p_ptr;
	card *c_ptr;
	power *o_ptr;
	char *name;
	int i, x, count, produced[6];
	int list[MAX_DECK];
	char msg[1024];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Check for card power used */
	if (c_idx >= 0)
	{
		/* Get card holding power used */
		c_ptr = &g->deck[c_idx];

		/* Get power pointer */
		o_ptr = &c_ptr->d_ptr->powers[o_idx];
	}
	else
	{
		/* Phase bonus is used */
		p_ptr->phase_bonus_used |= 1 << (0 - c_idx);

		/* Determine which bonus to use */
		if (c_idx == -1 || c_idx == -2)
		{
			/* Produce on a windfall world */
			produce_windfall(g, who, -1, -1);
		}
		else
		{
			/* Draw 3 cards */
			draw_cards(g, who, 3, "his Prestige Produce bonus");

			/* Count reward */
			p_ptr->phase_cards += 3;
		}

		/* Done */
		return;
	}

	/* Mark power used */
	c_ptr->used |= 1 << o_idx;

	/* Get name of card with power */
	name = c_ptr->d_ptr->name;

	/* Check for regular produce */
	if (o_ptr->code == P5_PRODUCE)
	{
		/* Produce */
		produce_world(g, who, c_idx, c_idx, o_idx);

		/* Done */
		return;
	}

	/* Check for discard to produce */
	if (o_ptr->code == (P5_DISCARD | P5_PRODUCE))
	{
		/* Discard to produce */
		discard_produce(g, who, c_idx, c_idx, o_idx);

		/* Done */
		return;
	}

	/* Check for draw cards */
	if (o_ptr->code & P5_DRAW)
	{
		/* Draw cards */
		draw_cards(g, who, o_ptr->value, name);

		/* Count reward */
		p_ptr->phase_cards += o_ptr->value;

		/* Done */
		return;
	}

	/* Check for draw per gene world */
	if (o_ptr->code & P5_DRAW_WORLD_GENE)
	{
		/* Assume no gene worlds */
		count = 0;

		/* Start at first active card */
		x = p_ptr->head[WHERE_ACTIVE];

		/* Loop over cards */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Check for gene world */
			if (c_ptr->d_ptr->good_type == GOOD_GENE ||
			    c_ptr->d_ptr->good_type == GOOD_ANY) count++;
		}

		/* Draw cards */
		draw_cards(g, who, count * o_ptr->value, name);

		/* Count reward */
		p_ptr->phase_cards += count * o_ptr->value;

		/* Done */
		return;
	}

	/* Check for draw per military world */
	if (o_ptr->code & P5_DRAW_MILITARY)
	{
		/* Count military worlds */
		count = count_active_flags(g, who, FLAG_MILITARY);

		/* Draw cards */
		draw_cards(g, who, count * o_ptr->value, name);

		/* Count reward */
		p_ptr->phase_cards += count * o_ptr->value;

		/* Done */
		return;
	}

	/* Check for draw per rebel world */
	if (o_ptr->code & P5_DRAW_REBEL)
	{
		/* Assume no rebel worlds */
		count = 0;

		/* Start at first active card */
		x = p_ptr->head[WHERE_ACTIVE];

		/* Loop over cards */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip developments */
			if (c_ptr->d_ptr->type == TYPE_DEVELOPMENT) continue;

			/* Check for rebel world */
			if (c_ptr->d_ptr->flags & FLAG_REBEL) count++;
		}

		/* Draw cards */
		draw_cards(g, who, count * o_ptr->value, name);

		/* Count reward */
		p_ptr->phase_cards += count * o_ptr->value;

		/* Done */
		return;
	}

	/* Check for draw per chromosome card */
	if (o_ptr->code & P5_DRAW_CHROMO)
	{
		/* Count chromosome worlds */
		count = count_active_flags(g, who, FLAG_CHROMO);

		/* Draw cards */
		draw_cards(g, who, count * o_ptr->value, name);

		/* Count reward */
		p_ptr->phase_cards += count * o_ptr->value;

		/* Done */
		return;
	}

	/* Check for prestige if most chromo worlds */
	if (o_ptr->code & P5_PRESTIGE_MOST_CHROMO)
	{
		/* Loop over opponents */
		for (i = 0; i < g->num_players; i++)
		{
			/* Skip given player */
			if (i == who) continue;

			/* Check for more or as many chromo worlds */
			if (count_active_flags(g, i, FLAG_CHROMO) >=
			    count_active_flags(g, who, FLAG_CHROMO))
			{
				/* No reward */
				return;
			}
		}

		/* Award prestige */
		gain_prestige(g, who, o_ptr->value, name);

		/* Track rewards */
		p_ptr->phase_prestige += o_ptr->value;

		/* Done */
		return;
	}

	/* Check for take saved cards */
	if (o_ptr->code & P5_TAKE_SAVED)
	{
		/* Get saved cards */
		count = get_player_area(g, who, list, WHERE_SAVED);

		/* Loop over saved cards */
		for (i = 0; i < count; i++)
		{
			/* Move card */
			move_card(g, list[i], who, WHERE_HAND);

			/* Private message */
			if (g->p[who].control->private_message)
			{
				/* Format message */
				sprintf(msg, "%s takes %s.\n", p_ptr->name,
				        g->deck[list[i]].d_ptr->name);

				/* Send message */
				g->p[who].control->private_message(g, who, msg,
				                                   FORMAT_DISCARD);
			}
		}

		/* Message */
		if (count > 0 && !g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s takes %d card%s from under %s.\n",
			        p_ptr->name, count, PLURAL(count),
			        g->deck[c_idx].d_ptr->name);

			/* Send message */
			message_add(g, msg);
		}

		/* Done */
		return;
	}

	/* Check for discard to produce on windfall */
	if ((o_ptr->code & P5_DISCARD) &&
	    (o_ptr->code & (P5_WINDFALL_ANY | P5_WINDFALL_NOVELTY |
			   P5_WINDFALL_RARE | P5_WINDFALL_GENE |
			   P5_WINDFALL_ALIEN)))
	{
		/* Choose discard to produce */
		discard_windfall(g, who, c_idx, o_idx);

		/* Done */
		return;
	}

	/* Check for produce on windfall */
	if (o_ptr->code & (P5_WINDFALL_ANY | P5_WINDFALL_NOVELTY |
			   P5_WINDFALL_RARE | P5_WINDFALL_GENE |
			   P5_WINDFALL_ALIEN))
	{
		/* Produce on a windfall world */
		produce_windfall(g, who, c_idx, o_idx);

		/* Done */
		return;
	}

	/* Clear production counts */
	for (i = 0; i < 6; i++) produced[i] = 0;

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Loop over active cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip cards that did not produce */
		if (!c_ptr->produced) continue;

		/* Count types */
		produced[c_ptr->produced]++;
	}

	/* Check for draw per novelty produced */
	if (o_ptr->code & P5_DRAW_EACH_NOVELTY)
	{
		/* Award cards */
		draw_cards(g, who, produced[GOOD_NOVELTY], name);

		/* Count reward */
		p_ptr->phase_cards += produced[GOOD_NOVELTY];

		/* Done */
		return;
	}

	/* Check for draw per rare produced */
	if (o_ptr->code & P5_DRAW_EACH_RARE)
	{
		/* Award cards */
		draw_cards(g, who, produced[GOOD_RARE], name);

		/* Count reward */
		p_ptr->phase_cards += produced[GOOD_RARE];

		/* Done */
		return;
	}

	/* Check for draw per gene produced */
	if (o_ptr->code & P5_DRAW_EACH_GENE)
	{
		/* Award cards */
		draw_cards(g, who, produced[GOOD_GENE], name);

		/* Count reward */
		p_ptr->phase_cards += produced[GOOD_GENE];

		/* Done */
		return;
	}

	/* Check for draw per alien produced */
	if (o_ptr->code & P5_DRAW_EACH_ALIEN)
	{
		/* Award cards */
		draw_cards(g, who, produced[GOOD_ALIEN], name);

		/* Count reward */
		p_ptr->phase_cards += produced[GOOD_ALIEN];

		/* Done */
		return;
	}

	/* Check for draw per different kind produced */
	if (o_ptr->code & P5_DRAW_DIFFERENT)
	{
		/* Start count at zero */
		count = 0;

		/* Count types */
		for (i = 0; i < 6; i++)
		{
			/* Check for this type produced */
			if (produced[i]) count++;
		}

		/* Award cards */
		draw_cards(g, who, count, name);

		/* Count reward */
		p_ptr->phase_cards += count;

		/* Done */
		return;
	}
}

/*
 * Loop over produce powers and use them.
 *
 * It is occasionally necessary to ask the player which order to use some
 * powers.
 */
int produce_action(game *g, int who)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100], *w_ptr;
	power *o_ptr;
	int cidx[MAX_DECK], oidx[MAX_DECK];
	int windfall[6], windfall_any = 0;
	int i, x, n, num = 0;
	uint64_t all_codes = 0;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Clear windfall counts */
	for (i = 0; i < 6; i++) windfall[i] = 0;

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Loop over active cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Skip non-windfall worlds */
		if (!(c_ptr->d_ptr->flags & FLAG_WINDFALL)) continue;

		/* Skip windfalls with goods already */
		if (c_ptr->covered != -1) continue;

		/* Windfall of this color needs production */
		windfall[c_ptr->d_ptr->good_type] = 1;

		/* At least one windfall available */
		windfall_any = 1;
	}

	/* Check for "any" windfall */
	if (windfall[GOOD_ANY])
	{
		/* Can count as any type */
		windfall[GOOD_NOVELTY] = 1;
		windfall[GOOD_RARE] = 1;
		windfall[GOOD_GENE] = 1;
		windfall[GOOD_ALIEN] = 1;
	}

	/* Get produce powers */
	n = get_powers(g, who, PHASE_PRODUCE, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power location pointer */
		w_ptr = &w_list[i];

		/* Get power pointer */
		o_ptr = w_ptr->o_ptr;

		/* Get card pointer */
		c_ptr = &g->deck[w_ptr->c_idx];

		/* Check for draw cards */
		if (o_ptr->code & P5_DRAW)
		{
			/* Use immediately */
			produce_chosen(g, who, w_ptr->c_idx, w_ptr->o_idx);
			continue;
		}

		/* Check for produce */
		if (o_ptr->code == P5_PRODUCE)
		{
			/* Skip worlds with good already */
			if (c_ptr->covered != -1) continue;

			/* Use power immediately */
			produce_chosen(g, who, w_ptr->c_idx, w_ptr->o_idx);
			continue;
		}

		/* Check for other draw powers */
		if (o_ptr->code & (P5_DRAW_WORLD_GENE |
				   P5_DRAW_MILITARY |
				   P5_DRAW_REBEL |
				   P5_DRAW_CHROMO |
				   P5_PRESTIGE_MOST_CHROMO |
				   P5_TAKE_SAVED))
		{
			/* Use power immediately */
			produce_chosen(g, who, w_ptr->c_idx, w_ptr->o_idx);
			continue;
		}

		/* Check for useless windfall production */
		if ((o_ptr->code & P5_WINDFALL_NOVELTY) &&
		    windfall[GOOD_NOVELTY] == 0) continue;
		if ((o_ptr->code & P5_WINDFALL_RARE) &&
		    windfall[GOOD_RARE] == 0) continue;
		if ((o_ptr->code & P5_WINDFALL_GENE) &&
		    windfall[GOOD_GENE] == 0) continue;
		if ((o_ptr->code & P5_WINDFALL_ALIEN) &&
		    windfall[GOOD_ALIEN] == 0) continue;
		if ((o_ptr->code & P5_WINDFALL_ANY) &&
		    windfall_any == 0) continue;

		/* Check for produce and already covered */
		if ((o_ptr->code & P5_PRODUCE) && c_ptr->covered != -1)
			continue;

		/* Check for discard needed and no cards in hand */
		if ((o_ptr->code & P5_DISCARD) &&
		    (count_player_area(g, who, WHERE_HAND) +
		     p_ptr->fake_hand - p_ptr->fake_discards < 1)) continue;

		/* Check for useable power */
		if (o_ptr->code & (P5_PRODUCE |
				   P5_WINDFALL_ANY |
				   P5_WINDFALL_NOVELTY |
				   P5_WINDFALL_RARE |
				   P5_WINDFALL_GENE |
				   P5_WINDFALL_ALIEN |
				   P5_DRAW_EACH_NOVELTY |
				   P5_DRAW_EACH_RARE |
				   P5_DRAW_EACH_GENE |
				   P5_DRAW_EACH_ALIEN |
				   P5_DRAW_DIFFERENT))
		{
			/* Add power to list */
			cidx[num] = w_list[i].c_idx;
			oidx[num] = w_list[i].o_idx;
			num++;

			/* Track all codes in list */
			all_codes |= o_ptr->code;
		}
	}

	/* Check for unused produce phase bonus */
	if (windfall_any && player_chose(g, who, ACT_PRODUCE) &&
	    !(p_ptr->phase_bonus_used & (1 << 1)))
	{
		/* Add bonus to list */
		cidx[num] = -1;
		oidx[num] = -1;
		num++;

		/* Add "produce on windfall" cost */
		all_codes |= P5_WINDFALL_ANY;
	}

	/* Check for prestige produce */
	if (windfall_any && player_chose(g, who, ACT_PRESTIGE | ACT_PRODUCE) &&
	    !(p_ptr->phase_bonus_used & (1 << 2)))
	{
		/* Add bonus to list */
		cidx[num] = -2;
		oidx[num] = -1;
		num++;

		/* Add "produce on windfall" cost */
		all_codes |= P5_WINDFALL_ANY;
	}

	/* Check for prestige produce */
	if (player_chose(g, who, ACT_PRESTIGE | ACT_PRODUCE) &&
	    !(p_ptr->phase_bonus_used & (1 << 3)))
	{
		/* Use bonus */
		produce_chosen(g, who, -3, -1);

		/* Check for aborted game */
		if (g->game_over) return 0;

		/* Done */
		return 1;
	}

	/* Check for no additional usable powers */
	if (!num) return 0;

	/* Check for no production powers remaining */
	if (!(all_codes & (P5_PRODUCE | P5_WINDFALL_ANY | P5_WINDFALL_NOVELTY |
	                   P5_WINDFALL_RARE | P5_WINDFALL_GENE |
	                   P5_WINDFALL_ALIEN)))
	{
		/* Use all remaining powers in any order */
		for (i = 0; i < num; i++)
		{
			/* Use power */
			produce_chosen(g, who, cidx[i], oidx[i]);

			/* Check for aborted game */
			if (g->game_over) return 0;
		}

		/* Done */
		return 1;
	}

	/* Check for multiple powers to use */
	if (num > 1)
	{
		/* Ask player to use power */
		ask_player(g, who, CHOICE_PRODUCE, cidx, &num, oidx, &num,
		           0, 0, 0);

		/* Check for aborted game */
		if (g->game_over) return 0;
	}

	/* Use chosen power */
	produce_chosen(g, who, cidx[0], oidx[0]);

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Successfully used power */
	return 1;
}

/*
 * Do end-of-phase produce powers for everyone.
 */
void phase_produce_end(game *g)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int rare[MAX_PLAYER], all[MAX_PLAYER], most;
	int i, j, x, n;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Assume no goods produced */
		rare[i] = all[i] = 0;

		/* Start at first active card */
		x = g->p[i].head[WHERE_ACTIVE];

		/* Loop over cards */
		for ( ; x != -1; x = g->deck[x].next)
		{
			/* Get card pointer */
			c_ptr = &g->deck[x];

			/* Skip cards that did not produce */
			if (!c_ptr->produced) continue;

			/* Count goods produced */
			all[i]++;

			/* Check for rare */
			if (c_ptr->produced == GOOD_RARE) rare[i]++;
		}
	}

	/* Loop over players to check for most rare produced */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Assume player created most rare */
		most = 1;

		/* Loop over other players */
		for (j = 0; j < g->num_players; j++)
		{
			/* Skip same player */
			if (i == j) continue;

			/* Check for no more rares produced */
			if (rare[j] >= rare[i]) most = 0;
		}

		/* Skip player who did not make most rare */
		if (!most) continue;

		/* Get list of produce powers */
		n = get_powers(g, i, PHASE_PRODUCE, w_list);

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Check for produced most rare */
			if (o_ptr->code & P5_DRAW_MOST_RARE)
			{
				/* Draw cards */
				draw_cards(g, i, o_ptr->value,
				           g->deck[w_list[j].c_idx].d_ptr->name);

				/* Count reward */
				p_ptr->phase_cards += o_ptr->value;
			}
		}
	}

	/* Loop over players to check for most goods produced */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Assume player created most goods */
		most = 1;

		/* Loop over other players */
		for (j = 0; j < g->num_players; j++)
		{
			/* Skip same player */
			if (i == j) continue;

			/* Check for no more goods produced */
			if (all[j] >= all[i]) most = 0;
		}

		/* Skip player who did not make most goods */
		if (!most) continue;

		/* Get list of produce powers */
		n = get_powers(g, i, PHASE_PRODUCE, w_list);

		/* Loop over powers */
		for (j = 0; j < n; j++)
		{
			/* Get power pointer */
			o_ptr = w_list[j].o_ptr;

			/* Check for produced most goods */
			if (o_ptr->code & P5_DRAW_MOST_PRODUCED)
			{
				/* Draw cards */
				draw_cards(g, i, o_ptr->value,
				           g->deck[w_list[j].c_idx].d_ptr->name);

				/* Count reward */
				p_ptr->phase_cards += o_ptr->value;
			}
		}
	}
}

/*
 * Perform the Produce Phase for one player.
 */
void produce_player(game *g, int who)
{
	player *p_ptr = &g->p[who];

	/* Clear phase rewards */
	p_ptr->phase_cards = p_ptr->phase_vp = 0;
	p_ptr->phase_prestige = 0;

	/* Set current player */
	g->turn = who;

	/* Use player's produce powers */
	while (produce_action(g, who));
}

/*
 * Handle the Produce Phase.
 */
void phase_produce(game *g)
{
	player *p_ptr;
	int i;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for prepare function */
		if (p_ptr->control->prepare_phase)
		{
			/* Ask player to prepare answers for produce phase */
			p_ptr->control->prepare_phase(g, i, PHASE_PRODUCE, 0);
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Handle player's produce phase */
		produce_player(g, i);
	}

	/* Handle end of phase powers */
	phase_produce_end(g);

	/* Check for simulated game */
	if (!g->simulation)
	{
		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Log rewards */
			log_rewards(g, i, p_ptr->phase_cards,
			            p_ptr->phase_vp, p_ptr->phase_prestige,
			            "for", "Produce phase", NULL);
		}
	}

	/* Clear any temp flags on cards */
	clear_temp(g);

	/* Check intermediate goals */
	check_goals(g);

	/* Check prestige leader */
	check_prestige(g);
}

/*
 * Handle the Discard Phase.
 */
void phase_discard(game *g)
{
	player *p_ptr;
	card *c_ptr;
	int taken = 0, message = 0, discarding[MAX_PLAYER];
	int i, j, n, list[MAX_DECK], target;
	char msg[1024];

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current player */
		g->turn = i;

		/* Assume player has no cards */
		n = 0;

		/* Assume player must discard to 10 */
		target = 10;

		/* Assume player is not discarding */
		discarding[i] = 0;

		/* Check for "discard to 12" flag */
		if (count_active_flags(g, i, FLAG_DISCARD_TO_12))
		{
			/* Set target count to 12 */
			target = 12;
		}

		/* Get cards in hand */
		n = get_player_area(g, i, list, WHERE_HAND);

		/* Assume no cards discarded */
		p_ptr->end_discard = 0;

		/* Check for not too many cards */
		if (n <= target) continue;

		/* Remember cards discarded */
		p_ptr->end_discard = n - target;

		/* Message */
		if (!g->simulation && !message)
		{
			/* Send formatted message */
			message_add_formatted(g, "--- End of round ---\n", FORMAT_PHASE);

			/* Remember message is sent */
			message = 1;
		}

		/* Check for opponent's turn in simulated game */
		if (g->simulation && g->sim_who != i)
		{
			/* Discard first cards */
			discard_callback(g, i, list, n - target);

			/* Next player */
			continue;
		}

		/* Ask player to discard to initial handsize */
		send_choice(g, i, CHOICE_DISCARD, list, &n, NULL, NULL,
		            n - target, 0, 0);

		/* Player is discarding */
		discarding[i] = 1;

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Wait for all decisions */
	wait_for_all(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Skip players who were not asked to discard */
		if (!discarding[i]) continue;

		/* Get discard choice */
		extract_choice(g, i, CHOICE_DISCARD, list, &n, NULL, NULL);

		/* Make discards */
		discard_callback(g, i, list, n);

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s discards %d card%s at end of round.\n",
			        g->p[i].name, n, PLURAL(n));

			/* Send message */
			message_add(g, msg);
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Check for "take discards" flag */
		if (count_active_flags(g, i, FLAG_TAKE_DISCARDS))
		{
			/* Check for common decks */
			if (!separate_decks(g))
			{
				/* Look for cards discarded by opponents */
				for (j = 0; j < g->deck_size; j++)
				{
					/* Get card pointer */
					c_ptr = &g->deck[j];

					/* Skip previously unowned cards */
					if (c_ptr->start_owner < 0) continue;

					/* Skip cards we previously owned */
					if (c_ptr->start_owner == i) continue;

					/* Skip cards that did not move */
					if (c_ptr->start_owner == c_ptr->owner)
						continue;

					/* Take card */
					move_card(g, j, i, WHERE_HAND);

					/* Check for simulation */
					if (g->simulation && g->p[i].control->private_message)
					{
						/* Format message */
						sprintf(msg, "%s takes %s.\n",
						        g->p[i].name, c_ptr->d_ptr->name);

						/* Send private message */
						g->p[i].control->private_message(g, i, msg,
						                                 FORMAT_DRAW);
					}

					/* Adjust known flags */
					c_ptr->known = (1 << i);

					/* Count cards taken */
					taken++;
				}

				/* Check for cards taken */
				if (taken > 0)
				{
					/* Message */
					if (!g->simulation)
					{
						/* Format message */
						sprintf(msg, "%s takes %d discard%s.\n",
						        g->p[i].name, taken, PLURAL(taken));

						/* Send message */
						message_add(g, msg);
					}
				}
			}

			/* Separate discard piles */
			else
			{
				/* Loop over other players */
				for (j = 0; j < g->num_players; j++)
				{
					/* Skip current player */
					if (i == j) continue;

					/* Add discards */
					taken += g->p[j].end_discard;
				}

				/* Check for discarded cards */
				if (taken > 0)
				{
					/* XXX Draw the same number of cards */
					draw_cards(g, i, taken, "Retrofit & Salvage, Inc");
				}
			}
		}
	}
}

/*
 * Return the minimum amount of progress needed to claim a "most" goal.
 */
int goal_minimum(int goal)
{
	/* Switch on goal type */
	switch (goal)
	{
		/* One-dimensional first goals */
		case GOAL_FIRST_5_VP: return 5;
		case GOAL_FIRST_4_TYPES: return 4;
		case GOAL_FIRST_3_ALIEN: return 3;
		case GOAL_FIRST_PHASE_POWER: return 6;
		case GOAL_FIRST_3_UPLIFT: return 3;
		case GOAL_FIRST_4_GOODS: return 4;
		case GOAL_FIRST_8_ACTIVE: return 8;

		/* Most goals */
		case GOAL_MOST_MILITARY: return 6;
		case GOAL_MOST_BLUE_BROWN: return 3;
		case GOAL_MOST_DEVEL: return 4;
		case GOAL_MOST_PRODUCTION: return 4;
		case GOAL_MOST_EXPLORE: return 3;
		case GOAL_MOST_REBEL: return 3;
		case GOAL_MOST_PRESTIGE: return 3;
		case GOAL_MOST_CONSUME: return 3;
	}

	/* Other goals are binary */
	return 1;
}

/*
 * Check a player's progress towards a goal.
 *
 * Return zero if the player does not qualify.
 */
static int check_goal_player(game *g, int goal, int who)
{
	player *p_ptr;
	card *c_ptr;
	power_where w_list[100];
	power *o_ptr;
	int good[6], phase[6], count = 0;
	int i, x, n;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Switch on goal type */
	switch (goal)
	{
		/* First to 5 VP chips */
		case GOAL_FIRST_5_VP:

			/* Return VP count */
			return p_ptr->vp;

		/* First to 4 good types */
		case GOAL_FIRST_4_TYPES:

			/* Clear good marks */
			for (i = 0; i < 6; i++) good[i] = 0;

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip non-worlds */
				if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

				/* Mark good type */
				good[c_ptr->d_ptr->good_type] = 1;
			}

			/* Count types */
			for (i = GOOD_ANY; i <= GOOD_ALIEN; i++)
			{
				/* Check for active type */
				if (good[i]) count++;
			}

			/* Return number of types */
			return count;

		/* First to three Alien cards */
		case GOAL_FIRST_3_ALIEN:

			/* Count number of Alien cards */
			return count_active_flags(g, who, FLAG_ALIEN);

		/* First to discard at end of turn */
		case GOAL_FIRST_DISCARD:

			/* Check for previous discard */
			return p_ptr->end_discard;

		/* First to have powers for each phase */
		case GOAL_FIRST_PHASE_POWER:

			/* Clear phase marks */
			for (i = 0; i < 6; i++) phase[i] = 0;

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Loop over card powers */
				for (i = 0; i < c_ptr->d_ptr->num_power; i++)
				{
					/* Get power pointer */
					o_ptr = &c_ptr->d_ptr->powers[i];

					/* Check for trade power */
					if (o_ptr->phase == PHASE_CONSUME &&
					    (o_ptr->code & P4_TRADE_MASK))
					{
						/* XXX Mark trade power */
						phase[0] = 1;
					}
					else
					{
						/* Mark phase */
						phase[o_ptr->phase] = 1;
					}
				}
			}

			/* Count phases with powers */
			for (i = 0; i < 6; i++)
			{
				/* Check for power */
				if (phase[i]) count++;
			}

			/* Return number of phases with powers */
			return count;

		/* First to have a six-cost development */
		case GOAL_FIRST_SIX_DEVEL:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip worlds */
				if (c_ptr->d_ptr->type == TYPE_WORLD) continue;

				/* Skip non-cost-6 cards */
				if (c_ptr->d_ptr->cost != 6) continue;

				/* Check for variable points */
				if (c_ptr->d_ptr->num_vp_bonus) return 1;
			}

			/* No six-cost developments */
			return 0;

		/* First to three Uplift cards */
		case GOAL_FIRST_3_UPLIFT:

			/* Count number of Uplift cards */
			return count_active_flags(g, who, FLAG_UPLIFT);

		/* First to 4 goods */
		case GOAL_FIRST_4_GOODS:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip non-worlds */
				if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

				/* Check for good */
				if (c_ptr->covered != -1) count++;
			}

			/* Return number of goods */
			return count;

		/* First to 8 active cards */
		case GOAL_FIRST_8_ACTIVE:

			/* Count number of cards */
			return count_player_area(g, who, WHERE_ACTIVE);

		/* First to have negative military or takeover power */
		case GOAL_FIRST_NEG_MILITARY:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip non-worlds */
				if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

				/* Count active worlds */
				count++;
			}

			/* Check for not at least 2 worlds */
			if (count < 2) return 0;

			/* Check for negative military */
			if (total_military(g, who) < 0) return 1;

			/* Check for takeovers disabled */
			if (g->takeover_disabled) return 0;

			/* Get count of military worlds */
			count = count_active_flags(g, who, FLAG_MILITARY);

			/* Check for less than 2 military worlds */
			if (count < 2) return 0;

			/* Get Settle phase powers */
			n = get_powers(g, who, PHASE_SETTLE, w_list);

			/* Loop over powers */
			for (i = 0; i < n; i++)
			{
				/* Get power pointer */
				o_ptr = w_list[i].o_ptr;

				/* Check for takeover power */
				if (o_ptr->code & (P3_TAKEOVER_REBEL |
				                   P3_TAKEOVER_IMPERIUM |
				                   P3_TAKEOVER_MILITARY |
				                   P3_TAKEOVER_PRESTIGE))
				{
					/* Takeover condition met */
					return 1;
				}
			}

			/* Goal not met */
			return 0;

		/* First to 2 prestige and 3 VP chips */
		case GOAL_FIRST_2_PRESTIGE:

			/* Check for enough prestige and VP */
			return p_ptr->prestige >= 2 && p_ptr->vp >= 3;

		/* First to 3 Imperium cards or 4 military worlds */
		case GOAL_FIRST_4_MILITARY:

			/* Check for enough Imperium or military */
			return count_active_flags(g, who, FLAG_IMPERIUM) >= 3 ||
			       count_active_flags(g, who, FLAG_MILITARY) >= 4;

		/* Most military (minimum 6) */
		case GOAL_MOST_MILITARY:

			/* Get military strength */
			return total_military(g, who);

		/* Most blue/brown worlds (minimum 3) */
		case GOAL_MOST_BLUE_BROWN:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip non-worlds */
				if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

				/* Check for blue or brown */
				if (c_ptr->d_ptr->good_type == GOOD_NOVELTY ||
				    c_ptr->d_ptr->good_type == GOOD_RARE)
				{
					/* Count world */
					count++;
				}

				/* Check for "any" kind */
				if (c_ptr->d_ptr->good_type == GOOD_ANY &&
				    (g->oort_kind == GOOD_ANY ||
				     g->oort_kind == GOOD_NOVELTY ||
				     g->oort_kind == GOOD_RARE))
				{
					/* Count world */
					count++;
				}
			}

			/* Return count */
			return count;

		/* Most developments (minimum 4) */
		case GOAL_MOST_DEVEL:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip worlds */
				if (c_ptr->d_ptr->type == TYPE_WORLD) continue;

				/* Count developments */
				count++;
			}

			/* Return count */
			return count;

		/* Most production worlds (minimum 4) */
		case GOAL_MOST_PRODUCTION:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip non-worlds */
				if (c_ptr->d_ptr->type != TYPE_WORLD) continue;

				/* Skip windfall worlds */
				if (c_ptr->d_ptr->flags & FLAG_WINDFALL)
					continue;

				/* Skip worlds with no good type */
				if (!c_ptr->d_ptr->good_type) continue;

				/* Count world */
				count++;
			}

			/* Return count */
			return count;

		/* Most explore powers (minimum 3) */
		case GOAL_MOST_EXPLORE:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Loop over card powers */
				for (i = 0; i < c_ptr->d_ptr->num_power; i++)
				{
					/* Get power pointer */
					o_ptr = &c_ptr->d_ptr->powers[i];

					/* Check for explore phase */
					if (o_ptr->phase == PHASE_EXPLORE)
					{
						/* Count card */
						count++;

						/* Stop looking */
						break;
					}
				}
			}

			/* Return count */
			return count;

		/* Most Rebel military worlds (minimum 3) */
		case GOAL_MOST_REBEL:

			/* Count military Rebel worlds */
			return count_active_flags(g, who, FLAG_REBEL |
			                                  FLAG_MILITARY);

		/* Most prestige (minimum 3) */
		case GOAL_MOST_PRESTIGE:

			/* Return amount of prestige */
			return p_ptr->prestige;

		/* Most cards with consume powers (minimum 3) */
		case GOAL_MOST_CONSUME:

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Loop over card powers */
				for (i = 0; i < c_ptr->d_ptr->num_power; i++)
				{
					/* Get power pointer */
					o_ptr = &c_ptr->d_ptr->powers[i];

					/* Check for consume phase */
					if (o_ptr->phase == PHASE_CONSUME)
					{
						/* Skip trade powers */
						if (o_ptr->code & P4_TRADE_MASK)
						{
							/* Skip power */
							continue;
						}

						/* Count card */
						count++;

						/* Stop looking */
						break;
					}
				}
			}

			/* Return count */
			return count;
	}

	/* XXX */
	return 0;
}

/*
 * Printable good names (which start at cost/value 2).
 */
char *good_printable[MAX_GOOD] =
{
	"",
	"Any",
	"Novelty",
	"Rare",
	"Genes",
	"Alien",
};

/*
 * Goal names.
 */
char *goal_name[MAX_GOAL] =
{
	"Galactic Standard of Living",
	"System Diversity",
	"Overlord Discoveries",
	"Budget Surplus",
	"Innovation Leader",
	"Galactic Status",
	"Uplift Knowledge",
	"Galactic Riches",
	"Expansion Leader",
	"Peace/War Leader",
	"Galactic Standing",
	"Military Influence",

	"Greatest Military",
	"Largest Industry",
	"Greatest Infrastructure",
	"Production Leader",
	"Research Leader",
	"Propaganda Edge",
	"Galactic Prestige",
	"Prosperity Lead",
};

/*
 * Check for loss of a "most" goal.  These goals can be lost at any time
 * by discarding active cards, for example.
 */
void check_goal_loss(game *g, int who, int goal)
{
	player *p_ptr = &g->p[who];
	char msg[1024];
	int count, most = 0;
	int i;

	/* Check for inactive goal */
	if ((g->goal_active & (1 << goal)) == 0) return;

	/* Only check for player who has claimed goal */
	if ((p_ptr->goal_claimed & (1 << goal)) == 0) return;

	/* Recheck progress */
	count = check_goal_player(g, goal, who);

	/* Save progress */
	p_ptr->goal_progress[goal] = count;

	/* Check for under the minimum */
	if (count < goal_minimum(goal)) count = 0;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Check for more progress */
		if (g->p[i].goal_progress[goal] > most)
		{
			/* Remember most */
			most = g->p[i].goal_progress[goal];
		}
	}

	/* Save new most */
	g->goal_most[goal] = most;

	/* Check for less than most or less than minimum */
	if (count < most || count == 0)
	{
		/* Goal is now unclaimed */
		p_ptr->goal_claimed &= ~(1 << goal);
		g->goal_avail |= 1 << goal;

		/* Message */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s loses %s goal.\n", p_ptr->name,
			        goal_name[goal]);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_GOAL);
		}
	}
}

/*
 * Award goals to players who meet requirements.
 */
void check_goals(game *g)
{
	player *p_ptr;
	int count[MAX_PLAYER], most;
	int i, j, k;
	char msg[1024];

	/* Loop over "first" goals */
	for (i = GOAL_FIRST_5_VP; i <= GOAL_FIRST_4_MILITARY; i++)
	{
		/* Skip inactive goals */
		if ((g->goal_active & (1 << i)) == 0) continue;

		/* Skip already claimed goals */
		if ((g->goal_avail & (1 << i)) == 0) continue;

		/* Do not check goals that cannot happen yet */
		switch (i)
		{
			/* End of turn only */
			case GOAL_FIRST_DISCARD:

				/* Only check at end of turn */
				if (g->cur_action != ACT_ROUND_END) continue;
				break;

			/* Develop phase only */
			case GOAL_FIRST_SIX_DEVEL:

				/* Only check after develop */
				if (g->cur_action != ACT_DEVELOP &&
				    g->cur_action != ACT_DEVELOP2) continue;
				break;

			/* Settle phase only */
			case GOAL_FIRST_4_TYPES:

				/* Only check after settle */
				if (g->cur_action != ACT_SETTLE &&
				    g->cur_action != ACT_SETTLE2) continue;
				break;

			/* Develop/Settle phases only */
			case GOAL_FIRST_3_ALIEN:
			case GOAL_FIRST_PHASE_POWER:
			case GOAL_FIRST_3_UPLIFT:
			case GOAL_FIRST_8_ACTIVE:
			case GOAL_FIRST_NEG_MILITARY:
			case GOAL_FIRST_4_MILITARY:

				/* Only check after develop/settle */
				if (g->cur_action != ACT_DEVELOP &&
				    g->cur_action != ACT_DEVELOP2 &&
				    g->cur_action != ACT_SETTLE &&
				    g->cur_action != ACT_SETTLE2) continue;
				break;

			/* Consume phase or beginning of turn only */
			case GOAL_FIRST_5_VP:

				/* Only check after round start/consume */
				if (g->cur_action != ACT_CONSUME_TRADE &&
				    g->cur_action != ACT_ROUND_START)
					continue;
				break;

			/* Settle or Produce phases */
			case GOAL_FIRST_4_GOODS:

				/* Only check after settle or produce */
				if (g->cur_action != ACT_SETTLE &&
				    g->cur_action != ACT_SETTLE2 &&
				    g->cur_action != ACT_PRODUCE) continue;
				break;

			/* Any phase */
			case GOAL_FIRST_2_PRESTIGE:
				break;
		}

		/* Loop over players */
		for (j = 0; j < g->num_players; j++)
		{
			/* Get player pointer */
			p_ptr = &g->p[j];

			/* Get player's progress */
			count[j] = check_goal_player(g, i, j);

			/* Save progress */
			g->p[j].goal_progress[i] = count[j];

			/* Check for player meeting requirement */
			if (count[j] >= goal_minimum(i))
			{
				/* Claim goal */
				p_ptr->goal_claimed |= 1 << i;

				/* Remove goal availability */
				g->goal_avail &= ~(1 << i);

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s claims %s goal.\n",
					        p_ptr->name, goal_name[i]);

					/* Send message */
					message_add_formatted(g, msg, FORMAT_GOAL);
				}
			}
		}
	}

	/* Loop over "most" goals */
	for (i = GOAL_MOST_MILITARY; i <= GOAL_MOST_CONSUME; i++)
	{
		/* Skip inactive goals */
		if ((g->goal_active & (1 << i)) == 0) continue;

		/* Do not check goals that cannot happen yet */
		switch (i)
		{
			/* Settle phase only */
			case GOAL_MOST_BLUE_BROWN:
			case GOAL_MOST_PRODUCTION:
			case GOAL_MOST_REBEL:

				/* Only check after settle */
				if (g->cur_action != ACT_SETTLE &&
				    g->cur_action != ACT_SETTLE2) continue;
				break;

			/* Develop phase only */
			case GOAL_MOST_DEVEL:

				/* Only check after develop */
				if (g->cur_action != ACT_DEVELOP &&
				    g->cur_action != ACT_DEVELOP2) continue;
				break;

			/* Develop/Settle phases only */
			case GOAL_MOST_MILITARY:
			case GOAL_MOST_EXPLORE:
			case GOAL_MOST_CONSUME:

				/* Only check after develop/settle */
				if (g->cur_action != ACT_DEVELOP &&
				    g->cur_action != ACT_DEVELOP2 &&
				    g->cur_action != ACT_SETTLE &&
				    g->cur_action != ACT_SETTLE2) continue;
				break;

			/* Any phase */
			case GOAL_MOST_PRESTIGE:
				break;
		}

		/* Clear most progress */
		g->goal_most[i] = 0;

		/* Loop over each player */
		for (j = 0; j < g->num_players; j++)
		{
			/* Get player's progress */
			count[j] = check_goal_player(g, i, j);

			/* Save progress */
			g->p[j].goal_progress[i] = count[j];

			/* Check for more than most */
			if (count[j] > g->goal_most[i])
			{
				/* Remember most */
				g->goal_most[i] = count[j];
			}

			/* Check for insufficient progress */
			if (count[j] < goal_minimum(i)) count[j] = 0;
		}

		/* Check for losing goal */
		for (j = 0; j < g->num_players; j++)
		{
			/* Get player pointer */
			p_ptr = &g->p[j];

			/* Check for goal claimed and lost */
			if ((p_ptr->goal_claimed & (1 << i)) && !count[j])
			{
				/* Lose goal */
				g->goal_avail |= 1 << i;
				p_ptr->goal_claimed &= ~(1 << i);

				/* Message */
				if (!g->simulation)
				{
					/* Format message */
					sprintf(msg, "%s loses %s goal.\n",
					        p_ptr->name, goal_name[i]);

					/* Send message */
					message_add_formatted(g, msg, FORMAT_GOAL);
				}
			}
		}

		/* Loop over players */
		for (j = 0; j < g->num_players; j++)
		{
			/* Assume this player has most */
			most = 1;

			/* Loop over opponents */
			for (k = 0; k < g->num_players; k++)
			{
				/* Do not compete with ourself */
				if (j == k) continue;

				/* Check for no more than opponent */
				if (count[j] <= count[k]) most = 0;
			}

			/* Get player pointer */
			p_ptr = &g->p[j];

			/* Check for more than anyone else */
			if (most && (p_ptr->goal_claimed & (1 << i)) == 0)
			{
				/* Goal is no longer available */
				g->goal_avail &= ~(1 << i);

				/* Loop over players */
				for (k = 0; k < g->num_players; k++)
				{
					/* Get player pointer */
					p_ptr = &g->p[k];

					/* Check for matching player */
					if (j == k)
					{
						/* Award goal to player */
						p_ptr->goal_claimed |= 1 << i;
					}
					else
					{
						/* Remove goal from player */
						p_ptr->goal_claimed &= ~(1 << i);
					}
				}

				/* Message */
				if (!g->simulation)
				{
					/* Get player pointer */
					p_ptr = &g->p[j];

					/* Format message */
					sprintf(msg, "%s claims %s goal.\n",
					        p_ptr->name, goal_name[i]);

					/* Send message */
					message_add_formatted(g, msg, FORMAT_GOAL);
				}
			}
		}
	}
}

/*
 * Rotate players one spot.
 *
 * This will be called until the player with the lowest start world is
 * player number 0.
 */
static void rotate_players(game *g)
{
	player temp, *p_ptr;
	card *c_ptr;
	int i, bit;

	/* Store copy of player 0 */
	temp = g->p[0];

	/* Loop over players */
	for (i = 0; i < g->num_players - 1; i++)
	{
		/* Copy players one space */
		g->p[i] = g->p[i + 1];
	}

	/* Store old player 0 in last spot */
	g->p[i] = temp;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards owned by no one */
		if (c_ptr->owner == -1) continue;

		/* Adjust owner */
		c_ptr->owner--;

		/* Check for wraparound */
		if (c_ptr->owner < 0) c_ptr->owner = g->num_players - 1;

		/* Track lowest bit of known */
		bit = c_ptr->known & 1;

		/* Adjust known bits */
		c_ptr->known >>= 1;

		/* Rotate old lowest bit to highest position */
		c_ptr->known |= bit << (g->num_players - 1);
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Notify player of rotation */
		p_ptr->control->notify_rotation(g, i);
	}
}

/*
 * Checks for and performs any debug rotations.
 */
static void check_debug_rotate(game *g)
{
	int i;
	char msg[1024];

	/* Check for debug rotation */
	if (g->debug_rotate)
	{
		/* Rotate players */
		for (i = 0; i < g->debug_rotate; ++i) rotate_players(g);

		/* Format message */
		sprintf(msg, "%s is now the first player.\n", g->p[0].name);

		/* Add message */
		message_add_formatted(g, msg, FORMAT_DEBUG);

		/* Clear rotation */
		g->debug_rotate = 0;
	}
}

/*
 * Called when a player has chosen a start world and initial discards.
 */
int start_callback(game *g, int who, int list[], int n, int special[], int ns)
{
	char msg[1024];
	player *p_ptr = &g->p[who];

	/* Ensure exactly one start world chosen */
	if (ns != 1) return 0;

	/* Remember start card */
	p_ptr->start = special[0];

	/* Check for 2 cards discarded */
	if (n != 2) return 0;

	/* Discard chosen cards */
	discard_callback(g, who, list, n);

	/* Message */
	if (!g->simulation && p_ptr->control->private_message)
	{
		/* Format message */
		sprintf(msg, "%s discards the start world %s.\n",
		        p_ptr->name,
		        g->deck[special[1]].d_ptr->name);

		/* Send message */
		p_ptr->control->private_message(g, who, msg, FORMAT_DISCARD);
	}

	/* Place start card */
	place_card(g, who, special[0]);

	/* Choice is good */
	return 1;
}

/*
 * Perform the initial draft.
 */
static void perform_draft(game *g, int start_picks[MAX_PLAYER][2])
{
	char msg[1024];
	int draws[MAX_PLAYER][9], n, ns = 2, swap;
	int i, j, k, m, choice;
	int draw, rounds, cards, rotation, card;

	/* Start drafting phase */
	g->cur_action = ACT_DRAFTING;

	/* Compute number of drafting rounds */
	if (g->expanded == EXPANSION_TGS && g->num_players == 2) rounds = 13;
	else if (g->expanded == EXPANSION_TGS && g->num_players == 3) rounds = 8;

	else if (g->expanded == EXPANSION_RVI && g->num_players == 2) rounds = 17;
	else if (g->expanded == EXPANSION_RVI && g->num_players == 3) rounds = 11;
	else if (g->expanded == EXPANSION_RVI && g->num_players == 4) rounds = 8;
	else if (g->expanded == EXPANSION_RVI && g->num_players == 5) rounds = 6;

	else if (g->expanded == EXPANSION_BOW && g->num_players == 2) rounds = 23;
	else if (g->expanded == EXPANSION_BOW && g->num_players == 3) rounds = 15;
	else if (g->expanded == EXPANSION_BOW && g->num_players == 4) rounds = 8;
	else if (g->expanded == EXPANSION_BOW && g->num_players == 5) rounds = 5;
	else rounds = 4;

	/* Default draw count is 5 */
	draw = 5;

	/* Adjust number of cards drawn in each round */
	if (g->expanded == EXPANSION_BOW && g->num_players == 4) draw = 7;
	else if (g->expanded == EXPANSION_BOW && g->num_players > 4) draw = 9;

	/* Loop over all rounds */
	for (i = 0; i < rounds; ++i)
	{
		/* Reset rotation */
		rotation = 0;

		/* Check for needing to adjust cards drawn */
		if (g->expanded == EXPANSION_BOW && i == rounds - 1)
		{
			/* Adjust number of cards drawn for last round */
			if (g->num_players == 2) draw = 2;
			else if (g->num_players == 3) draw = 4;
			else if (g->num_players == 4) draw = 6;
			else if (g->num_players == 5) draw = 7;
			else if (g->num_players == 6) draw = 9;
		}

		/* Format message */
		sprintf(msg, "=== Draft round %d (of %d) ===\n", i + 1, rounds);

		/* Send message */
		message_add_formatted(g, msg, FORMAT_EM);

		/* Format message */
		sprintf(msg, "All players draw %d card%s.\n", draw, PLURAL(draw));

		/* Send message */
		message_add(g, msg);

		/* Loop over players */
		for (j = 0; j < g->num_players; ++j)
		{
			/* For each card */
			for (k = 0; k < draw; ++k)
			{
				/* Draw card */
				draws[j][k] = random_draw(g, -1, NULL);

				/* Put card in hand */
				move_card(g, draws[j][k], -1, WHERE_HAND);

				/* Check for private message */
				if (g->p[j].control->private_message)
				{
					/* Format message */
					sprintf(msg, "%s draws %s.\n", g->p[j].name,
					        g->deck[draws[j][k]].d_ptr->name);

					/* Send message */
					g->p[j].control->private_message(g, j, msg, FORMAT_DRAW);
				}
			}
		}

		/* Check for needing to write direction */
		if (g->num_players > 2)
		{
			/* Format message */
			sprintf(msg, "Cards are passed %s.\n", i % 2 ? "right" : "left");

			/* Send message */
			message_add(g, msg);
		}

		/* Loop over all passes */
		for (j = 0; j < draw; ++j)
		{
			/* Compute cards left */
			n = draw - j;

			/* Find choice to use */
			choice = j == 0 ? CHOICE_DRAFT_FIRST : CHOICE_DRAFT;

			/* Loop over players */
			for (k = 0; k < g->num_players; ++k)
			{
				/* Find cards */
				cards = (k + rotation) % g->num_players;

				/* Loop over cards */
				for (m = 0; m < n; ++m)
				{
					/* Give cards to player and set known location */
					move_card(g, draws[cards][m], k, WHERE_HAND);
					g->deck[draws[cards][m]].known = 1 << k;
				}

				/* Ask player */
				send_choice(g, k, choice, draws[cards], &n,
				            start_picks[k], &ns, draw, 0, 0);

				/* Check for aborted game */
				if (g->game_over) return;
			}

			/* Wait for all choices */
			wait_for_all(g);

			/* Loop over players */
			for (k = 0; k < g->num_players; ++k)
			{
				/* Find cards */
				cards = (k + rotation) % g->num_players;

				/* Get answer */
				card = extract_choice(g, k, choice, draws[cards], &n,
				                      start_picks[k], &ns);

				/* Reset swap */
				swap = n-1;

				/* Loop over choices */
				for (m = 0; m < n; ++m)
				{
					/* Check if card was selected */
					if (draws[cards][m] == card)
					{
						/* Move to private deck */
						move_card(g, card, k, WHERE_DECK);

						/* Check for private message */
						if (g->p[k].control->private_message)
						{
							/* Format message */
							sprintf(msg, "%s drafts %s.\n", g->p[k].name,
							        g->deck[card].d_ptr->name);

							/* Send message */
							g->p[k].control->private_message(g, k, msg, "");
						}

						/* Save swap position */
						swap = m;
					}
					else
					{
						/* Set aside card */
						move_card(g, draws[cards][m], -1, WHERE_HAND);
					}
				}

				/* Swap cards */
				draws[cards][swap] = draws[cards][n-1];
				draws[cards][n-1] = -1;
			}

			/* Pass cards */
			rotation += i % 2 ? 1 : -1;
			if (rotation < 0) rotation += g->num_players;
		}
	}

	/* Send message */
	message_add_formatted(g, "=== End of draft ===\n", FORMAT_EM);

	/* Format message */
	sprintf(msg, "Each player has %d cards.\n", count_draw(g, 0) + 2);

	/* Send message */
	message_add(g, msg);

	/* Count remaining cards */
	cards = count_draw(g, -1);

	/* Loop over remaining cards */
	for (i = 0; i < cards; ++i)
	{
		/* Remove card from the game */
		move_card(g, random_draw(g, -1, NULL), -1, WHERE_REMOVED);
	}

	/* Check if any cards were removed */
	if (cards)
	{
		/* Format message */
		sprintf(msg, "%d card%s %s removed from the game.\n",
		        cards, PLURAL(cards), cards == 1 ? "is" : "are");

		/* Send message */
		message_add(g, msg);
	}

	/* End drafting phase */
	g->cur_action = ACT_GAME_START;
}

/*
 * Start the game by splitting the deck.
 */
static void split_deck(game *g, int num_start)
{
	char msg[1024];
	int i, j, count;

	/* Calculate number of cards per player */
	count = (g->deck_size / g->num_players) - num_start;

	/* Loop over players */
	for (i = 0; i < g->num_players; ++i)
	{
		/* Give each player the same amount of cards */
		for (j = 0; j < count; ++j)
			move_card(g, random_draw(g, -1, NULL), i, WHERE_DECK);
	}

	/* Format message */
	sprintf(msg, "Each player has %d cards.\n", count + num_start);

	/* Send message */
	message_add(g, msg);

	/* Count remaining cards */
	count = count_draw(g, -1);

	/* Loop over remaining cards */
	for (i = 0; i < count; ++i)
	{
		/* Remove card from the game */
		move_card(g, random_draw(g, -1, NULL), -1, WHERE_REMOVED);
	}

	/* Check if any cards were removed */
	if (count)
	{
		/* Format message */
		sprintf(msg, "%d card%s %s removed from the game.\n",
		        count, PLURAL(count), count == 1 ? "is" : "are");

		/* Send message */
		message_add(g, msg);
	}
}

/*
 * Deal out start worlds and ask for initial discards.
 */
void begin_game(game *g)
{
	player *p_ptr;
	card *c_ptr;
	int start[MAX_DECK], start_red[MAX_DECK], start_blue[MAX_DECK];
	int start_picks[MAX_PLAYER][2], original_start_picks[MAX_PLAYER][2];
	int hand[MAX_DECK], discarding[MAX_PLAYER];
	int i, j, n, ns;
	int lowest = MAX_DECK, low_i = -1;
	int num_start = 0, num_start_red = 0, num_start_blue = 0;
	char msg[1024];

	/* Format message */
	sprintf(msg, "Race for the Galaxy " RELEASE ": %s.\n", exp_names[g->expanded]);

	/* Send message */
	message_add(g, msg);

	/* Format num players and advanced message */
	if (!g->advanced)
	{
		sprintf(msg, "%s.\n", player_labels[g->num_players - 2]);
	}
	else
	{
		sprintf(msg, "%s, advanced game.\n", player_labels[g->num_players - 2]);
	}

	/* Send message */
	message_add(g, msg);

	/* Check for variant */
	if (g->variant)
	{
		/* Format message */
		sprintf(msg, "Variant: %s.\n", variant_labels[g->variant]);

		/* Send message */
		message_add(g, msg);
	}

	/* Check for expansion with goals */
	if (g->expanded >= EXPANSION_TGS)
	{
		/* Check for disabled goals */
		if (g->goal_disabled)
		{
			/* Send message */
			message_add(g, "Goals disabled.\n");
		}
		else
		{
			/* Send message */
			message_add_formatted(g, "Goals enabled.\n", FORMAT_GOAL);
		}
	}

	/* Check for expansion with takeovers */
	if (g->expanded >= EXPANSION_RVI)
	{
		/* Check for disabled takeovers */
		if (g->takeover_disabled)
		{
			/* Send message */
			message_add(g, "Takeovers disabled.\n");
		}
		else
		{
			/* Send message */
			message_add_formatted(g, "Takeovers enabled.\n", FORMAT_TAKEOVER);
		}
	}

	/* Start game */
	g->cur_action = ACT_GAME_START;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];
		
		/* Check for preset variant */
		if (g->variant == VARIANT_PRESET)
		{
			/* Skip start world if not part of a preset hand */
			if (c_ptr->d_ptr->preset == 0 ||
			    c_ptr->d_ptr->preset > g->num_players)
				continue;
		}

		/* Check for start world */
		if (c_ptr->d_ptr->flags & FLAG_START)
		{
			/* Add to list */
			start[num_start++] = i;
		}

		/* Check for red start world */
		if (c_ptr->d_ptr->flags & FLAG_START_RED)
		{
			/* Add to list */
			start_red[num_start_red++] = i;
		}

		/* Check for blue start world */
		if (c_ptr->d_ptr->flags & FLAG_START_BLUE)
		{
			/* Add to list */
			start_blue[num_start_blue++] = i;
		}
	}

	/* Check for takeover scenario */
	if (g->variant == VARIANT_TAKEOVER)
	{
		/* Check for wanting to rotate players */
		if (game_rand(g, -1) % 2)
		{
			/* Rotate players one step */
			rotate_players(g);
		}

		/* Loop over cards in deck */
		for (i = 0; i < g->deck_size; i++)
		{
			/* Get card pointer */
			c_ptr = &g->deck[i];

			/* Check for start cards */
			if (!strcmp(c_ptr->d_ptr->name, "Rebel Alliance") ||
			    !strcmp(c_ptr->d_ptr->name, "Imperium Seat"))
			{
				/* Set aside card */
				move_card(g, i, -1, WHERE_REMOVED);
			}

			/* Check for Rebel Cantina start card */
			if (!strcmp(c_ptr->d_ptr->name, "Rebel Cantina"))
			{
				/* Remember start world */
				g->p[0].start = i;

				/* Give chosen world to player */
				place_card(g, 0, g->p[0].start);
			}

			/* Check for Imperium Warlord start card */
			if (!strcmp(c_ptr->d_ptr->name, "Imperium Warlord"))
			{
				/* Remember start world */
				g->p[1].start = i;

				/* Give chosen world to player */
				place_card(g, 1, g->p[1].start);
			}
		}

		/* Send start of game message */
		message_add_formatted(g, "=== Start of game ===\n", FORMAT_EM);

		/* Loop over players again */
		for (i = 0; i < g->num_players; i++)
		{
			/* Give player six cards */
			draw_cards(g, i, 6, NULL);
		}
	}

	/* Check for two start world choices */
	else if (g->expanded >= EXPANSION_RVI || g->variant == VARIANT_DRAFTING)
	{
		/* Send start of game message */
		message_add_formatted(g, "=== Start of game ===\n", FORMAT_EM);

		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Choose a Red start world */
			n = game_rand(g, -1) % num_start_red;

			/* Add to start world choices */
			start_picks[i][0] = start_red[n];
			original_start_picks[i][0] = start_picks[i][0];

			/* Collapse list */
			start_red[n] = start_red[--num_start_red];

			/* Choose a Blue start world */
			n = game_rand(g, -1) % num_start_blue;

			/* Add to start world choices */
			start_picks[i][1] = start_blue[n];
			original_start_picks[i][1] = start_picks[i][1];

			/* Collapse list */
			start_blue[n] = start_blue[--num_start_blue];

			/* Message */
			if (g->p[i].control->private_message)
			{
				/* Format message */
				sprintf(msg, "%s draws the start world %s.\n",
				        g->p[i].name,
				        g->deck[start_picks[i][0]].d_ptr->name);

				/* Send message */
				g->p[i].control->private_message(g, i, msg, FORMAT_DRAW);
			}

			/* Message */
			if (g->p[i].control->private_message)
			{
				/* Format message */
				sprintf(msg, "%s draws the start world %s.\n",
				        g->p[i].name,
				        g->deck[start_picks[i][1]].d_ptr->name);

				/* Send message */
				g->p[i].control->private_message(g, i, msg, FORMAT_DRAW);
			}

			/* XXX Move cards to discard */
			discard_card(g, i, start_picks[i][0]);
			discard_card(g, i, start_picks[i][1]);
		}

		/* Check for drafting */
		if (g->variant == VARIANT_DRAFTING)
		{
			/* Perform the initial draft */
			perform_draft(g, start_picks);

			/* Check for aborted game */
			if (g->game_over) return;
		}

		/* Check for separate decks */
		else if (g->variant == VARIANT_SEPARATE)
		{
			/* Separate the deck */
			split_deck(g, 2);
		}

		/* Loop over players again */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Give player six cards */
			draw_cards(g, i, 6, NULL);

			/* Reset list of cards in hand */
			n = 0;

			/* Loop over cards */
			for (j = 0; j < g->deck_size; j++)
			{
				/* Get card pointer */
				c_ptr = &g->deck[j];

				/* Skip unowned cards */
				if (c_ptr->owner != i) continue;

				/* Skip cards not in hand */
				if (c_ptr->where != WHERE_HAND) continue;

				/* Add card to list */
				hand[n++] = j;
			}

			/* Two choices for homeworld */
			ns = 2;

			/* Ask player which start world they want */
			send_choice(g, i, CHOICE_START, hand, &n,
			            start_picks[i], &ns, 0, 0, 0);

			/* Check for aborted game */
			if (g->game_over) return;
		}

		/* Wait for answers from all players before revealing choices */
		wait_for_all(g);

		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Get answer */
			extract_choice(g, i, CHOICE_START, hand, &n,
			               start_picks[i], &ns);

			/* XXX Check for selecting second start world */
			if (original_start_picks[i][0] != start_picks[i][0])
			{
				/* Remember other world */
				start_picks[i][1] = original_start_picks[i][0];
			}

			/* Apply choice */
			start_callback(g, i, hand, n, start_picks[i], ns);
		}
	}
	else
	{
		/* Loop over start worlds */
		for (i = 0; i < num_start; i++)
		{
			/* Temporarily move card to discard pile */
			move_card(g, start[i], -1, WHERE_DISCARD);
		}

		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Choose a start world number */
			n = game_rand(g, -1) % num_start;

			/* Remember start world */
			g->p[i].start = start[n];

			/* Collapse list */
			start[n] = start[--num_start];
		}

		/* Loop over start worlds */
		for (i = 0; i < num_start; i++)
		{
			/* Move card back to deck */
			move_card(g, start[i], -1, WHERE_DECK);
		}

		/* Check for separate decks */
		if (g->variant == VARIANT_SEPARATE)
		{
			/* Separate the deck */
			split_deck(g, 1);
		}

		/* Send start of game message */
		message_add_formatted(g, "=== Start of game ===\n", FORMAT_EM);

		/* Check for preset hand variant */
		if (g->variant == VARIANT_PRESET)
		{
			/* Loop over players */
			for (i = 0; i < g->num_players; i++)
			{
				/* Loop over cards */
				for (j = 0; j < g->deck_size; j++)
				{
					/* Get card pointer */
					c_ptr = &g->deck[j];

					/* Skip used worlds */
					if (c_ptr->where != WHERE_DECK) continue;

					/* Skip development copies */
					if (j > 0 && c_ptr->d_ptr == g->deck[j-1].d_ptr) continue;

					/* Check for matching start hand */
					if (c_ptr->d_ptr->preset ==
					    g->deck[g->p[i].start].d_ptr->preset)
					{
						/* Give card to player */
						move_card(g, j, i, WHERE_HAND);

						/* Card's location is known to the player */
						c_ptr->known |= 1 << i;

						/* Message */
						if (!g->simulation)
						{
							/* Format message */
							sprintf(msg, "%s receives %s.\n",
							        g->p[i].name, c_ptr->d_ptr->name);

							/* Add message */
							message_add_formatted(g, msg, FORMAT_VERBOSE);
						}
					}
				}
			}
		}

		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Give chosen world to player */
			place_card(g, i, g->p[i].start);
		}

		/* Check for preset variant */
		if (g->variant != VARIANT_PRESET)
		{
			/* Loop over players again */
			for (i = 0; i < g->num_players; i++)
			{
				/* Give player six cards */
				draw_cards(g, i, 6, NULL);
			}
		}
	}

	/* Find lowest numbered start world */
	for (i = 0; i < g->num_players; i++)
	{
		/* Check for lower number */
		if (g->p[i].start < lowest)
		{
			/* Remember lowest number */
			lowest = g->p[i].start;
			low_i = i;
		}
	}

	/* Rotate players until player 0 holds lowest start world */
	for (i = 0; i < low_i; i++) rotate_players(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Get player's start world */
		c_ptr = &g->deck[p_ptr->start];

		/* Format message */
		sprintf(msg, "%s starts with %s.\n", p_ptr->name,
		        c_ptr->d_ptr->name);

		/* Send message */
		message_add(g, msg);
	}

	/* Format message */
	sprintf(msg, "%s is the first player.\n", g->p[0].name);

	/* Send message */
	message_add_formatted(g, msg, FORMAT_VERBOSE);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Get list of cards in hand */
		n = get_player_area(g, i, hand, WHERE_HAND);

		/* Assume player gets four cards */
		j = 4;

		/* Get player's start world */
		c_ptr = &g->deck[p_ptr->start];

		/* Check for starting with less */
		if (c_ptr->d_ptr->flags & FLAG_STARTHAND_3) j = 3;

		/* Assume not discarding */
		discarding[i] = 0;

		/* Check for nothing to discard */
		if (n == j) continue;

		/* Ask player to discard to initial handsize */
		send_choice(g, i, CHOICE_DISCARD, hand, &n, NULL, NULL,
		            n - j, 0, 0);

		/* Player is discarding */
		discarding[i] = 1;

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Wait for all decisions */
	wait_for_all(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Skip players who were not asked to discard */
		if (!discarding[i]) continue;

		/* Get discard choice */
		extract_choice(g, i, CHOICE_DISCARD, hand, &n, NULL, NULL);

		/* Make discards */
		discard_callback(g, i, hand, n);
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Get player's start world */
		c_ptr = &g->deck[p_ptr->start];

		/* Check for starting with saved card */
		if (c_ptr->d_ptr->flags & FLAG_START_SAVE)
		{
			/* Get cards in hand */
			n = get_player_area(g, i, hand, WHERE_HAND);

			/* Ask player to save one card */
			ask_player(g, i, CHOICE_SAVE, hand, &n, NULL, NULL,
			           0, 0, 0);

			/* Check for aborted game */
			if (g->game_over) return;

			/* Move card to saved area */
			move_card(g, hand[0], i, WHERE_SAVED);

			/* Format message */
			sprintf(msg, "%s saves 1 card under Galactic Scavengers.\n",
			        p_ptr->name);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);

			/* Private message */
			if (g->p[i].control->private_message)
			{
				/* Format message */
				sprintf(msg, "%s saves %s.\n", p_ptr->name,
				        g->deck[hand[0]].d_ptr->name);

				/* Send message */
				g->p[i].control->private_message(g, i, msg,
				                                 FORMAT_DISCARD);
			}
		}
	}

	/* Check for takeover scenario */
	if (g->variant == VARIANT_TAKEOVER)
	{
		/* Loop over cards in deck */
		for (i = 0; i < g->deck_size; i++)
		{
			/* Get card pointer */
			c_ptr = &g->deck[i];

			/* Clear player */
			j = -1;

			/* Check for Rebel Alliance card */
			if (!strcmp(c_ptr->d_ptr->name, "Rebel Alliance"))
			{
				/* Give card to first player */
				j = 0;
			}

			/* Check for Imperium Seat card */
			else if (!strcmp(c_ptr->d_ptr->name, "Imperium Seat"))
			{
				/* Give card to second player */
				j = 1;
			}

			/* Check for no players receiving card */
			if (j < 0) continue;

			/* Give card to player */
			move_card(g, i, j, WHERE_HAND);

			/* Card's location is known to the player */
			c_ptr->known |= 1 << j;

			/* Message */
			if (!g->simulation)
			{
				/* Format message */
				sprintf(msg, "%s receives %s.\n",
				        g->p[j].name, c_ptr->d_ptr->name);

				/* Add message */
				message_add(g, msg);
			}
		}
	}

	/* Clear temporary flags on drawn cards */
	clear_temp(g);

	/* XXX Pretend settle phase to set goal progress properly */
	g->cur_action = ACT_SETTLE;
	check_goals(g);
	g->cur_action = ACT_ROUND_START;
}

/*
 * Action names.
 */
char *actname[MAX_ACTION * 2 - 1] =
{
	"Search",
	"Explore +5",
	"Explore +1,+1",
	"Develop",
	"Develop",
	"Settle",
	"Settle",
	"Consume-Trade",
	"Consume-x2",
	"Produce",
	"Prestige Explore +5",
	"Prestige Explore +1,+1",
	"Prestige Develop",
	"Prestige Develop",
	"Prestige Settle",
	"Prestige Settle",
	"Prestige Consume-Trade",
	"Prestige Consume-x2",
	"Prestige Produce",
};

/*
 * Plain action names.
 */
char *plain_actname[MAX_ACTION + 1] =
{
	"Search",
	"Explore",
	"Explore",
	"Develop",
	"Second develop",
	"Settle",
	"Second settle",
	"Consume",
	"Consume",
	"Produce",
	"End of round",
};

/*
 * Return an action name.
 */
char *action_name(int act)
{
	/* Check for prestige */
	if (act & ACT_PRESTIGE)
	{
		/* Return prestige name */
		return actname[(act & ACT_MASK) + MAX_ACTION - 1];
	}
	else
	{
		/* Just return regular name */
		return actname[act];
	}
}

/*
 * One game round.
 */
int game_round(game *g)
{
	player *p_ptr;
	int i, j, target, act[2];
	char msg[1024], last;

	/* Increment round counter */
	g->round++;

	/* Set current phase to start of round */
	g->cur_action = ACT_ROUND_START;

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Message */
	if (!g->simulation)
	{
		/* Format message */
		sprintf(msg, "=== Round %d begins ===\n", g->round);

		/* Send message */
		message_add_formatted(g, msg, FORMAT_EM);
	}

	/* Award prestige bonuses */
	start_prestige(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current turn */
		g->turn = i;

		/* Check for "select last" */
		last = count_active_flags(g, i, FLAG_SELECT_LAST);

		/* Skip players with select last except in advanced game */
		if (!g->advanced && last) continue;

		/* Set number of choices to present */
		j = 2;

		/* Ask for player's role choice(s) */
		send_choice(g, i, CHOICE_ACTION, act, &j, NULL, NULL,
		            last, 0, 0);

		/* Check for aborted game */
		if (g->game_over) return 0;
	}

	/* Wait for all responses */
	wait_for_all(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for "select last" */
		last = count_active_flags(g, i, FLAG_SELECT_LAST);

		/* Do not print anything in non-advanced game if last */
		if (last && !g->advanced) continue;

		/* Get chosen actions */
		extract_choice(g, i, CHOICE_ACTION, p_ptr->action, &j,
		               NULL, NULL);

		/* Check for real game */
		if (!g->simulation && (!g->advanced || last))
		{
			/* Format message */
			sprintf(msg, "%s chooses %s.\n", p_ptr->name,
				action_name(p_ptr->action[0]));

			if (p_ptr->action[0] == ACT_SEARCH || p_ptr->action[0] & ACT_PRESTIGE)
			{
				/* Send colored message */
				message_add_formatted(g, msg, FORMAT_PRESTIGE);
			}
			else
			{
				/* Send non-colored message */
				message_add(g, msg);
			}
		}

		/* Check for real advanced game */
		else if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s chooses %s/%s.\n", p_ptr->name,
			        action_name(p_ptr->action[0]),
			        action_name(p_ptr->action[1]));

			if (p_ptr->action[0] == ACT_SEARCH || p_ptr->action[0] & ACT_PRESTIGE ||
			    p_ptr->action[1] == ACT_SEARCH || p_ptr->action[1] & ACT_PRESTIGE)
			{
				/* Send colored message */
				message_add_formatted(g, msg, FORMAT_PRESTIGE);
			}
			else
			{
				/* Send non-colored message */
				message_add(g, msg);
			}
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Set current turn */
		g->turn = i;

		/* Only do players with "select last" */
		if (!count_active_flags(g, i, FLAG_SELECT_LAST)) continue;

		/* Set number of choices to present */
		j = 2;

		/* Get player's role choice(s) */
		ask_player(g, i, CHOICE_ACTION, p_ptr->action, &j, NULL, NULL,
		           2, 0, 0);

		/* Check for aborted game */
		if (g->game_over) return 0;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Only do player with "select last" */
		if (!count_active_flags(g, i, FLAG_SELECT_LAST)) continue;

		/* Check for real game */
		if (!g->simulation && !g->advanced)
		{
			/* Format message */
			sprintf(msg, "%s chooses %s.\n", p_ptr->name,
				action_name(p_ptr->action[0]));

			if (p_ptr->action[0] == ACT_SEARCH || p_ptr->action[0] & ACT_PRESTIGE)
			{
				/* Send colored message */
				message_add_formatted(g, msg, FORMAT_PRESTIGE);
			}
			else
			{
				/* Send non-colored message */
				message_add(g, msg);
			}
		}

		/* Check for real game */
		if (!g->simulation && g->advanced)
		{
			/* Format message */
			sprintf(msg, "%s chooses %s/%s.\n", p_ptr->name,
			        action_name(p_ptr->action[0]),
			        action_name(p_ptr->action[1]));

			if (p_ptr->action[0] == ACT_SEARCH || p_ptr->action[0] & ACT_PRESTIGE ||
			    p_ptr->action[1] == ACT_SEARCH || p_ptr->action[1] & ACT_PRESTIGE)
			{
				/* Send colored message */
				message_add_formatted(g, msg, FORMAT_PRESTIGE);
			}
			else
			{
				/* Send non-colored message */
				message_add(g, msg);
			}
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Loop over action choices */
		for (j = 0; j < 2; j++)
		{
			/* Skip empty action */
			if (p_ptr->action[j] == -1) continue;

			/* Mark action as selected */
			g->action_selected[p_ptr->action[j] & ACT_MASK] = 1;

			/* Check for prestige action selected */
			if (p_ptr->action[j] & ACT_PRESTIGE)
			{
				/* Mark prestige action as taken */
				p_ptr->prestige_action_used = 1;

				/* Spend a prestige */
				spend_prestige(g, i, 1);
			}

			/* Check for search action selected */
			if (p_ptr->action[j] == ACT_SEARCH)
			{
				/* Mark prestige/search as taken */
				p_ptr->prestige_action_used = 1;
			}
		}
	}

	/* Collapse explore actions */
	if (g->action_selected[ACT_EXPLORE_1_1])
	{
		/* Set first explore action */
		g->action_selected[ACT_EXPLORE_5_0] = 1;

		/* Clear second explore action */
		g->action_selected[ACT_EXPLORE_1_1] = 0;
	}

	/* Collapse consume actions */
	if (g->action_selected[ACT_CONSUME_X2])
	{
		/* Set first consume action */
		g->action_selected[ACT_CONSUME_TRADE] = 1;

		/* Clear second consume action */
		g->action_selected[ACT_CONSUME_X2] = 0;
	}

	/* Loop over actions in order */
	for (i = ACT_SEARCH; i <= ACT_PRODUCE; i++)
	{
		/* Set current action */
		g->cur_action = i;

		/* Skip unchosen phases */
		if (!g->action_selected[i]) continue;

		/* Check for rotation */
		check_debug_rotate(g);

		/* Check for real game */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "--- %s phase ---\n", plain_actname[i]);

			/* Add message */
			message_add_formatted(g, msg, FORMAT_PHASE);
		}

		/* Handle phase */
		switch (i)
		{
			/* Search */
			case ACT_SEARCH:

				/* Run search phase */
				phase_search(g);
				break;

			/* Explore */
			case ACT_EXPLORE_5_0:

				/* Run explore phase */
				phase_explore(g);
				break;

			/* Develop */
			case ACT_DEVELOP:
			case ACT_DEVELOP2:

				/* Run develop phase */
				phase_develop(g);
				break;

			/* Settle */
			case ACT_SETTLE:
			case ACT_SETTLE2:

				/* Run settle phase */
				phase_settle(g);
				break;

			/* Consume */
			case ACT_CONSUME_TRADE:

				/* Run consume phase */
				phase_consume(g);
				break;

			/* Produce */
			case ACT_PRODUCE:

				/* Run produce phase */
				phase_produce(g);
				break;
		}

		/* Check for aborted game */
		if (g->game_over) return 0;
	}

	/* Set current phase to end of round */
	g->cur_action = ACT_ROUND_END;

	/* Check for rotation */
	check_debug_rotate(g);

	/* Handle discard phase */
	phase_discard(g);

	/* Check for aborted game */
	if (g->game_over) return 0;

	/* Check intermediate goals */
	check_goals(g);

	/* Check for out of VPs */
	if (g->vp_pool <= 0) g->game_over = 1;

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Assume player needs 12 cards to end game */
		target = 12;

		/* Check for "game ends at 14" flag */
		if (count_active_flags(g, i, FLAG_GAME_END_14)) target = 14;

		/* Check for 12 or more cards played */
		if (count_player_area(g, i, WHERE_ACTIVE) >= target)
		{
			/* Game is over */
			g->game_over = 1;
		}

		/* Check for 15 or more prestige */
		if (g->p[i].prestige >= 15) g->game_over = 1;

		/* Copy actions to previous */
		g->p[i].prev_action[0] = g->p[i].action[0];
		g->p[i].prev_action[1] = g->p[i].action[1];
	}

	/* Check for too many rounds */
	if (g->round >= 30) g->game_over = 1;

	/* Clear phases */
	for (i = 0; i < MAX_ACTION; i++) g->action_selected[i] = 0;

	/* Clear action choices */
	for (i = 0; i < g->num_players; i++)
	{
		/* Clear both choices */
		g->p[i].action[0] = g->p[i].action[1] = -1;
	}

	/* Check for finished game */
	if (g->game_over) return 0;

	/* Continue game */
	return 1;
}

/*
 * Return non-specific military strength.
 */
int total_military(game *g, int who)
{
	power_where w_list[100];
	power *o_ptr;
	int i, n, amt = 0;

	/* Get list of settle powers */
	n = get_powers(g, who, PHASE_SETTLE, w_list);

	/* Loop over powers */
	for (i = 0; i < n; i++)
	{
		/* Get power pointer */
		o_ptr = w_list[i].o_ptr;

		/* Check for non-specific military */
		if (o_ptr->code == P3_EXTRA_MILITARY)
		{
			/* Add to military */
			amt += o_ptr->value;
		}

		/* Check for non-specific military per military world */
		if (o_ptr->code == (P3_EXTRA_MILITARY | P3_PER_MILITARY))
		{
			/* Add to military */
			amt += count_active_flags(g, who, FLAG_MILITARY);
		}

		/* Check for non-specific military per chromosome flag */
		if (o_ptr->code == (P3_EXTRA_MILITARY | P3_PER_CHROMO))
		{
			/* Add to military */
			amt += count_active_flags(g, who, FLAG_CHROMO);
		}

		/* Check for only if Imperium card active */
		if (o_ptr->code == (P3_EXTRA_MILITARY | P3_IF_IMPERIUM))
		{
			/* Check for Imperium flag */
			if (count_active_flags(g, who, FLAG_IMPERIUM))
			{
				/* Add power's value */
				amt += o_ptr->value;
			}
		}
	}

	/* Return amount of military */
	return amt;
}

/*
 * Return true if bonus criteria matches given card design.
 */
static int bonus_match(game *g, vp_bonus *v_ptr, design *d_ptr)
{
	power *o_ptr;
	int i;
	uint64_t type = v_ptr->type;

	/* Switch on bonus type */
	switch (type)
	{
		/* Production */
		case VP_NOVELTY_PRODUCTION:
		case VP_RARE_PRODUCTION:
		case VP_GENE_PRODUCTION:
		case VP_ALIEN_PRODUCTION:

			/* Check for non-good */
			if (!d_ptr->good_type) return 0;

			/* Check for windfall */
			if (d_ptr->flags & FLAG_WINDFALL) return 0;

			/* Check for correct type */
			return type == d_ptr->good_type +
			                   VP_NOVELTY_PRODUCTION - GOOD_NOVELTY;

		/* Windfall */
		case VP_NOVELTY_WINDFALL:
		case VP_RARE_WINDFALL:
		case VP_GENE_WINDFALL:
		case VP_ALIEN_WINDFALL:

			/* Check for non-good */
			if (!d_ptr->good_type) return 0;

			/* Check for non-windfall */
			if (!(d_ptr->flags & FLAG_WINDFALL)) return 0;

			/* Check for "any" kind */
			if (d_ptr->good_type == GOOD_ANY)
			{
				/* Check for current correct type */
				return type == g->oort_kind +
				             VP_NOVELTY_WINDFALL - GOOD_NOVELTY;
			}

			/* Check for correct type */
			return type == d_ptr->good_type +
			                     VP_NOVELTY_WINDFALL - GOOD_NOVELTY;

		/* Explore powers */
		case VP_DEVEL_EXPLORE:
		case VP_WORLD_EXPLORE:

			/* Skip wrong type */
			if (d_ptr->type == TYPE_WORLD &&
			    type == VP_DEVEL_EXPLORE) return 0;
			if (d_ptr->type == TYPE_DEVELOPMENT &&
			    type == VP_WORLD_EXPLORE) return 0;

			/* Loop over powers */
			for (i = 0; i < d_ptr->num_power; i++)
			{
				/* Get power pointer */
				o_ptr = &d_ptr->powers[i];

				/* Check for explore power */
				if (o_ptr->phase == PHASE_EXPLORE) return 1;
			}

			/* No explore powers */
			return 0;

		/* Trade/Consume powers */
		case VP_DEVEL_TRADE:
		case VP_WORLD_TRADE:
		case VP_DEVEL_CONSUME:
		case VP_WORLD_CONSUME:

			/* Skip worlds with development bonuses */
			if (d_ptr->type == TYPE_WORLD &&
			    (type == VP_DEVEL_TRADE ||
			     type == VP_DEVEL_CONSUME))
			{
				/* No match */
				return 0;
			}

			/* Skip developments with world bonuses */
			if (d_ptr->type == TYPE_DEVELOPMENT &&
			    (type == VP_WORLD_TRADE ||
			     type == VP_WORLD_CONSUME))
			{
				/* No match */
				return 0;
			}

			/* Loop over powers */
			for (i = 0; i < d_ptr->num_power; i++)
			{
				/* Get power pointer */
				o_ptr = &d_ptr->powers[i];

				/* Skip non-consume/trade power */
				if (o_ptr->phase != PHASE_CONSUME) continue;

				/* Check for trade power */
				if ((o_ptr->code & P4_TRADE_MASK) &&
				    (type == VP_DEVEL_TRADE ||
				     type == VP_WORLD_TRADE)) return 1;

				/* Check for consume power */
				if (!(o_ptr->code & P4_TRADE_MASK) &&
				     (type == VP_DEVEL_CONSUME ||
				      type == VP_WORLD_CONSUME)) return 1;
			}

			/* No correct powers */
			return 0;

		/* Six-cost development */
		case VP_SIX_DEVEL:

			/* Check for non-development */
			if (d_ptr->type == TYPE_WORLD) return 0;

			/* Check for correct cost */
			return d_ptr->cost == 6;

		/* Development */
		case VP_DEVEL:

			/* Check for development */
			return d_ptr->type == TYPE_DEVELOPMENT;

		/* World */
		case VP_WORLD:

			/* Check for world */
			return d_ptr->type == TYPE_WORLD;

		/* Rebel flag */
		case VP_REBEL_FLAG:

			/* Check for flag */
			return d_ptr->flags & FLAG_REBEL;

		/* Alien flag */
		case VP_ALIEN_FLAG:

			/* Check for flag */
			return d_ptr->flags & FLAG_ALIEN;

		/* Terraforming flag */
		case VP_TERRAFORMING_FLAG:

			/* Check for flag */
			return d_ptr->flags & FLAG_TERRAFORMING;

		/* Uplift flag */
		case VP_UPLIFT_FLAG:

			/* Check for flag */
			return d_ptr->flags & FLAG_UPLIFT;

		/* Imperium flag */
		case VP_IMPERIUM_FLAG:

			/* Check for flag */
			return d_ptr->flags & FLAG_IMPERIUM;

		/* Chromosome flag */
		case VP_CHROMO_FLAG:

			/* Check for flag */
			return d_ptr->flags & FLAG_CHROMO;

		/* Military flag */
		case VP_MILITARY:

			/* Check for flag */
			return d_ptr->flags & FLAG_MILITARY;

		/* Rebel military world */
		case VP_REBEL_MILITARY:

			/* Check for non-military */
			if (!(d_ptr->flags & FLAG_MILITARY)) return 0;

			/* Check for Rebel flag */
			return d_ptr->flags & FLAG_REBEL;

		/* Specific name */
		case VP_NAME:

			/* Check for correct name */
			return !strcmp(v_ptr->name, d_ptr->name);
	}

	/* Other types never match */
	return 0;
}

/*
 * Compute VPs from a given card.
 */
int compute_card_vp(game *g, int who, int which)
{
	player *p_ptr;
	card *c_ptr, *score;
	vp_bonus *v_ptr;
	int i, j, x, vp = 0, count = 0, types[6];

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get scoring card pointer */
	score = &g->deck[which];

	/* Loop over bonuses */
	for (i = 0; i < score->d_ptr->num_vp_bonus; i++)
	{
		/* Get VP bonus pointer */
		v_ptr = &score->d_ptr->bonuses[i];

		/* Check for simple bonuses */
		if (v_ptr->type == VP_THREE_VP)
		{
			/* Add bonus for VP chips */
			vp += p_ptr->vp / 3;
		}
		else if (v_ptr->type == VP_TOTAL_MILITARY)
		{
			/* Add bonus for military strength */
			vp += total_military(g, who);
		}
		else if (v_ptr->type == VP_NEGATIVE_MILITARY)
		{
			/* Add bonus for negative military strength */
			vp -= total_military(g, who);
		}
		else if (v_ptr->type == VP_PRESTIGE)
		{
			/* Add bonus for prestige */
			vp += p_ptr->prestige;
		}
		else if (v_ptr->type == VP_KIND_GOOD)
		{
			/* Clear type flags */
			for (j = 0; j < 6; j++) types[j] = 0;

			/* Start at first active card */
			x = p_ptr->head[WHERE_ACTIVE];

			/* Loop over active cards */
			for ( ; x != -1; x = g->deck[x].next)
			{
				/* Get card pointer */
				c_ptr = &g->deck[x];

				/* Skip developments */
				if (c_ptr->d_ptr->type == TYPE_DEVELOPMENT)
					continue;

				/* Check for "any" kind */
				if (c_ptr->d_ptr->good_type == GOOD_ANY)
				{
					/* Mark current kind */
					types[g->oort_kind] = 1;
				}
				else
				{
					/* Mark type */
					types[c_ptr->d_ptr->good_type] = 1;
				}
			}

			/* Count types */
			for (j = GOOD_NOVELTY; j <= GOOD_ALIEN; j++)
			{
				/* Count type if it appears */
				if (types[j]) count++;
			}

			/* Award points based on number of types */
			switch (count)
			{
				case 1: vp += 1; break;
				case 2: vp += 3; break;
				case 3: vp += 6; break;
				case 4: vp += 10; break;
			}
		}
	}

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Loop over active cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Loop over scoring card's bonuses */
		for (i = 0; i < score->d_ptr->num_vp_bonus; i++)
		{
			/* Get bonus pointer */
			v_ptr = &score->d_ptr->bonuses[i];

			/* Check for match against current power */
			if (bonus_match(g, v_ptr, c_ptr->d_ptr))
			{
				/* Add score */
				vp += v_ptr->point;

				/* Skip remaining bonuses */
				break;
			}
		}
	}

	/* Return the score */
	return vp;
}

/*
 * Score VP from active cards for the given player.
 */
static void score_game_player(game *g, int who)
{
	player *p_ptr = &g->p[who];
	card *c_ptr;
	int i, x, count;

	/* Reset goal vp */
	p_ptr->goal_vp = 0;

	/* Start with VP chips */
	p_ptr->end_vp = p_ptr->vp;

	/* Start at first active card */
	x = p_ptr->head[WHERE_ACTIVE];

	/* Loop over active cards */
	for ( ; x != -1; x = g->deck[x].next)
	{
		/* Get card pointer */
		c_ptr = &g->deck[x];

		/* Add points from card */
		p_ptr->end_vp += c_ptr->d_ptr->vp;

		/* Check for VP bonuses */
		if (c_ptr->d_ptr->num_vp_bonus)
		{
			/* Add in bonuses */
			p_ptr->end_vp += compute_card_vp(g, who, x);
		}
	}

	/* Loop over "first" goals */
	for (i = GOAL_FIRST_5_VP; i <= GOAL_FIRST_4_MILITARY; i++)
	{
		/* Skip inactive goals */
		if ((g->goal_active & (1 << i)) == 0) continue;

		/* Check for goal claimed */
		if (p_ptr->goal_claimed & (1 << i)) p_ptr->goal_vp += 3;
	}

	/* Loop over "most" goals */
	for (i = GOAL_MOST_MILITARY; i <= GOAL_MOST_CONSUME; i++)
	{
		/* Skip inactive goals */
		if ((g->goal_active & (1 << i)) == 0) continue;

		/* Get progress toward goal */
		count = g->p[who].goal_progress[i];

		/* Check for insufficient progress */
		if (count < goal_minimum(i)) continue;

		/* Check for goal claimed */
		if (p_ptr->goal_claimed & (1 << i))
		{
			/* Award most points */
			p_ptr->goal_vp += 5;
		}
		else
		{
			/* Check for as much as most */
			if (count == g->goal_most[i])
			{
				/* Award tie points */
				p_ptr->goal_vp += 3;
			}
		}
	}

	/* Add goal points to end score */
	p_ptr->end_vp += p_ptr->goal_vp;

	/* Add prestige to end score */
	p_ptr->end_vp += p_ptr->prestige;
}

/*
 * Handle end-game scoring.
 */
void score_game(game *g)
{
	game sim;
	player *p_ptr;
	card *c_ptr;
	int i, j, b_s = -999;
	int oort_owner = -1;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards that don't have "any" good type */
		if (c_ptr->d_ptr->good_type != GOOD_ANY) continue;

		/* Skip the card if it is not active */
		if (c_ptr->where != WHERE_ACTIVE) break;

		/* Remember owner of card */
		oort_owner = c_ptr->owner;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Start with no goal points */
		p_ptr->goal_vp = 0;

		/* Check for owner of "any" good type */
		if (i == oort_owner)
		{
			/* Loop over available good types */
			for (j = GOOD_NOVELTY; j <= GOOD_ALIEN; j++)
			{
				/* Simulate game */
				sim = *g;

				/* Mark game as simulation */
				sim.simulation = 1;

				/* Try this kind of world */
				sim.oort_kind = j;

				/* Check goal loss */
				check_goal_loss(&sim, i, GOAL_MOST_BLUE_BROWN);

				/* Score game for this player */
				score_game_player(&sim, i);

				/* Check for better score than before */
				if (sim.p[i].end_vp > b_s)
				{
					/* Remember best score */
					b_s = sim.p[i].end_vp;

					/* Remember best selection of kind */
					g->best_oort_kind = j;

					/* Remember goal score */
					g->p[i].goal_vp = sim.p[i].goal_vp;
				}
			}

			/* Set score to score from best type */
			p_ptr->end_vp = b_s;
		}
		else
		{
			/* Score points for active cards */
			score_game_player(g, i);
		}
	}
}

/*
 * Declare winner.
 */
void declare_winner(game *g)
{
	player *p_ptr;
	card *c_ptr;
	int i, oort_owner = -1, th, tg, b_s = -1, b_t = -1, num_b_s = 0;
	char msg[1024];

	/* Check for simulation */
	if (!g->simulation)
	{
		/* Send end of game message */
		message_add_formatted(g, "=== End of game ===\n", FORMAT_EM);
	}

	/* Score game */
	score_game(g);

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards that don't have "any" good type */
		if (c_ptr->d_ptr->good_type != GOOD_ANY) continue;

		/* Skip the card if it is not active */
		if (c_ptr->where != WHERE_ACTIVE) break;

		/* Remember owner of card */
		oort_owner = c_ptr->owner;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for bigger score */
		if (p_ptr->end_vp > b_s) b_s = p_ptr->end_vp;

		/* Check for real game and owner of "any" good type */
		if (!g->simulation && i == oort_owner)
		{
			/* Format message */
			sprintf(msg, "%s changes Alien Oort Cloud Refinery's "
			        "kind to %s.\n",
			        p_ptr->name, good_printable[g->best_oort_kind]);

			/* Send message */
			message_add_formatted(g, msg, FORMAT_VERBOSE);
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Skip players who do not have best score */
		if (p_ptr->end_vp < b_s) continue;

		/* Add one to number of players with best score */
		++num_b_s;

		/* Get tiebreaker */
		th = count_player_area(g, i, WHERE_HAND) +
		     count_player_area(g, i, WHERE_GOOD);

		/* Track biggest tiebreaker */
		if (th > b_t) b_t = th;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Check for simulation */
		if (!g->simulation)
		{
			/* Format message */
			sprintf(msg, "%s ends with %d VP%s.\n", g->p[i].name,
			        g->p[i].end_vp, PLURAL(g->p[i].end_vp));

			/* Send message */
			message_add(g, msg);
		}

		/* Skip players who do not have best score */
		if (p_ptr->end_vp < b_s) continue;

		/* Get tiebreaker (hand cards) */
		th = count_player_area(g, i, WHERE_HAND);

		/* Get tiebreaker (goods) */
		tg = count_player_area(g, i, WHERE_GOOD);

		/* Check for simulation */
		if (!g->simulation && num_b_s > 1)
		{
			/* Format message */
			sprintf(msg, "%s has %d card%s in hand and %d good%s "
			        "on worlds.\n", g->p[i].name,
			        th, PLURAL(th), tg, PLURAL(tg));

			/* Send message */
			message_add(g, msg);
		}

		/* Skip players who do not have best tiebreaker */
		if (th + tg < b_t) continue;

		/* Set winner flag */
		p_ptr->winner = 1;
	}

	/* Check for simulation */
	if (!g->simulation)
	{
		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Check for winner */
			if (p_ptr->winner)
			{
				/* Check for tie breaker needed */
				if (num_b_s > 1)
				{
					/* Format message */
					sprintf(msg, "%s wins with %d VP%s "
					        "and %d as tie breaker.\n",
					        g->p[i].name, g->p[i].end_vp,
					        PLURAL(g->p[i].end_vp), b_t);
				}
				else
				{
					/* Format message */
					sprintf(msg, "%s wins with %d VP%s.\n", g->p[i].name,
							g->p[i].end_vp, PLURAL(g->p[i].end_vp));
				}

				/* Send message */
				message_add_formatted(g, msg, FORMAT_EM);
			}
		}

		/* Check for offline game */
		if (g->session_id < 0)
		{
			/* Format seed message */
			sprintf(msg, "(The seed for this game was %u.)\n", g->start_seed);

			/* Send message */
			message_add(g, msg);
		}

		/* Check for debug game */
		if (g->debug_game)
		{
			/* Add debug note */
			message_add_formatted(g, "(Debug game.)\n", FORMAT_DEBUG);
		}
	}
}
