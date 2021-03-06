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
* \file keyboard.c
* \brief Gestion des dialogues de type confirmation (yes or no).
*/

#include <hel2.h>
#include <stdarg.h>

#include "../headers/fat.h"

/** \brief Variable pour stocker un premier paramètre (variable) pour une boite de dialogue. */
u8 param1;

/**
 * \brief Fonction principale de la boite de dialog SAVE
 */
void FAT_yesno_dialogSave_mainFunc() {
    hel_ObjTransmit();

    hel_PadCapture();
    if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Pressed.Select) {
        FAT_yesno_close();
        hel_BgTextPrint(TEXT_LAYER, 24, 16, 0, "Cancel");
    } else if (hel_PadQuery()->Pressed.R || hel_PadQuery()->Pressed.Start) {
        FAT_filesystem_saveTrack(param1);//with RLE compression :)
        FAT_yesno_close();
        hel_BgTextPrint(TEXT_LAYER, 24, 16, 0, "Saved ");
    }
}

/**
 * \brief Fonction principale de la boite de dialog LOAD
 */
void FAT_yesno_dialogLoad_mainFunc() {
    hel_ObjTransmit();

    hel_PadCapture();
    if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Pressed.Select) {
        FAT_yesno_close();
        hel_BgTextPrint(TEXT_LAYER, 24, 16, 0, "Cancel");
    } else if (hel_PadQuery()->Pressed.R || hel_PadQuery()->Pressed.Start) {
        FAT_player_stopPlayer();
        FAT_filesystem_loadTrack(param1);
        FAT_setCurrentScreen(SCREEN_PROJECT_ID);
        FAT_yesno_close();
        hel_BgTextPrint(TEXT_LAYER, 24, 16, 0, "Loaded");
    }
}

/**
 * \brief Fonction principale de la boite de dialog NEW PROJECT.
 */
void FAT_yesno_dialogNewProject_mainFunc() {
    hel_ObjTransmit();

    hel_PadCapture();
    if (hel_PadQuery()->Pressed.L || hel_PadQuery()->Pressed.Select) {
        FAT_yesno_close();
        hel_BgTextPrint(TEXT_LAYER, 24, 16, 0, "Cancel");
    } else if (hel_PadQuery()->Pressed.R || hel_PadQuery()->Pressed.Start) {
        FAT_player_stopPlayer();

        // déplacer le curseur de la popup. C'est un peu naze comme ça. Mais c'est comme mon envie de faire mieux ... naze.
        FAT_popup_moveSelectionCursorRight();
        FAT_popup_moveSelectionCursorRight();
        FAT_popup_moveSelectionCursorRight();
        FAT_popup_setSelectedIcon(SCREEN_SONG_ID);

        FAT_data_initData();
        FAT_setCurrentScreen(SCREEN_SONG_ID);
        FAT_yesno_close();
        hel_BgTextPrint(TEXT_LAYER, 22, 16, 0, "Created");
    }
}

/**
 * \brief Fonction principale pour une boite ou il suffit de répondre "OUI"
 */
void FAT_onlyyes_dialog_mainFunc() {
    hel_ObjTransmit();
    hel_PadCapture();
    if (hel_PadQuery()->Pressed.R) {
        FAT_yesno_close();
    }
}

/**
 * \brief Ferme la boite de dialogue et reinitialise la popup (qui est sur le même layer).
 */
void FAT_yesno_close() {
    FAT_popup_init();
    hel_IntrUpdateHandler(INT_TYPE_VBL, (void*) &VBLInterruptHandler);
    FAT_switchToScreen(FAT_getCurrentScreen(), SCREEN_DIALOG_ANY);
}

/**
 * \brief Ouvre la boite de dialogue (modale) permettant la sauvegarde du projet.
 */
void FAT_yesno_dialogSave() {

    ham_bg[DIALOG_LAYER].ti = ham_InitTileSet((void*)ResData(RES_SCREEN_DIALOG_SAVE_RAW), RES_SCREEN_DIALOG_SAVE_RAW_SIZE16, 1, 1);
    hel_MapCreate(DIALOG_LAYER, 32, 20, ResData(RES_SCREEN_DIALOG_SAVE_MAP), sizeof(u16), MAP_FLAGS_DEFAULT);
    ham_InitBg(DIALOG_LAYER, 1, 0, 0);

    hel_IntrUpdateHandler(INT_TYPE_VBL, (void*) &FAT_yesno_dialogSave_mainFunc);

}

/**
 * \brief Ouvre la boite de dialogue (modale) pour le chargement du projet.
 */
void FAT_yesno_dialogLoad() {

    ham_bg[DIALOG_LAYER].ti = ham_InitTileSet((void*)ResData(RES_SCREEN_DIALOG_LOAD_RAW), RES_SCREEN_DIALOG_LOAD_RAW_SIZE16, 1, 1);
    hel_MapCreate(DIALOG_LAYER, 32, 20, ResData(RES_SCREEN_DIALOG_LOAD_MAP), sizeof(u16), MAP_FLAGS_DEFAULT);
    ham_InitBg(DIALOG_LAYER, 1, 0, 0);

    hel_IntrUpdateHandler(INT_TYPE_VBL, (void*) &FAT_yesno_dialogLoad_mainFunc);


}

/**
 * \brief Boite de dialogue indiquant pour la confirmation d'un nouveau projet.
 *
 */
void FAT_yesno_dialogNewProject() {
    ham_bg[DIALOG_LAYER].ti = ham_InitTileSet((void*)ResData(RES_SCREEN_DIALOG_NEW_RAW), RES_SCREEN_DIALOG_NEW_RAW_SIZE16, 1, 1);
    hel_MapCreate(DIALOG_LAYER, 32, 20, ResData(RES_SCREEN_DIALOG_NEW_MAP), sizeof(u16), MAP_FLAGS_DEFAULT);
    ham_InitBg(DIALOG_LAYER, 1, 0, 0);

    hel_IntrUpdateHandler(INT_TYPE_VBL, (void*) &FAT_yesno_dialogNewProject_mainFunc);
}

/**
 * \brief Fonction wrapper permettant l'ouverture de toutes les boites de dialogues définies.
 *
 * @param idDialog l'id de la boite de dialogue
 */
void FAT_yesno_show(u8 idDialog, ...) {

    va_list params;
    va_start(params, idDialog);

    if (ham_bg[DIALOG_LAYER].ti) {
        ham_DeInitTileSet(ham_bg[DIALOG_LAYER].ti);
        ham_DeInitMapSet(ham_bg[DIALOG_LAYER].mi);
    }

    switch (idDialog) {
        case DIALOG_SAVE:
            param1 = va_arg(params, int);
            FAT_yesno_dialogSave();
            break;
        case DIALOG_LOAD:
            param1 = va_arg(params, int);
            FAT_yesno_dialogLoad();
            break;
        case DIALOG_NEW:
            FAT_yesno_dialogNewProject();
            break;
    }

    va_end(params);
}
