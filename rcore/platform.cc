// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "platform.hh"
#include "main.hh"
#include "strings.hh"
#include "thread.hh"
#include "randomhash.hh"
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <sys/resource.h>
#if defined (__i386__) || defined (__x86_64__)
#  include <x86intrin.h>        // __rdtsc
#endif
#include <glib.h>

namespace Rapicorn {

// == CPUInfo ==

/* figure architecture name from compiler */
static const char*
get_arch_name (void)
{
#if     defined  __alpha__
  return "Alpha";
#elif   defined __frv__
  return "frv";
#elif   defined __s390__
  return "s390";
#elif   defined __m32c__
  return "m32c";
#elif   defined sparc
  return "Sparc";
#elif   defined __m32r__
  return "m32r";
#elif   defined __x86_64__ || defined __amd64__
  return "AMD64";
#elif   defined __ia64__
  return "Intel Itanium";
#elif   defined __m68k__
  return "mc68000";
#elif   defined __powerpc__ || defined PPC || defined powerpc || defined __PPC__
  return "PPC";
#elif   defined __arc__
  return "arc";
#elif   defined __arm__
  return "Arm";
#elif   defined __mips__ || defined mips
  return "Mips";
#elif   defined __tune_i686__ || defined __i686__
  return "i686";
#elif   defined __tune_i586__ || defined __i586__
  return "i586";
#elif   defined __tune_i486__ || defined __i486__
  return "i486";
#elif   defined i386 || defined __i386__
  return "i386";
#else
  return "unknown-arch";
#warning platform.cc needs updating for this processor type
#endif
}

/* --- X86 detection via CPUID --- */
#if     defined __i386__
#  define x86_has_cpuid()       ({                              \
  unsigned int __eax, __ecx;                                    \
  __asm__ __volatile__                                          \
    (                                                           \
     /* copy EFLAGS into eax and ecx */                         \
     "pushf ; pop %0 ; mov %0, %1 \n\t"                         \
     /* toggle the ID bit and store back to EFLAGS */           \
     "xor $0x200000, %0 ; push %0 ; popf \n\t"                  \
     /* read back EFLAGS with possibly modified ID bit */       \
     "pushf ; pop %0 \n\t"                                      \
     : "=a" (__eax), "=c" (__ecx)                               \
     : /* no inputs */                                          \
     : "cc"                                                     \
     );                                                         \
  bool __result = (__eax ^ __ecx) & 0x00200000;                 \
  __result;                                                     \
})
/* save EBX around CPUID, because gcc doesn't like it to be clobbered with -fPIC */
#  define x86_cpuid(input, eax, ebx, ecx, edx)  \
  __asm__ __volatile__ (                        \
    /* save ebx in esi */                       \
    "mov %%ebx, %%esi \n\t"                     \
    /* get CPUID with eax=input */              \
    "cpuid \n\t"                                \
    /* swap ebx and esi */                      \
    "xchg %%ebx, %%esi"                         \
    : "=a" (eax), "=S" (ebx),                   \
      "=c" (ecx), "=d" (edx)                    \
    : "0" (input)                               \
    : "cc")
#elif   defined __x86_64__ || defined __amd64__
/* CPUID is always present on AMD64, see:
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/24594.pdf
 * "AMD64 Architecture Programmer's Manual Volume 3",
 * "Appendix D: Instruction Subsets and CPUID Feature Sets"
 */
#  define x86_has_cpuid()                       (1)
/* save EBX around CPUID, because gcc doesn't like it to be clobbered with -fPIC */
#  define x86_cpuid(input, eax, ebx, ecx, edx)  \
  __asm__ __volatile__ (                        \
    /* save ebx in esi */                       \
    "mov %%rbx, %%rsi \n\t"                     \
    /* get CPUID with eax=input */              \
    "cpuid \n\t"                                \
    /* swap ebx and esi */                      \
    "xchg %%rbx, %%rsi"                         \
    : "=a" (eax), "=S" (ebx),                   \
      "=c" (ecx), "=d" (edx)                    \
    : "0" (input)                               \
    : "cc")
