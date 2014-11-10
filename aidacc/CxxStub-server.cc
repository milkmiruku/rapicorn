
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; Rapicorn::Aida::fatal_error (std::string ("AIDA-ERROR: ") + errmsg); } while (0)
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
static Rapicorn::Init init_server_connection ([]() {
  server_connection = ObjectBroker::new_server_connection ($AIDA_server_feature_keys$);
});

// EmitResultHandler
static inline void erhandler_add (size_t id, const EmitResultHandler &function)
{
  return server_connection->emit_result_handler_add (id, function);
}

// objects
template<class Target> static inline Target*
remote_handle_to_interface (const RemoteHandle &remote)
{
  Rapicorn::Aida::ImplicitBase *instance = server_connection->interface_from_handle (remote).get();
  return dynamic_cast<Target*> (instance);
}

template<class SMH> static inline SMH
interface_to_remote_handle ($AIDA_iface_base$ *ibase)
{
  SMH target;
  struct CastingServerConnection : ServerConnection { using ServerConnection::cast_interface_handle; };
  CastingServerConnection *cs_con = (CastingServerConnection*) server_connection;
  cs_con->cast_interface_handle (target, ibase ? ibase->shared_from_this() : ImplicitBaseP());
  return target;
}

template<class Target> static inline void
field_buffer_add_interface (Rapicorn::Aida::FieldBuffer &fb, Target *instane)
{
  server_connection->add_interface (fb, instane ? instane->shared_from_this() : ImplicitBaseP());
}

template<class Target> static inline Target*
field_reader_pop_interface (Rapicorn::Aida::FieldReader &fr)
{
  return dynamic_cast<Target*> (server_connection->pop_interface (fr).get());
}

// messages
static inline void
post_msg (FieldBuffer *fb)
{
  ObjectBroker::post_msg (fb);
}

static inline void
add_header1_discon (FieldBuffer &fb, size_t signal_handler_id, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_DISCONNECT, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id), h, l);
}

static inline void
add_header1_emit (FieldBuffer &fb, size_t signal_handler_id, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_EMIT_ONEWAY, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id), h, l);
}

static inline void
add_header2_emit (FieldBuffer &fb, size_t signal_handler_id, uint64 h, uint64 l)
{
  fb.add_header2 (Rapicorn::Aida::MSGID_EMIT_TWOWAY, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id),
                  server_connection->connection_id(), h, l);
}

static inline FieldBuffer*
new_call_result (FieldReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return FieldBuffer::renew_into_result (fbr, Rapicorn::Aida::MSGID_CALL_RESULT, ObjectBroker::sender_connection_id (fbr.field_buffer()->first_id()), h, l, n);
}

static inline FieldBuffer*
new_connect_result (FieldReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return FieldBuffer::renew_into_result (fbr, Rapicorn::Aida::MSGID_CONNECT_RESULT, ObjectBroker::sender_connection_id (fbr.field_buffer()->first_id()), h, l, n);
}

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
}

} } // Anon::__AIDA_Local__

namespace Rapicorn { namespace Aida {
// namespace Rapicorn::Aida needed for argument dependent lookups of the operators
static void operator<<= (Rapicorn::Aida::FieldBuffer &fb, const Rapicorn::Aida::Any &v);
static void operator>>= (Rapicorn::Aida::FieldReader &fr, Rapicorn::Aida::Any &v);
static void
operator<<= (Rapicorn::Aida::FieldBuffer &fb, const Rapicorn::Any &v)
{
  fb.add_any (v, *__AIDA_Local__::server_connection);
}
static void
operator>>= (Rapicorn::Aida::FieldReader &fr, Rapicorn::Any &v)
{
  v = fr.pop_any (*__AIDA_Local__::server_connection);
}
} } // Rapicorn::Aida
