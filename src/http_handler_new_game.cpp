#include <request_new_game_body.pb.h>
#include <response_new_game_body.pb.h>

#include <http_handler_new_game.hpp>
#include <userver/http/status_code.hpp>

#include <userver/utils/boost_uuid4.hpp>

namespace wordle_server_game {

HttpHandlerNewGame::HttpHandlerNewGame(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context
                      .FindComponent<userver::components::Postgres>(
                          "postgres-db-wordle")
                      .GetCluster()) {}

std::string HttpHandlerNewGame::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  std::string_view octet_stream("application/octet-stream");
  auto& response = request.GetHttpResponse();

  if (request.HasHeader(octet_stream)) {
    const std::string& protobuf_data = request.GetHeader(octet_stream);
    wordle_data::RequestNewGameBody request_new_game_body;
    request_new_game_body.ParseFromString(protobuf_data);

    std::string new_game_uuid = userver::utils::ToString(
        userver::utils::generators::GenerateBoostUuid());

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
        new_game_uuid, request_new_game_body.user_uuid().value(), word_answer);

    wordle_data::ResponseNewGameBody response_new_game_body;
    wordle_data::UUID* game_uuid = new wordle_data::UUID;
    game_uuid->set_value(new_game_uuid);
    response_new_game_body.set_allocated_game_uuid(game_uuid);
    std::string response_new_game_body_serialize;
    response_new_game_body.SerializeToString(&response_new_game_body_serialize);

    response.SetContentType(octet_stream);
    response.SetStatus(userver::http::StatusCode::kOk);
    return response_new_game_body_serialize;
  } else {
    response.SetStatus(userver::http::StatusCode::kBadRequest);
    return "ERROR. No header application/octet-stream in request.";
  }
}
}  // namespace wordle_server_game
