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
* \file filesystem.c
* \brief Code contenant les routines permettant d'accéder au système de fichiers.
*/

#include <hel2.h>
#include <stdarg.h>

#include "../headers/fat.h"

/*
 * Toutes ces variables sont des repères pour le player. Afin de savoir quelle séquence/block/note jouer.
 * Attention. Les valeurs dans les tableaux représentent des NUMEROS DE LIGNES.
 * Ce ne sont pas les ids des séquences/blocks/notes.
 * Les 3 autres variables représentent, elles, des ids d'objets.
 */
/** \brief Repères pour le player: les valeurs dans les tableaux représentent des numéros de lignes (séquences). */
u8 actualSequencesForChannel[6];
/** \brief Repères pour le player: les valeurs dans les tableaux représentent des numéros de lignes (blocks). */
u8 actualBlocksForChannel[6];
/** \brief Repères pour le player: les valeurs dans les tableaux représentent des numéros de lignes (notes). */
u8 actualNotesForChannel[6];

/** \brief Id de la séquence en cours de lecture. */
u8 FAT_currentPlayedSequence = NULL_VALUE;
u8 FAT_player_getCurrentPlayedSequence (){
  return FAT_currentPlayedSequence;
}
/** \brief Numéro du channel en cours de lecture. */
u8 FAT_currentPlayedChannel = NULL_VALUE;
/** \brief Id du block en cours de lecture. */
u8 FAT_currentPlayedBlock = NULL_VALUE;
u8 FAT_player_getCurrentPlayedBlock (){
  return FAT_currentPlayedBlock;
}

/**
 * \brief Permet de savoir si le player est en train de jouer la chanson.
 */
bool FAT_isCurrentlyPlaying = 0;
bool FAT_getIsCurrentlyPlaying(){
  return FAT_isCurrentlyPlaying;
}

bufferPlayer FAT_player_buffer[6];
u8 firstAvailableSequenceForChannel[6];

/**
 * \brief Compteur pour le tempo.
 *
 * Lorsque cette variable atteint une valeur donné, alors la lecture de la note suivante est demandée. Le compteur
 * est réinitialisé ensuite.
 * Dans le cas contraire, on attend sans jouer de note.
 */
volatile int tempoReach = (60000 / 128) * 4;

/**
 * /brief Compteur de tick. Utile pour gérer les effets qui se joue dans le temps.
*/
volatile int tickCounter = (60000 / 100) * 4;

/**
 * /brief Pour savoir de quelle endroit l'utilisateur a lancé le son.
*/
u8 FAT_player_isPlayingFrom;

/**
 * /brief Stocke des booleens qui permettent de savoir si tel ou tel channel attend avant de jouer (=1) ou non (=0).
*/
u8 FAT_live_waitForOtherChannel[6];
u8 FAT_player_live_getWaitForOtherChannel (u8 n){
  return FAT_live_waitForOtherChannel[n];
}

/**
* \brief Pointeurs vers les fonctions de jeu de note.
*/
void (*FAT_player_playNoteWithCustomParams_chanX[6]) (note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output);

void FAT_player_firstInit (){
    u8 i;
    //R_TIM3CNT = 0x0003;

    memset(actualSequencesForChannel, NULL_VALUE, sizeof (u8)*6);
    memset(actualBlocksForChannel, 0, sizeof (u8)*6);
    memset(actualNotesForChannel, 0, sizeof (u8)*6);
    memset(firstAvailableSequenceForChannel, 0, sizeof (u8)*6);
    memset(FAT_live_waitForOtherChannel, 0, sizeof (u8)*6);
    memset(FAT_player_buffer, 0, sizeof(bufferPlayer)*6);
    for (i=0;i<6;i++){
        FAT_player_buffer[i].volume = NULL_VALUE;
        FAT_player_buffer[i].sweep = NULL_VALUE;
        FAT_player_buffer[i].output = NULL_VALUE;
    }

    FAT_player_playNoteWithCustomParams_chanX[0] = FAT_player_playNoteWithCustomParams_chan1;
    FAT_player_playNoteWithCustomParams_chanX[1] = FAT_player_playNoteWithCustomParams_chan2;
    FAT_player_playNoteWithCustomParams_chanX[2] = FAT_player_playNoteWithCustomParams_chan3;
    FAT_player_playNoteWithCustomParams_chanX[3] = FAT_player_playNoteWithCustomParams_chan4;
    FAT_player_playNoteWithCustomParams_chanX[4] = FAT_player_playNoteWithCustomParams_chan5;
    FAT_player_playNoteWithCustomParams_chanX[5] = FAT_player_playNoteWithCustomParams_chan6;

    hel_IntrStartHandler(INT_TYPE_TIM3, (void*)&FAT_player_timerFunc);

    FAT_player_initCursors();
    FAT_resetTempo();
}

