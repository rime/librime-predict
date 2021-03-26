#ifndef RIME_PREDICTOR_H_
#define RIME_PREDICTOR_H_

#include <rime/processor.h>

namespace rime {

class Predictor : public Processor {
 public:
  explicit Predictor(const Ticket& ticket) : Processor(ticket) {}

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;
};

}  // namespace rime

#endif  // RIME_PREDICTOR_H_
