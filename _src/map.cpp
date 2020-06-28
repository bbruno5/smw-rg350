#include "global.h"
#include <iostream>
using std::cout;
using std::endl;

#ifdef PNG_SAVE_FORMAT
	#include "savepng.h"
#endif

#ifdef _WIN32
	#ifndef _XBOX
		#pragma comment(linker, "/NODEFAULTLIB:libc.lib")

		#ifdef PNG_SAVE_FORMAT
			#pragma comment(lib, "libpng.lib")
			#pragma comment(lib, "zlib.lib")
		#endif
    #endif
#endif

#define fopen(f,m) fopen(f.c_str(),m)

#define COLORKEY_FLAGS		SDL_SRCCOLORKEY /* | SDL_RLEACCEL */
#define ALPHA_FLAGS		SDL_SRCALPHA /* | SDL_RLEACCEL */

extern gfxSprite spr_frontmap;
extern short g_iTileConversion[];
extern short g_iVersion[];
extern char * g_szBackgroundConversion[26];
extern short g_iMusicCategoryConversion[26];

CMap::CMap()
{
	platforms = NULL;
	iNumPlatforms = 0;

	for(short iSwitch = 0; iSwitch < 4; iSwitch++)
		iSwitches[iSwitch] = 0;

	for(short iTile = 0; iTile < TILESETSIZE; iTile++)
	{
		iThumbTileX[iTile] = (iTile % TILESETWIDTH) * THUMBTILESIZE;
		iThumbTileY[iTile] = (iTile / TILESETWIDTH) * THUMBTILESIZE;

		iPreviewTileX[iTile] = (iTile % TILESETWIDTH) * PREVIEWTILESIZE;
		iPreviewTileY[iTile] = (iTile / TILESETWIDTH) * PREVIEWTILESIZE;

		iGameTileX[iTile] = (iTile % TILESETWIDTH) * TILESIZE;
		iGameTileY[iTile] = (iTile / TILESETWIDTH) * TILESIZE;
	}
}

CMap::~CMap()
{
	for(short iTileSet = 0; iTileSet < 3; iTileSet++)
	{
		if(tilesetsurface[iTileSet])
		{
			SDL_FreeSurface(tilesetsurface[iTileSet]);
			tilesetsurface[iTileSet] = NULL;
		}
	}
}


void CMap::loadTileSet(const std::string& tilesetfile, const std::string tilesetpng[3])
{
	FILE *tsf;
	int i;

	clearTileSet();

	//1. load tileset file
    cout << "loading tileset from " << tilesetfile << " ... ";

	tsf = fopen(tilesetfile, "rb");
	if(tsf == NULL)
	{
        cout << endl << " ERROR: couldn't open tileset file." << endl;
		return;
	}
	
	for(i = 0; i < TILESETSIZE; i++)
	{
		tileset[i] = (TileType)ReadInt(tsf);
	}

	tileset[TILESETSIZE] = tile_nonsolid;	//this is the no tile selected tile

	fclose(tsf);
    cout << "done" << endl;
	
	//2. load tileset graphics
    
	for(short iTileSet = 0; iTileSet < 3; iTileSet++)
	{
		cout << "loading tile set suface from " << tilesetpng[iTileSet] << " ... ";

		if(tilesetsurface[iTileSet] != NULL)
		{
			SDL_FreeSurface(tilesetsurface[iTileSet]);
			tilesetsurface[iTileSet] = NULL;
		}

		SDL_Surface *temp = IMG_Load(tilesetpng[iTileSet].c_str());

		if (temp == NULL)
		{
			cout << endl << " ERROR: Couldn't load " << tilesetpng[iTileSet] << ": " << SDL_GetError() << endl;
			return;
		}

		if( SDL_SetColorKey(temp, COLORKEY_FLAGS, SDL_MapRGB(temp->format, 255, 0, 255)) < 0)
		{
			cout << endl << " ERROR: Couldn't set ColorKey + RLE: " << SDL_GetError() << endl;
			return;
		}

		if( (tilesetsurface[iTileSet] = SDL_DisplayFormat(temp)) == NULL)
		{
			cout << endl << " ERROR: Couldn't convert tileset to the display's "
				<< "pixel format: " << SDL_GetError() << endl;
			return;
		}

		SDL_FreeSurface(temp);
		cout << "done" << endl;
	}
}

//With the new 32x30 tile set, we need to convert old maps to use the 
//correct indexes into the tile set
void CMap::convertMap()
{
	int i, j, k;

	bool fTilesNeedConversion = true;
	bool fBlocksNeedConversion = true;

	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			for(k = 0; k < MAPLAYERS; k++)
			{
				//if we find an empty tile (value 960), then we already have a converted map
				if(mapdata[i][j][k] == TILESETSIZE)
					fTilesNeedConversion = false;
			}

			if(objectdata[i][j] == BLOCKSETSIZE)
				fBlocksNeedConversion = false;
		}
	}

	if(!fTilesNeedConversion && !fBlocksNeedConversion)
		return;

	//Otherwise, convert this map
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			for(k = 0; k < MAPLAYERS; k++)
			{
				mapdata[i][j][k] = g_iTileConversion[mapdata[i][j][k]];
			}
		}
	}
}

void CMap::saveTileSet(const std::string& tilesetfile)
{
	FILE *mapfile;
	int i;

    cout << "saving tileset " << tilesetfile << " ... ";

	mapfile = fopen(tilesetfile, "wb");
	if(mapfile == NULL)
	{
        cout << endl << " ERROR: couldn't open tileset file." << endl;
		return;
	}

	//save tileset
	for(i = 0; i < TILESETSIZE; i++)
	{
		WriteInt(tileset[i], mapfile);
	}
	

	fclose(mapfile);

    cout << "done" << endl;
}


void CMap::clearTileSet()
{
	int i;
	for(i=0; i<TILESETSIZE; i++)
	{
		tileset[i] = tile_nonsolid;
	}
	tilebltrect.w = TILESIZE;
	tilebltrect.h = TILESIZE;
}

void CMap::clearMap()
{
	int i, j, k;

	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			for(k = 0; k < MAPLAYERS; k++)
			{
				//reset tile
				mapdata[i][j][k] = TILESETSIZE;	//no tile selected
			}

			mapdatatop[i][j] = tile_nonsolid;
			objectdata[i][j] = BLOCKSETSIZE;
			warpdata[i][j].direction = -1;
			warpdata[i][j].connection = -1;

			nospawn[0][i][j] = false;
			nospawn[1][i][j] = false;
		}
	}

	bltrect.w = TILESIZE;
	bltrect.h = TILESIZE;
}

void CMap::clearPlatforms()
{
	if(platforms)
	{
		for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
		{
			delete platforms[iPlatform];
		}

		delete [] platforms;
		platforms = NULL;
	
		iNumPlatforms = 0;
	}
}