void FAT_player_runTimer (){
  M_TIM3COUNT_SET(0xffff - (0x4000 / 1000))
  // 0xffff = overflow
  // 0x4000 = 4000 cycles = 1 secondes avec SpeedSelect = 3
  // divisé par 1000 pour obtenir la ms (le tempo est calculé en ms)
  M_TIM3CNT_SPEED_SELECT_SET(3)
  M_TIM3CNT_IRQ_ENABLE
  M_TIM3CNT_TIMER_START

  /** EXTRAITS DE CODE SAMPLE HELL
  ham_StartIntHandler(INT_TYPE_TIM3,(void*)&ham_prof_timer3_int);
  M_TIM3COUNT_SET(65536-4399)
  M_TIM3CNT_SPEED_SELECT_SET(1)
  M_TIM3CNT_IRQ_ENABLE
  M_TIM3CNT_TIMER_START

  M_TIM3CNT_TIMER_STOP \
  M_TIM3CNT_IRQ_DISABLE \
  */
}

void FAT_player_stopTimer (){
  M_TIM3CNT_TIMER_STOP
  M_TIM3CNT_IRQ_DISABLE
}

/**
 * \brief Joue une note écrite dans le composer.
 *
 * @param noteLine le numéro de ligne de la note dans le composer
 */
void FAT_player_playComposerNote(u8 noteLine) {
    note* note = &(FAT_tracker.composer.notes[noteLine]);

    if (note->freq != NULL_VALUE) {

        FAT_player_playNoteWithCustomParams(    note,
                                                FAT_tracker.composer.channels[noteLine],
                                                FAT_tracker.composer.transpose + FAT_tracker.transpose,
                                                NULL_VALUE, NULL_VALUE, NULL_VALUE); // TODO commandes et paramètres.
    }
}

/**
 * \brief Joue une note sur un channel.
 *
 * Seule la valeur de transpose du projet est utilisée. Cette méthode est utile pour les previews et simulators.
 *
 * @param note l'objet NOTE à jouer
 * @param channel le channel sur lequel jouer la note
 */
void FAT_player_playNote(note* note, u8 channel, u8 forceVolume) {
  	// hack pour lire les notes types OSC. Pas terrible je sais :(
  	if (channel > INSTRUMENT_TYPE_SAMPLEB){
  		channel -= 2;
  	}

    FAT_player_playNoteWithCustomParams(note, channel, FAT_tracker.transpose, forceVolume, NULL_VALUE, NULL_VALUE);
}

void FAT_player_playNoteWithCustomParams_chan1(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output){
    instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);

    u8 sweepValue = sweep != NULL_VALUE ? sweep : inst->sweep;
    u16 sweepshifts = (sweepValue & 0x70) >> 4;
    u16 sweeptime = (sweepValue & 0x0F);
    u16 sweepdir = 1;
    if (sweeptime > 7) {
        sweeptime -= 8;
        sweepdir = 0;
    }

    snd_playSoundOnChannel1(
        sweeptime, sweepdir, sweepshifts,
        volume != 0xff ? volume : inst->envelope & 0x0f, (inst->envelope & 0x10) >> 4, (inst->envelope & 0xe0) >> 5, inst->wavedutyOrPolynomialStep,
        inst->soundlength, inst->loopmode,
        output,
        note->freq, transpose + FAT_tracker.transpose);

}
void FAT_player_playNoteWithCustomParams_chan2(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output){
    instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);

    snd_playSoundOnChannel2(
        volume != 0xff ? volume : inst->envelope & 0x0f, (inst->envelope & 0x10) >> 4, (inst->envelope & 0xe0) >> 5,
        inst->wavedutyOrPolynomialStep,
        inst->soundlength, inst->loopmode,
        output,
        note->freq, transpose + FAT_tracker.transpose);

}
void FAT_player_playNoteWithCustomParams_chan3(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output){
    instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);
    volume = (volume > 0x4) ? 0xff : volume;

    snd_playSoundOnChannel3(
        volume != 0xff ? volume : inst->volumeRatio & 0x0f, inst->soundlength, inst->loopmode, inst->voiceAndBank & 0x1f,
        (inst->voiceAndBank & 0x20) >> 5, (inst->voiceAndBank & 0x40) >> 6,
        output, note->freq, transpose + FAT_tracker.transpose);

}
void FAT_player_playNoteWithCustomParams_chan4(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output){
    instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);

    snd_playSoundOnChannel4(
        volume != 0xff ? volume : inst->envelope & 0x0f, (inst->envelope & 0x10) >> 4, (inst->envelope & 0xe0) >> 5, inst->soundlength,
        inst->loopmode, output, note->note & 0x0f, inst->wavedutyOrPolynomialStep,
        note->freq / NB_FREQUENCES, transpose + FAT_tracker.transpose);

}
void FAT_player_playNoteWithCustomParams_chan5(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output){
    instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);
    // todo pointeur de func pour plus de perf ?
    if (inst->type > INSTRUMENT_TYPE_SAMPLEB){
        // waveduty = shape
        snd_playOscillatorA (inst->wavedutyOrPolynomialStep, note->freq, inst->loopmode, inst->soundlength);
    } else {
        snd_playChannelASample(
            inst->kitNumber, note->freq, inst->volumeRatio >> 4,
            inst->speedOrLooping & 0x0f, inst->speedOrLooping >> 4,
            inst->loopmode, inst->soundlength, inst->offset, output); // loopmode est mal nommé : c'est le timedMode
    }
}
void FAT_player_playNoteWithCustomParams_chan6(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output){
    instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);

    if (inst->type > INSTRUMENT_TYPE_SAMPLEB){
        // waveduty = shape
        snd_playOscillatorB (inst->wavedutyOrPolynomialStep, note->freq, inst->loopmode, inst->soundlength);
    } else {
        snd_playChannelBSample(
            inst->kitNumber, note->freq, inst->volumeRatio >> 4,
            inst->speedOrLooping & 0x0f, inst->speedOrLooping >> 4,
            inst->loopmode, inst->soundlength, inst->offset, output);
    }
}

