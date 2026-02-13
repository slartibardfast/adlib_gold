# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.

# AdLib Gold WDM Audio Driver — Development Plan

## Overview

This document cross-references the Microsoft Windows 2000 DDK audio sample drivers (SB16, FM Synth, UART MIDI) with the Ad Lib Gold Developer Toolkit v1.01 SDK to produce an architectural plan for a WDM kernel-mode audio driver targeting the AdLib Gold sound card.

The SB16 sample is the primary structural template. The AdLib Gold's hardware is fundamentally different in how it exposes its subsystems, so this plan identifies where the SB16 patterns map cleanly and where AdLib Gold–specific adaptations are required.

---

## 1. Hardware Architecture Comparison

### SB16 (Reference)

The SB16 driver addresses a single base address range with multiple subsystems accessed via DSP commands, mixer index/data register pairs, and MPU-401 offsets:

| Subsystem | Access Method |
|-----------|--------------|
| DSP (wave audio) | Command/data at base+0Ch/0Ah/0Eh, DMA for bulk transfer |
| Mixer | Index register at base+04h, data at base+05h |
| OPL3 FM Synth | Ports at base+00h/01h (array 0), base+02h/03h (array 1) |
| MPU-401 MIDI | Data at base+00h (MPU base), status/command at base+01h |

Interrupt source identification: read DSP interrupt status from mixer register 0x82.

### AdLib Gold

The AdLib Gold uses a **completely different** register architecture with three logically independent subsystems sharing overlapping I/O ports:

| Subsystem | Chip | Default Ports | Access Method |
|-----------|------|--------------|---------------|
| FM Synthesis | YMF262 (OPL3) | 388h-38Bh | Register array 0: base+0/1, array 1: base+2/3 |
| Control/Mixer | Custom VLSI | 38Ah/38Bh | Write 0FFh to 38Ah to enable control bank; **mutually exclusive with OPL3 array 1** |
| Digital Audio + MIDI | YMZ263 (MMA) | 388h-396h | Direct register access at base+00h through base+0Eh |

**Critical difference from SB16:** The OPL3 register array 1 and the Control Chip share the same port pair (38Ah/38Bh). Access must be explicitly multiplexed:

- Write **0FFh** to 38Ah → subsequent writes to 38Ah/38Bh go to Control Chip registers
- Write **0FEh** to 38Ah → subsequent writes to 38Ah/38Bh go to OPL3 array 1

This bank-switching has no SB16 equivalent and requires a centralized arbitration mechanism in the adapter common object.

---

## 2. SB16 WDM Sample Structure

The SB16 sample (wdm.txt lines 21061–30189) establishes the port/miniport architecture we will follow:

```
DriverEntry → PcInitializeAdapterDriver
AddDevice   → PcAddAdapterDevice(StartDevice, MAX_MINIPORTS=4)
StartDevice → Create AdapterCommon
           → InstallSubdevice("Topology", PortTopology, MiniportTopology)
           → InstallSubdevice("Wave",     PortWaveCyclic, MiniportWave)
           → InstallSubdevice("FMSynth",  PortMidi, MiniportFMSynth)
           → InstallSubdevice("UART",     PortMidi/PortDMus, MiniportUART)
           → PcRegisterPhysicalConnection (wave↔topology, fm↔topology)
```

Key components:

| File | Role |
|------|------|
| `adapter.cpp` | DriverEntry, AddDevice, StartDevice, InstallSubdevice helper, AssignResources |
| `common.cpp/h` | IAdapterCommon — shared hardware access, interrupt sync, mixer register I/O, registry persistence |
| `algtopo.cpp/h` | Topology miniport — mixer nodes, volume/mute/tone property handlers |
| `algwave.cpp/h` | WaveCyclic miniport — DMA setup, stream creation, format programming |
| `fmsynth/miniport.cpp` | FM synth MIDI miniport — OPL3 register writes with 23µs delays, shadow registers |
| `uart/miniport.cpp` | UART MIDI miniport — MPU-401 FIFO management, ISR, read/write streams |

---

## 3. AdLib Gold Driver Architecture

### 3.1 File Layout