void CMap::loadMap(const std::string& file, ReadType iReadType)
{
	FILE * mapfile;
	short i, j, k;

	/*
    cout << "loading map " << file;
	
	if(iReadType == read_type_preview)
		cout << " (preview)";
	if(iReadType == read_type_summary)
		cout << " (summary)";

	cout << " ...";
	*/

	mapfile = fopen(file, "rb");
	if(mapfile == NULL)
	{
        cout << endl << " ERROR: Couldn't open map" << endl;
		return;
	}

	//Load version number
	int version[4];
	//version[0] = ReadInt(mapfile); //Major
	//version[1] = ReadInt(mapfile); //Minor
	//version[2] = ReadInt(mapfile); //Micro
	//version[3] = ReadInt(mapfile); //Build

	//TODO: Is this any faster than reading seperate ints at a time?
	ReadIntChunk(version, 4, mapfile);

	if(version[0] == 1 && version[1] == 7)
	{
		//Read summary information here
		if((version[2] == 0 && version[3] > 1) || version[2] >= 1)
		{
			int iAutoFilterValues[NUM_AUTO_FILTERS + 1];
			ReadIntChunk(iAutoFilterValues, NUM_AUTO_FILTERS + 1, mapfile);

			for(short iFilter = 0; iFilter < NUM_AUTO_FILTERS; iFilter++)
				fAutoFilter[iFilter] = iAutoFilterValues[iFilter] > 0;

			//Read density and don't do anything with it at the moment
			//int iDensity = ReadInt(mapfile);
		}
		else
		{
			for(short iFilter = 0; iFilter < NUM_AUTO_FILTERS; iFilter++)
				fAutoFilter[iFilter] = false;
		}

		if(iReadType == read_type_summary)
		{
			fclose(mapfile);
			return;
		}

		clearPlatforms();

		cout << "loading map " << file;
	
		if(iReadType == read_type_preview)
			cout << " (preview)";
		
		cout << " ";

		cout << "[Version " << version[0] << '.' << version[1] << '.'
			<< version[2] << '.' << version[3] << " Map Detected]\n";

		//2. load map data
		for(j = 0; j < MAPHEIGHT; j++)
		{
			for(i = 0; i < MAPWIDTH; i++)
			{
				for(k = 0; k < MAPLAYERS; k++)
				{
					mapdata[i][j][k] = (short)ReadInt(mapfile);
				}

				objectdata[i][j] = (short)ReadInt(mapfile);
			}
		}


		if((version[2] == 0 && version[3] > 1) || version[2] >= 1)
		{
			//Read in background to use
			ReadString(szBackgroundFile, 128, mapfile);
			//printf("Background: %s", szBackgroundFile);
		}
		else if(version[2] == 0 && version[3] == 1)
		{
			//Read in background to use
			ReadString(szBackgroundFile, 128, mapfile);
			
			for(short iBackground = 0; iBackground < 26; iBackground++)
			{
				char * szFindUnderscore = strstr(g_szBackgroundConversion[iBackground], "_");

				if(szFindUnderscore)
					szFindUnderscore++;

				if(!strcmp(szFindUnderscore, szBackgroundFile))
				{
					strcpy(szBackgroundFile, g_szBackgroundConversion[iBackground]);
				}
			}
		}
		else
		{
			//Read old background IDs and convert that to a background filename
			backgroundID = (short)ReadInt(mapfile);
			strcpy(szBackgroundFile, g_szBackgroundConversion[backgroundID]);
		}

		if((version[2] == 0 && version[3] > 0) || version[2] >= 1)
		{
			//Read on/off switches
			for(short iSwitch = 0; iSwitch < 4; iSwitch++)
			{
				iSwitches[iSwitch] = (short)ReadInt(mapfile);
			}
		}

		if((version[2] == 0 && version[3] > 1) || version[2] >= 1)
		{
			loadPlatforms(mapfile, iReadType == read_type_preview);
		}

		//Read in eyecandy to use
		eyecandyID = (short)ReadInt(mapfile);

		if((version[2] == 0 && version[3] > 0) || version[2] >= 1)
			musicCategoryID = ReadInt(mapfile);
		else
			musicCategoryID = g_iMusicCategoryConversion[backgroundID];

		for(j = 0; j < MAPHEIGHT; j++)
		{
			for(i = 0; i < MAPWIDTH; i++)
			{
				mapdatatop[i][j] = (TileType)ReadInt(mapfile);
				warpdata[i][j].direction = (short)ReadInt(mapfile);
				warpdata[i][j].connection = (short)ReadInt(mapfile);
				warpdata[i][j].id = (short)ReadInt(mapfile);
				
				for(short iType = 0; iType < NUMSPAWNAREATYPES; iType++)
					nospawn[iType][i][j] = ReadInt(mapfile) == 0 ? false : true;
			}
		}

		if(iReadType == read_type_preview)
		{
			fclose(mapfile);
			return;
		}

		maxConnection = 0;

		numwarpexits = (short)ReadInt(mapfile);
		for(i = 0; i < numwarpexits && i < MAXWARPS; i++)
		{
			warpexits[i].direction = (short)ReadInt(mapfile);
			warpexits[i].connection = (short)ReadInt(mapfile);
			warpexits[i].id = (short)ReadInt(mapfile);
			warpexits[i].x = (short)ReadInt(mapfile);
			warpexits[i].y = (short)ReadInt(mapfile);

			warpexits[i].lockx = (short)ReadInt(mapfile);
			warpexits[i].locky = (short)ReadInt(mapfile);

			warpexits[i].warpx = (short)ReadInt(mapfile);
			warpexits[i].warpy = (short)ReadInt(mapfile);
			warpexits[i].numblocks = (short)ReadInt(mapfile);

			if(warpexits[i].connection > maxConnection)
				maxConnection = warpexits[i].connection;
		}

		//Ignore any more warps than the max
		for(i = 0; i < numwarpexits - MAXWARPS; i++)
		{
			for(j = 0; j < 10; j++)
				ReadInt(mapfile);
		}

		if(numwarpexits > MAXWARPS)
			numwarpexits = MAXWARPS;

		//Read spawn areas

		for(i = 0; i < NUMSPAWNAREATYPES; i++)
		{
			totalspawnsize[i] = 0;
			numspawnareas[i] = (short)ReadInt(mapfile);

			if(numspawnareas[i] > MAXSPAWNAREAS)
			{
				cout << endl << " ERROR: Number of spawn areas (" << numspawnareas[i]
					<< ") was greater than max allowed (" << MAXSPAWNAREAS << ')'
					<< endl;
				return;
			}

			for(int m = 0; m < numspawnareas[i]; m++)
			{
				spawnareas[i][m].left = (short)ReadInt(mapfile);
				spawnareas[i][m].top = (short)ReadInt(mapfile);
				spawnareas[i][m].width = (short)ReadInt(mapfile);
				spawnareas[i][m].height = (short)ReadInt(mapfile);
				spawnareas[i][m].size = (short)ReadInt(mapfile);
			
				totalspawnsize[i] += spawnareas[i][m].size;
			}
		}

		//Read draw areas (foreground tiles drawing optimization)
		numdrawareas = (short)ReadInt(mapfile);

		if(numdrawareas > MAXDRAWAREAS)
		{
			cout << endl << " ERROR: Number of draw areas (" << numdrawareas
                << ") was greater than max allowed (" << MAXDRAWAREAS << ')'
                << endl;
			return;
		}

		//Load rects to help optimize drawing the foreground
		for(int m = 0; m < numdrawareas; m++)
		{
			drawareas[m].x = (Sint16)ReadInt(mapfile);
			drawareas[m].y = (Sint16)ReadInt(mapfile);
			drawareas[m].w = (Uint16)ReadInt(mapfile);
			drawareas[m].h = (Uint16)ReadInt(mapfile);
		}

		if(version[2] == 0 && version[3] < 2)
		{
			loadPlatforms(mapfile, iReadType == read_type_preview);
		}

		if(version[2] == 0 && version[3] == 0)
		{
			for(short iSwitch = 0; iSwitch < 4; iSwitch++)
			{
				iSwitches[iSwitch] = (short)ReadInt(mapfile);
			}
		}
	}
	else if(version[0] == 1 && version[1] == 6)
	{
		for(short iFilter = 0; iFilter < NUM_AUTO_FILTERS; iFilter++)
			fAutoFilter[iFilter] = false;

		if(iReadType == read_type_summary)
		{
			fclose(mapfile);
			return;
		}

		clearPlatforms();

		cout << "loading map " << file;

        cout << "[Version " << version[0] << '.' << version[1] << '.'
            << version[2] << '.' << version[3] << " Map Detected]\n";

		//2. load map data
		for(j = 0; j < MAPHEIGHT; j++)
		{
			for(i = 0; i < MAPWIDTH; i++)
			{
				for(k = 0; k < MAPLAYERS; k++)
				{
					//This converts from 1.6 tileset to 1.7 tileset
					short iTile = (short)ReadInt(mapfile);

					if(iTile == 300)
						mapdata[i][j][k] = TILESETSIZE;
					else
						mapdata[i][j][k] = g_iTileConversion[iTile];
				}

				mapdatatop[i][j] = tile_nonsolid;
				for(k = MAPLAYERS - 1; k >= 0; k--)
				{
					if(tileset[mapdata[i][j][k]] != tile_nonsolid)
					{
						mapdatatop[i][j] = tileset[mapdata[i][j][k]];
						break;
					}
				}

				objectdata[i][j] = (short)ReadInt(mapfile);
				if(objectdata[i][j] == 6)
					objectdata[i][j] = BLOCKSETSIZE;

				warpdata[i][j].direction = (short)ReadInt(mapfile);
				warpdata[i][j].connection = (short)ReadInt(mapfile);
				warpdata[i][j].id = (short)ReadInt(mapfile);

				if(version[2] == 1)
				{
					nospawn[0][i][j] = ReadInt(mapfile) == 0 ? false : true;

					for(short iType = 1; iType < NUMSPAWNAREATYPES; iType++)
						nospawn[iType][i][j] = nospawn[0][i][j];
				}
			}
		}

		//Read in background to use
		backgroundID = (short)ReadInt(mapfile);
		musicCategoryID = g_iMusicCategoryConversion[backgroundID];

		strcpy(szBackgroundFile, g_szBackgroundConversion[backgroundID]);

		//Read in eyecandy to use
		eyecandyID = (short)ReadInt(mapfile);
		
		maxConnection = 0;

		numwarpexits = (short)ReadInt(mapfile);
		for(i = 0; i < numwarpexits && i < MAXWARPS; i++)
		{
			warpexits[i].direction = (short)ReadInt(mapfile);
			warpexits[i].connection = (short)ReadInt(mapfile);
			warpexits[i].id = (short)ReadInt(mapfile);
			warpexits[i].x = (short)ReadInt(mapfile);
			warpexits[i].y = (short)ReadInt(mapfile);

			warpexits[i].lockx = (short)ReadInt(mapfile);
			warpexits[i].locky = (short)ReadInt(mapfile);

			warpexits[i].warpx = (short)ReadInt(mapfile);
			warpexits[i].warpy = (short)ReadInt(mapfile);
			warpexits[i].numblocks = (short)ReadInt(mapfile);

			if(warpexits[i].connection > maxConnection)
				maxConnection = warpexits[i].connection;
		}

		//Ignore any more warps than the max
		for(i = 0; i < numwarpexits - MAXWARPS; i++)
		{
			for(j = 0; j < 10; j++)
				ReadInt(mapfile);
		}

		if(numwarpexits > MAXWARPS)
			numwarpexits = MAXWARPS;

		//Read spawn areas
		totalspawnsize[0] = 0;
		numspawnareas[0] = (short)ReadInt(mapfile);

		if(numspawnareas[0] > MAXSPAWNAREAS)
		{
            cout << endl << " ERROR: Number of spawn areas (" << numspawnareas[0]
                 << ") was greater than max allowed (" << MAXSPAWNAREAS << ')'
                 << endl;
			return;
		}

		//Read the only spawn area definition in the file
		for(int m = 0; m < numspawnareas[0]; m++)
		{
			spawnareas[0][m].left = (short)ReadInt(mapfile);
			spawnareas[0][m].top = (short)ReadInt(mapfile);
			spawnareas[0][m].width = (short)ReadInt(mapfile);
			spawnareas[0][m].height = (short)ReadInt(mapfile);
			spawnareas[0][m].size = (short)ReadInt(mapfile);
		
			if(version[2] == 0)
			{
				spawnareas[0][m].width -= spawnareas[0][m].left;
				spawnareas[0][m].height -= spawnareas[0][m].top;
			}

			totalspawnsize[0] += spawnareas[0][m].size;
		}
		
		//Then duplicate it for all the other spawn areas
		for(short i = 1; i < NUMSPAWNAREATYPES; i++)
		{
			totalspawnsize[i] = 0;
			numspawnareas[i] = numspawnareas[0];

			//Read the only spawn area definition in the file
			for(int m = 0; m < numspawnareas[0]; m++)
			{
				spawnareas[i][m].left = spawnareas[0][m].left;
				spawnareas[i][m].top = spawnareas[0][m].top;
				spawnareas[i][m].width = spawnareas[0][m].width;
				spawnareas[i][m].height = spawnareas[0][m].height;
				spawnareas[i][m].size = spawnareas[0][m].size;
			
				if(version[2] == 0)
				{
					spawnareas[i][m].width -= spawnareas[i][m].left;
					spawnareas[i][m].height -= spawnareas[i][m].top;
				}

				totalspawnsize[i] += spawnareas[i][m].size;
			}
		}

		if(version[2] == 1)
		{
			numdrawareas = (short)ReadInt(mapfile);

			if(numdrawareas > MAXDRAWAREAS)
			{
            cout << endl << " ERROR: Number of draw areas (" << numdrawareas
                 << ") was greater than max allowed (" << MAXDRAWAREAS << ')'
                 << endl;
				return;
			}

			for(int m = 0; m < numdrawareas; m++)
			{
				drawareas[m].x = (Sint16)ReadInt(mapfile);
				drawareas[m].y = (Sint16)ReadInt(mapfile);
				drawareas[m].w = (Uint16)ReadInt(mapfile);
				drawareas[m].h = (Uint16)ReadInt(mapfile);
			}
		}

		for(short iSwitch = 0; iSwitch < 4; iSwitch++)
			iSwitches[iSwitch] = 0;
	}
	else //If the version is unrecognized (1.5 maps didn't have version numbers)
	{
		for(short iFilter = 0; iFilter < NUM_AUTO_FILTERS; iFilter++)
			fAutoFilter[iFilter] = false;

		if(iReadType == read_type_summary)
		{
			fclose(mapfile);
			return;
		}

		clearPlatforms();

		cout << "loading map " << file;
		cout << "[Version 1.5 Map Detected]\n";

		//Reset position of read cursor
		rewind(mapfile);

		//clear map (we won't be reading in all the layers so it needs to be cleared)
		clearMap();
		
		//2. load map data
		for(j = 0; j < MAPHEIGHT; j++)
		{
			for(i = 0; i < MAPWIDTH; i++)
			{
				//Read everything into layer 1
				mapdata[i][j][1] = (short)ReadInt(mapfile);
			}
		}

		//3. load objects data
		for(j = 0; j < MAPHEIGHT; j++)
		{
			for(i = 0; i < MAPWIDTH; i++)
			{
				objectdata[i][j] = (short)ReadInt(mapfile);
				if(objectdata[i][j] == 6)
					objectdata[i][j] = BLOCKSETSIZE;
			}
		}

		backgroundID = (short)ReadInt(mapfile);
		musicCategoryID = g_iMusicCategoryConversion[backgroundID];

		strcpy(szBackgroundFile, g_szBackgroundConversion[backgroundID]);
		
		//All 1.5 maps will use cloud eyecandy
		eyecandyID = 1;

		for(short iSwitch = 0; iSwitch < 4; iSwitch++)
			iSwitches[iSwitch] = 0;
	}

	fclose(mapfile);
	clearWarpLocks();

    cout << " done" << endl;
}

