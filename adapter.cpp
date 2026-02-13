/*****************************************************************************
 * adapter.cpp - Ad Lib Gold adapter driver implementation.
 *****************************************************************************
 *
 * Setup and resource allocation for the Ad Lib Gold sound card.
 * Controls which miniports are started and which resources are given
 * to each miniport.
 */

//
// All the GUIDs for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#define STR_MODULENAME "AdLibGold: "

#include "common.h"


#if (DBG)
#define SUCCEEDS(s) ASSERT(NT_SUCCESS(s))
#else
#define SUCCEEDS(s) (s)
#endif


/*****************************************************************************
 * Externals
 */
NTSTATUS
CreateMiniportTopologyAdLibGold
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

NTSTATUS
CreateMiniportMidiFMAdLibGold
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);


/*****************************************************************************
 * Referenced forward
 */
extern "C"
NTSTATUS
AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
);

NTSTATUS
StartDevice
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PRESOURCELIST   ResourceList
);


#pragma code_seg("INIT")

/*****************************************************************************
 * DriverEntry()
 *****************************************************************************
 * Called by the operating system when the driver is loaded.
 */
extern "C"
NTSTATUS
DriverEntry
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("DriverEntry"));

    return PcInitializeAdapterDriver(DriverObject,
                                     RegistryPathName,
                                     AddDevice);
}


#pragma code_seg("PAGE")

/*****************************************************************************
 * AddDevice()
 *****************************************************************************
 * Called by the operating system when the device is added.
 */
extern "C"
NTSTATUS
AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("AddDevice"));

    return PcAddAdapterDevice(DriverObject,
                              PhysicalDeviceObject,
                              PCPFNSTARTDEVICE(StartDevice),
                              MAX_MINIPORTS,
                              0);
}


/*****************************************************************************
 * InstallSubdevice()
 *****************************************************************************
 * Creates and registers a subdevice consisting of a port driver, a miniport
 * driver and a set of resources bound together.
 */
NTSTATUS
InstallSubdevice
(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN      PWCHAR              Name,
    IN      REFGUID             PortClassId,
    IN      REFGUID             MiniportClassId,
    IN      PFNCREATEINSTANCE   MiniportCreate      OPTIONAL,
    IN      PUNKNOWN            UnknownAdapter      OPTIONAL,
    IN      PRESOURCELIST       ResourceList,
    IN      REFGUID             PortInterfaceId,
    OUT     PUNKNOWN *          OutPortInterface     OPTIONAL,
    OUT     PUNKNOWN *          OutPortUnknown       OPTIONAL
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("InstallSubdevice %S", Name));

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Name);
    ASSERT(ResourceList);

    //
    // Create the port driver object.
    //
    PPORT       port;
    NTSTATUS    ntStatus = PcNewPort(&port, PortClassId);

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Deposit the port interface if requested.
        //
        if (OutPortInterface)
        {
            (void) port->QueryInterface(PortInterfaceId,
                                        (PVOID *)OutPortInterface);
        }

        //
        // Create the miniport object.
        //
        PUNKNOWN miniport;
        if (MiniportCreate)
        {
            ntStatus = MiniportCreate(&miniport,
                                      MiniportClassId,
                                      NULL,
                                      NonPagedPool);
        }
        else
        {
            ntStatus = PcNewMiniport((PMINIPORT *)&miniport,
                                     MiniportClassId);
        }

        if (NT_SUCCESS(ntStatus))
        {
            //
            // Init the port driver and miniport in one go.
            //
            ntStatus = port->Init(DeviceObject,
                                  Irp,
                                  miniport,
                                  UnknownAdapter,
                                  ResourceList);

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Register the subdevice (port/miniport combination).
                //
                ntStatus = PcRegisterSubdevice(DeviceObject,
                                               Name,
                                               port);
#if DBG
                if (!NT_SUCCESS(ntStatus))
                {
                    _DbgPrintF(DEBUGLVL_TERSE,
                        ("InstallSubdevice: PcRegisterSubdevice failed"));
                }
#endif
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,
                    ("InstallSubdevice: port->Init failed"));
            }

            miniport->Release();
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,
                ("InstallSubdevice: miniport creation failed"));
        }

        //
        // Deposit the port as IUnknown if requested.
        //
        if (NT_SUCCESS(ntStatus))
        {
            if (OutPortUnknown)
            {
                (void) port->QueryInterface(IID_IUnknown,
                                            (PVOID *)OutPortUnknown);
            }
        }
        else
        {
            //
            // Retract previously delivered port interface.
            //
            if (OutPortInterface && (*OutPortInterface))
            {
                (*OutPortInterface)->Release();
                *OutPortInterface = NULL;
            }
        }

        port->Release();
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("InstallSubdevice: PcNewPort failed"));
    }

    return ntStatus;
}


