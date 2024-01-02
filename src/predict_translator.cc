#include "predict_translator.h"

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

PredictTranslator::PredictTranslator(const Ticket& ticket,
                                     an<PredictEngine> predict_engine)
    : Translator(ticket), predict_engine_(predict_engine) {}

an<Translation> PredictTranslator::Query(const string& input,
                                         const Segment& segment) {
  if (!predict_engine_)
    return nullptr;
  if (predict_engine_->query().empty() || !segment.HasTag("prediction")) {
    return nullptr;
  }
  int num_candidates = predict_engine_->num_candidates();
  if (num_candidates > 0) {
    int max_candidates = predict_engine_->max_candidates();
    auto translation = New<FifoTranslation>();
    size_t end = segment.end;
    for (int i = 0; i < num_candidates; ++i) {
      translation->Append(New<SimpleCandidate>("prediction", end, end,
                                               predict_engine_->candidate(i)));
      if (max_candidates > 0 && i >= max_candidates)
        break;
    }
    return translation;
  }
  return nullptr;
}

PredictTranslatorComponent::PredictTranslatorComponent(
    an<PredictEngineComponent> engine_factory)
    : engine_factory_(engine_factory) {}

PredictTranslatorComponent::~PredictTranslatorComponent() {}

PredictTranslator* PredictTranslatorComponent::Create(const Ticket& ticket) {
  return new PredictTranslator(ticket, engine_factory_->GetInstance(ticket));
}

}  // namespace rime
