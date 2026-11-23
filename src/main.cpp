#include "CLI11.hpp"
#include "CurlReq.hpp"
#include <iostream>

int main(int argc, char **argv) {
  CLI::App app{"hun names client"};

  auto fetch = app.add_subcommand("fetch", "test");

  CLI11_PARSE(app, argc, argv);

  if (app.got_subcommand(fetch)) {
    CurlReq::Connection conn{};
    auto resp =
        conn.GET("https://magyarnevek.hu/api/v1/names",
                 {"x-client-id: good_luck", "x-api-key: tough_shit"}, {});

    auto rsp = resp.get();

    if (rsp) {
      std::cout << "success" << std::endl;
    } else {
      std::cout << (int)rsp.error() << std::endl;
    }
  }

  return 0;
}