void CMap::loadPlatforms(FILE * mapfile, bool fPreview)
{
	clearPlatforms();

	//Load moving platforms
	iNumPlatforms = (short)ReadInt(mapfile);
	platforms = new MovingPlatform*[iNumPlatforms];

	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		short iWidth = (short)ReadInt(mapfile);
		short iHeight = (short)ReadInt(mapfile);

		short ** tiles = new short*[iWidth];

		for(short iCol = 0; iCol < iWidth; iCol++)
		{
			tiles[iCol] = new short[iHeight];

			for(short iRow = 0; iRow < iHeight; iRow++)
			{
				tiles[iCol][iRow] = (short)ReadInt(mapfile);
			}
		}
	
		float fStartX = ReadFloat(mapfile);
		float fStartY = ReadFloat(mapfile);
		float fEndX = ReadFloat(mapfile);
		float fEndY = ReadFloat(mapfile);
		float fVelocity = ReadFloat(mapfile);
		
		MovingPlatformPath * path = new MovingPlatformPath(fVelocity, fStartX, fStartY, fEndX, fEndY);

		platforms[iPlatform] = new MovingPlatform(tiles, iWidth, iHeight, path, true, 0, fPreview);
	}
}

void CMap::saveMap(const std::string& file)
{
	FILE *mapfile;
	int i, j, k;

    cout << "saving map " << file << " ... ";

	mapfile = fopen(file, "wb");
	if(mapfile == NULL)
	{
        cout << endl << " ERROR: couldn't save map" << endl;
		return;
	}

	//First write the map compatibility version number 
	//(this will allow the map loader to identify if the map needs conversion)
	WriteInt(g_iVersion[0], mapfile); //Major
	WriteInt(g_iVersion[1], mapfile); //Minor
	WriteInt(g_iVersion[2], mapfile); //Micro
	WriteInt(g_iVersion[3], mapfile); //Build

	//Calculate warp zones
	bool usedtile[MAPWIDTH][MAPHEIGHT];
	for(j = 0; j < MAPHEIGHT; j++)
		for(i = 0; i < MAPWIDTH; i++)
			usedtile[i][j] = false;

	//bool fAutoMapFilters[NUM_AUTO_FILTERS];
	//for(short iFilter = 0; iFilter < NUM_AUTO_FILTERS; iFilter++)
		//fAutoMapFilters[iFilter] = false;

	int iHazardCount = 0;
	int iWarpCount = 0;
	int iIceCount = 0;
	int iPowerupBlockCount = 0;
	int iPlatformCount = 0;
	int iDensity = 0;
	int iOnOffBlockCount = 0;
	int iThrowBlockCount = 0;
	int iBreakableBlockCount = 0;

	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		for(short iCol = 0; iCol < platforms[iPlatform]->iTileWidth; iCol++)
		{
			for(short iRow = 0; iRow < platforms[iPlatform]->iTileHeight; iRow++)	
			{
				int iPlatformTile = platforms[iPlatform]->iTileData[iCol][iRow];

				if(iPlatformTile != TILESETSIZE)
					iPlatformCount++;

				if(tileset[iPlatformTile] == tile_death || tileset[iPlatformTile] == tile_death_on_top || tileset[iPlatformTile] == tile_death_on_bottom)
					iHazardCount++;

				if(tileset[iPlatformTile] == tile_ice)
					iIceCount++;
			}
		}
	}

	short numWarps = 0;
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			if(warpdata[i][j].connection != -1 && !usedtile[i][j])
			{
				int movex = 0, movey = 0;
				int currentx = i, currenty = j;

				if(warpdata[i][j].direction == 0 || warpdata[i][j].direction == 2)
					movex = 1;
				else
					movey = 1;

				while(currentx < MAPWIDTH && currenty < MAPHEIGHT)
				{
					if(warpdata[currentx][currenty].direction != warpdata[i][j].direction ||
						warpdata[currentx][currenty].connection != warpdata[i][j].connection)
					{
						break;
					}

					usedtile[currentx][currenty] = true;
					
					if(numWarps < MAXWARPS)
					{
						warpdata[currentx][currenty].id = numWarps;
					}
					else
					{
						warpdata[currentx][currenty].connection = -1;
						warpdata[currentx][currenty].direction = -1;
					}

					currentx += movex;
					currenty += movey;
				}

				numWarps++;
			}

			//Calculate auto map filters
			if(mapdatatop[i][j] == tile_death || mapdatatop[i][j] == tile_death_on_top || mapdatatop[i][j] == tile_death_on_bottom)
				iHazardCount++;

			if(warpdata[i][j].connection != -1)
				iWarpCount++;

			if(mapdatatop[i][j] == tile_ice)
				iIceCount++;

			if(objectdata[i][j] == 1) //Powerup Block
				iPowerupBlockCount++;

			if(objectdata[i][j] == 0) //Breakable Block
				iBreakableBlockCount++;
			
			if(objectdata[i][j] == 6) //Throw Block
				iThrowBlockCount++;

			if(objectdata[i][j] >= 11 && objectdata[i][j] <= 14) //On/Off Block
				iOnOffBlockCount++;

			if(mapdatatop[i][j] != tile_nonsolid && mapdatatop[i][j] != tile_solid_on_top)
				iDensity++;
		}
	}

	//Save summary info for auto map filters (hazards, ice, warps, platforms, bonus blocks, density, etc)
	WriteInt(iHazardCount, mapfile);
	WriteInt(iWarpCount, mapfile);
	WriteInt(iIceCount, mapfile);
	WriteInt(iPowerupBlockCount, mapfile);
	WriteInt(iBreakableBlockCount, mapfile);
	WriteInt(iThrowBlockCount, mapfile);
	WriteInt(iOnOffBlockCount, mapfile);
	WriteInt(iPlatformCount, mapfile);
	WriteInt(iDensity, mapfile);

	//save map tiles and blocks
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			for(k = 0; k < MAPLAYERS; k++)
			{
				WriteInt(mapdata[i][j][k], mapfile);
			}
			
			WriteInt(objectdata[i][j], mapfile);
		}
	}

	//Write background File
	WriteString(szBackgroundFile, mapfile);

	//Save the default on/off switch states
	for(short iSwitch = 0; iSwitch < 4; iSwitch++)
		WriteInt(iSwitches[iSwitch], mapfile);

	//Write moving platforms
	WriteInt(iNumPlatforms, mapfile);

	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		WriteInt(platforms[iPlatform]->iTileWidth, mapfile);
		WriteInt(platforms[iPlatform]->iTileHeight, mapfile);

		for(short iCol = 0; iCol < platforms[iPlatform]->iTileWidth; iCol++)
		{
			for(short iRow = 0; iRow < platforms[iPlatform]->iTileHeight; iRow++)	
			{
				WriteInt(platforms[iPlatform]->iTileData[iCol][iRow], mapfile);
			}
		}

		WriteFloat(platforms[iPlatform]->pPath->fStartX, mapfile);
		WriteFloat(platforms[iPlatform]->pPath->fStartY, mapfile);
		WriteFloat(platforms[iPlatform]->pPath->fEndX, mapfile);
		WriteFloat(platforms[iPlatform]->pPath->fEndY, mapfile);
		WriteFloat(platforms[iPlatform]->pPath->fVelocity, mapfile);
	}

	//Write eyecandy ID
	WriteInt(eyecandyID, mapfile);

	/*
	//This code runs though the backgrounds and assigns them the default music category
	for(short iBackground = 0; iBackground < 26; iBackground++)
	{
		if(!strcmp(g_szBackgroundConversion[iBackground], szBackgroundFile))
		{
			musicCategoryID = g_iMusicCategoryConversion[iBackground];
		}
	}
	*/

	//Write music category
	WriteInt(musicCategoryID, mapfile);

	//Write the rest of the map data later so that we can just load part of the map for the preview
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			/*
			//Figure out what the top tile's type is
			TileType type = tile_nonsolid;
			for(k = MAPLAYERS - 1; k >= 0; k--)
			{
				if(tileset[mapdata[i][j][k]] != tile_nonsolid)
				{
					type = tileset[mapdata[i][j][k]];
					break;
				}
			}
			*/
			
			WriteInt(mapdatatop[i][j], mapfile);
			WriteInt(warpdata[i][j].direction, mapfile);
			WriteInt(warpdata[i][j].connection, mapfile);
			WriteInt(warpdata[i][j].id, mapfile);
			
			for(short iType = 0; iType < NUMSPAWNAREATYPES; iType++)
				WriteInt((int)nospawn[iType][i][j], mapfile);
		}
	}

	//Write number of warp zones
	WriteInt(numWarps, mapfile);

	for(j = 0; j < MAPHEIGHT; j++)
		for(i = 0; i < MAPWIDTH; i++)
			usedtile[i][j] = false;

	//scan warp data and write out warp exits
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			//if we found a warp area and it hasn't been consumed by another warp area
			//then scan for surrounding warp areas and consume them to create a warp exit
			if(warpdata[i][j].connection != -1 && !usedtile[i][j])
			{
				int movex = 0, movey = 0;
				int currentx = i, currenty = j;

				if(warpdata[i][j].direction == 0 || warpdata[i][j].direction == 2)
					movex = 1;
				else
					movey = 1;


				int numblocks = 0;
				while(currentx < MAPWIDTH && currenty < MAPHEIGHT)
				{
					if(warpdata[currentx][currenty].direction != warpdata[i][j].direction ||
						warpdata[currentx][currenty].connection != warpdata[i][j].connection)
					{
						break;
					}
					
					usedtile[currentx][currenty] = true;

					currentx += movex;
					currenty += movey;
					numblocks++;
				}

				//We're on the next block that isn't a warp
				currentx -= movex;
				currenty -= movey;

				WriteInt(warpdata[i][j].direction, mapfile);
				WriteInt(warpdata[i][j].connection, mapfile);
				WriteInt(warpdata[i][j].id, mapfile);
				
				if(warpdata[i][j].direction == 0)
				{
					//Write out warp exit for player
					WriteInt(((currentx * TILESIZE + TILESIZE - i * TILESIZE) >> 1) + i * TILESIZE - HALFPW, mapfile);
					WriteInt(j * TILESIZE - 1 + PHOFFSET, mapfile);

					//Write out position for lock icon
					WriteInt(((currentx * TILESIZE + TILESIZE - i * TILESIZE) >> 1) + i * TILESIZE - 16, mapfile);
					WriteInt(j * TILESIZE, mapfile);
				}
				else if(warpdata[i][j].direction == 2)
				{
					WriteInt(((currentx * TILESIZE + TILESIZE - i * TILESIZE) >> 1) + i * TILESIZE - HALFPW, mapfile);
					WriteInt(j * TILESIZE + 1 + PHOFFSET, mapfile);

					WriteInt(((currentx * TILESIZE + TILESIZE - i * TILESIZE) >> 1) + i * TILESIZE - 16, mapfile);
					WriteInt(j * TILESIZE, mapfile);
				}
				else if(warpdata[i][j].direction == 1)
				{
					WriteInt(i * TILESIZE + TILESIZE - PW - PWOFFSET, mapfile);
					WriteInt(currenty * TILESIZE + TILESIZE - PH - 1, mapfile);

					WriteInt(i * TILESIZE, mapfile);
					WriteInt(((currenty * TILESIZE + TILESIZE - j * TILESIZE) >> 1) + j * TILESIZE - 16, mapfile);
				}
				else if(warpdata[i][j].direction == 3)
				{
					WriteInt(i * TILESIZE - 1 + PWOFFSET, mapfile);
					WriteInt(currenty * TILESIZE + TILESIZE - PH - 1, mapfile);

					WriteInt(i * TILESIZE, mapfile);
					WriteInt(((currenty * TILESIZE + TILESIZE - j * TILESIZE) >> 1) + j * TILESIZE - 16, mapfile);
				}

				WriteInt(i, mapfile);
				WriteInt(j, mapfile);
				WriteInt(numblocks, mapfile);

			}
		}
	}

	calculatespawnareas(0, false);
	
	if(numspawnareas[0] == 0)
		calculatespawnareas(0, true);

	calculatespawnareas(1, false);

	//Write spawn areas
	for(i = 0; i < NUMSPAWNAREATYPES; i++)
	{
		WriteInt(numspawnareas[i], mapfile);

		for(int m = 0; m < numspawnareas[i]; m++)
		{
			WriteInt(spawnareas[i][m].left, mapfile);
			WriteInt(spawnareas[i][m].top, mapfile);
			WriteInt(spawnareas[i][m].width, mapfile);
			WriteInt(spawnareas[i][m].height, mapfile);
			WriteInt(spawnareas[i][m].size, mapfile);
		}
	}

	
	for(j = 0; j < MAPHEIGHT; j++)
		for(i = 0; i < MAPWIDTH; i++)
			usedtile[i][j] = false;

	//Figure out areas that contain tiles in layers 2 and 3
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			if(mapdata[i][j][2] == TILESETSIZE && mapdata[i][j][3] == TILESETSIZE)
				usedtile[i][j] = true;
		}
	}

	//Figure out where the overlayer draw rects are
	numdrawareas = 0;
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			if(!usedtile[i][j])
			{
				bool fDownDone = false;
				bool fRightDone = false;

				int downsize = j + 1;
				int rightsize = i + 1;
				int attempt = 1;

				while(true)
				{
					if(!fRightDone)
					{
						for(int right = i; right < rightsize; right++)
						{
							if(right >= MAPWIDTH || j + attempt >= MAPHEIGHT || usedtile[right][j + attempt])
								fRightDone = true;
						}

						if(!fRightDone)
							downsize++;
					}

					if(!fDownDone)
					{
						for(int down = j; down < downsize; down++)
						{
							if(i + attempt >= MAPWIDTH || down >= MAPHEIGHT || usedtile[i + attempt][down])
								fDownDone = true;
						}

						if(!fDownDone)
							rightsize++;
					}

					if(fDownDone && fRightDone)
					{
						if(numdrawareas < MAXDRAWAREAS)
						{
							drawareas[numdrawareas].x = (Sint16)(i * TILESIZE);
							drawareas[numdrawareas].y = (Sint16)(j * TILESIZE);
							drawareas[numdrawareas].w = (Uint16)((rightsize - i) * TILESIZE);
							drawareas[numdrawareas].h = (Uint16)((downsize - j) * TILESIZE);

							numdrawareas++;
						}

						for(int down = j; down < downsize; down++)
						{
							for(int right = i; right < rightsize; right++)
							{
								usedtile[right][down] = true;
							}
						}
						
						break;
					}

					attempt++;
				}
			}
		}
	}

	//Write draw areas
	WriteInt(numdrawareas, mapfile);

	for(int m = 0; m < numdrawareas; m++)
	{
		WriteInt(drawareas[m].x, mapfile);
		WriteInt(drawareas[m].y, mapfile);
		WriteInt(drawareas[m].w, mapfile);
		WriteInt(drawareas[m].h, mapfile);
	}

	fclose(mapfile);

    cout << "done" << endl;