/**
 * \brief Joue une note sur un channel en ajoutant un transpose.
 *
 * @param note l'objet NOTE à jouer
 * @param channel le numéro de channel sur lequel jouer la note
 * @param transpose la valeur de transpose, elle sera ajoutée à celle du projet
 */
void FAT_player_playNoteWithCustomParams(note* note, u8 channel, u8 transpose, u8 volume, u8 sweep, u8 output) {
    if (note->freq != NULL_VALUE) {
        instrument* inst = &(FAT_tracker.allInstruments[note->instrument]);
        u8 outputValue = output != NULL_VALUE ? output : inst->output;

        // play it on the right channel. Happy pointer :D
        (*FAT_player_playNoteWithCustomParams_chanX[channel]) (note,channel,transpose,volume,sweep,outputValue);
    }

    if (note->effect.name != NULL_VALUE) {
        snd_tryToApplyEffect(channel, noteEffectNum[note->effect.name >> 1], note->effect.value);
    }
}

u8 FAT_player_searchFirstAvailableSequenceForChannel_returnLine (u8 channel, u8 startingLine){
    s8 i = startingLine;
    u8 firstLine = NULL_VALUE;

    // 1. je cherche en partant de la ligne courante.
    while (i < NB_SEQUENCES_IN_ONE_CHANNEL){
        if (FAT_tracker.channels[channel].sequences[i] != NULL_VALUE){
            firstLine = i; // j'ai trouvé un bloc valide !
            break;
        }
        i++;
    }

    // 2. si je n'ai pas trouvé, je cherche en partant de 0
    if (firstLine == NULL_VALUE){
        i = 0;
        while (i < startingLine){
            if (FAT_tracker.channels[channel].sequences[i] != NULL_VALUE){
                firstLine = i;
                break;
            }
            i++;
        }
    }

    // 3. si j'ai trouvé un bloc valide, on s'assure qu'il s'agit bien du premier
    // attention, seulement en mode greedPlay
    if (FAT_tracker.greedPlay){
        if (firstLine != NULL_VALUE){
            i = firstLine;
            while (i >= 0){
                if (FAT_tracker.channels[channel].sequences[i] == NULL_VALUE){
                    break;
                }
                firstLine = i;
                i--;
            }
        }
    }

    return firstLine;
}

/**
 * \brief Lance la lecture de toute la track.
 */
void FAT_player_startPlayerFromSequences(u8 startLine) {

    // initialisation des séquences au démarrage
    memset(actualSequencesForChannel, NULL_VALUE, sizeof (u8)*6);
    memset(actualBlocksForChannel, 0, sizeof (u8)*6);
    memset(actualNotesForChannel, 0, sizeof (u8)*6);
    memset(firstAvailableSequenceForChannel, 0, sizeof (u8)*6);

    FAT_resetTempo ();
    FAT_isCurrentlyPlaying = 1;
    FAT_live_nbChannelPlaying = 0;
    FAT_player_isPlayingFrom = SCREEN_SONG_ID;

    // détermine pour chaque channel, quelle est la premiere sequence jouable.
    u8 i;
    for (i = 0; i<6; i++){
        firstAvailableSequenceForChannel[i] = FAT_player_searchFirstAvailableSequenceForChannel_returnLine(i, startLine);
        actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
        if (actualSequencesForChannel[i]!=NULL_VALUE){
            FAT_live_nbChannelPlaying++;
        }
        FAT_player_moveOrHideCursor(i);
    }

    FAT_player_runTimer ();
}

