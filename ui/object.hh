// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_OBJECT_HH_
#define __RAPICORN_OBJECT_HH_

#include <ui/serverapi.hh>

namespace Rapicorn {

// == Forward Declarations ==
typedef std::shared_ptr<ObjectIface> ObjectIfaceP;

/// ObjectImpl is the base type for all server side objects in Rapicorn and implements the IDL base type ObjectIface.
class ObjectImpl : public virtual ObjectIface, public virtual DataListContainer {
protected:
  virtual /*dtor*/                 ~ObjectImpl    () override;
  virtual void                      do_changed    (const String &name);
  class                    InterfaceMatcher;
  template<class C>  class InterfaceMatch;
public:
  explicit                          ObjectImpl    ();
  Signal<void (const String &name)> sig_changed;
  void                              changed       (const String &name);
};
inline bool operator== (const ObjectImpl &object1, const ObjectImpl &object2) { return &object1 == &object2; }
inline bool operator!= (const ObjectImpl &object1, const ObjectImpl &object2) { return &object1 != &object2; }

struct ObjectImpl::InterfaceMatcher {
  explicit      InterfaceMatcher (const String &ident) : ident_ (ident), match_found_ (false) {}
  bool          done             () const { return match_found_; }
  virtual  bool match            (ObjectImpl *object, const String &ident = String()) = 0;
  RAPICORN_CLASS_NON_COPYABLE (InterfaceMatcher);
protected:
  const String &ident_;
  bool          match_found_;
};

template<class C>
struct ObjectImpl::InterfaceMatch : ObjectImpl::InterfaceMatcher {
  typedef C&    Result;
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatcher (ident), instance_ (NULL) {}
  C&            result          (bool may_throw) const;
  virtual bool  match           (ObjectImpl *obj, const String &ident);
protected:
  C            *instance_;
};
template<class C>
struct ObjectImpl::InterfaceMatch<C&> : InterfaceMatch<C> {
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatch<C> (ident) {}
};
template<class C>
struct ObjectImpl::InterfaceMatch<C*> : InterfaceMatch<C> {
  typedef C*    Result;
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatch<C> (ident) {}
  C*            result          (bool may_throw) const { return InterfaceMatch<C>::instance_; }
};

template<class C> bool
ObjectImpl::InterfaceMatch<C>::match (ObjectImpl *obj, const String &ident)
{
  if (!instance_)
    {
      const String &id = ident_;
      if (id.empty() || id == ident)
        {
          instance_ = dynamic_cast<C*> (obj);
          match_found_ = instance_ != NULL;
        }
    }
  return match_found_;
}

template<class C> C&
ObjectImpl::InterfaceMatch<C>::result (bool may_throw) const
{
  if (!this->instance_ && may_throw)
    throw NullInterface();
  return *this->instance_;
}

} // Rapicorn

#endif  /* __RAPICORN_OBJECT_HH_ */