/*
	//Save thumbnail
	char szSaveThumbnail[256];
	strcpy(szSaveThumbnail, "maps/cache/");
	char * pszSaveThumbnail = szSaveThumbnail + strlen(szSaveThumbnail);
	GetNameFromFileName(pszSaveThumbnail, file.c_str());

#ifdef PNG_SAVE_FORMAT	
	strcat(szSaveThumbnail, ".png");
#else
	strcat(szSaveThumbnail, ".bmp");
#endif

	saveThumbnail(convertPath(szSaveThumbnail), true);
*/
}

//Save thumbnail image
void CMap::saveThumbnail(const std::string &sFile, bool fUseClassicPack)
{
	SDL_Surface * sThumbnail = SDL_CreateRGBSurface(screen->flags, 160, 120, 16, 0, 0, 0, 0);

	char szBackgroundFile[128];
	std::string path;

	if(fUseClassicPack)
	{
		sprintf(szBackgroundFile, "gfx/packs/Classic/backgrounds/%s", g_map.szBackgroundFile);
		path = convertPath(szBackgroundFile);

		//if the background file doesn't exist, use the classic background
		if(!File_Exists(path))
			path = convertPath("gfx/packs/Classic/backgrounds/Land_Classic.png");
	}
	else
	{
		sprintf(szBackgroundFile, "gfx/packs/backgrounds/%s", g_map.szBackgroundFile);
		path = convertPath(szBackgroundFile, gamegraphicspacklist.current_name());

		//if the background file doesn't exist, use the classic background
		if(!File_Exists(path))
			path = convertPath("gfx/packs/backgrounds/Land_Classic.png", gamegraphicspacklist.current_name());
	}

	SDL_Surface * temp = IMG_Load(path.c_str());

	SDL_Surface * sBackground = SDL_DisplayFormat(temp);
	if(!sBackground)
	{
		printf("ERROR: Couldn't convert thumbnail background to diplay pixel format: %s\n", SDL_GetError());
		return;
	}

	SDL_FreeSurface(temp);

	SDL_Rect srcRectBackground = {0, 0, 640, 480};
	SDL_Rect dstRectBackground = {0, 0, 160, 120};

	if(SDL_SoftStretch(sBackground, &srcRectBackground, sThumbnail, &dstRectBackground) < 0)
	{
		fprintf(stderr, "SDL_SoftStretch error: %s\n", SDL_GetError());
		return;
	}

	SDL_FreeSurface(sBackground);

	preDrawPreviewBackground(sThumbnail, true);
	drawThumbnailPlatforms(sThumbnail);
	preDrawPreviewForeground(sThumbnail, true);
	preDrawPreviewWarps(sThumbnail, true);

	//Save the screenshot with the same name as the map file

#ifdef PNG_SAVE_FORMAT
	IMG_SavePNG(sThumbnail, sFile.c_str());
#else
	SDL_SaveBMP(sThumbnail, sFile.c_str());
#endif

}

