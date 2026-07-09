---- MODULE NotifyLiveness ----
\* adlib_gold interrupt-notification liveness (call/0034, plan/0016). The retest
\* heard the cyclic DMA buffer loop its prefill: DMA, FIFO and DAC all ran, and the
\* portcls refill never came, so the one dead leg is interrupt -> notify -> refill.
\* This models that loop: the sampling FIFO flag, the MMA timer flag, the shared
\* line as the OR of unmasked flags, the edge-triggered ISA controller (delivery
\* only on a rising edge), and the service routine's exact read sequence. Hardware
\* semantics the SDK leaves open are chosen nondeterministically at Init, so one
\* TLC run covers every world:
\*   fifoFires   - whether the DMA-mode FIFO threshold interrupt fires periodically,
\*                 or never while auto-initialize DMA keeps the FIFO fed (the SDK's
\*                 programming tips read it as an end-of-transfer indicator, manual
\*                 ch07; in that world the naive design receives no interrupts at
\*                 all, which is the retest symptom).
\*   clearOnRead - whether a status read clears the FIFO flag (a latched event) or
\*                 the flag stands while its condition stands (a level).
\* The naive design is the shipped code: notification rides the FIFO interrupt
\* alone. The hardened design masks the DMA stream's FIFO interrupt and runs an
\* MMA timer as the service clock (the plan/0014 timer-notification shape, pulled
\* forward). The checked property is liveness under weak fairness: the buffer
\* keeps being refilled in every world. Model-checked with TLC.

EXTENDS Naturals

CONSTANTS Design, RefillTarget

ASSUME Design \in {"naive", "hardened"}
ASSUME RefillTarget \in Nat /\ RefillTarget >= 1

VARIABLES
    fifoFires,      \* world: the FIFO threshold interrupt fires under healthy DMA
    clearOnRead,    \* world: a status read clears the FIFO flag
    fifoFlag,       \* the sampling FIFO interrupt flag
    timerFlag,      \* the MMA timer flag (the hardened design's service clock)
    irr,            \* the controller has latched a rising edge, not yet delivered
    isrPc,          \* the service routine: "idle" | "read" | "notify"
    refills         \* portcls refills of the cyclic buffer, bounded

vars == <<fifoFires, clearOnRead, fifoFlag, timerFlag, irr, isrPc, refills>>

\* The hardened design masks the DMA stream's FIFO interrupt and runs the timer;
\* the naive design leaves the FIFO interrupt unmasked and has no timer.
FifoMasked == Design = "hardened"
TimerOn    == Design = "hardened"

\* The shared interrupt line: the OR of the unmasked flags.
LineIs(ff, tf) == (ff /\ ~FifoMasked) \/ tf

\* The edge-triggered controller latches a request only when the line rises.
Latch(ff, tf, ffN, tfN) == irr \/ (~LineIs(ff, tf) /\ LineIs(ffN, tfN))

TypeOK == /\ fifoFires \in BOOLEAN
          /\ clearOnRead \in BOOLEAN
          /\ fifoFlag \in BOOLEAN
          /\ timerFlag \in BOOLEAN
          /\ irr \in BOOLEAN
          /\ isrPc \in {"idle", "read", "notify"}
          /\ refills \in 0..RefillTarget

Init == /\ fifoFires \in BOOLEAN
        /\ clearOnRead \in BOOLEAN
        /\ fifoFlag = FALSE
        /\ timerFlag = FALSE
        /\ irr = FALSE
        /\ isrPc = "idle"
        /\ refills = 0

\* The FIFO drains to its interrupt point. In the fifoFires = FALSE world this
\* never happens while the stream plays: auto-initialize DMA refills the FIFO
\* ahead of the threshold, so the flag never rises.
FifoCross == /\ fifoFires
             /\ ~fifoFlag
             /\ fifoFlag' = TRUE
             /\ irr' = Latch(fifoFlag, timerFlag, TRUE, timerFlag)
             /\ UNCHANGED <<fifoFires, clearOnRead, timerFlag, isrPc, refills>>

\* In the level-semantics world the flag stands only while the condition stands:
\* the DMA refill recovers the FIFO and the flag falls. A falling line latches
\* nothing.
FifoRecover == /\ ~clearOnRead
               /\ fifoFlag
               /\ fifoFlag' = FALSE
               /\ UNCHANGED <<fifoFires, clearOnRead, timerFlag, irr, isrPc,
                              refills>>

\* The MMA timer fires at the programmed service cadence.
TimerFire == /\ TimerOn
             /\ ~timerFlag
             /\ timerFlag' = TRUE
             /\ irr' = Latch(fifoFlag, timerFlag, fifoFlag, TRUE)
             /\ UNCHANGED <<fifoFires, clearOnRead, fifoFlag, isrPc, refills>>

\* The controller delivers a latched request once the routine is idle; an edge
\* latched during service is held and delivered after the return.
Deliver == /\ irr
           /\ isrPc = "idle"
           /\ irr' = FALSE
           /\ isrPc' = "read"
           /\ UNCHANGED <<fifoFires, clearOnRead, fifoFlag, timerFlag, refills>>

\* The routine reads the MMA status and re-arms Timer 0 (stop, then start
\* reloads the counter), which ends the elapsed condition, so the timer flag
\* falls within the service step whatever the flag semantics (the status flags
\* are level-sensitive, mmastatus.h). The FIFO flag clears on read only in the
\* latched-semantics world.
IsrRead == /\ isrPc = "read"
           /\ timerFlag' = FALSE
           /\ fifoFlag' = IF clearOnRead THEN FALSE ELSE fifoFlag
           /\ isrPc' = "notify"
           /\ UNCHANGED <<fifoFires, clearOnRead, irr, refills>>

\* The routine notifies the port driver, which refills the cyclic buffer.
IsrNotify == /\ isrPc = "notify"
             /\ refills' = IF refills < RefillTarget THEN refills + 1
                           ELSE refills
             /\ isrPc' = "idle"
             /\ UNCHANGED <<fifoFires, clearOnRead, fifoFlag, timerFlag, irr>>

Next == FifoCross \/ FifoRecover \/ TimerFire \/ Deliver \/ IsrRead \/ IsrNotify

Spec == /\ Init
        /\ [][Next]_vars
        /\ WF_vars(FifoCross)
        /\ WF_vars(FifoRecover)
        /\ WF_vars(TimerFire)
        /\ WF_vars(Deliver)
        /\ WF_vars(IsrRead)
        /\ WF_vars(IsrNotify)

\* Liveness: while the stream runs, the cyclic buffer keeps being refilled. The
\* bounded counter reaching its target stands for infinitely many refills.
NotifyLive == <>(refills = RefillTarget)
====
