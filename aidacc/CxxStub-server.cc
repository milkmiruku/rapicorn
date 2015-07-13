
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

// connection
static Rapicorn::Aida::ServerConnection *server_connection = NULL;

// EmitResultHandler
static inline void erhandler_add (size_t id, const EmitResultHandler &function)
{
  return server_connection->emit_result_handler_add (id, function);
}

// objects
template<class Target> static inline void
proto_msg_add_interface (Rapicorn::Aida::ProtoMsg &fb, Target *instance)
{
  server_connection->add_interface (fb, instance ? instance->shared_from_this() : ImplicitBaseP());
}

template<class Target> static inline std::shared_ptr<Target>
field_reader_pop_interface (Rapicorn::Aida::FieldReader &fr)
{
  return std::dynamic_pointer_cast<Target> (server_connection->pop_interface (fr));
}

// messages
static inline void
post_msg (ProtoMsg *fb)
{
  ObjectBroker::post_msg (fb);
}

static inline void
add_header1_discon (ProtoMsg &fb, size_t signal_handler_id, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_DISCONNECT, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id), h, l);
}

static inline void
add_header1_emit (ProtoMsg &fb, size_t signal_handler_id, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_EMIT_ONEWAY, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id), h, l);
}

static inline void
add_header2_emit (ProtoMsg &fb, size_t signal_handler_id, uint64 h, uint64 l)
{
  fb.add_header2 (Rapicorn::Aida::MSGID_EMIT_TWOWAY, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id),
                  server_connection->connection_id(), h, l);
}

static inline ProtoMsg*
new_call_result (FieldReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Rapicorn::Aida::MSGID_CALL_RESULT, ObjectBroker::sender_connection_id (fbr.proto_msg()->first_id()), h, l, n);
}

static inline ProtoMsg*
new_connect_result (FieldReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Rapicorn::Aida::MSGID_CONNECT_RESULT, ObjectBroker::sender_connection_id (fbr.proto_msg()->first_id()), h, l, n);
}

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
}

} } // Anon::__AIDA_Local__

namespace Rapicorn { namespace Aida {
#ifndef RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
#define RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
// namespace Rapicorn::Aida needed for argument dependent lookups of the operators
static void operator<<= (Rapicorn::Aida::ProtoMsg &fb, const Rapicorn::Aida::Any &v) __attribute__ ((unused));
static void operator>>= (Rapicorn::Aida::FieldReader &fr, Rapicorn::Aida::Any &v) __attribute__ ((unused));
static void
operator<<= (Rapicorn::Aida::ProtoMsg &fb, const Rapicorn::Any &v)
{
  fb.add_any (v, *__AIDA_Local__::server_connection);
}
static void
operator>>= (Rapicorn::Aida::FieldReader &fr, Rapicorn::Any &v)
{
  v = fr.pop_any (*__AIDA_Local__::server_connection);
}
#endif // RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
} } // Rapicorn::Aida
