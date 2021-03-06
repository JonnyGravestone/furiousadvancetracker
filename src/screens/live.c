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
* \file live.c
* \brief Ecran live.
*/

#include <hel2.h>
#include <stdarg.h>

#include "../headers/fat.h"

/** \brief Permet de savoir si la popup de déplacement est affichée au dessus de l'écran. */
bool FAT_screenLive_isPopuped = 0;

/** \brief Cette variable contient le numéro de la première ligne actuellement affichée. */
u8 FAT_screenLive_currentStartLine = 0;
u8 FAT_screenLive_getCurrentStartLine (){
  return FAT_screenLive_currentStartLine;
}
void FAT_screenLive_setCurrentStartLine (u8 value){
  FAT_screenLive_currentStartLine = value;
}
void FAT_screenLive_incCurrentStartLine(u8 add){
  FAT_screenLive_currentStartLine += add;
}
void FAT_screenLive_decCurrentStartLine(u8 remove){
  FAT_screenLive_currentStartLine -= remove;
}

/** Ces buffers permettent d'éviter les effets escaliers sur les changements de valeurs dans l'écran LIVE */
u8 FAT_screenLive_bufferTempo;
u8 FAT_screenLive_bufferVolume[6];
u8 FAT_screenLive_bufferTranspose[6];
bool FAT_screenLive_haveToApplyBuffer = false;

/**
 * \brief Affiche uniquement la valeur du tempo au bon endroit.
 */
void FAT_screenLive_printTempo() {
    hel_BgTextPrintF(TEXT_LAYER, 15, 16, 0, "%3d", FAT_screenLive_bufferTempo);
    hel_BgTextPrintF(TEXT_LAYER, 21, 7, 0, "Tmpo %.3d", FAT_tracker.tempo);
}

/**
 * \brief Affiche uniquement la valeur du transpose au bon endroit.
 */
void FAT_screenLive_printTranspose() {
    hel_BgTextPrintF(TEXT_LAYER, 10, 16, 0, "%.2x", FAT_tracker.transpose);
}

/**
 * \brief Affiche uniquement la valeur du mode live au bon endroit.
 */
void FAT_screenLive_printLiveMode() {
    if (FAT_tracker.liveData.liveMode) {
        hel_BgTextPrint(TEXT_LAYER, 6, 16, 0, "MAN");
    } else {
        hel_BgTextPrintF(TEXT_LAYER, 6, 16, 0, "AUT");
    }
}

void FAT_screenLive_printVolumes() {
    hel_BgTextPrintF(TEXT_LAYER, 2, 13, 0, " %.2x %.2x %.2x %.2x %.2x %.2x",
            FAT_screenLive_bufferVolume[0] != NULL_VALUE ? FAT_screenLive_bufferVolume[0] : 0xdf,
            FAT_screenLive_bufferVolume[1] != NULL_VALUE ? FAT_screenLive_bufferVolume[1] : 0xdf,
            FAT_screenLive_bufferVolume[2] != NULL_VALUE ? FAT_screenLive_bufferVolume[2] : 0xdf,
            FAT_screenLive_bufferVolume[3] != NULL_VALUE ? FAT_screenLive_bufferVolume[3] : 0xdf,
            FAT_screenLive_bufferVolume[4] != NULL_VALUE ? FAT_screenLive_bufferVolume[4] : 0xdf,
            FAT_screenLive_bufferVolume[5] != NULL_VALUE ? FAT_screenLive_bufferVolume[5] : 0xdf);
}

void FAT_screenLive_printTransposes() {
    hel_BgTextPrintF(TEXT_LAYER, 2, 14, 0, " %.2x %.2x %.2x %.2x %.2x %.2x",
            FAT_screenLive_bufferTranspose[0],
            FAT_screenLive_bufferTranspose[1],
            FAT_screenLive_bufferTranspose[2],
            FAT_screenLive_bufferTranspose[3],
            FAT_screenLive_bufferTranspose[4],
            FAT_screenLive_bufferTranspose[5]);
}

