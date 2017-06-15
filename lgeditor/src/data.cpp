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

#define tr(___STR___) ___STR___
#define trd(__D__,__STR__) __STR__
#define STRCMP(__STR1,__STR2) (strcmp(__STR1,__STR2)==0)

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
typedef struct {
    char *id;
    char *name;
} Trgt_Type;
typedef struct {
    char *id;
    char *name;
#ifdef WITH_SOUND
    Wav *wav_move;
#endif
} Mov_Type;
typedef struct {
    char *id;
    char *name;
#define UC_PT_NONE   0
#define UC_PT_NORMAL 1
#define UC_PT_TRSP   2
    int  purchase;
} Unit_Class;

enum { TARGET_TYPE_LIMIT = 10 };

typedef struct {
    SDL_Surface *str;
    int str_w, str_h;
    SDL_Surface *atk;
    SDL_Surface *mov;
    SDL_Surface *guard;
} Unit_Info_Icons;
Unit_Info_Icons *unit_info_icons = 0;

typedef struct {
    char *id;       /* identification of this entry */
    char *name;     /* name */
    int nation;     /* nation */
    int uclass;      /* unit class */
    int trgt_type;  /* target type */
    int ini;        /* inititative */
    int mov;        /* movement */
    int mov_type;   /* movement type */
    int spt;        /* spotting */
    int rng;        /* attack range */
    int atk_count;  /* number of attacks per turn */
    int atks[TARGET_TYPE_LIMIT]; /* attack values (number is determined by global target_type_count) */
    int def_grnd;   /* ground defense (non-flying units) */
    int def_air;    /* air defense */
    int def_cls;    /* close defense (infantry against non-infantry) */
    int entr_rate;  /* default is 2, if flag LOW_ENTR_RATE is set it's only 1 and
                       if INFANTRY is set it's 3, this modifies the rugged defense
                       chance */
    int ammo;       /* max ammunition */
    int fuel;       /* max fuel (0 if not used) */
    SDL_Surface *icon;      /* tactical icon */
    SDL_Surface *icon_tiny; /* half the size; used to display air and ground unit at one tile */
    int icon_type;          /* either single or all_dirs */
    int icon_w, icon_h;     /* single icon size */
    int icon_tiny_w, icon_tiny_h; /* single icon size */
    int flags;
    int start_year, start_month, last_year; /* time of usage */
    int cost; /* purchase cost in prestige points */

#ifdef WITH_SOUND
    int wav_alloc;          /* if this flag is set wav_move must be freed else it's a pointer */
    Wav *wav_move;  /* pointer to the unit class default sound if wav_alloc is not set */
#endif
    int eval_score; /* between 0 - 1000 indicating the worth of the unit relative the
                       best one */
} __attribute((packed)) Unit_Lib_Entry;

int unit_lib_main_loaded = 0;
Trgt_Type *trgt_types = 0;
int trgt_type_count = 0;
Mov_Type *mov_types = 0;
int mov_type_count = 0;
Unit_Class *unit_classes= 0;
int unit_class_count = 0;
MyList *unit_lib = 0;

enum {
    UNIT_ICON_SINGLE = 0,
    UNIT_ICON_FIXED,
    UNIT_ICON_ALL_DIRS
};

static void unit_lib_delete_entry( void *ptr )
{
    Unit_Lib_Entry *entry = (Unit_Lib_Entry*)ptr;
    if ( entry == 0 ) return;
    if ( entry->id ) free( entry->id );
    if ( entry->name ) free( entry->name );
    if ( entry->icon ) SDL_FreeSurface( entry->icon );
    if ( entry->icon_tiny ) SDL_FreeSurface( entry->icon_tiny );
#ifdef WITH_SOUND
    if ( entry->wav_alloc && entry->wav_move )
        wav_free( entry->wav_move );
#endif
    free( entry );
}

/*
    create an surface
    MUST NOT BE USED IF NO SDLSCREEN IS SET
*/
SDL_Surface* create_surf(int w, int h, int f)
{
	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
	SDL_Surface *sur;
    sur = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);
    if (sur == NULL) {
        fprintf(stderr, "ERR: ssur_create: not enough memory to create surface...\n");
        exit(1);
    }
//    SDL_SetColorKey(sur, SDL_SRCCOLORKEY, 0x0);
//    SDL_SetAlpha(sur, 0, 0); /* no alpha */
    return sur;
}

/* set pixel */
Uint32 set_pixel( SDL_Surface *surf, int x, int y, int pixel )
{
    int pos = 0;

    pos = y * surf->pitch + x * surf->format->BytesPerPixel;
    memcpy( (char*)surf->pixels + pos, &pixel, surf->format->BytesPerPixel );
    return pixel;
}