#else
#  define x86_has_cpuid()                       (false)
#  define x86_cpuid(input, eax, ebx, ecx, edx)  do {} while (0)
#endif


static jmp_buf cpu_info_jmp_buf;

static void RAPICORN_NORETURN
cpu_info_sigill_handler (int dummy)
{
  longjmp (cpu_info_jmp_buf, 1);
}

static bool
get_x86_cpu_features (CPUInfo *ci,
                      char     vendor[13])
{
  memset (ci, 0, sizeof (*ci));
  /* check if the CPUID instruction is supported */
  if (!x86_has_cpuid ())
    return false;

  /* query intel CPUID range */
  unsigned int eax, ebx, ecx, edx;
  x86_cpuid (0, eax, ebx, ecx, edx);
  unsigned int v_ebx = ebx, v_ecx = ecx, v_edx = edx;
  *((unsigned int*) &vendor[0]) = ebx;
  *((unsigned int*) &vendor[4]) = edx;
  *((unsigned int*) &vendor[8]) = ecx;
  vendor[12] = 0;
  if (eax >= 1)                 /* may query version and feature information */
    {
      x86_cpuid (1, eax, ebx, ecx, edx);
      if (ecx & (1 << 0))
        ci->x86_sse3 = true;
      if (ecx & (1 << 9))
        ci->x86_ssse3 = true;
      if (ecx & (1 << 13))
        ci->x86_cx16 = true;
      if (ecx & (1 << 19))
        ci->x86_sse4_1 = true;
      if (ecx & (1 << 20))
        ci->x86_sse4_2 = true;
      if (edx & (1 << 0))
        ci->x86_fpu = true;
      if (edx & (1 << 4))
        ci->x86_tsc = true;
      if (edx & (1 << 23))
        ci->x86_mmx = true;
      if (edx & (1 << 25))
        {
          ci->x86_sse = true;
          ci->x86_mmxext = true;
        }
      if (edx & (1 << 26))
        ci->x86_sse2 = true;
      if (edx & (1 << 28))
        ci->x86_htt = true;
      /* http://www.intel.com/content/www/us/en/processors/processor-identification-cpuid-instruction-note.html
       * "Intel Processor Identification and the CPUID Instruction"
       */
    }

  /* query extended CPUID range */
  x86_cpuid (0x80000000, eax, ebx, ecx, edx);
  if (eax >= 0x80000001 &&      /* may query extended feature information */
      v_ebx == 0x68747541 &&    /* AuthenticAMD */
      v_ecx == 0x444d4163 && v_edx == 0x69746e65)
    {
      x86_cpuid (0x80000001, eax, ebx, ecx, edx);
      if (edx & (1 << 31))
        ci->x86_3dnow = true;
      if (edx & (1 << 22))
        ci->x86_mmxext = true;
      if (edx & (1 << 30))
        ci->x86_3dnowext = true;
      /* www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25481.pdf
       * "AMD CPUID Specification"
       */
    }

  /* check system support for SSE */
  if (ci->x86_sse)
    {
      struct sigaction action, old_action;
      action.sa_handler = cpu_info_sigill_handler;
      sigemptyset (&action.sa_mask);
      action.sa_flags = SA_NOMASK;
      sigaction (SIGILL, &action, &old_action);
      if (setjmp (cpu_info_jmp_buf) == 0)
        {
#if     defined __i386__ || defined __x86_64__ || defined __amd64__
          unsigned int mxcsr;
          __asm__ __volatile__ ("stmxcsr %0 ; sfence ; emms" : "=m" (mxcsr));
          /* executed SIMD instructions without exception */
          ci->x86_ssesys = true;
#endif // x86
        }
      else
        {
          /* signal handler jumped here */
          // g_printerr ("caught SIGILL\n");
        }
      sigaction (SIGILL, &old_action, NULL);
    }

  return true;
}

