#include <request_row_body.pb.h>
#include <response_row_body.pb.h>

#include <format>
#include <http_handler_row.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/uuid4.hpp>

template <>
struct userver::storages::postgres::io::CppToUserPg<wordle_data::TheCharColor> {
  static constexpr DBTypeName postgres_name =
      "server_game_schema.enum_the_char_color";
  static constexpr USERVER_NAMESPACE::utils::TrivialBiMap enumerators =
      [](auto selector) {
        return selector()
            .Case("kNoneTheCharColor",
                  wordle_data::TheCharColor::kNoneTheCharColor)
            .Case("kGreen", wordle_data::TheCharColor::kGreen)
            .Case("kYellow", wordle_data::TheCharColor::kYellow);
      };
};

template <>
struct userver::storages::postgres::io::CppToUserPg<wordle_data::RowResult> {
  static constexpr DBTypeName postgres_name =
      "server_game_schema.enum_row_result";
  static constexpr USERVER_NAMESPACE::utils::TrivialBiMap enumerators =
      [](auto selector) {
        return selector()
            .Case("kNoneRowResult", wordle_data::RowResult::kNoneRowResult)
            .Case("kWordDoNotExists", wordle_data::RowResult::kWordDoNotExists)
            .Case("kWordExists", wordle_data::RowResult::kWordExists)
            .Case("kWordIsAnswer", wordle_data::RowResult::kWordIsAnswer)
            .Case("kServerError", wordle_data::RowResult::kServerError)
            .Case("kGameOver", wordle_data::RowResult::kGameOver);
      };
};

namespace wordle_server_game {

HttpHandlerRow::HttpHandlerRow(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context
                      .FindComponent<userver::components::Postgres>(
                          "postgres-db-wordle")
                      .GetCluster()) {}

std::string HttpHandlerRow::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  std::string_view octet_stream("application/octet-stream");
  auto& response = request.GetHttpResponse();
  if (request.HasHeader(octet_stream) == false) {
    response.SetStatus(userver::http::StatusCode::kBadRequest);
    return "ERROR. No header application/octet-stream in request.";
  }
  const std::string& protobuf_data = request.GetHeader(octet_stream);
  wordle_data::RequestRowBody request_row_body;
  request_row_body.ParseFromString(protobuf_data);
  const std::string& word_asked = request_row_body.word();
  const std::string& game_uuid = request_row_body.game_uuid().value();

  auto word_answer_query = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT word_answer FROM server_game_schema.game_sessions WHERE "
      "game_uuid=$1;",
      game_uuid);
  if (word_answer_query.IsEmpty()) {
    response.SetStatus(userver::http::StatusCode::kNotFound);
    return std::format(
        "ERROR. game_uuid{} not found in server_game_schema.game_sessions",
        game_uuid);
  }
  std::string word_answer = word_answer_query.AsSingleRow<std::string>();

  auto number_of_attempts_query = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT COUNT(*) FROM server_game_schema.game_row_response WHERE "
      "game_uuid=$1;",
      game_uuid);
  if (number_of_attempts_query.IsEmpty()) {
    response.SetStatus(userver::http::StatusCode::kNotFound);
    return std::format(
        "ERROR. game_uuid{} not found in server_game_schema.game_row_response",
        game_uuid);
  }
  int number_of_attempts = number_of_attempts_query.AsSingleRow<int>();

  auto is_word_exists_query = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT COUNT(*) FROM server_game_schema.words WHERE word=$1;",
      word_asked);
  if (is_word_exists_query.IsEmpty()) {
    response.SetStatus(userver::http::StatusCode::kNotFound);
    return std::format("ERROR. query server_game_schema.words", game_uuid);
  }
  bool is_word_exists = is_word_exists_query.AsSingleRow<int>();

  wordle_data::RowResult result;
  std::string word_answer_to_send;
  if (is_word_exists) {
    number_of_attempts += 1;
    if (word_answer == word_asked) {
      result = wordle_data::RowResult::kWordIsAnswer;
    } else if (number_of_attempts >= 5) {
      result = wordle_data::RowResult::kGameOver;
    } else {
      result = wordle_data::RowResult::kWordExists;
    };
  } else {
    result = wordle_data::RowResult::kWordDoNotExists;
  }

  std::vector<wordle_data::TheCharColor> vec_colors;
  if (result == wordle_data::RowResult::kWordDoNotExists) {
    vec_colors.push_back(wordle_data::TheCharColor::kNoneTheCharColor);
    vec_colors.push_back(wordle_data::TheCharColor::kNoneTheCharColor);
    vec_colors.push_back(wordle_data::TheCharColor::kNoneTheCharColor);
    vec_colors.push_back(wordle_data::TheCharColor::kNoneTheCharColor);
    vec_colors.push_back(wordle_data::TheCharColor::kNoneTheCharColor);
  } else if (result == wordle_data::RowResult::kWordExists) {
    for (size_t i = 0; i < word_asked.size(); ++i) {
      if (word_answer[i] == word_asked[i]) {
        vec_colors.push_back(wordle_data::TheCharColor::kGreen);
      } else if (word_answer.find(word_answer[i]) != std::string::npos) {
        vec_colors.push_back(wordle_data::TheCharColor::kYellow);
      } else {
        vec_colors.push_back(wordle_data::TheCharColor::kNoneTheCharColor);
      }
    }
  }

  [[maybe_unused]]
  auto result_insert_game_row_response = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO server_game_schema.game_row_response(game_uuid, "
      "word_asked, result, vec_colors) VALUES($1, $2, $3, $4)",
      game_uuid, word_asked, result, vec_colors);

  [[maybe_unused]] wordle_data::ResponseRowBody response_row_body;
  response_row_body.set_number_of_attempts(number_of_attempts);
  for (const auto& color : vec_colors) {
    response_row_body.add_the_char_colors(color);
  }
  response_row_body.set_row_result(result);
  response_row_body.set_word_answer(word_answer_to_send);

  std::string response_row_body_serialize;
  response_row_body.SerializeToString(&response_row_body_serialize);

  response.SetContentType(octet_stream);
  response.SetStatus(userver::http::StatusCode::kOk);
  return response_row_body_serialize;
}
}  // namespace wordle_server_game
