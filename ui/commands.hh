// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <ui/item.hh> // unguarded, because item.hh includes commands.hh

#ifndef __RAPICORN_COMMANDS_HH__
#define __RAPICORN_COMMANDS_HH__

namespace Rapicorn {

struct Command : ReferenceCountable {
  const String ident;
  const String blurb;
  bool         needs_arg;
  Command (const char *cident, const char *cblurb, bool has_arg);
  virtual bool   exec (Deletable *obj, const StringSeq &args) = 0;
protected:
  virtual ~Command();
};

struct CommandList {
  uint      n_commands;
  Command **commands;
  CommandList () : n_commands (0), commands (NULL) {}
  template<typename Array>
  CommandList (Array             &a,
               const CommandList &chain = CommandList()) :
    n_commands (0),
    commands (NULL)
  {
    uint array_length = sizeof (a) / sizeof (a[0]);
    n_commands = array_length + chain.n_commands;
    commands = new Command*[n_commands];
    uint i;
    for (i = 0; i < array_length; i++)
      commands[i] = ref_sink (a[i]);
    for (; i < n_commands; i++)
      commands[i] = ref (chain.commands[i - array_length]);
  }
  ~CommandList()
  {
    for (uint i = 0; i < n_commands; i++)
      unref (commands[i]);
    delete[] commands;
  }
};

#define RAPICORN_MakeNamedCommand(Type, cident, blurb, method, data)   \
  create_command (&Type::method, cident, blurb, data)
#define RAPICORN_MakeSimpleCommand(Type, method, blurb)                 \
  create_command (&Type::method, #method, blurb)

/* --- command implementations --- */
/* command with data and arg string */
template<class Class, class Data>
struct CommandDataArg : Command {
  typedef bool (Class::*CommandMethod) (Data data, const StringSeq &args);
  bool (Class::*command_method) (Data data, const StringSeq &args);
  Data data;
  CommandDataArg (bool (Class::*method) (Data, const String&),
                  const char *cident, const char *cblurb, const Data &method_data);
  virtual bool exec (Deletable *obj, const StringSeq &args);
};
template<class Class, class Data> inline Command*
create_command (bool (Class::*method) (Data, const String&),
                const char *ident, const char *blurb, const Data &method_data)
{ return new CommandDataArg<Class,Data> (method, ident, blurb, method_data); }

/* command with data */
template<class Class, class Data>
struct CommandData : Command {
  typedef bool (Class::*CommandMethod) (Data data);
  bool (Class::*command_method) (Data data);
  Data data;
  CommandData (bool (Class::*method) (Data),
               const char *cident, const char *cblurb, const Data &method_data);
  virtual bool exec (Deletable *obj, const StringSeq&);
};
template<class Class, class Data> inline Command*
create_command (bool (Class::*method) (Data),
                const char *ident, const char *blurb, const Data &method_data)
{ return new CommandData<Class,Data> (method, ident, blurb, method_data); }

/* command with arg string */
template<class Class>
struct CommandArg: Command {
  typedef bool (Class::*CommandMethod) (const StringSeq &args);
  bool (Class::*command_method) (const StringSeq &args);
  CommandArg (bool (Class::*method) (const String&),
              const char *cident, const char *cblurb);
  virtual bool exec (Deletable *obj, const StringSeq &args);
};
template<class Class> inline Command*
create_command (bool (Class::*method) (const String&),
                const char *ident, const char *blurb)
{ return new CommandArg<Class> (method, ident, blurb); }

/* simple command */
template<class Class>
struct CommandSimple : Command {
  typedef bool (Class::*CommandMethod) ();
  bool (Class::*command_method) ();
  CommandSimple (bool (Class::*method) (),
                 const char *cident, const char *cblurb);
  virtual bool exec (Deletable *obj, const StringSeq&);
};
template<class Class> inline Command*
create_command (bool (Class::*method) (),
                const char *ident, const char *blurb)
{ return new CommandSimple<Class> (method, ident, blurb); }

/* --- implementations --- */
/* command with data and arg string */
template<class Class, typename Data>
CommandDataArg<Class,Data>::CommandDataArg (bool (Class::*method) (Data, const String&),
                                            const char *cident, const char *cblurb, const Data &method_data) :
  Command (cident, cblurb, true),
  command_method (method),
  data (method_data)
{}
template<class Class, typename Data> bool
CommandDataArg<Class,Data>::exec (Deletable *obj, const StringSeq &args)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) (data, args);
}

/* command arg string */
template<class Class>
CommandArg<Class>::CommandArg (bool (Class::*method) (const String&),
                               const char *cident, const char *cblurb) :
  Command (cident, cblurb, true),
  command_method (method)
{}
template<class Class> bool
CommandArg<Class>::exec (Deletable *obj, const StringSeq &args)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) (args);
}

/* command with data */
template<class Class, typename Data>
CommandData<Class,Data>::CommandData (bool (Class::*method) (Data),
                                      const char *cident, const char *cblurb, const Data &method_data) :
  Command (cident, cblurb, false),
  command_method (method),
  data (method_data)
{}
template<class Class, typename Data> bool
CommandData<Class,Data>::exec (Deletable *obj, const StringSeq&)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) (data);
}

/* simple command */
template<class Class>
CommandSimple<Class>::CommandSimple (bool (Class::*method) (),
                                     const char *cident, const char *cblurb) :
  Command (cident, cblurb, false),
  command_method (method)
{}
template<class Class> bool
CommandSimple<Class>::exec (Deletable *obj, const StringSeq&)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) ();
}

} // Rapicorn

#endif  /* __RAPICORN_COMMANDS_HH__ */
