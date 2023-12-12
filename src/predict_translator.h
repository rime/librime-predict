#ifndef RIME_PREDICT_TRANSLATOR_H_
#define RIME_PREDICT_TRANSLATOR_H_

#include <rime/translator.h>

namespace rime {

class Context;
class PredictEngine;
class PredictEngineComponent;

class PredictTranslator : public Translator {
 public:
  PredictTranslator(const Ticket& ticket, an<PredictEngine> predict_engine);

  an<Translation> Query(const string& input, const Segment& segment) override;

 private:
  an<PredictEngine> predict_engine_;
};

class PredictTranslatorComponent : public PredictTranslator::Component {
 public:
  explicit PredictTranslatorComponent(
      an<PredictEngineComponent> engine_factory);
  virtual ~PredictTranslatorComponent();

  PredictTranslator* Create(const Ticket& ticket) override;

 protected:
  an<PredictEngineComponent> engine_factory_;
};

}  // namespace rime

#endif  // RIME_PREDICT_TRANSLATOR_H_
