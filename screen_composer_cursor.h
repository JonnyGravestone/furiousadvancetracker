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
 * \file screen_composer_cursor.h
 * \brief Ce fichier permet de gérer le curseur sur l'écran COMPOSER.
 */

#ifndef _SCREEN_COMPOSER_CURSOR_H_
#define	_SCREEN_COMPOSER_CURSOR_H_

/** \brief Position actuelle du curseur pour le composeur. */
u8 FAT_screenComposer_cursorX; 
/** \brief Position actuelle du curseur pour le composeur. */
u8 FAT_screenComposer_cursorY;
/** \brief Numéro de ligne actuellement sélectionné par le curseur. */
u8 FAT_screenComposer_currentSelectedLine; 
/** \brief Numéro de colonne actuellement sélectionné par le curseur. */
u8 FAT_screenComposer_currentSelectedColumn;

// taille d'un block 
/** \brief Taille d'un block mesurant 3 cases (en pixels). */
#define SCREENCOMPOSER_BLOCK3_SIZE_X 24
/** \brief Taille d'un block mesurant 2 cases (en pixels). */
#define SCREENCOMPOSER_BLOCK2_SIZE_X 16
/** \brief Taille d'un block en hauteur (en pixels). */
#define SCREENCOMPOSER_BLOCK_SIZE_Y 8

// paramètre de l'interface
/** \brief Taille de l'espace blanc entre chaque block sur X. */
#define SCREENCOMPOSER_WHITE_SPACE_X 8
/** \brief Position Y du dernier block (en pixels). */
#define SCREENCOMPOSER_LAST_BLOCK_Y 111
/** \brief Position X du premier block (en pixels). */
#define SCREENCOMPOSER_FIRST_BLOCK_X 88
/** \brief Position Y du premier block (en pixels). */
#define SCREENCOMPOSER_FIRST_BLOCK_Y 56

// prototypes
void FAT_screenComposer_initCursor();
void FAT_screenComposer_moveCursorRight();
void FAT_screenComposer_moveCursorLeft();
void FAT_screenComposer_moveCursorDown();
void FAT_screenComposer_moveCursorUp();
void FAT_screenComposer_commitCursorMove();

/**
 * \brief Initialise le curseur pour le COMPOSER.
 */
void FAT_screenComposer_initCursor() {
    FAT_screenComposer_cursorX = SCREENCOMPOSER_FIRST_BLOCK_X - 1;
    FAT_screenComposer_cursorY = SCREENCOMPOSER_FIRST_BLOCK_Y - 1;

    FAT_screenComposer_currentSelectedLine = 0;
    FAT_screenComposer_currentSelectedColumn = 0;
}

/**
 * \brief Valide le déplacement du curseur et l'écrit dans la mémoire de la GBA. 
 */
void FAT_screenComposer_commitCursorMove() {
    if (FAT_screenComposer_currentSelectedColumn == 0) {
        ham_SetObjXY(FAT_cursor3_obj, FAT_screenComposer_cursorX, FAT_screenComposer_cursorY);
    } else {
        ham_SetObjXY(FAT_cursor2_obj, FAT_screenComposer_cursorX, FAT_screenComposer_cursorY);
    }
}

/**
 * \brief Déplace le curseur vers le bas.
 * 
 * Attention, la validation du déplacement doit être effectuée avec FAT_screenComposer_commitCursorMove() .
 */
void FAT_screenComposer_moveCursorDown() {
    if (FAT_screenComposer_currentSelectedLine < SCREENCOMPOSER_NB_LINES_ON_SCREEN) {
        if (!(FAT_screenComposer_cursorY >= SCREENCOMPOSER_LAST_BLOCK_Y - 1)) {
            FAT_screenComposer_cursorY += SCREENCOMPOSER_BLOCK_SIZE_Y;
            FAT_screenComposer_currentSelectedLine++;
            FAT_screenComposer_printInfos();
        }
    }
}

