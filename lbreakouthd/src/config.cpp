/*
 * config.cpp
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

#include "tools.h"
#include "sdl.h"

Config::Config()
{
	/* levels */
	levelset_id_home = 0;
	levelset_count_home = 0;
	/* player */
	player_count = 1;
	player_names[0] = "Michael";
	player_names[1] = "Mr.X";
	player_names[2] = "Mr.Y";
	player_names[3] = "Mr.Z";
	/* game */
	diff = 2;
	startlevel = 0;
	rel_warp_limit = 80;
	addBonusLevels = 1;
	/* controls */
	k_left = SDL_SCANCODE_LEFT;
	k_right = SDL_SCANCODE_RIGHT;
	k_rfire = SDL_SCANCODE_SPACE;
	k_lfire = SDL_SCANCODE_V;
	k_return = SDL_SCANCODE_BACKSPACE;
	k_turbo = SDL_SCANCODE_X ;
	k_warp = SDL_SCANCODE_W;
	k_maxballspeed = SDL_SCANCODE_C;
	grab = 1;
	rel_motion = 1;
	i_motion_mod = 120;
	motion_mod = 1.2;
	convex = 1;
	linear_corner = 0;
	invert = 0;
	i_key_speed = 500;
	key_speed = 0.5;
	random_angle = 1;
	maxballspeed_int1000 = 900;
	maxballspeed_float = 0.7;
	/* sounds */
	sound = 1;
	volume = 50;
	speech = 1;
	badspeech = 0;
	audio_buffer_size = 1024;
	channels = 16;
	/* graphics */
	anim = 2;
	mode = 0;
	fade = 1;
	bonus_info = 1;
	fps = 1;
	ball_level = BALL_BELOW_BONUS;
	debris_level = DEBRIS_ABOVE_BALL;
	/* various */
	use_hints = 1;
	return_on_click = 0;
	theme_id = 0;
	theme_count = 4;

	/* if config dir not found create necessary dirs */
	dname = CONFIGDIR;
	if (dname != ".") { /* . = disabled install */
		dname = string(getenv( "HOME" )?getenv( "HOME" ):".") + "/" + dname;
		if (!dirExists(dname)) {
			_loginfo("Configuration directory %s not found, creating.\n",
								dname.c_str());
			makeDir(dname);
			makeDir(dname + "/themes");
			makeDir(dname + "/levels");
		}
	}

	/* set file name */
	fname = dname + "/lbreakouthd.conf";

	/* load */
	_loginfo("Loading configuration %s\n",fname.c_str());
	FileParser fp(fname);
	fp.get("set_id_home", levelset_id_home );
	fp.get( "set_count_home", levelset_count_home );
	fp.get( "player_count", player_count );
	fp.get( "player0", player_names[0] );
	fp.get( "player1", player_names[1] );
	fp.get( "player2", player_names[2] );
	fp.get( "player3", player_names[3] );
	fp.get( "diff", diff );
	fp.get( "starting_level", startlevel );
	fp.get( "rel_warp_limit", rel_warp_limit );
	fp.get( "add_bonus_levels", addBonusLevels );
	fp.get( "left", k_left );
	fp.get( "right", k_right );
	fp.get( "fire_left", k_lfire );
	fp.get( "fire_right", k_rfire );
	fp.get( "return", k_return );
	fp.get( "turbo", k_turbo );
	fp.get( "ballturbo", k_maxballspeed );
	fp.get( "rel_motion", rel_motion );
	fp.get( "grab", grab );
	fp.get( "motion_mod", i_motion_mod );
	motion_mod = 0.01 * i_motion_mod;
	fp.get( "convex", convex );
	fp.get( "linear_corner", linear_corner );
	fp.get( "random_angle", random_angle );
	fp.get( "maxballspeed", maxballspeed_int1000 );
	maxballspeed_float = (float)maxballspeed_int1000 / 1000;
	fp.get( "invert", invert );
	fp.get( "sound", sound );
	fp.get( "volume", volume );
	fp.get( "speech", speech );
	fp.get( "badspeech", badspeech );
	fp.get( "audio_buffer_size", audio_buffer_size );
	fp.get( "channels", channels );
	fp.get( "anim", anim );
	fp.get( "mode", mode );
	fp.get( "fade", fade );
	fp.get( "bonus_info", bonus_info );
	fp.get( "fps", fps );
	fp.get( "ball_level", ball_level );
	fp.get( "debris_level", debris_level );
	fp.get( "i_key_speed", i_key_speed );
	key_speed = 0.001 * i_key_speed;
	fp.get( "use_hints", use_hints );
	fp.get( "return_on_click", return_on_click );
	fp.get( "theme_id", theme_id );
	fp.get( "theme_count", theme_count );
}

void Config::save()
{
	ofstream ofs(fname);
	if (!ofs.is_open()) {
		_logerr("Could not open config file %s\n",fname.c_str());
		/* TODO create CONFIGDIR if not . */
		return;
	}

	ofs << "set_id_home=" << levelset_id_home << "\n";
	ofs << "set_count_home=" << levelset_count_home << "\n";
	ofs << "player_count=" << player_count << "\n";
	ofs << "player0=" << player_names[0] << "\n";
	ofs << "player1=" << player_names[1] << "\n";
	ofs << "player2=" << player_names[2] << "\n";
	ofs << "player3=" << player_names[3] << "\n";
	ofs << "diff=" << diff << "\n";
	ofs << "starting_level=" << startlevel << "\n";
	ofs << "rel_warp_limit=" << rel_warp_limit << "\n";
	ofs << "add_bonus_levels=" << addBonusLevels << "\n";
	ofs << "left=" << k_left << "\n";
	ofs << "right=" << k_right << "\n";
	ofs << "fire_left=" << k_lfire << "\n";
	ofs << "fire_right=" << k_rfire << "\n";
	ofs << "return=" << k_return << "\n";
	ofs << "turbo=" << k_turbo << "\n";
	ofs << "ballturbo=" << k_maxballspeed << "\n";
	ofs << "rel_motion=" << rel_motion << "\n";
	ofs << "grab=" << grab << "\n";
	ofs << "motion_mod=" << i_motion_mod << "\n";
	ofs << "convex=" << convex << "\n";
	ofs << "linear_corner=" << linear_corner << "\n";
	ofs << "random_angle=" << random_angle << "\n";
	ofs << "maxballspeed=" << maxballspeed_int1000 << "\n";
	ofs << "invert=" << invert << "\n";
	ofs << "sound=" << sound << "\n";
	ofs << "volume=" << volume << "\n";
	ofs << "speech=" << speech << "\n";
	ofs << "badspeech=" << badspeech << "\n";
	ofs << "audio_buffer_size=" << audio_buffer_size << "\n";
	ofs << "channels=" << channels << "\n";
	ofs << "anim=" << anim << "\n";
	ofs << "mode=" << mode << "\n";
	ofs << "fade=" << fade << "\n";
	ofs << "bonus_info=" << bonus_info << "\n";
	ofs << "fps=" << fps << "\n";
	ofs << "ball_level=" << ball_level << "\n";
	ofs << "debris_level=" << debris_level << "\n";
	ofs << "i_key_speed=" << i_key_speed << "\n";
	ofs << "use_hints=" << use_hints << "\n";
	ofs << "return_on_click=" << return_on_click << "\n";
	ofs << "theme_id=" << theme_id << "\n";
	ofs << "theme_count=" << theme_count << "\n";

	ofs.close();
	_loginfo("Configuration saved to %s\n",fname.c_str());
}