static CPUInfo cached_cpu_info; /* = 0; */

CPUInfo
cpu_info (void)
{
  return cached_cpu_info;
}

static void
init_cpuinfo (const StringVector &args)
{
  static char vendor_buffer[13];
  CPUInfo lci;
  memset (&lci, 0, sizeof (lci));
  if (get_x86_cpu_features (&lci, vendor_buffer))
    {
      lci.machine = get_arch_name();
      lci.cpu_vendor = vendor_buffer;
    }
  else
    {
      memset (&lci, 0, sizeof (lci));
      lci.machine = get_arch_name();
      lci.cpu_vendor = "unknown";
    }
  cached_cpu_info = lci;
}
static InitHook _init_cpuinfo ("core/02 Init CPU Info", init_cpuinfo);

String
cpu_info_string (const CPUInfo &cpu_info)
{
  GString *gstring = g_string_new ("");
  g_string_append_printf (gstring,
                          "CPU Architecture: %s\n"
                          "CPU Vendor:       %s\n",
                          cpu_info.machine, cpu_info.cpu_vendor);
  /* processor flags */
  GString *pflags = g_string_new ("");
  if (cpu_info.x86_fpu)
    g_string_append_printf (pflags, " FPU");
  if (cpu_info.x86_tsc)
    g_string_append_printf (pflags, " TSC");
  if (cpu_info.x86_htt)
    g_string_append_printf (pflags, " HTT");
  /* MMX flags */
  GString *mflags = g_string_new ("");
  if (cpu_info.x86_mmx)
    g_string_append_printf (mflags, " MMX");
  if (cpu_info.x86_mmxext)
    g_string_append_printf (mflags, " MMXEXT");
  /* SSE flags */
  GString *sflags = g_string_new ("");
  if (cpu_info.x86_ssesys)
    g_string_append_printf (sflags, " SSESYS");
  if (cpu_info.x86_sse)
    g_string_append_printf (sflags, " SSE");
  if (cpu_info.x86_sse2)
    g_string_append_printf (sflags, " SSE2");
  if (cpu_info.x86_sse3)
    g_string_append_printf (sflags, " SSE3");
  if (cpu_info.x86_ssse3)
    g_string_append_printf (sflags, " SSSE3");
  if (cpu_info.x86_sse4_1)
    g_string_append_printf (sflags, " SSE4.1");
  if (cpu_info.x86_sse4_2)
    g_string_append_printf (sflags, " SSE4.2");
  if (cpu_info.x86_cx16)
    g_string_append_printf (sflags, " CMPXCHG16B");
  /* 3DNOW flags */
  GString *nflags = g_string_new ("");
  if (cpu_info.x86_3dnow)
    g_string_append_printf (nflags, " 3DNOW");
  if (cpu_info.x86_3dnowext)
    g_string_append_printf (nflags, " 3DNOWEXT");
  /* flag output */
  if (pflags->len)
    g_string_append_printf (gstring, "CPU Features:    %s\n", pflags->str);
  if (mflags->len)
    g_string_append_printf (gstring, "CPU Integer SIMD:%s\n", mflags->str);
  if (sflags->len)
    g_string_append_printf (gstring, "CPU Float SIMD:  %s\n", sflags->str);
  if (nflags->len)
    g_string_append_printf (gstring, "CPU Media SIMD:  %s\n", nflags->str);
  g_string_free (nflags, TRUE);
  g_string_free (sflags, TRUE);
  g_string_free (mflags, TRUE);
  g_string_free (pflags, TRUE);
  /* done */
  String retval = gstring->str;
  g_string_free (gstring, TRUE);
  return retval;
}

// == TaskStatus ==
TaskStatus::TaskStatus (int pid, int tid) :
  process_id (pid), task_id (tid >= 0 ? tid : pid), state (UNKNOWN), processor (-1), priority (0),
  utime (0), stime (0), cutime (0), cstime (0),
  ac_stamp (0), ac_utime (0), ac_stime (0), ac_cutime (0), ac_cstime (0)
{}