/**
 * \brief Imprime les numéros de lignes.
 *
 * L'impression démarre depuis la valeur de FAT_screenLive_currentStartLine jusqu'à FAT_screenLive_currentStartLine + SCREENSONG_NB_LINES_ON_SCREEN
 */
void FAT_screenLive_printLineColumns() {
    u8 y = SCREENLIVE_LINE_START_Y;
    int c;
    for (c = FAT_screenLive_currentStartLine; c < (SCREENLIVE_NB_LINES_ON_SCREEN + FAT_screenLive_currentStartLine); c++) {
        hel_BgTextPrintF(TEXT_LAYER, SCREENLIVE_LINE_X, y, 0, FAT_FORMAT_LINE, c);
        y += SCREENLIVE_LINE_SIZE_Y;
    }
}

/**
 * \brief Affiche quelques infos (nom du projet, ligne actuellement sélectionnée et nom du channel)
 * sur l'écran.
 */
void FAT_screenLive_printInfos() {
    hel_BgTextPrintF(TEXT_LAYER, 21, 3, 0, "%s", FAT_tracker.songName);
    hel_BgTextPrintF(TEXT_LAYER, 21, 4, 0, "Chan %s", CHANNEL_NAME[FAT_screenLive_getCurrentSelectedColumn()]);
    hel_BgTextPrintF(TEXT_LAYER, 21, 5, 0, "Line  %.2x", FAT_screenLive_getCurrentSelectedLine());
    hel_BgTextPrintF(TEXT_LAYER, 21, 6, 0, "TSP   %2.x", FAT_tracker.transpose);

    FAT_screenLive_printLiveMode();
    FAT_screenLive_printTempo();
}

/**
 * \brief Affiche une seule séquence.
 *
 * @param channel le numéro de channel sur lequel la séquence est inscrite
 * @param lineOnScreen le numéro de ligne à l'écran, compris entre 0 et screenLive_NB_LINES_ON_SCREEN
 * @param realLine le vrai numéro de ligne dans le tracker ou la séquence a été inscrite
 */
void FAT_screenLive_printSequence(u8 channel, u8 lineOnScreen, u8 realLine) {
    if (FAT_tracker.channels[channel].sequences[realLine] != NULL_VALUE) {
        hel_BgTextPrintF(TEXT_LAYER, SCREENLIVE_SEQUENCE_LINE_X + (3 * channel),
                lineOnScreen + SCREENLIVE_LINE_START_Y, 0,
                "%.2x\0", FAT_tracker.channels[channel].sequences[realLine]);
    } else {
        hel_BgTextPrint(TEXT_LAYER, SCREENLIVE_SEQUENCE_LINE_X + (3 * channel),
                lineOnScreen + SCREENLIVE_LINE_START_Y, 0, "  ");
    }
}

/**
 * \brief Affiche toutes les séquences actuellement visibles.
 */
void FAT_screenLive_printSequences() {
    u8 c;
    u8 v;
    for (v = 0; v < SCREENLIVE_NB_LINES_ON_SCREEN; v++) {

        for (c = 0; c < 6; c++) {
            if (FAT_tracker.channels[c].sequences[v + FAT_screenLive_currentStartLine] == NULL_VALUE) {
                hel_BgTextPrint(TEXT_LAYER, SCREENLIVE_SEQUENCE_LINE_X + (c * 3),
                        v + SCREENLIVE_LINE_START_Y, 0, "  ");
            } else {
                hel_BgTextPrintF(TEXT_LAYER, SCREENLIVE_SEQUENCE_LINE_X + (c * 3),
                        v + SCREENLIVE_LINE_START_Y, 0, "%.2x ",
                        FAT_tracker.channels[c].sequences[v + FAT_screenLive_currentStartLine]);
            }
        }

    }
}

/**
 * \brief Affiche tout le texte à l'écran (numéros de lignes, séquences et infos).
 */
void FAT_screenLive_printAllScreenText() {
    FAT_screenLive_printLineColumns();
    FAT_screenLive_printSequences();
    FAT_screenLive_printInfos();
}

/**
 * \brief Initialisation de l'écran.
 */