```
adlibgold/
├── adapter.cpp          # DriverEntry, AddDevice, StartDevice
├── common.cpp           # CAdapterCommon — Control Chip access, bank switching, ISR
├── common.h             # IAdapterCommon interface, register constants
├── algtopo.cpp          # Topology miniport — Control Chip mixer exposed as KS nodes
├── algtopo.h
├── algwave.cpp          # WaveCyclic miniport — YMZ263 MMA digital audio
├── algwave.h
├── fmsynth.cpp          # FM MIDI miniport — YMF262 OPL3 (adapted from DDK sample)
├── fmsynth.h
├── midi.cpp             # MIDI miniport — YMZ263 MIDI UART
├── midi.h
├── adlibgold.inf        # INF installation file
├── sources              # Build configuration
└── adlibgold.rc         # Version resource
```

### 3.2 Subdevice Registration

```
StartDevice:
  MAX_MINIPORTS = 5  (topology + wave + FM synth + MIDI + physical connections)

  1. Create CAdapterCommon → Init(ResourceList, DeviceObject)
     - Claim I/O range, create PINTERRUPTSYNC
     - Detect card: read Control Chip ID register (reg 00h, bits D3-D0)
     - Initialize Control Chip to known state

  2. InstallSubdevice("Topology",  CLSID_PortTopology,   MiniportTopology)
  3. InstallSubdevice("Wave",      CLSID_PortWaveCyclic,  MiniportWave)
  4. InstallSubdevice("FMSynth",   CLSID_PortMidi,        MiniportFMSynth)
  5. InstallSubdevice("MIDI",      CLSID_PortMidi,        MiniportMIDI)

  6. PcRegisterPhysicalConnection(Wave render → Topology DAC input)
  7. PcRegisterPhysicalConnection(Wave capture → Topology ADC output)
  8. PcRegisterPhysicalConnection(FMSynth → Topology FM mixer input)
```

---

## 4. Adapter Common (IAdapterCommon)

### Interface

Modeled on SB16's IAdapterCommon (wdm.txt lines 23266–23332) with AdLib Gold–specific methods:

```cpp
DECLARE_INTERFACE_(IAdapterCommon, IUnknown)
{
    // Lifecycle
    STDMETHOD_(NTSTATUS, Init)(PRESOURCELIST ResourceList, PDEVICE_OBJECT DeviceObject);
    STDMETHOD_(PINTERRUPTSYNC, GetInterruptSync)(void);

    // Control Chip access (replaces SB16 mixer reg read/write)
    STDMETHOD_(void, ControlRegWrite)(BYTE Register, BYTE Value);
    STDMETHOD_(BYTE, ControlRegRead)(BYTE Register);
    STDMETHOD_(void, ControlRegReset)(void);

    // Bank switching — the critical difference from SB16
    STDMETHOD_(void, EnableControlBank)(void);   // write 0FFh to base+2
    STDMETHOD_(void, EnableOPL3Bank1)(void);     // write 0FEh to base+2

    // OPL3 register access (delegates to FM miniport but synchronized here)
    STDMETHOD_(void, WriteOPL3)(ULONG Address, UCHAR Data);

    // MMA register access
    STDMETHOD_(void, WriteMMA)(BYTE Register, BYTE Value);
    STDMETHOD_(BYTE, ReadMMA)(BYTE Register);

    // Interrupt dispatch
    STDMETHOD_(void, SetWaveMiniport)(PWAVEMINIPORTADLIBGOLD Miniport);
    STDMETHOD_(void, SetMidiMiniport)(PMIDIMINIPORTADLIBGOLD Miniport);

    // Registry persistence (mirrors SB16 pattern)
    STDMETHOD_(NTSTATUS, SaveMixerSettingsToRegistry)(void);
    STDMETHOD_(NTSTATUS, RestoreMixerSettingsFromRegistry)(void);

    // EEPROM (AdLib Gold specific — Control Chip reg 00h)
    STDMETHOD_(NTSTATUS, SaveToEEPROM)(void);
    STDMETHOD_(NTSTATUS, RestoreFromEEPROM)(void);
};
```

### Control Chip Access Pattern

Derived from SDK Chapter 7.1 (sdk.txt ~lines 7732–8885):

