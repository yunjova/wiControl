
//----------------------------------------------------------------------------
// cgpiod_output.hpp : implementation of 2 outputs
//
// Copyright (c) Jo Simons, 2016-2016, All Rights Reserved.
//----------------------------------------------------------------------------
#include <gpiod.h>

  //--------------------------------------------------------------------------
  // configure
  //--------------------------------------------------------------------------
  tUint32 CGpiod::_outputOnConfig() 
  {
    tUint32      dwObj;
    tGpiodOutput *pObj = m_output; 

    // initialise all channels
    Debug.logTxt(CLSLVL_GPIOD_OUTPUT | 0x0000, "CGpiod::_outputOnConfig");
    memset(m_output, 0, sizeof(m_output)); 

    for (dwObj = 0; dwObj < CGPIOD_OUT_COUNT; dwObj++, pObj++) {
      // initialise defaults
      pObj->dwFlags  = CGPIOD_OUT_FLG_MQTT_ALL;
      pObj->dwPin    = (dwObj == CGPIOD_OUT0) ? CGPIOD_OUT0_PIN : 
                       (dwObj == CGPIOD_OUT1) ? CGPIOD_OUT1_PIN : 
                       (dwObj == CGPIOD_OUT2) ? CGPIOD_OUT2_PIN : 
                       (dwObj == CGPIOD_OUT3) ? CGPIOD_OUT3_PIN : -1; 
      pObj->dwPol    = CGPIOD_IO_POL_NORMAL; 
      pObj->dwRunDef = CGPIOD_OUT_RUN_DEF;
//    pObj->dwState  = CGPIOD_OUT_STATE_OFF; 
//    pObj->dwCmd    = CGPIOD_OUT_CMD_NONE; 
      } // for

    return m_dwError;
    } // _outputOnConfig

  //--------------------------------------------------------------------------
  // initialise output hw
  //--------------------------------------------------------------------------
  tUint32 CGpiod::_outputOnInit() 
  {
    tUint32      dwObj;
    tGpiodOutput *pObj = m_output; 

    Debug.logTxt(CLSLVL_GPIOD_OUTPUT | 0x0000, "CGpiod::_outputOnInit");
    for (dwObj = 0; dwObj < CGPIOD_OUT_COUNT; dwObj++, pObj++) {
      pObj->dwState  = CGPIOD_OUT_STATE_OFF; 
      pObj->dwCmd    = CGPIOD_OUT_CMD_NONE; 
      _ioSetPinVal(pObj->dwPin, pObj->dwState ^ pObj->dwPol);
      _ioSetPinDir(pObj->dwPin, CGPIOD_IO_DIR_OUTPUT);
      } // for

    return m_dwError;
    } // _outputOnInit

  //--------------------------------------------------------------------------
  // run outputs
  //--------------------------------------------------------------------------
  tUint32 CGpiod::_outputOnRun(tUint32 msNow) 
  {
    tUint32      dwObj;
    tGpiodOutput *pObj = m_output; 
    tGpiodEvt    evt = { msNow, CGPIOD_ORIG_OUTPUT, 0, 0, 0, 0 };

    // handle regular output objects
    for (dwObj = 0; dwObj < CGPIOD_OUT_COUNT; dwObj++, pObj++) {
      evt.dwObj = CGPIOD_OBJ_CLS_OUTPUT + dwObj;

      switch (pObj->dwCmd) {
        case CGPIOD_OUT_CMD_ONDELAYED:
        case CGPIOD_OUT_CMD_OFFTIMED:
          if (ChkTimer(msNow, pObj->dwRun)) {
            _outputSetState(pObj, CGPIOD_OUT_STATE_ON, &evt);
            pObj->dwCmd = CGPIOD_OUT_CMD_NONE;
            pObj->dwRun = 0;
            } // if
          break;

        case CGPIOD_OUT_CMD_OFFDELAYED:
        case CGPIOD_OUT_CMD_ONTIMED:
          if (ChkTimer(msNow, pObj->dwRun)) {
            _outputSetState(pObj, CGPIOD_OUT_STATE_OFF, &evt);
            pObj->dwCmd = CGPIOD_OUT_CMD_NONE;
            pObj->dwRun = 0;
            } // if
          break;

        case CGPIOD_OUT_CMD_TOGGLEDELAYED:
        case CGPIOD_OUT_CMD_TOGGLETIMED:
          if (ChkTimer(msNow, pObj->dwRun)) {
            _outputSetState(pObj, pObj->dwState ^ CGPIOD_OUT_STATE_ON, &evt);
            pObj->dwCmd    = CGPIOD_OUT_CMD_NONE;
            pObj->dwRun    = 0;
            pObj->dwFlags &= ~CGPIOD_OUT_FLG_LOCKED;
            } // if
          break;

        case CGPIOD_OUT_CMD_LOCKTIMED:
          if (ChkTimer(msNow, pObj->dwRun)) {
            pObj->dwCmd    = CGPIOD_OUT_CMD_NONE;
            pObj->dwRun    = 0;
            pObj->dwFlags &= ~CGPIOD_OUT_FLG_LOCKED;
            } // if
          break;

        case CGPIOD_OUT_CMD_BLINK:
          // handled by _outputDoEvt() for multi-channel in phase operation
          break;

        case CGPIOD_OUT_CMD_BLINKTIMED:
          if (ChkTimer(msNow, pObj->dwRun)) {
            _outputSetState(pObj, CGPIOD_OUT_STATE_OFF, &evt);
            pObj->dwCmd = CGPIOD_OUT_CMD_NONE;
            pObj->dwRun = 0;
            } // if
          break;

        default:
          break;
        } // switch
      } // for

    return 0; // m_dwState;
    } // CGpiod::_outputOnRun

  //--------------------------------------------------------------------------
  // leave outputs in decent state
  //--------------------------------------------------------------------------
  tUint32 CGpiod::_outputOnExit() 
  {
    tUint32      dwObj;
    tGpiodOutput *pObj = m_output; 

    // set outputs to off and switch to input
    Debug.logTxt(CLSLVL_GPIOD_OUTPUT | 0x0000, "CGpiod::_outputOnExit");
    for (dwObj = 0; dwObj < CGPIOD_OUT_COUNT; dwObj++, pObj++) {
      _ioSetPinVal(pObj->dwPin, (pObj->dwState = CGPIOD_OUT_STATE_OFF) ^ pObj->dwPol);
      _ioSetPinDir(pObj->dwPin, CGPIOD_IO_DIR_INPUT);
      } // for

    return m_dwError;
    } // CGpiod::_outputOnExit

  //--------------------------------------------------------------------------
  // local emulation handler, called by DoEvt() or DoCmd()
  //--------------------------------------------------------------------------
  tUint32 CGpiod::_outputDoEvt(tGpiodEvt* pEvt) 
  { 
    tUint32      dwObj;
    tGpiodCmd    cmd = { 0 };
    tGpiodEvt    evt = { 0 };
    tGpiodOutput *pObj = m_output; 

    do {
      // initialise vars
      cmd.msNow  = pEvt->msNow;
      evt.msNow  = pEvt->msNow;
      evt.dwOrig = pEvt->dwOrig;

      // handle input event
      if ((pEvt->dwObj & CGPIOD_OBJ_CLS_MASK) == CGPIOD_OBJ_CLS_INPUT) {

        switch (pEvt->dwEvt) {
          case CGPIOD_IN_EVT_OUTLT1:
            cmd.dwObj = CGPIOD_OBJ_CLS_OUTPUT | (pEvt->dwObj & CGPIOD_OBJ_NUM_MASK);
            cmd.dwCmd = CGPIOD_OUT_CMD_TOGGLE;
            _outputDoCmd(&cmd);
            break;

          case CGPIOD_IN_EVT_INGT2:
            cmd.dwObj             = CGPIOD_OBJ_CLS_OUTPUT | (pEvt->dwObj & CGPIOD_OBJ_NUM_MASK);
            cmd.parmsOutput.dwRun = m_output[pEvt->dwObj & CGPIOD_OBJ_NUM_MASK].dwRunDef;
            cmd.dwCmd             = (cmd.parmsOutput.dwRun) ? CGPIOD_OUT_CMD_BLINKTIMED : CGPIOD_OUT_CMD_BLINK;
            _outputDoCmd(&cmd);
            break;
          } // switch

        break;
        } // if 

      if ((pEvt->dwObj & CGPIOD_OBJ_CLS_MASK) == CGPIOD_OBJ_CLS_HBEAT) {
        // handle 1 second heartbeat for synchronous blinking outputs
        for (dwObj = 0; dwObj < CGPIOD_OUT_COUNT; dwObj++, pObj++) {
          evt.dwObj = CGPIOD_OBJ_CLS_OUTPUT | dwObj;

          if ((pObj->dwCmd == CGPIOD_OUT_CMD_BLINK) || (pObj->dwCmd == CGPIOD_OUT_CMD_BLINKTIMED))
            _outputSetState(pObj, (pEvt->dwEvt == CGPIOD_HB_EVT_ODD) ? CGPIOD_OUT_STATE_ON : CGPIOD_OUT_STATE_OFF, &evt);
          } // for

        } // if

      } while (0);

    return m_dwError;
    } // CGpiod::_outputDoEvt

  //--------------------------------------------------------------------------
  // execute command for outputs, called by _DoCmd()
  //--------------------------------------------------------------------------
  tUint32 CGpiod::_outputDoCmd(tGpiodCmd* pCmd) 
  { 
    tChar        str1[16], str2[16];
    tGpiodOutput *pObj = &m_output[pCmd->dwObj & CGPIOD_OBJ_NUM_MASK]; 
    tGpiodEvt    evt   = { pCmd->msNow, pCmd->dwOrig, pCmd->dwObj, 0, 0, 0 };

    PrintCmd(pCmd, CLSLVL_GPIOD_OUTPUT | 0x0000, "CGpiod::_outputDoCmd");
    switch (pCmd->dwCmd & CGPIOD_CMD_NUM_MASK) {
      case CGPIOD_OUT_CMD_STATUS: 
        // report current value
        evt.dwEvt = pObj->dwState;
        DoSta(&evt);
        break;

      case CGPIOD_OUT_CMD_ON: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_NONE;
        _outputSetState(pObj, CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_OFF: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_NONE;
        _outputSetState(pObj, CGPIOD_OUT_STATE_OFF, &evt);
        break;

      case CGPIOD_OUT_CMD_ONLOCKED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd    = CGPIOD_OUT_CMD_NONE;
        pObj->dwFlags |= CGPIOD_OUT_FLG_LOCKED;
        _outputSetState(pObj, CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_OFFLOCKED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd    = CGPIOD_OUT_CMD_NONE;
        pObj->dwFlags |= CGPIOD_OUT_FLG_LOCKED;
        _outputSetState(pObj, CGPIOD_OUT_STATE_OFF, &evt);
        break;

      case CGPIOD_OUT_CMD_TOGGLE: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd  = CGPIOD_OUT_CMD_NONE;
        _outputSetState(pObj, pObj->dwState ^ CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_UNLOCK: 
        pObj->dwFlags &= ~CGPIOD_OUT_FLG_LOCKED;
        break;

      case CGPIOD_OUT_CMD_ONDELAYED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_ONDELAYED;
        pObj->dwRun = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwDelay);
        break;

      case CGPIOD_OUT_CMD_OFFDELAYED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_OFFDELAYED;
        pObj->dwRun = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwDelay);
        break;

      case CGPIOD_OUT_CMD_ONTIMED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_ONTIMED;
        pObj->dwRun = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwRun);
        _outputSetState(pObj, CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_OFFTIMED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_OFFTIMED;
        pObj->dwRun = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwRun);
        _outputSetState(pObj, CGPIOD_OUT_STATE_OFF, &evt);
        break;

      case CGPIOD_OUT_CMD_TOGGLEDELAYED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd    = CGPIOD_OUT_CMD_TOGGLEDELAYED;
        pObj->dwRun    = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwDelay);
        pObj->dwFlags |= CGPIOD_OUT_FLG_LOCKED;
        break;

      case CGPIOD_OUT_CMD_TOGGLETIMED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd    = CGPIOD_OUT_CMD_TOGGLETIMED;
        pObj->dwRun    = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwRun);
        pObj->dwFlags |= CGPIOD_OUT_FLG_LOCKED;
        _outputSetState(pObj, CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_LOCK: 
        pObj->dwFlags |= CGPIOD_OUT_FLG_LOCKED;
        break;

      case CGPIOD_OUT_CMD_LOCKTIMED:
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        if (pObj->dwRun) {
          pObj->dwFlags |= CGPIOD_OUT_FLG_LOCKED;
          pObj->dwCmd    = CGPIOD_OUT_CMD_LOCKTIMED;
          } // if
        break;

      case CGPIOD_OUT_CMD_TIMEADD:
        if (pObj->dwRun) 
          pObj->dwRun = SetTimerSec(pObj->dwRun, pCmd->parmsOutput.dwRun);
        break;

      case CGPIOD_OUT_CMD_TIMESET:
        pObj->dwRun = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwRun);
        break;

      case CGPIOD_OUT_CMD_TIMEABORT:
        pObj->dwCmd = CGPIOD_OUT_CMD_NONE;
        pObj->dwRun = 0;
        break;

      case CGPIOD_OUT_CMD_BLINK: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_BLINK;
        _outputSetState(pObj, pObj->dwState ^ CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_BLINKTIMED: 
        if (pObj->dwFlags & CGPIOD_OUT_FLG_LOCKED) break;
        pObj->dwCmd = CGPIOD_OUT_CMD_BLINKTIMED;
        pObj->dwRun = SetTimerSec(pCmd->msNow, pCmd->parmsOutput.dwRun);
        _outputSetState(pObj, pObj->dwState ^ CGPIOD_OUT_STATE_ON, &evt);
        break;

      case CGPIOD_OUT_CMD_EMULTIME: 
        // handle command
        if (pCmd->dwParms & CGPIOD_OUT_PRM_EMULTIME)
          pObj->dwRunDef = pCmd->parmsOutput.dwRun;
        
        pCmd->dwRsp = pObj->dwRunDef;   
        evt.dwEvt   = pObj->dwRunDef;
        DoSta(&evt);
        break;

      default:
        pCmd->dwError = XERROR_NOT_FOUND;
        Debug.logTxt(CLSLVL_GPIOD_OUTPUT | 0x9999, "CGpiod::_outputDoCmd,unknown cmd %u", pCmd->dwCmd);
        break;
      } // switch

    // automatically copy state -> rsp
    if (pCmd->dwCmd & CGPIOD_CMD_RSP_AUTO)
      pCmd->dwRsp = pObj->dwState;

    return pCmd->dwError; 
    } // _outputDoCmd

  //--------------------------------------------------------------------------
  // set new state to hw and send event as needed
  //--------------------------------------------------------------------------
  void CGpiod::_outputSetState(tGpiodOutput* pObj, tUint32 dwState, tGpiodEvt* pEvt) 
  {
    // exit if no state change
    if (pObj->dwState == dwState) 
      return;

    // pObj->dwState dwState evt 
    // off           on      on
    // on            off     off
    switch (pObj->dwState = dwState) {
      case CGPIOD_OUT_STATE_OFF:
        _ioSetPinVal(pObj->dwPin, CGPIOD_OUT_STATE_OFF ^ pObj->dwPol);
        if (pEvt) pEvt->dwEvt = CGPIOD_OUT_EVT_OFF;
        break;

      case CGPIOD_OUT_STATE_ON:
        _ioSetPinVal(pObj->dwPin, CGPIOD_OUT_STATE_ON ^ pObj->dwPol);
        if (pEvt) pEvt->dwEvt = CGPIOD_OUT_EVT_ON;
        break;
      } // switch

    if (pEvt) DoEvt(pEvt);
    } // _outputSetState