void FAT_player_startPlayerFromLive_oneChannel(u8 line, u8 channel){
    // initialisation seulement si d'autres colonnes n'ont pas déjà été lancées !
    if (!FAT_isCurrentlyPlaying){
        memset(actualSequencesForChannel, NULL_VALUE, sizeof (u8)*6);
        memset(actualBlocksForChannel, 0, sizeof (u8)*6);
        memset(actualNotesForChannel, 0, sizeof (u8)*6);
        memset(firstAvailableSequenceForChannel, 0, sizeof (u8)*6);

        // les autres channels vont devoir attendre celui la !
        memset (FAT_live_waitForOtherChannel, 1, sizeof(u8)*6);
        FAT_live_waitForOtherChannel[channel] = 0;

        FAT_resetTempo ();

        FAT_isCurrentlyPlaying = 1;
        FAT_live_nbChannelPlaying = 0;
    }

    FAT_player_isPlayingFrom = SCREEN_LIVE_ID;

    // positionnement du player sur le channel
    firstAvailableSequenceForChannel[channel] = FAT_player_searchFirstAvailableSequenceForChannel_returnLine(channel, line);
    actualSequencesForChannel[channel] = firstAvailableSequenceForChannel[channel];
    actualBlocksForChannel[channel] = 0;
    actualNotesForChannel[channel] = 0;
    FAT_live_nbChannelPlaying ++;

    if (FAT_live_waitForOtherChannel[channel] == 0){
        FAT_player_moveOrHideCursor(channel);
        FAT_player_runTimer ();
    }
}

/**
 * \brief Démarrer la lecture d'une séquence (lit tous les blocks de la séquence).
 *
 * @param sequenceId l'id de la séquence à jouer
 * @param startLine le numéro de ligne de départ
 * @param channel le numéro de channel sur lequel on joue
 */
void FAT_player_startPlayerFromBlocks(u8 sequenceId, u8 startLine, u8 channel) {

    // initialisation
    memset(actualSequencesForChannel, NULL_VALUE, sizeof (u8)*6);
    memset(actualBlocksForChannel, 0, sizeof (u8)*6);
    memset(actualNotesForChannel, 0, sizeof (u8)*6);
    actualBlocksForChannel[channel] = startLine;
    FAT_currentPlayedSequence = sequenceId;
    FAT_currentPlayedChannel = channel;

    FAT_resetTempo ();
    FAT_isCurrentlyPlaying = 1;
    FAT_player_isPlayingFrom = SCREEN_BLOCKS_ID;

    FAT_player_runTimer ();

}

/**
 * \brief Démarre la lecture d'un block entier.
 *
 * @param blockId l'id du block à jouer
 * @param startLine la ligne de départ, usuellement 0 (zéro)
 * @param channel le numéro de channel sur lequel on joue
 */
void FAT_player_startPlayerFromNotes(u8 blockId, u8 startLine, u8 channel) {

    memset(actualSequencesForChannel, NULL_VALUE, sizeof (u8)*6);
    memset(actualBlocksForChannel, NULL_VALUE, sizeof (u8)*6);
    memset(actualNotesForChannel, 0, sizeof (u8)*6);
    actualNotesForChannel[channel] = startLine;
    FAT_currentPlayedBlock = blockId;
    FAT_currentPlayedChannel = channel;

    FAT_resetTempo ();
    FAT_isCurrentlyPlaying = 1;
    FAT_player_isPlayingFrom = SCREEN_NOTES_ID;

    FAT_player_runTimer ();
}

int FAT_player_debug_getTempoReach(){
  return tempoReach;
}

void ATTR_FASTFUNC FAT_player_timerFunc() {
    if (tempoReach >= 0) {
        tempoReach--;
    }
    tickCounter--;
    //hel_IntrAcknowledge(INT_TYPE_TIM3);
}

void FAT_player_progressInSong(u8 channel, sequence* seq){
    actualNotesForChannel[channel]++;
    if (actualNotesForChannel[channel] >= NB_NOTES_IN_ONE_BLOCK) {
        actualNotesForChannel[channel] = 0;
        actualBlocksForChannel[channel]++;
        if (actualBlocksForChannel[channel] >= NB_BLOCKS_IN_SEQUENCE
                || seq->blocks[actualBlocksForChannel[channel]] == NULL_VALUE) {
            actualBlocksForChannel[channel] = 0;

            actualSequencesForChannel[channel]++;
            if (actualSequencesForChannel[channel] > NB_MAX_SEQUENCES
                    || FAT_tracker.channels[channel].sequences[actualSequencesForChannel[channel]] == NULL_VALUE
                    || FAT_data_isSequenceEmpty(FAT_tracker.channels[channel].sequences[actualSequencesForChannel[channel]])) {

                actualSequencesForChannel[channel] = firstAvailableSequenceForChannel[channel];
            }
        }
    }
}

