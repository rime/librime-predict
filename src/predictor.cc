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

static const char* kPlaceholder = " ";

namespace rime {

Predictor::Predictor(const Ticket& ticket, PredictDb* db)
    : Processor(ticket), db_(db) {
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
  if (key_event.keycode() == XK_BackSpace) {
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
    return;
  }
  Predict(ctx, last_commit.text);
}

void Predictor::Predict(Context* ctx, const string& context_query) {
  if (const auto* candidates = db_->Lookup(context_query)) {
    ctx->set_input(kPlaceholder);
    int end = ctx->input().length();
    Segment segment(0, end);
    segment.status = Segment::kGuess;
    segment.tags.insert("prediction");
    ctx->composition().AddSegment(segment);
    ctx->composition().back().tags.erase("raw");

    auto translation = New<FifoTranslation>();
    for (auto* it = candidates->begin(); it != candidates->end(); ++it) {
      translation->Append(
          New<SimpleCandidate>("prediction", 0, end, db_->GetEntryText(*it)));
    }
    auto menu = New<Menu>();
    menu->AddTranslation(translation);
    ctx->composition().back().menu = menu;
  }
}

PredictorComponent::PredictorComponent() {}

PredictorComponent::~PredictorComponent() {}

Predictor* PredictorComponent::Create(const Ticket& ticket) {
  if (!db_) {
    the<ResourceResolver> res(
        Service::instance().CreateResourceResolver({"predict_db", "", ""}));
    auto db =
        std::make_unique<PredictDb>(res->ResolvePath("predict.db").string());
    if (db && db->Load()) {
      db_ = std::move(db);
    }
  }
  return new Predictor(ticket, db_.get());
}

}  // namespace rime
