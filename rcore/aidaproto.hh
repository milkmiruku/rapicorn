// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
namespace Rapicorn { namespace Aida {

/// @ingroup AidaManifoldTypes
/// Unpack FieldBuffer as signal arguments and emit() the signal.
template<class R, class A1, class A2> static inline R
field_buffer_emit_signal (const Aida::FieldBuffer &fb,
                          const std::function<R (A1, A2)> &func)
{
  const size_t NARGS = 2;
  Aida::FieldReader fbr (fb);
  fbr.skip_msgid(); // FIXME: check msgid
  fbr.skip();       // skip handler_id
  if (AIDA_UNLIKELY (fbr.remaining() != NARGS))
    Aida::error_printf ("invalid number of signal arguments");
  typename ValueType<A1>::T a1; typename ValueType<A2>::T a2;
  fbr >>= a1; fbr >>= a2;
  return func (a1, a2);
}

} } // Rapicorn::Aida
