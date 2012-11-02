// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "blobres.hh"
#include "thread.hh"
#include "strings.hh"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace Rapicorn {

// == BlobResource ==
struct BlobResource {
  virtual String        name            () = 0;
  virtual size_t        size            () = 0;
  virtual const char*   data            () = 0;
  virtual String        string          () = 0;
  virtual              ~BlobResource    () {}
};

// == ByteBlob ==
template<class Deleter>
struct ByteBlob : public BlobResource {
  const char           *m_data;
  size_t                m_size;
  String                m_name;
  String                m_string;
  Deleter               m_deleter;
  explicit              ByteBlob        (const String &name, size_t dsize, const char *data, const Deleter &deleter);
  virtual String        name            ()      { return m_name; }
  virtual size_t        size            ()      { return m_size; }
  virtual const char*   data            ()      { return m_data; }
  virtual String        string          ();
  virtual              ~ByteBlob        ();
};

template<class Deleter>
ByteBlob<Deleter>::ByteBlob (const String &name, size_t dsize, const char *data, const Deleter &deleter) :
  m_data (data), m_size (dsize), m_name (name), m_deleter (deleter)
{}

template<class Deleter>
ByteBlob<Deleter>::~ByteBlob ()
{
  m_deleter (m_data);
}

template<class Deleter> String
ByteBlob<Deleter>::string ()
{
  if (m_string.empty())
    {
      static Mutex mutex;
      ScopedLock<Mutex> g (mutex);
      if (m_string.empty())
        {
          size_t len = m_size;
          while (len && m_data[len - 1] == 0)
            len--;
          m_string = String (m_data, len);
        }
    }
  return m_string;
}

// == ResourceEntry ==
static ResourceEntry *res_head = NULL;
static Mutex          res_mutex;

void
ResourceEntry::reg_add (ResourceEntry *entry)
{
  assert_return (entry && !entry->next);
  ScopedLock<Mutex> sl (res_mutex);
  entry->next = res_head;
  res_head = entry;
}

ResourceEntry::~ResourceEntry()
{
  ScopedLock<Mutex> sl (res_mutex);
  ResourceEntry **ptr = &res_head;
  while (*ptr != this)
    ptr = &(*ptr)->next;
  *ptr = next;
}

const ResourceEntry*
ResourceEntry::find_entry (const String &res_name)
{
  ScopedLock<Mutex> sl (res_mutex);
  for (const ResourceEntry *e = res_head; e; e = e->next)
    if (res_name == e->name)
      return e;
  return NULL;
}

// == StringBlob ==
struct StringBlob : public BlobResource {
  String                m_name;
  String                m_string;
  explicit              StringBlob      (const String &name, const String &data) : m_name (name), m_string (data) {}
  virtual String        name            ()      { return m_name; }
  virtual size_t        size            ()      { return m_string.size(); }
  virtual const char*   data            ()      { return m_string.c_str(); }
  virtual String        string          ()      { return m_string; }
  virtual              ~StringBlob      ()      {}
};

static String // provides errno on error
string_read (const String &filename, const int fd, size_t guess)
{
  String data;
  if (guess)
    data.resize (guess + 1);            // pad by +1 to detect EOF reads
  else
    data.resize (4096);                 // default buffering for unknown sizes
  size_t stored = 0;
  for (ssize_t l = 1; l > 0; )
    {
      if (stored >= data.size())        // only triggered for unknown sizes
        data.resize (2 * data.size());
      do
        l = read (fd, &data[stored], data.size() - stored);
      while (l < 0 && (errno == EAGAIN || errno == EINTR));
      stored += MAX (0, l);
      if (l < 0)
        DEBUG ("%s: read: %s", filename.c_str(), strerror (errno));
      else
        errno = 0;
    }
  data.resize (stored);
  return data;
}

// == Blob ==
String          Blob::name   () const { return m_blob ? m_blob->name() : ""; }
size_t          Blob::size   () const { return m_blob ? m_blob->size() : 0; }
const char*     Blob::data   () const { return m_blob ? m_blob->data() : NULL; }
const uint8*    Blob::bytes  () const { return reinterpret_cast<const uint8*> (data()); }
String          Blob::string () const { return m_blob ? m_blob->string() : ""; }

Blob::Blob (const std::shared_ptr<BlobResource> &initblob) :
  m_blob (initblob)
{}

#define ISASCIIWHITESPACE(c)    (c == ' ' || c == '\t' || (c >= 11 && c <= 13))    // ' \t\v\f\r'
#define ISALPHA(c)              ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define ISDIGIT(c)              ((c >= '0' && c <= '9'))
#define ISALNUM(c)              (ISALPHA (c) || ISDIGIT (c))