void FAT_screenLive_init() {
    FAT_reinitScreen();

    // initialisation du fond (interface)
    ham_bg[SCREEN_LAYER].ti = ham_InitTileSet((void*)ResData(RES_SCREEN_LIVE_RAW), RES_SCREEN_LIVE_RAW_SIZE16, 1, 1);
    hel_MapCreate(SCREEN_LAYER, 32, 20, ResData(RES_SCREEN_LIVE_MAP), sizeof(u16), MAP_FLAGS_DEFAULT);

    // affichage du curseur
    FAT_cursors_hideCursor2();
    FAT_screenLive_commitCursorMove();
    FAT_cursors_showCursor2();

    FAT_screenLive_bufferTempo = FAT_tracker.tempo;

    u8 i;
    for (i = 0;i<6;i++){
        FAT_player_moveOrHideCursor(i);
        FAT_screenLive_bufferVolume[i] = FAT_tracker.liveData.volume[i];
        FAT_screenLive_bufferTranspose[i] = FAT_tracker.liveData.transpose[i];
    }

    FAT_screenLive_printAllScreenText();
    FAT_screenLive_printVolumes();
    FAT_screenLive_printTransposes();
}

/**
* \brief Applique les valeurs stockées dans les buffers sur les données réelles du tracker.
*
*/
void FAT_screenLive_applyBufferIfNeeded(){
    if (FAT_screenLive_haveToApplyBuffer){
        FAT_tracker.tempo = FAT_screenLive_bufferTempo;

        u8 i;
        for (i = 0;i<6;i++){
            FAT_tracker.liveData.volume[i] = FAT_screenLive_bufferVolume[i];
            FAT_tracker.liveData.transpose[i] = FAT_screenLive_bufferTranspose[i];
        }

        FAT_screenLive_haveToApplyBuffer = false;
        hel_BgTextPrintF(TEXT_LAYER, 21, 7, 0, "Tmpo %.3d", FAT_tracker.tempo);
    }
}

/**
 * \brief Teste les actions utilisateurs.
 */
void FAT_screenLive_checkButtons() {
    hel_PadCapture();

    if (hel_PadQuery()->Held.Select) {
        if (!FAT_screenLive_isPopuped) {
            FAT_cursors_hideCursor2();
            FAT_popup_show();
            FAT_screenLive_isPopuped = 1;
        }

        FAT_popup_checkButtons();

    } else {
        if (FAT_screenLive_isPopuped) {
            FAT_popup_hide();
            FAT_cursors_showCursor2();
            FAT_screenLive_isPopuped = 0;

            if (FAT_popup_getSelectedIcon() != SCREEN_LIVE_ID) {
                // TODO hide project cursor
                FAT_switchToScreen(FAT_popup_getSelectedIcon(), SCREEN_LIVE_ID);
            }
        }

        if (hel_PadQuery()->Pressed.R && hel_PadQuery()->Pressed.L) {
            FAT_showHelp(SCREEN_LIVE_ID);
        }

        if (hel_PadQuery()->Pressed.A || hel_PadQuery()->Held.A) {
            if (FAT_screenLive_getIsCursorInSequencer()){
                FAT_screenLive_pressOrHeldA_inSequencer();
            } else {
                FAT_screenLive_pressOrHeldA_inDataTable();
            }
        } else {
            // si des valeurs ont bougées, on les applique maintenant puisque le bouton A n'est pas enfoncé.
            FAT_screenLive_applyBufferIfNeeded ();

            if (hel_PadQuery()->Pressed.Right) {
                FAT_screenLive_moveCursorRight();
            }

            if (hel_PadQuery()->Pressed.Left) {
                FAT_screenLive_moveCursorLeft();
            }

            if (hel_PadQuery()->Pressed.Down) {
                if (hel_PadQuery()->Held.R) {
                    // changer de portion d'écran -> du séquenceur vers la table
                    FAT_screenLive_switchActivePart(0);

                } else if (hel_PadQuery()->Held.L) {
                    FAT_screenLive_moveCursorAllDown();
                } else {
                    FAT_screenLive_moveCursorDown();
                }
            }

            if (hel_PadQuery()->Pressed.Up) {
                if (hel_PadQuery()->Held.R) {
                    // changer de portion d'écran -> de la table vers le sequenceur
                    FAT_screenLive_switchActivePart(1);

                } else if (hel_PadQuery()->Held.L) {
                    FAT_screenLive_switchActivePart(1);
                    FAT_screenLive_moveCursorAllUp();
                } else {
                    FAT_screenLive_moveCursorUp();
                }
            }
        }

        if (hel_PadQuery()->Pressed.Start) {
            if(hel_PadQuery()->Held.R){
                // tout se lance en maintenant à partir de la ligne sélectionnée.
                if (!FAT_getIsCurrentlyPlaying()) {
                    FAT_player_startPlayerFromSequences(FAT_screenLive_getCurrentSelectedLine());
                } else {
                    FAT_player_stopPlayer();
                }
            }else {
                if (!FAT_getIsCurrentlyPlaying()) {
                    // seule la colonne sélectionnée se lance à partir de la ligne courante.
                    FAT_player_startPlayerFromLive_oneChannel(FAT_screenLive_getCurrentSelectedLine(), FAT_screenLive_getCurrentSelectedColumn());
                } else {
                    // si le channel joue actuellement, on l'arrete.
                    if (FAT_isChannelCurrentlyPlaying (FAT_screenLive_getCurrentSelectedColumn())){
                        FAT_player_stopPlayer_onlyOneColumn(FAT_screenLive_getCurrentSelectedColumn());
                    } else {
                        // sinon on le démarre
                        FAT_player_startPlayerFromLive_oneChannel(FAT_screenLive_getCurrentSelectedLine(), FAT_screenLive_getCurrentSelectedColumn());
                    }
                }
            }
        }

        if (!FAT_tracker.bufferChangeInLive){
            FAT_screenLive_applyBufferIfNeeded();
        }

        FAT_screenLive_commitCursorMove();
    }
}

