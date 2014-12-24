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

struct FactoryContext {
  const XmlNode *xnode;
  StringSeq     *type_tags;
  String         type;
  FactoryContext (const XmlNode *xn) : xnode (xn), type_tags (NULL) {}
};
static std::map<const XmlNode*, FactoryContext*> factory_context_map; /// @TODO: Make factory_context_map threadsafe

static void initialize_factory_lazily (void);

namespace Factory {

static const String
definition_name (const XmlNode &xnode)
{
  const StringVector &attributes_names = xnode.list_attributes(), &attributes_values = xnode.list_values();
  for (size_t i = 0; i < attributes_names.size(); i++)
    if (attributes_names[i] == "id")
      return attributes_values[i];
  return "";
}

static const String
parent_type_name (const XmlNode &xnode)
{
  return xnode.name();
}

class NodeData {
  void
  setup (XmlNode &xnode)
  {
    const StringVector &attributes_names = xnode.list_attributes(); // &attributes_values = xnode.list_values();
    for (size_t i = 0; i < attributes_names.size(); i++)
      if (attributes_names[i] == "tmpl:presuppose")
        tmpl_presuppose = true;
  }
  static struct NodeDataKey : public DataKey<NodeData*> {
    virtual void destroy (NodeData *data) { delete data; }
  } node_data_key;
public:
  bool gadget_definition, tmpl_presuppose;
  String domain;
  NodeData (XmlNode &xnode) : gadget_definition (false), tmpl_presuppose (false) { setup (xnode); }
  static NodeData&
  from_xml_node (XmlNode &xnode)
  {
    NodeData *ndata = xnode.get_data (&node_data_key);
    if (!ndata)
      {
        ndata = new NodeData (xnode);
        xnode.set_data (&node_data_key, ndata);
      }
    return *ndata;
  }
};

NodeData::NodeDataKey NodeData::node_data_key;

static const NodeData&
xml_node_data (const XmlNode &xnode)
{
  return NodeData::from_xml_node (*const_cast<XmlNode*> (&xnode));
}

static bool
is_definition (const XmlNode &xnode)
{
  const NodeData &ndata = xml_node_data (xnode);
  const bool isdef = ndata.gadget_definition;
  return isdef;
}

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

typedef map<String, const XmlNodeP> GadgetDefinitionMap;
static GadgetDefinitionMap gadget_definition_map;
static vector<String>      local_namespace_list;
static vector<String>      gadget_namespace_list;

static const XmlNode*
gadget_definition_lookup (const String &widget_identifier, const XmlNode *context_node)
{
  GadgetDefinitionMap::iterator it = gadget_definition_map.find (widget_identifier);
  if (it != gadget_definition_map.end())
    return &*it->second; // non-namespace lookup succeeded
  if (context_node)
    {
      const NodeData &ndata = xml_node_data (*context_node);
      if (!ndata.domain.empty())
        it = gadget_definition_map.find (ndata.domain + ":" + widget_identifier);
      if (it != gadget_definition_map.end())
        return &*it->second; // lookup in context namespace succeeded
    }
  for (ssize_t i = local_namespace_list.size() - 1; i >= 0; i--)
    {
      it = gadget_definition_map.find (local_namespace_list[i] + ":" + widget_identifier);
      if (it != gadget_definition_map.end())
        return &*it->second; // namespace searchpath lookup succeeded
    }
  for (ssize_t i = gadget_namespace_list.size() - 1; i >= 0; i--)
    {
      it = gadget_definition_map.find (gadget_namespace_list[i] + ":" + widget_identifier);
      if (it != gadget_definition_map.end())
        return &*it->second; // namespace searchpath lookup succeeded
    }
#if 0
  printerr ("%s: FAIL, no '%s' in namespaces:", __func__, widget_identifier.c_str());
  if (context_node)
    {
      String context_domain = context_node->get_data (&xml_node_domain_key);
      printerr (" %s", context_domain.c_str());
    }
  for (size_t i = 0; i < local_namespace_list.size(); i++)
    printerr (" %s", local_namespace_list[i].c_str());
  for (size_t i = 0; i < gadget_namespace_list.size(); i++)
    printerr (" %s", gadget_namespace_list[i].c_str());
  printerr ("\n");
#endif
  return NULL; // unmatched
}

void
use_ui_namespace (const String &uinamespace)
{
  initialize_factory_lazily();
  vector<String>::iterator it = find (gadget_namespace_list.begin(), gadget_namespace_list.end(), uinamespace);
  if (it != gadget_namespace_list.end())
    gadget_namespace_list.erase (it);
  gadget_namespace_list.push_back (uinamespace);
}

static void
force_ui_namespace_use (const String &uinamespace)
{
  vector<String>::const_iterator it = find (gadget_namespace_list.begin(), gadget_namespace_list.end(), uinamespace);
  if (it == gadget_namespace_list.end())
    gadget_namespace_list.push_back (uinamespace);
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
  if (is_definition (xnode))
    return definition_name (xnode);
  else
    return xnode.name();
}

String
factory_context_type (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  const XmlNode *xnode = fc->xnode;
  if (!is_definition (*xnode)) // lookup definition node from child node
    {
      xnode = gadget_definition_lookup (xnode->name(), xnode);
      assert_return (xnode != NULL, "");
    }
  assert_return (is_definition (*xnode), "");
  return definition_name (*xnode);
}

UserSource
factory_context_source (FactoryContext *fc)
{
  assert_return (fc != NULL, UserSource (""));
  const XmlNode *xnode = fc->xnode;
  if (!is_definition (*xnode)) // lookup definition node from child node
    {
      xnode = gadget_definition_lookup (xnode->name(), xnode);
      assert_return (xnode != NULL, UserSource (""));
    }
  assert_return (is_definition (*xnode), UserSource (""));
  return UserSource ("WidgetFactory", xnode->parsed_file(), xnode->parsed_line());
}

static void
factory_context_list_types (StringVector &types, const XmlNode *xnode, const bool need_ids, const bool need_variants)
{
  assert_return (xnode != NULL);
  if (!is_definition (*xnode)) // lookup definition node from child node
    {
      xnode = gadget_definition_lookup (xnode->name(), xnode);
      assert_return (xnode != NULL);
    }
  while (xnode)
    {
      assert_return (is_definition (*xnode));
      if (need_ids)
        types.push_back (definition_name (*xnode));
      const String parent_name = parent_type_name (*xnode);
      xnode = gadget_definition_lookup (parent_name, xnode);
      if (!xnode)
        {
          const WidgetTypeFactory *itfactory = lookup_widget_factory (parent_name);
          assert_return (itfactory != NULL);
          types.push_back (itfactory->type_name());
          std::vector<const char*> variants;
          itfactory->type_name_list (variants);
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
                             StringVector &out_names, StringVector &out_values, String *node_name, String *child_container_name);
  bool      try_set_property(WidgetImpl &widget, const String &property_name, const String &value);
  WidgetImplP build_scope   (const StringVector &caller_arg_names, const StringVector &caller_arg_values, const String &caller_location,
                             const XmlNode *factory_context_node);
  WidgetImplP build_widget  (const XmlNode *node, const XmlNode *factory_context_node, Flags bflags);
  explicit  Builder         (const XmlNode &definition_node);
  static String canonify_dashes (const String &key);
public:
  explicit  Builder             (const String &widget_identifier, const XmlNode *context_node);
  static WidgetImplP eval_and_build        (const String &widget_identifier,
                                            const StringVector &call_names, const StringVector &call_values, const String &call_location);
  static WidgetImplP build_from_definition (const String &widget_identifier,
                                            const StringVector &call_names, const StringVector &call_values, const String &call_location,
                                            const XmlNode *factory_context_node);
  static bool widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier);
};

Builder::Builder (const String &widget_identifier, const XmlNode *context_node) :
  dnode_ (gadget_definition_lookup (widget_identifier, context_node)), child_container_ (NULL)
{
  if (!dnode_)
    return;
}

Builder::Builder (const XmlNode &definition_node) :
  dnode_ (&definition_node), child_container_ (NULL)
{
  assert_return (is_definition (*dnode_));
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
  return build_from_definition (widget_identifier, call_names, call_values, call_location, NULL);
}

WidgetImplP
Builder::build_from_definition (const String &widget_identifier,
                                const StringVector &call_names, const StringVector &call_values, const String &call_location,
                                const XmlNode *factory_context_node)
{
  // initialize and check builder
  initialize_factory_lazily();
  Builder builder (widget_identifier, NULL);
  WidgetImplP widget;
  if (builder.dnode_)
    {
      widget = builder.build_scope (call_names, call_values, call_location, factory_context_node ? factory_context_node : builder.dnode_);
      if (!factory_context_node)
        FDEBUG ("%s: built widget '%s': %s", node_location (builder.dnode_), widget_identifier, widget ? widget->name() : "<null>");
    }
  else
    {
      const WidgetTypeFactory *itfactory = factory_context_node ? lookup_widget_factory (widget_identifier) : NULL;
      if (!itfactory)
        critical ("%s: unknown type identifier: %s", call_location, widget_identifier);
      else
        {
          if (call_names.size() != 1 || call_names[0] != "id" || call_values.size() != 1 ||
              call_values[0].size() < 1 || call_values[0][0] == '@')
            critical ("%s: invalid arguments to factory type: %s", call_location, widget_identifier);
          FactoryContext *fc = factory_context_map[factory_context_node];
          if (!fc)
            {
              fc = new FactoryContext (factory_context_node);
              factory_context_map[factory_context_node] = fc;
            }
          widget = itfactory->create_widget (fc);
          if (widget)
            widget->name (call_values[0]);
        }
    }
  return widget;
}

void
Builder::eval_args (const StringVector &in_names, const StringVector &in_values, const XmlNode *errnode,
                    StringVector &out_names, StringVector &out_values, String *node_name, String *child_container_name)
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
      else if (node_name && (cname == "name" || cname == "id"))
        *node_name = rvalue;
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
  eval_args (prop_names, prop_values, wnode, eprop_names, eprop_values, NULL, filter_child_container ? &child_container_name_ : NULL);
  // create widget and assign properties from attributes and property element syntax
  WidgetImplP widget = build_from_definition (wnode->name(), eprop_names, eprop_values, node_location (wnode), factory_context_node);
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
  const XmlNode *const anode = gadget_definition_lookup (ancestor_identifier, NULL); // maybe NULL
  const WidgetTypeFactory *const ancestor_itfactory = anode ? NULL : lookup_widget_factory (ancestor_identifier);
  if (widget_identifier == ancestor_identifier && (anode || ancestor_itfactory))
    return true; // potential fast path
  else if (!anode && !ancestor_itfactory)
    return false; // ancestor_identifier is non-existent
  String identifier = widget_identifier;
  const XmlNode *node = gadget_definition_lookup (identifier, NULL);
  while (node)
    {
      if (node == anode)
        return true; // widget ancestor matches
      identifier = parent_type_name (*node);
      node = gadget_definition_lookup (identifier, node);
    }
  if (anode)
    return false; // no node match possible
  const WidgetTypeFactory *widget_itfactory = lookup_widget_factory (identifier);
  return ancestor_itfactory && widget_itfactory == ancestor_itfactory;
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
  const XmlNode *xnode = fc->xnode;
  const NodeData &ndata = NodeData::from_xml_node (const_cast<XmlNode&> (*xnode));
  // create child within parent namespace
  local_namespace_list.push_back (ndata.domain);
  WidgetImplP widget = create_ui_widget (widget_identifier, arguments);
  local_namespace_list.pop_back();
  // add to parent
  if (autoadd)
    container.add (*widget);
  return widget;
}

