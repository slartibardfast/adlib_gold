---- MODULE BankAccess ----
\* adlib_gold shared bank-select access (plan/0008, the critical review finding).
\*
\* The Control Chip and OPL3 array 1 share the base+2/base+3 ports. Every access is a
\* three-step sequence: enable the control bank, write the register/data, restore the
\* OPL3 bank. Several contexts touch these ports: a mixer/property writer, the FM synth,
\* the surround/EEPROM paths, and the interrupt service routine, which runs at device
\* IRQL. Without serialization the ISR can interleave a writer's sequence and flip the
\* bank, so the writer's next byte reaches the wrong chip.
\*
\* This models the FIXED design: every context runs its enable/write/restore inside one
\* interrupt-sync lock (CallSynchronizedRoutine). It checks that the lock is mutually
\* exclusive and that each context's write always observes the bank it enabled, so no
\* byte is ever emitted against a bank another context switched. Model-checked with TLC.

EXTENDS Naturals

CONSTANTS Contexts        \* the set of accessing contexts, e.g. {mixer, fm, isr}

VARIABLES bank,           \* the shared bank-select state: "control" or "opl3"
          holder,         \* the context holding the sync lock, or "none"
          pc              \* per-context step: "idle" | "enabled" | "wrote"

vars == <<bank, holder, pc>>

TypeOK == /\ bank \in {"control", "opl3"}
          /\ holder \in (Contexts \cup {"none"})
          /\ pc \in [Contexts -> {"idle", "enabled", "wrote"}]

Init == /\ bank = "opl3"
        /\ holder = "none"
        /\ pc = [c \in Contexts |-> "idle"]

\* Acquire the interrupt-sync lock (only while free) and enable the control bank.
Acquire(c) == /\ pc[c] = "idle"
              /\ holder = "none"
              /\ holder' = c
              /\ bank' = "control"
              /\ pc' = [pc EXCEPT ![c] = "enabled"]

\* Write the register index and data. Legal only while this context holds the lock.
Write(c) == /\ pc[c] = "enabled"
            /\ holder = c
            /\ pc' = [pc EXCEPT ![c] = "wrote"]
            /\ UNCHANGED <<bank, holder>>

\* Restore the OPL3 bank and release the lock.
Release(c) == /\ pc[c] = "wrote"
              /\ holder = c
              /\ bank' = "opl3"
              /\ holder' = "none"
              /\ pc' = [pc EXCEPT ![c] = "idle"]

Next == \E c \in Contexts : Acquire(c) \/ Write(c) \/ Release(c)

Spec == Init /\ [][Next]_vars

\* Safety: the lock is mutually exclusive. At most one context is ever between Acquire
\* and Release, so two contexts never drive the shared port at once.
MutualExclusion == \A c1, c2 \in Contexts :
                     (pc[c1] # "idle" /\ pc[c2] # "idle") => (c1 = c2)

\* Safety: whenever a context is mid-sequence (has enabled the bank, up to its write),
\* the bank is still the control bank it enabled. No other context flipped it, so the
\* data byte reaches the intended chip.
WriteSeesControl == \A c \in Contexts :
                      (pc[c] \in {"enabled", "wrote"}) => bank = "control"
====