void CMap::calculatespawnareas(short iType, bool fUseTempBlocks)
{
	bool usedtile[MAPWIDTH][MAPHEIGHT];
	short i, j;

	for(j = 0; j < MAPHEIGHT; j++)
		for(i = 0; i < MAPWIDTH; i++)
			usedtile[i][j] = false;

	//Figure out areas where spawning is allowed
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			bool fUsed = false;

			if(j >= 13 || j == 0)
				fUsed = true;

			if(!fUsed)
			{
				if(nospawn[iType][i][j])
					fUsed = true;
			}

			if(!fUsed && mapdatatop[i][j] != tile_nonsolid && mapdatatop[i][j] != tile_solid_on_top)
			{
				fUsed = true;
			}
			
			//If there is a block there
			if(!fUsed)
			{
				if(objectdata[i][j] != BLOCKSETSIZE)
					fUsed = true;
			}

			//If this is a player spawn area, have extra restrictions like don't spawn over spikes etc.
			if(0 == iType)
			{
				//If there is a death tile directly above
				if(!fUsed)
				{
					if(j > 0)
					{
						if(mapdatatop[i][j - 1] == tile_death_on_bottom || mapdatatop[i][j - 1] == tile_death)
						{
							fUsed = true;
						}
					}
				}

				//If there is a death tile anywhere below this tile
				if(!fUsed)
				{
					int m;
					for(m = j; m < MAPHEIGHT; m++)
					{
						if(mapdatatop[i][m] == tile_death_on_top || mapdatatop[i][m] == tile_death)
						{
							fUsed = true;
							break;
						}
						
						if(fUseTempBlocks)
						{
							if(mapdatatop[i][m] != tile_nonsolid || objectdata[i][m] != BLOCKSETSIZE)
							{
								break;
							}
						}
						else
						{
							if(mapdatatop[i][m] != tile_nonsolid || (objectdata[i][m] != BLOCKSETSIZE && objectdata[i][m] != 0 && objectdata[i][m] != 2 && objectdata[i][m] != 6 && objectdata[i][m] < 11))
							{
								break;
							}
						}
					}

					//If we didn't find a landing spot from here to bottom, then try to wrap around and see
					if(m == MAPHEIGHT)
					{
						for(m = 0; m < j; m++)
						{
							if(mapdatatop[i][m] == tile_death_on_top || mapdatatop[i][m] == tile_death)
							{
								fUsed = true;
								break;
							}
							
							if(fUseTempBlocks)
							{
								if(mapdatatop[i][m] != tile_nonsolid || objectdata[i][m] != BLOCKSETSIZE)
								{
									break;
								}
							}
							else
							{
								if(mapdatatop[i][m] != tile_nonsolid || (objectdata[i][m] != BLOCKSETSIZE && objectdata[i][m] != 0 && objectdata[i][m] != 2))
								{
									break;
								}
							}
						}

						if(m == j)
							fUsed = true;
					}
				}
			}

			usedtile[i][j] = fUsed;
		}
	}

	//Figure out where the spawn areas are
	numspawnareas[iType] = 0;
	for(j = 0; j < MAPHEIGHT; j++)
	{
		for(i = 0; i < MAPWIDTH; i++)
		{
			if(!usedtile[i][j])
			{
				bool fDownDone = false;
				bool fRightDone = false;

				short downsize = j + 1;
				short rightsize = i + 1;
				short attempt = 1;

				while(true)
				{
					if(!fRightDone)
					{
						for(int right = i; right < rightsize; right++)
						{
							if(right >= MAPWIDTH || j + attempt >= MAPHEIGHT || usedtile[right][j + attempt])
								fRightDone = true;
						}

						if(!fRightDone)
							downsize++;
					}

					if(!fDownDone)
					{
						for(int down = j; down < downsize; down++)
						{
							if(i + attempt >= MAPWIDTH || down >= MAPHEIGHT || usedtile[i + attempt][down])
								fDownDone = true;
						}

						if(!fDownDone)
							rightsize++;
					}

					if(fDownDone && fRightDone)
					{
						if(numspawnareas[iType] < MAXSPAWNAREAS)
						{
							spawnareas[iType][numspawnareas[iType]].left = i;
							spawnareas[iType][numspawnareas[iType]].top = j;
							spawnareas[iType][numspawnareas[iType]].width = rightsize - 1 - i;
							spawnareas[iType][numspawnareas[iType]].height = downsize - 1 - j;
							spawnareas[iType][numspawnareas[iType]].size = (rightsize - i) * (downsize - j);

							numspawnareas[iType]++;
						}

						for(short down = j; down < downsize; down++)
						{
							for(short right = i; right < rightsize; right++)
							{
								usedtile[right][down] = true;
							}
						}
						
						break;
					}

					attempt++;
				}
			}
		}
	}
}