// == XML Parsing and Registration ==
static void
assign_xml_node_data_recursive (XmlNode *xnode, const String &domain)
{
  NodeData &ndata = NodeData::from_xml_node (*xnode);
  ndata.domain = domain;
  XmlNode::ConstNodes &children = xnode->children();
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNode *cnode = &**it;
      if (cnode->istext() || is_property (*cnode) || cnode->name() == "Argument")
        continue;
      assign_xml_node_data_recursive (const_cast<XmlNode*> (cnode), domain);
    }
}

static String
register_ui_node (const String &domain, XmlNodeP xnode, vector<String> *definitions)
{
  const String &nname = definition_name (*xnode);
  String ident = domain.empty () ? nname : domain + ":" + nname;
  GadgetDefinitionMap::iterator it = gadget_definition_map.find (ident);
  if (it != gadget_definition_map.end())
    return string_format ("%s: redefinition of: %s (previously at %s)",
                          node_location (&*xnode).c_str(), ident.c_str(), node_location (&*it->second).c_str());
  assign_xml_node_data_recursive (&*xnode, domain);
  NodeData &ndata = NodeData::from_xml_node (*xnode);
  ndata.gadget_definition = true;
  gadget_definition_map.insert (std::make_pair (ident, xnode));
  if (definitions)
    definitions->push_back (ident);
  FDEBUG ("register: %s", ident.c_str());
  return ""; // success;
}