static bool
update_task_status (TaskStatus &self)
{
  static long clk_tck = 0;
  if (!clk_tck)
    {
      clk_tck = sysconf (_SC_CLK_TCK);
      if (clk_tck <= 0)
        clk_tck = 100;
    }
  int pid = -1, ppid = -1, pgrp = -1, session = -1, tty_nr = -1, tpgid = -1;
  int exit_signal = 0, processor = 0;
  long cutime = 0, cstime = 0, priority = 0, nice = 0, dummyld = 0;
  long itrealvalue = 0, rss = 0;
  unsigned long flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
  unsigned long utime = 0, stime = 0, vsize = 0, rlim = 0, startcode = 0;
  unsigned long endcode = 0, startstack = 0, kstkesp = 0, kstkeip = 0;
  unsigned long signal = 0, blocked = 0, sigignore = 0, sigcatch = 0;
  unsigned long wchan = 0, nswap = 0, cnswap = 0, rt_priority = 0, policy = 0;
  unsigned long long starttime = 0;
  char state = 0, command[8192 + 1] = { 0 };
  String filename = string_format ("/proc/%u/task/%u/stat", self.process_id, self.task_id);
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    return false;
  int n = fscanf (file,
                  "%d %8192s %c "
                  "%d %d %d %d %d "
                  "%lu %lu %lu %lu %lu %lu %lu "
                  "%ld %ld %ld %ld %ld %ld "
                  "%llu %lu %ld "
                  "%lu %lu %lu %lu %lu "
                  "%lu %lu %lu %lu %lu "
                  "%lu %lu %lu %d %d "
                  "%lu %lu",
                  &pid, command, &state, // n=3
                  &ppid, &pgrp, &session, &tty_nr, &tpgid, // n=8
                  &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, // n=15
                  &cutime, &cstime, &priority, &nice, &dummyld, &itrealvalue, // n=21
                  &starttime, &vsize, &rss, // n=24
                  &rlim, &startcode, &endcode, &startstack, &kstkesp, // n=29
                  &kstkeip, &signal, &blocked, &sigignore, &sigcatch, // n=34
                  &wchan, &nswap, &cnswap, &exit_signal, &processor, // n=39
                  &rt_priority, &policy // n=41
                  );
  fclose (file);
  const double jiffies_to_usecs = 1000000.0 / clk_tck;
  if (n >= 3)
    self.state = TaskStatus::State (state);
  if (n >= 15)
    {
      self.ac_utime = utime * jiffies_to_usecs;
      self.ac_stime = stime * jiffies_to_usecs;
    }
  if (n >= 17)
    {
      self.ac_cutime = cutime * jiffies_to_usecs;
      self.ac_cstime = cstime * jiffies_to_usecs;
    }
  if (n >= 18)
    self.priority = priority;
  if (n >= 39)
    self.processor = 1 + processor;
  return true;
}

#define ACCOUNTING_MSECS        50

bool
TaskStatus::update ()
{
  const TaskStatus old (*this);
  const uint64 now = timestamp_realtime();              // usecs
  if (ac_stamp + ACCOUNTING_MSECS * 1000 >= now)
    return false;                                       // limit accounting to a few times per second
  if (!update_task_status (*this))
    return false;
  const double delta = 1000000.0 / MAX (1, now - ac_stamp);
  utime = uint64 (MAX (ac_utime - old.ac_utime, 0) * delta);
  stime = uint64 (MAX (ac_stime - old.ac_stime, 0) * delta);
  cutime = uint64 (MAX (ac_cutime - old.ac_cutime, 0) * delta);
  cstime = uint64 (MAX (ac_cstime - old.ac_cstime, 0) * delta);
  ac_stamp = now;
  return true;
}

String
TaskStatus::string ()
{
  return
    string_format ("pid=%d task=%d state=%c processor=%d priority=%d perc=%.2f%% utime=%.3fms stime=%.3fms cutime=%.3f cstime=%.3f",
                   process_id, task_id, state, processor, priority, (utime + stime) * 0.0001,
                   utime * 0.001, stime * 0.001, cutime * 0.001, cstime * 0.001);
}

