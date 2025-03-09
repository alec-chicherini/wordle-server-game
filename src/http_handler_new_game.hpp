#pragma once
//TODO
namespace wordle_server_game {
class HttpHandlerNewGame final
    : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-new-game";

  using server::handlers::HttpHandlerBase::HttpHandlerBase;

  HttpHandlerNewGame(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};
}  // namespace server_http