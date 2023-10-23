#include <rime/component.h>
#include <rime/registry.h>
#include <rime_api.h>

#include "predictor.h"

using namespace rime;

static void rime_predict_initialize() {
  Registry& r = Registry::instance();
  r.Register("predictor", new PredictorComponent);
}

static void rime_predict_finalize() {}

RIME_REGISTER_MODULE(predict)