// == Entropy ==
static Mutex       entropy_mutex;
static KeccakPRNG *entropy_global_pool = NULL;
static uint64      entropy_mix_simple = 0;

KeccakPRNG&
Entropy::entropy_pool()
{
  if (RAPICORN_LIKELY (entropy_global_pool))
    return *entropy_global_pool;
  assert (entropy_mutex.try_lock() == false); // pool *must* be locked by caller
  // create pool and seed it with system details
  KeccakPRNG *kpool = new KeccakPRNG();
  system_entropy (*kpool);
  // gather entropy from runtime information and mix into pool
  KeccakPRNG keccak;
  runtime_entropy (keccak);
  uint64_t seed_data[25];
  keccak.generate (&seed_data[0], &seed_data[25]);
  kpool->seed (seed_data, 25);
  // establish global pool
  assert (entropy_global_pool == NULL);
  entropy_global_pool = kpool;
  return *entropy_global_pool;
}

void
Entropy::slow_reseed ()
{
  // gather and mangle entropy data
  KeccakPRNG keccak;
  runtime_entropy (keccak);
  // mix entropy into global pool
  uint64_t seed_data[25];
  keccak.generate (&seed_data[0], &seed_data[25]);
  ScopedLock<Mutex> locker (entropy_mutex);
  entropy_pool().xor_seed (seed_data, 25);
}

static constexpr uint64_t
bytehash_fnv64a (const uint8_t *bytes, size_t n, uint64_t hash = 0xcbf29ce484222325)
{
  return n == 0 ? hash : bytehash_fnv64a (bytes + 1, n - 1, 0x100000001b3 * (hash ^ bytes[0]));
}

void
Entropy::add_data (const void *bytes, size_t n_bytes)
{
  const uint64_t bits = bytehash_fnv64a ((const uint8_t*) bytes, n_bytes);
  ScopedLock<Mutex> locker (entropy_mutex);
  entropy_mix_simple ^= bits + (entropy_mix_simple * 1664525);
}

uint64_t
Entropy::get_seed ()
{
  ScopedLock<Mutex> locker (entropy_mutex);
  KeccakPRNG &pool = entropy_pool();
  if (entropy_mix_simple)
    {
      pool.xor_seed (&entropy_mix_simple, 1);
      entropy_mix_simple = 0;
    }
  return pool();
}

static bool
hash_file (KeccakPRNG &pool, const char *filename, const size_t maxbytes = 16384)
{
  FILE *file = fopen (filename, "r");
  if (file)
    {
      uint64_t buffer[maxbytes / 8];
      const size_t l = fread (buffer, sizeof (buffer[0]), sizeof (buffer) / sizeof (buffer[0]), file);
      fclose (file);
      if (l > 0)
        {
          pool.xor_seed (buffer, l);
          return true;
        }
    }
  return false;
}

struct HashStamp {
  uint64_t bstamp;
#ifdef  CLOCK_PROCESS_CPUTIME_ID
  uint64_t tcpu;
#endif
#if     defined (__i386__) || defined (__x86_64__)
  uint64_t tsc;
#endif
};

static void __attribute__ ((__noinline__))
hash_time (HashStamp *hstamp)
{
  asm (""); // enfore __noinline__
  hstamp->bstamp = timestamp_benchmark();
#ifdef  CLOCK_PROCESS_CPUTIME_ID
  {
    struct timespec ts;
    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &ts);
    hstamp->tcpu = ts.tv_sec * uint64_t (1000000000) + ts.tv_nsec;
  }
#endif
#if     defined (__i386__) || defined (__x86_64__)
  hstamp->tsc = __rdtsc();
#endif
}

