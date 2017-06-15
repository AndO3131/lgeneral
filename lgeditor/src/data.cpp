/*
 * data.cpp
 *
 *  Created on: 28.02.2017
 *      Copyright 2017 Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include "widgets.h"
#include "data.h"
#include "parser.h"

extern std::string lgenpath;
static const char * get_gamedir() {
	return lgenpath.c_str();
}

/** helpers from lgeneral
 */
#define load_surf(__PATH, __FLAGS) \
	SDL_LoadBMP(((std::string)(lgenpath + "/gfx/" + __PATH)).c_str())



/** Nations */
typedef struct {
    char *id;
    char *name;
    int  flag_offset;
    int  no_purchase; /* whether nation has units to purchase;
                         updated when scenario is loaded */
} Nation;
Nation *nations = 0;
int nation_count = 0;
SDL_Surface *nation_flags = 0;
int nation_flag_width = 0, nation_flag_height = 0;

void nations_delete( void )
{
    int i;
    if ( nation_flags ) SDL_FreeSurface ( nation_flags ); nation_flags = 0;
    if ( nations == 0 ) return;
    for ( i = 0; i < nation_count; i++ ) {
        if ( nations[i].id ) free( nations[i].id );
        if ( nations[i].name ) free( nations[i].name );
    }
    free( nations ); nations = 0; nation_count = 0;
}
int nations_load( char *fname )
{
    int i;
    PData *pd, *sub;
    MyList *entries;
    char path[512];
    char *str;
    char *domain = 0;
    sprintf( path, "%s/nations/%s", get_gamedir(), fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    //domain = determine_domain(pd, fname);
    //locale_load_domain(domain, 0/*FIXME*/);
    /* icon size */
    if ( !parser_get_int( pd, "icon_width", &nation_flag_width ) ) goto failure;
    if ( !parser_get_int( pd, "icon_height", &nation_flag_height ) ) goto parser_failure;
    /* icons */
    if ( !parser_get_value( pd, "icons", &str, 0 ) ) goto parser_failure;
    sprintf( path, "flags/%s", str );
    if ( ( nation_flags = load_surf( path,  SDL_SWSURFACE ) ) == 0 ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    /* nations */
    if ( !parser_get_entries( pd, "nations", &entries ) ) goto parser_failure;
    nation_count = entries->count;
    nations = (Nation*)calloc( nation_count, sizeof( Nation ) );
    list_reset( entries ); i = 0;
    while ( ( sub = (PData*)list_next( entries ) ) ) {
        nations[i].id = strdup( sub->name );
        if ( !parser_get_localized_string( sub, "name", domain, &nations[i].name ) ) goto parser_failure;
        if ( !parser_get_int( sub, "icon_id", &nations[i].flag_offset ) ) goto parser_failure;
        nations[i].flag_offset *= nation_flag_height;
        i++;
    }
    parser_free( &pd );
    free(domain);
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    nations_delete();
    if ( pd ) parser_free( &pd );
    free(domain);
    return 0;
}

/** Terrain */
typedef struct {
    char *id;
    char *name;
    int flags;
} Weather_Type;
typedef struct {
    char id;
    char *name;
    SDL_Surface **images;
    int *spt; /* needed for weather */
    int min_entr;
    int max_entr;
    int max_ini;
} Terrain_Type;
int hex_w, hex_h;
int hex_x_offset, hex_y_offset;
Terrain_Type *terrain_types = 0;
int terrain_type_count = 0;
Weather_Type *weather_types = 0;
int weather_type_count = 0;

void terrain_delete( void )
{
    int i, j;
    /* terrain types */
    if ( terrain_types ) {
        for ( i = 0; i < terrain_type_count; i++ ) {
            if ( terrain_types[i].images ) {
                for ( j = weather_type_count - 1; j >= 0; j-- )
                    if ( terrain_types[i].images[j] ) {
                        if ( terrain_types[i].images[j] == terrain_types[i].images[0] )
                            if ( j > 0 )
                                continue; /* only a pointer */
                        SDL_FreeSurface( terrain_types[i].images[j] );
                    }
                free( terrain_types[i].images );
            }
            if ( terrain_types[i].name ) free( terrain_types[i].name );
        }
        free( terrain_types ); terrain_types = 0;
        terrain_type_count = 0;
    }
    /* weather */
    if ( weather_types ) {
        for ( i = 0; i < weather_type_count; i++ ) {
            if ( weather_types[i].id ) free( weather_types[i].id );
            if ( weather_types[i].name ) free( weather_types[i].name );
        }
        free( weather_types ); weather_types = 0;
        weather_type_count = 0;
    }
}

int terrain_load( char *fname )
{
    int i, j;
    PData *pd, *sub, *subsub;
    MyList *entries, *flags;
    char path[512];
    char *str;
    char *domain = 0;
    sprintf( path, "%s/maps/%s", get_gamedir(), fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    //domain = determine_domain(pd, fname);
    //locale_load_domain(domain, 0/*FIXME*/);
    /* get weather */
    if ( !parser_get_entries( pd, "weather", &entries ) ) goto parser_failure;
    weather_type_count = entries->count;
    weather_types = (Weather_Type*)calloc( weather_type_count, sizeof( Weather_Type ) );
    list_reset( entries ); i = 0;
    while ( ( sub = (PData*)list_next( entries ) ) ) {
        weather_types[i].id = strdup( sub->name );
        if ( !parser_get_localized_string( sub, "name", domain, &weather_types[i].name ) ) goto parser_failure;
        if ( !parser_get_values( sub, "flags", &flags ) ) goto parser_failure;
        list_reset( flags );
       // while ( ( flag = (char*)list_next( flags ) ) )
        //    weather_types[i].flags |= check_flag( flag, fct_terrain );
        i++;
    }
    /* hex tile geometry */
    if ( !parser_get_int( pd, "hex_width", &hex_w ) ) goto parser_failure;
    if ( !parser_get_int( pd, "hex_height", &hex_h ) ) goto parser_failure;
    if ( !parser_get_int( pd, "hex_x_offset", &hex_x_offset ) ) goto parser_failure;
    if ( !parser_get_int( pd, "hex_y_offset", &hex_y_offset ) ) goto parser_failure;

    /* terrain types */
    if ( !parser_get_entries( pd, "terrain", &entries ) ) goto parser_failure;
    terrain_type_count = entries->count;
    terrain_types = (Terrain_Type*)calloc( terrain_type_count, sizeof( Terrain_Type ) );
    list_reset( entries ); i = 0;
    while ( ( sub = (PData*)list_next( entries ) ) ) {
        /* id */
        terrain_types[i].id = sub->name[0];
        /* name */
        if ( !parser_get_localized_string( sub, "name", domain, &terrain_types[i].name ) ) goto parser_failure;
        /* each weather type got its own image -- if it's named 'default' we
           point towards the image of weather_type 0 */
        terrain_types[i].images = (SDL_Surface**)calloc( weather_type_count, sizeof( SDL_Surface* ) );
        for ( j = 0; j < weather_type_count; j++ ) {
            sprintf( path, "image/%s", weather_types[j].id );
            if ( !parser_get_value( sub, path, &str, 0 ) ) goto parser_failure;
            if ( strcmp( "default", str ) == 0 && j > 0 ) {
                /* just a pointer */
                terrain_types[i].images[j] = terrain_types[i].images[0];
            }
            else {
                sprintf( path, "terrain/%s", str );
                if ( ( terrain_types[i].images[j] = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto parser_failure;
                SDL_SetColorKey( terrain_types[i].images[j], SDL_TRUE, 0x00000000);
            }
        }
        /* spot cost */
        terrain_types[i].spt = (int*)calloc( weather_type_count, sizeof( int ) );
        if ( !parser_get_pdata( sub, "spot_cost",  &subsub ) ) goto parser_failure;
        for ( j = 0; j < weather_type_count; j++ )
            if ( !parser_get_int( subsub, weather_types[j].id, &terrain_types[i].spt[j] ) ) goto parser_failure;
        /* entrenchment */
        if ( !parser_get_int( sub, "min_entr",  &terrain_types[i].min_entr ) ) goto parser_failure;
        if ( !parser_get_int( sub, "max_entr",  &terrain_types[i].max_entr ) ) goto parser_failure;
        /* initiative modification */
        if ( !parser_get_int( sub, "max_init",  &terrain_types[i].max_ini ) ) goto parser_failure;
        /* next terrain */
        i++;
    }
    parser_free( &pd );
    //free(domain);
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
    terrain_delete();
    if ( pd ) parser_free( &pd );
    //free(domain);
    //printf(tr("If data seems to be missing, please re-run the converter lgc-pg.\n"));
    return 0;
}


/** Unit Lib */

/** Scenario */

/** Data container */

Data::Data(int w, int h)
{
	/* general data */
	terrain_load((char*)"pg.tdb");
	for (int i = 0; i < terrain_type_count; i++) {
		TerrainInfo ti;
		ti.id = terrain_types[i].id;
		ti.name = terrain_types[i].name;
		ti.tiles = new GridImage(terrain_types[i].images[0],hex_w,hex_h);
		terrain.push_back(ti);
	}
	for (int i = 0; i < weather_type_count; i++) {
		WeatherInfo wi;
		wi.id = weather_types[i].id;
		wi.name = weather_types[i].name;
		weather.push_back(wi);
	}
	hexw = hex_w;
	hexh = hex_h;
	hexxoff = hex_x_offset;
	hexyoff = hex_y_offset;
	terrain_delete();

	grid = new Image(lgenpath + "/gfx/terrain/pg/grid.bmp");
	selectFrame = new Image(lgenpath + "/gfx/terrain/pg/select_frame.bmp");

	nations_load((char*)"pg.ndb");
	Image *flags = new Image(nation_flags);
	for (int i = 0; i < nation_count; i++) {
		NationInfo ni;
		ni.id = nations[i].id;
		ni.name = nations[i].name;
		ni.icon = new Image(flags, 0, nations[i].flag_offset,
					nation_flag_width, nation_flag_height);
		countries.push_back(ni);
	}
	delete flags;
	nations_delete();

	/* generate empty standard map width size
	 * TODO load map, ignore w,h then
	 */
	mapw = w;
	maph = h;
	for (int i = 0; i < mapw; i++)
		for (int j = 0; j < maph; j++) {
			map[i][j].tid[0] = 0;
			map[i][j].tid[1] = rand() % 6 + 12;
			map[i][j].fid = -1;
			map[i][j].obj = 0;
			map[i][j].name = terrain[map[i][j].tid[0]].name;
		}
}
Data::~Data()
{
	delete grid;
	delete selectFrame;
	for (unsigned int i = 0; i < terrain.size(); i++)
		delete terrain[i].tiles;
	for (unsigned int i = 0; i < countries.size(); i++)
			delete countries[i].icon;
}

void Data::loadMap(std::string fname)
{
	const char *path = fname.c_str();
	int i, x, y, limit;
	unsigned int j;
	PData *pd;
	char *str, *tile;
	char *domain = 0;
	MyList *tiles, *names;

	printf("Loading %s\n",path);

	if ( ( pd = parser_read_file( "map", path ) ) == 0 ) goto parser_failure;
	/* map size */
	if ( !parser_get_int( pd, "width", &mapw ) ) goto parser_failure;
	if ( !parser_get_int( pd, "height", &maph ) ) goto parser_failure;
	/* load terrains */
	//if ( !parser_get_value( pd, "terrain_db", &str, 0 ) ) goto parser_failure;
	//if ( !terrain_load( str ) ) goto failure;
	if ( !parser_get_values( pd, "tiles", &tiles ) ) goto parser_failure;
	/* map itself */
	list_reset( tiles );
	for ( y = 0; y < maph; y++ )
		for ( x = 0; x < mapw; x++ ) {
			tile = (char*)list_next( tiles );
			/* no flag */
			map[x][y].fid = -1;
			map[x][y].obj = 0;
			map[x][y].tid[0] = 0;
			map[x][y].tid[1] = 0;
			/* check tile type */
			for ( j = 0; j < terrain.size(); j++ )
				if ( terrain[j].id.c_str()[0] == tile[0] )
					map[x][y].tid[0] = j;
			/* check image id -- set offset */
			limit = terrain[map[x][y].tid[0]].tiles->getGridSize();
			if ( tile[1] == '?' )
				/* set offset by random */
				map[x][y].tid[1] = rand() % limit;
			else
				map[x][y].tid[1] = atoi( tile + 1 );
			/* set name */
			map[x][y].name = terrain[map[x][y].tid[0]].name;
		}
	/* map names */
	if ( parser_get_values( pd, "names", &names ) ) {
		list_reset( names );
		for ( i = 0; i < names->count; i++ ) {
			str = (char*)list_next( names );
			x = i % mapw;
			y = i / mapw;
			map[x][y].name = str;
		}
	}
	parser_free( &pd );
	free(domain);
	return;

parser_failure:
	fprintf( stderr, "%s\n", parser_get_error() );
	if ( pd ) parser_free( &pd );
	free(domain);
	return;
}

void Data::saveMap(std::string fn)
{
	const char *path = fn.c_str();
	FILE *dest_file;
	int x, y;

	printf("Saving map as %s\n",path);

	/* open dest file */
	if ( ( dest_file = fopen( path, "wb" ) ) == NULL ) {
		fprintf( stderr, "%s: access denied\n", path );
		return;
	}

	/* magic for new file */
	fprintf( dest_file, "@\n" );
	/* terrain types */
	fprintf( dest_file, "terrain_db%c%s.tdb\n", 0xbb, "pg" );
	/* domain */
	fprintf( dest_file, "domain%cpg\n", 0xbb );
	/* write map size */
	fprintf( dest_file, "width%c%i\nheight%c%i\n", 0xbb, mapw, 0xbb, maph );
	/* picture ids */
	fprintf( dest_file, "tiles%c", 0xbb );
	for ( y = 0; y < maph; y++ ) {
		for ( x = 0; x < mapw; x++ ) {
			std::ostringstream txt;
			txt << terrain[map[x][y].tid[0]].id << map[x][y].tid[1];
			fprintf( dest_file, "%s", txt.str().c_str() );
			if ( y < maph - 1 || x < mapw - 1 )
				fprintf( dest_file, "%c", 0xb0 );
		}
	}
	fprintf( dest_file, "\n" );
	fprintf( dest_file, "names%c", 0xbb );
	for ( y = 0; y < maph; y++ ) {
		for ( x = 0; x < mapw; x++ ) {
			std::string n;
			n = map[x][y].name;
			fprintf( dest_file, "%s", n.c_str() );
			if ( y < maph - 1 || x < mapw - 1 )
				fprintf( dest_file, "%c", 0xb0 );
		}
	}
	fprintf( dest_file, "\n" );

	fclose( dest_file );
}