static String
register_rapicorn_definitions (const String &domain, XmlNode *xnode, vector<String> *definitions)
{
  // enforce sane toplevel node
  if (xnode->name() != "rapicorn-definitions")
    return string_format ("%s: invalid root: %s", node_location (xnode).c_str(), xnode->name().c_str());
  // register template children
  XmlNode::ConstNodes children = xnode->children();
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNodeP cnode = *it;
      if (cnode->istext())
        continue; // ignore top level text
      const String cerr = register_ui_node (domain, cnode, definitions);
      if (!cerr.empty())
        return cerr;
    }
  return ""; // success;
}

static String
parse_ui_data_internal (const String &domain, const String &data_name, size_t data_length,
                        const char *data, const String &i18n_domain, vector<String> *definitions)
{
  String pseudoroot; // automatically wrap definitions into root tag <rapicorn-definitions/>
  const size_t estart = MarkupParser::seek_to_element (data, data_length);
  if (estart + 21 < data_length && strncmp (data + estart, "<rapicorn-definitions", 21) != 0 && data[estart + 1] != '?')
    pseudoroot = "rapicorn-definitions";
  MarkupParser::Error perror;
  XmlNodeP xnode = XmlNode::parse_xml (data_name, data, data_length, &perror, pseudoroot);
  String errstr;
  if (perror.code)
    errstr = string_format ("%s:%d:%d: %s", data_name.c_str(), perror.line_number, perror.char_number, perror.message.c_str());
  else
    errstr = register_rapicorn_definitions (domain, &*xnode, definitions);
  return errstr;
}

String
parse_ui_data (const String &uinamespace, const String &data_name, size_t data_length,
               const char *data, const String &i18n_domain, vector<String> *definitions)
{
  initialize_factory_lazily();
  return parse_ui_data_internal (uinamespace, data_name, data_length, data, "", definitions);
}

} // Factory

static void
initialize_factory_lazily (void)
{
  do_once
    {
      const char *domain = "Rapicorn";
      Factory::force_ui_namespace_use (domain);
      Blob blob = Res ("@res Rapicorn/foundation.xml");
      Factory::parse_ui_data_internal (domain, "Rapicorn/foundation.xml", blob.size(), blob.data(), "", NULL);
      blob = Res ("@res Rapicorn/standard.xml");
      Factory::parse_ui_data_internal (domain, "Rapicorn/standard.xml", blob.size(), blob.data(), "", NULL);
    }
}

} // Rapicorn
