#pragma once

#include "json.hpp"
#include <string>
#include <vector>
namespace types {

enum class GETERR { NON200, FAILEDPARSE, CURLFAILURE };

inline std::string err_to_string(GETERR err) {
  switch (err) {
  case types::GETERR::NON200:
    return "curl received a non 200 satuscode";

  case types::GETERR::FAILEDPARSE:
    return "parsing failed internally";

  case types::GETERR::CURLFAILURE:
    return "curl failed to send or receive the request";
  };

  return "unknown error(segfault >:)";
}

struct Exit {
  int return_code;
  std::string reason;

  Exit(int return_code, std::string reason)
      : return_code(return_code), reason(reason) {}
};

struct Name {
  std::string name;
  int sex;
  std::string description;
  std::string descriptionEnglish;
  std::string phonetic;
  bool certificate;
  int group;
  std::string namedays;
};

struct Names {
  std::vector<Name> data;
  std::string nextPageToken;
};

struct Date {
  std::string hu;
  std::string en;
  std::string iso;
  bool main;
};

struct Dates {
  std::string name;
  std::vector<Date> dates;
};

struct FormFields {
  std::vector<std::string> latin;
  std::vector<std::string> cyrillic;
  std::vector<std::string> greek;
};

struct Forms {
  std::string baseName;
  FormFields forms;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Name, name, sex, description,
                                   descriptionEnglish, phonetic, certificate,
                                   group, namedays)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Names, data, nextPageToken)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Date, hu, en, iso, main)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Dates, name, dates)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FormFields, latin, cyrillic,
                                                greek)
} // namespace types