/**
 * \brief Déplace le curseur vers le haut.
 * 
 * Attention, la validation du déplacement doit être effectuée avec FAT_screenComposer_commitCursorMove() .
 */
void FAT_screenComposer_moveCursorUp() {

    if (FAT_screenComposer_currentSelectedLine > 0) {
        if (!(FAT_screenComposer_cursorY <= SCREENCOMPOSER_FIRST_BLOCK_Y - 1)) {
            FAT_screenComposer_cursorY -= SCREENCOMPOSER_BLOCK_SIZE_Y;
            FAT_screenComposer_currentSelectedLine--;
            FAT_screenComposer_printInfos();
        }
    }

}

/**
 * \brief Déplace le curseur vers la droite.
 * 
 * Attention, la validation du déplacement doit être effectuée avec FAT_screenComposer_commitCursorMove() .
 */
void FAT_screenComposer_moveCursorRight(){
    
    if (FAT_screenComposer_currentSelectedColumn == SCREENCOMPOSER_COLUMN_ID_NOTES){
        
        FAT_screenComposer_cursorX += SCREENCOMPOSER_BLOCK3_SIZE_X + SCREENCOMPOSER_WHITE_SPACE_X;
        FAT_screenComposer_currentSelectedColumn = SCREENCOMPOSER_COLUMN_ID_INST;
        FAT_cursors_hideCursor3();
        FAT_cursors_showCursor2();
    
    } else  if (FAT_screenComposer_currentSelectedColumn == SCREENCOMPOSER_COLUMN_ID_INST){
        
        FAT_screenComposer_cursorX += SCREENCOMPOSER_BLOCK2_SIZE_X + SCREENCOMPOSER_WHITE_SPACE_X;
        FAT_screenComposer_currentSelectedColumn = SCREENCOMPOSER_COLUMN_ID_CMD_NAME;
    
    } else if (FAT_screenComposer_currentSelectedColumn == SCREENCOMPOSER_COLUMN_ID_CMD_NAME){
        
        FAT_screenComposer_cursorX += SCREENCOMPOSER_BLOCK2_SIZE_X;
        FAT_screenComposer_currentSelectedColumn = SCREENCOMPOSER_COLUMN_ID_CMD_PARAM;
    }
    
}

/**
 * \brief Déplace le curseur vers la gauche.
 * 
 * Attention, la validation du déplacement doit être effectuée avec FAT_screenComposer_commitCursorMove() .
 */
void FAT_screenComposer_moveCursorLeft(){
    if (FAT_screenComposer_currentSelectedColumn == SCREENCOMPOSER_COLUMN_ID_INST){
        
        FAT_screenComposer_cursorX -= SCREENCOMPOSER_BLOCK3_SIZE_X + SCREENCOMPOSER_WHITE_SPACE_X;
        FAT_screenComposer_currentSelectedColumn = SCREENCOMPOSER_COLUMN_ID_NOTES;
        FAT_cursors_hideCursor2();
        FAT_cursors_showCursor3();
    
    } else  if (FAT_screenComposer_currentSelectedColumn == SCREENCOMPOSER_COLUMN_ID_CMD_NAME){
    
        FAT_screenComposer_cursorX -= SCREENCOMPOSER_BLOCK2_SIZE_X + SCREENCOMPOSER_WHITE_SPACE_X;
        FAT_screenComposer_currentSelectedColumn = SCREENCOMPOSER_COLUMN_ID_INST;
    
    } else if (FAT_screenComposer_currentSelectedColumn == SCREENCOMPOSER_COLUMN_ID_CMD_PARAM){
        FAT_screenComposer_cursorX -= SCREENCOMPOSER_BLOCK2_SIZE_X;
        FAT_screenComposer_currentSelectedColumn = SCREENCOMPOSER_COLUMN_ID_CMD_NAME;
    }
}



#endif	/* SCREEN_COMPOSER_CURSOR_H */

