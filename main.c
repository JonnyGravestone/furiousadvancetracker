/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @mainpage FuriousAdvanceTracker
 * 
 * FAT (FuriousAdvanceTracker) est un logiciel concu pour la composition musicale sur Gameboy Advance. 
 * <b>FAT est actuellement en version de test (il n'est pas encore complet).</b><br/>
 * FAT est un <a href="http://fr.wikipedia.org/wiki/Tracker_%28musique%29">tracker</a> dont l'interface 
 * s'inspire très profondément de <a href="http://www.littlesounddj.com">LSDJ</a> (LittleSoundDJ), 
 * un logiciel très connu utilisable sur Gameboy. Malgré cela, FAT se démarque en ajoutant des fonctionnalités 
 * inédites pour la composition de chiptunes sur GBA.
 *       
 * - la GameboyAdvance dispose de 6 canaux sonores. FAT les exploite(ra) tous.
 * - FAT est (sera) orienté live. Un écran spécial est même prévu pour improviser des lignes mélodiques en live.
 * - et plein d'autres idées qui s'ajouteront au fur et à mesure !
 *
 * FAT est gratuit et libre ! Vous pouvez le télécharger, l'utiliser avec un émulateur ou le graver
 * sur une cartouche GBA. Si vous remarquer un bug ou un comportement bizarre, faites un rapport de bug :
 * c'est simple, rapide, et surtout cela permet de faire évoluer le logiciel plus efficacement.<br/>
 * 
 * <b>Vous êtes actuellement en train de lire une documentation technique réservé aux développeurs. Si vous n'êtes
 * pas développeur et souhaitez simplement tester FAT, alors VOUS N'ETES PAS AU BON ENDROIT !. Si c'est le cas, retourner sur 
 * <a href="http://www.furiousadvancetracker.com">www.furiousadvancetracker.com</a> et téléchargez le manuel utilisateur au format PDF. Merci !</b>
 *
 */

/**
 * \file main.c 
 * \brief Fichier code source principal avec la fonction main et l'inclusion de fat.h.
 */

/**
 * \brief Si cette variable est positionné à 1, le mode DEBUG pour FAT est activé. Certaines,
 * informations seront affichées.
 */
#define DEBUG_ON 1
#undef DEBUG_ON

/**
 * \brief Version actuelle de FAT.
 */
#define FAT_VERSION "0.3.0"

#include <mygba.h>
#include <stdarg.h>

#include "soundApi/soundApi.h"
#include "fat.h"


bool FAT_newFrame = 0;

void FAT_mainVbl_func() {
    FAT_newFrame = 1;
}

void FAT_checkButtons() {
    switch (FAT_currentScreen) {
        case SCREEN_PROJECT_ID:
            FAT_screenProject_checkButtons();
            break;
        case SCREEN_LIVE_ID:
            FAT_screenLive_checkButtons();
            break;
        case SCREEN_SONG_ID:
            FAT_screenSong_checkButtons();
            break;
        case SCREEN_BLOCKS_ID:
            FAT_screenBlocks_checkButtons();
            break;
        case SCREEN_NOTES_ID:
            FAT_screenNotes_checkButtons();
            break;
        case SCREEN_EFFECTS_ID:
            FAT_screenEffects_checkButtons();
            break;
        case SCREEN_COMPOSER_ID:
            FAT_screenComposer_checkButtons();
            break;
        case SCREEN_INSTRUMENTS_ID:
            FAT_screenInstrument_checkButtons();
            break;
        case SCREEN_FILESYSTEM_ID:
            FAT_screenFilesystem_checkButtons();
            break;
        case SCREEN_HELP_ID:
            FAT_screenHelp_checkButtons();
            break;
    }
}

/**
 * \brief Fonction main. Si vous lisez le code source, il est judicieux de commencer par comprendre cette fonction.
 */
int main() {

    FAT_init();
    snd_init_soundApi();
    snd_init_kits();
    snd_onlyFAT_initMutex(&sampleMutex);

    FAT_showIntro();

    ham_StartIntHandler(INT_TYPE_VBL, (void*) &FAT_mainVbl_func);
    FAT_screenSong_init();

    while (1) {

        if (FAT_newFrame) {
            if (mutex) {

                if (iCanPressAKey) {
                    FAT_checkButtons();
                }

                ham_CopyObjToOAM();
            }

            FAT_newFrame = 0;
        }

    }
    return 0;
}


