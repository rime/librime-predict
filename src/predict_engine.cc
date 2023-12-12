#include "predict_engine.h"

#include "predict_db.h"
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/segmentation.h>
#include <rime/service.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/schema.h>
#include <rime/dict/db_pool_impl.h>

namespace rime {

static const ResourceType kPredictDbResourceType = {"predict_db", "", ""};

PredictEngine::PredictEngine(an<PredictDb> db,
                             int max_iterations,
                             int max_candidates)
    : db_(db),
      max_iterations_(max_iterations),
      max_candidates_(max_candidates) {}

PredictEngine::~PredictEngine() {}

bool PredictEngine::Predict(Context* ctx, const string& context_query) {
  DLOG(INFO) << "PredictEngine::Predict [" << context_query << "]";
  if (const auto* candidates = db_->Lookup(context_query)) {
    query_ = context_query;
    candidates_ = candidates;
    return true;
  } else {
    Clear();
    return false;
  }
}

void PredictEngine::Clear() {
  DLOG(INFO) << "PredictEngine::Clear";
  query_.clear();
  candidates_ = nullptr;
}

void PredictEngine::CreatePredictSegment(Context* ctx) const {
  DLOG(INFO) << "PredictEngine::CreatePredictSegment";
  int end = int(ctx->input().length());
  Segment segment(end, end);
  segment.tags.insert("prediction");
  segment.tags.insert("placeholder");
  ctx->composition().AddSegment(segment);
  ctx->composition().back().tags.erase("raw");
  DLOG(INFO) << "segments: " << ctx->composition();
}

an<Translation> PredictEngine::Translate(const Segment& segment) const {
  DLOG(INFO) << "PredictEngine::Translate";
  auto translation = New<FifoTranslation>();
  size_t end = segment.end;
  int i = 0;
  for (auto* it = candidates_->begin(); it != candidates_->end(); ++it) {
    translation->Append(
        New<SimpleCandidate>("prediction", end, end, db_->GetEntryText(*it)));
    i++;
    if (max_candidates_ > 0 && i >= max_candidates_)
      break;
  }
  return translation;
}

PredictEngineComponent::PredictEngineComponent()
    : db_pool_(the<ResourceResolver>(
          Service::instance().CreateResourceResolver(kPredictDbResourceType))) {
}

PredictEngineComponent::~PredictEngineComponent() {}

PredictEngine* PredictEngineComponent::Create(const Ticket& ticket) {
  string db_name = "predict.db";
  int max_candidates = 0;
  int max_iterations = 0;
  if (auto* schema = ticket.schema) {
    auto* config = schema->config();
    if (config->GetString("predictor/db", &db_name)) {
      LOG(INFO) << "custom predictor/db: " << db_name;
    }
    if (!config->GetInt("predictor/max_candidates", &max_candidates)) {
      LOG(INFO) << "predictor/max_candidates is not set in schema";
    }
    if (!config->GetInt("predictor/max_iterations", &max_iterations)) {
      LOG(INFO) << "predictor/max_iterations is not set in schema";
    }
  }
  if (auto db = db_pool_.GetDb(db_name)) {
    if (db->IsOpen() || db->Load()) {
      return new PredictEngine(db, max_iterations, max_candidates);
    } else {
      LOG(ERROR) << "failed to load predict db: " << db_name;
    }
  }
  return nullptr;
}

an<PredictEngine> PredictEngineComponent::GetInstance(const Ticket& ticket) {
  if (Schema* schema = ticket.schema) {
    auto found = predict_engine_by_schema_id.find(schema->schema_id());
    if (found != predict_engine_by_schema_id.end()) {
      if (auto instance = found->second.lock()) {
        return instance;
      }
    }
    an<PredictEngine> new_instance{Create(ticket)};
    if (new_instance) {
      predict_engine_by_schema_id[schema->schema_id()] = new_instance;
      return new_instance;
    }
  }
  return nullptr;
}

}  // namespace rime