/* get pixel */
Uint32 get_pixel( SDL_Surface *surf, int x, int y )
{
    int pos = 0;
    Uint32 pixel = 0;

    pos = y * surf->pitch + x * surf->format->BytesPerPixel;
    memcpy( &pixel, (char*)surf->pixels + pos, surf->format->BytesPerPixel );
    return pixel;
}

static void unit_get_icon_geometry( int icon_id, SDL_Surface *icons, int *width, int *height, int *offset, Uint32 *key )
{
    Uint32 mark;
    int y;
    int count = icon_id * 2; /* there are two pixels for one icon */

    /* nada white dot! take the first pixel found in the upper left corner as mark */
    mark = get_pixel( icons, 0, 0 );
    /* compute offset */
    for ( y = 0; y < icons->h; y++ )
        if ( get_pixel( icons, 0, y ) == mark ) {
            if ( count == 0 ) break;
            count--;
        }
    *offset = y;
    /* compute height */
    y++;
    while ( y < icons->h && get_pixel( icons, 0, y ) != mark )
        y++;
   (*height) = y - (*offset) + 1;
    /* compute width */
    y = *offset;
    *width = 1;
    while ( get_pixel( icons, (*width ), y ) != mark )
        (*width)++;
    (*width)++;
    /* pixel beside left upper measure key is color key */
    *key = get_pixel( icons, 1, *offset );
}

/*
====================================================================
Delete unit library.
====================================================================
*/
void unit_lib_delete( void )
{
    int i;
    if ( unit_lib ) {
        list_delete( unit_lib );
        unit_lib = 0;
    }
    if ( trgt_types ) {
        for ( i = 0; i < trgt_type_count; i++ ) {
            if ( trgt_types[i].id ) free( trgt_types[i].id );
            if ( trgt_types[i].name ) free( trgt_types[i].name );
        }
        free( trgt_types );
        trgt_types = 0; trgt_type_count = 0;
    }
    if ( mov_types ) {
        for ( i = 0; i < mov_type_count; i++ ) {
            if ( mov_types[i].id ) free( mov_types[i].id );
            if ( mov_types[i].name ) free( mov_types[i].name );
#ifdef WITH_SOUND
            if ( mov_types[i].wav_move )
                wav_free( mov_types[i].wav_move );
#endif
        }
        free( mov_types );
        mov_types = 0; mov_type_count = 0;
    }
    if ( unit_classes ) {
        for ( i = 0; i < unit_class_count; i++ ) {
            if ( unit_classes[i].id ) free( unit_classes[i].id );
            if ( unit_classes[i].name ) free( unit_classes[i].name );
        }
        free( unit_classes );
        unit_classes = 0; unit_class_count = 0;
    }
    if ( unit_info_icons ) {
        if ( unit_info_icons->str ) SDL_FreeSurface( unit_info_icons->str );
        if ( unit_info_icons->atk ) SDL_FreeSurface( unit_info_icons->atk );
        if ( unit_info_icons->mov ) SDL_FreeSurface( unit_info_icons->mov );
        if ( unit_info_icons->guard ) SDL_FreeSurface( unit_info_icons->guard );
        free( unit_info_icons );
        unit_info_icons = 0;
    }
    unit_lib_main_loaded = 0;
}

