/* BirnetSignal
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* this file is used to generate birnetsignalvariants.hh by mksignals.sh.
 * therein, certain phrases like "typename A1, typename A2, typename A3" are
 * rewritten into 0, 1, 2, ... 16 argument variants. so make sure all phrases
 * involving the signal argument count match those from mksignals.sh.
 */

/* --- Emission --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3>
struct Emission3 : public EmissionBase {
  typedef Handler3<R0, A1, A2, A3>           Handler;
  typedef Handler4<R0, Emitter&, A1, A2, A3> HandlerE;
  Emitter *m_emitter;
  R0 m_result; A1 m_a1; A2 m_a2; A3 m_a3;
  SignalBase::Link *m_last_link;
  Emission3 (Emitter *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_result(), m_a1 (a1), m_a2 (a2), m_a3 (a3), m_last_link (NULL)
  {}
  /* call Handler and store result, so handler templates need no <void> specialization */
  R0 call (SignalBase::Link *link)
  {
    if (m_last_link != link)
      {
        if (link->with_emitter)
          {
            HandlerE *handler = handler_cast<HandlerE*> (link);
            if (handler->callable)
              m_result = (*handler) (*m_emitter, m_a1, m_a2, m_a3);
          }
        else
          {
            Handler *handler = handler_cast<Handler*> (link);
            if (handler->callable)
              m_result = (*handler) (m_a1, m_a2, m_a3);
          }
        m_last_link = link;
      }
    return m_result;
  }
};
template<class Emitter, typename A1, typename A2, typename A3>
struct Emission3 <Emitter, void, A1, A2, A3> : public EmissionBase {
  typedef Handler3<void, A1, A2, A3>           Handler;
  typedef Handler4<void, Emitter&, A1, A2, A3> HandlerE;
  Emitter *m_emitter;
  A1 m_a1; A2 m_a2; A3 m_a3;
  Emission3 (Emitter *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_a1 (a1), m_a2 (a2), m_a3 (a3)
  {}
  /* call the handler and ignore result, so handler templates need no <void> specialization */
  void call (SignalBase::Link *link)
  {
    if (link->with_emitter)
      {
        HandlerE *handler = handler_cast<HandlerE*> (link);
        if (handler->callable)
          (*handler) (*m_emitter, m_a1, m_a2, m_a3);
      }
    else
      {
        Handler *handler = handler_cast<Handler*> (link);
        if (handler->callable)
          (*handler) (m_a1, m_a2, m_a3);
      }
  }
};

/* --- SignalEmittable3 --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector>
struct SignalEmittable3 : SignalBase {
  typedef Emission3 <Emitter, R0, A1, A2, A3> Emission;
  typedef typename Collector::result_type     Result;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, Link *link) : SignalBase::Iterator<Emission> (emission, link) {}
    R0 operator* () { return this->emission.call (this->current); }
  };
  inline Result emit (A1 a1, A2 a2, A3 a3)
  {
    if (m_emitter)
      m_emitter->ref();
    Emission emission (m_emitter, a1, a2, a3);
    Iterator it (emission, &start), last (emission, &start);
    ++it; /* walk from start to first */
    Collector collector;
    Result result = collector (it, last);
    if (m_emitter)
      m_emitter->unref();
    return result;
  }
  explicit SignalEmittable3 (Emitter *emitter) : m_emitter (emitter) {}
private:
  Emitter *m_emitter;
};
/* SignalEmittable3 for void returns */
template<class Emitter, typename A1, typename A2, typename A3, class Collector>
struct SignalEmittable3 <Emitter, void, A1, A2, A3, Collector> : SignalBase {
  typedef Emission3 <Emitter, void, A1, A2, A3> Emission;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, Link *link) : SignalBase::Iterator<Emission> (emission, link) {}
    void operator* () { return this->emission.call (this->current); }
  };
  inline void emit (A1 a1, A2 a2, A3 a3)
  {
    if (m_emitter)
      m_emitter->ref();
    Emission emission (m_emitter, a1, a2, a3);
    Iterator it (emission, &start), last (emission, &start);
    ++it; /* walk from start to first */
    Collector collector;
    collector (it, last);
    if (m_emitter)
      m_emitter->unref();
  }
  explicit SignalEmittable3 (Emitter *emitter) : m_emitter (emitter) {}
private:
  Emitter *m_emitter;
};

/* --- Signal3 --- */
/* Signal* */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector = CollectorDefault<R0> >
struct Signal3 : SignalEmittable3<Emitter, R0, A1, A2, A3, Collector>
{
  typedef Emission3 <Emitter, R0, A1, A2, A3> Emission;
  typedef Slot3<R0, A1, A2, A3>               Slot;
  typedef Slot4<R0, Emitter&, A1, A2, A3>     SlotE;
  typedef typename SignalBase::Link           Link;
  typedef SignalEmittable3<Emitter, R0, A1, A2, A3, Collector> SignalEmittable;
  explicit Signal3 (Emitter &emitter) :
    SignalEmittable (&emitter)
  { assert (&emitter != NULL); }
  explicit Signal3 (Emitter &emitter, R0 (Emitter::*method) (A1, A2, A3)) :
    SignalEmittable (&emitter)
  {
    assert (&emitter != NULL);
    connect (slot (emitter, method));
  }
  inline void connect    (const Slot  &s) { connect_link (s.get_handler()); }
  inline void connect    (const SlotE &s) { connect_link (s.get_handler(), true); }
  Signal3&    operator+= (const Slot  &s) { connect (s); return *this; }
  Signal3&    operator+= (const SlotE &s) { connect (s); return *this; }
  Signal3&    operator+= (R0 (*callback) (A1, A2, A3))            { connect (slot (callback)); return *this; }
  Signal3&    operator+= (R0 (*callback) (Emitter&, A1, A2, A3))  { connect (slot (callback)); return *this; }
  BIRNET_PRIVATE_CLASS_COPY (Signal3);
};

/* --- Signal<> --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector>
struct Signal<Emitter, R0 (A1, A2, A3), Collector> : Signal3<Emitter, R0, A1, A2, A3, Collector>
{
  typedef Signal3<Emitter, R0, A1, A2, A3, Collector> Signal3;
  explicit Signal (Emitter &emitter) :
    Signal3 (emitter)
    {}
  explicit Signal (Emitter &emitter, R0 (Emitter::*method) (A1, A2, A3)) :
    Signal3 (emitter, method)
    {}
  BIRNET_PRIVATE_CLASS_COPY (Signal);
};

