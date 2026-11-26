#include "CLI11.hpp"
#include "CurlReq.hpp"
#include "types.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// needed eps:
// name-forms
// stats
// search

int main(int argc, char **argv) {
  try {
    CLI::App app{"hun names client"};
    app.require_subcommand(1);

    std::string client{};
    std::string key{};

    app.add_option_function<std::string>(
           "--client-id",
           [&client](std::string client_id) { client = client_id; })
        ->required();
    app.add_option_function<std::string>(
           "--api-key", [&key](std::string api_key) { key = api_key; })
        ->required();

    auto names =
        app.add_subcommand("names", "get all names based on search filters");

    std::vector<std::pair<std::string, std::string>> names_parameters{};

    names->add_option_function<char>(
        "--firstLetter", [&names_parameters](char frst) {
          names_parameters.push_back({"firstLetter", std::string{frst}});
        });

    names->add_option_function<int>("--sex", [&names_parameters](int sex) {
      if (sex > 3 || sex < 1)
        throw types::Exit{0, "invalid integer given for sex"};

      names_parameters.push_back({"sex", std::to_string(sex)});
    });

    names->add_option_function<bool>("--certificate", [&names_parameters](
                                                          bool certified) {
      std::string param{};
      if (certified)
        param = "true";
      else
        param = "false";

      names_parameters.push_back({"certificate", std::to_string(certified)});
    });

    names->add_option_function<int>("--group", [&names_parameters](int grp) {
      names_parameters.push_back({"group", std ::to_string(grp)});
    });

    names->add_option_function<std::string>(
        "--search", [&names_parameters](std::string src) {
          names_parameters.push_back({"search", src});
        });

    names->add_option_function<std::string>(
        "--searchIn", [&names_parameters](std::string in) {
          if (in != "name" && in != "all")
            throw types::Exit{0, "invalid search in parameter"};
          names_parameters.push_back({"searchIn", in});
        });

    names->add_option_function<unsigned int>(
        "--limit", [&names_parameters](unsigned int limit) {
          if (limit > 20)
            throw types::Exit{0, "max allowed limit exceeded"};
          names_parameters.push_back({"limit", std::to_string(limit)});
        });

    names->add_option_function<std::string>(
        "--pageToken", [&names_parameters](std::string pin) {
          names_parameters.push_back({"pageToken", pin});
        });

    names->add_option_function<std::string>(
        "--fields", [&names_parameters](std::string field_s) {
          names_parameters.push_back({"fields", field_s});
        });

    auto name = app.add_subcommand("name", "get the info for a single name");

    std::string name_param{};
    std::vector<std::pair<std::string, std::string>> name_parameters{};

    name->add_option_function<std::string>(
            "name", [&name_param](std::string name) { name_param = name; })
        ->required();

    name->add_option_function<std::string>(
        "--fields", [&name_parameters](std::string fields) {
          name_parameters.push_back({"fields", fields});
        });

    auto namedays = app.add_subcommand(
        "namedays", "get info about the namedays of a specific name");

    std::string namedays_name{};

    namedays->add_option_function<std::string>(
        "namedays_name",
        [&namedays_name](std::string name) { namedays_name = name; });

    auto forms = app.add_subcommand("forms", "list different forms of a name");

    std::string form_name{};

    forms->add_option_function<std::string>(
        "form_name", [&form_name](std::string name) { form_name = name; });

    auto stats = app.add_subcommand("stats", "opeque statistics endpoint");

    std::vector<std::pair<std::string, std::string>> stats_parameters{};

    stats->add_option_function<std::string>(
        "--name", [&stats_parameters](std::string name) {
          stats_parameters.push_back({"name", name});
        });

    stats->add_option_function<unsigned int>(
        "--year", [&stats_parameters](unsigned int year) {
          stats_parameters.push_back({"year", std::to_string(year)});
        });

    stats->add_option_function<unsigned int>(
        "--sex", [&stats_parameters](unsigned int sex) {
          if (sex > 3 || sex < 1)
            throw types::Exit{0, "invalid integer given for sex"};

          stats_parameters.push_back({"sex", std::to_string(sex)});
        });

    stats->add_option_function<unsigned int>(
        "--limit", [&stats_parameters](unsigned int limit) {
          if (limit > 20)
            throw types::Exit{0, "max allowed limit exceeded"};
          stats_parameters.push_back({"limit", std::to_string(limit)});
        });

    stats->add_option_function<std::string>(
        "--compare", [&stats_parameters](std::string compare) {
          stats_parameters.push_back({"compare", compare});
        });

    auto search =
        app.add_subcommand("search", "search based on provided parameters");

    std::vector<std::pair<std::string, std::string>> search_parameters{};

    search->add_option_function<std::string>(
        "--q", [&search_parameters](std::string q) {
          search_parameters.push_back({"q", q});
        });

    search->add_option_function<unsigned int>(
        "--sex", [&search_parameters](unsigned int sex) {
          if (sex > 3 || sex < 1)
            throw types::Exit{0, "invalid integer given for sex"};
          search_parameters.push_back({"sex", std::to_string(sex)});
        });

    search->add_option_function<bool>(
        "--certificate", [&search_parameters](bool cert) {
          std::string certif{};
          if (cert)
            certif = "true";
          else
            certif = "false";

          search_parameters.push_back({"certificate", certif});
        });

    search->add_option_function<int>(
        "--group", [&search_parameters](int group) {
          search_parameters.push_back({"group", std::to_string(group)});
        });

    search->add_option_function<std::string>(
        "--nameStart", [&search_parameters](std::string name) {
          search_parameters.push_back({"nameStart", name});
        });

    search->add_option_function<std::string>(
        "--nameEnd", [&search_parameters](std::string name) {
          search_parameters.push_back({"nameEnd", name});
        });

    search->add_option_function<std::string>(
        "--descContains", [&search_parameters](std::string contains) {
          search_parameters.push_back({"descContains", contains});
        });

    search->add_flag_function("--vhLow", [&search_parameters](int _flag) {
      search_parameters.push_back({"vhLow", "true"});
    });

    search->add_flag_function("--vhMixed", [&search_parameters](int _flag) {
      search_parameters.push_back({"vhMixed", "true"});
    });

    search->add_flag_function("--vhHigh", [&search_parameters](int _flag) {
      search_parameters.push_back({"vhHigh", "true"});
    });

    search->add_flag_function(
        "--containConsonantsOnly", [&search_parameters](int _flag) {
          search_parameters.push_back({"containConsonantsOnly", "true"});
        });

    search->add_flag_function(
        "--containVowelsOnly", [&search_parameters](int _flag) {
          search_parameters.push_back({"containVowelsOnly", "true"});
        });

    search->add_option_function<std::string>(
        "--containVowels", [&search_parameters](std::string vowels) {
          search_parameters.push_back({"containVowels", vowels});
        });

    search->add_option_function<std::string>(
        "--containConsonants", [&search_parameters](std::string constants) {
          search_parameters.push_back({"containConsonants", constants});
        });

    search->add_option_function<unsigned int>(
        "--syllableMin", [&search_parameters](unsigned int min) {
          search_parameters.push_back({"syllableMin", std::to_string(min)});
        });

    search->add_option_function<unsigned int>(
        "--syllableMax", [&search_parameters](unsigned int max) {
          search_parameters.push_back({"syllableMax", std::to_string(max)});
        });

    search->add_option_function<unsigned int>(
        "--limit", [&search_parameters](unsigned int limit) {
          if (limit > 20)
            throw types::Exit{0, "max limit exceeded"};
          search_parameters.push_back({"limit", std::to_string(limit)});
        });

    search->add_option_function<std::string>(
        "--namedayFrom", [&search_parameters](std::string namedayfrm) {
          search_parameters.push_back({"namedayFrom", namedayfrm});
        });

    search->add_option_function<std::string>(
        "--namedayTo", [&search_parameters](std::string namedayto) {
          search_parameters.push_back({"namedayTo", namedayto});
        });

    search->add_option_function<std::string>(
        "--pageToken", [&search_parameters](std::string pin) {
          search_parameters.push_back({"pageToken", pin});
        });

    search->add_option_function<std::string>(
        "--fields", [&search_parameters](std::string fields) {
          search_parameters.push_back({"fields", fields});
        });

    CLI11_PARSE(app, argc, argv);

    const std::vector<std::string> headers{"x-client-id: " + client,
                                           "x-api-key: " + key};

    const std::string root_url = "https://magyarnevek.hu/api/v1";

    if (app.got_subcommand(names)) {
      CurlReq::Connection conn{};
      auto request = conn.GET(root_url + "/names", headers, names_parameters);

      auto response = request.get();

      if (response) {
        std::cout << "success" << std::endl;
      } else {
        throw types::Exit{1, types::err_to_string(response.error())};
      }
    }

    if (app.got_subcommand(name)) {
      CurlReq::Connection conn{};
      auto request =
          conn.GET(root_url + "/" + name_param, headers, name_parameters);

      auto response = request.get();

      if (response) {
        std::cout << "success" << std::endl;
      } else {
        throw types::Exit{1, types::err_to_string(response.error())};
      }
    }

    if (app.got_subcommand(namedays)) {
      CurlReq::Connection conn{};
      auto request = conn.GET(root_url + "/" + namedays_name, headers, {});

      auto response = request.get();

      if (response) {
        std::cout << "success" << std::endl;
      } else {
        throw types::Exit{1, types::err_to_string(response.error())};
      }
    }

    if (app.got_subcommand(forms)) {
      CurlReq::Connection conn{};
      auto request = conn.GET(root_url + "/" + form_name, headers, {});

      auto response = request.get();

      if (response) {
        std::cout << "success" << std::endl;
      } else {
        throw types::Exit{1, types::err_to_string(response.error())};
      }
    }

    if (app.got_subcommand(stats)) {
      CurlReq::Connection conn{};
      auto request = conn.GET(root_url + "/stats", headers, stats_parameters);

      auto response = request.get();

      if (response) {
        std::cout << "success" << std::endl;
      } else {
        throw types::Exit{1, types::err_to_string(response.error())};
      }
    }

    if (app.got_subcommand(namedays)) {
      CurlReq::Connection conn{};
      auto request = conn.GET(root_url + "/search", headers, search_parameters);

      auto response = request.get();

      if (response) {
        std::cout << "success" << std::endl;
      } else {
        throw types::Exit{1, types::err_to_string(response.error())};
      }
    }
  } catch (const types::Exit &exit) {
    std::cout << exit.reason << std::endl;
    return exit.return_code;
  } catch (const std::exception &e) {
    std::cout << "an undifined error occured" << std::endl;
    return 1;
  }

  return 0;
}
