// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_APPLICATION_HH__
#define __RAPICORN_APPLICATION_HH__

#include <ui/commands.hh>

namespace Rapicorn {

class ApplicationImpl : public ApplicationIface {
  vector<WindowIface*> windows_;
  int                  tc_;
public:
  explicit            ApplicationImpl        ();
  virtual String      auto_path              (const String  &file_name,
                                              const String  &binary_path,
                                              bool           search_vpath = true) override;
  virtual StringSeq   auto_load              (const std::string &file_name,
                                              const std::string &binary_path,
                                              const std::string &i18n_domain = "") override;
  virtual void        load_string            (const std::string &xml_string,
                                              const std::string &i18n_domain = "") override;
  virtual WindowIface* create_window         (const std::string &window_identifier,
                                              const StringSeq &arguments = StringSeq()) override;
  void                add_window             (WindowIface &window);
  bool                remove_window          (WindowIface &window);
  virtual void        close_all              () override;
  virtual WindowIface*query_window           (const String &selector) override;
  virtual WindowList  query_windows          (const String &selector) override;
  virtual WindowList  list_windows           () override;
  virtual BindableRelayIface*  create_bindable_relay   () override;
  virtual ListModelRelayIface* create_list_model_relay (const std::string &name = "") override;
  virtual void        test_counter_set       (int val) override;
  virtual void        test_counter_add       (int val) override;
  virtual int         test_counter_get       () override;
  virtual int         test_counter_inc_fetch () override;
  virtual int64       test_hook              () override;
  void                lost_primaries         ();
  static ApplicationImpl& the                ();
};

} // Rapicorn

#endif  /* __RAPICORN_APPLICATION_HH__ */
