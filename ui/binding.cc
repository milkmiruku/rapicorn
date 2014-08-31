// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "binding.hh"
#include "application.hh"
#include <algorithm>

namespace Rapicorn {

// == BindableRelayImpl ==
BindableRelayImpl::Request::Request (Type t, uint64 n, const String &p, const Any &a) :
  type (t), nonce (n), path (p), any (a)
{
  assert_return (nonce > 0);
  assert_return (path.empty() == false);
}

BindableRelayImpl::BindableRelayImpl ()
{}

BindableRelayImpl::~BindableRelayImpl ()
{}

void
BindableRelayImpl::bindable_set (const BindablePath &bpath, const Any &any)
{
  const Request req (Request::SET, random_nonce(), bpath.path);
  requests_.push_back (req);
  sig_relay_set.emit (req.path, req.nonce, any);
}

void
BindableRelayImpl::bindable_get (const BindablePath &bpath, Any &any)
{
  auto it = std::find_if (requests_.begin(), requests_.end(),
                          [&bpath] (const Request &req) {
                            return req.type == Request::CACHE && req.path == bpath.path;
                          });
  if (it != requests_.end()) // result provided from previous property read out
    {
      Request &req = *it;
      any = req.any;
      requests_.erase (it);
      return;
    }
  const Request req (Request::GET, random_nonce(), bpath.path);
  requests_.push_back (req);
  sig_relay_get.emit (req.path, req.nonce);
  // we're not immediately returning a value,
  // once the remote result is in, we'll notify about change
}

void
BindableRelayImpl::report_result (int64 nonce, const Any &result, const String &error)
{
  auto it = std::find_if (requests_.begin(), requests_.end(), [nonce] (const Request &req) { return req.nonce == uint64 (nonce); });
  if (it == requests_.end())
    {
      RAPICORN_DIAG ("Rapicorn::BindableRelayImpl::report_result: discarding unsolicited result (nonce=0x%016x)", nonce);
      return;
    }
  Request &req = *it;
  if (req.type == Request::GET && error.empty())
    {
      // mutate this request into a cached value for a future bindable_get call
      req.type = Request::CACHE;
      req.nonce = 0;
      req.any = result;
      // cause another bindable_get call to fetch this new value
      bindable_notify (req.path);
      return; // preserve request
    }
  if (!error.empty())
    critical ("BindableRelayImpl: error from remote: %s", error.empty() ? "unknown" : error);
  requests_.erase (it);
}

BindableRelayIface*
ApplicationImpl::create_bindable_relay ()
{
  struct BindableRelayImpl : public Rapicorn::BindableRelayImpl {};
  BindableRelayImpl *brelay = new BindableRelayImpl();
  return ref_sink (brelay); // @TODO: FIXME: reference leak, needs IDL codegen fixes
}

// == Binding ==
Binding::Binding (ObjectIface &instance, const String &instance_property, const String &binding_path) :
  instance_ (instance), instance_property_ (instance_property), binding_path_ (binding_path)
{}

Binding::~Binding ()
{}

void
Binding::bind_context (ObjectIfaceP object)
{
  BindableIfaceP binding_context = std::dynamic_pointer_cast<BindableIface> (object);
  if (binding_context.get() != binding_context_.get())
    {
      if (binding_context_)
        reset();
      binding_context_ = binding_context;
      if (binding_context_)
        {
          bindable_to_object();
          binding_sigid_ = binding_context_->sig_bindable_notify() += slot (this, &Binding::bindable_notify);
        }
    }
}

void
Binding::bindable_notify (const String &property)
{
  if (property == binding_path_)
    bindable_to_object();
}

void
Binding::bindable_to_object ()
{
  BindablePath bpath { binding_path_ };
  Any a;
  binding_context_->bindable_get (bpath, a);
  if (a.kind())
    {
      struct ObjectIface : Rapicorn::ObjectIface { using Rapicorn::ObjectIface::__aida_setter__; };
      ((ObjectIface*) &instance_)->__aida_setter__ (instance_property_, a.as_string());
    }
}

void
Binding::object_to_bindable ()
{
  struct ObjectIface : Rapicorn::ObjectIface { using Rapicorn::ObjectIface::__aida_getter__; };
  Any a;
  String stringvalue = ((ObjectIface*) &instance_)->__aida_getter__ (instance_property_);
  a <<= stringvalue; // FIXME: Aida needs Any setters/getters
  BindablePath bpath { binding_path_ };
  binding_context_->bindable_set (bpath, a);
}

void
Binding::reset()
{
  binding_context_->sig_bindable_notify() -= binding_sigid_;
  binding_sigid_ = 0;
  binding_context_.reset();
}

} // Rapicorn