void FAT_screenLive_pressOrHeldA_inSequencer(){

}

void FAT_screenLive_pressOrHeldA_inDataTable(){
    switch (FAT_screenLive_getCurrentTableSelectedLine()){
        case 0:
            // volume colonne
            if (hel_PadQuery()->Pressed.Right) {
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_changeAllVolumes (1);
                } else{
                    FAT_screenLive_changeVolume (FAT_screenLive_getCurrentSelectedColumn(), 1);
                }
            }

            if (hel_PadQuery()->Pressed.Left) {
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_changeAllVolumes (-1);
                } else{
                    FAT_screenLive_changeVolume (FAT_screenLive_getCurrentSelectedColumn(), -1);
                }

            }

            if (hel_PadQuery()->Pressed.Up){
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_setAllVolumes (0xf);
                } else {
                    FAT_screenLive_changeVolume (FAT_screenLive_getCurrentSelectedColumn(), NULL_VALUE - FAT_screenLive_bufferVolume[FAT_screenLive_getCurrentSelectedColumn()]);
                }
            }

            if (hel_PadQuery()->Pressed.Down){
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_setAllVolumes (0);
                }else{
                    FAT_screenLive_changeVolume (FAT_screenLive_getCurrentSelectedColumn(), -FAT_screenLive_bufferVolume[FAT_screenLive_getCurrentSelectedColumn()]);
                }

            }

            FAT_screenLive_printVolumes();
            break;
        case 1:
            // tsp column
            if (hel_PadQuery()->Pressed.Right ) {
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_changeAllTransposes (1);
                } else {
                    FAT_screenLive_changeTranspose (FAT_screenLive_getCurrentSelectedColumn(), 1);
                }
            }

            if (hel_PadQuery()->Pressed.Left ) {
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_changeAllTransposes (-1);
                } else {
                    FAT_screenLive_changeTranspose (FAT_screenLive_getCurrentSelectedColumn(), -1);
                }
            }

            if (hel_PadQuery()->Pressed.Up) {
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_changeAllTransposes (12);
                } else {
                    FAT_screenLive_changeTranspose (FAT_screenLive_getCurrentSelectedColumn(), 12);
                }
            }

            if (hel_PadQuery()->Pressed.Down) {
                if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Held.L){
                    FAT_screenLive_changeAllTransposes (-12);
                } else {
                    FAT_screenLive_changeTranspose (FAT_screenLive_getCurrentSelectedColumn(), -12);
                }
            }

            FAT_screenLive_printTransposes();
            break;
        case 2:
            // mode, tempo
            if (FAT_screenLive_getCurrentSelectedColumn() == 1){
                if (hel_PadQuery()->Pressed.Right) {
                    FAT_tracker.liveData.liveMode = 0;
                }

                if (hel_PadQuery()->Pressed.Left) {
                    FAT_tracker.liveData.liveMode = 1;
                }

                FAT_screenLive_printLiveMode();
            } else if (FAT_screenLive_getCurrentSelectedColumn() == 4){
                if (hel_PadQuery()->Pressed.Right && FAT_screenLive_bufferTempo < MAX_TEMPO) {
                    FAT_screenLive_bufferTempo ++;
                    FAT_screenLive_haveToApplyBuffer = true;
                }

                if (hel_PadQuery()->Pressed.Left && FAT_screenLive_bufferTempo > 0x00) {
                    FAT_screenLive_bufferTempo --;
                    FAT_screenLive_haveToApplyBuffer = true;
                }

                if (hel_PadQuery()->Pressed.Up ) {
                    if (FAT_screenLive_bufferTempo < MAX_TEMPO-10){
                        FAT_screenLive_bufferTempo += 10;
                    } else {
                        FAT_screenLive_bufferTempo = 255;
                    }

                    FAT_screenLive_haveToApplyBuffer = true;
                }

                if (hel_PadQuery()->Pressed.Down) {
                    if (FAT_screenLive_bufferTempo > 10){
                        FAT_screenLive_bufferTempo -= 10;
                    } else {
                        FAT_screenLive_bufferTempo = 0;
                    }

                    FAT_screenLive_haveToApplyBuffer = true;
                }

                FAT_screenLive_printTempo();
            }
            break;
    }
}

