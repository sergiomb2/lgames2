/*
 * clientgame.cpp
 * (C) 2018 by Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

using namespace std;

#include "tools.h"
#include "clientgame.h"

extern GameDiff diffs[DIFF_COUNT];

ClientGame::ClientGame(Config &cfg) : config(cfg), levelset(0), game(0),
		curPlayer(0), lastDeadPlayer(NULL), msg(""), extrasActive(false),
		lastpx(-1)
{
	extrasUpdateTimeout.set(200);
}

ClientGame::~ClientGame()
{
	if (levelset)
		levelset_delete(&levelset);
	if (game)
		game_delete(&game);
}

int ClientGame::init(const string& setname, int levelid)
{
	/* kill running game if any */
	if (levelset)
		levelset_delete(&levelset);
	if (game)
		game_delete(&game);

	/* init players */
	curPlayer = 0;
	players.clear();
	for (int i = 0; i < config.player_count; i++)
		players.push_back(
			unique_ptr<ClientPlayer>(
				new ClientPlayer(config.player_names[i],
						diffs[config.diff].lives,
						diffs[config.diff].max_lives)));

	/* load levelset from install directory or home directory */
	if (setname == TOURNAMENT) {
		if (!loadAllLevels())
			return -1;
	} else if ((levelset = levelset_load(setname.c_str(), config.add_bonus_levels)) == 0) {
		_logerr("Could not load levelset %s\n",setname.c_str());
		return -1;
	}
	if (levelset->count == 0) {
		_logerr("Levelset %s is empty\n",setname.c_str());
		return -1;
	}
	/* create game context and init first level */
	if ((game = game_create(GT_LOCAL,config.diff,config.rel_warp_limit)) == 0) {
		_logerr("Could not create game context\n");
		return -1;
	}
	game->localServerGame = 1; /* for special levels */
	game_set_current(game);
	game_init(game,levelset->levels[levelid]);
	game_set_convex_paddle( config.convex );
	game_set_ball_auto_return( !config.return_on_click );
	game_set_ball_random_angle( config.random_angle );
	game_set_ball_accelerated_speed( config.maxballspeed_float );
	game->ball_auto_speedup = config.ball_auto_turbo;
	extrasActive = false;

	/* set first level as snapshot to all players */
	for (auto& p : players)
		p->setLevelSnapshot(levelset->levels[levelid]);
	return 0;
}

/** Cycle through list to get next player or NULL if none (=game over) */
ClientPlayer *ClientGame::getNextPlayer()
{
	ClientPlayer *p = NULL;
	uint startId = curPlayer;

	do {
		curPlayer++;
		if (curPlayer == players.size())
			curPlayer = 0;
		p = players[curPlayer].get();
		if (p->getLives() > 0 && p->getLevel() < (uint)levelset->count)
			return p;
	} while (curPlayer != startId);
	return NULL;
}

/** ms is passed milliseconds since last call
 * rx is either relative motion of paddle, or 0 for no motion
 * 	  or absolute mouse position (depending on config.rel_motion)
 * pis is what controls have been activated
 * return flags what has to be rendered new
 */