void CMap::draw(SDL_Surface *targetSurface, int layer, bool fForeground)
{
	int i, j, ts;

	CEyecandyContainer * animatedtilesdest = &animatedtilesback;
	if(fForeground)
		animatedtilesdest = &animatedtilesfront;

	//draw left to right full vertical
	bltrect.x = 0;
	for(i = 0; i < MAPWIDTH; i++)
	{
		bltrect.y = -TILESIZE;	//this is okay, see

		for(j = 0; j < MAPHEIGHT; j++)
		{
			bltrect.y += TILESIZE;	// here

			ts = mapdata[i][j][layer];
			if(ts == TILESETSIZE)
				continue;

			tilebltrect.x = iGameTileX[ts];
			tilebltrect.y = iGameTileY[ts];

			if(ts >= 940 && ts <= 942)
				animatedtilesdest->add(new EC_LoopingAnimation(&spr_maplava, bltrect.x, bltrect.y, 3, 8, 0, 0, TILESIZE, ts - 940, TILESIZE, TILESIZE));
			else if(ts >= 652 && ts <= 655)
				animatedtilesdest->add(new EC_LoopingAnimation(&spr_mapwater, bltrect.x, bltrect.y, 4, 8, 0, 0, 0, ts - 652, TILESIZE, TILESIZE));
			else if(ts == 559 || ts == 495 || ts == 780)
				animatedtilesdest->add(new EC_LoopingAnimation(&spr_mapwaterfall, bltrect.x, bltrect.y, 4, 8, 0, 0, 0, 0, TILESIZE, TILESIZE));
			else if(ts == 591 || ts == 527 || ts == 812)
				animatedtilesdest->add(new EC_LoopingAnimation(&spr_mapwaterfall, bltrect.x, bltrect.y, 4, 8, 0, 0, TILESIZE, 0, TILESIZE, TILESIZE));
			else if(ts == 817)
				animatedtilesdest->add(new EC_LoopingAnimation(&spr_maplamp, bltrect.x, bltrect.y, 4, 8, 0, 0, 0, 0, TILESIZE, TILESIZE));
			else
				SDL_BlitSurface(tilesetsurface[0], &tilebltrect, targetSurface, &bltrect);
		}

		bltrect.x += TILESIZE;
	}

	bltrect.x = 0;
	bltrect.y = 0;
	bltrect.w = 640;
	bltrect.h = 480;
}

void CMap::drawThumbnailPlatforms(SDL_Surface * targetSurface)
{
	blitdest = targetSurface;

	short iTileSize = 8;
	//Draw platforms to screenshot
	SDL_Rect rSrc = {0, 0, iTileSize, iTileSize};
	SDL_Rect rDst = {0, 0, iTileSize, iTileSize};

	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		short iStartX = ((short)platforms[iPlatform]->pPath->fStartX - platforms[iPlatform]->iHalfWidth) / PREVIEWTILESIZE;
		short iStartY = ((short)platforms[iPlatform]->pPath->fStartY - platforms[iPlatform]->iHalfHeight) / PREVIEWTILESIZE;

		for(short iPlatformX = 0; iPlatformX < platforms[iPlatform]->iTileWidth; iPlatformX++)
		{
			for(short iPlatformY = 0; iPlatformY < platforms[iPlatform]->iTileHeight; iPlatformY++)
			{
				short iTile = platforms[iPlatform]->iTileData[iPlatformX][iPlatformY];

				if(iTile != TILESETSIZE)
				{
					rSrc.x = iTile % TILESETWIDTH * iTileSize;
					rSrc.y = iTile / TILESETWIDTH * iTileSize;

					rDst.x = (iStartX + iPlatformX) * iTileSize;
					rDst.y = (iStartY + iPlatformY) * iTileSize;

					SDL_BlitSurface(tilesetsurface[2], &rSrc, blitdest, &rDst);
				}
			}
		}
	}

	//And add platform paths
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		MovingPlatformPath * pPath = platforms[iPlatform]->pPath;
		short iStartX = ((short)pPath->fStartX - platforms[iPlatform]->iHalfWidth) / PREVIEWTILESIZE;
		short iStartY = ((short)pPath->fStartY - platforms[iPlatform]->iHalfHeight) / PREVIEWTILESIZE;
		short iEndX = ((short)pPath->fEndX - platforms[iPlatform]->iHalfWidth) / PREVIEWTILESIZE;
		
		if(iStartX != iEndX)
		{
			short iCenterOffsetY = (platforms[iPlatform]->iHeight - 16) >> 2;

			bool fMoveToRight = iStartX < iEndX;

			short iSpotLeft = (fMoveToRight ? iStartX : iEndX) + 1;
			short iSpotRight = (fMoveToRight ? iEndX : iStartX) - 2 + platforms[iPlatform]->iTileWidth;

			for(short iSpot = iSpotLeft; iSpot <= iSpotRight; iSpot++)
				spr_thumbnail_platformarrows.draw(iSpot * iTileSize, iStartY * iTileSize + iCenterOffsetY, iTileSize * 5, 0, iTileSize, iTileSize);

			spr_thumbnail_platformarrows.draw((iSpotLeft - 1) * iTileSize, iStartY * iTileSize + iCenterOffsetY, iTileSize * 2, 0, iTileSize, iTileSize);
			spr_thumbnail_platformarrows.draw((iSpotRight + 1) * iTileSize, iStartY * iTileSize + iCenterOffsetY, iTileSize * 3, 0, iTileSize, iTileSize);
		}
		else
		{
			short iEndY = ((short)pPath->fEndY - platforms[iPlatform]->iHalfHeight) / PREVIEWTILESIZE;

			short iCenterOffsetX = (platforms[iPlatform]->iWidth - 16) >> 2;

			bool fMoveUp = iStartY < iEndY;

			short iSpotTop = (short)(fMoveUp ? iStartY : iEndY) + 1;
			short iSpotBottom = (short)(fMoveUp ? iEndY : iStartY) - 2 + platforms[iPlatform]->iTileHeight;

			for(short iSpot = iSpotTop; iSpot <= iSpotBottom; iSpot++)
				spr_thumbnail_platformarrows.draw(iStartX * iTileSize + iCenterOffsetX, iSpot * iTileSize, iTileSize * 4, 0, iTileSize, iTileSize);

			spr_thumbnail_platformarrows.draw(iStartX * iTileSize + iCenterOffsetX, (iSpotTop - 1) * iTileSize, 0, 0, iTileSize, iTileSize);
			spr_thumbnail_platformarrows.draw(iStartX * iTileSize + iCenterOffsetX, (iSpotBottom + 1) * iTileSize, iTileSize, 0, iTileSize, iTileSize);
		}
	}

	blitdest = screen;
}

