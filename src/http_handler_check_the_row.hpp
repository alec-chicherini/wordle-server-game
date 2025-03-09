#pragma once

namespace wordle_server_game {
class HttpHandlerCheckTheRow final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-row";

  using server::handlers::HttpHandlerBase::HttpHandlerBase;

  HttpHandlerCheckTheRow(const components::ComponentConfig& config,
                     const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};
}  // namespace wordle_server_game