void FAT_player_progressInSequence (sequence* seq){
    actualNotesForChannel[FAT_currentPlayedChannel]++;
    if (actualNotesForChannel[FAT_currentPlayedChannel] >= NB_NOTES_IN_ONE_BLOCK) {
        actualNotesForChannel[FAT_currentPlayedChannel] = 0;

        actualBlocksForChannel[FAT_currentPlayedChannel]++;
        if (actualBlocksForChannel[FAT_currentPlayedChannel] >= NB_BLOCKS_IN_SEQUENCE
                || seq->blocks[actualBlocksForChannel[FAT_currentPlayedChannel]] == NULL_VALUE) {
            actualBlocksForChannel[FAT_currentPlayedChannel] = 0;
        }
    }
}

void FAT_player_progressInBlock (){
    actualNotesForChannel[FAT_currentPlayedChannel]++;
    if (actualNotesForChannel[FAT_currentPlayedChannel] >= NB_NOTES_IN_ONE_BLOCK) {
        actualNotesForChannel[FAT_currentPlayedChannel] = 0;
    }
}

// DEJA DOCUMENTEE
void FAT_player_playFromSequences() {
    #ifdef DEBUG_ON
    hel_BgTextPrintF(TEXT_LAYER, 22, 15, 0, "%.d", tempoReach);
    #endif
    if (tempoReach < 0) {
        tempoReach = 0;
        #ifdef DEBUG_ON
        hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "TICK");
        #endif
        u8 i;
        for (i = 0; i < 6; i++) {
            FAT_player_buffer[i].haveToPlay = 0;
            FAT_player_buffer[i].volume = NULL_VALUE;
            FAT_player_buffer[i].sweep = NULL_VALUE;
            FAT_player_buffer[i].output = NULL_VALUE;
            if (FAT_isChannelCurrentlyPlaying(i) && actualSequencesForChannel[i] < NB_MAX_SEQUENCES){
                FAT_currentPlayedSequence = FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]];
                if (FAT_currentPlayedSequence != NULL_VALUE) {
                    // lire la séquence actuelle
                    sequence* seq = &(FAT_tracker.allSequences[FAT_currentPlayedSequence]);

                    FAT_currentPlayedBlock = seq->blocks[actualBlocksForChannel[i]];
                    if (FAT_currentPlayedBlock != NULL_VALUE) {
                        block* block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);

                        effect* effect = FAT_data_note_getEffect(FAT_currentPlayedBlock, actualNotesForChannel[i]);
                        if (effect){
                            switch (((effect->name & 0xfe) >> 1)){
                                case EFFECT_VOLUME:
                                    FAT_player_buffer[i].volume = effect->value;
                                    break;
                                case EFFECT_SWEEP:
                                    FAT_player_buffer[i].sweep = effect->value;
                                    break;
                                case EFFECT_OUTPUT:
                                    FAT_player_buffer[i].output = effect->value;
                                    break;
                                case EFFECT_HOP:
                                    // determine new block
                                    actualNotesForChannel[i] = effect->value;
                                    actualBlocksForChannel[i]++;
                                    if (actualBlocksForChannel[i] >= NB_BLOCKS_IN_SEQUENCE
                                            || seq->blocks[actualBlocksForChannel[i]] == NULL_VALUE) {
                                        actualBlocksForChannel[i] = 0;

                                        actualSequencesForChannel[i]++;
                                        if (actualSequencesForChannel[i] > NB_MAX_SEQUENCES
                                                || FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]] == NULL_VALUE
                                                || FAT_data_isSequenceEmpty(FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]])) {

                                            actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                                        }
                                    }

                                    FAT_currentPlayedBlock = seq->blocks[actualBlocksForChannel[i]];
                                    block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);
                                    break;
                            }
                        }

                        FAT_player_buffer[i].note = &(block->notes[actualNotesForChannel[i]]);
                        FAT_player_buffer[i].transpose =  seq->transpose[actualBlocksForChannel[i]];
                        FAT_player_buffer[i].haveToPlay = 1;

                        FAT_player_progressInSong (i, seq);

                        // Déplacement des curseurs de lecture
                        FAT_player_moveOrHideCursor(i);

                    } else {
                        actualBlocksForChannel[i] = 0;
                        actualSequencesForChannel[i]++;
                        if (actualSequencesForChannel[i] > NB_MAX_SEQUENCES) {
                            actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                        }
                        FAT_player_moveOrHideCursor(i);
                    }

                } else {
                    actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                    FAT_player_moveOrHideCursor(i);
                }
            }

        }

        for (i = 0; i < 6; i++) {
            if (FAT_player_buffer[i].haveToPlay){
                FAT_player_playNoteWithCustomParams(
                   FAT_player_buffer[i].note , i,
                   FAT_player_buffer[i].transpose,
                   FAT_player_buffer[i].volume,
                   FAT_player_buffer[i].sweep,
                   FAT_player_buffer[i].output);
            }
        }
        FAT_resetTempo ();
    }else {hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "    ");}
}

