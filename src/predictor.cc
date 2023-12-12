#include "predictor.h"

#include "predict_engine.h"
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/segmentation.h>
#include <rime/service.h>
#include <rime/translation.h>
#include <rime/schema.h>
#include <rime/dict/db_pool_impl.h>

namespace rime {

Predictor::Predictor(const Ticket& ticket, an<PredictEngine> predict_engine)
    : Processor(ticket), predict_engine_(predict_engine) {
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
    predict_engine_->Clear();
    iteration_counter_ = 0;
    auto* ctx = engine_->context();
    if (!ctx->composition().empty() &&
        ctx->composition().back().HasTag("prediction")) {
      ctx->Clear();
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
  if (self_updating_ || !predict_engine_ || !ctx ||
      !ctx->composition().empty() || !ctx->get_option("prediction") ||
      last_action_ == kDelete) {
    return;
  }
  LOG(INFO) << "Predictor::OnContextUpdate";
  if (ctx->commit_history().empty()) {
    PredictAndUpdate(ctx, "$");
    return;
  }
  auto last_commit = ctx->commit_history().back();
  if (last_commit.type == "punct" || last_commit.type == "raw" ||
      last_commit.type == "thru") {
    predict_engine_->Clear();
    iteration_counter_ = 0;
    return;
  }
  if (last_commit.type == "prediction") {
    int max_iterations = predict_engine_->max_iterations();
    iteration_counter_++;
    if (max_iterations > 0 && iteration_counter_ >= max_iterations) {
      predict_engine_->Clear();
      iteration_counter_ = 0;
      auto* ctx = engine_->context();
      if (!ctx->composition().empty() &&
          ctx->composition().back().HasTag("prediction")) {
        ctx->Clear();
      }
      return;
    }
  }
  PredictAndUpdate(ctx, last_commit.text);
}

void Predictor::PredictAndUpdate(Context* ctx, const string& context_query) {
  if (predict_engine_->Predict(ctx, context_query)) {
    predict_engine_->CreatePredictSegment(ctx);
    self_updating_ = true;
    ctx->update_notifier()(ctx);
    self_updating_ = false;
  }
}

PredictorComponent::PredictorComponent(
    an<PredictEngineComponent> engine_factory)
    : engine_factory_(engine_factory) {}

PredictorComponent::~PredictorComponent() {}

Predictor* PredictorComponent::Create(const Ticket& ticket) {
  return new Predictor(ticket, engine_factory_->GetInstance(ticket));
}

}  // namespace rime