```cpp
void CAdapterCommon::ControlRegWrite(BYTE Register, BYTE Value)
{
    // Must be called with interrupts synchronized via PINTERRUPTSYNC

    // 1. Enable control bank
    WRITE_PORT_UCHAR(m_PortBase + 2, 0xFF);

    // 2. Poll status until not busy (SB and RB bits, D6-D7 of status register)
    UCHAR status;
    ULONG timeout = 1000;
    do {
        status = READ_PORT_UCHAR(m_PortBase + 2);
    } while ((status & 0xC0) && --timeout);

    // 3. Write register index
    WRITE_PORT_UCHAR(m_PortBase + 2, Register);

    // 4. Write data
    WRITE_PORT_UCHAR(m_PortBase + 3, Value);

    // 5. Apply required delay
    //    Regs 4-8: 450µs (poll SB/RB)
    //    Regs 9-16h: 5µs
    if (Register >= 4 && Register <= 8) {
        // Poll SB/RB for completion
        timeout = 1000;
        do {
            status = READ_PORT_UCHAR(m_PortBase + 2);
        } while ((status & 0xC0) && --timeout);
    } else if (Register >= 9 && Register <= 0x16) {
        KeStallExecutionProcessor(5);
    }

    // 6. Re-enable OPL3 bank 1 access
    WRITE_PORT_UCHAR(m_PortBase + 2, 0xFE);
}
```

### Interrupt Service Routine

The AdLib Gold status register at 38Ah (when Control Chip bank is enabled) provides interrupt source bits in D3-D0:

| Bit | Source |
|-----|--------|
| D3 | SCSI interrupt |
| D2 | Telephone interrupt |
| D1 | Sampling (MMA) interrupt |
| D0 | FM (OPL3 timer) interrupt |

```cpp
// ISR pattern — read status, dispatch to appropriate miniport
NTSTATUS CAdapterCommon::InterruptServiceRoutine(
    PINTERRUPTSYNC InterruptSync, PVOID DynamicContext)
{
    CAdapterCommon* that = (CAdapterCommon*)DynamicContext;

    // Enable control bank to read status
    WRITE_PORT_UCHAR(that->m_PortBase + 2, 0xFF);
    UCHAR status = READ_PORT_UCHAR(that->m_PortBase + 2);
    WRITE_PORT_UCHAR(that->m_PortBase + 2, 0xFE);

    if (!(status & 0x0F))
        return STATUS_UNSUCCESSFUL;  // Not our interrupt

    if (status & 0x02)  // Sampling interrupt
        that->m_pWaveMiniport->ServiceWaveISR();

    if (status & 0x01)  // FM timer interrupt
        ; // Handle FM timer if needed

    // MMA status register auto-resets on read
    // Read MMA status to clear the interrupt source
    READ_PORT_UCHAR(that->m_PortBase + 0);  // MMA status at base+0

    return STATUS_SUCCESS;
}
```

---

## 5. Wave Miniport (YMZ263 MMA Digital Audio)

### SB16 Mapping

| SB16 Concept | AdLib Gold Equivalent |
|-------------|----------------------|
| DSP commands for sample rate | MMA reg 09h: frequency select bits |
| DSP commands to start/stop DMA | MMA reg 09h: playback/record enable bits |
| Mixer register for IRQ/DMA config | Control Chip regs 13h/14h: IRQ and DMA channel select |
| 8-bit and 16-bit DMA channels | Two independent DMA channels (ch0 and ch1) |
| DmaChannel->Start/Stop | DMA mode via MMA reg 0Ch bit 0 |

### Key MMA Registers (sdk.txt Chapter 7.3, ~lines 10545+)

| Offset | Register | Purpose |
|--------|----------|---------|
| base+00h | Status | Read: interrupt flags (timer, FIFO, MIDI), auto-clears on read |
| base+02h-07h | Timer counters | 5 hardware timers |
| base+08h | Timer control | Timer enable/mask |
| base+09h | Playback/Record | PCM/ADPCM mode, sample rate, channel select, start/stop |
| base+0Ah | Volume | Output volume for digital audio |
| base+0Bh | PCM data | FIFO data port for PCM/ADPCM samples |
| base+0Ch | Format | Interleaving, data format, FIFO interrupt threshold, DMA mode |
| base+0Dh | MIDI control | MIDI interrupt enable, reset, FIFO control |
| base+0Eh | MIDI data | MIDI data port |

