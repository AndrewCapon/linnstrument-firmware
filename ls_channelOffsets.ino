/*************************************** ChannelOffset *****************************************/
// offset Pitch bend and note data for each channel in each split
//
// pitchOffset          : The pitch offset for this channel, this is in 48 note resolution 
//                        but constrained to the pitchbend range of the split
// historicPitchOffset  : Stores the historic/last pitch offset. For example if you slide 
//                        one note and hold it, then play another and hold and release the first
//                        the pitch offset of the released note will be stored here.
// noteOffset           : The midi note offset in semitones calculated from pitchOffset+historicPitchOffset
// row & col            : the row and column of the cell in control of setting pitchOffset
// initState            : True if we are in initialised state, so no currently held notes for split/channel

struct __attribute__ ((packed)) ChannelOffset {
  void initialize() {
    pitchOffset = 0;
    historicPitchOffset = 0;
    noteOffset = 0;
    row = 0;
    col = 0;
    initState = true;
    noteNum = -1;
  }

  // return true if we are for column/row
  inline bool isForColRow(byte useCol, byte useRow) const {
    return useCol==col && useRow==row;
  }

  // return true if the historicPitchOffset should be updated
  inline bool shouldApplyHistoricPitchOffset(byte useCol, byte useRow) const {
    return (initState == false) && (isForColRow(useCol, useRow) == false);
  }

  // Return the total pitch offset
  inline int totalPitchOffset() const {
    return pitchOffset + historicPitchOffset;
  }

  // return the pitch offset to use for a column/row
  inline int pitchOffsetForColRow(byte useCol, byte useRow) const{
    int pitchOffsetResult;
    if(isForColRow(useCol, useRow)){
      // this is for our column and row, so add historic pitch offset
      pitchOffsetResult = historicPitchOffset;
    }
    else{
      // this is not for our column and row so adjust return total plitchbend
      pitchOffsetResult = totalPitchOffset();
    }

    return pitchOffsetResult;
  }




  int     pitchOffset;
  int     historicPitchOffset;
  int8_t  noteOffset;
  byte    col:5;
  byte    row:3;
  bool    initState;
  int8_t  noteNum;
};


/*************************************** ChannelOffsets *****************************************/
// Contains a ChannelOffset for each channel in each split.
struct ChannelOffsets {
  void initialize()
  {
    for(byte channel = 0; channel < 16; channel++)
    {
      for(byte split = 0; split < NUMSPLITS; split++)
        offset[channel][split].initialize();
    }
  }

  struct ChannelOffset  offset[16][NUMSPLITS];
};


// our global ChannelOffsets
struct ChannelOffsets channelOffsets;



// Initialize Channel Offsets
void initializeChannelOffsets() {
  channelOffsets.initialize();
}


// Update the column and row in control of the offset for a channel/split
inline void updateChannelOffsetColRow(byte split, byte channel, byte col, byte row){
  channelOffsets.offset[channel][split].col = col;
  channelOffsets.offset[channel][split].row = row;
}

// Store Pitch bend offset for a channel
// also calculate note offset
inline void storeChannelOffset(byte split, int &pitch, byte col, byte row, byte channel, int8_t noteNum) {
  static int32_t fpPBPerNote = FXD_DIV(8192, 48); // Only calculate once

  ChannelOffset &channelOffset = channelOffsets.offset[channel][split];

  // in monoAlterPitch mode only update if no existing note, or the note is the same as the stored note
  if((Split[sensorSplit].monoMode != monoAlterPitch) || ((Split[sensorSplit].monoMode == monoAlterPitch) && ((noteNum == channelOffset.noteNum) || (channelOffset.noteNum == -1)) )) {
    if(channelOffset.shouldApplyHistoricPitchOffset(sensorCol, sensorRow)){
      channelOffset.historicPitchOffset = channelOffset.totalPitchOffset();
    }

    byte bendRange = getBendRange(split);


    int8_t  noteOffset = FXD_TO_INT(FXD_DIV(FXD_FROM_INT(channelOffset.totalPitchOffset()), fpPBPerNote));
    noteOffset = constrain(noteOffset, 0-bendRange, bendRange);

    channelOffset.pitchOffset = constrainPitch(split, pitch);
    channelOffset.noteOffset = noteOffset;
    channelOffset.col = col;
    channelOffset.row = row;
    channelOffset.initState = false;
    channelOffset.noteNum = noteNum;
    //DEBUGPRINTF(0, "updateChannelOffsetColRow pitch = %d, constrained pitch = %d, notenum = %d\n", pitch, channelOffset.pitchOffset, noteNum);
  }
}

// Handle release cell for split/channel, if no notes held initialize offset
inline void handleChannelOffsetRelease(byte split, byte channel){
  if(countTouchesForMidiChannel(split, channel) == 0){
    channelOffsets.offset[channel][split].initialize();
  }
}

inline bool channelOffsetShouldSendNoteOn(byte split, byte channel, int8_t noteNum) {
  DEBUGPRINTF(0,"count = %d\n", countTouchesForMidiChannel(split, channel));
  return ((Split[sensorSplit].monoMode != monoAlterPitch) || ((Split[sensorSplit].monoMode == monoAlterPitch) && countTouchesForMidiChannel(split, channel) == 1 ));

  // ChannelOffset &channelOffset = channelOffsets.offset[channel][split];

  // return channelOffset.shouldSendNoteOn(Split[sensorSplit].monoMode, noteNum);
}


// Return the ChannelOffset for a split/channel
inline const struct ChannelOffset &getChannelOffset(byte split, byte channel) {
  return channelOffsets.offset[channel][split];
}
