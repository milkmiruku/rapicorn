// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_BLOBRES_HH__
#define __RAPICORN_BLOBRES_HH__

#include <rcore/utilities.hh>
#include <memory>

namespace Rapicorn {

// == Blob ==
class BlobResource;
class Blob {
  std::shared_ptr<BlobResource> m_blob;
  typedef size_t (Blob::*_UBool) () const;              // unspecified-type-boolean for non-numeric operator bool() result
  static _UBool _ubool1 ()     { return &Blob::size; }  // unspecified-type-boolean true value
  _UBool        _bool () const { return m_blob && size() ? _ubool1() : 0; }
  explicit     Blob   (const std::shared_ptr<BlobResource> &initblob);
public:
  explicit     Blob   ();                               ///< Default construct a NULL blob.
  String       name   () const;                         ///< Provide the name of this resource Blob.
  size_t       size   () const;                         ///< Retrieve the size of a Blob in bytes, this may be 0.
  const char*  data   () const;                         ///< Access the data of a Blob.
  const uint8* bytes  () const;                         ///< Access the data of a Blob.
  String       string () const;                         ///< Access data as string, strips trailing 0s.
  operator     _UBool () const { return _bool(); }      ///< Checks if the blob contains accessible data.
  static Blob  load   (const String &res_path);         ///< Load Blob at @a res_path, sets errno on error.
  static Blob  from   (const String &blob_string);      ///< Create a Blob containing @a blob_string.
};

// == Resource Macros ==

/// Statically declare a ResourceBlob data variable.
#define RAPICORN_STATIC_RESOURCE_DATA(IDENT)            \
  static const char __Rapicorn_static_resourceD__##IDENT[] __attribute__ ((__aligned__ (2 * sizeof (size_t))))

/// Statically register a ResourceBlob entry, referring a previously declared RAPICORN_STATIC_RESOURCE_DATA(IDENT) variable.
#define RAPICORN_STATIC_RESOURCE_ENTRY(IDENT, PATH, ...) \
  static const Rapicorn::ResourceEntry __Rapicorn_static_resourceE__##IDENT = { PATH, __Rapicorn_static_resourceD__##IDENT, __VA_ARGS__ };

// == Internals ==
///@cond
class ResourceEntry {
  ResourceEntry *next;
  const char    *const name;
  const char    *const pdata;
  const size_t   psize, dsize;
  friend         class Blob;
  static const ResourceEntry* find_entry (const String&);
  static void                 reg_add    (ResourceEntry*);
public:
  template <size_t N> ResourceEntry (const char *res, const char (&idata) [N], size_t data_size = 0) :
    next (NULL), name (res), pdata (idata), psize (N), dsize (data_size)
  { reg_add (this); }
  /*dtor*/           ~ResourceEntry();
};
///@endcond

} // Rapicorn

#endif // __RAPICORN_BLOBRES_HH__