### Supported Formats

From SDK documentation:

| Format | Sample Rates | Channels | Bits |
|--------|-------------|----------|------|
| PCM | 5.5, 11.025, 22.05, 44.1 kHz | Mono, Stereo | 8, 12 |
| ADPCM | 5.5, 11.025, 22.05, 44.1 kHz | Mono, Stereo | 4 (compressed) |

### Stream Implementation

```cpp
NTSTATUS CMiniportWaveStream::SetState(KSSTATE NewState)
{
    switch (NewState) {
    case KSSTATE_RUN:
        // 1. Program MMA format register (0Ch): DMA mode, FIFO threshold
        // 2. Program MMA playback register (09h): rate, PCM/ADPCM, channel, start
        // 3. Start DMA via PortCls DmaChannel->Start()
        break;

    case KSSTATE_PAUSE:
        // 1. Clear start bit in MMA reg 09h
        // 2. DmaChannel->Stop()
        break;

    case KSSTATE_STOP:
        // 1. Clear start bit in MMA reg 09h
        // 2. DmaChannel->Stop()
        // 3. Reset FIFO
        break;
    }
}
```

### DMA Configuration

DMA channels are configured through the Control Chip, not the MMA:

- **Channel 0** (reg 13h): DMA line select (bits D7-D6), DMA enable (D5), IRQ select (D3-D1), IRQ enable (D4)
- **Channel 1** (reg 14h): DMA line select (bits D7-D6), DMA enable (D5)

Gold 1000 supports DMA 1-3 only. Gold 2000 supports DMA 0-3.

Full-duplex (simultaneous record + playback) requires both DMA channels active at matching sample rates.

---

## 6. FM Synth Miniport (YMF262 OPL3)

### Reuse Strategy

The DDK fmsynth sample (wdm.txt lines 16498–19119) can be adapted with minimal changes. The core `SoundMidiSendFM` function already handles dual register arrays:

```cpp
// From DDK sample — this maps directly to AdLib Gold OPL3 access
WRITE_PORT_UCHAR(PortBase + (Address < 0x100 ? 0 : 2), (UCHAR)Address);
KeStallExecutionProcessor(23);
WRITE_PORT_UCHAR(PortBase + (Address < 0x100 ? 1 : 3), Data);
KeStallExecutionProcessor(23);
```

### Required Modifications

1. **Bank coordination**: Writes to ports base+2/3 (OPL3 array 1) must be serialized with Control Chip access via the adapter common object's bank switching. The FM miniport must call `EnableOPL3Bank1()` before writing to array 1 registers.

2. **NEW bit management**: AdLib Gold's OPL3 features (stereo output, 4-op voices, extended waveforms) require the NEW bit (D0 at register 05h, array 1) to be set during initialization and cleared on exit. The DDK sample doesn't do this.

3. **4-Operator voice support**: Connection Select register (04h, array 1) enables pairing 2-op voices into 4-op voices. The DDK sample uses 2-op voices only. Supporting 4-op voices is an enhancement for a later phase.

4. **Stereo output bits**: STL/STR bits (D5/D4 at registers C0h-C8h) control per-voice stereo panning. The DDK sample ignores these (outputs to both channels in compatibility mode).

---

## 7. Topology Miniport (Control Chip Mixer)

### SB16 Topology Mapping

The SB16's mixer is a simple indexed register bank. The AdLib Gold Control Chip is more complex, with asymmetric register layouts and different value encodings.

### Node Map

