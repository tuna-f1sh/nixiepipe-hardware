#define ST_ANY        -1
#define ST_CLOCK      0
#define ST_SETTIME    1
#define ST_SETALARM   2
#define ST_SETCOLOUR  3
#define ST_SETCOUNT   4
#define ST_COUNTER    5
#define ST_CUPDATE    6
#define ST_RTCFAIL    7
#define ST_FLASH      8
#define ST_DIM        9
#define ST_TERM       10

#define EV_ANY        -1
#define EV_NONE       0
#define EV_TB0_PRESS  10
#define EV_TB1_PRESS  11
#define EV_TB0_HOLD   12
#define EV_TB1_HOLD   13
#define EV_COUNTEND   14
#define EV_RTCFAIL    15
#define EV_ALARM      16

typedef struct {
  int st;
  int ev;
  /* int (*fn)(void);*/
  int nst;
} tTransition;

tTransition trans[] = {
  { ST_CLOCK, EV_TB0_PRESS, ST_SETTIME },
  { ST_CLOCK, EV_TB0_HOLD, ST_SETCOLOUR },
  { ST_CLOCK, EV_TB1_PRESS, ST_SETCOUNT },
  { ST_CLOCK, EV_TB1_HOLD, ST_SETALARM },
  { ST_CLOCK, EV_RTCFAIL, ST_RTCFAIL },
  { ST_CLOCK, EV_ALARM, ST_FLASH },
  { ST_CLOCK, EV_COUNTEND, ST_FLASH },
  { ST_FLASH, EV_NONE, ST_CLOCK },
  { ST_FLASH, EV_TB0_PRESS, ST_CLOCK },
  { ST_FLASH, EV_TB0_HOLD, ST_CLOCK },
  { ST_FLASH, EV_TB1_PRESS, ST_CLOCK },
  { ST_FLASH, EV_TB1_HOLD, ST_CLOCK },
  { ST_RTCFAIL, EV_NONE, ST_CLOCK },
  { ST_SETTIME, EV_ANY, ST_CLOCK },
  { ST_SETALARM, EV_ANY, ST_CLOCK },
  { ST_SETCOUNT, EV_ANY, ST_COUNTER },
  { ST_SETCOLOUR, EV_ANY, ST_CLOCK },
  { ST_COUNTER, EV_COUNTEND, ST_CLOCK },
  { ST_COUNTER, EV_TB0_PRESS, ST_CLOCK },
  { ST_COUNTER, EV_TB0_HOLD, ST_CLOCK },
  { ST_COUNTER, EV_TB1_PRESS, ST_CUPDATE },
  { ST_COUNTER, EV_TB1_HOLD, ST_CLOCK },
  { ST_CUPDATE, EV_ANY, ST_COUNTER },
};

#define TRANS_COUNT (sizeof(trans)/sizeof(*trans))
