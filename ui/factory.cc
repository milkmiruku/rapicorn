// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "factory.hh"
#include "evaluator.hh"
#include "window.hh"
#include <stdio.h>
#include <stack>
#include <cstring>
#include <algorithm>

#define FDEBUG(...)     RAPICORN_KEY_DEBUG ("Factory", __VA_ARGS__)
#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("Factory-Eval", __VA_ARGS__)

namespace Rapicorn {
namespace Factory {

// == Utilities ==
static String
node_location (const XmlNode *xnode)
{
  return string_format ("%s:%d", xnode->parsed_file().c_str(), xnode->parsed_line());
}

static String
node_location (const XmlNodeP xnode)
{
  return node_location (xnode.get());
}

// == InterfaceFile ==
struct InterfaceFile : public virtual std::enable_shared_from_this<InterfaceFile> {
  const String   file_name;
  const XmlNodeP root; // <interfaces/>
  explicit InterfaceFile (const String &f, const XmlNodeP r) : file_name (f), root (r) {}
};
typedef std::shared_ptr<InterfaceFile> InterfaceFileP;

static std::vector<InterfaceFileP> interface_file_list;

static String
register_interface_file (String file_name, const XmlNodeP root, vector<String> *definitions)
{
  assert_return (file_name.empty() == false, "missing file");
  assert_return (root->name() == "interfaces", "invalid file");
  InterfaceFileP ifile = std::make_shared<InterfaceFile> (file_name, root);
  const size_t reset_size = definitions ? definitions->size() : 0;
  for (auto dnode : root->children())
    if (dnode->istext() == false)
      {
        const String id = dnode->get_attribute ("id");
        if (id.empty())
          {
            if (definitions)
              definitions->resize (reset_size);
            return string_format ("%s: interface definition without id", node_location (dnode));
          }
        if (definitions)
          definitions->push_back (id);
      }
  interface_file_list.insert (interface_file_list.begin(), ifile);
  FDEBUG ("%s: registering %d interfaces\n", file_name, root->children().size());
  return "";
}

static const XmlNode*
lookup_interface_node (const String &identifier, const XmlNode *context_node)
{
  for (auto ifile : interface_file_list)
    for (auto node : ifile->root->children())
      {
        const String id = node->get_attribute ("id");
        if (id == identifier)
          return node.get();
      }
  return NULL;
}

static bool
check_interface_node (const XmlNode &xnode)
{
  const XmlNode *parent = xnode.parent();
  return parent && !parent->parent() && parent->name() == "interfaces";
}

} // Factory


// == FactoryContext ==
struct FactoryContext {
  const XmlNode *xnode;
  StringSeq     *type_tags;
  String         type;
  FactoryContext (const XmlNode *xn) : xnode (xn), type_tags (NULL) {}
};
static std::map<const XmlNode*, FactoryContext*> factory_context_map; /// @TODO: Make factory_context_map threadsafe

static void initialize_factory_lazily (void);

namespace Factory {

// == WidgetTypeFactory ==
static std::list<const WidgetTypeFactory*>&
widget_type_list()
{
  static std::list<const WidgetTypeFactory*> *widget_type_factories_p = NULL;
  do_once
    {
      widget_type_factories_p = new std::list<const WidgetTypeFactory*>();
    }
  return *widget_type_factories_p;
}

static const WidgetTypeFactory*
lookup_widget_factory (String namespaced_ident)
{
  std::list<const WidgetTypeFactory*> &widget_type_factories = widget_type_list();
  namespaced_ident = namespaced_ident;
  std::list<const WidgetTypeFactory*>::iterator it;
  for (it = widget_type_factories.begin(); it != widget_type_factories.end(); it++)
    if ((*it)->qualified_type == namespaced_ident)
      return *it;
  return NULL;
}

void
WidgetTypeFactory::register_widget_factory (const WidgetTypeFactory &itfactory)
{
  std::list<const WidgetTypeFactory*> &widget_type_factories = widget_type_list();
  const char *ident = itfactory.qualified_type.c_str();
  const char *base = strrchr (ident, ':');
  if (!base || strncmp (ident, "Rapicorn_Factory", base - ident) != 0)
    fatal ("WidgetTypeFactory registration with invalid/missing domain name: %s", ident);
  String domain_name;
  domain_name.assign (ident, base - ident - 1);
  widget_type_factories.push_back (&itfactory);
}

WidgetTypeFactory::WidgetTypeFactory (const char *namespaced_ident) :
  qualified_type (namespaced_ident)
{}

WidgetTypeFactory::~WidgetTypeFactory ()
{}

void
WidgetTypeFactory::sanity_check_identifier (const char *namespaced_ident)
{
  if (strncmp (namespaced_ident, "Rapicorn_Factory:", 17) != 0)
    fatal ("WidgetTypeFactory: identifier lacks factory qualification: %s", namespaced_ident);
}

// == Public factory_context API ==
String
factory_context_name (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  const XmlNode &xnode = *fc->xnode;
  if (check_interface_node (xnode))
    return xnode.get_attribute ("id");
  else
    return xnode.name();
}

String
factory_context_type (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  const XmlNode *xnode = fc->xnode;
  if (!check_interface_node (*xnode)) // lookup definition node from child node
    {
      xnode = lookup_interface_node (xnode->name(), xnode);
      assert_return (xnode != NULL, "");
    }
  assert_return (check_interface_node (*xnode), "");
  return xnode->get_attribute ("id");
}

UserSource
factory_context_source (FactoryContext *fc)
{
  assert_return (fc != NULL, UserSource (""));
  const XmlNode *xnode = fc->xnode;
  if (!check_interface_node (*xnode)) // lookup definition node from child node
    {
      xnode = lookup_interface_node (xnode->name(), xnode);
      assert_return (xnode != NULL, UserSource (""));
    }
  assert_return (check_interface_node (*xnode), UserSource (""));
  return UserSource ("WidgetFactory", xnode->parsed_file(), xnode->parsed_line());
}

static void
factory_context_list_types (StringVector &types, const XmlNode *xnode, const bool need_ids, const bool need_variants)
{
  assert_return (xnode != NULL);
  if (!check_interface_node (*xnode)) // lookup definition node from child node
    {
      xnode = lookup_interface_node (xnode->name(), xnode);
      assert_return (xnode != NULL);
    }
  while (xnode)
    {
      assert_return (check_interface_node (*xnode));
      if (need_ids)
        types.push_back (xnode->get_attribute ("id"));
      const String parent_name = xnode->name();
      const XmlNode *last = xnode;
      xnode = lookup_interface_node (parent_name, xnode);
      if (!xnode && last->name() == "Rapicorn_Factory")
        {
          const StringVector &attributes_names = last->list_attributes(), &attributes_values = last->list_values();
          const WidgetTypeFactory *widget_factory = NULL;
          for (size_t i = 0; i < attributes_names.size(); i++)
            if (attributes_names[i] == "factory-type" || attributes_names[i] == "factory_type")
              {
                widget_factory = lookup_widget_factory (attributes_values[i]);
                break;
              }
          assert_return (widget_factory != NULL);
          types.push_back (widget_factory->type_name());
          std::vector<const char*> variants;
          widget_factory->type_name_list (variants);
          for (auto n : variants)
            if (n)
              types.push_back (n);
        }
    }
}

static void
factory_context_update_cache (FactoryContext *fc)
{
  if (UNLIKELY (!fc->type_tags))
    {
      const XmlNode *xnode = fc->xnode;
      fc->type_tags = new StringSeq;
      StringVector &types = *fc->type_tags;
      factory_context_list_types (types, xnode, false, false);
      fc->type = types.size() ? types[types.size() - 1] : "";
      types.clear();
      factory_context_list_types (types, xnode, true, true);
    }
}

const StringSeq&
factory_context_tags (FactoryContext *fc)
{
  assert_return (fc != NULL, *(StringSeq*) NULL);
  factory_context_update_cache (fc);
  return *fc->type_tags;
}

String
factory_context_impl_type (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  factory_context_update_cache (fc);
  return fc->type;
}

// == Builder ==
class Builder {
  enum Flags { SCOPE_CHILD = 0, SCOPE_WIDGET = 1 };
  const XmlNode   *const dnode_;               // definition of gadget to be created
  String           child_container_name_;
  ContainerImpl   *child_container_;           // captured child_container_ widget during build phase
  VariableMap      locals_;
  void      eval_args       (const StringVector &in_names, const StringVector &in_values, const XmlNode *errnode,
                             StringVector &out_names, StringVector &out_values, String *child_container_name);
  bool      try_set_property(WidgetImpl &widget, const String &property_name, const String &value);
  WidgetImplP build_scope   (const StringVector &caller_arg_names, const StringVector &caller_arg_values, const String &caller_location,
                             const XmlNode *factory_context_node);
  WidgetImplP build_widget  (const XmlNode *node, const XmlNode *factory_context_node, Flags bflags);
  static String canonify_dashes (const String &key);
public:
  explicit  Builder             (const String &widget_identifier, const XmlNode *context_node);
  static WidgetImplP eval_and_build     (const String &widget_identifier,
                                         const StringVector &call_names, const StringVector &call_values, const String &call_location);
  static WidgetImplP build_from_factory (const XmlNode *factory_node,
                                         const StringVector &attr_names, const StringVector &attr_values,
                                         const XmlNode *factory_context_node);
  static bool widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier);
};

Builder::Builder (const String &widget_identifier, const XmlNode *context_node) :
  dnode_ (lookup_interface_node (widget_identifier, context_node)), child_container_ (NULL)
{
  if (!dnode_)
    return;
}

String
Builder::canonify_dashes (const String &key)
{
  String s = key;
  for (uint i = 0; i < s.size(); i++)
    if (key[i] == '-')
      s[i] = '_'; // unshares string
  return s;
}

static bool
is_property (const XmlNode &xnode, String *element = NULL, String *attribute = NULL)
{
  // parse property element syntax, e.g. <Button.label>...</Button.label>
  const String &pname = xnode.name();
  const size_t i = pname.rfind (".");
  if (i == String::npos || i + 1 >= pname.size())
    return false;                                       // no match
  if (element)
    *element = pname.substr (0, i);
  if (attribute)
    *attribute = pname.substr (i + 1);
  return true;                                          // matched element.attribute
}

WidgetImplP
Builder::eval_and_build (const String &widget_identifier,
                         const StringVector &call_names, const StringVector &call_values, const String &call_location)
{
  assert_return (call_names.size() == call_values.size(), NULL);
  // initialize and check builder
  initialize_factory_lazily();
  Builder builder (widget_identifier, NULL);
  if (!builder.dnode_)
    {
      critical ("%s: unknown type identifier: %s", call_location, widget_identifier);
      return NULL;
    }
  // evaluate call arguments
  Evaluator env;
  StringVector ecall_names, ecall_values;
  for (size_t i = 0; i < call_names.size(); i++)
    {
      const String &cname = call_names[i];
      String cvalue = call_values[i];
      if (cname.find (':') == String::npos && // never eval namespaced attributes
          string_startswith (cvalue, "@eval "))
        cvalue = env.parse_eval (cvalue.substr (6));
      ecall_names.push_back (cname);
      ecall_values.push_back (cvalue);
    }
  // build widget
  WidgetImplP widget = builder.build_scope (call_names, call_values, call_location, builder.dnode_);
  FDEBUG ("%s: built widget '%s': %s", node_location (builder.dnode_), widget_identifier, widget ? widget->name() : "<null>");
  return widget;
}

WidgetImplP
Builder::build_from_factory (const XmlNode *factory_node,
                             const StringVector &attr_names, const StringVector &attr_values,
                             const XmlNode *factory_context_node)
{
  assert_return (factory_context_node != NULL, NULL);
  // extract arguments
  String factory_id, factory_type;
  for (size_t i = 0; i < attr_names.size(); i++)
    if (attr_names[i] == "id")
      factory_id = attr_values[i];
    else if (attr_names[i] == "factory_type")
      factory_type = attr_values[i];
  // lookup widget factory
  const WidgetTypeFactory *widget_factory = lookup_widget_factory (factory_type);
  // sanity check factory node
  if (factory_node->name() != "Rapicorn_Factory" || !widget_factory ||
      attr_names.size() != 2 || factory_node->list_attributes().size() != 2 ||
      factory_id.empty() || factory_id[0] == '@' || factory_type.empty() || factory_type[0] == '@' ||
      factory_node->children().size() != 0)
    {
      critical ("%s: invalid factory type: %s", node_location (factory_node), factory_node->name());
      return NULL;
    }
  // build factory widget
  FactoryContext *fc = factory_context_map[factory_context_node];
  if (!fc)
    {
      fc = new FactoryContext (factory_context_node);
      factory_context_map[factory_context_node] = fc;
    }
  WidgetImplP widget = widget_factory->create_widget (fc);
  if (widget)
    widget->name (factory_id);
  return widget;
}

void
Builder::eval_args (const StringVector &in_names, const StringVector &in_values, const XmlNode *errnode,
                    StringVector &out_names, StringVector &out_values, String *child_container_name)
{
  Evaluator env;
  env.push_map (locals_);
  out_names.reserve (in_names.size());
  out_values.reserve (in_values.size());
  for (size_t i = 0; i < in_names.size(); i++)
    {
      const String cname = canonify_dashes (in_names[i]);
      const String &ivalue = in_values[i];
      String rvalue;
      if (string_startswith (ivalue, "@eval "))
        {
          rvalue = env.parse_eval (ivalue.substr (6));
          EDEBUG ("%s: eval %s=\"%s\": %s", String (errnode ? node_location (errnode) : "Rapicorn:Factory"),
                  in_names[i].c_str(), ivalue.c_str(), rvalue.c_str());
        }
      else
        rvalue = ivalue;
      if (child_container_name && cname == "child_container")
        *child_container_name = rvalue;
      else
        {
          out_names.push_back (cname);
          out_values.push_back (rvalue);
        }
    }
  env.pop_map (locals_);
}

bool
Builder::try_set_property (WidgetImpl &widget, const String &property_name, const String &value)
{
  if (value[0] == '@')
    {
      if (strncmp (value.data(), "@bind ", 6) == 0) /// @TODO: FIXME: implement proper @-string parsing
        {
          widget.add_binding (property_name, value.data() + 6);
          return true;
        }
    }
  return widget.try_set_property (property_name, value);
}

WidgetImplP
Builder::build_scope (const StringVector &caller_arg_names, const StringVector &caller_arg_values, const String &caller_location,
                      const XmlNode *factory_context_node)
{
  assert_return (dnode_ != NULL, NULL);
  assert_return (factory_context_node != NULL, NULL);
  // create environment for @eval
  Evaluator env;
  String name_argument; // target name for this scope widget
  // extract Argument defaults from definition
  StringVector argument_names, argument_values;
  for (const XmlNodeP cnode : dnode_->children())
    if (cnode->name() == "Argument")
      {
        const String aname = canonify_dashes (cnode->get_attribute ("id")); // canonify argument id
        if (aname.empty() || aname == "id" || aname == "name")
          critical ("%s: invalid argument id: ", node_location (cnode), aname.empty() ? "<missing>" : aname);
        else
          {
            argument_names.push_back (aname);
            String avalue = cnode->get_attribute ("default");
            if (string_startswith (avalue, "@eval "))
              avalue = env.parse_eval (avalue.substr (6));
            argument_values.push_back (avalue);
          }
      }
  // assign Argument values from caller args, collect caller properties
  StringVector caller_property_names, caller_property_values;
  for (size_t i = 0; i < caller_arg_names.size(); i++)
    {
      const String cname = canonify_dashes (caller_arg_names[i]), &cvalue = caller_arg_values.at (i);
      if (cname == "name" || cname == "id")
        {
          name_argument = cvalue;
          continue;
        }
      else if (cname.find (':') != String::npos)
        continue; // ignore namespaced attributes
      vector<String>::const_iterator it = find (argument_names.begin(), argument_names.end(), cname);
      if (it != argument_names.end())
        argument_values[it - argument_names.begin()] = cvalue;
      else
        {
          caller_property_names.push_back (cname);
          caller_property_values.push_back (cvalue);
        }
    }
  // use Argument values to prepare variable map for evaluator
  Evaluator::populate_map (locals_, argument_names, argument_values);
  argument_names.clear(), argument_values.clear();
  // build main definition widget
  WidgetImplP widget = build_widget (dnode_, factory_context_node, SCOPE_WIDGET);
  if (!widget)
    return NULL;
  // assign caller properties
  if (!name_argument.empty())
    widget->name (name_argument);
  for (size_t i = 0; i < caller_property_names.size(); i++)
    {
      const String &cname = caller_property_names[i], &cvalue = caller_property_values[i];
      if (!try_set_property (*widget, cname, cvalue))
        critical ("%s: widget %s: unknown property: %s", caller_location, widget->name(), cname);
    }
  // assign child container
  if (child_container_)
    {
      ContainerImpl *container = widget->as_container_impl();
      if (!container)
        critical ("%s: invalid type for child container: %s", node_location (dnode_), dnode_->name());
      else
        container->child_container (child_container_);
    }
  else if (!child_container_name_.empty())
    critical ("%s: failed to find child container: %s", node_location (dnode_), child_container_name_);
  return widget;
}

WidgetImplP
Builder::build_widget (const XmlNode *const wnode, const XmlNode *const factory_context_node, const Flags bflags)
{
  const bool skip_argument_child = bflags & SCOPE_WIDGET;
  const bool filter_child_container = bflags & SCOPE_WIDGET;
  // collect properties from XML attributes
  StringVector prop_names = wnode->list_attributes(), prop_values = wnode->list_values();
  // collect properties from XML property element syntax
  bool skip_child[std::max (size_t (1), wnode->children().size())] = { 0, };
  size_t ski = 0; // skip child index
  for (const XmlNodeP cnode : wnode->children())
    {
      String prop_object, pname;
      if (is_property (*cnode, &prop_object, &pname) && wnode->name() == prop_object)
        {
          pname = canonify_dashes (pname);
          prop_names.push_back (pname);
          prop_values.push_back (cnode->xml_string (0, false));
          skip_child[ski++] = true;
        }
      else if (cnode->istext() || (skip_argument_child && cnode->name() == "Argument"))
        skip_child[ski++] = true;
      else
        skip_child[ski++] = false;
    }
  assert_return (ski == wnode->children().size(), NULL);
  // evaluate property values
  StringVector eprop_names, eprop_values;
  eval_args (prop_names, prop_values, wnode, eprop_names, eprop_values, filter_child_container ? &child_container_name_ : NULL);
  // create widget and assign properties from attributes and property element syntax
  WidgetImplP widget;
  {
    Builder inner_builder (wnode->name(), NULL);
    if (inner_builder.dnode_)
      widget = inner_builder.build_scope (eprop_names, eprop_values, node_location (wnode), factory_context_node);
    else
      widget = build_from_factory (wnode, eprop_names, eprop_values, factory_context_node);
  }
  if (!widget)
    {
      critical ("%s: failed to create widget: %s", node_location (wnode), wnode->name());
      return NULL;
    }
  ContainerImpl *container = NULL;
  // find child container within scope
  if (!child_container_name_.empty() && child_container_name_ == widget->name())
    {
      ContainerImpl *cc = widget->as_container_impl();
      if (cc)
        {
          if (child_container_)
            critical ("%s: duplicate child container: %s", node_location (dnode_), node_location (wnode));
          else
            child_container_ = cc;
        }
      else
        critical ("%s: invalid child container type: %s", node_location (dnode_), node_location (wnode));
    }
  // create and add children
  ski = 0;
  for (const XmlNodeP cnode : wnode->children())
    if (skip_child[ski++])
      continue;
    else
      {
        if (!container)
          container = widget->as_container_impl();
        if (!container)
          {
            critical ("%s: invalid container type: %s", node_location (wnode), wnode->name());
            break;
          }
        WidgetImplP child = build_widget (&*cnode, &*cnode, SCOPE_CHILD);
        if (child)
          try {
            // be verbose...
            FDEBUG ("%s: built child '%s': %s", node_location (cnode), cnode->name(), child ? child->name() : "<null>");
            container->add (*child);
          } catch (std::exception &exc) {
            critical ("%s: adding %s to parent failed: %s", node_location (cnode), cnode->name(), exc.what());
          }
      }
  return widget;
}

bool
Builder::widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier)
{
  initialize_factory_lazily();
  const XmlNode *const ancestor_node = lookup_interface_node (ancestor_identifier, NULL); // maybe NULL
  const WidgetTypeFactory *const ancestor_itfactory = ancestor_node ? NULL : lookup_widget_factory (ancestor_identifier);
  if (widget_identifier == ancestor_identifier && (ancestor_node || ancestor_itfactory))
    return true; // potential fast path
  if (!ancestor_node && !ancestor_itfactory)
    return false; // ancestor_identifier is non-existent
  String identifier = widget_identifier;
  const XmlNode *node = lookup_interface_node (identifier, NULL), *last = node;
  while (node)
    {
      if (node == ancestor_node)
        return true; // widget ancestor matches
      identifier = node->name();
      last = node;
      node = lookup_interface_node (identifier, node);
    }
  if (ancestor_node)
    return false; // no node match possible
  if (last && last->name() == "Rapicorn_Factory")
    {
      const StringVector &attributes_names = last->list_attributes(), &attributes_values = last->list_values();
      const WidgetTypeFactory *widget_factory = NULL;
      for (size_t i = 0; i < attributes_names.size(); i++)
        if (attributes_names[i] == "factory-type" || attributes_names[i] == "factory_type")
          {
            widget_factory = lookup_widget_factory (attributes_values[i]);
            break;
          }
      return widget_factory && widget_factory == ancestor_itfactory;
    }
  return false;
}