void FAT_player_liveSynchro(){
    u8 j;
    for (j=0;j<6;j++){
        if(FAT_isChannelCurrentlyPlaying(j)){
            FAT_live_waitForOtherChannel[j] = 0;
            FAT_player_moveOrHideCursor(j);
        }
    }
}

void FAT_player_playFromLive(){
    #ifdef DEBUG_ON
    hel_BgTextPrintF(TEXT_LAYER, 22, 15, 0, "%.d", tempoReach);
    #endif
    if (tempoReach < 0) {
        tempoReach = 0;
        #ifdef DEBUG_ON
        hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "TICK");
        #endif
        bool willHaveToSyncAfterNote = 0;
        u8 i;
        for (i = 0; i < 6; i++) {
            FAT_player_buffer[i].haveToPlay = 0;
            FAT_player_buffer[i].volume = NULL_VALUE;
            FAT_player_buffer[i].sweep = NULL_VALUE;
            FAT_player_buffer[i].output = NULL_VALUE;
            if (FAT_isChannelCurrentlyPlaying(i) && actualSequencesForChannel[i] < NB_MAX_SEQUENCES && FAT_live_waitForOtherChannel[i] == 0){

                FAT_currentPlayedSequence = FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]];

                if (FAT_currentPlayedSequence != NULL_VALUE) {
                    // lire la séquence actuelle
                    sequence* seq = &(FAT_tracker.allSequences[FAT_currentPlayedSequence]);

                    FAT_currentPlayedBlock = seq->blocks[actualBlocksForChannel[i]];
                    if (FAT_currentPlayedBlock != NULL_VALUE) {
                        block* block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);

                        effect* effect = FAT_data_note_getEffect(FAT_currentPlayedBlock, actualNotesForChannel[i]);
                        if (effect){
                            switch (((effect->name & 0xfe) >> 1)){
                                case EFFECT_VOLUME:
                                    FAT_player_buffer[i].volume = effect->value;
                                    break;
                                case EFFECT_SWEEP:
                                    FAT_player_buffer[i].sweep = effect->value;
                                    break;
                                case EFFECT_OUTPUT:
                                    FAT_player_buffer[i].output = effect->value;
                                    break;
                                case EFFECT_HOP:
                                    // determine new block
                                    actualNotesForChannel[i] = effect->value;
                                    actualBlocksForChannel[i]++;
                                    if (actualBlocksForChannel[i] >= NB_BLOCKS_IN_SEQUENCE
                                            || seq->blocks[actualBlocksForChannel[i]] == NULL_VALUE) {
                                        actualBlocksForChannel[i] = 0;

                                        if (!FAT_tracker.liveData.liveMode){ // mode auto?
                                            actualSequencesForChannel[i]++;
                                            if (actualSequencesForChannel[i] > NB_MAX_SEQUENCES
                                                    || FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]] == NULL_VALUE
                                                    || FAT_data_isSequenceEmpty(FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]])) {

                                                actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                                                // si pas de séquences dispo -> NULL_VALUE
                                            }
                                        }
                                        willHaveToSyncAfterNote = 1;
                                        // Déplacement des curseurs de lecture
                                        FAT_player_moveOrHideCursor(i);
                                    }

                                    FAT_currentPlayedBlock = seq->blocks[actualBlocksForChannel[i]];
                                    block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);
                                    break;
                            }
                        }

                        FAT_player_buffer[i].note = &(block->notes[actualNotesForChannel[i]]);
                        FAT_player_buffer[i].transpose =  seq->transpose[actualBlocksForChannel[i]];
                        FAT_player_buffer[i].haveToPlay = 1;

                        actualNotesForChannel[i]++;
                        if (actualNotesForChannel[i] >= NB_NOTES_IN_ONE_BLOCK) {
                            actualNotesForChannel[i] = 0;
                            actualBlocksForChannel[i]++;
                            if (actualBlocksForChannel[i] >= NB_BLOCKS_IN_SEQUENCE
                                    || seq->blocks[actualBlocksForChannel[i]] == NULL_VALUE) {
                                actualBlocksForChannel[i] = 0;

                                if (!FAT_tracker.liveData.liveMode){ // mode auto?
                                    actualSequencesForChannel[i]++;
                                    if (actualSequencesForChannel[i] > NB_MAX_SEQUENCES
                                            || FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]] == NULL_VALUE
                                            || FAT_data_isSequenceEmpty(FAT_tracker.channels[i].sequences[actualSequencesForChannel[i]])) {

                                        actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                                        // si pas de séquences dispo -> NULL_VALUE
                                    }
                                }
                                willHaveToSyncAfterNote = 1;
                                // Déplacement des curseurs de lecture
                                FAT_player_moveOrHideCursor(i);
                            }
                        }
                    } else {
                        actualBlocksForChannel[i] = 0;

                        if (!FAT_tracker.liveData.liveMode){ // mode auto?
                            actualSequencesForChannel[i]++;
                            if (actualSequencesForChannel[i] > NB_MAX_SEQUENCES) {
                                actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                            }
                            FAT_player_moveOrHideCursor(i);
                        }

                        willHaveToSyncAfterNote = 1;
                    }

                } else if (!FAT_tracker.liveData.liveMode){ // mode auto?
                    actualSequencesForChannel[i] = firstAvailableSequenceForChannel[i];
                    willHaveToSyncAfterNote = 1;
                    FAT_player_moveOrHideCursor(i);
                }
            }

            FAT_player_live_showOrHideCursorWait (i);

        }

        u8 volume = NULL_VALUE;
        for (i = 0; i < 6; i++) {
            if (FAT_player_buffer[i].haveToPlay){

                // calcul du volume
                if (FAT_tracker.liveData.volume[i] == NULL_VALUE){
                    volume = FAT_player_buffer[i].volume;
                } else if (FAT_player_buffer[i].volume == NULL_VALUE){
                    volume = FAT_tracker.liveData.volume[i];
                } else {
                    volume = (FAT_player_buffer[i].volume + FAT_tracker.liveData.volume[i])/2;
                }

                FAT_player_playNoteWithCustomParams(
                   FAT_player_buffer[i].note , i,
                   FAT_player_buffer[i].transpose + FAT_tracker.liveData.transpose[i],
                   volume, FAT_player_buffer[i].sweep, FAT_player_buffer[i].output);
            }
        }

        if (willHaveToSyncAfterNote){
            FAT_player_liveSynchro();
        }

        FAT_resetTempo ();
    }else {hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "    ");}
}

