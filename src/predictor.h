#ifndef RIME_PREDICTOR_H_
#define RIME_PREDICTOR_H_

#include <rime/processor.h>

namespace rime {

class Context;
class PredictEngine;
class PredictEngineComponent;

class Predictor : public Processor {
 public:
  Predictor(const Ticket& ticket, an<PredictEngine> predict_engine);
  virtual ~Predictor();

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;

 protected:
  void OnContextUpdate(Context* ctx);
  void OnSelect(Context* ctx);
  void PredictAndUpdate(Context* ctx, const string& context_query);

 private:
  enum Action { kUnspecified, kSelect, kDelete };
  Action last_action_ = kUnspecified;
  bool self_updating_ = false;
  int iteration_counter_ = 0;  // times has been predicted

  an<PredictEngine> predict_engine_;
  connection select_connection_;
  connection context_update_connection_;
};

class PredictorComponent : public Predictor::Component {
 public:
  explicit PredictorComponent(an<PredictEngineComponent> engine_factory);
  virtual ~PredictorComponent();

  Predictor* Create(const Ticket& ticket) override;

 protected:
  an<PredictEngineComponent> engine_factory_;
};

}  // namespace rime

#endif  // RIME_PREDICTOR_H_