// == UI Creation API ==
bool
check_ui_window (const String &widget_identifier)
{
  return Builder::widget_has_ancestor (widget_identifier, "Rapicorn_Factory:Window");
}

WidgetImplP
create_ui_widget (const String &widget_identifier, const ArgumentList &arguments)
{
  const String call_location = string_format ("Factory::create_ui_widget(%s)", widget_identifier);
  StringVector anames, avalues;
  for (size_t i = 0; i < arguments.size(); i++)
    {
      const String &arg = arguments[i];
      const size_t pos = arg.find ('=');
      if (pos != String::npos)
        anames.push_back (arg.substr (0, pos)), avalues.push_back (arg.substr (pos + 1));
      else
        FDEBUG ("%s: argument without value: %s", call_location, arg);
    }
  WidgetImplP widget = Builder::eval_and_build (widget_identifier, anames, avalues, call_location);
  if (!widget)
    critical ("%s: failed to create widget: %s", call_location, widget_identifier);
  return widget;
}

WidgetImplP
create_ui_child (ContainerImpl &container, const String &widget_identifier, const ArgumentList &arguments, bool autoadd)
{
  // figure XML context
  FactoryContext *fc = container.factory_context();
  assert_return (fc != NULL, NULL);
  // create child within parent namespace
  //local_namespace_list.push_back (namespace_domain);
  WidgetImplP widget = create_ui_widget (widget_identifier, arguments);
  //local_namespace_list.pop_back();
  // add to parent
  if (autoadd)
    container.add (*widget);
  return widget;
}