static void
hash_cpu_usage (KeccakPRNG &pool)
{
  union {
    uint64_t      ui64[24];
    struct {
      struct rusage rusage;     // 144 bytes
      struct tms    tms;        //  32 bytes
      clock_t       clk;        //   8 bytes
    };
  } u = { 0, };
  getrusage (RUSAGE_SELF, &u.rusage);
  u.clk = times (&u.tms);
  pool.xor_seed (u.ui64, sizeof (u.ui64) / sizeof (u.ui64[0]));
}

void
Entropy::system_entropy (KeccakPRNG &pool)
{
  HashStamp hash_stamps[128] = { 0, };
  HashStamp *stamp = &hash_stamps[0];
  hash_time (stamp++);
  uint64_t uint_array[64] = { 0, };
  uint64_t *uintp = &uint_array[0];
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  *uintp++ = timestamp_startup();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  hash_file (pool, "/proc/sys/kernel/random/boot_id");
  hash_time (stamp++);  hash_file (pool, "/proc/version");
  hash_time (stamp++);  hash_file (pool, "/proc/cpuinfo");
  hash_time (stamp++);  hash_file (pool, "/proc/devices");
  hash_time (stamp++);  hash_file (pool, "/proc/meminfo");
  hash_time (stamp++);  hash_file (pool, "/proc/buddyinfo");
  hash_time (stamp++);  hash_file (pool, "/proc/diskstats");
  hash_time (stamp++);  hash_file (pool, "/proc/uptime");
  hash_time (stamp++);  hash_file (pool, "/proc/self/schedstat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/dev");
  hash_time (stamp++);  hash_file (pool, "/proc/net/netstat");
  hash_time (stamp++);  *uintp++ = getuid();
  hash_time (stamp++);  *uintp++ = getgid();
  hash_time (stamp++);  *uintp++ = getpid();
  hash_time (stamp++);  *uintp++ = getppid();
  hash_time (stamp++);  *uintp++ = size_t (&system_entropy);    // code segment
  hash_time (stamp++);  *uintp++ = size_t (&entropy_mutex);     // data segment
  hash_time (stamp++);  *uintp++ = size_t (&stamp);             // stack segment
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);
  assert (uintp <= &uint_array[sizeof (uint_array) / sizeof (uint_array[0])]);
  assert (stamp <= &hash_stamps[sizeof (hash_stamps) / sizeof (hash_stamps[0])]);
  pool.xor_seed ((uint64_t*) &hash_stamps[0], (stamp - &hash_stamps[0]) * sizeof (hash_stamps[0]) / sizeof (uint64_t));
  pool.xor_seed (&uint_array[0], uintp - &uint_array[0]);
}

void
Entropy::runtime_entropy (KeccakPRNG &pool)
{
  HashStamp hash_stamps[128] = { 0, };
  HashStamp *stamp = &hash_stamps[0];
  hash_time (stamp++);
  uint64_t uint_array[64] = { 0, };
  uint64_t *uintp = &uint_array[0];
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  hash_file (pool, "/dev/urandom", 400);
  hash_time (stamp++);  hash_file (pool, "/proc/sys/kernel/random/uuid");
  hash_time (stamp++);  hash_file (pool, "/proc/interrupts");
  hash_time (stamp++);  hash_file (pool, "/proc/loadavg");
  hash_time (stamp++);  hash_file (pool, "/proc/schedstat");
  hash_time (stamp++);  hash_file (pool, "/proc/softirqs");
  hash_time (stamp++);  hash_file (pool, "/proc/stat");
  hash_time (stamp++);  *uintp++ = ThisThread::thread_pid();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);
  assert (uintp <= &uint_array[sizeof (uint_array) / sizeof (uint_array[0])]);
  assert (stamp <= &hash_stamps[sizeof (hash_stamps) / sizeof (hash_stamps[0])]);
  pool.xor_seed ((uint64_t*) &hash_stamps[0], (stamp - &hash_stamps[0]) * sizeof (hash_stamps[0]) / sizeof (uint64_t));
  pool.xor_seed (&uint_array[0], uintp - &uint_array[0]);
}

} // Rapicorn