int ClientGame::update(uint ms, double rx, PaddleInputState &pis)
{
	int oldScore = game->paddles[0]->score;
	int ret = 0;

	/* reset old modifications (View needs current mods afterwards) */
	game_reset_mods();

	/* as long as any extra is active render active extras every 200 ms */
	if (extrasUpdateTimeout.update(ms)) {
		bool oldExtrasActive = extrasActive;
		for (int i = 0; i < EX_NUMBER; i++)
			if (game->extra_active[i]) {
				extrasActive = true;
				break;
			}
		if (!(ret & CGF_UPDATEEXTRAS))
			for (int i = 0; i < EX_NUMBER; i++)
				if (game->paddles[0]->extra_active[i]) {
					extrasActive = true;
					break;
				}
		if (extrasActive || oldExtrasActive)
			ret |= CGF_UPDATEEXTRAS;
		extrasUpdateTimeout.reset();

		/* XXX use timer to update bonus level info as well */
		if (game->isBonusLevel)
			ret |= CGF_UPDATEINFO;
	}

	/* handle paddle movement, px is resulting absolute position */
	double px = game->paddles[0]->cur_x;
	if (game->paddles[0]->frozen)
		rx = 0; /* no friction as well */
	else if (pis.left || pis.right) {
		if (pis.left)
			px -= config.key_speed * (ms << pis.turbo);
		if (pis.right)
			px += config.key_speed * (ms << pis.turbo);
	} else if (config.rel_motion)
		px += rx;
	else {
		/* for absolute position only update if new mouse
		 * position comes in to allow control by keys */
		if (rx != lastpx) {
			px = rx;
			lastpx = px;
		}
	}

	/* check friction if normal paddle has moved */
	if (!config.convex) {
		Paddle *paddle = game->paddles[0];
		if (!config.rel_motion)
			rx = px - paddle->cur_x;
		if (rx != 0 || pis.left || pis.right) {
			double diff = px - paddle->cur_x;
			paddle->v_x = diff / ms;
			/* limit mouse speed */
			if ( rx != 0 ) {
				if (paddle->v_x > 5.0) paddle->v_x = 5.0;
				if (paddle->v_x < -5.0) paddle->v_x = -5.0;
			}
			frictionTimeout.set(200);
		} else if (frictionTimeout.update(ms))
			paddle->v_x = 0;
	}

	/* update bottom paddle state */
	game_set_paddle_state(0,px,0,pis.leftFire,pis.rightFire,pis.recall);

	/* recall idle balls */
	game->paddles[0]->ball_return_key_pressed = pis.recall;

	/* update all game objects */
	game->paddles[0]->maxballspeed_request = pis.speedUp;
	game_update(ms);
	game->paddles[0]->maxballspeed_request_old = pis.speedUp;

	/* can and wants to warp */
	if (pis.warp && game->bricks_left < game->warp_limit) {
		game->level_over = 1;
		game->winner = PADDLE_BOTTOM;
	}

	/* switch level/player? */
	if (game->level_over) {
		ClientPlayer *p = players[curPlayer].get();
		/* bonus levels are just skipped on failure */
		if (game->winner == PADDLE_BOTTOM || game->level_type != LT_NORMAL) {
			if (p->nextLevel() < (uint)levelset->count)
				p->setLevelSnapshot(levelset->levels[p->getLevel()]);
			else {
				strprintf(msg,_("Congratulations, %s, you cleared all levels!"),p->getName().c_str());
				ret |= CGF_PLAYERMESSAGE;
			}
		} else {
			p->setLevelSnapshot(NULL);
			ret |= CGF_LIFELOST; /* for sound */
			if (p->looseLife() == 0) {
				strprintf(msg,_("Game over, %s!"), p->getName().c_str());
				ret |= CGF_LASTLIFELOST;
				ret |= CGF_PLAYERMESSAGE;
				lastDeadPlayer = p; /* remember for continue */
			}
		}
		p = getNextPlayer();
		if (p == NULL) {
			_logdebug(1,"Game over!\n");
			ret |= CGF_GAMEOVER;
			return ret;
		}
		_logdebug(1,"Next player: %s\n",p->getName().c_str());
		game_finalize(game);
		game_init(game,p->getLevelSnapshot());
		/* score is reset to 0 again so adjust */
		game->paddles[0]->score = p->getScore();
		ret |= CGF_NEWLEVEL;
		return ret;
	}

	/* handle (some) collected extras (most is done in game itself) */
	for (int i = 0; i < game->mod.collected_extra_count[0]; i++) {
		switch (game->mod.collected_extras[0][i]) {
		case EX_LIFE:
			players[curPlayer]->gainLife();
			ret |= CGF_UPDATEBACKGROUND; /* life is on the frame */
		break;
		}
	}

	/* handle other modifications */
	if (game->mod.brick_hit_count > 0) {
		ret |= CGF_UPDATEBRICKS | CGF_NEWANIMATIONS;
		if (game->bricks_left < game->warp_limit)
			ret |= CGF_WARPOK;
	}
	if (game->paddles[0]->score != oldScore) {
		players[curPlayer]->setScore(game->paddles[0]->score);
		ret |= CGF_UPDATESCORE;
	}
	return ret;
}

void ClientGame::updateHiscores()
{
	HiscoreChart *hs = hiscores.get(levelset->name);
	for (auto& p : players)
		hs->add(p->getName(), p->getLevel(), p->getScore());
}

/* Continue game if lastDeadPlayer is set. Clear score and set lives.
 * If current players is dead get valid next player. */