void CMap::preDrawPreviewWarps(SDL_Surface * targetSurface, bool fThumbnail)
{
	short iTileSize = 16;
	short iScreenshotSize = 0;

	if(fThumbnail)
	{
		iTileSize = 8;
		iScreenshotSize = 1;
	}

	for(int j = 0; j < MAPHEIGHT; j++)
	{
		for(int i = 0; i < MAPWIDTH; i++)
		{
			Warp * warp = &g_map.warpdata[i][j];
			
			if(warp->connection != -1)
			{
				SDL_Rect rSrc = {warp->connection * iTileSize, warp->direction * iTileSize, iTileSize, iTileSize};
				SDL_Rect rDst = {i * iTileSize, j * iTileSize, iTileSize, iTileSize};

				SDL_BlitSurface(spr_thumbnail_warps[iScreenshotSize].getSurface(), &rSrc, targetSurface, &rDst);
			}
		}
	}
}

void CMap::preDrawPreviewBackground(SDL_Surface * targetSurface, bool fThumbnail)
{
	drawPreview(targetSurface, 0, fThumbnail);

	SDL_Delay(10);

	drawPreview(targetSurface, 1, fThumbnail);

	SDL_Delay(10);

	if(!game_values.toplayer)
	{
		drawPreview(targetSurface, 2, fThumbnail);

		SDL_Delay(10);

		drawPreview(targetSurface, 3, fThumbnail);

		SDL_Delay(10);
	}

	drawPreviewBlocks(targetSurface, fThumbnail);
}

void CMap::preDrawPreviewBackground(gfxSprite * spr_background, SDL_Surface * targetSurface, bool fThumbnail)
{
	SDL_Rect srcrect;
	srcrect.x = 0;
	srcrect.y = 0;
	srcrect.w = 640;
	srcrect.h = 480;

	SDL_Rect dstrect;
	dstrect.x = 0;
	dstrect.y = 0;

	if(fThumbnail)
	{
		dstrect.w = 160;
		dstrect.h = 120;
	}
	else
	{
		dstrect.w = 320;
		dstrect.h = 240;
	}

	if(SDL_SoftStretch(spr_background->getSurface(), &srcrect, targetSurface, &dstrect) < 0)
	{
		fprintf(stderr, "SDL_SoftStretch error: %s\n", SDL_GetError());
		return;
	}

	SDL_Delay(10);  //Sleeps to help the music from skipping
	
	preDrawPreviewBackground(targetSurface, fThumbnail);
}

void CMap::preDrawPreviewForeground(SDL_Surface * targetSurface, bool fThumbnail)
{
	if(!fThumbnail)
	{
		SDL_FillRect(targetSurface, NULL, SDL_MapRGB(targetSurface->format, 255, 0, 255));
		SDL_SetColorKey(targetSurface, SDL_SRCCOLORKEY, SDL_MapRGB(targetSurface->format, 255, 0, 255));
		SDL_Delay(10);
	}

	if(!game_values.toplayer)
		return;

	drawPreview(targetSurface, 2, fThumbnail);

	SDL_Delay(10);

	drawPreview(targetSurface, 3, fThumbnail);
}

void CMap::drawPreview(SDL_Surface * targetSurface, int layer, bool fThumbnail)
{
	int i, j, ts;

	//draw left to right full vertical

	short iBlockSize = PREVIEWTILESIZE;

	if(fThumbnail)
		iBlockSize = THUMBTILESIZE;

	SDL_Rect rectSrc;
	rectSrc.w = iBlockSize;
	rectSrc.h = iBlockSize;

	SDL_Rect rectDst;
	rectDst.w = iBlockSize;
	rectDst.h = iBlockSize;

	rectDst.x = 0;
	for(i = 0; i < MAPWIDTH; i++)
	{
		rectDst.y = -iBlockSize;	//this is okay, see

		for(j = 0; j < MAPHEIGHT; j++)
		{
			rectDst.y += iBlockSize;	// here

			ts = mapdata[i][j][layer];
			if(ts == TILESETSIZE)
				continue;

			if(fThumbnail)
			{
				rectSrc.x = iThumbTileX[ts];
				rectSrc.y = iThumbTileY[ts];
			}
			else
			{
				rectSrc.x = iPreviewTileX[ts];
				rectSrc.y = iPreviewTileY[ts];
			}
		
			if(fThumbnail)
				SDL_BlitSurface(tilesetsurface[2], &rectSrc, targetSurface, &rectDst);
			else
				SDL_BlitSurface(tilesetsurface[1], &rectSrc, targetSurface, &rectDst);
		}

		rectDst.x += iBlockSize;
	}
}

void CMap::drawPreviewBlocks(SDL_Surface * targetSurface, bool fThumbnail)
{
	int i, j, ts;
	
	short iBlockSize = PREVIEWTILESIZE;

	if(fThumbnail)
		iBlockSize = THUMBTILESIZE;

	SDL_Rect rectDst;
	rectDst.w = iBlockSize;
	rectDst.h = iBlockSize;

	SDL_Rect rectSrc;
	rectSrc.w = iBlockSize;
	rectSrc.h = iBlockSize;
	
	rectDst.x = 0;
	for(i = 0; i < MAPWIDTH; i++)
	{
		rectDst.y = -iBlockSize;	//this is okay, see

		for(j = 0; j < MAPHEIGHT; j++)
		{
			rectDst.y += iBlockSize;	// here

			ts = objectdata[i][j];
			if(ts == BLOCKSETSIZE)
				continue;

			rectSrc.x = (Sint16)(ts * iBlockSize);
			rectSrc.y = iBlockSize * 30;

			//Draw the turned off switch blocks too
			if(ts >= 7 && ts <= 14)
				if(iSwitches[(ts - 7) % 4] == 1)
					rectSrc.y = iBlockSize * 31;
		
			if(fThumbnail)
				SDL_BlitSurface(tilesetsurface[2], &rectSrc, targetSurface, &rectDst);
			else
				SDL_BlitSurface(tilesetsurface[1], &rectSrc, targetSurface, &rectDst);
		}

		rectDst.x += iBlockSize;
	}
}

void CMap::predrawbackground(gfxSprite &background, gfxSprite &mapspr)
{
	SDL_Rect r;
	r.x = 0;
	r.y = 0;
	r.w = 640;
	r.h = 480;

	SDL_BlitSurface(background.getSurface(), NULL, mapspr.getSurface(), &r);
	
	draw(mapspr.getSurface(), 0, false);
	draw(mapspr.getSurface(), 1, false);

	if(!game_values.toplayer)
	{
		draw(mapspr.getSurface(), 2, false);
		draw(mapspr.getSurface(), 3, false);
	}

	//Draws the spawn areas
	/*
	SDL_Rect dest;
	dest.w = 32;
	dest.h = 32;
	short iType = 0;
	
	for(int m = 0; m < numspawnareas[iType]; m++)  //use [1] for item spawn areas
	{
		dest.x = spawnareas[iType][m].left * TILESIZE;
		dest.y = spawnareas[iType][m].top * TILESIZE;

		dest.w = (spawnareas[iType][m].width) * TILESIZE + TILESIZE;
		dest.h = (spawnareas[iType][m].height) * TILESIZE + TILESIZE;

		int color = 0x00 << 24 | rand() % 256 << 16 | rand() % 256 << 8 | rand() % 256;

		SDL_FillRect(mapspr.getSurface(), &dest, color);		//fill empty area with black
	}
	*/
}

void CMap::predrawforeground(gfxSprite &foregroundspr)
{
	SDL_FillRect(foregroundspr.getSurface(), NULL, SDL_MapRGB(foregroundspr.getSurface()->format, 255, 0, 255));
	SDL_SetColorKey(foregroundspr.getSurface(), SDL_SRCCOLORKEY, SDL_MapRGB(foregroundspr.getSurface()->format, 255, 0, 255));

	draw(foregroundspr.getSurface(), 2, true);
	draw(foregroundspr.getSurface(), 3, true);

	/*
	for(int k = 0; k < numdrawareas; k++)
	{
		SDL_Rect rect;
		rect.x = drawareas[k].x;
		rect.y = drawareas[k].y;
		rect.w = drawareas[k].w;
		rect.h = drawareas[k].h;
		int color = 0x00 << 24 | rand() % 256 << 16 | rand() % 256 << 8 | rand() % 256;

		SDL_FillRect(foregroundspr.getSurface(), &rect, color);
	}
	*/
}

void CMap::updatePlatforms()
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		platforms[iPlatform]->update();
	}
}

void CMap::drawPlatforms()
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		platforms[iPlatform]->draw();
	}
}

void CMap::drawPlatforms(short iOffsetX, short iOffsetY)
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		platforms[iPlatform]->draw(iOffsetX, iOffsetY);
	}
}