void FAT_screenLive_changeVolume (u8 channel, s8 addedValue){
    if (addedValue > 0 && FAT_screenLive_bufferVolume[channel] < NULL_VALUE){

        FAT_screenLive_bufferVolume[channel] += addedValue;

        if ( (channel != 2 && FAT_screenLive_bufferVolume[channel] > 0xf)
                            || (channel == 2 && FAT_screenLive_bufferVolume[channel] > 0x4) ){
            FAT_screenLive_bufferVolume[channel] = NULL_VALUE;
        }

        FAT_screenLive_haveToApplyBuffer = true;

    } else if (addedValue < 0 && FAT_screenLive_bufferVolume[channel] > 0){

        FAT_screenLive_bufferVolume[channel] += addedValue;

        if ( (channel != 2 && FAT_screenLive_bufferVolume[channel] > 0xf) ){
            FAT_screenLive_bufferVolume[channel] = 0x0f;
        } else if ((channel == 2 && FAT_screenLive_bufferVolume[channel] > 0x4)){
            FAT_screenLive_bufferVolume[channel] = 0x04;
        }

        FAT_screenLive_haveToApplyBuffer = true;
    }
}

void FAT_screenLive_changeTranspose (u8 channel, s8 addedValue){
    FAT_screenLive_bufferTranspose[channel] += addedValue; // no controls ! let's tourn around FF and 00
    FAT_screenLive_haveToApplyBuffer = true;
}

void FAT_screenLive_changeAllVolumes (s8 addedValue){
    u8 i;
    for (i = 0; i < 6; i++){
        FAT_screenLive_changeVolume (i, addedValue);
    }
}

void FAT_screenLive_changeAllTransposes (s8 addedValue){
    u8 i;
    for (i = 0; i < 6; i++){
        FAT_screenLive_changeTranspose (i, addedValue);
    }
}

void FAT_screenLive_setAllVolumes (u8 value){
    u8 i;
    for (i = 0; i < 6; i++){
        FAT_screenLive_bufferVolume[i] = value;
        if (i == 2){
            if (FAT_screenLive_bufferVolume[i] > 0x4){
                FAT_screenLive_bufferVolume[i] = 0x04;
            }
        }
    }
    FAT_screenLive_haveToApplyBuffer = true;
}
