/*
 * theme.h
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

#ifndef SRC_THEME_H_
#define SRC_THEME_H_

class View;
class Menu;
class SelectDialog;

enum {
	MAXWALLPAPERS= 10
};

class Theme {
	friend View;
	friend Menu;
	friend SelectDialog;

	string stdPath; /* path to standard theme for fallbacks */

	/* loaded from theme.ini */
	string title;
	string author;
	string version;
	uint brickFileWidth;
	uint brickFileHeight;
	int shadowOffset;
	string fontSmallName;
	uint fontSmallSize;
	string fontNormalName;
	uint fontNormalSize;
	SDL_Color fontColorNormal;
	SDL_Color fontColorHighlight;
	uint shotFrameNum;
	uint shotAnimDelay;
	uint weaponFrameNum;
	uint weaponAnimDelay;
	uint explFrameNum;
	uint explAnimDelay;
	uint menuX, menuY, menuItemWidth, menuItemHeight;
	string menuFontNormalName, menuFontFocusName;
	uint menuFontNormalSize, menuFontFocusSize;
	SDL_Color menuFontColorNormal, menuFontColorFocus;

	Image menuBackground;
	Image wallpapers[MAXWALLPAPERS];
	uint numWallpapers;
	Image frame, frameShadow;
	GridImage bricks, bricksShadow;
	GridImage paddles, paddlesShadow;
	GridImage balls, ballsShadow;
	GridImage extras, extrasShadow;
	GridImage shot, shotShadow;
	GridImage weapon;
	GridImage explosions;
	GridImage life;
	Font fSmall, fNormal;
	Font fMenuNormal, fMenuFocus;

	Sound sReflectBrick, sReflectPaddle, sAttach;
	Sound sBrickHit, sExplosion, sEnergyHit, sShot;
	Sound sDamn, sDammit, sExcellent, sVeryGood;
	Sound sClick, sMenuClick, sMenuMotion;
	Sound sLooseLife, sExtras[EX_NUMBER];

	void addBox(Image &img, int x, int y, int w, int h);
	string testRc(const string &path, const string &fname) {
		if (fileExists(path + "/" + fname))
			return path + "/" + fname;
		else
			return stdPath + "/" + fname;
	}
public:
	Theme() : brickFileWidth(0), brickFileHeight(0), shadowOffset(0),
			fontSmallSize(0), fontNormalSize(0),
			shotFrameNum(0), shotAnimDelay(0),
			weaponFrameNum(0), weaponAnimDelay(0),
			explFrameNum(0), explAnimDelay(0),
			menuX(0), menuY(0), menuItemWidth(0), menuItemHeight(0),
			menuFontNormalSize(0), menuFontFocusSize(0),
			numWallpapers(0)
		{
		stdPath = string(DATADIR) + "/themes/Standard";
	}
	void load(string name, uint screenWidth, uint screenHeight,
				uint brickScreenWidth, uint brickScreenHeight);
};

#endif /* SRC_THEME_H_ */
