// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CPU_HH__
#define __RAPICORN_CPU_HH__

#include <rcore/utilities.hh>
#include <rcore/randomhash.hh>

namespace Rapicorn {

/// Acquire information about the runtime architecture and CPU type.
struct CPUInfo {
  // architecture name
  const char *machine;
  // CPU Vendor ID
  const char *cpu_vendor;
  // CPU features on X86
  uint x86_fpu : 1, x86_ssesys : 1, x86_tsc   : 1, x86_htt      : 1;
  uint x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  uint x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_ssse3    : 1;
  uint x86_cx16 : 1, x86_sse4_1 : 1, x86_sse4_2 : 1;
};

CPUInfo cpu_info	(void);
String  cpu_info_string	(const CPUInfo &cpu_info);

/// Acquire information about a task (process or thread) at runtime.
struct TaskStatus {
  enum State { UNKNOWN = '?', RUNNING = 'R', SLEEPING = 'S', DISKWAIT = 'D', STOPPED = 'T', PAGING = 'W', ZOMBIE = 'Z', DEBUG = 'X', };
  int           process_id;     ///< Process ID.
  int           task_id;        ///< Process ID or thread ID.
  String        name;           ///< Thread name (set by user).
  State         state;          ///< Thread state.
  int           processor;      ///< Rrunning processor number.
  int           priority;       ///< Priority or nice value.
  uint64        utime;          ///< Userspace time.
  uint64        stime;          ///< System time.
  uint64        cutime;         ///< Userspace time of dead children.
  uint64        cstime;         ///< System time of dead children.
  uint64        ac_stamp;       ///< Accounting stamp.
  uint64        ac_utime, ac_stime, ac_cutime, ac_cstime;
  explicit      TaskStatus (int pid, int tid = -1); ///< Construct from process ID and optionally thread ID.
  bool          update     ();  ///< Update status information, might return false if called too frequently.
  String        string     ();  ///< Retrieve string representation of the status information.
};

/// Entropy gathering and provisioning class.
class Entropy {
  static KeccakPRNG& entropy_pool();
protected:
  static void     system_entropy  (KeccakPRNG &pool);
  static void     runtime_entropy (KeccakPRNG &pool);
public:
  /// Add up to 64 bits to entropy pool.
  static void     add_bits      (uint64_t bits)                 { add_data (&bits, sizeof (bits)); }
  /// Add an arbitrary number of bytes to the entropy pool.
  static void     add_data      (const void *bytes, size_t n_bytes);
  /// Gather system statistics (might take several milliseconds) and add to the entropy pool.
  static void     slow_reseed   ();
  /// Get 64 bit of high quality random bits useful for seeding PRNGs.
  static uint64_t get_seed      ();
  /// Fill the range [begin, end) with high quality random seed values.
  template<typename RandomAccessIterator> static void
  generate (RandomAccessIterator begin, RandomAccessIterator end)
  {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type Value;
    while (begin != end)
      {
        const uint64_t rbits = get_seed();
        *begin++ = Value (rbits);
        if (sizeof (Value) <= 4 && begin != end)
          *begin++ = Value (rbits >> 32);
      }
  }
};

} // Rapicorn

#endif /* __RAPICORN_CPU_HH__ */