| KS Node Type | Control Chip Register | Range | Notes |
|-------------|----------------------|-------|-------|
| KSNODETYPE_VOLUME (Master L) | Reg 04h | +6dB to -80dB, 2dB steps | D6-D7 must be 1 |
| KSNODETYPE_VOLUME (Master R) | Reg 05h | +6dB to -80dB, 2dB steps | D6-D7 must be 1 |
| KSNODETYPE_TONE (Bass) | Reg 06h | +15dB to -12dB, 3dB steps | D4-D7 must be 1 |
| KSNODETYPE_TONE (Treble) | Reg 07h | +12dB to -12dB, 3dB steps | D4-D7 must be 1 |
| KSNODETYPE_MUTE | Reg 08h, bit D5 (MU) | On/Off | |
| KSNODETYPE_STEREO_WIDE | Reg 08h, bits D3-D2 | Mono/Linear/Pseudo/Spatial | |
| KSNODETYPE_MUX (Source) | Reg 08h, bits D1-D0 | Both/Left/Right | |
| KSNODETYPE_VOLUME (FM L) | Reg 09h | 128–255 linear | <128 = negative polarity, avoid |
| KSNODETYPE_VOLUME (FM R) | Reg 0Ah | 128–255 linear | |
| KSNODETYPE_VOLUME (Sampling L) | Reg 0Bh | 128–255 linear | |
| KSNODETYPE_VOLUME (Sampling R) | Reg 0Ch | 128–255 linear | |
| KSNODETYPE_VOLUME (Aux L) | Reg 0Dh | 128–255 linear | |
| KSNODETYPE_VOLUME (Aux R) | Reg 0Eh | 128–255 linear | |
| KSNODETYPE_VOLUME (Mic) | Reg 0Fh | 128–255 linear | Mono |
| KSNODETYPE_AGC (Gain L) | Reg 02h | 0–255, Gain=(val×10)/256 | |
| KSNODETYPE_AGC (Gain R) | Reg 03h | 0–255, Gain=(val×10)/256 | |

### Physical Connections (Topology Filter)

```
Pin: Wave Render  ──► [Sampling Vol L/R] ──►─┐
Pin: FM Synth     ──► [FM Volume L/R]    ──►─┤
Pin: Aux Input    ──► [Aux Volume L/R]   ──►─┤ [Mixer Sum] ──► [Bass] ──► [Treble]
Pin: Mic Input    ──► [Mic Volume]       ──►─┤    ──► [Source Mux] ──► [Master Vol L/R]
Pin: PC Speaker   ──► (direct)           ──►─┘    ──► [Stereo Mode] ──► [Mute] ──► Pin: Line Out
                                                   ──► [Surround] (optional)
Pin: Wave Capture ◄── [Sampling Gain L/R] ◄── [ADC Source] ◄── (Mic/Aux/FM)
```

---

## 8. MIDI Miniport (YMZ263 UART)

### Mapping to DDK UART Sample

The DDK UART sample (wdm.txt lines 28420–30111) targets the MPU-401 chip. The YMZ263's MIDI interface is accessed differently but serves the same function.

| MPU-401 (DDK) | YMZ263 MMA (AdLib Gold) |
|---------------|------------------------|
| Status at base+01h (read) | MMA reg 00h (status), bit flags for MIDI Rx/Tx ready |
| Data at base+00h (read/write) | MMA reg 0Eh (MIDI data port) |
| Command at base+01h (write) | MMA reg 0Dh (MIDI control: reset, IRQ enable, FIFO control) |
| IRQ on data available | MMA status bit for MIDI receive interrupt |

### Adaptations Required

1. **FIFO management**: The YMZ263 has internal FIFOs for both MIDI transmit and receive. The DDK sample uses a software ring buffer (`m_MPUInputBuffer`). We should still maintain a software buffer but drain the hardware FIFO in the ISR.

2. **Reset sequence**: Instead of MPU-401 UART mode command (0x3F), the YMZ263 MIDI is reset via control bits in MMA reg 0Dh.

3. **Overrun handling**: MMA reg 0Dh has explicit bits to clear transmit and receive overrun conditions.

---

## 9. Phased Implementation Plan

### Phase 1: Skeleton + Control Chip

**Goal**: Driver loads, detects card, exposes no audio functionality yet.

1. `adapter.cpp` — DriverEntry, AddDevice, StartDevice (topology only)
2. `common.cpp` — IAdapterCommon with Control Chip register access, bank switching, status polling
3. Card detection via Control Chip ID register (reg 00h, D3-D0)
4. **Verify**: Driver installs, Device Manager shows device, DbgPrint confirms card detection

### Phase 2: Topology Miniport

**Goal**: Windows mixer panel shows volume controls.

