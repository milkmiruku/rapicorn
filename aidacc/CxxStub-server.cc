
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; Rapicorn::Aida::fatal_error (__FILE__, __LINE__, errmsg); } while (0)
#endif

namespace { // Anon
using Rapicorn::Aida::uint64;

namespace __AIDA_Local__ {
using namespace Rapicorn::Aida;

// types
typedef ServerConnection::EmitResultHandler EmitResultHandler;
typedef ServerConnection::MethodRegistry    MethodRegistry;
typedef ServerConnection::MethodEntry       MethodEntry;

static_assert (std::is_base_of<Rapicorn::Aida::ImplicitBase, $AIDA_iface_base$>::value,
               "IDL interface base '$AIDA_iface_base$' must derive 'Rapicorn::Aida::ImplicitBase'");

// objects
static inline ProtoMsg*
new_call_result (ProtoReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Rapicorn::Aida::MSGID_CALL_RESULT, h, l, n);
}

static inline ProtoMsg*
new_connect_result (ProtoReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Rapicorn::Aida::MSGID_CONNECT_RESULT, h, l, n);
}

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
}

} } // Anon::__AIDA_Local__
