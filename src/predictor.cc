#include "predictor.h"

#include "predict_db.h"
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/segmentation.h>
#include <rime/service.h>
#include <rime/translation.h>
#include <rime/schema.h>

namespace rime {

Predictor::Predictor(const Ticket& ticket,
                     PredictDb* db,
                     const int max_candidates,
                     const int max_iteration)
    : Processor(ticket),
      db_(db),
      max_iteration_(max_iteration),
      max_candidates_(max_candidates) {
  // update prediction on context change.
  auto* context = engine_->context();
  select_connection_ = context->select_notifier().connect(
      [this](Context* ctx) { OnSelect(ctx); });
  context_update_connection_ = context->update_notifier().connect(
      [this](Context* ctx) { OnContextUpdate(ctx); });
}

Predictor::~Predictor() {
  select_connection_.disconnect();
  context_update_connection_.disconnect();
}

ProcessResult Predictor::ProcessKeyEvent(const KeyEvent& key_event) {
  auto keycode = key_event.keycode();
  if (keycode == XK_BackSpace || keycode == XK_Escape) {
    last_action_ = kDelete;
    auto* ctx = engine_->context();
    if (!ctx->composition().empty() &&
        ctx->composition().back().HasTag("prediction")) {
      ctx->PopInput(ctx->composition().back().length);
      return kAccepted;
    }
  } else {
    last_action_ = kUnspecified;
  }
  return kNoop;
}

void Predictor::OnSelect(Context* ctx) {
  last_action_ = kSelect;
}

void Predictor::OnContextUpdate(Context* ctx) {
  if (!db_ || !ctx || !ctx->composition().empty())
    return;
  if (last_action_ == kDelete) {
    return;
  }
  if (ctx->commit_history().empty()) {
    Predict(ctx, "$");
    return;
  }
  auto last_commit = ctx->commit_history().back();
  if (last_commit.type == "punct" || last_commit.type == "raw" ||
      last_commit.type == "thru") {
    iteration_counter_ = 0;
    return;
  }
  if (last_commit.type == "prediction") {
    iteration_counter_++;
    if (max_iteration_ > 0 && iteration_counter_ >= max_iteration_) {
      iteration_counter_ = 0;
      auto* ctx = engine_->context();
      if (!ctx->composition().empty() &&
          ctx->composition().back().HasTag("prediction")) {
        ctx->Clear();
      }
      return;
    }
  }
  Predict(ctx, last_commit.text);
}

void Predictor::Predict(Context* ctx, const string& context_query) {
  if (const auto* candidates = db_->Lookup(context_query)) {
    int end = ctx->input().length();
    Segment segment(end, end);
    segment.status = Segment::kGuess;
    segment.tags.insert("prediction");
    ctx->composition().AddSegment(segment);
    ctx->composition().back().tags.erase("raw");

    auto translation = New<FifoTranslation>();
    int i = 0;
    for (auto* it = candidates->begin(); it != candidates->end(); ++it) {
      translation->Append(
          New<SimpleCandidate>("prediction", end, end, db_->GetEntryText(*it)));
      i++;
      if (max_candidates_ > 0 && i >= max_candidates_)
        break;
    }
    auto menu = New<Menu>();
    menu->AddTranslation(translation);
    ctx->composition().back().menu = menu;
  }
}

PredictorComponent::PredictorComponent() {}

PredictorComponent::~PredictorComponent() {}

Predictor* PredictorComponent::Create(const Ticket& ticket) {
  int max_iteration_ = 0, max_candidates_ = 0;
  if (!db_) {
    the<ResourceResolver> res(
        Service::instance().CreateResourceResolver({"predict_db", "", ""}));
    // try to load keys from schema,
    // "predictor/db", "predictor/max_iteration", "predictor/max_candidates"
    auto* schema = ticket.schema;
    if (schema) {
      auto* config = schema->config();
      string customized_db;
      if (!config->GetInt("predictor/max_iteration", &max_iteration_)) {
        LOG(INFO) << "preditor/max_iteration is not set in schema";
      }
      if (!config->GetInt("predictor/max_candidates", &max_candidates_)) {
        LOG(INFO) << "preditor/max_candidates is not set in schema";
      }
      if (config->GetString("predictor/db", &customized_db) &&
          !customized_db.empty()) {
        auto db = std::make_unique<PredictDb>(
            res->ResolvePath(customized_db).string());
        if (db && db->Load()) {
          db_ = std::move(db);
        } else {
          LOG(ERROR) << "failed to load db file: "
                     << res->ResolvePath(customized_db).string()
                     << ". try to load default predict.db";
          goto default_db;
        }
      } else {
        LOG(INFO) << "no config key \"predictor/db\" or it's empty, try to "
                     "load default predict db file: predict.db";
        goto default_db;
      }
    } else {
    default_db:
      auto db =
          std::make_unique<PredictDb>(res->ResolvePath("predict.db").string());
      if (db && db->Load()) {
        db_ = std::move(db);
      }
    }
  }
  return new Predictor(ticket, db_.get(), max_candidates_, max_iteration_);
}

}  // namespace rime