1. `algtopo.cpp` — Define PCFILTER_DESCRIPTOR with all mixer nodes
2. Property handlers for volume (master, FM, sampling, aux, mic), bass, treble, mute
3. Registry persistence for mixer settings
4. **Verify**: sndvol32 shows sliders; adjusting master volume changes hardware register values

### Phase 3: FM Synth Miniport

**Goal**: MIDI playback through OPL3 works.

1. Adapt DDK fmsynth sample, change port base and add bank switch coordination
2. Set NEW bit on init, clear on shutdown
3. Enable stereo output bits (STL/STR)
4. Shadow register array for power management
5. **Verify**: `midiOutOpen` + `midiOutShortMsg` produces sound; Windows Media Player plays .mid files

### Phase 4: Wave Miniport (Digital Audio)

**Goal**: PCM playback and recording through YMZ263.

1. `algwave.cpp` — CMiniportWaveCyclic with slave DMA channel allocation
2. Program Control Chip regs 13h/14h for IRQ and DMA channel assignment
3. Stream implementation: SetFormat (program MMA reg 09h), SetState (start/stop), FIFO/DMA management
4. Support PCM 8-bit/12-bit at 5.5/11.025/22.05/44.1 kHz, mono and stereo
5. Capture stream support
6. **Verify**: `sndPlaySound` plays a WAV file; Sound Recorder records and plays back

### Phase 5: MIDI Miniport

**Goal**: External MIDI I/O through YMZ263.

1. Adapt DDK UART sample for YMZ263 register layout
2. ISR integration — drain MMA MIDI receive FIFO, buffer in software ring buffer
3. Transmit via polling MMA status for Tx ready
4. **Verify**: External MIDI keyboard input reaches a sequencer application

### Phase 6: Polish

1. Power management (save/restore all register state)
2. EEPROM persistence (Control Chip reg 00h save/restore commands — 2.5ms restore delay)
3. ADPCM format support (if desired, via MMA reg 09h mode bits)
4. Surround sound module support (YM7128 via Control Chip reg 18h — optional)
5. INF file, co-installer, proper PnP resource handling
6. Stress testing, WHQL preparation

---

## 10. Critical Implementation Notes

### Timing Constraints

All derived from SDK documentation; violation causes data corruption or hangs:

| Operation | Required Delay | Method |
|-----------|---------------|--------|
| Control Chip regs 4-8 write | 450µs | Poll SB/RB status bits |
| Control Chip regs 9-16h write | 5µs | KeStallExecutionProcessor |
| EEPROM restore (reg 00h) | 2.5ms | KeStallExecutionProcessor |
| OPL3 register write | 23µs | KeStallExecutionProcessor (per DDK sample) |
| Bank switch (0FFh/0FEh to 38Ah) | Immediate | No documented delay, but poll SB/RB before next access |

### Interrupt Safety

The SDK mandates disabling interrupts during hardware access sequences (pushf/cli/popf pattern). In WDM, this is handled by the `PINTERRUPTSYNC` object — all hardware access must be performed within `InterruptSync->CallSynchronizedRoutine()` or at DIRQL within the ISR.

### Bank Switching Serialization

This is the single most error-prone aspect of the driver. The OPL3 array 1 and Control Chip share ports 38Ah/38Bh. All access must go through the adapter common object which maintains current bank state and serializes transitions. A spinlock or synchronized routine must protect the enable-control/write-register/enable-OPL3 sequence atomically.

### Resource Allocation

The card's I/O range is 16 ports starting at 388h by default (relocatable via Control Chip reg 15h). The INF must claim:

- I/O ports: base through base+0Eh (15 ports for MMA) plus base through base+3 (4 ports for OPL3/Control)
- IRQ: One line, shared across all subsystems (selected via Control Chip reg 13h)
- DMA: Up to two channels (selected via Control Chip regs 13h/14h)

### Reference Locations in Source Documents

