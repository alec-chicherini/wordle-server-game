#include <request_check_the_row_body.pb.h>
#include <request_new_game_body.pb.h>
#include <response_check_the_row_body.pb.h>
#include <response_new_game_body.pb.h>

#include <http_handler_new_game.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/uuid4.hpp>

namespace wordle_server_game {

HttpHandlerNewGame::HttpHandlerNewGame(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-wordle")
              .GetCluster()) {}

std::string HttpHandlerNewGame::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  std::string_view octet_stream("application/octet-stream");

  if (request.HasHeader(octet_stream)) {
    const std::string& protobuf_data = request.GetHeader(octet_stream);
    wordle_data::RequestNewGameBody request_new_game_body;
    request_new_game_body.ParseFromString(protobuf_data.c_str());

    std::string new_game_uuid = userver::utils::generators::GenerateBoostUuid();

    auto result_random_word = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT* FROM server_game_schema.words ORDER BY RANDOM() LIMIT 1;");
    std::string word_answer = result_random_word.AsSingleRow<std::string>();

    [[maybe_unused]] auto result_insert_new_game = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO server_game_schema.game_sessions(game_uuid, user_uuid, "
        "word_answer) "
        "VALUES($1, $2, $3) "
        "ON CONFLICT (game_uuid) "
        "DO NOTHING",
        new_game_uuid, request_new_game_body.user_uuid(), word_answer);

    wordle_data::ResponseNewGameBody response_new_game_body;
    wordle_data::UUID* game_uuid = new wordl_data::UUID;
    game_uuid->set_value(new_game_uuid);
    response_new_game_body.set_allocated_game_uuid(game_uuid);
    std::string response_new_game_body_serialize;
    response_new_game_body.SerializeToString(&response_new_game_body_serialize);

    auto& response = request.GetHttpResponse();
    response.SetContentType(octet_stream);
    response.SetStatus(userver::headers::StatusCode::kOk);
    return response_new_game_body_serialize;
  } else {
    response.SetStatus(userver::headers::StatusCode::kBadRequest);
    return "ERROR. No header application/octet-stream in request.";
  }

}  // namespace server_http
