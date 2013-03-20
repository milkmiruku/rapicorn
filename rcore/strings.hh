// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_STRINGS_HH__
#define __RAPICORN_STRINGS_HH__

#include <rcore/utilities.hh>
#include <string>

namespace Rapicorn {

// == i18n ==
const char*                     rapicorn_gettext        (const char *text) RAPICORN_FORMAT (1);
#ifdef __RAPICORN_BUILD__
#define _(str)  Rapicorn::rapicorn_gettext (str)
#define N_(str) (str)
#endif

// == Macros ==
#ifdef RAPICORN_CONVENIENCE
/// Produce a const char* string, wrapping @a str into C-style double quotes.
#define CQUOTE(str)                                     RAPICORN_CQUOTE(str)
/// Create a Rapicorn::StringVector, from a const char* C-style array.
#define STRING_VECTOR_FROM_ARRAY(ConstCharArray)        RAPICORN_STRING_VECTOR_FROM_ARRAY(ConstCharArray)
#endif // RAPICORN_CONVENIENCE

// == String ==
String                          string_multiply          (const String &s, uint64 count);
String                          string_canonify          (const String &s, const String &valid_chars, const String &substitute);
String                          string_set_a2z           ();
String                          string_set_A2Z           ();
String                          string_set_ascii_alnum   ();
String  			string_tolower           (const String &str);
String  			string_toupper           (const String &str);
String  			string_totitle           (const String &str);
String  			string_printf            (const char *format, ...) RAPICORN_PRINTF (1, 2);
String  			string_vprintf           (const char *format, va_list vargs);
String  			string_locale_printf     (const char *format, ...) RAPICORN_PRINTF (1, 2);
String  			string_locale_vprintf    (const char *format, va_list vargs);
StringVector 			string_split             (const String &string, const String &splitter = "");
String  			string_join              (const String &junctor, const StringVector &strvec);
bool    			string_to_bool           (const String &string, bool fallback = false);
String  			string_from_bool         (bool value);
uint64  			string_to_uint           (const String &string, uint base = 10);
String  			string_from_uint         (uint64 value);
bool    			string_has_int           (const String &string);
int64   			string_to_int            (const String &string, uint base = 10);
String  			string_from_int          (int64 value);
String  			string_from_float        (float value);
double  			string_to_double         (const String &string);
double  			string_to_double         (const char *dblstring, const char **endptr);
String                          string_from_double       (double value);
inline String                   string_from_float        (double value)         { return string_from_double (value); }
inline double                   string_to_float          (const String &string) { return string_to_double (string); }
template<typename Type> Type    string_to_type           (const String &string);
template<typename Type> String  string_from_type         (Type          value);
template<> inline double        string_to_type<double>   (const String &string) { return string_to_double (string); }
template<> inline String        string_from_type<double> (double         value) { return string_from_double (value); }
template<> inline float         string_to_type<float>    (const String &string) { return string_to_float (string); }
template<> inline String        string_from_type<float>  (float         value)  { return string_from_float (value); }
template<> inline bool          string_to_type<bool>     (const String &string) { return string_to_bool (string); }
template<> inline String        string_from_type<bool>   (bool         value)   { return string_from_bool (value); }
template<> inline int16         string_to_type<int16>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int16>  (int16         value)  { return string_from_int (value); }
template<> inline uint16        string_to_type<uint16>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint16> (uint16        value)  { return string_from_uint (value); }
template<> inline int           string_to_type<int>      (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int>    (int         value)    { return string_from_int (value); }
template<> inline uint          string_to_type<uint>     (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint>   (uint           value) { return string_from_uint (value); }
template<> inline int64         string_to_type<int64>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int64>  (int64         value)  { return string_from_int (value); }
template<> inline uint64        string_to_type<uint64>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint64> (uint64         value) { return string_from_uint (value); }
template<> inline String        string_to_type<String>   (const String &string) { return string; }
template<> inline String        string_from_type<String> (String         value) { return value; }
vector<double>                  string_to_double_vector  (const String         &string);
String                          string_from_double_vector(const vector<double> &dvec,
                                                          const String         &delim = " ");
String  			string_from_errno        (int         errno_val);
bool                            string_is_uuid           (const String &uuid_string); /* check uuid formatting */
int                             string_cmp_uuid          (const String &uuid_string1,
                                                          const String &uuid_string2); /* -1=smaller, 0=equal, +1=greater (assuming valid uuid strings) */
bool                            string_startswith        (const String &string, const String &fragment);
bool                            string_endswith          (const String &string, const String &fragment);
String  string_from_pretty_function_name                 (const char *gnuc_pretty_function);
String  string_to_cescape                                (const String &str);
String  string_to_cquote                                 (const String &str);
String  string_from_cquote                               (const String &input);
String  string_lstrip                                    (const String &input);
String  string_rstrip                                    (const String &input);
String  string_strip                                     (const String &input);
String  string_substitute_char                           (const String &input, const char match, const char subst);
String  string_vector_find (const StringVector &svector, const String &key, const String &fallback);
StringVector cstrings_to_vector (const char*, ...) RAPICORN_SENTINEL;
void         memset4		(uint32 *mem, uint32 filler, uint length);
long double posix_locale_strtold   (const char *nptr, char **endptr);
long double current_locale_strtold (const char *nptr, char **endptr);

// == String Options ==
bool    string_option_check     (const String   &option_string,
                                 const String   &option);
String  string_option_get       (const String   &option_string,
                                 const String   &option);
void    string_options_split    (const String   &option_string,
                                 vector<String> &option_names,
                                 vector<String> &option_values,
                                 const String   &empty_default = "");

// == Strings ==
/// Convenience Constructor for StringSeq or std::vector<std::string>
class Strings : public std::vector<std::string>
{
  typedef const std::string CS;
public:
  explicit Strings (CS &s1);
  explicit Strings (CS &s1, CS &s2);
  explicit Strings (CS &s1, CS &s2, CS &s3);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA, CS &sB);
  explicit Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA, CS &sB, CS &sC);
};

// == Charset Conversions ==
bool    text_convert    (const String &to_charset,
                         String       &output_string,
                         const String &from_charset,
                         const String &input_string,
                         const String &fallback_charset = "ISO-8859-15",
                         const String &output_mark = "");

// == Implementations ==
#define RAPICORN_STRING_VECTOR_FROM_ARRAY(ConstCharArray)               ({ \
  Rapicorn::StringVector __a;                                           \
  const Rapicorn::uint64 __l = RAPICORN_ARRAY_SIZE (ConstCharArray);    \
  for (Rapicorn::uint64 __ai = 0; __ai < __l; __ai++)                   \
    __a.push_back (ConstCharArray[__ai]);                               \
  __a; })
#define RAPICORN_CQUOTE(str)    (Rapicorn::string_to_cquote (str).c_str())

} // Rapicorn

#endif /* __RAPICORN_STRINGS_HH__ */