// DEJA DOCUMENTEE
void FAT_player_playFromBlocks() {
    #ifdef DEBUG_ON
    hel_BgTextPrintF(TEXT_LAYER, 22, 15, 0, "%.d", tempoReach);
    #endif
    u8 volume=NULL_VALUE;
    u8 sweep = NULL_VALUE;
    u8 output = NULL_VALUE;
    if (tempoReach < 0) {
        tempoReach = 0;
        #ifdef DEBUG_ON
        hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "TICK");
        #endif
        if (FAT_currentPlayedSequence != NULL_VALUE) {
            // lire la séquence actuelle
            sequence* seq = &(FAT_tracker.allSequences[FAT_currentPlayedSequence]);

            FAT_currentPlayedBlock = seq->blocks[actualBlocksForChannel[FAT_currentPlayedChannel]];
            if (FAT_currentPlayedBlock != NULL_VALUE) {
                block* block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);

                // Déplacement des curseurs de lecture
                FAT_player_moveOrHideCursor(FAT_currentPlayedChannel);

                effect* effect = FAT_data_note_getEffect(FAT_currentPlayedBlock, actualNotesForChannel[FAT_currentPlayedChannel]);
                volume = NULL_VALUE; sweep = NULL_VALUE; output = NULL_VALUE;
                if (effect){
                    switch (((effect->name & 0xfe) >> 1)){
                        case EFFECT_VOLUME:
                            volume = effect->value;
                            break;
                        case EFFECT_SWEEP:
                            sweep = effect->value;
                            break;
                        case EFFECT_OUTPUT:
                            output = effect->value;
                            break;
                        case EFFECT_HOP:
                            // determine new block
                            actualNotesForChannel[FAT_currentPlayedChannel] = effect->value;
                            actualBlocksForChannel[FAT_currentPlayedChannel]++;
                            if (actualBlocksForChannel[FAT_currentPlayedChannel] >= NB_BLOCKS_IN_SEQUENCE
                                    || seq->blocks[actualBlocksForChannel[FAT_currentPlayedChannel]] == NULL_VALUE) {
                                actualBlocksForChannel[FAT_currentPlayedChannel] = 0;
                            }
                            FAT_currentPlayedBlock = seq->blocks[actualBlocksForChannel[FAT_currentPlayedChannel]];
                            block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);
                            break;
                    }
                }

                FAT_player_playNoteWithCustomParams(&(block->notes[actualNotesForChannel[FAT_currentPlayedChannel]]), FAT_currentPlayedChannel,
                        seq->transpose[actualBlocksForChannel[FAT_currentPlayedChannel]], volume, sweep, output);

                FAT_player_progressInSequence (seq);
            } else {
                actualBlocksForChannel[FAT_currentPlayedChannel] = 0;
            }

        }

        FAT_resetTempo ();
    }else {hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "    ");}

}