/*****************************************************************************
 * StartDevice()
 *****************************************************************************
 * Called by the operating system when the device is started.
 * Creates the adapter common object and installs subdevices.
 *
 * Ad Lib Gold resource layout (from INF):
 *   1 I/O port range:  base+0 through base+7  (8 ports)
 *   1 IRQ
 *   1 DMA channel      (playback)
 *
 * All subsystems (FM, Control Chip, MMA) share the single port range.
 */
NTSTATUS
StartDevice
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(ResourceList);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("StartDevice: ports=%d IRQs=%d DMAs=%d",
        ResourceList->NumberOfPorts(),
        ResourceList->NumberOfInterrupts(),
        ResourceList->NumberOfDmas()));

    //
    // Validate minimum resources: 1 port range, 1 IRQ.
    // DMA is needed for wave but not for Phase 1 (topology only).
    //
    if ((ResourceList->NumberOfPorts() < 1) ||
        (ResourceList->NumberOfInterrupts() < 1))
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("StartDevice: insufficient resources"));
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Port driver unknowns for registering physical connections later.
    //
    PUNKNOWN    unknownTopology = NULL;
    PUNKNOWN    unknownWave     = NULL;
    PUNKNOWN    unknownFmSynth  = NULL;

    //
    // Build the resource sub-list for the adapter common object.
    // It needs: I/O port range + IRQ (for interrupt sync).
    //
    PRESOURCELIST resourceListAdapter = NULL;
    ntStatus = PcNewResourceSublist(&resourceListAdapter,
                                    NULL,
                                    PagedPool,
                                    ResourceList,
                                    2);
    if (NT_SUCCESS(ntStatus))
    {
        SUCCEEDS(resourceListAdapter->AddPortFromParent(ResourceList, 0));
        SUCCEEDS(resourceListAdapter->AddInterruptFromParent(ResourceList, 0));
    }

    //
    // Create and initialize the adapter common object.
    //
    PADAPTERCOMMON pAdapterCommon = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        PUNKNOWN pUnknownCommon;

        ntStatus = NewAdapterCommon(&pUnknownCommon,
                                    IID_IAdapterCommon,
                                    NULL,
                                    NonPagedPool);
        if (NT_SUCCESS(ntStatus))
        {
            ASSERT(pUnknownCommon);

            ntStatus = pUnknownCommon->QueryInterface(IID_IAdapterCommon,
                                                      (PVOID *)&pAdapterCommon);
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = pAdapterCommon->Init(resourceListAdapter,
                                                DeviceObject);
                if (NT_SUCCESS(ntStatus))
                {
                    ntStatus = PcRegisterAdapterPowerManagement(
                        (PUNKNOWN)pAdapterCommon,
                        DeviceObject);
                }
            }

            pUnknownCommon->Release();
        }
    }

    if (resourceListAdapter)
    {
        resourceListAdapter->Release();
    }

    //
    // Build the resource sub-list for the topology miniport.
    // It only needs the I/O port range (for mixer register access
    // via the adapter common object passed as UnknownAdapter).
    //
    PRESOURCELIST resourceListTopology = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewResourceSublist(&resourceListTopology,
                                        NULL,
                                        PagedPool,
                                        ResourceList,
                                        1);
        if (NT_SUCCESS(ntStatus))
        {
            SUCCEEDS(resourceListTopology->AddPortFromParent(ResourceList, 0));
        }
    }

    //
    // Install the topology miniport.
    //
    if (NT_SUCCESS(ntStatus) && resourceListTopology)
    {
        ntStatus = InstallSubdevice(DeviceObject,
                                    Irp,
                                    L"Topology",
                                    CLSID_PortTopology,
                                    CLSID_PortTopology,     /* not used */
                                    CreateMiniportTopologyAdLibGold,
                                    pAdapterCommon,
                                    resourceListTopology,
                                    GUID_NULL,
                                    NULL,
                                    &unknownTopology);
    }

    if (resourceListTopology)
    {
        resourceListTopology->Release();
    }

    //
    // Build the resource sub-list for the FM synth miniport (ports only).
    //
    PRESOURCELIST resourceListFmSynth = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewResourceSublist(&resourceListFmSynth,
                                        NULL,
                                        PagedPool,
                                        ResourceList,
                                        1);
        if (NT_SUCCESS(ntStatus))
        {
            SUCCEEDS(resourceListFmSynth->AddPortFromParent(ResourceList, 0));
        }
    }

    //
    // Install the FM synth miniport.
    //
    if (NT_SUCCESS(ntStatus) && resourceListFmSynth)
    {
        ntStatus = InstallSubdevice(DeviceObject,
                                    Irp,
                                    L"FMSynth",
                                    CLSID_PortMidi,
                                    CLSID_PortMidi,     /* not used */
                                    CreateMiniportMidiFMAdLibGold,
                                    pAdapterCommon,
                                    resourceListFmSynth,
                                    GUID_NULL,
                                    NULL,
                                    &unknownFmSynth);

        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("StartDevice: FM synth install failed (0x%08X)", ntStatus));
            ntStatus = STATUS_SUCCESS;  /* Non-fatal -- topology still works */
        }
    }

    if (resourceListFmSynth)
    {
        resourceListFmSynth->Release();
    }

    //
    // Register physical connection: FM synth bridge output -> Topology FM input.
    //
    if (unknownTopology && unknownFmSynth)
    {
        PcRegisterPhysicalConnection(
            (PDEVICE_OBJECT)DeviceObject,
            unknownFmSynth,
            1,                  /* FM synth pin 1 = bridge output        */
            unknownTopology,
            1);                 /* Topology pin 1 = PIN_FMSYNTH_SOURCE   */
    }

    //
    // Future phases will install additional subdevices here:
    //
    //   Wave (CLSID_PortWaveCyclic) — needs ports + IRQ + DMA
    //   MIDI (CLSID_PortMidi)      — needs ports + IRQ
    //
    // Physical connections will be registered between them:
    //   Wave render  -> Topology (sampling volume input)
    //   Topology     -> Line Out
    //

    //
    // Release the adapter common object.
    //
    if (pAdapterCommon)
    {
        pAdapterCommon->Release();
    }

    //
    // Release port unknowns.
    //
    if (unknownTopology)
    {
        unknownTopology->Release();
    }
    if (unknownWave)
    {
        unknownWave->Release();
    }
    if (unknownFmSynth)
    {
        unknownFmSynth->Release();
    }

    return ntStatus;
}


#pragma code_seg()

/*****************************************************************************
 * _purecall()
 *****************************************************************************
 * Stub for C++ pure virtual function calls.  Required by the DDK runtime
 * when using classes with pure virtual methods in kernel mode.
 */
int __cdecl
_purecall(void)
{
    ASSERT(!"Pure virtual function called");
    return 0;
}