void ClientGame::continueGame()
{
	if (!lastDeadPlayer)
		return; /* should not happen */

	bool wasLastPlayer = (players[curPlayer]->getLives() == 0);
	lastDeadPlayer->setScore(0);
	lastDeadPlayer->setLives(diffs[config.diff].lives);
	lastDeadPlayer = NULL;
	if (wasLastPlayer) { /* no init in cgame.update() */
		ClientPlayer *p = getNextPlayer();
		game_finalize(game);
		game_init(game,p->getLevelSnapshot());
	}
}

int ClientGame::destroyBrick(int x, int y)
{
	if (x < 1 || y < 1 || x >= MAPWIDTH-1 || y >= MAPHEIGHT-1)
		return 0;
	if (game->bricks[x][y].type == MAP_EMPTY)
		return 0;

	/* copied from libgame/bricks.c::brick_start_expl
	 * as functions seems to get optimized out as not used in lib */
	game->bricks[x][y].exp_time = BRICK_EXP_TIME;
	game->bricks[x][y].exp_paddle = game->paddles[0];
	game->bricks[x][y].mx = x;
	game->bricks[x][y].my = y;
	game->bricks[x][y].score = -10 * players[curPlayer]->getScore() / 100;
	list_add(game->exp_bricks, &game->bricks[x][y] );
	return 1;
}

const string &ClientGame::getBonusLevelInfo()
{
	static string info;
	if (!game->isBonusLevel)
		info = "normal level";
	else switch (game->level_type) {
	case LT_BARRIER:
		strprintf(info, _("Level: %d, Size: %d/12"),
				game->blBarrierLevel, game->blBarrierLevel+2);
		break;
	case LT_SITTING_DUCKS:
		strprintf(info, _("Total Hits: %d, Current Price: %d"),
				game->blNumCompletedRuns, game->blMaxScore);
		break;
	case LT_DEFENDER:
		strprintf(info, _("Wave: %d, Invaders: %d/%d"),
				game->blNumCompletedRuns+1,
				game->blNumInvaders, game->blInvaderLimit);
		break;
	case LT_OUTBREAK:
		strprintf(info, _("Wave: %d, Infections: %d/%d, Active: %d/%d"),
				game->blNumCompletedRuns+1,
				game->blCancerCount, game->blCancerLimit,
				game->bricks_left, game->blCancerSimLimit);
		break;
	case LT_HUNTER:
		strprintf(info, _("Hits: %d, Current Price: %d, Time: %d secs"),
				game->blNumCompletedRuns, game->blMaxScore, game->blHunterTimeLeft/1000);
		break;
	case LT_JUMPING_JACK:
		strprintf(info, _("Hits: %d, Current Price: %d, Time: %d secs"),
				game->blNumCompletedRuns, game->blMaxScore, game->bl_jj_time/1000);
		break;
	}
	return info;
}

int ClientGame::loadAllLevels()
{
	vector<string> list;
	List *sets;

	readDir(string(DATADIR)+"/levels", RD_FILES, list);
#ifdef WIN32
	/* we need to make item non-const which will make compiler
	 * complain about c_str() being const. */
	sets = list_create( LIST_AUTO_DELETE, 0 );
	for (auto& s : list)
		list_add(sets, strdup(s.c_str()));
#else
	sets = list_create( LIST_NO_AUTO_DELETE, 0 );
	for (auto& s : list)
		list_add(sets, s.c_str());
#endif

	levelset = levelset_load_all(sets, config.freakout_seed,
						config.add_bonus_levels);
	list_delete(sets);

	return (levelset!=NULL);
}

/** Restart level */
int ClientGame::restartLevel()
{
	ClientPlayer *p = players[curPlayer].get();

	_logdebug(1,"Restarting level ...\n");

	p->looseLife(); /* we checked that this is not the last life */
	p->setLevelSnapshot(levelset->levels[p->getLevel()]);
	p = getNextPlayer();
	if (p == NULL)
		_logerr("Next player is NULL while restarting?!?\n");

	_logdebug(1,"Next player: %s\n",p->getName().c_str());
	game_finalize(game);
	game_init(game,p->getLevelSnapshot());
	/* score is reset to 0 again so adjust */
	game->paddles[0]->score = p->getScore();

	return CGF_RESTARTLEVEL | CGF_LIFELOST;
}