static bool
split_uri_scheme (const String &uri, String *scheme, String *specificpart)
{ // parse URI scheme, see: http://tools.ietf.org/html/rfc3986
  size_t i = 0;
  // skip spaces before URI
  while (ISASCIIWHITESPACE (uri[i]))
    i++;
  const size_t sstart = i;
  // schemes start with an ALPHA
  if (ISALPHA (uri[i]))
    i++;
  else
    return false;
  // schemes contain ALPHA, DIGIT, or "+-."
  while (ISALNUM (uri[i]) || uri[i] == '+' || uri[i] == '-' || uri[i] == '.')
    i++;
  // schemes are always separated by a colon
  if (uri[i] == ':')
    i++;
  else
    return false;
  // actual splitting
  *scheme = string_tolower (uri.substr (sstart, i - sstart));
  *specificpart = uri.substr (i);
  return true;
}

Blob
Blob::load (const String &res_path)
{
  String scheme, specificpart;
  if (!split_uri_scheme (res_path, &scheme, &specificpart))
    { // fallback to file paths
      scheme = "file:";
      specificpart = res_path;
    }
  // blob from builtin resources
  const ResourceEntry *entry = scheme == "res:" ? ResourceEntry::find_entry (specificpart) : NULL;
  struct NoDelete { void operator() (const char*) {} }; // prevent delete on const data
  if (entry && (entry->dsize == entry->psize ||         // uint8[] array
                entry->dsize + 1 == entry->psize))      // string initilization with 0-termination
    {
      if (entry->dsize + 1 == entry->psize)
        assert (entry->pdata[entry->dsize] == 0);
      return Blob (std::make_shared<ByteBlob<NoDelete>> (res_path, entry->dsize, entry->pdata, NoDelete()));
    }
  else if (entry && entry->psize && entry->dsize == 0)  // variable length array with automatic size
    return Blob (std::make_shared<ByteBlob<NoDelete>> (res_path, entry->psize, entry->pdata, NoDelete()));
  // blob from compressed resources
  if (entry && entry->psize < entry->dsize)
    {
      const uint8 *u8data = zintern_decompress (entry->dsize, reinterpret_cast<const uint8*> (entry->pdata), entry->psize);
      const char *data = reinterpret_cast<const char*> (u8data);
      struct ZinternDeleter { void operator() (const char *d) { zintern_free ((uint8*) d); } };
      return Blob (std::make_shared<ByteBlob<ZinternDeleter>> (res_path, entry->dsize, data, ZinternDeleter()));
    }
  // blob from file
  errno = 0;
  const int fd = scheme == "file:" ? open (specificpart.c_str(), O_RDONLY | O_NOCTTY | O_CLOEXEC, 0) : -1;
  if (fd < 0)
    {
      DEBUG ("%s: open: %s", res_path.c_str(), strerror (errno));
      return Blob (std::shared_ptr<BlobResource> (nullptr));
    }
  struct stat sbuf = { 0, };
  size_t file_size = 0;
  if (fstat (fd, &sbuf) == 0 && sbuf.st_size)
    file_size = sbuf.st_size;
  // blob via mmap
  void *maddr;
  if (file_size >= 128 * 1024 &&
      MAP_FAILED != (maddr = mmap (NULL, file_size, PROT_READ, MAP_SHARED | MAP_DENYWRITE | MAP_POPULATE, fd, 0)))
    {
      close (fd); // mmap keeps its own file reference
      struct MunmapDeleter {
        const size_t length; MunmapDeleter (size_t l) : length (l) {}
        void operator() (const char *d) { munmap ((void*) d, length); }
      };
      return Blob (std::make_shared<ByteBlob<MunmapDeleter>> (res_path, file_size, (const char*) maddr, MunmapDeleter (file_size)));
    }
  // blob via read
  String iodata = string_read (res_path, fd, file_size);
  int e = errno;
  close (fd);
  errno = e;
  if (!errno)
    return Blob (std::make_shared<StringBlob> (res_path, iodata));
  // blob loading error
  e = errno;
  Blob errb = Blob (std::shared_ptr<BlobResource> (nullptr));
  errno = e ? e : ENOENT;
  return errb;
}

static constexpr uint64
consthash_fnv64a (const char *string, uint64 hash = 0xcbf29ce484222325)
{
  return string[0] == 0 ? hash : consthash_fnv64a (string + 1, 0x100000001b3 * (hash ^ string[0]));
}

Blob
Blob::from (const String &blob_string)
{
  String res_url = string_printf ("res:/.tmp/%016llx", consthash_fnv64a (blob_string.c_str()));
  return Blob (std::make_shared<StringBlob> (res_url, blob_string));
}

/**
 * @class Blob
 * Binary data access for builtin resources and files.
 * A Blob provides access to binary data (BLOB = Binary Large OBject) which can be
 * preassembled and compiled into a program or located in a resource path directory.
 * Locators for resources should generally adhere to the form: @code
 *      res: [relative_path/] resource_name
 * @endcode
 * See also: RAPICORN_STATIC_RESOURCE_DATA(), RAPICORN_STATIC_RESOURCE_ENTRY().
 * <BR> Example: @SNIPPET{rcore/tests/multitest.cc, Blob-EXAMPLE}
 */

} // Rapicorn