// DEJA DOCUMENTEE
void FAT_player_playFromNotes() {
    #ifdef DEBUG_ON
    hel_BgTextPrintF(TEXT_LAYER, 22, 15, 0, "%.d", tempoReach);
    #endif
    u8 volume=NULL_VALUE;
    u8 sweep = NULL_VALUE;
    u8 output = NULL_VALUE;
    if (tempoReach < 0) {
        tempoReach = 0;
        #ifdef DEBUG_ON
        hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "TICK");
        #endif
        if (FAT_currentPlayedBlock != NULL_VALUE) {
            block* block = &(FAT_tracker.allBlocks[FAT_currentPlayedBlock]);

            // Déplacement des curseurs de lecture
            FAT_player_moveOrHideCursor(FAT_currentPlayedChannel);

            effect* effect = FAT_data_note_getEffect(FAT_currentPlayedBlock, actualNotesForChannel[FAT_currentPlayedChannel]);
            volume = NULL_VALUE; sweep = NULL_VALUE; output = NULL_VALUE;
            if (effect){
                switch (((effect->name & 0xfe) >> 1)){
                    case EFFECT_VOLUME:
                        volume = effect->value;
                        break;
                    case EFFECT_SWEEP:
                        sweep = effect->value;
                        break;
                    case EFFECT_OUTPUT:
                        output = effect->value;
                        break;
                    case EFFECT_HOP:
                        // get back to 0
                        actualNotesForChannel[FAT_currentPlayedChannel] = effect->value;
                        break;
                }
            }

            FAT_player_playNoteWithCustomParams(&(block->notes[actualNotesForChannel[FAT_currentPlayedChannel]]),
                    FAT_currentPlayedChannel, 0, volume, sweep, output);

            FAT_player_progressInBlock ();
        }

        FAT_resetTempo ();

    }else {hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "    ");}

}

void FAT_player_continueToPlay() {
    switch (FAT_player_isPlayingFrom) {
        case SCREEN_SONG_ID:
            FAT_player_playFromSequences();
            break;
        case SCREEN_LIVE_ID:
            FAT_player_playFromLive();
            break;
        case SCREEN_BLOCKS_ID:
            FAT_player_playFromBlocks();
            break;
        case SCREEN_NOTES_ID:
            FAT_player_playFromNotes();
            break;
    }
}

/**
 * \brief Arrete la lecture d'un channel. Si c'est le dernier, stop le player completement.
 */
void FAT_player_stopPlayer_onlyOneColumn(u8 channel){
    actualSequencesForChannel[channel] = NULL_VALUE;
    actualBlocksForChannel[channel] = 0;
    actualNotesForChannel[channel] = 0;
    firstAvailableSequenceForChannel[channel] = 0;
    FAT_live_waitForOtherChannel[channel] = 1;

    FAT_live_nbChannelPlaying --;

    // TODO et si un channel attend ?
    if (FAT_live_nbChannelPlaying == 0){
        FAT_player_stopPlayer();
    } else {
        memset(&FAT_player_buffer[channel], 0, sizeof(bufferPlayer));
        FAT_player_buffer[channel].volume = NULL_VALUE;
        snd_effect_kill (channel, 0x00);
        FAT_player_hidePlayerSequenceCursor (channel);
    }
}

bool FAT_isChannelCurrentlyPlaying (u8 channel){
    return actualSequencesForChannel[channel] != NULL_VALUE;
}

/**
 * \brief Arrète la lecture de la track.
 */
void FAT_player_stopPlayer() {

    //R_TIM3CNT = 0x0003;
    //hel_IntrStopHandler(INT_TYPE_TIM3);
    FAT_player_stopTimer();

    // stop le son
    snd_stopAllSounds();

    // cache tous les curseurs de lecture
    FAT_player_hideAllCursors();

    // la lecture est terminée.
    FAT_isCurrentlyPlaying = 0;
    FAT_player_isPlayingFrom = 0;
    FAT_live_nbChannelPlaying = 0;

    // réinit propre.
    memset(actualSequencesForChannel, NULL_VALUE, sizeof (u8)*6);
    memset (FAT_live_waitForOtherChannel, 0, sizeof(u8)*6);
    memset(FAT_player_buffer, 0, sizeof(bufferPlayer)*6);

    hel_BgTextPrintF(TEXT_LAYER, 26, 16, 0, "    ");

    u8 i;
    for (i=0;i<6;i++){
        FAT_player_buffer[i].volume = NULL_VALUE;
    }
}

void FAT_resetTempo (){
    tempoReach = (60000 / FAT_tracker.tempo) / 4;
}