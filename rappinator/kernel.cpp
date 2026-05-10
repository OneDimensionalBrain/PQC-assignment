extern "C" int run_benchmarks (void);

#include "kernel.h"
#include <circle/gpiopin.h>
#include <circle/string.h>
#include <circle/util.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

#define PARTITION "emmc1-1"
#define FILENAME  "circle.txt"

#define GPIO_SCOPE_PIN 13

static const char FromKernel[] = "kernel";

/* ----------------------------
   GLOBAL BENCH IO STATE
---------------------------- */
static CFATFileSystem *g_pFileSystem = nullptr;
static CScreenDevice  *g_pScreen      = nullptr;
static unsigned g_nBenchFileHandle    = 0;

/* ----------------------------
   GPIO scope pin
---------------------------- */
CGPIOPin *g_pScopePin = nullptr;

extern "C" void scope_high(void)
{
    if (g_pScopePin) g_pScopePin->Write(1);
}

extern "C" void scope_low(void)
{
    if (g_pScopePin) g_pScopePin->Write(0);
}

/* ----------------------------
   SYSTEM TIMER
---------------------------- */
extern "C" uint64_t get_system_micros(void)
{
    return CTimer::Get()->GetTicks();
}

/* ----------------------------
   SAFE PRINTF FOR BENCHMARKING
---------------------------- */
extern "C" int store_printf(const char *format, ...)
{
    // char Buffer[512];

    // g_pScreen->Write("Is it Msg?", 11);
    va_list args;
    va_start(args, format);
    CString Msg;
    Msg.FormatV(format, args);
    va_end(args);

    // g_pScreen->Write("No\n", 3);

    int length = Msg.GetLength();

    if (length <= 0)
        return length;

    const char *text = (const char *)Msg;

    if (g_pScreen)
    {
        g_pScreen->Write(text, (unsigned)length);
    }

    if (g_pFileSystem && g_nBenchFileHandle)
    {
        g_pFileSystem->FileWrite(
            g_nBenchFileHandle,
            text,
            (unsigned)length
        );
    }

    return length;
}

/* =========================================================
   KERNEL CLASS
========================================================= */

CKernel::CKernel (void)
:   m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
    m_Timer (&m_Interrupt),
    m_Logger (m_Options.GetLogLevel (), &m_Timer),
    m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
    m_scopePin (GPIO_SCOPE_PIN, GPIOModeOutput)
{
    m_ActLED.Blink(5);
    g_pScopePin = &m_scopePin;
}

CKernel::~CKernel (void) {}

/* ----------------------------
   INITIALIZE
---------------------------- */
boolean CKernel::Initialize (void)
{
    boolean bOK = TRUE;

    if (bOK) bOK = m_Screen.Initialize ();
    if (bOK) bOK = m_Serial.Initialize (115200);

    if (bOK)
    {
        CDevice *pTarget =
            m_DeviceNameService.GetDevice(m_Options.GetLogDevice (), FALSE);

        if (!pTarget)
            pTarget = &m_Screen;

        bOK = m_Logger.Initialize (pTarget);
    }

    if (bOK) bOK = m_Interrupt.Initialize ();
    if (bOK) bOK = m_Timer.Initialize ();
    if (bOK) bOK = m_EMMC.Initialize ();

    m_scopePin.Write(0);

    return bOK;
}

/* ----------------------------
   RUN
---------------------------- */
TShutdownMode CKernel::Run (void)
{
    m_Logger.Write(FromKernel, LogNotice,
        "Compile time: " __DATE__ " " __TIME__);

    m_DeviceNameService.ListDevices(&m_Screen, TRUE);

    /* mount filesystem */
    CDevice *pPartition =
        m_DeviceNameService.GetDevice(PARTITION, TRUE);

    if (!pPartition)
    {
        m_Logger.Write(FromKernel, LogPanic,
            "Partition not found: %s", PARTITION);
    }

    if (!m_FileSystem.Mount(pPartition))
    {
        m_Logger.Write(FromKernel, LogPanic,
            "Cannot mount partition: %s", PARTITION);
    }

    /* show directory */
    TDirentry Direntry;
    TFindCurrentEntry CurrentEntry;

    unsigned nEntry =
        m_FileSystem.RootFindFirst(&Direntry, &CurrentEntry);

    for (unsigned i = 0; nEntry != 0; i++)
    {
        if (!(Direntry.nAttributes & FS_ATTRIB_SYSTEM))
        {
            CString FileName;
            FileName.Format("%-14s", Direntry.chTitle);

            m_Screen.Write((const char *)FileName,
                           FileName.GetLength());

            if (i % 5 == 4)
                m_Screen.Write("\n", 1);
        }

        nEntry =
            m_FileSystem.RootFindNext(&Direntry, &CurrentEntry);
    }

    m_Screen.Write("\n", 1);

    /* create benchmark file */
    unsigned hFile =
        m_FileSystem.FileCreate(FILENAME);

    if (!hFile)
    {
        m_Logger.Write(FromKernel, LogPanic,
            "Cannot create file: %s", FILENAME);
    }

    /* IMPORTANT: initialize globals BEFORE benchmark */
    g_pFileSystem = &m_FileSystem;
    g_pScreen = &m_Screen;
    g_nBenchFileHandle = hFile;

    /* run benchmark */
    store_printf(">>> ENTER BENCHMARK <<<\n");
    int r = run_benchmarks();
    store_printf("returned: %d", r);

    // Confirm CPU frequency using 1MHz reference timer vs PMU cycle counter
    // uint32_t c0, c1;
    // asm volatile("mrc p15, 0, %0, c15, c12, 1" : "=r"(c0));
    // CTimer::SimpleusDelay(1000000); // exactly 1 second
    // asm volatile("mrc p15, 0, %0, c15, c12, 1" : "=r"(c1));
    // store_printf("CPU freq: %u Hz\n", (unsigned)(c1 - c0));

    /* close file */
    if (!m_FileSystem.FileClose(hFile))
    {
        m_Logger.Write(FromKernel, LogError,
            "Cannot close file");
    }

    m_Logger.Write(FromKernel, LogNotice,
        "Benchmark complete");

    return ShutdownHalt;
}
