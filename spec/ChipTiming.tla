---- MODULE ChipTiming ----
\* adlib_gold chip-access timing (call/0013). The Ad Lib Gold chips need a minimum
\* delay between register writes. On period CPUs that delay came for free; on radically
\* faster CPUs the driver must stall explicitly. This models a write path guarded by a
\* calibrated stall and checks that every write honours the chip's minimum inter-write
\* delay, so the design is correct independent of CPU speed. Model-checked with TLC.

EXTENDS Naturals

CONSTANTS MinDelay, MaxWrites, MaxClock

ASSUME MinDelay \in Nat /\ MinDelay >= 1
ASSUME MaxWrites \in Nat
ASSUME MaxClock \in Nat

VARIABLES clock, lastWrite, writes, lastGap

vars == <<clock, lastWrite, writes, lastGap>>

TypeOK == /\ clock \in 0..MaxClock
          /\ lastWrite \in 0..MaxClock
          /\ writes \in 0..MaxWrites
          /\ lastGap \in 0..MaxClock

Init == /\ clock = 0
        /\ lastWrite = 0
        /\ writes = 0
        /\ lastGap = MinDelay

\* Time passes as the CPU runs; bounded so the model is finite.
Tick == /\ clock < MaxClock
        /\ clock' = clock + 1
        /\ UNCHANGED <<lastWrite, writes, lastGap>>

\* A register write, allowed only once the chip's minimum delay has elapsed since the
\* previous write (the calibrated stall). It records the realised gap.
Write == /\ writes < MaxWrites
         /\ clock - lastWrite >= MinDelay
         /\ lastGap' = clock - lastWrite
         /\ lastWrite' = clock
         /\ writes' = writes + 1
         /\ UNCHANGED clock

Next == Tick \/ Write

Spec == Init /\ [][Next]_vars

\* Safety: every register write honoured the chip's minimum inter-write delay, so a
\* faster CPU never outruns the chip.
DelayHonored == lastGap >= MinDelay
====