// == XML Parsing and Registration ==
static String
parse_ui_data_internal (const String &data_name, size_t data_length,
                        const char *data, const String &i18n_domain, vector<String> *definitions)
{
  String pseudoroot; // automatically wrap definitions into root tag <interfaces/>
  const size_t estart = MarkupParser::seek_to_element (data, data_length);
  if (estart + 11 < data_length && strncmp (data + estart, "<interfaces", 11) != 0 && data[estart + 1] != '?')
    pseudoroot = "interfaces";
  MarkupParser::Error perror;
  XmlNodeP xnode = XmlNode::parse_xml (data_name, data, data_length, &perror, pseudoroot);
  String errstr;
  if (perror.code)
    errstr = string_format ("%s:%d:%d: %s", data_name.c_str(), perror.line_number, perror.char_number, perror.message.c_str());
  else
    {
      errstr = register_interface_file (data_name, xnode, definitions);
    }
  return errstr;
}

String
parse_ui_data (const String &data_name, size_t data_length,
               const char *data, const String &i18n_domain, vector<String> *definitions)
{
  initialize_factory_lazily();
  return parse_ui_data_internal (data_name, data_length, data, "", definitions);
}

} // Factory

static void
initialize_factory_lazily (void)
{
  do_once
    {
      Blob blob = Res ("@res Rapicorn/foundation.xml");
      Factory::parse_ui_data_internal ("Rapicorn/foundation.xml", blob.size(), blob.data(), "", NULL);
      blob = Res ("@res Rapicorn/standard.xml");
      Factory::parse_ui_data_internal ("Rapicorn/standard.xml", blob.size(), blob.data(), "", NULL);
    }
}

} // Rapicorn
