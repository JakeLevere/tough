#include <val_task1/val_task1.h>

// finite state machine name for tackling task-1
FSM(val_task1)
{
  //define the states
  FSM_STATES
  {
    STATE_INIT, // fails --> error
    STATE_DETECT_PANEL, // fails --> indicate error and stay transition to error
    STATE_WALK_TO_CONTROL, // fails --> go back, error,
    STATE_DETECT_HANDLE_CENTER, // indicate error and stay
    STATE_ADJUST_ARMS, // fails --> go back , error
    STATE_CORRECT_PITCH, // fails --> go back STATE_DETECT_HANDLE_CENTER
    STATE_CORRECT_YAW, // // fails --> go back STATE_DETECT_HANDLE_CENTER
    STATE_DETECT_FINISH, // fails --> indicate error and stay transition to error
    STATE_WALK_TO_FINISH, // go back
    STATE_END,
    STATE_ERROR
  }

  // give the start state
  FSM_START(STATE_INIT);

  // state machine structure and logic, describe the states tansitions
  FSM_BGN // begin state machine
  {
    FSM_STATE(STATE_INIT)
    {
      // state excecution, call the task
      FSM_CALL_TASK(STATE_INIT)

       // state transitions
       FSM_TRANSITIONS
      {
        // transitions
        FSM_ON_EVENT("/INIT_RETRY", FSM_NEXT(STATE_INIT))
        FSM_ON_EVENT("/INIT_SUCESSFUL", FSM_NEXT(STATE_DETECT_PANEL))
        FSM_ON_EVENT("/INIT_FAILED", FSM_NEXT(STATE_ERROR))

        // or on condition or next state directly
      }
    }
    FSM_STATE(STATE_DETECT_PANEL)
    {
      FSM_CALL_TASK(STATE_DETECT_PANEL)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/DETECT_PANEL_RETRY", FSM_NEXT(STATE_DETECT_PANEL))
        FSM_ON_EVENT("/DETECTED_PANEL", FSM_NEXT(STATE_WALK_TO_CONTROL))
        FSM_ON_EVENT("/DETECT_PANEL_FAILED", FSM_NEXT(STATE_ERROR))
      }
    }
    FSM_STATE(STATE_WALK_TO_CONTROL)
    {
      FSM_CALL_TASK(STATE_WALK_TO_CONTROL)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/WALK_FAILED", FSM_NEXT(STATE_ERROR))
        //FSM_ON_EVENT("/WALK_EXECUTING", FSM_NEXT(STATE_WALK_TO_CONTROL))
        FSM_ON_EVENT("/WALK_RETRY", FSM_NEXT(STATE_DETECT_PANEL))
        FSM_ON_EVENT("/REACHED_PANEL", FSM_NEXT(STATE_DETECT_HANDLE_CENTER))
      }
    }
    FSM_STATE(STATE_DETECT_HANDLE_CENTER)
    {
      FSM_CALL_TASK(STATE_DETECT_HANDLE_CENTER)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/DETECT_HANDLE_RETRY", FSM_NEXT(STATE_DETECT_HANDLE_CENTER))
        FSM_ON_EVENT("/DETECT_HANDLE_FAILED", FSM_NEXT(STATE_ERROR))
        FSM_ON_EVENT("/DETECTED_HANDLE", FSM_NEXT(STATE_ADJUST_ARMS))
      }
    }
    FSM_STATE(STATE_ADJUST_ARMS)
    {

      FSM_CALL_TASK(STATE_ADJUST_ARMS)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/ADJUST_ARMS_RETRY", FSM_NEXT(STATE_ADJUST_ARMS))
        FSM_ON_EVENT("/ADJUST_ARMS_FAILED", FSM_NEXT(STATE_DETECT_HANDLE_CENTER))
        FSM_ON_EVENT("/ADJUST_ARMS_ERROR", FSM_NEXT(STATE_ERROR))
        FSM_ON_EVENT("/ADJUST_ARMS_SUCESSFUL", FSM_NEXT(STATE_CORRECT_PITCH))
      }
    }
    FSM_STATE(STATE_CORRECT_PITCH)
    {

      FSM_CALL_TASK(STATE_CORRECT_PITCH)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/PITCH_CORRECTION_RETRY", FSM_NEXT(STATE_CORRECT_PITCH))
        FSM_ON_EVENT("/PITCH_CORRECTION_FAILED", FSM_NEXT(STATE_DETECT_HANDLE_CENTER))
        FSM_ON_EVENT("/PITCH_CORRECTION_SUCESSFUL", FSM_NEXT(STATE_CORRECT_YAW))
      }
    }
    FSM_STATE(STATE_CORRECT_YAW)
    {

      FSM_CALL_TASK(STATE_CORRECT_YAW)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/YAW_CORRECTION_RETRY", FSM_NEXT(STATE_CORRECT_YAW))
        FSM_ON_EVENT("/YAW_CORRECTION_FAILED", FSM_NEXT(STATE_DETECT_HANDLE_CENTER))
        FSM_ON_EVENT("/YAW_CORRECTION_SUCESSFUL", FSM_NEXT(STATE_DETECT_FINISH))
      }
    }
    FSM_STATE(STATE_DETECT_FINISH)
    {

      FSM_CALL_TASK(STATE_DETECT_FINISH)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/DETECT_FINISH_RETRY", FSM_NEXT(STATE_DETECT_FINISH))
        FSM_ON_EVENT("/DETECT_FINISH_FAILED", FSM_NEXT(STATE_ERROR))
        FSM_ON_EVENT("/DETECT_FINISH_SUCESSFUL", FSM_NEXT(STATE_WALK_TO_FINISH))
      }
    }
    FSM_STATE(STATE_WALK_TO_FINISH)
    {

      FSM_CALL_TASK(STATE_WALK_TO_FINISH)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/WALK_TO_FINISH_RETRY", FSM_NEXT(STATE_DETECT_FINISH))
        FSM_ON_EVENT("/WALK_TO_FINISH_ERROR", FSM_NEXT(STATE_ERROR))
        FSM_ON_EVENT("/WALK_TO_FINISH_SUCESSFUL", FSM_NEXT(STATE_END))
      }
    }
    FSM_STATE(STATE_END)
    {

      FSM_CALL_TASK(STATE_END)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/END_SUCESSFUL", FSM_NEXT(STATE_END))
      }
    }
    FSM_STATE(STATE_ERROR)
    {

      FSM_CALL_TASK(STATE_ERROR)

      FSM_TRANSITIONS
      {
        FSM_ON_EVENT("/RESTART", FSM_NEXT(STATE_INIT))
      }
    }
  }
  FSM_END // end of state machine
}