| Topic | File | Lines |
|-------|------|-------|
| SB16 adapter entry points | wdm.txt | 21061–22069 |
| SB16 IAdapterCommon interface | wdm.txt | 23051–23351 |
| SB16 adapter common implementation | wdm.txt | 22070–23050 |
| SB16 wave miniport | wdm.txt | 25058–26787 |
| SB16 topology miniport | wdm.txt | 23392–25057 |
| DDK FM synth miniport | wdm.txt | 16498–19119 |
| DDK UART MIDI miniport | wdm.txt | 28420–30111 |
| AdLib Gold Control Chip registers | sdk.txt | ~7732–8885 |
| AdLib Gold OPL3 (YMF262) programming | sdk.txt | ~8886–10544 |
| AdLib Gold MMA (YMZ263) programming | sdk.txt | ~10545–11365 |
| AdLib Gold hardware description | sdk.txt | ~536–1033 |
| AdLib Gold surround (YM7128) | sdk.txt | ~13583–14618 |

---

## 11. Host Environment

### Target Platform

This is a **32-bit WDM kernel-mode driver** targeting **Windows 98 SE and above** (Windows 98 SE, Windows Me, Windows 2000, Windows XP). All code must be i386/x86 only — no 64-bit considerations apply.

### Build Toolchain

The driver is built using the **Microsoft Windows 2000 DDK** (Device Driver Kit) build environment. This is a strict requirement — the DDK's `build.exe` utility and associated toolchain must be used, not a standalone Visual Studio project.

The Windows 2000 DDK ships with **Microsoft Visual C++ 6.0 (cl.exe version 12.00)**. This is the only supported compiler. All code must compile cleanly under MSVC 6.0's C++ front-end. Key constraints:

| Constraint | Detail |
|-----------|--------|
| C++ standard | Pre-C++98 in practice; MSVC 6.0 has incomplete C++98 support |
| No `typename` in dependent contexts | MSVC 6.0 often rejects or ignores `typename` where the standard requires it |
| No partial template specialization | Not supported |
| No member templates | Severely limited support |
| No `explicit` on multi-arg constructors | Only single-arg supported |
| No covariant return types | Not supported |
| No `for`-scoped variables | `for(int i=...)` leaks `i` into the enclosing scope (non-conforming) |
| No `__int64` in switch cases | Use `if`/`else` chains instead |
| No C99 features in C mode | No `//` comments in .c files (use `/* */`), no mixed declarations and code, no `__func__` |
| Struct/class member alignment | Default is 8-byte; DDK headers assume this — do not change `/Zp` |

### DDK Build Environment Usage

Builds are invoked from a DDK command shell (`setenv.bat`), not from a regular command prompt:

- **Checked (debug) build**: `build -ceZ` from a Checked Build Environment shell
- **Free (release) build**: `build -ceZ` from a Free Build Environment shell

The `sources` file (not a Makefile) controls compilation. It specifies `TARGETNAME`, `TARGETTYPE=DRIVER`, source file lists, include paths, and link libraries. The DDK's `build.exe` reads `sources` and invokes `cl.exe` and `link.exe` with the correct kernel-mode flags (`/Gz` for `__stdcall`, `/GF` for string pooling, `/Oy` for frame pointer omission in free builds, etc.).

### Coding Rules Imposed by the Toolchain

- **No C++ exceptions** (`/EHs-c-` is implicit in kernel mode). Do not use `try`/`catch`/`throw`.
- **No RTTI** (`/GR-` is implicit). Do not use `dynamic_cast` or `typeid`.
- **No C++ standard library**. No `<iostream>`, `<vector>`, `<string>`, etc. Use DDK/NT kernel APIs only.
- **No floating point** in kernel mode (no FPU context is saved across interrupts). All volume/gain calculations must use integer or fixed-point arithmetic.
- **`extern "C"`** is required for `DriverEntry` and any functions called by the kernel or PortCls.
- **`#pragma code_seg("PAGE")`** for pageable code; **`#pragma code_seg()`** for non-paged (IRQL >= DISPATCH_LEVEL) code. Incorrect segment placement causes blue screens.
- All DDK headers expect `_X86_` to be defined (set automatically by the build environment).

### Summary

Write plain C++ that would compile under MSVC 6.0. Prefer C-style constructs in ambiguous cases. Do not use any language feature introduced after Visual C++ 6.0 (1998). When in doubt, keep it simple — the DDK compiler will reject anything it does not understand, and its error messages are often unhelpful.