void CMap::movingPlatformCollision(CPlayer * player)
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		platforms[iPlatform]->collide(player);

		if(!player->isready())
			return;
	}
}

void CMap::movingPlatformCollision(IO_MovingObject * object)
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		platforms[iPlatform]->collide(object);
	}
}

void CMap::resetPlatforms()
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		platforms[iPlatform]->ResetPath();
	}
}

WarpExit * CMap::getRandomWarpExit(int connection, int currentID)
{
	int indices[MAXWARPS];
	int numIndices = 0;

	WarpExit * currentWarp = NULL;

	for(int k = 0; k < numwarpexits; k++)
	{
		if(warpexits[k].connection == connection)
		{
			if(warpexits[k].id == currentID)
				currentWarp = &warpexits[k];
			else
				indices[numIndices++] = k;
		}
	}

	if(numIndices == 0)
		return currentWarp;

	return &warpexits[indices[rand() % numIndices]];
}

void CMap::clearWarpLocks()
{
	for(int k = 0; k < 10; k++)
	{
		warplocktimer[k] = 0;
		warplocked[k] = false;
	}
}

void CMap::drawWarpLocks()
{
	for(int k = 0; k < numwarpexits; k++)
	{
		if(warplocked[warpexits[k].connection])
		{
			spr_warplock.draw(warpexits[k].lockx, warpexits[k].locky);
		}
	}
}

void CMap::update()
{
	for(int k = 0; k <= maxConnection; k++)
	{
		if(warplocked[k])
		{
			if(++warplocktimer[k] > game_values.warplocks)
			{
				warplocked[k] = false;
				warplocktimer[k] = 0;
			}
		}
	}

	animatedtilesfront.update();
	animatedtilesback.update();
}

void CMap::drawbackanimations()
{
	animatedtilesback.draw();
}

void CMap::drawfrontanimations()
{
	animatedtilesfront.draw();
}

void CMap::findspawnpoint(short iType, short * x, short * y, short width, short height, bool tilealigned)
{
	if(totalspawnsize[iType] <= 0)
	{
		*x = 320;
		*y = 224;
		return;
	}

	//Calculate where the platforms are and create no spawn zones around them

	float dPathTime = 0.0f;
	if(game_values.spawnstyle == 1)
		dPathTime = 36.0f;
	else if(game_values.spawnstyle == 2)
		dPathTime = 51.0f;

	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
		platforms[iPlatform]->CalculateNoSpawnZone(dPathTime);

	bool fDone = false;
	short iTries = 0;
	while(iTries++ < MAX_PLAYER_SPAWN_TRIES && !fDone)
	{
		int spawnarea = rand() % totalspawnsize[iType];

		int currentsize = 0;
		for(int m = 0; m < numspawnareas[iType]; m++)
		{
			currentsize += spawnareas[iType][m].size;

			if(spawnarea >= currentsize)
				continue;

			short areawidth = spawnareas[iType][m].width * TILESIZE + TILESIZE;
			short areaheight = spawnareas[iType][m].height * TILESIZE + TILESIZE;

			if(width > areawidth || height > areaheight)
				continue;

			if(tilealigned)
			{
				short xoffset = spawnareas[iType][m].width;
				short yoffset = spawnareas[iType][m].height;

				if(xoffset > 0)
					xoffset = (short)(rand() % xoffset);

				if(yoffset > 0)
					yoffset = (short)(rand() % yoffset);

				*x = xoffset * TILESIZE + spawnareas[iType][m].left * TILESIZE + (TILESIZE >> 1) - (width >> 1);
				*y = yoffset * TILESIZE + spawnareas[iType][m].top * TILESIZE + (TILESIZE >> 1) - (height >> 1);
			}
			else
			{
				short xoffset = areawidth - width - 2;
				short yoffset = areaheight - height - 2;

				if(xoffset > 0)
					xoffset = (short)(rand() % xoffset) + 1;

				if(yoffset > 0)
					yoffset = (short)(rand() % yoffset) + 1;

				*x = xoffset + spawnareas[iType][m].left * TILESIZE;
				*y = yoffset + spawnareas[iType][m].top * TILESIZE;
			}

			break;
		}

		for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
		{
			fDone = !platforms[iPlatform]->IsInNoSpawnZone(*x, *y, width, height);
			
			if(!fDone)
				break;
		}
	}
}

void CMap::CalculatePlatformNoSpawnZones()
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
		platforms[iPlatform]->CalculateNoSpawnZone(0.0f);
}

bool CMap::IsInPlatformNoSpawnZone(short x, short y, short width, short height)
{
	for(short iPlatform = 0; iPlatform < iNumPlatforms; iPlatform++)
	{
		if(platforms[iPlatform]->IsInNoSpawnZone(x, y, width, height))
			return true;
	}

	return false;
}

void CMap::drawfrontlayer()
{
	for(int k = 0; k < numdrawareas; k++)
		spr_frontmap.draw(drawareas[k].x, drawareas[k].y, drawareas[k].x, drawareas[k].y, drawareas[k].w, drawareas[k].h);
}

void CMap::WriteInt(int out, FILE * outFile)
{
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	int t = out;

    ((char*)&out)[0] = ((char*)&t)[3];
    ((char*)&out)[1] = ((char*)&t)[2];
    ((char*)&out)[2] = ((char*)&t)[1];
    ((char*)&out)[3] = ((char*)&t)[0];
#endif

	fwrite(&out, sizeof(int), 1, outFile);
}

int CMap::ReadInt(FILE * inFile)
{
	int in;
	fread(&in, sizeof(int), 1, inFile);

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	int t = in;

    ((char*)&in)[0] = ((char*)&t)[3];
    ((char*)&in)[1] = ((char*)&t)[2];
    ((char*)&in)[2] = ((char*)&t)[1];
    ((char*)&in)[3] = ((char*)&t)[0];
#endif

	return in;
}

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
void CMap::ReadIntChunk(int * mem, size_t iQuantity, FILE * inFile)
{
	for(unsigned int i = 0; i < iQuantity; i++)
	{
		fread(&mem[i], sizeof(int), 1, inFile);

		int t = mem[i];

		((char*)&mem[i])[0] = ((char*)&t)[3]; 
		((char*)&mem[i])[1] = ((char*)&t)[2]; 
		((char*)&mem[i])[2] = ((char*)&t)[1]; 
		((char*)&mem[i])[3] = ((char*)&t)[0]; 
	}
}
#else
void CMap::ReadIntChunk(int * mem, size_t iQuantity, FILE * inFile)
{
	fread(mem, sizeof(int), iQuantity, inFile);
}
#endif


void CMap::WriteFloat(float out, FILE * outFile)
{
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	float t = out;

    ((char*)&out)[0] = ((char*)&t)[3];
    ((char*)&out)[1] = ((char*)&t)[2];
    ((char*)&out)[2] = ((char*)&t)[1];
    ((char*)&out)[3] = ((char*)&t)[0];
#endif

	fwrite(&out, sizeof(float), 1, outFile);
}

float CMap::ReadFloat(FILE * inFile)
{
	float in;
	fread(&in, sizeof(float), 1, inFile);

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	float t = in;

    ((char*)&in)[0] = ((char*)&t)[3];
    ((char*)&in)[1] = ((char*)&t)[2];
    ((char*)&in)[2] = ((char*)&t)[1];
    ((char*)&in)[3] = ((char*)&t)[0];
#endif

	return in;
}

void CMap::WriteString(char * szString, FILE * outFile)
{
	int iLen = strlen(szString) + 1;
	WriteInt(iLen, outFile);
	fwrite(szString, sizeof(char), iLen, outFile);
}

void CMap::ReadString(char * szString, short size, FILE * outFile)
{
	int iLen = ReadInt(outFile);

	char * szReadString = new char[iLen];
	fread(szReadString, sizeof(char), iLen, outFile);

	//Prevent buffer overflow
	strncpy(szString, szReadString, size - 1);
	szString[size - 1] = 0;

	delete [] szReadString;
}

void CMap::clearAnimations()
{
	animatedtilesback.clean();
	animatedtilesfront.clean();
}

void CMap::optimize()
{
	for(int j = 0; j < MAPHEIGHT; j++)
	{
		for(int i = 0; i < MAPWIDTH; i++)
		{
			for(int m = 1; m < MAPLAYERS; m++)
			{
				if(tileset[mapdata[i][j][m]] != tile_nonsolid && tileset[mapdata[i][j][m]] != tile_solid_on_top)
				{
					for(int k = m - 1; k >= 0; k--)
					{
						if(mapdata[i][j][k] == TILESETSIZE)
						{
							mapdata[i][j][k] = mapdata[i][j][k + 1];
							mapdata[i][j][k + 1] = TILESETSIZE;
						}
						else
						{
							break;
						}
					}
				}
			}
		}
	}
}