/*
====================================================================
Load a unit library. If UNIT_LIB_MAIN is passed target_types,
mov_types and unit classes will be loaded (may only happen once)
====================================================================
*/
int unit_lib_load(const char *fname, int main )
{
    int i, j, icon_id;
    SDL_Surface *icons = NULL;
    int icon_type;
    int width, height, offset;
    Uint32 color_key;
    int byte_size, y_offset;
    char *pix_buffer;
    Unit_Lib_Entry *unit;
    MyList *entries;
    PData *pd, *sub, *subsub;
    char path[512];
    char *str;
    char *domain = 0;
    float scale;
    /* log info */
    int  log_dot_limit = 40; /* maximum of dots */
    int  log_dot_count = 0; /* actual number of dots displayed */
    int  log_units_per_dot = 0; /* number of units a dot represents */
    int  log_unit_count = 0; /* if > units_per_dot a new dot is added */
    char log_str[128];

    /* there can be only one main library */
    if ( main && unit_lib_main_loaded ) {
        fprintf( stderr, tr("%s: can't load as main unit library (which is already loaded): loading as 'additional' instead\n"),
                 fname );
        main = 0;
    }
    /* parse file */
    sprintf( path, "%s/units/%s", get_gamedir(), fname );
    sprintf( log_str, tr("  Parsing '%s'"), fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    /* if main read target types & co */
    if ( main ) {
        unit_lib = list_create( LIST_AUTO_DELETE, unit_lib_delete_entry );
        /* target types */
        if ( !parser_get_entries( pd, "target_types", &entries ) ) goto parser_failure;
        trgt_type_count = entries->count;
        if ( trgt_type_count > TARGET_TYPE_LIMIT ) {
            fprintf( stderr, tr("%i target types is the limit!\n"), TARGET_TYPE_LIMIT );
            trgt_type_count = TARGET_TYPE_LIMIT;
        }
        trgt_types = (Trgt_Type*)calloc( trgt_type_count, sizeof( Trgt_Type ) );
        list_reset( entries ); i = 0;
        while ( ( sub = (PData*)list_next( entries ) ) ) {
            trgt_types[i].id = strdup( sub->name );
            if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
            trgt_types[i].name = strdup(trd(domain, str));
            i++;
        }
        /* movement types */
        if ( !parser_get_entries( pd, "move_types", &entries ) ) goto parser_failure;
        mov_types = (Mov_Type*)calloc( entries->count, sizeof( Mov_Type ) );
        list_reset( entries ); mov_type_count = 0;
        while ( ( sub = (PData*)list_next( entries ) ) ) {
            mov_types[mov_type_count].id = strdup( sub->name );
            if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
            mov_types[mov_type_count].name = strdup(trd(domain, str));
#ifdef WITH_SOUND
            if ( parser_get_value( sub, "sound", &str, 0 ) ) {
                mov_types[mov_type_count].wav_move = wav_load( str, 0 );
            }
#endif
            mov_type_count++;
        }
        /* unit classes */
        if ( !parser_get_entries( pd, "unit_classes", &entries ) ) goto parser_failure;
        unit_classes = (Unit_Class*)calloc( entries->count, sizeof( Unit_Class ) );
        list_reset( entries ); unit_class_count = 0;
        while ( ( sub = (PData*)list_next( entries ) ) ) {
            unit_classes[unit_class_count].id = strdup( sub->name );
            if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
            unit_classes[unit_class_count].name = strdup(trd(domain, str));
	    unit_classes[unit_class_count].purchase = UC_PT_NONE;
	    if (parser_get_value( sub, "purchase", &str, 0 )) {
		    if (strcmp(str,"trsp") == 0)
			    unit_classes[unit_class_count].purchase = UC_PT_TRSP;
		    else if (strcmp(str,"normal") == 0)
			    unit_classes[unit_class_count].purchase = UC_PT_NORMAL;
	    }
	    unit_class_count++;
            /* ignore sounds so far */
        }
        /* unit map tile icons */
        unit_info_icons = (Unit_Info_Icons*)calloc( 1, sizeof( Unit_Info_Icons ) );
        if ( !parser_get_value( pd, "strength_icons", &str, 0 ) ) goto parser_failure;
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->str = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
        if ( !parser_get_int( pd, "strength_icon_width", &unit_info_icons->str_w ) ) goto parser_failure;
        if ( !parser_get_int( pd, "strength_icon_height", &unit_info_icons->str_h ) ) goto parser_failure;
        if ( !parser_get_value( pd, "attack_icon", &str, 0 ) ) goto parser_failure;
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->atk = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
        if ( !parser_get_value( pd, "move_icon", &str, 0 ) ) goto parser_failure;
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->mov = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
        if ( !parser_get_value( pd, "guard_icon", &str, 0 ) )
                sprintf( path, "units/%s", str );
        else
        	sprintf( path, "units/%s", "pg_guard.bmp" );
        if ( ( unit_info_icons->guard = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
    }
    /* icons */
    if ( !parser_get_value( pd, "icon_type", &str, 0 ) ) goto parser_failure;
    if (STRCMP( str, "fixed" ) )
        icon_type = UNIT_ICON_FIXED;
    else if ( STRCMP( str, "single" ) )
        icon_type = UNIT_ICON_SINGLE;
    else
        icon_type = UNIT_ICON_ALL_DIRS;
    if ( !parser_get_value( pd, "icons", &str, 0 ) ) goto parser_failure;
    sprintf( path, "units/%s", str );
    if ( ( icons = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
    /* unit lib entries */
    if ( !parser_get_entries( pd, "unit_lib", &entries ) ) goto parser_failure;
      /* LOG INIT */
      log_units_per_dot = entries->count / log_dot_limit;
      log_dot_count = 0;
      log_unit_count = 0;
      /* (LOG) */
    list_reset( entries );
    while ( ( sub = (PData*)list_next( entries ) ) ) {
        /* read unit entry */
        unit = (Unit_Lib_Entry*)calloc( 1, sizeof( Unit_Lib_Entry ) );
        /* identification */
        unit->id = strdup( sub->name );
        /* name */
        if ( !parser_get_value( sub, "name", &str, 0) ) goto parser_failure;
        unit->name = strdup(trd(domain, str));
	/* nation (if not found or 'none' unit can't be purchased) */
	unit->nation = -1; /* undefined */
	if ( parser_get_value( sub, "nation", &str, 0) && strcmp(str,"none") ) {
		/* XXX somehow get nation shit together
		for (Uint8 i = 0; i < countries.size(); i++)
			if (countries[i].id == str) {
				unit->nation = i;
				break;
			}*/
	}
        /* class id */
        unit->uclass = 0;
        if ( !parser_get_value( sub, "class", &str, 0 ) ) goto parser_failure;
        for ( i = 0; i < unit_class_count; i++ )
            if ( STRCMP( str, unit_classes[i].id ) ) {
                unit->uclass = i;
                break;
            }
        /* target type id */
        unit->trgt_type = 0;
        if ( !parser_get_value( sub, "target_type", &str, 0 ) ) goto parser_failure;
        for ( i = 0; i < trgt_type_count; i++ )
            if ( STRCMP( str, trgt_types[i].id ) ) {
                unit->trgt_type = i;
                break;
            }
        /* initiative */
        if ( !parser_get_int( sub, "initiative", &unit->ini ) ) goto parser_failure;
        /* spotting */
        if ( !parser_get_int( sub, "spotting", &unit->spt ) ) goto parser_failure;
        /* movement */
        if ( !parser_get_int( sub, "movement", &unit->mov ) ) goto parser_failure;
        /* move type id */
        unit->mov_type = 0;
        if ( !parser_get_value( sub, "move_type", &str, 0 ) ) goto parser_failure;
        for ( i = 0; i < mov_type_count; i++ )
            if ( STRCMP( str, mov_types[i].id ) ) {
                unit->mov_type = i;
                break;
            }
        /* fuel */
        if ( !parser_get_int( sub, "fuel", &unit->fuel ) ) goto parser_failure;
        /* range */
        if ( !parser_get_int( sub, "range", &unit->rng ) ) goto parser_failure;
        /* ammo */
        if ( !parser_get_int( sub, "ammo", &unit->ammo ) ) goto parser_failure;
        /* attack count */
        if ( !parser_get_int( sub, "attack/count", &unit->atk_count ) ) goto parser_failure;
        /* attack values */
        if ( !parser_get_pdata( sub, "attack", &subsub ) ) goto parser_failure;
        for ( i = 0; i < trgt_type_count; i++ )
            if ( !parser_get_int( subsub, trgt_types[i].id, &unit->atks[i] ) ) goto parser_failure;
        /* ground defense */
        if ( !parser_get_int( sub, "def_ground", &unit->def_grnd ) ) goto parser_failure;
        /* air defense */
        if ( !parser_get_int( sub, "def_air", &unit->def_air ) ) goto parser_failure;
        /* close defense */
        if ( !parser_get_int( sub, "def_close", &unit->def_cls ) ) goto parser_failure;
        /* flags
        if ( parser_get_values( sub, "flags", &flags ) ) {
            list_reset( flags );
            while ( ( flag = list_next( flags ) ) )
                unit->flags |= check_flag( flag, fct_units );
        } */
        /* set the entrenchment rate
        unit->entr_rate = 2;
        if ( unit->flags & LOW_ENTR_RATE )
            unit->entr_rate = 1;
        else
            if ( unit->flags & INFANTRY )
                unit->entr_rate = 3; */
        /* time period of usage (0 == cannot be purchased) */
        unit->start_year = unit->start_month = unit->last_year = 0;
        parser_get_int( sub, "start_year", &unit->start_year );
        parser_get_int( sub, "start_month", &unit->start_month );
        parser_get_int( sub, "last_year", &unit->last_year );
	/* cost of unit (0 == cannot be purchased) */
	unit->cost = 0;
	parser_get_int( sub, "cost", &unit->cost );
        /* icon */
        /* icon id */
        if ( !parser_get_int( sub, "icon_id", &icon_id ) ) goto parser_failure;
        /* icon_type */
        unit->icon_type = icon_type;
        /* get position and size in icons surface */
        unit_get_icon_geometry( icon_id, icons, &width, &height, &offset, &color_key );
        /* picture is copied from unit_pics first
         * if picture_type is not ALL_DIRS, picture is a single picture looking to the right;
         * add a flipped picture looking to the left
         */
        {
            /* set size */
            unit->icon_w = width;
            unit->icon_h = height;
            /* create pic and copy first pic */
            unit->icon = create_surf( unit->icon_w * 2, unit->icon_h, SDL_SWSURFACE );
            SDL_Rect srect = {0, 0, 0, offset};
            SDL_Rect drect = {0, 0, unit->icon_w, unit->icon_h};
            SDL_BlitSurface(icons, &srect, unit->icon, &drect);
            /* remove measure dots */
            set_pixel( unit->icon, 0, 0, color_key );
            set_pixel( unit->icon, 0, unit->icon_h - 1, color_key );
            set_pixel( unit->icon, unit->icon_w - 1, 0, color_key );
            /* set transparency */
//            SDL_SetColorKey( unit->icon, SDL_SRCCOLORKEY, color_key );
            /* get format info */
            byte_size = icons->format->BytesPerPixel;
            y_offset = 0;
            pix_buffer = (char*)calloc( byte_size, sizeof( char ) );
            /* get second by flipping first one */
            for ( j = 0; j < unit->icon_h; j++ ) {
                for ( i = 0; i < unit->icon_w; i++ ) {
                    memcpy( pix_buffer,
                            (char*)unit->icon->pixels +
                            y_offset +
                            ( unit->icon_w - 1 - i ) * byte_size,
                            byte_size );
                    memcpy( (char*)unit->icon->pixels +
                            y_offset +
                            unit->icon_w * byte_size +
                            i * byte_size,
                            pix_buffer, byte_size );
                }
                y_offset += unit->icon->pitch;
            }
            /* free mem */
            free( pix_buffer );
        }
        scale = 1.5;
        unit->icon_tiny = create_surf( unit->icon->w * ( 1.0 / scale ), unit->icon->h * ( 1.0 / scale ), SDL_SWSURFACE );
        unit->icon_tiny_w = unit->icon_w * ( 1.0 / scale ); unit->icon_tiny_h = unit->icon_h * ( 1.0 / scale );
        for ( j = 0; j < unit->icon_tiny->h; j++ ) {
            for ( i = 0; i < unit->icon_tiny->w; i++ )
                set_pixel( unit->icon_tiny,
                           i, j,
                           get_pixel( unit->icon, scale * i, scale * j ) );
        }
        /* use color key of 'big' picture */
  //      SDL_SetColorKey( unit->icon_tiny, SDL_SRCCOLORKEY, color_key );
        /* add unit to database */
        list_add( unit_lib, unit );
        /* absolute evaluation
        unit_lib_eval_unit( unit ); */
        /* LOG */
        log_unit_count++;
        if ( log_unit_count >= log_units_per_dot ) {
            log_unit_count = 0;
            if ( log_dot_count < log_dot_limit ) {
                log_dot_count++;
                strcpy( log_str, "  [                                        ]" );
                for ( i = 0; i < log_dot_count; i++ )
                    log_str[3 + i] = '*';
            }
        }
    }
    parser_free( &pd );
    free(domain);
    SDL_FreeSurface(icons);
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    unit_lib_delete();
    if ( pd ) parser_free( &pd );
    free(domain);
    SDL_FreeSurface(icons);
    return 0;
}


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

	unit_lib_load("pg.udb",1);
	for (int i = 0; i < unit_class_count; i++) {
		UnitClassInfo uci;
		uci.id = unit_classes[i].id;
		uci.name = unit_classes[i].name;
		uclasses.push_back(uci);
	}
	list_reset(unit_lib);
	Unit_Lib_Entry *u;
	while ((u = (Unit_Lib_Entry*)list_next(unit_lib))) {
		UnitInfo ui;
		ui.id = u->id;
		ui.name = u->name;
		ui.icons = new GridImage(u->icon,u->icon_w,u->icon_h);
		ui.cid = u->uclass;
		units.push_back(ui);
	}
	unit_lib_delete();

	/* generate empty standard map width size */
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
	for (unsigned int i = 0; i < units.size(); i++)
		delete units[i].icons;
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

int Data::countUnitsInClass(int cid)
{
	int c = 0;
	for (unsigned int i = 0; i < units.size(); i++)
		if (units[i].cid == cid)
			c++;
	return c;
}

int Data::getUnitByIndex(int cid, int uid)
{
	for (unsigned int i = 0; i < units.size(); i++)
		if (units[i].cid == cid) {
			if (uid == 0)
				return i;
			uid--;
		}
	return 0;
}
